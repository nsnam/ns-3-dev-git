/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "vht-frame-exchange-manager.h"

#include "ns3/abort.h"
#include "ns3/log.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

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
