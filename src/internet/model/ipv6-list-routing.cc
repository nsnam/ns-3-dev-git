/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "ipv6-list-routing.h"

#include "ipv6-route.h"
#include "ipv6.h"

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6ListRouting");

NS_OBJECT_ENSURE_REGISTERED(Ipv6ListRouting);

TypeId
Ipv6ListRouting::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv6ListRouting")
                            .SetParent<Ipv6RoutingProtocol>()
                            .SetGroupName("Internet")
                            .AddConstructor<Ipv6ListRouting>();
    return tid;
}

Ipv6ListRouting::Ipv6ListRouting()
    : m_ipv6(nullptr)
{
    NS_LOG_FUNCTION(this);
}

Ipv6ListRouting::~Ipv6ListRouting()
{
    NS_LOG_FUNCTION(this);
}

void
Ipv6ListRouting::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (Ipv6RoutingProtocolList::iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        // Note:  Calling dispose on these protocols causes memory leak
        //        The routing protocols should not maintain a pointer to
        //        this object, so Dispose () shouldn't be necessary.
        (*rprotoIter).second = nullptr;
    }
    m_routingProtocols.clear();
    m_ipv6 = nullptr;
}

Ptr<Ipv6Route>
Ipv6ListRouting::RouteOutput(Ptr<Packet> p,
                             const Ipv6Header& header,
                             Ptr<NetDevice> oif,
                             Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << header.GetDestination() << header.GetSource() << oif);
    Ptr<Ipv6Route> route;

    for (Ipv6RoutingProtocolList::const_iterator i = m_routingProtocols.begin();
         i != m_routingProtocols.end();
         i++)
    {
        NS_LOG_LOGIC("Checking protocol " << (*i).second->GetInstanceTypeId() << " with priority "
                                          << (*i).first);
        NS_LOG_LOGIC("Requesting source address for destination " << header.GetDestination());
        route = (*i).second->RouteOutput(p, header, oif, sockerr);
        if (route)
        {
            NS_LOG_LOGIC("Found route " << route);
            sockerr = Socket::ERROR_NOTERROR;
            return route;
        }
    }
    NS_LOG_LOGIC("Done checking " << GetTypeId());
    NS_LOG_LOGIC("");
    sockerr = Socket::ERROR_NOROUTETOHOST;
    return nullptr;
}

// Patterned after Linux ip_route_input and ip_route_input_slow
bool
Ipv6ListRouting::RouteInput(Ptr<const Packet> p,
                            const Ipv6Header& header,
                            Ptr<const NetDevice> idev,
                            const UnicastForwardCallback& ucb,
                            const MulticastForwardCallback& mcb,
                            const LocalDeliverCallback& lcb,
                            const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(p << header << idev);
    NS_LOG_LOGIC("RouteInput logic for node: " << m_ipv6->GetObject<Node>()->GetId());

    NS_ASSERT(m_ipv6);
    // Check if input device supports IP
    NS_ASSERT(m_ipv6->GetInterfaceForDevice(idev) >= 0);
    Ipv6Address dst = header.GetDestination();

    // Check if input device supports IP forwarding
    uint32_t iif = m_ipv6->GetInterfaceForDevice(idev);
    if (!m_ipv6->IsForwarding(iif))
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, header, Socket::ERROR_NOROUTETOHOST);
        return true;
    }

    // We disable error callback for the called protocols.
    ErrorCallback nullEcb =
        MakeNullCallback<void, Ptr<const Packet>, const Ipv6Header&, Socket::SocketErrno>();

    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        if ((*rprotoIter).second->RouteInput(p, header, idev, ucb, mcb, lcb, nullEcb))
        {
            return true;
        }
    }

    // No routing protocol has found a route.
    ecb(p, header, Socket::ERROR_NOROUTETOHOST);
    return false;
}

void
Ipv6ListRouting::NotifyInterfaceUp(uint32_t interface)
{
    NS_LOG_FUNCTION(this << interface);
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyInterfaceUp(interface);
    }
}

