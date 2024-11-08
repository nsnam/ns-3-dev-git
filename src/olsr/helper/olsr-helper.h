/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef OLSR_HELPER_H
#define OLSR_HELPER_H

#include "ns3/ipv4-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"

#include <map>
#include <set>

namespace ns3
{

/**
 * @ingroup olsr
 *
 * @brief Helper class that adds OLSR routing to nodes.
 *
 * This class is expected to be used in conjunction with
 * ns3::InternetStackHelper::SetRoutingHelper
 */
class OlsrHelper : public Ipv4RoutingHelper
{
  public:
    /**
     * Create an OlsrHelper that makes life easier for people who want to install
     * OLSR routing to nodes.
     */
    OlsrHelper();

    /**
     * @brief Construct an OlsrHelper from another previously initialized instance
     * (Copy Constructor).
     *
     * @param o object to copy
     */
    OlsrHelper(const OlsrHelper& o);

    // Delete assignment operator to avoid misuse
    OlsrHelper& operator=(const OlsrHelper&) = delete;

    /**
     * @returns pointer to clone of this OlsrHelper
     *
     * This method is mainly for internal use by the other helpers;
     * clients are expected to free the dynamic memory allocated by this method
     */
    OlsrHelper* Copy() const override;

    /**
     * @param node the node for which an exception is to be defined
     * @param interface an interface of node on which OLSR is not to be installed
     *
     * This method allows the user to specify an interface on which OLSR is not to be installed on
     */
    void ExcludeInterface(Ptr<Node> node, uint32_t interface);

    /**
     * @param node the node on which the routing protocol will run
     * @returns a newly-created routing protocol
     *
     * This method will be called by ns3::InternetStackHelper::Install
     */
    Ptr<Ipv4RoutingProtocol> Create(Ptr<Node> node) const override;

    /**
     * @param name the name of the attribute to set
     * @param value the value of the attribute to set.
     *
     * This method controls the attributes of ns3::olsr::RoutingProtocol
     */
    void Set(std::string name, const AttributeValue& value);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.  The Install() method of the InternetStackHelper
     * should have previously been called by the user.
     *
     * @param stream first stream index to use
     * @param c NodeContainer of the set of nodes for which the OlsrRoutingProtocol
     *          should be modified to use a fixed stream
     * @return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NodeContainer c, int64_t stream);

  private:
    ObjectFactory m_agentFactory; //!< Object factory

    std::map<Ptr<Node>, std::set<uint32_t>>
        m_interfaceExclusions; //!< container of interfaces excluded from OLSR operations
};

} // namespace ns3

#endif /* OLSR_HELPER_H */
