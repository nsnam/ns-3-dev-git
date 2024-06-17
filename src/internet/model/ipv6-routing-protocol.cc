/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

/* taken from src/node/ipv4-routing-protocol.cc and adapted to IPv6 */

#include "ipv6-routing-protocol.h"

#include "ns3/assert.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(Ipv6RoutingProtocol);

TypeId
Ipv6RoutingProtocol::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv6RoutingProtocol").SetParent<Object>().SetGroupName("Internet");
    return tid;
}

} /* namespace ns3 */
