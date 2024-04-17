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

#include <algorithm>

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
                          MakeBooleanChecker())
            .AddAttribute("UseAuxPhyCca",
                          "Whether the CCA performed in the last PIFS interval by a non-TX "
                          "capable aux PHY should be used when the main PHY ends switching to "
                          "the aux PHY's link to determine whether TX can start or not (and what "
                          "bandwidth can be used for transmission) independently of whether the "
                          "aux PHY bandwidth is smaller than the main PHY bandwidth or not.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedEmlsrManager::m_useAuxPhyCca),
                          MakeBooleanChecker())
            .AddAttribute("SwitchMainPhyBackDelay",
                          "Duration of the timer started in case of non-TX capable aux PHY (that "
                          "does not switch link) when medium is sensed busy during the PIFS "
                          "interval preceding/following the main PHY switch end. When the timer "
                          "expires, the main PHY is switched back to the primary link.",
                          TimeValue(MilliSeconds(5)),
                          MakeTimeAccessor(&AdvancedEmlsrManager::m_switchMainPhyBackDelay),
                          MakeTimeChecker());
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

std::pair<bool, Time>
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
                    return {false, phy->GetDelayUntilIdle()};
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
                    return {false, phy->GetDelayUntilIdle()};
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
                        return {false, phy->GetDelayUntilIdle()};
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
                    return {false, Max(macHdrDuration - timeSinceRxStart, Time{0})};
                }
                continue;
            }
        }
    }

    return {true, Time{0}};
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

std::pair<bool, Time>
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
        return {false, timeToCtsEnd};
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

    return {true, Time{0}};
}

void
AdvancedEmlsrManager::CheckNavAndCcaLastPifs(Ptr<WifiPhy> phy, uint8_t linkId, Ptr<QosTxop> edca)
{
    NS_LOG_FUNCTION(this << phy->GetPhyId() << linkId << edca->GetAccessCategory());

    const auto caManager = GetStaMac()->GetChannelAccessManager(linkId);
    const auto pifs = phy->GetSifs() + phy->GetSlot();

    const auto isBusy = caManager->IsBusy(); // check NAV and CCA on primary20
    // check CCA on the entire channel
    auto width = caManager->GetLargestIdlePrimaryChannel(pifs, Simulator::Now());

    if (!isBusy && width > 0)
    {
        // medium idle, start TXOP
        width = std::min(width, GetChannelForMainPhy(linkId).GetTotalWidth());

        // if this function is called at the end of the main PHY switch, it is executed before the
        // main PHY is connected to this link in order to use the CCA information of the aux PHY.
        // Schedule now the TXOP start so that we first connect the main PHY to this link.
        m_ccaLastPifs = Simulator::ScheduleNow([=, this]() {
            if (GetEhtFem(linkId)->HeFrameExchangeManager::StartTransmission(edca, width))
            {
                NotifyUlTxopStart(linkId);
            }
            else if (!m_switchAuxPhy)
            {
                // switch main PHY back to primary link if SwitchAuxPhy is false
                SwitchMainPhyBackToPrimaryLink(linkId);
            }
        });
    }
    else
    {
        // medium busy, restart channel access
        NS_LOG_DEBUG("Medium busy in the last PIFS interval");
        edca->NotifyChannelReleased(linkId); // to set access to NOT_REQUESTED
        edca->StartAccessAfterEvent(linkId,
                                    Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                                    Txop::CHECK_MEDIUM_BUSY);

        // the main PHY must stay for some time on this link to check if it gets channel access.
        // The timer is stopped if a DL or UL TXOP is started. When the timer expires, the main PHY
        // switches back to the preferred link if SwitchAuxPhy is false
        m_switchMainPhyBackEvent = Simulator::Schedule(m_switchMainPhyBackDelay, [this, linkId]() {
            if (!m_switchAuxPhy)
            {
                SwitchMainPhyBackToPrimaryLink(linkId);
            }
        });
    }
}

void
AdvancedEmlsrManager::DoNotifyIcfReceived(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    m_switchMainPhyBackEvent.Cancel();
    m_ccaLastPifs.Cancel();
}

