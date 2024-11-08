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
