/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv6-raw-socket-factory.h"

#include "ns3/uinteger.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(Ipv6RawSocketFactory);

TypeId
Ipv6RawSocketFactory::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv6RawSocketFactory").SetParent<SocketFactory>().SetGroupName("Internet");
    return tid;
}

} // namespace ns3
