/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-mac-queue-scheduler.h"

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
    m_mac = mac;
}

Ptr<WifiMac>
WifiMacQueueScheduler::GetMac() const
{
    return m_mac;
}

} // namespace ns3
