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

#include "zigbee-aps.h"
#include "zigbee-group-table.h"
#include "zigbee-nwk.h"

#include "ns3/lr-wpan-mac-base.h"
#include "ns3/lr-wpan-net-device.h"
#include "ns3/traced-callback.h"
#include "ns3/zigbee-zdo.h"

#include <stdint.h>

namespace ns3
{

class Node;

namespace zigbee
{

class ZigbeeNwk;
class ZigbeeAps;
class ZigbeeZdo;

/**
 * @ingroup zigbee
 *
 * Selects which Zigbee layers are instantiated by a ZigbeeStack. The values are
 * cumulative: each level includes all the layers of the levels below it.
 */
enum class StackLayers : uint8_t
{
    ONLY_NWK = 0,    //!< Only the NWK layer
    NWK_AND_APS = 1, //!< NWK layer + APS sub-layer
    FULL_STACK = 2   //!< NWK layer + APS sub-layer + ZDO (default)
};

/**
 * @ingroup zigbee
 *
 *  Zigbee protocol stack to device interface.
 *
 * This class is an encapsulating class representing the protocol stack as described
 * by the Zigbee Specification. The set of layers instantiated by this class is
 * selectable via ::StackLayers, ranging from the network layer (NWK) alone up to
 * the full NWK + APS + ZDO stack. This class is meant to be later extended to
 * include other layers and sublayers part of the Zigbee Specification.
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
     * Get the APS layer used by this ZigbeeStack.
     *
     * @return the APS object
     */
    Ptr<ZigbeeAps> GetAps() const;

    /**
     * Set the APS layer used by this ZigbeeStack.
     *
     * @param aps The APS layer object
     */
    void SetAps(Ptr<ZigbeeAps> aps);

    /**
     * Get the Zigbee Device Object (ZDO) used by this ZigbeeStack.
     *
     * @return the Zigbee device object (ZDO)
     */
    Ptr<ZigbeeZdo> GetZdo() const;

    /**
     * Set the Zigbee Device Object (ZDO) used by this ZigbeeStack.
     *
     * @param zdo The zigbee device object (ZDO)
     */
    void SetZdo(Ptr<ZigbeeZdo> zdo);

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

    /**
     * Selects which set of Zigbee layers should be instantiated by this stack.
     * Must be called before the stack is initialized.
     *
     * @param layers The set of layers to instantiate (see ::StackLayers).
     */
    void SetLayers(StackLayers layers);

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
    /**
     *  This function is a special function that works as a demultiplexer for the
     * APSDE-DATA.indication. It is used to direct the result of an APSDE-DATA.indication to either
     * the Application Framework [Endpoints 1~254] or to the Zigbee Device Object (ZDO) [Endpoint 0]
     * depending on the destination endpoint.
     *
     * @param params The parameters of the APSDE-DATA.indication.
     * @param p The packet received.
     */
    void ApsDataIndicationDispatcher(ApsdeDataIndicationParams params, Ptr<Packet> p);

    Ptr<lrwpan::LrWpanMacBase> m_mac; //!< The underlying LrWpan MAC connected to this Zigbee Stack.
    Ptr<ZigbeeNwk> m_nwk;             //!< The Zigbee Network layer.
    Ptr<ZigbeeAps> m_aps;             //!< The Zigbee Application Support Sub-layer
    Ptr<ZigbeeZdo> m_zdo;             //!< The Zigbee Device Object (ZDO).
    Ptr<ZigbeeGroupTable> m_groupTable; //!< The Zigbee Group Table used by both NWK and APS layers.
    Ptr<Node> m_node;                   //!< The node associated with this NetDevice.
    Ptr<NetDevice> m_netDevice;         //!< Smart pointer to the underlying NetDevice.
    StackLayers m_layers;               //!< Selected set of Zigbee layers to instantiate.
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_STACK_H */
