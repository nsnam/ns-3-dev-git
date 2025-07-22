//
// Copyright (c) 2008 University of Washington
//
// SPDX-License-Identifier: GPL-2.0-only
//

#include "global-routing.h"

#include "global-route-manager.h"
#include "ipv4-route.h"
#include "ipv4-routing-table-entry.h"
#include "ipv6-route.h"

#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/object.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <iomanip>
#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GlobalRouting");

template <typename T>
TypeId
GlobalRouting<T>::GetTypeId()
{
    if (IsIpv4)
    {
        static TypeId tid =
            TypeId("ns3::Ipv4GlobalRouting")
                .SetParent<T>()
                .SetGroupName("Internet")
                .template AddConstructor<GlobalRouting<T>>()
                .AddAttribute(
                    "RandomEcmpRouting",
                    "Set to true if packets are randomly routed among ECMP; set to false for "
                    "using only one route consistently",
                    BooleanValue(false),
                    MakeBooleanAccessor(&GlobalRouting<T>::m_randomEcmpRouting),
                    MakeBooleanChecker())
                .AddAttribute(
                    "RespondToInterfaceEvents",
                    "Set to true if you want to dynamically recompute the global routes upon "
                    "Interface notification events (up/down, or add/remove address)",
                    BooleanValue(false),
                    MakeBooleanAccessor(&GlobalRouting<T>::m_respondToInterfaceEvents),
                    MakeBooleanChecker());
        return tid;
    }
    else
    {
        static TypeId tid =
            TypeId("ns3::Ipv6GlobalRouting")
                .SetParent<T>()
                .SetGroupName("Internet")
                .template AddConstructor<GlobalRouting<T>>()
                .AddAttribute(
                    "RandomEcmpRouting",
                    "Set to true if packets are randomly routed among ECMP; set to false for "
                    "using only one route consistently",
                    BooleanValue(false),
                    MakeBooleanAccessor(&GlobalRouting<T>::m_randomEcmpRouting),
                    MakeBooleanChecker())
                .AddAttribute(
                    "RespondToInterfaceEvents",
                    "Set to true if you want to dynamically recompute the global routes upon "
                    "Interface notification events (up/down, or add/remove address)",
                    BooleanValue(false),
                    MakeBooleanAccessor(&GlobalRouting<T>::m_respondToInterfaceEvents),
                    MakeBooleanChecker());
        return tid;
    }
}

template <typename T>
GlobalRouting<T>::GlobalRouting()
    : m_randomEcmpRouting(false),
      m_respondToInterfaceEvents(false)
{
    NS_LOG_FUNCTION(this);

    m_rand = CreateObject<UniformRandomVariable>();
}

template <typename T>
GlobalRouting<T>::~GlobalRouting()
{
    NS_LOG_FUNCTION(this);
}

template <typename T>
void
GlobalRouting<T>::AddHostRouteTo(IpAddress dest, IpAddress nextHop, uint32_t interface)
{
    NS_LOG_FUNCTION(this << dest << nextHop << interface);
    auto route = new IpRoutingTableEntry();
    *route = IpRoutingTableEntry::CreateHostRouteTo(dest, nextHop, interface);
    for (auto routePointer : m_hostRoutes)
    {
        if (*routePointer == *route)
        {
            NS_LOG_LOGIC("Route already exists");
            delete route;
            return;
        }
    }
    m_hostRoutes.push_back(route);
}

template <typename T>
void
GlobalRouting<T>::AddHostRouteTo(IpAddress dest, uint32_t interface)
{
    NS_LOG_FUNCTION(this << dest << interface);
    auto route = new IpRoutingTableEntry();
    *route = IpRoutingTableEntry::CreateHostRouteTo(dest, interface);
    for (auto routePointer : m_hostRoutes)
    {
        if (*routePointer == *route)
        {
            NS_LOG_LOGIC("Route already exists");
            delete route;
            return;
        }
    }
    m_hostRoutes.push_back(route);
}

