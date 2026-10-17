/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "default-power-save-manager.h"

#include "sta-wifi-mac.h"
#include "wifi-mpdu.h"
#include "wifi-phy.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultPowerSaveManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultPowerSaveManager);

TypeId
DefaultPowerSaveManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::DefaultPowerSaveManager")
            .SetParent<PowerSaveManager>()
            .SetGroupName("Wifi")
            .AddConstructor<DefaultPowerSaveManager>()
            .AddAttribute("PsmTimeout",
                          "The length of the extra time during which the PHY is kept in active "
                          "state before being put to sleep state. If channel access is requested "
                          "(to transmit a frame) during such extra time, the PHY is kept in active "
                          "state to attempt to gain channel access and transmit. See the TGax "
                          "Simulation Scenarios document IEEE 802.11-14/0980r16.",
                          TimeValue(Time{0}),
                          MakeTimeAccessor(&DefaultPowerSaveManager::m_psmTimeout),
                          MakeTimeChecker(Time{0}))
            .AddAttribute("ListenAdvance",
                          "The amount of time the STA wakes up in advance prior to the Target "
                          "Beacon Transmission Time.",
                          TimeValue(Time{0}),
                          MakeTimeAccessor(&DefaultPowerSaveManager::m_listenAdvance),
                          MakeTimeChecker(Time{0}));
    return tid;
}

DefaultPowerSaveManager::DefaultPowerSaveManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultPowerSaveManager::~DefaultPowerSaveManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
DefaultPowerSaveManager::GoToSleepIfPossible(linkId_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    if (!GetStaMac()->IsAssociated())
    {
        NS_LOG_DEBUG("STA is not associated");
        return;
    }

    if (GetStaMac()->GetPmMode(linkId) == WIFI_PM_ACTIVE)
    {
        NS_LOG_DEBUG("STA on link " << +linkId << " is in active mode");
        return;
    }

    if (GetStaMac()->GetWifiPhy(linkId)->IsStateSleep())
    {
        NS_LOG_DEBUG("PHY operating on link " << +linkId << " is already in sleep state");
        return;
    }

    auto& staInfo = GetStaInfo(linkId);

    if (staInfo.beaconInterval.IsZero() || staInfo.lastBeaconTimestamp.IsZero())
    {
        NS_LOG_DEBUG("No Beacon received yet, cannot put PHY to sleep");
        return;
    }

    if (staInfo.pendingUnicast)
    {
        NS_LOG_DEBUG("AP has pending unicast frames, do not put PHY to sleep");
        return;
    }

    if (staInfo.pendingGroupcast)
    {
        NS_LOG_DEBUG("AP has pending groupcast frames, do not put PHY to sleep");
        return;
    }

    if (HasRequestedOrGainedChannel(linkId))
    {
        NS_LOG_DEBUG("Channel access requested or gained, do not put PHY to sleep");
        return;
    }

    if (m_sleepEvents[linkId].IsPending())
    {
        NS_LOG_DEBUG("Already scheduled a sleep event, do nothing");
        return;
    }

    auto putToSleep = [=, this] {
        NS_LOG_DEBUG("PHY operating on link " << +linkId << " is put to sleep");
        GetStaMac()->GetWifiPhy(linkId)->SetSleepMode(true);

        if (auto it = m_wakeUpEvents.find(linkId); it != m_wakeUpEvents.cend())
        {
            it->second.Cancel();
        }

        // schedule wakeup before next Beacon frame in listen interval periods
        NS_ASSERT(GetListenInterval() > 0);

        auto delay = (Simulator::Now() - staInfo.lastBeaconTimestamp) % staInfo.beaconInterval;
        delay = staInfo.beaconInterval - delay;
        delay += (GetListenInterval() - 1) * staInfo.beaconInterval;
        delay -= m_listenAdvance;
        delay = Max(delay, Time{0});

        NS_LOG_DEBUG("Scheduling PHY on link " << +linkId << " to wake up in "
                                               << delay.As(Time::US));
        m_wakeUpEvents[linkId] =
            Simulator::Schedule(delay, &WifiPhy::ResumeFromSleep, GetStaMac()->GetWifiPhy(linkId));
    };

    NS_LOG_DEBUG("Scheduling sleep state for PHY on link " << +linkId << "at time "
                                                           << Simulator::Now() + m_psmTimeout);
    m_sleepEvents[linkId] = Simulator::Schedule(m_psmTimeout, putToSleep);
}

