/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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

#include "default-emlsr-manager.h"

#include "ns3/log.h"
#include "ns3/wifi-mpdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultEmlsrManager);

TypeId
DefaultEmlsrManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DefaultEmlsrManager")
                            .SetParent<EmlsrManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<DefaultEmlsrManager>();
    return tid;
}

DefaultEmlsrManager::DefaultEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultEmlsrManager::~DefaultEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
DefaultEmlsrManager::DoNotifyMgtFrameReceived(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << *mpdu << linkId);

    if (mpdu->GetHeader().IsAssocResp() && GetStaMac()->IsAssociated() && GetTransitionTimeout())
    {
        m_assocLinkId = linkId;
    }
}

uint8_t
DefaultEmlsrManager::GetLinkToSendEmlNotification()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_assocLinkId, "No recorded link on which Assoc Response was received");
    return *m_assocLinkId;
}

std::optional<uint8_t>
DefaultEmlsrManager::ResendNotification(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_assocLinkId, "No recorded link on which Assoc Response was received");
    return *m_assocLinkId;
}

void
DefaultEmlsrManager::NotifyEmlsrModeChanged()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