void
AdvancedEmlsrManager::DoNotifyUlTxopStart(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    m_switchMainPhyBackEvent.Cancel();
    m_ccaLastPifs.Cancel();
}

bool
AdvancedEmlsrManager::RequestMainPhyToSwitch(uint8_t linkId, AcIndex aci)
{
    NS_LOG_FUNCTION(this << linkId << aci);

    // the aux PHY is not TX capable; check if main PHY has to switch to the aux PHY's link
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);
    const auto mainPhyLinkId = GetStaMac()->GetLinkForPhy(mainPhy);

    // if main PHY is not operating on a link, it is switching, hence do not request another switch
    if (!mainPhyLinkId.has_value())
    {
        NS_LOG_DEBUG("Main PHY is not operating on any link");
        return false;
    }

    // if the main PHY is already trying to get access on a link, do not request another switch
    if (m_ccaLastPifs.IsPending() || m_switchMainPhyBackEvent.IsPending())
    {
        NS_LOG_DEBUG("Main PHY is trying to get access on another link");
        return false;
    }

    switch (mainPhy->GetState()->GetState())
    {
    case WifiPhyState::IDLE:
        // proceed to try requesting main PHY to switch
        break;
    case WifiPhyState::CCA_BUSY:
        // if the main PHY is receiving the PHY header of a PPDU, we decide to proceed or give up
        // based on the AllowUlTxopInRx attribute
        if (mainPhy->IsReceivingPhyHeader() && !m_allowUlTxopInRx)
        {
            NS_LOG_DEBUG("Main PHY receiving PHY header and AllowUlTxopInRx is false");
            return false;
        }
        break;
    case WifiPhyState::RX:
        if (auto macHdr = GetEhtFem(*mainPhyLinkId)->GetReceivedMacHdr())
        {
            // information on the MAC header of the PSDU being received is available; if we cannot
            // use it or the main PHY is receiving an ICF, give up requesting main PHY to switch
            if (const auto& hdr = macHdr->get();
                !m_useNotifiedMacHdr ||
                (hdr.IsTrigger() && (hdr.GetAddr1().IsBroadcast() ||
                                     hdr.GetAddr1() == GetEhtFem(*mainPhyLinkId)->GetAddress())))
            {
                NS_LOG_DEBUG("Receiving an ICF or cannot use MAC header information");
                return false;
            }
        }
        // information on the MAC header of the PSDU being received is not available, we decide to
        // proceed or give up based on the AllowUlTxopInRx attribute
        else if (!m_allowUlTxopInRx)
        {
            NS_LOG_DEBUG("Receiving PSDU, no MAC header information, AllowUlTxopInRx is false");
            return false;
        }
        break;
    default:
        NS_LOG_DEBUG("Cannot request main PHY to switch when in state "
                     << mainPhy->GetState()->GetState());
        return false;
    }

    // request to switch main PHY if we expect the main PHY to get channel access on this link
    // more quickly, i.e., if ALL the ACs with queued frames (that can be transmitted on the link
    // on which the main PHY is currently operating) and with priority higher than or equal to
    // that of the AC for which Aux PHY gained TXOP have their backoff counter greater than the
    // channel switch delay plus PIFS

    auto requestSwitch = false;
    const auto now = Simulator::Now();

    for (const auto& [acIndex, ac] : wifiAcList)
    {
        if (auto edca = GetStaMac()->GetQosTxop(acIndex);
            acIndex >= aci && edca->HasFramesToTransmit(linkId))
        {
            requestSwitch = true;

            const auto backoffEnd =
                GetStaMac()->GetChannelAccessManager(*mainPhyLinkId)->GetBackoffEndFor(edca);
            NS_LOG_DEBUG("Backoff end for " << acIndex
                                            << " on primary link: " << backoffEnd.As(Time::US));

            if (backoffEnd <= now + mainPhy->GetChannelSwitchDelay() +
                                  GetStaMac()->GetWifiPhy(linkId)->GetPifs() &&
                edca->HasFramesToTransmit(*mainPhyLinkId))
            {
                requestSwitch = false;
                break;
            }
        }
    }

    return requestSwitch;
}

