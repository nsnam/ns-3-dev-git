/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-configuration.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UhrConfiguration");

NS_OBJECT_ENSURE_REGISTERED(UhrConfiguration);

UhrConfiguration::UhrConfiguration()
{
    NS_LOG_FUNCTION(this);
}

UhrConfiguration::~UhrConfiguration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
UhrConfiguration::GetTypeId()
{
    static ns3::TypeId tid = ns3::TypeId("ns3::UhrConfiguration")
                                 .SetParent<Object>()
                                 .SetGroupName("Wifi")
                                 .AddConstructor<UhrConfiguration>();
    return tid;
}

} // namespace ns3