void
DefaultPowerSaveManager::DoNotifyAssocCompleted()
{
    NS_LOG_FUNCTION(this);

    const auto linkIds = GetStaMac()->GetSetupLinkIds();
    for (const auto linkId : linkIds)
    {
        NS_LOG_DEBUG("PM mode for link " << +linkId << ": " << GetStaMac()->GetPmMode(linkId));
        if (GetStaMac()->GetPmMode(linkId) == WifiPowerManagementMode::WIFI_PM_POWERSAVE)
        {
            GoToSleepIfPossible(linkId);
        }
    }
}

void
DefaultPowerSaveManager::DoNotifyDisassociation()
{
    NS_LOG_FUNCTION(this);

    // base class has resumed PHYs from sleep; here we have to cancel timers
    for (auto& [linkId, event] : m_wakeUpEvents)
    {
        event.Cancel();
    }
    for (auto& [linkId, event] : m_sleepEvents)
    {
        event.Cancel();
    }
}

void
DefaultPowerSaveManager::DoNotifyPmModeChanged(WifiPowerManagementMode pmMode, linkId_t linkId)
{
    NS_LOG_FUNCTION(this << pmMode << linkId);

    if (pmMode == WifiPowerManagementMode::WIFI_PM_ACTIVE)
    {
        // base class has resumed the PHY from sleep; here we have to cancel the timers
        if (const auto it = m_wakeUpEvents.find(linkId); it != m_wakeUpEvents.cend())
        {
            it->second.Cancel();
        }
        if (const auto it = m_sleepEvents.find(linkId); it != m_sleepEvents.cend())
        {
            it->second.Cancel();
        }
    }
}

void
DefaultPowerSaveManager::DoNotifyReceivedBeacon(const MgtBeaconHeader& beacon, linkId_t linkId)
{
    NS_LOG_FUNCTION(this << beacon << linkId);

    if (GetStaMac()->GetPmMode(linkId) != WIFI_PM_POWERSAVE)
    {
        return;
    }

    if (GetStaInfo(linkId).pendingUnicast)
    {
        GetStaMac()->EnqueuePsPoll(linkId);
    }
    else
    {
        GoToSleepIfPossible(linkId);
    }
}

void
DefaultPowerSaveManager::DoNotifyReceivedFrameAfterPsPoll(Ptr<const WifiMpdu> mpdu, linkId_t linkId)
{
    NS_LOG_FUNCTION(this << mpdu << linkId);

    if (GetStaInfo(linkId).pendingUnicast)
    {
        NS_LOG_LOGIC("Waiting for more unicast frames; enqueue a PS-Poll frame");
        GetStaMac()->EnqueuePsPoll(linkId);
    }
    else
    {
        GoToSleepIfPossible(linkId);
    }
}

void
DefaultPowerSaveManager::DoNotifyReceivedGroupcast(Ptr<const WifiMpdu> mpdu, linkId_t linkId)
{
    NS_LOG_FUNCTION(this << mpdu << linkId);

    GoToSleepIfPossible(linkId);
}

void
DefaultPowerSaveManager::DoNotifyRequestAccess(Ptr<Txop> txop, linkId_t linkId)
{
    NS_LOG_FUNCTION(this << txop << linkId);

    m_sleepEvents[linkId].Cancel();

    // if channel access is being requested, it means that the MAC has frames to transmit on the
    // given link, hence wake up the PHY if it is in sleep state
    if (auto phy = GetStaMac()->GetWifiPhy(linkId); phy->IsStateSleep())
    {
        NS_LOG_DEBUG("Resume from sleep STA operating on link " << +linkId);
        phy->ResumeFromSleep();
        if (auto it = m_wakeUpEvents.find(linkId); it != m_wakeUpEvents.cend())
        {
            it->second.Cancel();
        }
    }
}

void
DefaultPowerSaveManager::DoNotifyChannelReleased(Ptr<Txop> txop, linkId_t linkId)
{
    NS_LOG_FUNCTION(this << txop << linkId);

    GoToSleepIfPossible(linkId);
}

void
DefaultPowerSaveManager::DoTxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << reason << *mpdu);

    if (mpdu->GetHeader().IsPsPoll())
    {
        auto addr2 = mpdu->GetHeader().GetAddr2();
        const auto linkId = GetStaMac()->GetLinkIdByAddress(addr2);
        NS_ASSERT_MSG(linkId.has_value(), addr2 << " is not a link address");

        NS_LOG_DEBUG("PS-Poll dropped. Give up polling the AP on link " << +linkId.value());

        if (auto& staInfo = GetStaInfo(*linkId); staInfo.pendingUnicast)
        {
            staInfo.pendingUnicast = false;
        }
    }

    for (const auto& id : GetStaMac()->GetLinkIds())
    {
        GoToSleepIfPossible(id);
    }
}

} // namespace ns3
