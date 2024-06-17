/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv4-raw-socket-factory-impl.h"

#include "ipv4-l3-protocol.h"

#include "ns3/log.h"
#include "ns3/socket.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4RawSocketFactoryImpl");

Ptr<Socket>
Ipv4RawSocketFactoryImpl::CreateSocket()
{
    NS_LOG_FUNCTION(this);
    Ptr<Ipv4> ipv4 = GetObject<Ipv4>();
    Ptr<Socket> socket = ipv4->CreateRawSocket();
    return socket;
}

} // namespace ns3
