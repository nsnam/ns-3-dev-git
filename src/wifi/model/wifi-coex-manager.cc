/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#include "wifi-coex-manager.h"

#include "wifi-net-device.h"

#include "ns3/coex-arbitrator.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiCoexManager");

NS_OBJECT_ENSURE_REGISTERED(WifiCoexManager);

TypeId
WifiCoexManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiCoexManager")
                            .SetParent<coex::DeviceManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiCoexManager>();
    return tid;
}

WifiCoexManager::WifiCoexManager()
{
    NS_LOG_FUNCTION(this);
}

WifiCoexManager::~WifiCoexManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
WifiCoexManager::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    m_device = nullptr;
    coex::DeviceManager::DoDispose();
}

void
WifiCoexManager::SetWifiNetDevice(Ptr<WifiNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    m_device = device;
}

} // namespace ns3
