/*
 * Copyright (c) 2023 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_HELPER_H
#define ZIGBEE_HELPER_H

#include "zigbee-stack-container.h"

#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"

#include <string>

namespace ns3
{

class Node;
class AttributeValue;
class Time;

/**
 * @ingroup zigbee
 *
 *  Setup a Zigbee stack to be used with LrWpanNetDevice.
 */
class ZigbeeHelper
{
  public:
    /*
     * Construct a ZigbeeHelper
     */
    ZigbeeHelper();
    /**
     * Set an attribute on each ns3::ZigbeeStack created by
     * ZigbeeHelper::Install.
     *
     * @param n1 [in] The name of the attribute to set.
     * @param v1 [in] The value of the attribute to set.
     */
    void SetDeviceAttribute(std::string n1, const AttributeValue& v1);

    /**
     * Install the Zigbee stack on top of an existing LrWpanNetDevice.
     *
     * This function requires a set of properly configured LrWpanNetDevices
     * passed in as the parameter "c". After the installation,
     * the set Zigbee stack contains the upper layers that communicate
     * directly with the application.
     *
     * Note that Zigbee is specific designed to be used on top of
     * LrWpanDevice (IEEE 802.15.4).
     * Any other protocol will be discarded by Zigbee.
     *
     *
     * @param [in] c The NetDevice container with LrWpanNetDevices.
     * @return A container with the newly created ZigbeeStacks.
     */
    zigbee::ZigbeeStackContainer Install(NetDeviceContainer c);

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model. Return the number of streams (possibly zero) that
     * have been assigned. The Install() method should have previously been
     * called by the user.
     *
     * @param [in] c NetDeviceContainer of the set of net devices for which the
     *          ZigbeeStack should be modified to use a fixed stream.
     * @param [in] stream First stream index to use.
     * @return The number of stream indices assigned by this helper.
     */
    int64_t AssignStreams(NetDeviceContainer c, int64_t stream);

  private:
    ObjectFactory m_stackFactory; //!< Zigbee stack object factory.
};

} // namespace ns3

#endif /* ZIGBEE_HELPER_H */
