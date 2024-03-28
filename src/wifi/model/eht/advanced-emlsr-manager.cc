/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "advanced-emlsr-manager.h"

#include "eht-frame-exchange-manager.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdvancedEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(AdvancedEmlsrManager);

TypeId
AdvancedEmlsrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AdvancedEmlsrManager")
            .SetParent<DefaultEmlsrManager>()
            .SetGroupName("Wifi")
            .AddConstructor<AdvancedEmlsrManager>()
            .AddAttribute("UseNotifiedMacHdr",
                          "Whether to use the information about the MAC header of the MPDU "
                          "being received, if notified by the PHY.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_useNotifiedMacHdr),
                          MakeBooleanChecker())
            .AddAttribute("AllowUlTxopInRx",
                          "Whether a (main or aux) PHY is allowed to start an UL TXOP if "
                          "another PHY is receiving a PPDU (possibly starting a DL TXOP). "
                          "If this attribute is true, the PPDU may be dropped.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_allowUlTxopInRx),
                          MakeBooleanChecker())
            .AddAttribute("InterruptSwitch",
                          "Whether the main PHY can be interrupted while switching to start "
                          "switching to another link.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_interruptSwitching),
                          MakeBooleanChecker());
    return tid;
}

AdvancedEmlsrManager::AdvancedEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

AdvancedEmlsrManager::~AdvancedEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
AdvancedEmlsrManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (auto phy : GetStaMac()->GetDevice()->GetPhys())
    {
        phy->TraceDisconnectWithoutContext(
            "PhyRxMacHeaderEnd",
            MakeCallback(&AdvancedEmlsrManager::ReceivedMacHdr, this).Bind(phy));
    }
    DefaultEmlsrManager::DoDispose();
}

void
AdvancedEmlsrManager::DoSetWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);

    for (auto phy : GetStaMac()->GetDevice()->GetPhys())
    {
        phy->TraceConnectWithoutContext(
            "PhyRxMacHeaderEnd",
            MakeCallback(&AdvancedEmlsrManager::ReceivedMacHdr, this).Bind(phy));
    }
}

Time
AdvancedEmlsrManager::DoGetDelayUntilAccessRequest(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // prevent or allow an UL TXOP depending on whether another PHY is receiving a PPDU
    for (const auto id : GetStaMac()->GetLinkIds())
    {
        if (id != linkId && GetStaMac()->IsEmlsrLink(id))
        {
            auto phy = GetStaMac()->GetWifiPhy(id);

            if (auto macHdr = GetEhtFem(id)->GetReceivedMacHdr(); macHdr && m_useNotifiedMacHdr)
            {
                NS_ASSERT(phy &&
                          phy->GetState()->GetLastTime({WifiPhyState::RX}) == Simulator::Now());
                // we are receiving the MAC payload of a PSDU; if the PSDU being received on
                // another link is an ICF, give up the TXOP and restart channel access at the
                // end of PSDU reception. Note that we cannot be sure that the PSDU being received
                // is an ICF addressed to us until we receive the entire PSDU
                if (const auto& hdr = macHdr->get();
                    hdr.IsTrigger() &&
                    (hdr.GetAddr1().IsBroadcast() || hdr.GetAddr1() == GetEhtFem(id)->GetAddress()))
                {
                    return phy->GetDelayUntilIdle();
                }
                continue;
            }

            if (phy && phy->IsReceivingPhyHeader())
            {
                // we don't know yet the type of the frame being received; prevent or allow
                // the UL TXOP based on user configuration
                if (!m_allowUlTxopInRx)
                {
                    // retry channel access after the end of the current PHY header field
                    return phy->GetDelayUntilIdle();
                }
                continue;
            }

            if (phy && phy->IsStateRx())
            {
                // we don't know yet the type of the frame being received; prevent or allow
                // the UL TXOP based on user configuration
                if (!m_allowUlTxopInRx)
                {
                    if (!m_useNotifiedMacHdr)
                    {
                        // restart channel access at the end of PSDU reception
                        return phy->GetDelayUntilIdle();
                    }

                    // retry channel access after the expected end of the MAC header reception
                    auto macHdrSize = WifiMacHeader(WIFI_MAC_QOSDATA).GetSerializedSize() +
                                      4 /* A-MPDU subframe header length */;
                    auto ongoingRxInfo = GetEhtFem(id)->GetOngoingRxInfo();
                    // if a PHY is in RX state, it should have info about received MAC header.
                    // The exception is represented by this situation:
                    // - an aux PHY is disconnected from the MAC stack because the main PHY is
                    //   operating on its link
                    // - the main PHY notifies the MAC header info to the FEM and then leaves the
                    //   link (e.g., because it recognizes that the MPDU is not addressed to the
                    //   EMLSR client). Disconnecting the main PHY from the MAC stack causes the
                    //   MAC header info to be discarded by the FEM
                    // - the aux PHY is re-connected to the MAC stack and is still in RX state
                    //   when the main PHY gets channel access on another link (and we get here)
                    if (!ongoingRxInfo.has_value())
                    {
                        NS_ASSERT_MSG(phy != GetStaMac()->GetDevice()->GetPhy(GetMainPhyId()),
                                      "Main PHY should have MAC header info when in RX state");
                        // we are in the situation described above; if the MPDU being received
                        // by the aux PHY is not addressed to the EMLSR client, we can ignore it
                        continue;
                    }
                    const auto& txVector = ongoingRxInfo->get().txVector;
                    if (txVector.IsMu())
                    {
                        // this is not an ICF, ignore it
                        continue;
                    }
                    auto macHdrDuration = DataRate(txVector.GetMode().GetDataRate(txVector))
                                              .CalculateBytesTxTime(macHdrSize);
                    const auto timeSinceRxStart =
                        Simulator::Now() - phy->GetState()->GetLastTime({WifiPhyState::CCA_BUSY});
                    return Max(macHdrDuration - timeSinceRxStart, Time{0});
                }
                continue;
            }
        }
    }

    return Time{0};
}

