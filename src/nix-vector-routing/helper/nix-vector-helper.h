/*
 * Copyright (c) 2009 The Georgia Institute of Technology
 * Copyright (c) 2021 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is adapted from the old ipv4-nix-vector-helper.h.
 *
 * Authors: Josh Pelkey <jpelkey@gatech.edu>
 *
 * Modified by: Ameya Deshpande <ameyanrd@outlook.com>
 */

#ifndef NIX_VECTOR_HELPER_H
#define NIX_VECTOR_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/object-factory.h"

namespace ns3
{

/**
 * @ingroup nix-vector-routing
 *
 * @brief Helper class that adds Nix-vector routing to nodes.
 *
 * This class is expected to be used in conjunction with
 * ns3::InternetStackHelper::SetRoutingHelper
 *
 * @internal
 * Since this class is meant to be specialized only by Ipv4RoutingHelper or
 * Ipv6RoutingHelper the implementation of this class doesn't need to be
 * exposed here; it is in nix-vector-helper.cc.

 */
template <typename T>
class NixVectorHelper : public std::enable_if_t<std::is_same_v<Ipv4RoutingHelper, T> ||
                                                    std::is_same_v<Ipv6RoutingHelper, T>,
                                                T>
{
    /// Alias for determining whether the parent is Ipv4RoutingHelper or Ipv6RoutingHelper
    static constexpr bool IsIpv4 = std::is_same_v<Ipv4RoutingHelper, T>;
    /// Alias for Ipv4 and Ipv6 classes
    using Ip = typename std::conditional_t<IsIpv4, Ipv4, Ipv6>;
    /// Alias for Ipv4Address and Ipv6Address classes
    using IpAddress = typename std::conditional_t<IsIpv4, Ipv4Address, Ipv6Address>;
    /// Alias for Ipv4RoutingProtocol and Ipv6RoutingProtocol classes
    using IpRoutingProtocol =
        typename std::conditional_t<IsIpv4, Ipv4RoutingProtocol, Ipv6RoutingProtocol>;

  public:
    /**
     * Construct an NixVectorHelper to make life easier while adding Nix-vector
     * routing to nodes.
     */
    NixVectorHelper();

    /**
     * @brief Construct an NixVectorHelper from another previously
     * initialized instance (Copy Constructor).
     *
     * @param o object to copy
     */
    NixVectorHelper(const NixVectorHelper<T>& o);

    // Delete assignment operator to avoid misuse
    NixVectorHelper& operator=(const NixVectorHelper&) = delete;

    /**
     * @returns pointer to clone of this NixVectorHelper
     *
     * This method is mainly for internal use by the other helpers;
     * clients are expected to free the dynamic memory allocated by this method
     */
    NixVectorHelper<T>* Copy() const override;

    /**
     * @param node the node on which the routing protocol will run
     * @returns a newly-created routing protocol
     *
     * This method will be called by ns3::InternetStackHelper::Install
     */
    Ptr<IpRoutingProtocol> Create(Ptr<Node> node) const override;

    /**
     * @brief prints the routing path for a source and destination at a particular time.
     * If the routing path does not exist, it prints that the path does not exist between
     * the nodes in the ostream.
     * @param printTime the time at which the routing path is supposed to be printed.
     * @param source the source node pointer to start traversing
     * @param dest the IP destination address
     * @param stream the output stream object to use
     * @param unit the time unit to be used in the report
     *
     * This method calls the PrintRoutingPath() method of the
     * NixVectorRouting for the source and destination to provide
     * the routing path at the specified time.
     */
    void PrintRoutingPathAt(Time printTime,
                            Ptr<Node> source,
                            IpAddress dest,
                            Ptr<OutputStreamWrapper> stream,
                            Time::Unit unit = Time::S);

  private:
    ObjectFactory m_agentFactory; //!< Object factory

    /**
     * @brief prints the routing path for the source and destination. If the routing path
     * does not exist, it prints that the path does not exist between the nodes in the ostream.
     * @param source the source node pointer to start traversing
     * @param dest the IP destination address
     * @param stream the output stream object to use
     * @param unit the time unit to be used in the report
     *
     * This method calls the PrintRoutingPath() method of the
     * NixVectorRouting for the source and destination to provide
     * the routing path.
     */
    static void PrintRoute(Ptr<Node> source,
                           IpAddress dest,
                           Ptr<OutputStreamWrapper> stream,
                           Time::Unit unit = Time::S);
};

/**
 * @ingroup nix-vector-routing
 * Create the typedef Ipv4NixVectorHelper with T as Ipv4RoutingHelper
 *
 * Note: This typedef enables also backwards compatibility with original Ipv4RoutingHelper.
 */
typedef NixVectorHelper<Ipv4RoutingHelper> Ipv4NixVectorHelper;

/**
 * @ingroup nix-vector-routing
 * Create the typedef Ipv6NixVectorHelper with T as Ipv6RoutingHelper
 */
typedef NixVectorHelper<Ipv6RoutingHelper> Ipv6NixVectorHelper;
} // namespace ns3

#endif /* NIX_VECTOR_HELPER_H */
