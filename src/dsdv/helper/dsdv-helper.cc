/*
 * Copyright (c) 2010 Hemanth Narra
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Hemanth Narra <hemanth@ittc.ku.com>, written after OlsrHelper by Mathieu Lacage
 * <mathieu.lacage@sophia.inria.fr>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */
#include "dsdv-helper.h"

#include "ns3/dsdv-routing-protocol.h"
#include "ns3/ipv4-list-routing.h"
#include "ns3/names.h"
#include "ns3/node-list.h"

namespace ns3
{
DsdvHelper::~DsdvHelper()
{
}

DsdvHelper::DsdvHelper()
    : Ipv4RoutingHelper()
{
    m_agentFactory.SetTypeId("ns3::dsdv::RoutingProtocol");
}

DsdvHelper*
DsdvHelper::Copy() const
{
    return new DsdvHelper(*this);
}

Ptr<Ipv4RoutingProtocol>
DsdvHelper::Create(Ptr<Node> node) const
{
    Ptr<dsdv::RoutingProtocol> agent = m_agentFactory.Create<dsdv::RoutingProtocol>();
    node->AggregateObject(agent);
    return agent;
}

void
DsdvHelper::Set(std::string name, const AttributeValue& value)
{
    m_agentFactory.Set(name, value);
}

} // namespace ns3
