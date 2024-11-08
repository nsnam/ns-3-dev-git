/*
 * Copyright (c) 2011 Blake Hurd
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Blake Hurd  <naimorai@gmail.com>
 */
#ifndef OPENFLOW_SWITCH_HELPER_H
#define OPENFLOW_SWITCH_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/object-factory.h"
#include "ns3/openflow-interface.h"

#include <string>

namespace ns3
{

class Node;
class AttributeValue;
class Controller;

/**
 * @brief Add capability to switch multiple LAN segments (IEEE 802.1D bridging)
 */
class OpenFlowSwitchHelper
{
  public:
    /*
     * Construct a OpenFlowSwitchHelper
     */
    OpenFlowSwitchHelper();

    /**
     * Set an attribute on each ns3::OpenFlowSwitchNetDevice created by
     * OpenFlowSwitchHelper::Install
     *
     * @param n1 the name of the attribute to set
     * @param v1 the value of the attribute to set
     */
    void SetDeviceAttribute(std::string n1, const AttributeValue& v1);

    /**
     * This method creates an ns3::OpenFlowSwitchNetDevice with the attributes
     * configured by OpenFlowSwitchHelper::SetDeviceAttribute, adds the device
     * to the node, attaches the given NetDevices as ports of the
     * switch, and sets up a controller connection using the provided
     * Controller.
     *
     * @param node The node to install the device in
     * @param c Container of NetDevices to add as switch ports
     * @param controller The controller connection.
     * @returns A container holding the added net device.
     */
    NetDeviceContainer Install(Ptr<Node> node,
                               NetDeviceContainer c,
                               Ptr<ns3::ofi::Controller> controller);

    /**
     * This method creates an ns3::OpenFlowSwitchNetDevice with the attributes
     * configured by OpenFlowSwitchHelper::SetDeviceAttribute, adds the device
     * to the node, and attaches the given NetDevices as ports of the
     * switch.
     *
     * @param node The node to install the device in
     * @param c Container of NetDevices to add as switch ports
     * @returns A container holding the added net device.
     */
    NetDeviceContainer Install(Ptr<Node> node, NetDeviceContainer c);

    /**
     * This method creates an ns3::OpenFlowSwitchNetDevice with the attributes
     * configured by OpenFlowSwitchHelper::SetDeviceAttribute, adds the device
     * to the node, and attaches the given NetDevices as ports of the
     * switch.
     *
     * @param nodeName The name of the node to install the device in
     * @param c Container of NetDevices to add as switch ports
     * @returns A container holding the added net device.
     */
    NetDeviceContainer Install(std::string nodeName, NetDeviceContainer c);

  private:
    ObjectFactory m_deviceFactory; //!< Object factory
};

} // namespace ns3

#endif /* OPENFLOW_SWITCH_HELPER_H */
