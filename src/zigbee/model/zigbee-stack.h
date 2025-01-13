/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_STACK_H
#define ZIGBEE_STACK_H

#include "zigbee-nwk.h"

#include "ns3/lr-wpan-mac-base.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/traced-callback.h"

#include <stdint.h>
#include <string>

namespace ns3
{

class Node;

namespace zigbee
{

class ZigbeeNwk;

/**
 * @ingroup zigbee
 *
 *  Zigbee protocol stack to device interface.
 *
 * This class is an encapsulating class representing the protocol stack as described
 * by the Zigbee Specification. In the current implementation only the Zigbee
 * network layer (NWK) is included. However, this class is meant be later
 * be extended to include other layer and sublayers part of the Zigbee Specification.
 * The implementation is analogous to a NetDevice which encapsulates PHY and
 * MAC layers and provide the necessary hooks. Zigbee Stack is meant to encapsulate
 * NWK, APS, ZLC layers (and others if applicable).
 */
class ZigbeeStack : public Object
{
  public:
    /**
     * Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Default constructor
     */
    ZigbeeStack();
    ~ZigbeeStack() override;

    /**
     * Get the Channel object of the underlying LrWpanNetDevice
     * @return The LrWpanNetDevice Channel Object
     */
    Ptr<Channel> GetChannel() const;

    /**
     * Get the node currently using this ZigbeeStack.
     * @return The reference to the node object using this ZigbeeStack.
     */
    Ptr<Node> GetNode() const;

    /**
     * Get the NWK layer used by this ZigbeeStack.
     *
     * @return the NWK object
     */
    Ptr<ZigbeeNwk> GetNwk() const;

    /**
     * Set the NWK layer used by this ZigbeeStack.
     *
     * @param nwk The NWK layer object
     */
    void SetNwk(Ptr<ZigbeeNwk> nwk);

    /**
     *  Returns a smart pointer to the underlying NetDevice.
     *
     * @return A smart pointer to the underlying NetDevice.
     */
    Ptr<NetDevice> GetNetDevice() const;

    /**
     * Setup Zigbee to be the next set of higher layers for the specified NetDevice.
     * All the packets incoming and outgoing from the NetDevice will be
     * processed by ZigbeeStack.
     *
     * @param netDevice A smart pointer to the NetDevice used by Zigbee.
     */
    void SetNetDevice(Ptr<NetDevice> netDevice);

  protected:
    /**
     * Dispose of the Objects used by the ZigbeeStack
     */
    void DoDispose() override;

    /**
     * Initialize of the Objects used by the ZigbeeStack
     */
    void DoInitialize() override;

  private:
    Ptr<lrwpan::LrWpanMacBase> m_mac; //!< The underlying LrWpan MAC connected to this Zigbee Stack.
    Ptr<ZigbeeNwk> m_nwk;             //!< The Zigbee Network layer.
    Ptr<Node> m_node;                 //!< The node associated with this NetDevice.
    Ptr<NetDevice> m_netDevice;       //!< Smart pointer to the underlying NetDevice.
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_STACK_H */
