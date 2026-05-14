/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-mac-queue-scheduler.h"

#include "wifi-mac-queue.h"
#include "wifi-mac.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiMacQueueScheduler");

NS_OBJECT_ENSURE_REGISTERED(WifiMacQueueScheduler);

TypeId
WifiMacQueueScheduler::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiMacQueueScheduler").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

void
WifiMacQueueScheduler::DoDispose()
{
    m_mac = nullptr;
}

void
WifiMacQueueScheduler::SetWifiMac(Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    NS_ABORT_MSG_IF(m_mac, "Cannot set the MAC twice");
    m_mac = mac;
}

void
WifiMacQueueScheduler::SetWifiMacQueue(AcIndex ac, Ptr<WifiMacQueue> queue)
{
    NS_LOG_FUNCTION(this << ac << queue);
    NS_ABORT_MSG_IF(!queue, "Cannot set a null wifi MAC queue");
    queue->SetScheduler(this);
}

Ptr<WifiMac>
WifiMacQueueScheduler::GetMac() const
{
    return m_mac;
}

} // namespace ns3
