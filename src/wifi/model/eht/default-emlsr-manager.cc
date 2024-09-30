/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "default-emlsr-manager.h"

#include "eht-frame-exchange-manager.h"

#include "ns3/boolean.h"
#include "ns3/channel-access-manager.h"
#include "ns3/log.h"
#include "ns3/qos-txop.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultEmlsrManager);

TypeId
DefaultEmlsrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DefaultEmlsrManager")
            .SetParent<EmlsrManager>()
            .SetGroupName("Wifi")
            .AddConstructor<DefaultEmlsrManager>()
            .AddAttribute("SwitchAuxPhy",
                          "Whether Aux PHY should switch channel to operate on the link on which "
                          "the Main PHY was operating before moving to the link of the Aux PHY. "
                          "Note that, if the Aux PHY does not switch channel, the main PHY will "
                          "switch back to its previous link once the TXOP terminates (otherwise, "
                          "no PHY will be listening on that EMLSR link).",
                          BooleanValue(true),
                          MakeBooleanAccessor(&DefaultEmlsrManager::m_switchAuxPhy),
                          MakeBooleanChecker());
    return tid;
}

DefaultEmlsrManager::DefaultEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultEmlsrManager::~DefaultEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
DefaultEmlsrManager::DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);
}

uint8_t
DefaultEmlsrManager::GetLinkToSendEmlOmn()
{
    NS_LOG_FUNCTION(this);
    auto linkId = GetStaMac()->GetLinkForPhy(m_mainPhyId);
    NS_ASSERT_MSG(linkId, "Link on which the main PHY is operating not found");
    return *linkId;
}

std::optional<uint8_t>
DefaultEmlsrManager::ResendNotification(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this);
    auto linkId = GetStaMac()->GetLinkForPhy(m_mainPhyId);
    NS_ASSERT_MSG(linkId, "Link on which the main PHY is operating not found");
    return *linkId;
}

void
DefaultEmlsrManager::NotifyEmlsrModeChanged()
{
    NS_LOG_FUNCTION(this);
}

void
DefaultEmlsrManager::NotifyMainPhySwitch(std::optional<uint8_t> currLinkId,
                                         uint8_t nextLinkId,
                                         Ptr<WifiPhy> auxPhy,
                                         Time duration)
{
    NS_LOG_FUNCTION(this << (currLinkId ? std::to_string(*currLinkId) : "") << nextLinkId << auxPhy
                         << duration.As(Time::US));

    if (m_switchAuxPhy)
    {
        // cancel any previously requested aux PHY switch
        m_auxPhySwitchEvent.Cancel();

        if (nextLinkId == m_mainPhySwitchInfo.from)
        {
            // the main PHY is now switching to the link where it is coming from; nothing else
            // needs to be done
            return;
        }

        // schedule Aux PHY switch so that it operates on the link on which the main PHY was
        // operating
        NS_LOG_DEBUG("Aux PHY (" << auxPhy << ") operating on link " << +nextLinkId
                                 << " will switch to link " << +m_mainPhySwitchInfo.from << " in "
                                 << duration.As(Time::US));
        SwitchAuxPhyAfterMainPhy(auxPhy, nextLinkId, m_mainPhySwitchInfo.from, duration);
        return;
    }

    if (currLinkId.has_value() && currLinkId != GetMainPhyId())
    {
        // the main PHY is leaving an auxiliary link, hence an aux PHY needs to be reconnected
        NS_ASSERT_MSG(
            m_auxPhyToReconnect,
            "There should be an aux PHY to reconnect when the main PHY leaves an auxiliary link");

        // the Aux PHY is not actually switching (hence no switching delay)
        GetStaMac()->NotifySwitchingEmlsrLink(m_auxPhyToReconnect, *currLinkId, Seconds(0));
        SetCcaEdThresholdOnLinkSwitch(m_auxPhyToReconnect, *currLinkId);
    }

    // if currLinkId has no value, it means that the main PHY switch is interrupted, hence reset
    // the aux PHY to reconnect. Doing so when the main PHY is leaving the preferred link makes
    // no harm (the aux PHY to reconnect is set below), thus no need to add an 'if' condition
    m_auxPhyToReconnect = nullptr;

    if (nextLinkId != GetMainPhyId())
    {
        // the main PHY is moving to an auxiliary link and the aux PHY does not switch link
        m_auxPhyToReconnect = auxPhy;
    }
}

