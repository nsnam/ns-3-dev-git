/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "power-save-manager.h"

#include "sta-wifi-mac.h"

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
    m_staMac = nullptr;
    Object::DoDispose();
}

void
PowerSaveManager::SetWifiMac(Ptr<StaWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_staMac = mac;
}

Ptr<StaWifiMac>
PowerSaveManager::GetStaMac() const
{
    return m_staMac;
}

} // namespace ns3
