/*
 * Copyright 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

/**
 * @ingroup ipv4Routing
 * @defgroup globalrouting Global Routing.
 *
 * Performs pre-simulation static route computation
 * on a layer-3 IPv4 topology.
 *
 * @section model Model
 *
 * ns-3 global routing performs pre-simulation static route computation
 * on a layer-3 IPv4 topology.  The user API from the script level is
 * fairly minimal; once a topology has been constructed and addresses
 * assigned, the user may call ns3::GlobalRouteManager::PopulateRoutingTables()
 * and the simulator will initialize the routing database and set up
 * static unicast forwarding tables for each node.
 *
 * The model assumes that all nodes on an ns-3 channel are reachable to
 * one another, regardless of whether the nodes can use the channel
 * successfully (in the case of wireless).  Therefore, this model
 * should typically be used only on wired topologies.  Layer-2 bridge
 * devices are supported.  API does not yet exist to control the subset
 * of a topology to which this global static routing is applied.
 *
 * If the topology changes during the simulation, by default, routing
 * will not adjust.  There are two ways to make it adjust.
 * - Set the attribute Ipv4GlobalRouting::RespondToInterfaceEvents to true
 * - Manually call the sequence of GlobalRouteManager methods to delete global
 *   routes, build global routing database, and initialize routes.
 *   There is a helper method that encapsulates this
 *   (Ipv4GlobalRoutingHelper::RecomputeRoutingTables())
 *
 * @section api API and Usage
 *
 * Users must include ns3/global-route-manager.h header file.  After the
 * IPv4 topology has been built and addresses assigned, users call
 * ns3::GlobalRouteManager::PopulateRoutingTables (), prior to the
 * ns3::Simulator::Run() call.
 *
 * There are two attributes of Ipv4GlobalRouting that govern behavior.
 * - Ipv4GlobalRouting::RandomEcmpRouting
 * - Ipv4GlobalRouting::RespondToInterfaceEvents
 *
 * @section impl Implementation
 *
 * A singleton object, ns3::GlobalRouteManager, builds a global routing
 * database of information about the topology, and executes a Dijkstra
 * Shortest Path First (SPF) algorithm on the topology for each node, and
 * stores the computed routes in each node's IPv4 forwarding table by
 * making use of the routing API in class ns3::Ipv4.
 *
 * The nodes that export data are those that have had an ns3::GlobalRouter
 * object aggregated to them.  The ns3::GlobalRouter can be thought of
 * as a per-node agent that exports topology information to the
 * ns3::GlobalRouteManager.  When it comes time to build the global
 * routing database, the list of nodes is iterated and each node with
 * an ns3::GlobalRouter object is asked to export routing information
 * concerning the links to which it is attached.
 *
 * The format of the data exported conforms to the OSPFv2 standard
 * \RFC{2328}.  In particular, the
 * information is exported in the form of ns3::GlobalLSA objects that
 * semantically match the Link State Advertisements of OSPF.
 *
 * By using a standard data format for reporting topology, existing
 * OSPF route computation code can be reused, and that is what is done
 * by the ns3::GlobalRouteManager.  The main computation functions are
 * ported from the quagga routing suite (https://www.nongnu.org/quagga/).
 *
 */

#ifndef GLOBAL_ROUTING_H
#define GLOBAL_ROUTING_H

#include "global-route-manager.h"
#include "ipv4-header.h"
#include "ipv4-routing-protocol.h"
#include "ipv4.h"
#include "ipv6-interface-address.h"
#include "ipv6-routing-protocol.h"
#include "ipv6-routing-table-entry.h"

#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/ptr.h"
#include "ns3/random-variable-stream.h"

#include <list>
#include <stdint.h>