void
AdvancedEmlsrManager::ReceivedMacHdr(Ptr<WifiPhy> phy,
                                     const WifiMacHeader& macHdr,
                                     const WifiTxVector& txVector,
                                     Time psduDuration)
{
    auto linkId = GetStaMac()->GetLinkForPhy(phy);
    if (!linkId.has_value())
    {
        return;
    }
    NS_LOG_FUNCTION(this << *linkId << macHdr << txVector << psduDuration.As(Time::MS));

    auto& ongoingTxopEnd = GetEhtFem(*linkId)->GetOngoingTxopEndEvent();

    if (m_useNotifiedMacHdr && ongoingTxopEnd.IsPending() &&
        macHdr.GetAddr1() != GetEhtFem(*linkId)->GetAddress() && !macHdr.GetAddr1().IsBroadcast() &&
        !(macHdr.IsCts() && macHdr.GetAddr1() == GetEhtFem(*linkId)->GetBssid() /* CTS-to-self */))
    {
        // the EMLSR client is no longer involved in the TXOP and switching to listening mode
        ongoingTxopEnd.Cancel();
        // this method is a callback connected to the PhyRxMacHeaderEnd trace source of WifiPhy
        // and is called within a for loop that executes all the callbacks. The call to NotifyTxop
        // below leads the main PHY to be connected back to the primary link, thus
        // the ResetPhy() method of the FEM on the non-primary link is called, which disconnects
        // another callback (FEM::ReceivedMacHdr) from the PhyRxMacHeaderEnd trace source of
        // the main PHY, thus invalidating the list of callbacks on which the for loop iterates.
        // Hence, schedule the call to NotifyTxopEnd to execute it outside such for loop.
        Simulator::ScheduleNow(&AdvancedEmlsrManager::NotifyTxopEnd, this, *linkId, false, false);
    }
}

