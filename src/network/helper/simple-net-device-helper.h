/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */
#ifndef SIMPLE_NETDEVICE_HELPER_H
#define SIMPLE_NETDEVICE_HELPER_H

#include "net-device-container.h"
#include "node-container.h"

#include "ns3/attribute.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/simple-channel.h"

#include <string>

namespace ns3
{

/**
 * @brief build a set of SimpleNetDevice objects
 */
class SimpleNetDeviceHelper
{
  public:
    /**
     * Construct a SimpleNetDeviceHelper.
     */
    SimpleNetDeviceHelper();

    virtual ~SimpleNetDeviceHelper()
    {
    }

    /**
     * Each net device must have a queue to pass packets through.
     * This method allows one to set the type of the queue that is automatically
     * created when the device is created and attached to a node.
     *
     * @tparam Ts \deduced Argument types
     * @param type the type of queue
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * Set the type of queue to create and associated to each
     * SimpleNetDevice created through SimpleNetDeviceHelper::Install.
     */
    template <typename... Ts>
    void SetQueue(std::string type, Ts&&... args);

    /**
     * Each net device must have a channel to pass packets through.
     * This method allows one to set the type of the channel that is automatically
     * created when the device is created and attached to a node.
     *
     * @tparam Ts \deduced Argument types
     * @param type the type of channel
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * Set the type of channel to create and associated to each
     * SimpleNetDevice created through SimpleNetDeviceHelper::Install.
     */
    template <typename... Ts>
    void SetChannel(std::string type, Ts&&... args);

    /**
     * @param n1 the name of the attribute to set
     * @param v1 the value of the attribute to set
     *
     * Set these attributes on each ns3::SimpleNetDevice created
     * by SimpleNetDeviceHelper::Install
     */
    void SetDeviceAttribute(std::string n1, const AttributeValue& v1);

    /**
     * @param n1 the name of the attribute to set
     * @param v1 the value of the attribute to set
     *
     * Set these attributes on each ns3::CsmaChannel created
     * by SimpleNetDeviceHelper::Install
     */
    void SetChannelAttribute(std::string n1, const AttributeValue& v1);

    /**
     * SimpleNetDevice is Broadcast capable and ARP needing. This function
     * limits the number of SimpleNetDevices on one channel to two, disables
     * Broadcast and ARP and enables PointToPoint mode.
     *
     * @warning It must be used before installing a NetDevice on a node.
     *
     * @param pointToPointMode True for PointToPoint SimpleNetDevice
     */
    void SetNetDevicePointToPointMode(bool pointToPointMode);

    /**
     * Disable flow control only if you know what you are doing. By disabling
     * flow control, this NetDevice will be sent packets even if there is no
     * room for them (such packets will be likely dropped by this NetDevice).
     * Also, any queue disc installed on this NetDevice will have no effect,
     * as every packet enqueued to the traffic control layer queue disc will
     * be immediately dequeued.
     */
    void DisableFlowControl();

    /**
     * This method creates an ns3::SimpleChannel with the attributes configured by
     * SimpleNetDeviceHelper::SetChannelAttribute, an ns3::SimpleNetDevice with the attributes
     * configured by SimpleNetDeviceHelper::SetDeviceAttribute and then adds the device
     * to the node and attaches the channel to the device.
     *
     * @param node The node to install the device in
     * @returns A container holding the added net device.
     */
    NetDeviceContainer Install(Ptr<Node> node) const;

    /**
     * This method creates an ns3::SimpleNetDevice with the attributes configured by
     * SimpleNetDeviceHelper::SetDeviceAttribute and then adds the device to the node and
     * attaches the provided channel to the device.
     *
     * @param node The node to install the device in
     * @param channel The channel to attach to the device.
     * @returns A container holding the added net device.
     */
    NetDeviceContainer Install(Ptr<Node> node, Ptr<SimpleChannel> channel) const;

    /**
     * This method creates an ns3::SimpleChannel with the attributes configured by
     * SimpleNetDeviceHelper::SetChannelAttribute.  For each Ptr<node> in the provided
     * container: it creates an ns3::SimpleNetDevice (with the attributes
     * configured by SimpleNetDeviceHelper::SetDeviceAttribute); adds the device to the
     * node; and attaches the channel to the device.
     *
     * @param c The NodeContainer holding the nodes to be changed.
     * @returns A container holding the added net devices.
     */
    NetDeviceContainer Install(const NodeContainer& c) const;

    /**
     * For each Ptr<node> in the provided container, this method creates an
     * ns3::SimpleNetDevice (with the attributes configured by
     * SimpleNetDeviceHelper::SetDeviceAttribute); adds the device to the node; and attaches
     * the provided channel to the device.
     *
     * @param c The NodeContainer holding the nodes to be changed.
     * @param channel The channel to attach to the devices.
     * @returns A container holding the added net devices.
     */
    NetDeviceContainer Install(const NodeContainer& c, Ptr<SimpleChannel> channel) const;

  private:
    /**
     * This method creates an ns3::SimpleNetDevice with the attributes configured by
     * SimpleNetDeviceHelper::SetDeviceAttribute and then adds the device to the node and
     * attaches the provided channel to the device.
     *
     * @param node The node to install the device in
     * @param channel The channel to attach to the device.
     * @returns The new net device.
     */
    Ptr<NetDevice> InstallPriv(Ptr<Node> node, Ptr<SimpleChannel> channel) const;

    ObjectFactory m_queueFactory;   //!< Queue factory
    ObjectFactory m_deviceFactory;  //!< NetDevice factory
    ObjectFactory m_channelFactory; //!< Channel factory
    bool m_pointToPointMode;        //!< Install PointToPoint SimpleNetDevice or Broadcast ones
    bool m_enableFlowControl;       //!< whether to enable flow control
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
SimpleNetDeviceHelper::SetQueue(std::string type, Ts&&... args)
{
    QueueBase::AppendItemTypeIfNotPresent(type, "Packet");

    m_queueFactory.SetTypeId(type);
    m_queueFactory.Set(std::forward<Ts>(args)...);
}

template <typename... Ts>
void
SimpleNetDeviceHelper::SetChannel(std::string type, Ts&&... args)
{
    m_channelFactory.SetTypeId(type);
    m_channelFactory.Set(std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* SIMPLE_NETDEVICE_HELPER_H */
