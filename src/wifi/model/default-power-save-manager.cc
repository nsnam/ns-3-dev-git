/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "default-power-save-manager.h"

#include "sta-wifi-mac.h"
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
    static TypeId tid = TypeId("ns3::DefaultPowerSaveManager")
                            .SetParent<PowerSaveManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<DefaultPowerSaveManager>();
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
DefaultPowerSaveManager::GoToSleepIfPossible(uint8_t linkId)
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

    // if nothing to do, put PHY to sleep
    if (!staInfo.pendingUnicast && !staInfo.pendingGroupcast && !HasFramesToSend(linkId))
    {
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

        NS_LOG_DEBUG("Scheduling PHY on link " << +linkId << " to wake up in "
                                               << delay.As(Time::US));
        m_wakeUpEvents[linkId] =
            Simulator::Schedule(delay, &WifiPhy::ResumeFromSleep, GetStaMac()->GetWifiPhy(linkId));
    }
}

void
DefaultPowerSaveManager::DoNotifyAssocCompleted()
{
    NS_LOG_FUNCTION(this);
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
}

void
DefaultPowerSaveManager::DoNotifyReceivedBeacon(const MgtBeaconHeader& beacon, uint8_t linkId)
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

} // namespace ns3
