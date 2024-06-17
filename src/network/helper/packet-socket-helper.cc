/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "packet-socket-helper.h"

#include "ns3/names.h"
#include "ns3/packet-socket-factory.h"

namespace ns3
{

void
PacketSocketHelper::Install(NodeContainer c) const
{
    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        Install(*i);
    }
}

void
PacketSocketHelper::Install(Ptr<Node> node) const
{
    Ptr<PacketSocketFactory> factory = CreateObject<PacketSocketFactory>();
    node->AggregateObject(factory);
}

void
PacketSocketHelper::Install(std::string nodeName) const
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    Install(node);
}

} // namespace ns3
