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

#include "eht-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/log.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "][mac=" << m_self << "] "

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(EhtFrameExchangeManager);

TypeId
EhtFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EhtFrameExchangeManager")
                            .SetParent<HeFrameExchangeManager>()
                            .AddConstructor<EhtFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

EhtFrameExchangeManager::EhtFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
}

EhtFrameExchangeManager::~EhtFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

Ptr<WifiMpdu>
EhtFrameExchangeManager::CreateAlias(Ptr<WifiMpdu> mpdu) const
{
    NS_LOG_FUNCTION(this << *mpdu);

    // alias needs only be created for non-broadcast data frames exchanged between two MLDs
    if (!mpdu->GetHeader().IsData() || m_mac->GetNLinks() == 1 ||
        mpdu->GetHeader().GetAddr1().IsGroup() ||
        !GetWifiRemoteStationManager()->GetMldAddress(mpdu->GetHeader().GetAddr1()))
    {
        return HeFrameExchangeManager::CreateAlias(mpdu);
    }

    mpdu = mpdu->CreateAlias(m_linkId);
    mpdu->GetHeader().SetAddr2(GetAddress());
    auto address =
        GetWifiRemoteStationManager()->GetAffiliatedStaAddress(mpdu->GetHeader().GetAddr1());
    NS_ASSERT(address);
    mpdu->GetHeader().SetAddr1(*address);

    return mpdu;
}

} // namespace ns3
