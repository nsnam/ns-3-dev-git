/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef IPV4_GLOBAL_ROUTING_HELPER_H
#define IPV4_GLOBAL_ROUTING_HELPER_H

#include "ipv4-routing-helper.h"

#include "ns3/node-container.h"

namespace ns3
{

/**
 * @ingroup ipv4Helpers
 *
 * @brief Helper class that adds ns3::Ipv4GlobalRouting objects
 */
class Ipv4GlobalRoutingHelper : public Ipv4RoutingHelper
{
  public:
    /**
     * @brief Construct a GlobalRoutingHelper to make life easier for managing
     * global routing tasks.
     */
    Ipv4GlobalRoutingHelper();

    /**
     * @brief Construct a GlobalRoutingHelper from another previously initialized
     * instance (Copy Constructor).
     * @param o object to be copied
     */
    Ipv4GlobalRoutingHelper(const Ipv4GlobalRoutingHelper& o);

    // Delete assignment operator to avoid misuse
    Ipv4GlobalRoutingHelper& operator=(const Ipv4GlobalRoutingHelper&) = delete;

    /**
     * @returns pointer to clone of this Ipv4GlobalRoutingHelper
     *
     * This method is mainly for internal use by the other helpers;
     * clients are expected to free the dynamic memory allocated by this method
     */
    Ipv4GlobalRoutingHelper* Copy() const override;

    /**
     * @param node the node on which the routing protocol will run
     * @returns a newly-created routing protocol
     *
     * This method will be called by ns3::InternetStackHelper::Install
     */
    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override;

    /**
     * @brief Build a routing database and initialize the routing tables of
     * the nodes in the simulation.  Makes all nodes in the simulation into
     * routers.
     *
     * All this function does is call the functions
     * BuildGlobalRoutingDatabase () and  InitializeRoutes ().
     *
     */
    static void PopulateRoutingTables();

    /**
     * @brief prints the routing path for a source and destination to the standard cout output
     * stream. If the routing path does not exist, it prints that the path does not exist between
     * the nodes in the ostream. This is a scheduler for the PrintRoute call.
     *
     * @param sourceNode the source node
     * @param dest the IPv4 destination address
     * @param nodeIdLookup print the node id
     * @param unit the time unit to be used in the report
     *
     * @details This method calls the PrintRoutingPath() method of the
     * Ipv4GlobalRouting for the source and destination to provide
     * the routing path at the specified time.
     * Early return will be triggered if inputs are invalid. for example:
     * if source or destination nodes do not exist, source or destination nodes lack IPv4 instances,
     * or source node lacks a global routing instance.
     * If the destination node has multiple IpAddresses, the routing path will be printed for the
     * first Ip address of the destination Ipv4 Stack.
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ipv4Address dest,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

    /**
     * @brief prints the routing path for a source and destination.
     * @copydetails PrintRoute(Ptr<Node>, Ipv4Address, bool, Time::Unit)
     * @param stream the output stream object to use
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ipv4Address dest,
                           Ptr<OutputStreamWrapper> stream,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

    /**
     * @copybrief PrintRoute(Ptr<Node>, Ipv4Address,  bool,
     * Time::Unit)
     * If the routing path does not exist, it prints that the path does not exist between
     * the nodes in the ostream. This is a scheduler for the PrintRoute call.
     *
     * @param sourceNode the source node
     * @param dest the destination node
     * @param nodeIdLookup print the node id
     * @param unit the time unit to be used in the report
     *
     * @details This method calls the PrintRoutingPath() method of the
     * Ipv4GlobalRouting for the source and destination to provide
     * the routing path at the specified time.
     * Early return will be triggered if inputs are invalid. for example:
     * if source or destination nodes do not exist, source or destination nodes lack IPv4 instances,
     * or source node lacks a global routing instance.
     * If the destination node has multiple IpAddresses, the routing path will be printed for the
     * first Ip address of the destination Ipv4 Stack.
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ptr<Node> dest,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

    /**
     * @copybrief PrintRoute(Ptr<Node>, Ipv4Address, Ptr<OutputStreamWrapper>, bool,
     * Time::Unit)
     * @copydetails PrintRoute(Ptr<Node>, Ptr<Node>, bool, Time::Unit)
     * @param stream the output stream object to use
     */
    static void PrintRoute(Ptr<Node> sourceNode,
                           Ptr<Node> dest,
                           Ptr<OutputStreamWrapper> stream,
                           bool nodeIdLookup = true,
                           Time::Unit unit = Time::S);

    /**
     * @brief prints the routing path for the source and destination at a particular time to the
     * standard cout output stream.
     * @copydetails PrintRoute(Ptr<Node>, Ipv4Address, bool, Time::Unit)
     * @param printTime the time at which the routing path should be printed.
     */
    static void PrintRouteAt(Ptr<Node> sourceNode,
                             Ipv4Address dest,
                             Time printTime,
                             bool nodeIdLookup = true,
                             Time::Unit unit = Time::S);

    /**
     * @brief Prints the routing path for the source and destination at a particular time.
     * @copydetails PrintRoute(Ptr<Node>, Ipv4Address,bool, Time::Unit)
     * @param printTime the time at which the routing path should be printed.
     * @param stream the output stream object to use
     */
    static void PrintRouteAt(Ptr<Node> sourceNode,
                             Ipv4Address dest,
                             Time printTime,
                             Ptr<OutputStreamWrapper> stream,
                             bool nodeIdLookup = true,
                             Time::Unit unit = Time::S);

    /**
     * @copybrief PrintRouteAt(Ptr<Node>, Ipv4Address, Time, bool, Time::Unit)
     * @copydetails PrintRoute(Ptr<Node>, Ptr<Node>, bool, Time::Unit)
     * @param printTime the time at which the routing path should be printed.
     */
    static void PrintRouteAt(Ptr<Node> sourceNode,
                             Ptr<Node> dest,
                             Time printTime,
                             bool nodeIdLookup = true,
                             Time::Unit unit = Time::S);

    /**
     * @copybrief PrintRouteAt(Ptr<Node>, Ipv4Address, Time, bool, Time::Unit)
     * @copydetails PrintRoute(Ptr<Node>, Ptr<Node>, bool, Time::Unit)
     * @param printTime the time at which the routing path should be printed.
     * @param stream the output stream object to use.
     */
    static void PrintRouteAt(Ptr<Node> sourceNode,
                             Ptr<Node> dest,
                             Time printTime,
                             Ptr<OutputStreamWrapper> stream,
                             bool nodeIdLookup = true,
                             Time::Unit unit = Time::S);

    /**
     * @brief Remove all routes that were previously installed in a prior call
     * to either PopulateRoutingTables() or RecomputeRoutingTables(), and
     * add a new set of routes.
     *
     * This method does not change the set of nodes
     * over which GlobalRouting is being used, but it will dynamically update
     * its representation of the global topology before recomputing routes.
     * Users must first call PopulateRoutingTables() and then may subsequently
     * call RecomputeRoutingTables() at any later time in the simulation.
     *
     */
    static void RecomputeRoutingTables();
};

} // namespace ns3

#endif /* IPV4_GLOBAL_ROUTING_HELPER_H */
