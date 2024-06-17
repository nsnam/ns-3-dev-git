/*
 * Copyright (c) 2007 Emmanuelle Laprise
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */
#include "packet-socket-factory.h"

#include "packet-socket.h"

#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketSocketFactory");

NS_OBJECT_ENSURE_REGISTERED(PacketSocketFactory);

TypeId
PacketSocketFactory::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PacketSocketFactory").SetParent<SocketFactory>().SetGroupName("Network");
    return tid;
}

PacketSocketFactory::PacketSocketFactory()
{
    NS_LOG_FUNCTION(this);
}

Ptr<Socket>
PacketSocketFactory::CreateSocket()
{
    NS_LOG_FUNCTION(this);
    Ptr<Node> node = GetObject<Node>();
    Ptr<PacketSocket> socket = CreateObject<PacketSocket>();
    socket->SetNode(node);
    return socket;
}
} // namespace ns3
