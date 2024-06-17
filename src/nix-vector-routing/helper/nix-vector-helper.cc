/*
 * Copyright (c) 2009 The Georgia Institute of Technology
 * Copyright (c) 2021 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is adapted from the old ipv4-nix-vector-helper.cc.
 *
 * Authors: Josh Pelkey <jpelkey@gatech.edu>
 *
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 */

#include "nix-vector-helper.h"

#include "ns3/assert.h"
#include "ns3/nix-vector-routing.h"

namespace ns3
{

template <typename T>
NixVectorHelper<T>::NixVectorHelper()
{
    std::string name;
    if constexpr (IsIpv4)
    {
        name = "Ipv4";
    }
    else
    {
        name = "Ipv6";
    }

    m_agentFactory.SetTypeId("ns3::" + name + "NixVectorRouting");
}

template <typename T>
NixVectorHelper<T>::NixVectorHelper(const NixVectorHelper<T>& o)
    : m_agentFactory(o.m_agentFactory)
{
    // Check if the T is Ipv4RoutingHelper or Ipv6RoutingHelper.
    static_assert((std::is_same_v<Ipv4RoutingHelper, T> || std::is_same_v<Ipv6RoutingHelper, T>),
                  "Template parameter is not Ipv4RoutingHelper or Ipv6Routing Helper");
}

template <typename T>
NixVectorHelper<T>*
NixVectorHelper<T>::Copy() const
{
    return new NixVectorHelper<T>(*this);
}

template <typename T>
Ptr<typename NixVectorHelper<T>::IpRoutingProtocol>
NixVectorHelper<T>::Create(Ptr<Node> node) const
{
    Ptr<NixVectorRouting<IpRoutingProtocol>> agent =
        m_agentFactory.Create<NixVectorRouting<IpRoutingProtocol>>();
    agent->SetNode(node);
    node->AggregateObject(agent);
    return agent;
}

template <typename T>
void
NixVectorHelper<T>::PrintRoutingPathAt(Time printTime,
                                       Ptr<Node> source,
                                       IpAddress dest,
                                       Ptr<OutputStreamWrapper> stream,
                                       Time::Unit unit)
{
    Simulator::Schedule(printTime, &NixVectorHelper<T>::PrintRoute, source, dest, stream, unit);
}

template <typename T>
void
NixVectorHelper<T>::PrintRoute(Ptr<Node> source,
                               IpAddress dest,
                               Ptr<OutputStreamWrapper> stream,
                               Time::Unit unit)
{
    Ptr<NixVectorRouting<IpRoutingProtocol>> rp =
        T::template GetRouting<NixVectorRouting<IpRoutingProtocol>>(
            source->GetObject<Ip>()->GetRoutingProtocol());
    NS_ASSERT(rp);
    rp->PrintRoutingPath(source, dest, stream, unit);
}

template class NixVectorHelper<Ipv4RoutingHelper>;
template class NixVectorHelper<Ipv6RoutingHelper>;

} // namespace ns3