template <typename T>
void
GlobalRouting<T>::AddNetworkRouteTo(IpAddress network,
                                    IpMaskOrPrefix networkMask,
                                    IpAddress nextHop,
                                    uint32_t interface)
{
    NS_LOG_FUNCTION(this << network << networkMask << nextHop << interface);
    auto route = new IpRoutingTableEntry();
    *route = IpRoutingTableEntry::CreateNetworkRouteTo(network, networkMask, nextHop, interface);
    for (auto routePointer : m_networkRoutes)
    {
        if (*routePointer == *route)
        {
            NS_LOG_LOGIC("Route already exists");
            delete route;
            return;
        }
    }
    m_networkRoutes.push_back(route);
}

template <typename T>
void
GlobalRouting<T>::AddNetworkRouteTo(IpAddress network,
                                    IpMaskOrPrefix networkMask,
                                    uint32_t interface)
{
    NS_LOG_FUNCTION(this << network << networkMask << interface);
    auto route = new IpRoutingTableEntry();
    *route = IpRoutingTableEntry::CreateNetworkRouteTo(network, networkMask, interface);
    for (auto routePointer : m_networkRoutes)
    {
        if (*routePointer == *route)
        {
            NS_LOG_LOGIC("Route already exists");
            delete route;
            return;
        }
    }
    m_networkRoutes.push_back(route);
}

template <typename T>
void
GlobalRouting<T>::AddASExternalRouteTo(IpAddress network,
                                       IpMaskOrPrefix networkMask,
                                       IpAddress nextHop,
                                       uint32_t interface)
{
    NS_LOG_FUNCTION(this << network << networkMask << nextHop << interface);
    auto route = new IpRoutingTableEntry();
    *route = IpRoutingTableEntry::CreateNetworkRouteTo(network, networkMask, nextHop, interface);
    for (auto routePointer : m_ASexternalRoutes)
    {
        if (*routePointer == *route)
        {
            NS_LOG_LOGIC("Route already exists");
            delete route;
            return;
        }
    }
    m_ASexternalRoutes.push_back(route);
}