void
Ipv6ListRouting::NotifyInterfaceDown(uint32_t interface)
{
    NS_LOG_FUNCTION(this << interface);
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyInterfaceDown(interface);
    }
}

void
Ipv6ListRouting::NotifyAddAddress(uint32_t interface, Ipv6InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyAddAddress(interface, address);
    }
}

void
Ipv6ListRouting::NotifyRemoveAddress(uint32_t interface, Ipv6InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyRemoveAddress(interface, address);
    }
}

void
Ipv6ListRouting::NotifyAddRoute(Ipv6Address dst,
                                Ipv6Prefix mask,
                                Ipv6Address nextHop,
                                uint32_t interface,
                                Ipv6Address prefixToUse)
{
    NS_LOG_FUNCTION(this << dst << mask << nextHop << interface);
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyAddRoute(dst, mask, nextHop, interface, prefixToUse);
    }
}

void
Ipv6ListRouting::NotifyRemoveRoute(Ipv6Address dst,
                                   Ipv6Prefix mask,
                                   Ipv6Address nextHop,
                                   uint32_t interface,
                                   Ipv6Address prefixToUse)
{
    NS_LOG_FUNCTION(this << dst << mask << nextHop << interface);
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyRemoveRoute(dst, mask, nextHop, interface, prefixToUse);
    }
}

void
Ipv6ListRouting::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    NS_LOG_FUNCTION(this);

    *stream->GetStream() << "Node: " << m_ipv6->GetObject<Node>()->GetId()
                         << ", Time: " << Now().As(unit)
                         << ", Local time: " << m_ipv6->GetObject<Node>()->GetLocalTime().As(unit)
                         << ", Ipv6ListRouting table" << std::endl;
    for (Ipv6RoutingProtocolList::const_iterator i = m_routingProtocols.begin();
         i != m_routingProtocols.end();
         i++)
    {
        *stream->GetStream() << "  Priority: " << (*i).first
                             << " Protocol: " << (*i).second->GetInstanceTypeId() << std::endl;
        (*i).second->PrintRoutingTable(stream, unit);
    }
}

void
Ipv6ListRouting::SetIpv6(Ptr<Ipv6> ipv6)
{
    NS_LOG_FUNCTION(this << ipv6);
    NS_ASSERT(!m_ipv6);
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->SetIpv6(ipv6);
    }
    m_ipv6 = ipv6;
}

void
Ipv6ListRouting::AddRoutingProtocol(Ptr<Ipv6RoutingProtocol> routingProtocol, int16_t priority)
{
    NS_LOG_FUNCTION(this << routingProtocol->GetInstanceTypeId() << priority);
    m_routingProtocols.emplace_back(priority, routingProtocol);
    m_routingProtocols.sort(Compare);
    if (m_ipv6)
    {
        routingProtocol->SetIpv6(m_ipv6);
    }
}

uint32_t
Ipv6ListRouting::GetNRoutingProtocols() const
{
    NS_LOG_FUNCTION(this);
    return m_routingProtocols.size();
}

Ptr<Ipv6RoutingProtocol>
Ipv6ListRouting::GetRoutingProtocol(uint32_t index, int16_t& priority) const
{
    NS_LOG_FUNCTION(index);
    if (index >= m_routingProtocols.size())
    {
        NS_FATAL_ERROR("Ipv6ListRouting::GetRoutingProtocol ():  index " << index
                                                                         << " out of range");
    }
    uint32_t i = 0;
    for (Ipv6RoutingProtocolList::const_iterator rprotoIter = m_routingProtocols.begin();
         rprotoIter != m_routingProtocols.end();
         rprotoIter++, i++)
    {
        if (i == index)
        {
            priority = (*rprotoIter).first;
            return (*rprotoIter).second;
        }
    }
    return nullptr;
}

bool
Ipv6ListRouting::Compare(const Ipv6RoutingProtocolEntry& a, const Ipv6RoutingProtocolEntry& b)
{
    return a.first > b.first;
}

} // namespace ns3
