/*
 * Copyright (c) 2008 INRIA
 * Copyright (c) 2022 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * This file is adapted from bulk-send-helper.h.
 *
 * Author: Ameya Deshpande <ameyanrd@outlook.com>
 */

#ifndef FLENT_HELPER_H
#define FLENT_HELPER_H

#include "ns3/application-container.h"
#include "ns3/attribute.h"
#include "ns3/nstime.h"
#include "ns3/object-factory.h"

#include <string>

namespace ns3
{

/**
 * @ingroup flent
 * @brief A helper to make it easier to instantiate an ns3::FlentApplication
 */
class FlentHelper
{
  public:
    /**
     * Create an FlentHelper to make it easier to work with FlentApplication
     *
     * @param [in] testname the name of the test to run on FlentApplication
     *        The supported tests are ping, tcp_upload, tcp_download and rrul.
     * @param [in] address the address of the remote host to send traffic to.
     */
    FlentHelper(std::string testname, Address address);

    /**
     * Helper function used to set the underlying application attributes.
     *
     * @param name the name of the application attribute to set
     * @param value the value of the application attribute to set
     */
    void SetAttribute(std::string name, const AttributeValue& value);

    /**
     * @brief Set the test start time
     * @param start Test start time
     */
    void SetTestStartTime(Time start);

    /**
     * @brief Set the length of the Flent test
     * @param length Test length
     */
    void SetTestLength(Time length);

    /**
     * @brief Get the overall stop time of the simulation
     * @returns Stop time (start + length + 10s cooldown)
     */
    Time GetStopTime() const;

    /**
     * Install an ns3::FlentApplication on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param node The node on which an FlentApplication will be installed.
     * @returns Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(Ptr<Node> node) const;

    /**
     * Install an ns3::FlentApplication on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param nodeName The node on which an FlentApplication will be installed.
     * @returns Container of Ptr to the applications installed.
     */
    ApplicationContainer Install(std::string nodeName) const;

  private:
    /**
     * Install an ns3::FlentApplication on the node configured with all the
     * attributes set with SetAttribute.
     *
     * @param node The node on which an FlentApplication will be installed.
     * @returns Ptr to the application installed.
     */
    Ptr<Application> InstallPriv(Ptr<Node> node) const;

    ObjectFactory m_factory;    //!< Object factory.
    Time m_startTime{0};        //!< The start time of the Flent test
    Time m_length{Seconds(60)}; //!< The duration of the Flent test
};

} // namespace ns3

#endif /* FLENT_HELPER_H */
