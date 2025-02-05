/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "dso-manager.h"

#include "uhr-configuration.h"
#include "uhr-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/log.h"
#include "ns3/sta-wifi-mac.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsoManager");

NS_OBJECT_ENSURE_REGISTERED(DsoManager);

TypeId
DsoManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DsoManager").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

DsoManager::DsoManager()
{
    NS_LOG_FUNCTION(this);
}

DsoManager::~DsoManager()
{
    NS_LOG_FUNCTION(this);
}

void
DsoManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_staMac = nullptr;
    Object::DoDispose();
}

void
DsoManager::SetWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    NS_ASSERT(mac);
    m_staMac = mac;

    NS_ABORT_MSG_IF(!m_staMac->GetUhrConfiguration(), "DsoManager requires UHR support");
    NS_ABORT_MSG_IF(!m_staMac->GetUhrConfiguration()->GetDsoActivated(),
                    "DsoManager requires DSO to be activated");
    NS_ABORT_MSG_IF(m_staMac->GetTypeOfStation() != STA,
                    "DsoManager can only be installed on non-AP MLDs");
}

Ptr<StaWifiMac>
DsoManager::GetStaMac() const
{
    return m_staMac;
}

Ptr<UhrFrameExchangeManager>
DsoManager::GetUhrFem(uint8_t linkId) const
{
    return StaticCast<UhrFrameExchangeManager>(m_staMac->GetFrameExchangeManager(linkId));
}

} // namespace ns3
