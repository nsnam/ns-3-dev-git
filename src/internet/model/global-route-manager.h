/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Craig Dowell (craigdo@ee.washington.edu)
 *           Tom Henderson (tomhend@u.washington.edu)
 */

#ifndef GLOBAL_ROUTE_MANAGER_H
#define GLOBAL_ROUTE_MANAGER_H

#include "ns3/ipv4-routing-helper.h"

#include <cstdint>

namespace ns3
{

/**
 * @ingroup globalrouting
 *
 * @brief A global global router
 *
 * This singleton object can query interface each node in the system
 * for a GlobalRouter interface.  For those nodes, it fetches one or
 * more Link State Advertisements and stores them in a local database.
 * Then, it can compute shortest paths on a per-node basis to all routers,
 * and finally configure each of the node's forwarding tables.
 *
 * The design is guided by OSPFv2 \RFC{2328} section 16.1.1 and quagga ospfd.
 */
class GlobalRouteManager
{
  public:
    // Delete copy constructor and assignment operator to avoid misuse
    GlobalRouteManager(const GlobalRouteManager&) = delete;
    GlobalRouteManager& operator=(const GlobalRouteManager&) = delete;

    /**
     * @brief Allocate a 32-bit router ID from monotonically increasing counter.
     * @returns A new new RouterId.
     */
    static uint32_t AllocateRouterId();

    /**
     * @brief Delete all static routes on all nodes that have a
     * GlobalRouterInterface
     *
     */
    static void DeleteGlobalRoutes();

    /**
     * @brief Build the routing database by gathering Link State Advertisements
     * from each node exporting a GlobalRouter interface.
     */
    static void BuildGlobalRoutingDatabase();

    /**
     * @brief Compute routes using a Dijkstra SPF computation and populate
     * per-node forwarding tables
     */
    static void InitializeRoutes();

    /**
     * @brief Reset the router ID counter to zero. This should only be called by tests to reset the
     * router ID counter between simulations within the same program. This function should not be
     * called In typical simulations or when using the GlobalRouting helper.
     */
    static void ResetRouterId();

    /**
     * @brief prints the path from this node to the destination node at a particular time.
     * @param sourceNode The source node.
     * @param dest The IPv4 address of the destination node.
     * @param stream The output stream to which the routing path will be written.
     * @param nodeIdLookup Print the Node Id
     * @param unit The time unit for timestamps in the printed output.
     * @see Ipv4GlobalRoutingHelper::PrintRoute
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ipv4Address dest,
                           Ptr<OutputStreamWrapper> stream,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

    /**
     *@brief prints the path from this node to the destination node at a particular time.
     * @param sourceNode The source node.
     * @param dest The IPv4 address of the destination node.
     * @param nodeIdLookup Print the Node Id
     * @param unit The time unit for timestamps in the printed output.
     * @see Ipv4GlobalRoutingHelper::PrintRoute
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ipv4Address dest,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

    /**
     *@brief prints the path from this node to the destination node at a particular time.
     * @param sourceNode  The source node.
     * @param dest The destination node.
     * @param stream The output stream to which the routing path will be written.
     * @param nodeIdLookup Print the Node Id
     * @param unit The time unit for timestamps in the printed output.
     * @see Ipv4GlobalRoutingHelper::PrintRoute
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ptr<Node> dest,
                           Ptr<OutputStreamWrapper> stream,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

    /**
     *@brief prints the path from this node to the destination node at a particular time.
     * @param sourceNode The source node.
     * @param dest The destination node.
     * @param nodeIdLookup Print the Node Id
     * @param unit The time unit for timestamps in the printed output.
     * @see Ipv4GlobalRoutingHelper::PrintRoute
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ptr<Node> dest,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

  private:
    static uint32_t routerId; //!< Router ID counter
};

} // namespace ns3

#endif /* GLOBAL_ROUTE_MANAGER_H */