template <typename T>
Ptr<typename GlobalRouting<T>::IpRoute>
GlobalRouting<T>::LookupGlobal(IpAddress dest, Ptr<NetDevice> oif)
{
    NS_LOG_FUNCTION(this << dest << oif);
    NS_LOG_LOGIC("Looking for route for destination " << dest);
    Ptr<IpRoute> rtentry = nullptr;
    // store all available routes that bring packets to their destination
    typedef std::vector<IpRoutingTableEntry*> RouteVec_t;
    RouteVec_t allRoutes;

    NS_LOG_LOGIC("Number of m_hostRoutes = " << m_hostRoutes.size());
    for (auto i = m_hostRoutes.begin(); i != m_hostRoutes.end(); i++)
    {
        NS_ASSERT((*i)->IsHost());
        if ((*i)->GetDest() == dest)
        {
            if (oif)
            {
                if (oif != m_ip->GetNetDevice((*i)->GetInterface()))
                {
                    NS_LOG_LOGIC("Not on requested interface, skipping");
                    continue;
                }
            }
            allRoutes.push_back(*i);
            NS_LOG_LOGIC(allRoutes.size() << "Found global host route" << *i);
        }
    }
    if (allRoutes.empty()) // if no host route is found
    {
        NS_LOG_LOGIC("Number of m_networkRoutes" << m_networkRoutes.size());
        // store the length of the longest mask.
        uint16_t longest_mask = 0;
        for (auto j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
        {
            IpMaskOrPrefix mask;
            if constexpr (IsIpv4)
            {
                mask = (*j)->GetDestNetworkMask();
            }
            else
            {
                mask = (*j)->GetDestNetworkPrefix();
            }
            uint16_t masklen = mask.GetPrefixLength();

            IpAddress entry = (*j)->GetDestNetwork();
            if (mask.IsMatch(dest, entry))
            {
                if (oif)
                {
                    if (oif != m_ip->GetNetDevice((*j)->GetInterface()))
                    {
                        NS_LOG_LOGIC("Not on requested interface, skipping");
                        continue;
                    }
                }
                NS_LOG_LOGIC(allRoutes.size() << "Found global network route" << *j);
                if (masklen < longest_mask) // Not interested if got shorter mask
                {
                    NS_LOG_LOGIC("Previous match longer, skipping");
                    continue;
                }
                else if (masklen == longest_mask)
                {
                    NS_LOG_LOGIC("Equal mask length, adding this to the list");
                    allRoutes.push_back(*j);
                }
                else
                {
                    NS_LOG_LOGIC("Longer mask length found, clearing the list and adding");
                    allRoutes.clear();
                    allRoutes.push_back(*j);
                }
            }
        }
    }
    if (allRoutes.empty()) // consider external if no host/network found
    {
        for (auto k = m_ASexternalRoutes.begin(); k != m_ASexternalRoutes.end(); k++)
        {
            IpMaskOrPrefix mask;
            if constexpr (IsIpv4)
            {
                mask = (*k)->GetDestNetworkMask();
            }
            else
            {
                mask = (*k)->GetDestNetworkPrefix();
            }

            IpAddress entry = (*k)->GetDestNetwork();
            if (mask.IsMatch(dest, entry))
            {
                NS_LOG_LOGIC("Found external route" << *k);
                if (oif)
                {
                    if (oif != m_ip->GetNetDevice((*k)->GetInterface()))
                    {
                        NS_LOG_LOGIC("Not on requested interface, skipping");
                        continue;
                    }
                }
                allRoutes.push_back(*k);
                break;
            }
        }
    }
    if (!allRoutes.empty()) // if route(s) is found
    {
        // pick up one of the routes uniformly at random if random
        // ECMP routing is enabled, or always select the first route
        // consistently if random ECMP routing is disabled
        uint32_t selectIndex;
        if (m_randomEcmpRouting)
        {
            selectIndex = m_rand->GetInteger(0, allRoutes.size() - 1);
        }
        else
        {
            selectIndex = 0;
        }
        IpRoutingTableEntry* route = allRoutes.at(selectIndex);
        // create a Ipv4Route object from the selected routing table entry
        rtentry = Create<IpRoute>();
        rtentry->SetDestination(route->GetDest());
        /// @todo handle multi-address case
        if constexpr (IsIpv4)
        {
            rtentry->SetSource(m_ip->GetAddress(route->GetInterface(), 0).GetAddress());
        }
        else
        {
            rtentry->SetSource(m_ip->GetAddress(route->GetInterface(), 1).GetAddress());
        }
        rtentry->SetGateway(route->GetGateway());
        uint32_t interfaceIdx = route->GetInterface();
        rtentry->SetOutputDevice(m_ip->GetNetDevice(interfaceIdx));
        return rtentry;
    }
    else
    {
        return nullptr;
    }
}

template <typename T>
uint32_t
GlobalRouting<T>::GetNRoutes() const
{
    NS_LOG_FUNCTION(this);
    uint32_t n = 0;
    n += m_hostRoutes.size();
    n += m_networkRoutes.size();
    n += m_ASexternalRoutes.size();
    return n;
}

template <typename T>
GlobalRouting<T>::IpRoutingTableEntry*
GlobalRouting<T>::GetRoute(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    if (index < m_hostRoutes.size())
    {
        uint32_t tmp = 0;
        for (auto i = m_hostRoutes.begin(); i != m_hostRoutes.end(); i++)
        {
            if (tmp == index)
            {
                return *i;
            }
            tmp++;
        }
    }
    index -= m_hostRoutes.size();
    uint32_t tmp = 0;
    if (index < m_networkRoutes.size())
    {
        for (auto j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
        {
            if (tmp == index)
            {
                return *j;
            }
            tmp++;
        }
    }
    index -= m_networkRoutes.size();
    tmp = 0;
    for (auto k = m_ASexternalRoutes.begin(); k != m_ASexternalRoutes.end(); k++)
    {
        if (tmp == index)
        {
            return *k;
        }
        tmp++;
    }
    NS_ASSERT(false);
    // quiet compiler.
    return nullptr;
}

template <typename T>
void
GlobalRouting<T>::RemoveRoute(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    if (index < m_hostRoutes.size())
    {
        uint32_t tmp = 0;
        for (auto i = m_hostRoutes.begin(); i != m_hostRoutes.end(); i++)
        {
            if (tmp == index)
            {
                NS_LOG_LOGIC("Removing route " << index << "; size = " << m_hostRoutes.size());
                delete *i;
                m_hostRoutes.erase(i);
                NS_LOG_LOGIC("Done removing host route "
                             << index << "; host route remaining size = " << m_hostRoutes.size());
                return;
            }
            tmp++;
        }
    }
    index -= m_hostRoutes.size();
    uint32_t tmp = 0;
    for (auto j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j++)
    {
        if (tmp == index)
        {
            NS_LOG_LOGIC("Removing route " << index << "; size = " << m_networkRoutes.size());
            delete *j;
            m_networkRoutes.erase(j);
            NS_LOG_LOGIC("Done removing network route "
                         << index << "; network route remaining size = " << m_networkRoutes.size());
            return;
        }
        tmp++;
    }
    index -= m_networkRoutes.size();
    tmp = 0;
    for (auto k = m_ASexternalRoutes.begin(); k != m_ASexternalRoutes.end(); k++)
    {
        if (tmp == index)
        {
            NS_LOG_LOGIC("Removing route " << index << "; size = " << m_ASexternalRoutes.size());
            delete *k;
            m_ASexternalRoutes.erase(k);
            NS_LOG_LOGIC("Done removing network route "
                         << index << "; network route remaining size = " << m_networkRoutes.size());
            return;
        }
        tmp++;
    }
    NS_ASSERT(false);
}

template <typename T>
int64_t
GlobalRouting<T>::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_rand->SetStream(stream);
    return 1;
}

template <typename T>
void
GlobalRouting<T>::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (auto i = m_hostRoutes.begin(); i != m_hostRoutes.end(); i = m_hostRoutes.erase(i))
    {
        delete (*i);
    }
    for (auto j = m_networkRoutes.begin(); j != m_networkRoutes.end(); j = m_networkRoutes.erase(j))
    {
        delete (*j);
    }
    for (auto l = m_ASexternalRoutes.begin(); l != m_ASexternalRoutes.end();
         l = m_ASexternalRoutes.erase(l))
    {
        delete (*l);
    }

    IpRoutingProtocol::DoDispose();
}

