/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "default-dso-manager.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DefaultDsoManager");

NS_OBJECT_ENSURE_REGISTERED(DefaultDsoManager);

TypeId
DefaultDsoManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DefaultDsoManager")
                            .SetParent<DsoManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<DefaultDsoManager>();
    return tid;
}

DefaultDsoManager::DefaultDsoManager()
{
    NS_LOG_FUNCTION(this);
}

DefaultDsoManager::~DefaultDsoManager()
{
    NS_LOG_FUNCTION(this);
}

} // namespace ns3
