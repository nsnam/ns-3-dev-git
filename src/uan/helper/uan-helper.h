/*
 * Copyright (c) 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@u.washington.edu>
 */

#ifndef UAN_HELPER_H
#define UAN_HELPER_H

#include "ns3/attribute.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/uan-net-device.h"

#include <string>

namespace ns3
{

class UanChannel;

/**
 * @ingroup uan
 *
 * UAN configuration helper.
 */
class UanHelper
{
  public:
    UanHelper();          //!< Default constructor.
    virtual ~UanHelper(); //!< Destructor.

    /**
     * Set MAC attributes.
     *
     * @tparam Ts \deduced Argument types
     * @param type The type of ns3::UanMac to create.
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * All the attributes specified in this method should exist
     * in the requested mac.
     */
    template <typename... Ts>
    void SetMac(std::string type, Ts&&... args);

    /**
     * Set PHY attributes.
     *
     * @tparam Ts \deduced Argument types
     * @param phyType The type of ns3::UanPhy to create.
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * All the attributes specified in this method should exist
     * in the requested Phy.
     */
    template <typename... Ts>
    void SetPhy(std::string phyType, Ts&&... args);

    /**
     * Set the transducer attributes.
     *
     * @tparam Ts \deduced Argument types
     * @param type The type of ns3::Transducer to create.
     * @param [in] args Name and AttributeValue pairs to set.
     *
     * All the attributes specified in this method should exist
     * in the requested transducer.
     */
    template <typename... Ts>
    void SetTransducer(std::string type, Ts&&... args);
    /**
     * Enable ascii output on the specified deviceid within the
     * specified nodeid if it is of type ns3::UanNetDevice and dump
     * that to the specified stdc++ output stream.
     *
     * @param os Output stream.
     * @param nodeid The id of the node to generate ascii output for.
     * @param deviceid The id of the device to generate ascii output for.
     */
    static void EnableAscii(std::ostream& os, uint32_t nodeid, uint32_t deviceid);
    /**
     * Enable ascii output on each device which is of the
     * ns3::UanNetDevice type and which is located in the input
     * device container and dump that to the specified
     * stdc++ output stream.
     *
     * @param os Output stream.
     * @param d Device container.
     */
    static void EnableAscii(std::ostream& os, NetDeviceContainer d);
    /**
     * Enable ascii output on each device which is of the
     * ns3::UanNetDevice type and which is located in one
     * of the input node and dump that to the specified
     * stdc++ output stream.
     *
     * @param os Output stream.
     * @param n Node container.
     */
    static void EnableAscii(std::ostream& os, NodeContainer n);
    /**
     * Enable ascii output on each device which is of the
     * ns3::UanNetDevice type and dump that to the specified
     * stdc++ output stream.
     *
     * @param os Output stream.
     */
    static void EnableAsciiAll(std::ostream& os);

    /**
     * This method creates a simple ns3::UanChannel (with a default
     * ns3::UanNoiseModelDefault and ns3::UanPropModelIdeal) and
     * creates, for each of the input nodes, a new ns3::UanNetDevice
     * attached to this shared channel. Each ns3::UanNetDevice is also
     * configured with an ns3::UanTransducerHd, ns3::UanMac, and,
     * ns3::UanPhy, all of which are created based on the user-specified
     * attributes specified in UanHelper::SetTransducer,
     * UanHelper::SetMac, and, UanHelper::SetPhy.
     *
     * @param c A set of nodes.
     * @return The installed netdevices.
     */
    NetDeviceContainer Install(NodeContainer c) const;

    /**
     * For each of the input nodes, a new ns3::UanNetDevice is attached
     * to the shared input channel. Each ns3::UanNetDevice is also
     * configured with an ns3::UanTransducerHd, a ns3::UanMac, and ns3::UanPhy,
     * all of which are created based on the user-specified attributes
     * specified in UanHelper::SetTransducer, UanHelper::SetMac, and
     * UanHelper::SetPhy.
     *
     * @param c A set of nodes.
     * @param channel A channel to use.
     * @return The installed netdevices.
     */
    NetDeviceContainer Install(NodeContainer c, Ptr<UanChannel> channel) const;

    /**
     * Create a default an stack.
     *
     * The stack includes:
     * - default channel, ideal propagation and default noise model.
     * - default physical layer, with UanPhyGen.
     * - default transducer, half duplex acoustic modem with UanTransducerHd.
     * - default MAC layer, with UanMacAloha.
     *
     * Channel, physical layer, transducer and mac layer are added to the
     * UanNetDevice and then added to the node.
     *
     * @param node A node where to install the uan components.
     * @param channel A channel to use.
     * @return The installed UanNetDevice.
     */
    Ptr<UanNetDevice> Install(Ptr<Node> node, Ptr<UanChannel> channel) const;

    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model. Return the number of streams (possibly zero) that
     * have been assigned. The Install() method should have previously been
     * called by the user.
     *
     * @param c NetDeviceContainer of the set of net devices for which the
     *          UanNetDevice should be modified to use a fixed stream.
     * @param stream First stream index to use.
     * @return The number of stream indices assigned by this helper.
     */
    int64_t AssignStreams(NetDeviceContainer c, int64_t stream);

  private:
    ObjectFactory m_device;     //!< The device.
    ObjectFactory m_mac;        //!< The MAC layer.
    ObjectFactory m_phy;        //!< The PHY layer.
    ObjectFactory m_transducer; //!< The transducer.
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
UanHelper::SetMac(std::string type, Ts&&... args)
{
    m_mac = ObjectFactory(type, std::forward<Ts>(args)...);
}

template <typename... Ts>
void
UanHelper::SetPhy(std::string phyType, Ts&&... args)
{
    m_phy = ObjectFactory(phyType, std::forward<Ts>(args)...);
}

template <typename... Ts>
void
UanHelper::SetTransducer(std::string type, Ts&&... args)
{
    m_transducer = ObjectFactory(type, std::forward<Ts>(args)...);
}

} // end namespace ns3

#endif /* UAN_HELPER_H */