// Formatted like output of "route -n" command
template <typename T>
void
GlobalRouting<T>::PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit) const
{
    NS_LOG_FUNCTION(this << stream);
    std::ostream* os = stream->GetStream();
    // Copy the current ostream state
    std::ios oldState(nullptr);
    oldState.copyfmt(*os);

    *os << std::resetiosflags(std::ios::adjustfield) << std::setiosflags(std::ios::left);

    std::string name;
    if constexpr (IsIpv4)
    {
        name = "Ipv4";
    }
    else
    {
        name = "Ipv6";
    }

    *os << "Node: " << m_ip->template GetObject<Node>()->GetId() << ", Time: " << Now().As(unit)
        << ", Local time: " << m_ip->template GetObject<Node>()->GetLocalTime().As(unit) << ", "
        << name << "GlobalRouting table" << std::endl;

    if (GetNRoutes() > 0)
    {
        *os << "Destination                    Next Hop                   Flag Met Ref Use If"
            << std::endl;
        for (uint32_t j = 0; j < GetNRoutes(); j++)
        {
            std::ostringstream dest;
            std::ostringstream gw;
            std::ostringstream mask;
            std::ostringstream flags;
            IpRoutingTableEntry route = GetRoute(j);
            if constexpr (IsIpv4)
            {
                dest << route.GetDest();
                *os << std::setw(16) << dest.str();
                gw << route.GetGateway();
                *os << std::setw(16) << gw.str();
                mask << route.GetDestNetworkMask();
                *os << std::setw(16) << mask.str();
                ;
            }
            else
            {
                dest << route.GetDest() << "/"
                     << int(route.GetDestNetworkPrefix().GetPrefixLength());
                *os << std::setw(31) << dest.str();
                gw << route.GetGateway();
                *os << std::setw(27) << gw.str();
            }
            flags << "U";
            if (route.IsHost())
            {
                flags << "H";
            }
            else if (route.IsGateway())
            {
                flags << "G";
            }
            *os << std::setw(6) << flags.str();
            // Metric not implemented
            *os << "-"
                << "   ";
            // Ref ct not implemented
            *os << "-"
                << "   ";
            // Use not implemented
            *os << "-"
                << "   ";
            if (!Names::FindName(m_ip->GetNetDevice(route.GetInterface())).empty())
            {
                *os << Names::FindName(m_ip->GetNetDevice(route.GetInterface()));
            }
            else
            {
                *os << route.GetInterface();
            }
            *os << std::endl;
        }
    }
    *os << std::endl;
    // Restore the previous ostream state
    (*os).copyfmt(oldState);
}

