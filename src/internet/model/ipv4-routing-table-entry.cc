/*
 * Copyright (c) 2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv4-routing-table-entry.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4RoutingTableEntry");

/*****************************************************
 *     Network Ipv4RoutingTableEntry
 *****************************************************/

Ipv4RoutingTableEntry::Ipv4RoutingTableEntry()
{
    NS_LOG_FUNCTION(this);
}

Ipv4RoutingTableEntry::Ipv4RoutingTableEntry(const Ipv4RoutingTableEntry& route)
    : m_dest(route.m_dest),
      m_destNetworkMask(route.m_destNetworkMask),
      m_gateway(route.m_gateway),
      m_interface(route.m_interface)
{
    NS_LOG_FUNCTION(this << route);
}

Ipv4RoutingTableEntry::Ipv4RoutingTableEntry(const Ipv4RoutingTableEntry* route)
    : m_dest(route->m_dest),
      m_destNetworkMask(route->m_destNetworkMask),
      m_gateway(route->m_gateway),
      m_interface(route->m_interface)
{
    NS_LOG_FUNCTION(this << route);
}

Ipv4RoutingTableEntry::Ipv4RoutingTableEntry(Ipv4Address dest,
                                             Ipv4Address gateway,
                                             uint32_t interface)
    : m_dest(dest),
      m_destNetworkMask(Ipv4Mask::GetOnes()),
      m_gateway(gateway),
      m_interface(interface)
{
}

Ipv4RoutingTableEntry::Ipv4RoutingTableEntry(Ipv4Address dest, uint32_t interface)
    : m_dest(dest),
      m_destNetworkMask(Ipv4Mask::GetOnes()),
      m_gateway(Ipv4Address::GetZero()),
      m_interface(interface)
{
}

Ipv4RoutingTableEntry::Ipv4RoutingTableEntry(Ipv4Address network,
                                             Ipv4Mask networkMask,
                                             Ipv4Address gateway,
                                             uint32_t interface)
    : m_dest(network),
      m_destNetworkMask(networkMask),
      m_gateway(gateway),
      m_interface(interface)
{
    NS_LOG_FUNCTION(this << network << networkMask << gateway << interface);
}

Ipv4RoutingTableEntry::Ipv4RoutingTableEntry(Ipv4Address network,
                                             Ipv4Mask networkMask,
                                             uint32_t interface)
    : m_dest(network),
      m_destNetworkMask(networkMask),
      m_gateway(Ipv4Address::GetZero()),
      m_interface(interface)
{
    NS_LOG_FUNCTION(this << network << networkMask << interface);
}

bool
Ipv4RoutingTableEntry::IsHost() const
{
    NS_LOG_FUNCTION(this);
    return m_destNetworkMask == Ipv4Mask::GetOnes();
}

Ipv4Address
Ipv4RoutingTableEntry::GetDest() const
{
    NS_LOG_FUNCTION(this);
    return m_dest;
}

bool
Ipv4RoutingTableEntry::IsNetwork() const
{
    NS_LOG_FUNCTION(this);
    return !IsHost();
}

bool
Ipv4RoutingTableEntry::IsDefault() const
{
    NS_LOG_FUNCTION(this);
    return m_dest == Ipv4Address::GetZero();
}

Ipv4Address
Ipv4RoutingTableEntry::GetDestNetwork() const
{
    NS_LOG_FUNCTION(this);
    return m_dest;
}

Ipv4Mask
Ipv4RoutingTableEntry::GetDestNetworkMask() const
{
    NS_LOG_FUNCTION(this);
    return m_destNetworkMask;
}

bool
Ipv4RoutingTableEntry::IsGateway() const
{
    NS_LOG_FUNCTION(this);
    return m_gateway != Ipv4Address::GetZero();
}

Ipv4Address
Ipv4RoutingTableEntry::GetGateway() const
{
    NS_LOG_FUNCTION(this);
    return m_gateway;
}

uint32_t
Ipv4RoutingTableEntry::GetInterface() const
{
    NS_LOG_FUNCTION(this);
    return m_interface;
}

Ipv4RoutingTableEntry
Ipv4RoutingTableEntry::CreateHostRouteTo(Ipv4Address dest, Ipv4Address nextHop, uint32_t interface)
{
    NS_LOG_FUNCTION(dest << nextHop << interface);
    return Ipv4RoutingTableEntry(dest, nextHop, interface);
}

Ipv4RoutingTableEntry
Ipv4RoutingTableEntry::CreateHostRouteTo(Ipv4Address dest, uint32_t interface)
{
    NS_LOG_FUNCTION(dest << interface);
    return Ipv4RoutingTableEntry(dest, interface);
}

Ipv4RoutingTableEntry
Ipv4RoutingTableEntry::CreateNetworkRouteTo(Ipv4Address network,
                                            Ipv4Mask networkMask,
                                            Ipv4Address nextHop,
                                            uint32_t interface)
{
    NS_LOG_FUNCTION(network << networkMask << nextHop << interface);
    return Ipv4RoutingTableEntry(network, networkMask, nextHop, interface);
}

Ipv4RoutingTableEntry
Ipv4RoutingTableEntry::CreateNetworkRouteTo(Ipv4Address network,
                                            Ipv4Mask networkMask,
                                            uint32_t interface)
{
    NS_LOG_FUNCTION(network << networkMask << interface);
    return Ipv4RoutingTableEntry(network, networkMask, interface);
}

Ipv4RoutingTableEntry
Ipv4RoutingTableEntry::CreateDefaultRoute(Ipv4Address nextHop, uint32_t interface)
{
    NS_LOG_FUNCTION(nextHop << interface);
    return Ipv4RoutingTableEntry(Ipv4Address::GetZero(), Ipv4Mask::GetZero(), nextHop, interface);
}

