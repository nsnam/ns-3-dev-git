/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "ipv4-list-routing-helper.h"

#include "ns3/ipv4-list-routing.h"
#include "ns3/node.h"

namespace ns3
{

Ipv4ListRoutingHelper::Ipv4ListRoutingHelper()
{
}

Ipv4ListRoutingHelper::~Ipv4ListRoutingHelper()
{
    for (auto i = m_list.begin(); i != m_list.end(); ++i)
    {
        delete i->first;
    }
}

Ipv4ListRoutingHelper::Ipv4ListRoutingHelper(const Ipv4ListRoutingHelper& o)
{
    for (auto i = o.m_list.begin(); i != o.m_list.end(); ++i)
    {
        m_list.emplace_back(const_cast<const Ipv4RoutingHelper*>(i->first->Copy()), i->second);
    }
}

Ipv4ListRoutingHelper*
Ipv4ListRoutingHelper::Copy() const
{
    return new Ipv4ListRoutingHelper(*this);
}

void
Ipv4ListRoutingHelper::Add(const Ipv4RoutingHelper& routing, int16_t priority)
{
    m_list.emplace_back(const_cast<const Ipv4RoutingHelper*>(routing.Copy()), priority);
}

Ptr<Ipv4RoutingProtocol>
Ipv4ListRoutingHelper::Create(Ptr<Node> node) const
{
    Ptr<Ipv4ListRouting> list = CreateObject<Ipv4ListRouting>();
    for (auto i = m_list.begin(); i != m_list.end(); ++i)
    {
        Ptr<Ipv4RoutingProtocol> prot = i->first->Create(node);
        list->AddRoutingProtocol(prot, i->second);
    }
    return list;
}

} // namespace ns3
