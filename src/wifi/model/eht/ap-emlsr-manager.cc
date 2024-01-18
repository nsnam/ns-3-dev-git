/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
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

#include "ap-emlsr-manager.h"

#include "eht-configuration.h"
#include "eht-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ApEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(ApEmlsrManager);

TypeId
ApEmlsrManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::ApEmlsrManager").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

ApEmlsrManager::ApEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

ApEmlsrManager::~ApEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
ApEmlsrManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_apMac = nullptr;
    Object::DoDispose();
}

void
ApEmlsrManager::SetWifiMac(Ptr<ApWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    NS_ASSERT(mac);
    m_apMac = mac;

    NS_ABORT_MSG_IF(!m_apMac->GetEhtConfiguration(), "ApEmlsrManager requires EHT support");
    NS_ABORT_MSG_IF(m_apMac->GetNLinks() <= 1, "ApEmlsrManager can only be installed on MLDs");
    NS_ABORT_MSG_IF(m_apMac->GetTypeOfStation() != AP,
                    "ApEmlsrManager can only be installed on AP MLDs");
}

Ptr<ApWifiMac>
ApEmlsrManager::GetApMac() const
{
    return m_apMac;
}

Ptr<EhtFrameExchangeManager>
ApEmlsrManager::GetEhtFem(uint8_t linkId) const
{
    return StaticCast<EhtFrameExchangeManager>(m_apMac->GetFrameExchangeManager(linkId));
}

} // namespace ns3
