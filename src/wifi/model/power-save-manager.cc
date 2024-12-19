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
#include "wifi-mac-queue.h"
#include "wifi-mpdu.h"
#include "wifi-phy.h"

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/pair.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PowerSaveManager");

NS_OBJECT_ENSURE_REGISTERED(PowerSaveManager);

TypeId
PowerSaveManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PowerSaveManager")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddAttribute(
                "PowerSaveMode",
                "Enable/disable power save mode on the given links. If a string is used to "
                "set this attribute, the string must contain a comma-separated list of pairs, "
                "where each pair is made of the link ID and a boolean value (indicating whether "
                "to enable PS mode on that link), separated by a space. If this object is not "
                "initialized yet when this attribute is set, the settings are stored and notified "
                "to the STA wifi MAC upon initialization.",
                StringValue(""),
                MakeAttributeContainerAccessor<PairValue<UintegerValue, BooleanValue>>(
                    &PowerSaveManager::SetPowerSaveMode),
                MakeAttributeContainerChecker<PairValue<UintegerValue, BooleanValue>>(
                    MakePairChecker<UintegerValue, BooleanValue>(MakeUintegerChecker<uint8_t>(),
                                                                 MakeBooleanChecker())))
            .AddAttribute("ListenInterval",
                          "Interval (in beacon periods) between successive switches from sleep to "
                          "listen for a PS STAs",
                          UintegerValue(1),
                          MakeUintegerAccessor(&PowerSaveManager::m_listenInterval),
                          MakeUintegerChecker<uint32_t>(1));
    return tid;
}

PowerSaveManager::~PowerSaveManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
PowerSaveManager::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    for (const auto& [linkId, enablePs] : m_linkIdEnableMap)
    {
        m_staMac->SetPowerSaveMode({enablePs, linkId});
    }
    m_linkIdEnableMap.clear();
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

uint32_t
PowerSaveManager::GetListenInterval() const
{
    return m_listenInterval;
}

PowerSaveManager::StaInfo&
PowerSaveManager::GetStaInfo(uint8_t linkId)
{
    NS_ASSERT(m_staInfo.contains(linkId));
    return m_staInfo.at(linkId);
}

bool
PowerSaveManager::HasFramesToSend(uint8_t linkId) const
{
    const auto acList =
        GetStaMac()->GetQosSupported() ? edcaAcIndices : std::list<AcIndex>{AC_BE_NQOS};

    for (const auto aci : acList)
    {
        if (GetStaMac()->GetTxopQueue(aci)->Peek(linkId))
        {
            return true;
        }
    }
    return false;
}

void
PowerSaveManager::SetPowerSaveMode(const std::map<uint8_t, bool>& linkIdEnableMap)
{
    for (const auto& [linkId, enablePs] : linkIdEnableMap)
    {
        if (m_staMac && m_staMac->IsInitialized())
        {
            m_staMac->SetPowerSaveMode({enablePs, linkId});
        }
        else
        {
            m_linkIdEnableMap[linkId] = enablePs;
        }
    }
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

    DoNotifyAssocCompleted();
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

    DoNotifyDisassociation();
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

    DoNotifyReceivedBeacon(beacon, linkId);
}

void
PowerSaveManager::NotifyReceivedFrameAfterPsPoll(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    auto& staInfo = GetStaInfo(linkId);

    if (!staInfo.pendingUnicast)
    {
        NS_LOG_DEBUG("Not expecting a buffered unit on link " << +linkId);
        return;
    }

    // use More Data flag
    staInfo.pendingUnicast = mpdu->GetHeader().IsMoreData();

    if (GetStaMac()->GetPmMode(linkId) == WIFI_PM_POWERSAVE)
    {
        // The PS-Poll is dequeued when the Ack transmission starts. In order to avoid that the STA
        // is not put to sleep because a frame (the PS-Poll) is queued, notify subclasses after
        // that the Ack transmission has started.
        Simulator::Schedule(m_staMac->GetWifiPhy(linkId)->GetSifs() + TimeStep(1),
                            &PowerSaveManager::DoNotifyReceivedFrameAfterPsPoll,
                            this,
                            mpdu,
                            linkId);
    }
}

void
PowerSaveManager::NotifyReceivedGroupcast(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    auto& staInfo = GetStaInfo(linkId);

    if (!staInfo.pendingGroupcast)
    {
        NS_LOG_DEBUG("Not expecting a group addressed frame on link " << +linkId);
        return;
    }

    // use More Data flag
    staInfo.pendingGroupcast = mpdu->GetHeader().IsMoreData();

    if (GetStaMac()->GetPmMode(linkId) == WIFI_PM_POWERSAVE)
    {
        DoNotifyReceivedGroupcast(mpdu, linkId);
    }
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