void
AdvancedEmlsrManager::DoNotifyTxopEnd(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    if (m_switchAuxPhy && (!mainPhy->IsStateSwitching() || !m_interruptSwitching))
    {
        NS_LOG_DEBUG("SwitchAuxPhy true, nothing to do");
        return;
    }

    if (!m_switchAuxPhy && !m_auxPhyToReconnect)
    {
        NS_LOG_DEBUG("SwitchAuxPhy false, nothing to do");
        return;
    }

    // we get here if:
    // - SwitchAuxPhy is true, the main PHY is switching and switching can be interrupted
    // or
    // - SwitchAuxPhy is false and there is an aux PHY to reconnect

    // Note that the main PHY may be switching at the end of a TXOP when, e.g., the main PHY
    // starts switching to a link on which an aux PHY gained a TXOP and sent an RTS, but the CTS
    // is not received and the UL TXOP ends before the main PHY channel switch is completed.
    // In such cases, wait until the main PHY channel switch is completed (unless the channel
    // switching can be interrupted) before requesting a new channel switch. Given that the
    // TXOP ended, the event to put the aux PHY to sleep can be cancelled.
    // Backoff shall not be reset on the link left by the main PHY because a TXOP ended and
    // a new backoff value must be generated.
    m_auxPhyToSleepEvent.Cancel();

    if (m_switchAuxPhy || !mainPhy->IsStateSwitching() || m_interruptSwitching)
    {
        NS_ASSERT_MSG(
            !m_switchAuxPhy || m_mainPhySwitchInfo.end >= Simulator::Now(),
            "Aux PHY next link ID should have a value when interrupting a main PHY switch");
        uint8_t nextLinkId = m_switchAuxPhy ? m_mainPhySwitchInfo.from : GetMainPhyId();
        SwitchMainPhy(nextLinkId, false, DONT_RESET_BACKOFF, REQUEST_ACCESS);
    }
    else
    {
        // delay link switch until current channel switching is completed
        Simulator::Schedule(mainPhy->GetDelayUntilIdle(), [=, this]() {
            // request the main PHY to switch back to the primary link only if in the meantime
            // no TXOP started on another link (which will require the main PHY to switch link)
            if (!GetEhtFem(linkId)->UsingOtherEmlsrLink())
            {
                SwitchMainPhy(GetMainPhyId(), false, DONT_RESET_BACKOFF, REQUEST_ACCESS);
            }
        });
    }
}

Time
AdvancedEmlsrManager::GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    if (!m_interruptSwitching)
    {
        return DefaultEmlsrManager::GetDelayUnlessMainPhyTakesOverUlTxop(linkId);
    }

    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);
    auto state = mainPhy->GetState()->GetState();

    NS_ABORT_MSG_UNLESS(state == WifiPhyState::SWITCHING || state == WifiPhyState::RX ||
                            state == WifiPhyState::IDLE || state == WifiPhyState::CCA_BUSY,
                        "Main PHY cannot be in state " << state);

    auto timeToCtsEnd = GetTimeToCtsEnd(linkId);
    auto switchingTime = mainPhy->GetChannelSwitchDelay();

    if (switchingTime > timeToCtsEnd)
    {
        // switching takes longer than RTS/CTS exchange, release channel
        NS_LOG_DEBUG("Not enough time for main PHY to switch link (main PHY state: "
                     << mainPhy->GetState()->GetState() << ")");
        // retry channel access when the CTS was expected to be received
        return timeToCtsEnd;
    }

    // TXOP can be started, schedule main PHY switch. Main PHY shall terminate the channel switch
    // at the end of CTS reception
    const auto delay = timeToCtsEnd - switchingTime;

    NS_ASSERT(delay.IsPositive());
    NS_LOG_DEBUG("Schedule main Phy switch in " << delay.As(Time::US));
    m_ulMainPhySwitch[linkId] = Simulator::Schedule(delay,
                                                    &AdvancedEmlsrManager::SwitchMainPhy,
                                                    this,
                                                    linkId,
                                                    false,
                                                    RESET_BACKOFF,
                                                    DONT_REQUEST_ACCESS);

    return Time{0};
}

