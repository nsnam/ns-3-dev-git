/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ipv4-raw-socket-factory.h"

#include "ns3/log.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4RawSocketFactory");

NS_OBJECT_ENSURE_REGISTERED(Ipv4RawSocketFactory);

TypeId
Ipv4RawSocketFactory::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::Ipv4RawSocketFactory").SetParent<SocketFactory>().SetGroupName("Internet");
    return tid;
}

} // namespace ns3