void
AdvancedEmlsrManager::SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci)
{
    NS_LOG_FUNCTION(this << linkId << aci);

    NS_ASSERT_MSG(!m_auxPhyTxCapable,
                  "This function should only be called if aux PHY is not TX capable");
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    if (RequestMainPhyToSwitch(linkId, aci))
    {
        const auto auxPhy = GetStaMac()->GetWifiPhy(linkId);
        const auto pifs = auxPhy->GetSifs() + auxPhy->GetSlot();

        // schedule actions to take based on CCA sensing for a PIFS
        if (m_useAuxPhyCca || GetChannelForAuxPhy(linkId).GetTotalWidth() >=
                                  GetChannelForMainPhy(linkId).GetTotalWidth())
        {
            // use aux PHY CCA in the last PIFS interval before main PHY switch end
            NS_LOG_DEBUG("Schedule CCA check at the end of main PHY switch");
            m_ccaLastPifs = Simulator::Schedule(mainPhy->GetChannelSwitchDelay(),
                                                &AdvancedEmlsrManager::CheckNavAndCcaLastPifs,
                                                this,
                                                auxPhy,
                                                linkId,
                                                GetStaMac()->GetQosTxop(aci));
        }
        else
        {
            // use main PHY CCA in the last PIFS interval after main PHY switch end
            NS_LOG_DEBUG("Schedule CCA check a PIFS after the end of main PHY switch");
            m_ccaLastPifs = Simulator::Schedule(mainPhy->GetChannelSwitchDelay() + pifs,
                                                &AdvancedEmlsrManager::CheckNavAndCcaLastPifs,
                                                this,
                                                mainPhy,
                                                linkId,
                                                GetStaMac()->GetQosTxop(aci));
        }

        // switch main PHY
        SwitchMainPhy(linkId, false, RESET_BACKOFF, DONT_REQUEST_ACCESS);
        return;
    }

    // Determine if and when we need to request channel access again for the aux PHY based on
    // the main PHY state.
    // Note that, if we have requested the main PHY to switch (above), the function has returned
    // and the EHT FEM will start a TXOP if medium is idle for a PIFS interval preceding/following
    // the end of the main PHY channel switch.
    // If the main PHY has been requested to switch by another aux PHY, this aux PHY will request
    // channel access again when we have completed the CCA assessment on the other link.
    // If the state is switching, CCA_BUSY or RX, then we request channel access again for the
    // aux PHY when the main PHY state is back to IDLE.
    // If the state is TX, it means that the main PHY is involved in a TXOP. Do nothing because
    // the channel access will be requested when unblocking links at the end of the TXOP.
    // If the state is IDLE, then either no AC has traffic to send or the backoff on the link
    // of the main PHY is shorter than the channel switch delay. In the former case, do
    // nothing because channel access will be triggered when new packets arrive; in the latter
    // case, do nothing because the main PHY will start a TXOP and at the end of such TXOP
    // links will be unblocked and the channel access requested on all links

    Time delay{};

    if (m_ccaLastPifs.IsPending() || m_switchMainPhyBackEvent.IsPending())
    {
        delay = std::max(Simulator::GetDelayLeft(m_ccaLastPifs),
                         Simulator::GetDelayLeft(m_switchMainPhyBackEvent));
    }
    else if (mainPhy->IsStateSwitching() || mainPhy->IsStateCcaBusy() || mainPhy->IsStateRx())
    {
        delay = mainPhy->GetDelayUntilIdle();
        NS_ASSERT(delay.IsStrictlyPositive());
    }

    NS_LOG_DEBUG("Main PHY state is " << mainPhy->GetState()->GetState());

    if (delay.IsZero())
    {
        NS_LOG_DEBUG("Do nothing");
        return;
    }

    auto edca = GetStaMac()->GetQosTxop(aci);
    edca->NotifyChannelReleased(linkId); // to set access to NOT_REQUESTED

    NS_LOG_DEBUG("Schedule channel access request on link "
                 << +linkId << " at time " << (Simulator::Now() + delay).As(Time::NS));
    Simulator::Schedule(delay, [=]() {
        edca->StartAccessAfterEvent(linkId,
                                    Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT,
                                    Txop::CHECK_MEDIUM_BUSY);
    });
}

} // namespace ns3