std::ostream&
operator<<(std::ostream& os, const Ipv4RoutingTableEntry& route)
{
    if (route.IsDefault())
    {
        NS_ASSERT(route.IsGateway());
        os << "default out=" << route.GetInterface() << ", next hop=" << route.GetGateway();
    }
    else if (route.IsHost())
    {
        if (route.IsGateway())
        {
            os << "host=" << route.GetDest() << ", out=" << route.GetInterface()
               << ", next hop=" << route.GetGateway();
        }
        else
        {
            os << "host=" << route.GetDest() << ", out=" << route.GetInterface();
        }
    }
    else if (route.IsNetwork())
    {
        if (route.IsGateway())
        {
            os << "network=" << route.GetDestNetwork() << ", mask=" << route.GetDestNetworkMask()
               << ",out=" << route.GetInterface() << ", next hop=" << route.GetGateway();
        }
        else
        {
            os << "network=" << route.GetDestNetwork() << ", mask=" << route.GetDestNetworkMask()
               << ",out=" << route.GetInterface();
        }
    }
    else
    {
        NS_ASSERT(false);
    }
    return os;
}

bool
operator==(const Ipv4RoutingTableEntry a, const Ipv4RoutingTableEntry b)
{
    return (a.GetDest() == b.GetDest() && a.GetDestNetworkMask() == b.GetDestNetworkMask() &&
            a.GetGateway() == b.GetGateway() && a.GetInterface() == b.GetInterface());
}

/*****************************************************
 *     Ipv4MulticastRoutingTableEntry
 *****************************************************/

Ipv4MulticastRoutingTableEntry::Ipv4MulticastRoutingTableEntry()
{
    NS_LOG_FUNCTION(this);
}

Ipv4MulticastRoutingTableEntry::Ipv4MulticastRoutingTableEntry(
    const Ipv4MulticastRoutingTableEntry& route)
    : m_origin(route.m_origin),
      m_group(route.m_group),
      m_inputInterface(route.m_inputInterface),
      m_outputInterfaces(route.m_outputInterfaces)
{
    NS_LOG_FUNCTION(this << route);
}

Ipv4MulticastRoutingTableEntry::Ipv4MulticastRoutingTableEntry(
    const Ipv4MulticastRoutingTableEntry* route)
    : m_origin(route->m_origin),
      m_group(route->m_group),
      m_inputInterface(route->m_inputInterface),
      m_outputInterfaces(route->m_outputInterfaces)
{
    NS_LOG_FUNCTION(this << route);
}

Ipv4MulticastRoutingTableEntry::Ipv4MulticastRoutingTableEntry(
    Ipv4Address origin,
    Ipv4Address group,
    uint32_t inputInterface,
    std::vector<uint32_t> outputInterfaces)
{
    NS_LOG_FUNCTION(this << origin << group << inputInterface << &outputInterfaces);
    m_origin = origin;
    m_group = group;
    m_inputInterface = inputInterface;
    m_outputInterfaces = outputInterfaces;
}

Ipv4Address
Ipv4MulticastRoutingTableEntry::GetOrigin() const
{
    NS_LOG_FUNCTION(this);
    return m_origin;
}

Ipv4Address
Ipv4MulticastRoutingTableEntry::GetGroup() const
{
    NS_LOG_FUNCTION(this);
    return m_group;
}

uint32_t
Ipv4MulticastRoutingTableEntry::GetInputInterface() const
{
    NS_LOG_FUNCTION(this);
    return m_inputInterface;
}

uint32_t
Ipv4MulticastRoutingTableEntry::GetNOutputInterfaces() const
{
    NS_LOG_FUNCTION(this);
    return m_outputInterfaces.size();
}

uint32_t
Ipv4MulticastRoutingTableEntry::GetOutputInterface(uint32_t n) const
{
    NS_LOG_FUNCTION(this << n);
    NS_ASSERT_MSG(n < m_outputInterfaces.size(),
                  "Ipv4MulticastRoutingTableEntry::GetOutputInterface (): index out of bounds");

    return m_outputInterfaces[n];
}

std::vector<uint32_t>
Ipv4MulticastRoutingTableEntry::GetOutputInterfaces() const
{
    NS_LOG_FUNCTION(this);
    return m_outputInterfaces;
}

Ipv4MulticastRoutingTableEntry
Ipv4MulticastRoutingTableEntry::CreateMulticastRoute(Ipv4Address origin,
                                                     Ipv4Address group,
                                                     uint32_t inputInterface,
                                                     std::vector<uint32_t> outputInterfaces)
{
    NS_LOG_FUNCTION(origin << group << inputInterface << outputInterfaces);
    return Ipv4MulticastRoutingTableEntry(origin, group, inputInterface, outputInterfaces);
}

std::ostream&
operator<<(std::ostream& os, const Ipv4MulticastRoutingTableEntry& route)
{
    os << "origin=" << route.GetOrigin() << ", group=" << route.GetGroup()
       << ", input interface=" << route.GetInputInterface() << ", output interfaces=";

    for (uint32_t i = 0; i < route.GetNOutputInterfaces(); ++i)
    {
        os << route.GetOutputInterface(i) << " ";
    }

    return os;
}

bool
operator==(const Ipv4MulticastRoutingTableEntry a, const Ipv4MulticastRoutingTableEntry b)
{
    return (a.GetOrigin() == b.GetOrigin() && a.GetGroup() == b.GetGroup() &&
            a.GetInputInterface() == b.GetInputInterface() &&
            a.GetOutputInterfaces() == b.GetOutputInterfaces());
}

} // namespace ns3