void
AdvancedEmlsrManager::SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci)
{
    NS_LOG_FUNCTION(this << linkId << aci);

    NS_ASSERT_MSG(!m_auxPhyTxCapable,
                  "This function should only be called if aux PHY is not TX capable");

    // the aux PHY is not TX capable; check if main PHY has to switch to the aux PHY's link
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    // if the main PHY is idle, switch main PHY if we expect the main PHY to get channel access on
    // this link more quicky, i.e., if ALL the ACs with queued frames and with priority higher than
    // or equal to that of the AC for which Aux PHY gained TXOP have their backoff counter greater
    // than the channel switch delay plus PIFS

    auto requestSwitch = false;

    if (mainPhy->IsStateIdle())
    {
        auto mainPhyLinkId = GetStaMac()->GetLinkForPhy(mainPhy);
        NS_ASSERT(mainPhyLinkId.has_value());

        // update backoff on main PHY link for all ACs
        GetStaMac()
            ->GetChannelAccessManager(*mainPhyLinkId)
            ->NeedBackoffUponAccess(GetStaMac()->GetQosTxop(AC_BE),
                                    Txop::HAD_FRAMES_TO_TRANSMIT,
                                    Txop::CHECK_MEDIUM_BUSY);

        for (const auto& [acIndex, ac] : wifiAcList)
        {
            if (auto edca = GetStaMac()->GetQosTxop(acIndex);
                acIndex >= aci && edca->HasFramesToTransmit(linkId))
            {
                requestSwitch = true;

                auto backoffEnd =
                    GetStaMac()->GetChannelAccessManager(*mainPhyLinkId)->GetBackoffEndFor(edca);
                NS_LOG_DEBUG("Backoff end for " << acIndex
                                                << " on primary link: " << backoffEnd.As(Time::US));

                if (backoffEnd <= Simulator::Now() + mainPhy->GetChannelSwitchDelay() +
                                      GetStaMac()->GetWifiPhy(linkId)->GetPifs() &&
                    edca->HasFramesToTransmit(*mainPhyLinkId))
                {
                    requestSwitch = false;
                    break;
                }
            }
        }
    }

    if ((mainPhy->IsStateCcaBusy() && !mainPhy->IsReceivingPhyHeader()) ||
        (mainPhy->IsStateIdle() && requestSwitch))
    {
        // switch main PHY
        SwitchMainPhy(linkId, false, RESET_BACKOFF, REQUEST_ACCESS);

        return;
    }

    // Determine if and when we need to request channel access again for the aux PHY based on
    // the main PHY state.
    // Note that, if we have requested the main PHY to switch (above), the function has returned
    // and the EHT FEM will start a TXOP if the medium is idle for a PIFS interval following
    // the end of the main PHY channel switch.
    // If the state is switching, but we have not requested the main PHY to switch, then we
    // request channel access again for the aux PHY a PIFS after that the main PHY state is back
    // to IDLE (to avoid stealing the main PHY from the non-primary link which the main PHY is
    // switching to), and then we will determine if the main PHY has to switch link.
    // If the state is CCA_BUSY, the medium is busy but the main PHY is not receiving a PPDU.
    // In this case, we request channel access again for the aux PHY a PIFS after that the main
    // PHY state is back to IDLE, and then we will determine if the main PHY has to switch link.
    // If the state is TX or RX, it means that the main PHY is involved in a TXOP. In this
    // case, do nothing because the channel access will be requested when unblocking links
    // at the end of the TXOP.
    // If the state is IDLE, then either no AC has traffic to send or the backoff on the link
    // of the main PHY is shorter than the channel switch delay. In the former case, do
    // nothing because channel access will be triggered when new packets arrive; in the latter
    // case, do nothing because the main PHY will start a TXOP and at the end of such TXOP
    // links will be unblocked and the channel access requested on all links

    if (!mainPhy->IsStateSwitching() && !mainPhy->IsStateCcaBusy())
    {
        NS_LOG_DEBUG("Main PHY state is " << mainPhy->GetState()->GetState() << ". Do nothing");
        return;
    }

    auto delay = mainPhy->GetDelayUntilIdle();
    NS_ASSERT(delay.IsStrictlyPositive());
    delay += mainPhy->GetSifs() + mainPhy->GetSlot();

    NS_LOG_DEBUG("Main PHY state is " << mainPhy->GetState()->GetState()
                                      << ". Schedule channel access request on link " << +linkId
                                      << " at time " << (Simulator::Now() + delay).As(Time::NS));
    Simulator::Schedule(delay, [=, this]() {
        for (const auto& [aci, ac] : wifiAcList)
        {
            auto edca = GetStaMac()->GetQosTxop(aci);
            if (edca->GetAccessStatus(linkId) != Txop::REQUESTED &&
                edca->HasFramesToTransmit(linkId))
            {
                NS_LOG_DEBUG("Request channel access on link " << +linkId << " for " << aci);
                GetStaMac()->GetChannelAccessManager(linkId)->RequestAccess(edca);
            }
        }
    });
}

} // namespace ns3