namespace ns3
{

class Packet;
class NetDevice;
class Ipv4Interface;
class Ipv4Address;
class Ipv4Header;
class Ipv4RoutingTableEntry;
class Ipv4MulticastRoutingTableEntry;
class Node;
template <typename>
class GlobalRouteManagerImpl;

/**
 * @ingroup ipv4
 *
 * @brief Global routing protocol for IPv4 stacks.
 *
 * In ns-3 we have the concept of a pluggable routing protocol.  Routing
 * protocols are added to a list maintained by the Ipv4L3Protocol.  Every
 * stack gets one routing protocol for free -- the Ipv4StaticRouting routing
 * protocol is added in the constructor of the Ipv4L3Protocol (this is the
 * piece of code that implements the functionality of the IP layer).
 *
 * As an option to running a dynamic routing protocol, a GlobalRouteManager
 * object has been created to allow users to build routes for all participating
 * nodes.  One can think of this object as a "routing oracle"; it has
 * an omniscient view of the topology, and can construct shortest path
 * routes between all pairs of nodes.  These routes must be stored
 * somewhere in the node, so therefore this class Ipv4GlobalRouting
 * is used as one of the pluggable routing protocols.  It is kept distinct
 * from Ipv4StaticRouting because these routes may be dynamically cleared
 * and rebuilt in the middle of the simulation, while manually entered
 * routes into the Ipv4StaticRouting may need to be kept distinct.
 *
 * This class deals with Ipv4 unicast routes only.
 *
 * @see Ipv4RoutingProtocol
 * @see GlobalRouteManager
 */
template <typename T>
class GlobalRouting : public std::enable_if_t<std::is_same_v<Ipv4RoutingProtocol, T> ||
                                                  std::is_same_v<Ipv6RoutingProtocol, T>,
                                              T>
{
    template <typename>
    friend class GlobalRouteManagerImpl;

    /// Alias for determining whether the parent is Ipv4RoutingHelper or Ipv6RoutingHelper
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4RoutingProtocol, T>;
    /// Alias for Ipv4 and Ipv6 classes
    using Ip = typename std::conditional_t<IsIpv4, Ipv4, Ipv6>;
    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;
    /// Alias for Ipv4RoutingProtocol and Ipv6RoutingProtocol classes
    using IpRoutingProtocol =
        typename std::conditional_t<IsIpv4, Ipv4RoutingProtocol, Ipv6RoutingProtocol>;

    /// Alias for Ipv4Header and Ipv6Header classes
    using IpHeader = typename std::conditional_t<IsIpv4, Ipv4Header, Ipv6Header>;

    /// Alias for Ipv4Route and Ipv6Route classes
    using IpRoute = typename std::conditional_t<IsIpv4, Ipv4Route, Ipv6Route>;

    /// Alias for Ipv4Mask And Ipv6Prefix
    using IpMaskOrPrefix = typename std::conditional_t<IsIpv4, Ipv4Mask, Ipv6Prefix>;

    /// Alias for Ipv4RoutingTableEntry and Ipv6RoutingTableEntry classes
    using IpRoutingTableEntry =
        typename std::conditional_t<IsIpv4, Ipv4RoutingTableEntry, Ipv6RoutingTableEntry>;

    /// Alias for Ipv4Manager and Ipv6Manager classes
    using IpManager = typename std::conditional_t<IsIpv4, Ipv4Manager, Ipv6Manager>;

    /// Alias for Ipv4InterfaceAddress and Ipv6InterfaceAddress classes
    using IpInterfaceAddress =
        typename std::conditional_t<IsIpv4, Ipv4InterfaceAddress, Ipv6InterfaceAddress>;

  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    /**
     * @brief Construct an empty Ipv4GlobalRouting routing protocol,
     *
     * The Ipv4GlobalRouting class supports host and network unicast routes.
     * This method initializes the lists containing these routes to empty.
     *
     * @see Ipv4GlobalRouting
     */
    GlobalRouting();
    ~GlobalRouting();

    // These methods inherited from base class

    /* From Ipv4RoutingProtocol and Ipv6RoutingProtocol */

    /**
     * @brief Query routing cache for an existing route, for an outbound packet
     * @param p packet to be routed.  Note that this method may modify the packet.
     *          Callers may also pass in a null pointer.
     * @param header input parameter (used to form key to search for the route)
     * @param oif Output interface Netdevice.  May be zero, or may be bound via
     *            socket options to a particular output interface.
     * @param sockerr Output parameter; socket errno
     *
     * @returns a code that indicates what happened in the lookup
     *
     * @sa Ipv4RoutingProtocol::RouteOutput
     * @sa Ipv6RoutingProtocol::RouteOutput
     */
    Ptr<IpRoute> RouteOutput(Ptr<Packet> p,
                             const IpHeader& header,
                             Ptr<NetDevice> oif,
                             Socket::SocketErrno& sockerr);

    /// Callback for IPv4 unicast packets to be forwarded
    typedef Callback<void, Ptr<IpRoute>, Ptr<const Packet>, const IpHeader&>
        UnicastForwardCallbackv4;

    /// Callback for IPv6 unicast packets to be forwarded
    typedef Callback<void, Ptr<const NetDevice>, Ptr<IpRoute>, Ptr<const Packet>, const IpHeader&>
        UnicastForwardCallbackv6;

    /// Callback for unicast packets to be forwarded
    typedef typename std::conditional_t<IsIpv4, UnicastForwardCallbackv4, UnicastForwardCallbackv6>
        UnicastForwardCallback;

    /// Callback for IPv4 multicast packets to be forwarded
    typedef Callback<void, Ptr<Ipv4MulticastRoute>, Ptr<const Packet>, const IpHeader&>
        MulticastForwardCallbackv4;

    /// Callback for IPv6 multicast packets to be forwarded
    typedef Callback<void,
                     Ptr<const NetDevice>,
                     Ptr<Ipv6MulticastRoute>,
                     Ptr<const Packet>,
                     const IpHeader&>
        MulticastForwardCallbackv6;

    /// Callback for multicast packets to be forwarded
    typedef
        typename std::conditional_t<IsIpv4, MulticastForwardCallbackv4, MulticastForwardCallbackv6>
            MulticastForwardCallback;

    /// Callback for packets to be locally delivered
    typedef Callback<void, Ptr<const Packet>, const IpHeader&, uint32_t> LocalDeliverCallback;

    /// Callback for routing errors (e.g., no route found)
    typedef Callback<void, Ptr<const Packet>, const IpHeader&, Socket::SocketErrno> ErrorCallback;

    /**
     * @brief Route an input packet (to be forwarded or locally delivered)
     * @param p received packet
     * @param header input parameter used to form a search key for a route
     * @param idev Pointer to ingress network device
     * @param ucb Callback for the case in which the packet is to be forwarded
     *            as unicast
     * @param mcb Callback for the case in which the packet is to be forwarded
     *            as multicast
     * @param lcb Callback for the case in which the packet is to be locally
     *            delivered
     * @param ecb Callback to call if there is an error in forwarding
     *
     * @returns true if GlobalRouting class takes responsibility for
     *          forwarding or delivering the packet, false otherwise
     *
     * @sa Ipv4RoutingProtocol::RouteInput
     * @sa Ipv6RoutingProtocol::RouteInput
     */
    bool RouteInput(Ptr<const Packet> p,
                    const IpHeader& header,
                    Ptr<const NetDevice> idev,
                    const UnicastForwardCallback& ucb,
                    const MulticastForwardCallback& mcb,
                    const LocalDeliverCallback& lcb,
                    const ErrorCallback& ecb);

    /**
     * @param interface the index of the interface we are being notified about
     *
     * @sa Ipv4RoutingProtocol::NotifyInterfaceUp
     * @sa Ipv6RoutingProtocol::NotifyInterfaceUp
     */
    void NotifyInterfaceUp(uint32_t interface);
    /**
     * @param interface the index of the interface we are being notified about
     *
     * @sa Ipv4RoutingProtocol::NotifyInterfaceDown
     * @sa Ipv6RoutingProtocol::NotifyInterfaceDown
     */
    void NotifyInterfaceDown(uint32_t interface);
    /**
     * @param interface the index of the interface we are being notified about
     * @param address a new address being added to an interface
     *
     * @sa Ipv4RoutingProtocol::NotifyAddAddress
     * @sa Ipv6RoutingProtocol::NotifyAddAddress
     */
    void NotifyAddAddress(uint32_t interface, IpInterfaceAddress address);
    /**
     * @param interface the index of the interface we are being notified about
     * @param address a new address being added to an interface
     *
     * @sa Ipv4RoutingProtocol::NotifyRemoveAddress
     * @sa Ipv6RoutingProtocol::NotifyRemoveAddress
     */
    void NotifyRemoveAddress(uint32_t interface, IpInterfaceAddress address);

    /**
     * @brief Notify a new route.
     *
     * Typically this is used to add another route from IPv6 stack (i.e. ICMPv6
     * redirect case, ...).
     * @param dst destination address
     * @param mask destination mask
     * @param nextHop nextHop for this destination
     * @param interface output interface
     * @param prefixToUse prefix to use as source with this route
     */
    virtual void NotifyAddRoute(Ipv6Address dst,
                                Ipv6Prefix mask,
                                Ipv6Address nextHop,
                                uint32_t interface,
                                Ipv6Address prefixToUse = Ipv6Address::GetZero());
    /**
     * @brief Notify route removing.
     * @param dst destination address
     * @param mask destination mask
     * @param nextHop nextHop for this destination
     * @param interface output interface
     * @param prefixToUse prefix to use as source with this route
     */
    virtual void NotifyRemoveRoute(Ipv6Address dst,
                                   Ipv6Prefix mask,
                                   Ipv6Address nextHop,
                                   uint32_t interface,
                                   Ipv6Address prefixToUse = Ipv6Address::GetZero());

    /* From IPv4RoutingProtocol */
    /**
     * @brief Typically, invoked directly or indirectly from ns3::Ipv4::SetRoutingProtocol
     *
     * @param ipv4 the ipv4 object this routing protocol is being associated with
     *
     * @sa Ipv4RoutingProtocol::SetIpv4
     */
    virtual void SetIpv4(Ptr<Ip> ipv4);

    /* From IPv6RoutingProtocol */
    /**
     * @brief Typically, invoked directly or indirectly from ns3::Ipv6::SetRoutingProtocol
     *
     * @param ipv6 the ipv6 object this routing protocol is being associated with
     *
     * @sa Ipv6RoutingProtocol::SetIpv6
     */
    virtual void SetIpv6(Ptr<Ip> ipv6);

    /**
     * @brief Print the Routing Table entries
     *
     * @param stream The ostream the Routing table is printed to
     * @param unit The time unit to be used in the report
     *
     * @sa Ipv4RoutingProtocol::PrintRoutingTable
     * @sa Ipv6RoutingProtocol::PrintRoutingTable
     */
    void PrintRoutingTable(Ptr<OutputStreamWrapper> stream, Time::Unit unit = Time::S) const;

    /**
     * @brief Add a host route to the global routing table.
     *
     * @param dest The Ipv4Address destination for this route.
     * @param nextHop The Ipv4Address of the next hop in the route.
     * @param interface The network interface index used to send packets to the
     * destination.
     *
     * @see Ipv4Address
     */
    void AddHostRouteTo(IpAddress dest, IpAddress nextHop, uint32_t interface);
    /**
     * @brief Add a host route to the global routing table.
     *
     * @param dest The Ipv4Address destination for this route.
     * @param interface The network interface index used to send packets to the
     * destination.
     *
     * @see Ipv4Address
     */
    void AddHostRouteTo(IpAddress dest, uint32_t interface);

    /**
     * @brief Add a network route to the global routing table.
     *
     * @param network The Ipv4Address network for this route.
     * @param networkMask The Ipv4Mask to extract the network.
     * @param nextHop The next hop in the route to the destination network.
     * @param interface The network interface index used to send packets to the
     * destination.
     *
     * @see Ipv4Address
     */
    void AddNetworkRouteTo(IpAddress network,
                           IpMaskOrPrefix networkMask,
                           IpAddress nextHop,
                           uint32_t interface);

    /**
     * @brief Add a network route to the global routing table.
     *
     * @param network The Ipv4Address network for this route.
     * @param networkMask The Ipv4Mask to extract the network.
     * @param interface The network interface index used to send packets to the
     * destination.
     *
     * @see Ipv4Address
     */
    void AddNetworkRouteTo(IpAddress network, IpMaskOrPrefix networkMask, uint32_t interface);

    /**
     * @brief Add an external route to the global routing table.
     *
     * @param network The Ipv4Address network for this route.
     * @param networkMask The Ipv4Mask to extract the network.
     * @param nextHop The next hop Ipv4Address
     * @param interface The network interface index used to send packets to the
     * destination.
     */
    void AddASExternalRouteTo(IpAddress network,
                              IpMaskOrPrefix networkMask,
                              IpAddress nextHop,
                              uint32_t interface);

    /**
     * @brief Get the number of individual unicast routes that have been added
     * to the routing table.
     *
     * @warning The default route counts as one of the routes.
     * @returns the number of routes
     */
    uint32_t GetNRoutes() const;

    /**
     * @brief Get a route from the global unicast routing table.
     *
     * Externally, the unicast global routing table appears simply as a table with
     * n entries.  The one subtlety of note is that if a default route has been set
     * it will appear as the zeroth entry in the table.  This means that if you
     * add only a default route, the table will have one entry that can be accessed
     * either by explicitly calling GetDefaultRoute () or by calling GetRoute (0).
     *
     * Similarly, if the default route has been set, calling RemoveRoute (0) will
     * remove the default route.
     *
     * @param i The index (into the routing table) of the route to retrieve.  If
     * the default route has been set, it will occupy index zero.
     * @return If route is set, a pointer to that Ipv4RoutingTableEntry is returned, otherwise
     * a zero pointer is returned.
     *
     * @see Ipv4RoutingTableEntry
     * @see Ipv4GlobalRouting::RemoveRoute
     */
    IpRoutingTableEntry* GetRoute(uint32_t i) const;

    /**
     * @brief Remove a route from the global unicast routing table.
     *
     * Externally, the unicast global routing table appears simply as a table with
     * n entries.  The one subtlety of note is that if a default route has been set
     * it will appear as the zeroth entry in the table.  This means that if the
     * default route has been set, calling RemoveRoute (0) will remove the
     * default route.
     *
     * @param i The index (into the routing table) of the route to remove.  If
     * the default route has been set, it will occupy index zero.
     *
     * @see Ipv4RoutingTableEntry
     * @see Ipv4GlobalRouting::GetRoute
     * @see Ipv4GlobalRouting::AddRoute
     */
    void RemoveRoute(uint32_t i);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    int64_t AssignStreams(int64_t stream);

  protected:
    /**
     * @sa Ipv4RoutingProtocol::DoDispose
     * @sa Ipv6RoutingProtocol::DoDispose
     */
    void DoDispose();

  private:
    /// Set to true if packets are randomly routed among ECMP; set to false for using only one route
    /// consistently
    bool m_randomEcmpRouting;
    /// Set to true if this interface should respond to interface events by globally recomputing
    /// routes
    bool m_respondToInterfaceEvents;
    /// A uniform random number generator for randomly routing packets among ECMP
    Ptr<UniformRandomVariable> m_rand;

    /// container of Ipv4RoutingTableEntry (routes to hosts)
    typedef std::list<IpRoutingTableEntry*> HostRoutes;
    /// const iterator of container of Ipv4RoutingTableEntry (routes to hosts)
    typedef std::list<IpRoutingTableEntry*>::const_iterator HostRoutesCI;
    /// iterator of container of Ipv4RoutingTableEntry (routes to hosts)
    typedef std::list<IpRoutingTableEntry*>::iterator HostRoutesI;

    /// container of Ipv4RoutingTableEntry (routes to networks)
    typedef std::list<IpRoutingTableEntry*> NetworkRoutes;
    /// const iterator of container of Ipv4RoutingTableEntry (routes to networks)
    typedef std::list<IpRoutingTableEntry*>::const_iterator NetworkRoutesCI;
    /// iterator of container of Ipv4RoutingTableEntry (routes to networks)
    typedef std::list<IpRoutingTableEntry*>::iterator NetworkRoutesI;

    /// container of Ipv4RoutingTableEntry (routes to external AS)
    typedef std::list<IpRoutingTableEntry*> ASExternalRoutes;
    /// const iterator of container of Ipv4RoutingTableEntry (routes to external AS)
    typedef std::list<IpRoutingTableEntry*>::const_iterator ASExternalRoutesCI;
    /// iterator of container of Ipv4RoutingTableEntry (routes to external AS)
    typedef std::list<IpRoutingTableEntry*>::iterator ASExternalRoutesI;

    /**
     * @brief Lookup in the forwarding table for destination.
     * @param dest destination address
     * @param oif output interface if any (put 0 otherwise)
     * @return Ipv4Route to route the packet to reach dest address
     */
    Ptr<IpRoute> LookupGlobal(IpAddress dest, Ptr<NetDevice> oif = nullptr);

    HostRoutes m_hostRoutes;             //!< Routes to hosts
    NetworkRoutes m_networkRoutes;       //!< Routes to networks
    ASExternalRoutes m_ASexternalRoutes; //!< External routes imported

    Ptr<Ip> m_ip; //!< associated IPv4 instance
};

/**
 * @ingroup ipv4
 * Create the typedef Ipv4GlobalRouting with T as Ipv4RoutingProtocol
 */
typedef GlobalRouting<Ipv4RoutingProtocol> Ipv4GlobalRouting;

/**
 * @ingroup ipv6
 * Create the typedef Ipv6GlobalRouting with T as Ipv6RoutingProtocol
 */
typedef GlobalRouting<Ipv6RoutingProtocol> Ipv6GlobalRouting;

} // Namespace ns3

#endif /* GLOBAL_ROUTING_H */
