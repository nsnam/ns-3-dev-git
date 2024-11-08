/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>, written after OlsrHelper by Mathieu Lacage
 * <mathieu.lacage@sophia.inria.fr>
 */

#ifndef AODV_HELPER_H
#define AODV_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"

namespace ns3
{
/**
 * @ingroup aodv
 * @brief Helper class that adds AODV routing to nodes.
 */
class AodvHelper : public Ipv4RoutingHelper
{
  public:
    AodvHelper();

    /**
     * @returns pointer to clone of this AodvHelper
     *
     * @internal
     * This method is mainly for internal use by the other helpers;
     * clients are expected to free the dynamic memory allocated by this method
     */
    AodvHelper* Copy() const override;

    /**
     * @param node the node on which the routing protocol will run
     * @returns a newly-created routing protocol
     *
     * This method will be called by ns3::InternetStackHelper::Install
     *
     * @todo support installing AODV on the subset of all available IP interfaces
     */
    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override;
    /**
     * @param name the name of the attribute to set
     * @param value the value of the attribute to set.
     *
     * This method controls the attributes of ns3::aodv::RoutingProtocol
     */
    void Set(std::string name, const AttributeValue& value);
    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.  The Install() method of the InternetStackHelper
     * should have previously been called by the user.
     *
     * @param stream first stream index to use
     * @param c NodeContainer of the set of nodes for which AODV
     *          should be modified to use a fixed stream
     * @return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NodeContainer c, int64_t stream);

  private:
    /** the factory to create AODV routing object */
    ObjectFactory m_agentFactory;
};

} // namespace ns3

#endif /* AODV_HELPER_H */
