/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-frame-exchange-manager.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UhrFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(UhrFrameExchangeManager);

TypeId
UhrFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UhrFrameExchangeManager")
                            .SetParent<EhtFrameExchangeManager>()
                            .AddConstructor<UhrFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

UhrFrameExchangeManager::UhrFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
}

UhrFrameExchangeManager::~UhrFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

} // namespace ns3