void
DefaultEmlsrManager::SwitchAuxPhyAfterMainPhy(Ptr<WifiPhy> auxPhy,
                                              uint8_t currLinkId,
                                              uint8_t nextLinkId,
                                              Time duration)
{
    NS_LOG_FUNCTION(this << auxPhy << currLinkId << nextLinkId << duration.As(Time::US));

    if (duration.IsStrictlyPositive())
    {
        auto lambda = [=, this]() {
            if (GetStaMac()->GetWifiPhy(currLinkId) == auxPhy)
            {
                // the aux PHY is still operating on the link, likely because it is receiving a
                // PPDU and connecting the main PHY to the link has been postponed
                const auto [maybeIcf, extension] = CheckPossiblyReceivingIcf(currLinkId);
                if (maybeIcf && extension.IsStrictlyPositive())
                {
                    NS_LOG_DEBUG("Switching aux PHY to link " << +nextLinkId << " is postponed by "
                                                              << extension.As(Time::US));
                    SwitchAuxPhyAfterMainPhy(auxPhy, currLinkId, nextLinkId, extension);
                    return;
                }
            }
            const auto isSleeping = auxPhy->IsStateSleep();
            if (isSleeping)
            {
                // if the aux PHY is sleeping, it cannot switch channel
                auxPhy->ResumeFromSleep();
            }
            SwitchAuxPhy(auxPhy, currLinkId, nextLinkId);
            if (isSleeping)
            {
                // sleep mode will be postponed until the end of channel switch
                auxPhy->SetSleepMode(true);
            }
        };

        m_auxPhySwitchEvent = Simulator::Schedule(duration, lambda);
    }
    else
    {
        SwitchAuxPhy(auxPhy, currLinkId, nextLinkId);
    }
}

std::pair<bool, Time>
DefaultEmlsrManager::DoGetDelayUntilAccessRequest(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
    return {true, Time{0}}; // start the TXOP
}

void
DefaultEmlsrManager::DoNotifyIcfReceived(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
}

void
DefaultEmlsrManager::DoNotifyUlTxopStart(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);
}

void
DefaultEmlsrManager::DoNotifyTxopEnd(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // switch main PHY to the previous link, if needed
    if (!m_switchAuxPhy)
    {
        const auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);
        const auto delay = mainPhy->IsStateSwitching() ? mainPhy->GetDelayUntilIdle() : Time{0};
        SwitchMainPhyBackToPreferredLink(linkId, EmlsrTxopEndedTrace(delay));
    }
}

void
DefaultEmlsrManager::SwitchMainPhyBackToPreferredLink(uint8_t linkId,
                                                      EmlsrMainPhySwitchTrace&& traceInfo)
{
    NS_LOG_FUNCTION(this << linkId << traceInfo.GetName());

    NS_ABORT_MSG_IF(m_switchAuxPhy, "This method can only be called when SwitchAuxPhy is false");

    if (!m_auxPhyToReconnect)
    {
        return;
    }

    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);

    // the main PHY may be switching at the end of a TXOP when, e.g., the main PHY starts
    // switching to a link on which an aux PHY gained a TXOP and sent an RTS, but the CTS
    // is not received and the UL TXOP ends before the main PHY channel switch is completed.
    // In such cases, wait until the main PHY channel switch is completed before requesting
    // a new channel switch.
    // Backoff shall not be reset on the link left by the main PHY because a TXOP ended and
    // a new backoff value must be generated.
    if (!mainPhy->IsStateSwitching())
    {
        SwitchMainPhy(GetMainPhyId(),
                      false,
                      DONT_RESET_BACKOFF,
                      REQUEST_ACCESS,
                      std::forward<EmlsrMainPhySwitchTrace>(traceInfo));
    }
    else
    {
        Simulator::Schedule(mainPhy->GetDelayUntilIdle(), [=, info = traceInfo.Clone(), this]() {
            // request the main PHY to switch back to the preferred link only if
            // in the meantime no TXOP started on another link (which will
            // require the main PHY to switch link)
            if (!GetEhtFem(linkId)->UsingOtherEmlsrLink())
            {
                SwitchMainPhy(GetMainPhyId(),
                              false,
                              DONT_RESET_BACKOFF,
                              REQUEST_ACCESS,
                              std::move(*info));
            }
        });
    }
}

void
DefaultEmlsrManager::SwitchMainPhyIfTxopGainedByAuxPhy(uint8_t linkId, AcIndex aci)
{
    NS_LOG_FUNCTION(this << linkId << aci);
    NS_LOG_DEBUG("Do nothing, aux PHY is not TX capable");
}