template <typename T>
Ptr<typename GlobalRouting<T>::IpRoute>
GlobalRouting<T>::RouteOutput(Ptr<Packet> p,
                              const IpHeader& header,
                              Ptr<NetDevice> oif,
                              Socket::SocketErrno& sockerr)
{
    NS_LOG_FUNCTION(this << p << &header << oif << &sockerr);
    //
    // First, see if this is a multicast packet we have a route for.  If we
    // have a route, then send the packet down each of the specified interfaces.
    //
    if (header.GetDestination().IsMulticast())
    {
        NS_LOG_LOGIC("Multicast destination-- returning false");
        return nullptr; // Let other routing protocols try to handle this
    }
    //
    // See if this is a unicast packet we have a route for.
    //
    NS_LOG_LOGIC("Unicast destination- looking up");
    Ptr<IpRoute> rtentry = LookupGlobal(header.GetDestination(), oif);
    if (rtentry)
    {
        sockerr = Socket::ERROR_NOTERROR;
    }
    else
    {
        sockerr = Socket::ERROR_NOROUTETOHOST;
    }
    return rtentry;
}

template <typename T>
bool
GlobalRouting<T>::RouteInput(Ptr<const Packet> p,
                             const IpHeader& header,
                             Ptr<const NetDevice> idev,
                             const UnicastForwardCallback& ucb,
                             const MulticastForwardCallback& mcb,
                             const LocalDeliverCallback& lcb,
                             const ErrorCallback& ecb)
{
    NS_LOG_FUNCTION(this << p << header << header.GetSource() << header.GetDestination() << idev
                         << &lcb << &ecb);
    // Check if input device supports IP
    NS_ASSERT(m_ip->GetInterfaceForDevice(idev) >= 0);
    uint32_t iif = m_ip->GetInterfaceForDevice(idev);
    if constexpr (IsIpv4)
    {
        if (m_ip->IsDestinationAddress(header.GetDestination(), iif))
        {
            if (!lcb.IsNull())
            {
                NS_LOG_LOGIC("Local delivery to " << header.GetDestination());
                lcb(p, header, iif);
                return true;
            }
            else
            {
                // The local delivery callback is null.  This may be a multicast
                // or broadcast packet, so return false so that another
                // multicast routing protocol can handle it.  It should be possible
                // to extend this to explicitly check whether it is a unicast
                // packet, and invoke the error callback if so
                return false;
            }
        }
    }
    else
    {
        // for ipv6 the local delivery callback is never called. Local delivery is already handled
        // in Ipv6l3Protocol::Receive() for now do nothing
    }
    // Check if input device supports IP forwarding
    if (!m_ip->IsForwarding(iif))
    {
        NS_LOG_LOGIC("Forwarding disabled for this interface");
        ecb(p, header, Socket::ERROR_NOROUTETOHOST);
        return true;
    }
    // Next, try to find a route
    NS_LOG_LOGIC("Unicast destination- looking up global route");
    Ptr<IpRoute> rtentry = LookupGlobal(header.GetDestination());
    if (rtentry)
    {
        NS_LOG_LOGIC("Found unicast destination- calling unicast callback");
        if constexpr (IsIpv4)
        {
            ucb(rtentry, p, header);
        }
        else
        {
            ucb(idev, rtentry, p, header);
        }
        return true;
    }
    else
    {
        NS_LOG_LOGIC("Did not find unicast destination- returning false");
        return false; // Let other routing protocols try to handle this
                      // route request.
    }
}

