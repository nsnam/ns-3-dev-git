/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "power-save-manager.h"

#include "mgt-headers.h"
#include "sta-wifi-mac.h"
#include "txop.h"
#include "wifi-mpdu.h"
#include "wifi-phy.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PowerSaveManager");

NS_OBJECT_ENSURE_REGISTERED(PowerSaveManager);

TypeId
PowerSaveManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PowerSaveManager").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

PowerSaveManager::~PowerSaveManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
PowerSaveManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_staMac->TraceDisconnectWithoutContext("DroppedMpdu",
                                            MakeCallback(&PowerSaveManager::TxDropped, this));
    m_staMac = nullptr;
    Object::DoDispose();
}

void
PowerSaveManager::SetWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_staMac = mac;
    m_staMac->TraceConnectWithoutContext("DroppedMpdu",
                                         MakeCallback(&PowerSaveManager::TxDropped, this));
}

Ptr<StaWifiMac>
PowerSaveManager::GetStaMac() const
{
    return m_staMac;
}

PowerSaveManager::StaInfo&
PowerSaveManager::GetStaInfo(uint8_t linkId)
{
    NS_ASSERT(m_staInfo.contains(linkId));
    return m_staInfo.at(linkId);
}

void
PowerSaveManager::NotifyAssocCompleted()
{
    NS_LOG_FUNCTION(this);

    m_staInfo.clear();
    const auto linkIds = m_staMac->GetSetupLinkIds();
    for (const auto linkId : linkIds)
    {
        m_staInfo[linkId] = StaInfo{};
    }
}

void
PowerSaveManager::NotifyDisassociation()
{
    NS_LOG_FUNCTION(this);

    m_staInfo.clear();
    for (const auto linkId : m_staMac->GetLinkIds())
    {
        auto phy = m_staMac->GetWifiPhy(linkId);
        if (phy && phy->IsStateSleep())
        {
            phy->ResumeFromSleep();
        }
    }
}

void
PowerSaveManager::NotifyPmModeChanged(WifiPowerManagementMode pmMode, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << pmMode << linkId);

    if (pmMode == WifiPowerManagementMode::WIFI_PM_ACTIVE)
    {
        auto phy = m_staMac->GetWifiPhy(linkId);
        if (phy && phy->IsStateSleep())
        {
            phy->ResumeFromSleep();
        }
    }
}

void
PowerSaveManager::NotifyReceivedBeacon(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    NS_ASSERT(mpdu->GetHeader().IsBeacon());
    NS_ASSERT(mpdu->GetHeader().GetAddr2() == m_staMac->GetBssid(linkId));

    MgtBeaconHeader beacon;
    mpdu->GetPacket()->PeekHeader(beacon);

    auto& staInfo = GetStaInfo(linkId);
    staInfo.beaconInterval = MicroSeconds(beacon.m_beaconInterval);
    staInfo.lastBeaconTimestamp = MicroSeconds(beacon.GetTimestamp());

    auto& tim = beacon.Get<Tim>();
    staInfo.pendingUnicast = tim->HasAid(m_staMac->GetAssociationId());
    staInfo.pendingGroupcast = tim->m_hasMulticastPending;
}

void
PowerSaveManager::NotifyReceivedFrameAfterPsPoll(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);
}

void
PowerSaveManager::NotifyReceivedGroupcast(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);
}

void
PowerSaveManager::NotifyRequestAccess(Ptr<Txop> txop, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << txop << linkId);
}

void
PowerSaveManager::TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << reason << *mpdu);
}

} // namespace ns3
