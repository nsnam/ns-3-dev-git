/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv6-raw-socket-factory-impl.h"

#include "ipv6-l3-protocol.h"

#include "ns3/socket.h"

namespace ns3
{

Ptr<Socket>
Ipv6RawSocketFactoryImpl::CreateSocket()
{
    Ptr<Ipv6L3Protocol> ipv6 = GetObject<Ipv6L3Protocol>();
    Ptr<Socket> socket = ipv6->CreateRawSocket();
    return socket;
}

} /* namespace ns3 */
