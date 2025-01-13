/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef APPLICATIONS_HELPER_H
#define APPLICATIONS_HELPER_H

#include "application-container.h"
#include "node-container.h"

#include "ns3/address.h"
#include "ns3/attribute.h"
#include "ns3/object-factory.h"

#include <string>

namespace ns3
{

/**
 * @brief A helper to make it easier to instantiate an application on a set of nodes.
 */
class ApplicationHelper
{
  public:
    /**
     * Create an application of a given type ID
     *
     * @param typeId the type ID.
     */
    explicit ApplicationHelper(TypeId typeId);

    /**
     * Create an application of a given type ID
     *
     * @param typeId the type ID expressed as a string.
     */
    explicit ApplicationHelper(const std::string& typeId);

    /**
     * Allow the helper to be repurposed for another application type
     *
     * @param typeId the type ID.
     */
    void SetTypeId(TypeId typeId);

    /**
     * Allow the helper to be repurposed for another application type
     *
     * @param typeId the type ID expressed as a string.
     */
    void SetTypeId(const std::string& typeId);

    /**
     * Helper function used to set the underlying application attributes.
     *
     * @param name the name of the application attribute to set
     * @param value the value of the application attribute to set
     */
    void SetAttribute(const std::string& name, const AttributeValue& value);

    /**
     * Install an application on each node of the input container
     * configured with all the attributes set with SetAttribute.
     *
     * @param c NodeContainer of the set of nodes on which an application
     * will be installed.
     * @return Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(NodeContainer c);

    /**
     * Install an application on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param node The node on which an application will be installed.
     * @return Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(Ptr<Node> node);

    /**
     * Install an application on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param nodeName The node on which an application will be installed.
     * @return Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(const std::string& nodeName);

    /**
     * Assigns a unique (monotonically increasing) stream number to all applications that match the
     * configured type of this application helper instance. Return the number of streams (possibly
     * zero) that have been assigned. The Install() method should have previously been called by the
     * user.
     *
     * @param stream first stream index to use
     * @param c NodeContainer of the set of nodes for which the application should be modified to
     * use a fixed stream
     * @return the number of stream indices assigned by this helper
     */
    int64_t AssignStreams(NodeContainer c, int64_t stream);

    /**
     * Assign a fixed random variable stream number to the random variables used by all the
     * applications. Return the number of streams (possibly zero) that have been assigned. The
     * Install() method should have previously been called by the user.
     *
     * @param stream first stream index to use
     * @param c NodeContainer of the set of nodes for which their applications should be modified to
     * use a fixed stream
     * @return the number of stream indices assigned by this helper
     */
    static int64_t AssignStreamsToAllApps(NodeContainer c, int64_t stream);

  protected:
    /**
     * Install an application on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param node The node on which an application will be installed.
     * @return Ptr to the application installed.
     */
    virtual Ptr<Application> DoInstall(Ptr<Node> node);

    ObjectFactory m_factory; //!< Object factory.
};

} // namespace ns3

#endif /* APPLICATIONS_HELPER_H */