Time
DefaultEmlsrManager::GetTimeToCtsEnd(uint8_t linkId) const
{
    NS_LOG_FUNCTION(this << linkId);

    const auto stationManager = GetStaMac()->GetWifiRemoteStationManager(linkId);
    const auto bssid = GetEhtFem(linkId)->GetBssid();
    const auto allowedWidth = GetEhtFem(linkId)->GetAllowedWidth();

    return GetTimeToCtsEnd(linkId, stationManager->GetRtsTxVector(bssid, allowedWidth));
}

Time
DefaultEmlsrManager::GetTimeToCtsEnd(uint8_t linkId, const WifiTxVector& rtsTxVector) const
{
    NS_LOG_FUNCTION(this << linkId << rtsTxVector);

    auto phy = GetStaMac()->GetWifiPhy(linkId);
    NS_ASSERT_MSG(phy, "No PHY operating on link " << +linkId);

    const auto stationManager = GetStaMac()->GetWifiRemoteStationManager(linkId);
    const auto bssid = GetEhtFem(linkId)->GetBssid();
    const auto ctsTxVector = stationManager->GetCtsTxVector(bssid, rtsTxVector.GetMode());

    const auto rtsTxTime =
        WifiPhy::CalculateTxDuration(GetRtsSize(), rtsTxVector, phy->GetPhyBand());
    const auto ctsTxTime =
        WifiPhy::CalculateTxDuration(GetCtsSize(), ctsTxVector, phy->GetPhyBand());

    // the main PHY shall terminate the channel switch at the end of CTS reception;
    // the time remaining to the end of CTS reception includes two propagation delays
    return rtsTxTime + phy->GetSifs() + ctsTxTime + MicroSeconds(2 * MAX_PROPAGATION_DELAY_USEC);
}

std::pair<bool, Time>
DefaultEmlsrManager::GetDelayUnlessMainPhyTakesOverUlTxop(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);
    const auto timeToCtsEnd = GetTimeToCtsEnd(linkId);
    auto switchingTime = mainPhy->GetChannelSwitchDelay();

    switch (mainPhy->GetState()->GetState())
    {
    case WifiPhyState::SWITCHING:
        // the main PHY is switching (to another link), hence the remaining time to
        // the end of the current channel switch needs to be added up
        switchingTime += mainPhy->GetDelayUntilIdle();
        [[fallthrough]];
    case WifiPhyState::RX:
    case WifiPhyState::IDLE:
    case WifiPhyState::CCA_BUSY:
        if (switchingTime > timeToCtsEnd)
        {
            // switching takes longer than RTS/CTS exchange, release channel
            NS_LOG_DEBUG("Not enough time for main PHY to switch link (main PHY state: "
                         << mainPhy->GetState()->GetState() << ")");
            // retry channel access when the CTS was expected to be received
            return {false, timeToCtsEnd};
        }
        break;
    default:
        NS_ABORT_MSG("Main PHY cannot be in state " << mainPhy->GetState()->GetState());
    }

    // TXOP can be started, main PHY will be scheduled to switch by NotifyRtsSent as soon as the
    // transmission of the RTS is notified
    m_switchMainPhyOnRtsTx[linkId] = Simulator::Now();

    return {true, Time{0}};
}

void
DefaultEmlsrManager::NotifyRtsSent(uint8_t linkId,
                                   Ptr<const WifiPsdu> rts,
                                   const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *rts << txVector);

    const auto it = m_switchMainPhyOnRtsTx.find(linkId);

    if (it == m_switchMainPhyOnRtsTx.cend() || it->second != Simulator::Now())
    {
        // No request for main PHY to switch or obsolete request
        return;
    }

    // Main PHY shall terminate the channel switch at the end of CTS reception
    auto mainPhy = GetStaMac()->GetDevice()->GetPhy(m_mainPhyId);
    const auto delay = GetTimeToCtsEnd(linkId, txVector) - mainPhy->GetChannelSwitchDelay();
    NS_ASSERT_MSG(delay.IsPositive(),
                  "RTS is being sent, but not enough time for main PHY to switch");

    NS_LOG_DEBUG("Schedule main Phy switch in " << delay.As(Time::US));
    m_ulMainPhySwitch[linkId] = Simulator::Schedule(delay, [=, this]() {
        SwitchMainPhy(linkId,
                      false,
                      RESET_BACKOFF,
                      DONT_REQUEST_ACCESS,
                      EmlsrUlTxopRtsSentByAuxPhyTrace{});
    });

    m_switchMainPhyOnRtsTx.erase(it);
}

} // namespace ns3
