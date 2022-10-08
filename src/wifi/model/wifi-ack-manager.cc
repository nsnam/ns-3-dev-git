/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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

#include "wifi-ack-manager.h"

#include "wifi-mac.h"
#include "wifi-psdu.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiAckManager");

NS_OBJECT_ENSURE_REGISTERED(WifiAckManager);

TypeId
WifiAckManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiAckManager").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

WifiAckManager::WifiAckManager()
    : m_linkId(0)
{
    NS_LOG_FUNCTION(this);
}

WifiAckManager::~WifiAckManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
WifiAckManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_mac = nullptr;
    Object::DoDispose();
}

void
WifiAckManager::SetWifiMac(Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
}

Ptr<WifiRemoteStationManager>
WifiAckManager::GetWifiRemoteStationManager() const
{
    return m_mac->GetWifiRemoteStationManager(m_linkId);
}

void
WifiAckManager::SetLinkId(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    m_linkId = linkId;
}

void
WifiAckManager::SetQosAckPolicy(Ptr<WifiMpdu> item, const WifiAcknowledgment* acknowledgment)
{
    NS_LOG_FUNCTION(*item << acknowledgment);

    WifiMacHeader& hdr = item->GetHeader();
    if (!hdr.IsQosData())
    {
        return;
    }
    NS_ASSERT(acknowledgment);

    hdr.SetQosAckPolicy(acknowledgment->GetQosAckPolicy(hdr.GetAddr1(), hdr.GetQosTid()));
}

void
WifiAckManager::SetQosAckPolicy(Ptr<WifiPsdu> psdu, const WifiAcknowledgment* acknowledgment)
{
    NS_LOG_FUNCTION(*psdu << acknowledgment);

    if (psdu->GetNMpdus() == 1)
    {
        SetQosAckPolicy(*psdu->begin(), acknowledgment);
        return;
    }

    NS_ASSERT(acknowledgment);

    for (const auto& tid : psdu->GetTids())
    {
        psdu->SetAckPolicyForTid(tid, acknowledgment->GetQosAckPolicy(psdu->GetAddr1(), tid));
    }
}

} // namespace ns3
