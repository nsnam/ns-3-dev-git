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

#include "vht-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/log.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "][mac=" << m_self << "] "

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VhtFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(VhtFrameExchangeManager);

TypeId
VhtFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::VhtFrameExchangeManager")
                            .SetParent<HtFrameExchangeManager>()
                            .AddConstructor<VhtFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

VhtFrameExchangeManager::VhtFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
}

VhtFrameExchangeManager::~VhtFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

Ptr<WifiPsdu>
VhtFrameExchangeManager::GetWifiPsdu(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector) const
{
    return Create<WifiPsdu>(mpdu, txVector.GetModulationClass() >= WIFI_MOD_CLASS_VHT);
}

uint32_t
VhtFrameExchangeManager::GetPsduSize(Ptr<const WifiMpdu> mpdu, const WifiTxVector& txVector) const
{
    return (txVector.GetModulationClass() >= WIFI_MOD_CLASS_VHT
                ? MpduAggregator::GetSizeIfAggregated(mpdu->GetSize(), 0)
                : HtFrameExchangeManager::GetPsduSize(mpdu, txVector));
}

} // namespace ns3
