/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ipv4-routing-protocol.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4RoutingProtocol");

NS_OBJECT_ENSURE_REGISTERED(Ipv4RoutingProtocol);

TypeId
Ipv4RoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv4RoutingProtocol").SetParent<Object>().SetGroupName("Internet");
    return tid;
}

} // namespace ns3