template <typename T>
void
GlobalRouting<T>::NotifyInterfaceUp(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    if (m_respondToInterfaceEvents && Simulator::Now().GetSeconds() > 0) // avoid startup events
    {
        GlobalRouteManager<IpManager>::DeleteGlobalRoutes();
        GlobalRouteManager<IpManager>::BuildGlobalRoutingDatabase();
        GlobalRouteManager<IpManager>::InitializeRoutes();
    }
}

template <typename T>
void
GlobalRouting<T>::NotifyInterfaceDown(uint32_t i)
{
    NS_LOG_FUNCTION(this << i);
    if (m_respondToInterfaceEvents && Simulator::Now().GetSeconds() > 0) // avoid startup events
    {
        GlobalRouteManager<IpManager>::DeleteGlobalRoutes();
        GlobalRouteManager<IpManager>::BuildGlobalRoutingDatabase();
        GlobalRouteManager<IpManager>::InitializeRoutes();
    }
}

template <typename T>
void
GlobalRouting<T>::NotifyAddAddress(uint32_t interface, IpInterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    if (m_respondToInterfaceEvents && Simulator::Now().GetSeconds() > 0) // avoid startup events
    {
        GlobalRouteManager<IpManager>::DeleteGlobalRoutes();
        GlobalRouteManager<IpManager>::BuildGlobalRoutingDatabase();
        GlobalRouteManager<IpManager>::InitializeRoutes();
    }
}

template <typename T>
void
GlobalRouting<T>::NotifyRemoveAddress(uint32_t interface, IpInterfaceAddress address)
{
    NS_LOG_FUNCTION(this << interface << address);
    if (m_respondToInterfaceEvents && Simulator::Now().GetSeconds() > 0) // avoid startup events
    {
        GlobalRouteManager<IpManager>::DeleteGlobalRoutes();
        GlobalRouteManager<IpManager>::BuildGlobalRoutingDatabase();
        GlobalRouteManager<IpManager>::InitializeRoutes();
    }
}

template <typename T>
void
GlobalRouting<T>::NotifyAddRoute(Ipv6Address dst,
                                 Ipv6Prefix mask,
                                 Ipv6Address nextHop,
                                 uint32_t interface,
                                 Ipv6Address prefixToUse)
{
}

template <typename T>
void
GlobalRouting<T>::NotifyRemoveRoute(Ipv6Address dst,
                                    Ipv6Prefix prefix,
                                    Ipv6Address nextHop,
                                    uint32_t ifIndex,
                                    Ipv6Address origin)
{
    // no-op or real behavior
}

template <typename T>
void
GlobalRouting<T>::SetIpv4(Ptr<Ip> ipv4)
{
    NS_LOG_FUNCTION(this << ipv4);
    NS_ASSERT(!m_ip && ipv4);

    m_ip = ipv4;
}

template <typename T>
void
GlobalRouting<T>::SetIpv6(Ptr<Ip> ipv6)
{
    NS_LOG_FUNCTION(this << ipv6);
    NS_ASSERT(!m_ip && ipv6);
    m_ip = ipv6;
}

// template <typename T>
// void
// GlobalRouting<T>::InitializeRouters()
//{
//     NS_LOG_FUNCTION(this);
//       for (uint32_t i = 0; i < m_ip->GetNInterfaces(); i++)
//     {
//         m_ip->SetForwarding(i, true);
//     }
// }

NS_OBJECT_TEMPLATE_CLASS_DEFINE(GlobalRouting, Ipv4RoutingProtocol);
NS_OBJECT_TEMPLATE_CLASS_DEFINE(GlobalRouting, Ipv6RoutingProtocol);

} // namespace ns3
