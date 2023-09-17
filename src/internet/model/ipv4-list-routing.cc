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

#include "ipv4-list-routing.h"

#include "ipv4-route.h"
#include "ipv4.h"

#include "ns3/log.h"
#include "ns3/node.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4ListRouting");

NS_OBJECT_ENSURE_REGISTERED(Ipv4ListRouting);

TypeId
Ipv4ListRouting::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv4ListRouting")
                            .SetParent<Ipv4RoutingProtocol>()
                            .SetGroupName("Internet")
                            .AddConstructor<Ipv4ListRouting>();
    return tid;
}

Ipv4ListRouting::Ipv4ListRouting()
    : m_ipv4(nullptr)
{
    NS_LOG_FUNCTION(this);
}

Ipv4ListRouting::~Ipv4ListRouting()
{
    NS_LOG_FUNCTION(this);
}

void
Ipv4ListRouting::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        // Note:  Calling dispose on these protocols causes memory leak
        //        The routing protocols should not maintain a pointer to
        //        this object, so Dispose() shouldn't be necessary.
        (*rprotoIter).second = nullptr;
    }
    m_routingProtocols.clear();
    m_ipv4 = nullptr;
}

void
Ipv4ListRouting::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    NS_LOG_FUNCTION(this << stream);
    *stream->GetStream() << "Node: " << m_ipv4->GetObject<Node>()->GetId()
                         << ", Time: " << Now().As(unit)
                         << ", Local time: " << m_ipv4->GetObject<Node>()->GetLocalTime().As(unit)
                         << ", Ipv4ListRouting table" << std::endl;
    for (auto i = m_routingProtocols.begin(); i != m_routingProtocols.end(); i++)
    {
        *stream->GetStream() << "  Priority: " << (*i).first
                             << " Protocol: " << (*i).second->GetInstanceTypeId() << std::endl;
        (*i).second->PrintRoutingTable(stream, unit);
    }
}

void
Ipv4ListRouting::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        Ptr<Ipv4RoutingProtocol> protocol = (*rprotoIter).second;
        protocol->Initialize();
    }
    Ipv4RoutingProtocol::DoInitialize();
}

Ptr<Ipv4Route>
Ipv4ListRouting::RouteOutput(Ptr<Packet> p,
                             const Ipv4Header& header,
                             Ptr<NetDevice> oif,
                             Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << p << header.GetDestination() << header.GetSource() << oif << sockerr);
    Ptr<Ipv4Route> route;

    for (auto i = m_routingProtocols.begin(); i != m_routingProtocols.end(); i++)
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
Ipv4ListRouting::RouteInput(Ptr<const Packet> p,
                            const Ipv4Header& header,
                            Ptr<const NetDevice> idev,
                            const UnicastForwardCallback& ucb,
                            const MulticastForwardCallback& mcb,
                            const LocalDeliverCallback& lcb,
                            const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p << header << idev << &ucb << &mcb << &lcb << &ecb);
    bool retVal = false;
    NS_LOG_LOGIC("RouteInput logic for node: " << m_ipv4->GetObject<Node>()->GetId());

    NS_ASSERT(m_ipv4);
    // Check if input device supports IP
    NS_ASSERT(m_ipv4->GetInterfaceForDevice(idev) >= 0);
    uint32_t iif = m_ipv4->GetInterfaceForDevice(idev);

    retVal = m_ipv4->IsDestinationAddress(header.GetDestination(), iif);
    if (retVal)
    {
        NS_LOG_LOGIC("Address " << header.GetDestination() << " is a match for local delivery");
        if (header.GetDestination().IsMulticast())
        {
            Ptr<Packet> packetCopy = p->Copy();
            lcb(packetCopy, header, iif);
            retVal = true;
            // Fall through
        }
        else
        {
            lcb(p, header, iif);
            return true;
        }
    }
    // Check if input device supports IP forwarding
    if (!m_ipv4->IsForwarding(iif))
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, header, Socket::ERROR_NOROUTETOHOST);
        return true;
    }
    // Next, try to find a route
    // If we have already delivered a packet locally (e.g. multicast)
    // we suppress further downstream local delivery by nulling the callback
    LocalDeliverCallback downstreamLcb = lcb;
    if (retVal)
    {
        downstreamLcb = MakeNullCallback<void, Ptr<const Packet>, const Ipv4Header&, uint32_t>();
    }
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        if ((*rprotoIter).second->RouteInput(p, header, idev, ucb, mcb, downstreamLcb, ecb))
        {
            NS_LOG_LOGIC("Route found to forward packet in protocol "
                         << (*rprotoIter).second->GetInstanceTypeId().GetName());
            return true;
        }
    }
    // No routing protocol has found a route.
    return retVal;
}

void
Ipv4ListRouting::NotifyInterfaceUp(uint32_t interface)
{
    NS_LOG_FUNCTION(this << interface);
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyInterfaceUp(interface);
    }
}

void
Ipv4ListRouting::NotifyInterfaceDown(uint32_t interface)
{
    NS_LOG_FUNCTION(this << interface);
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyInterfaceDown(interface);
    }
}

void
Ipv4ListRouting::NotifyAddAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyAddAddress(interface, address);
    }
}

void
Ipv4ListRouting::NotifyRemoveAddress(uint32_t interface, Ipv4InterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->NotifyRemoveAddress(interface, address);
    }
}

void
Ipv4ListRouting::SetIpv4(Ptr<Ipv4> ipv4)
{
    NS_LOG_FUNCTION(this << ipv4);
    NS_ASSERT(!m_ipv4);
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
         rprotoIter++)
    {
        (*rprotoIter).second->SetIpv4(ipv4);
    }
    m_ipv4 = ipv4;
}

void
Ipv4ListRouting::AddRoutingProtocol(Ptr<Ipv4RoutingProtocol> routingProtocol, int16_t priority)
{
    NS_LOG_FUNCTION(this << routingProtocol->GetInstanceTypeId() << priority);
    m_routingProtocols.emplace_back(priority, routingProtocol);
    m_routingProtocols.sort(Compare);
    if (m_ipv4)
    {
        routingProtocol->SetIpv4(m_ipv4);
    }
}

uint32_t
Ipv4ListRouting::GetNRoutingProtocols() const
{
    NS_LOG_FUNCTION(this);
    return m_routingProtocols.size();
}

Ptr<Ipv4RoutingProtocol>
Ipv4ListRouting::GetRoutingProtocol(uint32_t index, int16_t& priority) const
{
    NS_LOG_FUNCTION(this << index << priority);
    if (index >= m_routingProtocols.size())
    {
        NS_FATAL_ERROR("Ipv4ListRouting::GetRoutingProtocol():  index " << index
                                                                        << " out of range");
    }
    uint32_t i = 0;
    for (auto rprotoIter = m_routingProtocols.begin(); rprotoIter != m_routingProtocols.end();
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
Ipv4ListRouting::Compare(const Ipv4RoutingProtocolEntry& a, const Ipv4RoutingProtocolEntry& b)
{
    NS_LOG_FUNCTION(a.first << a.second << b.first << b.second);
    return a.first > b.first;
}

} // namespace ns3
