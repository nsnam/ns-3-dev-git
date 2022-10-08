/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef POINT_TO_POINT_HELPER_H
#define POINT_TO_POINT_HELPER_H

#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/queue.h"
#include "ns3/trace-helper.h"

#include <string>

namespace ns3
{

class NetDevice;
class Node;

/**
 * \brief Build a set of PointToPointNetDevice objects
 *
 * Normally we eschew multiple inheritance, however, the classes
 * PcapUserHelperForDevice and AsciiTraceUserHelperForDevice are
 * "mixins".
 */
class PointToPointHelper : public PcapHelperForDevice, public AsciiTraceHelperForDevice
{
  public:
    /**
     * Create a PointToPointHelper to make life easier when creating point to
     * point networks.
     */
    PointToPointHelper();

    ~PointToPointHelper() override
    {
    }

    /**
     * Each point to point net device must have a queue to pass packets through.
     * This method allows one to set the type of the queue that is automatically
     * created when the device is created and attached to a node.
     *
     * \tparam Ts \deduced Argument types
     * \param type the type of queue
     * \param [in] args Name and AttributeValue pairs to set.
     *
     * Set the type of queue to create and associated to each
     * PointToPointNetDevice created through PointToPointHelper::Install.
     */
    template <typename... Ts>
    void SetQueue(std::string type, Ts&&... args);

    /**
     * Set an attribute value to be propagated to each NetDevice created by the
     * helper.
     *
     * \param name the name of the attribute to set
     * \param value the value of the attribute to set
     *
     * Set these attributes on each ns3::PointToPointNetDevice created
     * by PointToPointHelper::Install
     */
    void SetDeviceAttribute(std::string name, const AttributeValue& value);

    /**
     * Set an attribute value to be propagated to each Channel created by the
     * helper.
     *
     * \param name the name of the attribute to set
     * \param value the value of the attribute to set
     *
     * Set these attribute on each ns3::PointToPointChannel created
     * by PointToPointHelper::Install
     */
    void SetChannelAttribute(std::string name, const AttributeValue& value);

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
     * \param c a set of nodes
     * \return a NetDeviceContainer for nodes
     *
     * This method creates a ns3::PointToPointChannel with the
     * attributes configured by PointToPointHelper::SetChannelAttribute,
     * then, for each node in the input container, we create a
     * ns3::PointToPointNetDevice with the requested attributes,
     * a queue for this ns3::NetDevice, and associate the resulting
     * ns3::NetDevice with the ns3::Node and ns3::PointToPointChannel.
     */
    NetDeviceContainer Install(NodeContainer c);

    /**
     * \param a first node
     * \param b second node
     * \return a NetDeviceContainer for nodes
     *
     * Saves you from having to construct a temporary NodeContainer.
     * Also, if MPI is enabled, for distributed simulations,
     * appropriate remote point-to-point channels are created.
     */
    NetDeviceContainer Install(Ptr<Node> a, Ptr<Node> b);

    /**
     * \param a first node
     * \param bName name of second node
     * \return a NetDeviceContainer for nodes
     *
     * Saves you from having to construct a temporary NodeContainer.
     */
    NetDeviceContainer Install(Ptr<Node> a, std::string bName);

    /**
     * \param aName Name of first node
     * \param b second node
     * \return a NetDeviceContainer for nodes
     *
     * Saves you from having to construct a temporary NodeContainer.
     */
    NetDeviceContainer Install(std::string aName, Ptr<Node> b);

    /**
     * \param aNode Name of first node
     * \param bNode Name of second node
     * \return a NetDeviceContainer for nodes
     *
     * Saves you from having to construct a temporary NodeContainer.
     */
    NetDeviceContainer Install(std::string aNode, std::string bNode);

  private:
    /**
     * \brief Enable pcap output the indicated net device.
     *
     * NetDevice-specific implementation mechanism for hooking the trace and
     * writing to the trace file.
     *
     * \param prefix Filename prefix to use for pcap files.
     * \param nd Net device for which you want to enable tracing.
     * \param promiscuous If true capture all possible packets available at the device.
     * \param explicitFilename Treat the prefix as an explicit filename if true
     */
    void EnablePcapInternal(std::string prefix,
                            Ptr<NetDevice> nd,
                            bool promiscuous,
                            bool explicitFilename) override;

    /**
     * \brief Enable ascii trace output on the indicated net device.
     *
     * NetDevice-specific implementation mechanism for hooking the trace and
     * writing to the trace file.
     *
     * \param stream The output stream object to use when logging ascii traces.
     * \param prefix Filename prefix to use for ascii trace files.
     * \param nd Net device for which you want to enable tracing.
     * \param explicitFilename Treat the prefix as an explicit filename if true
     */
    void EnableAsciiInternal(Ptr<OutputStreamWrapper> stream,
                             std::string prefix,
                             Ptr<NetDevice> nd,
                             bool explicitFilename) override;

    ObjectFactory m_queueFactory;   //!< Queue Factory
    ObjectFactory m_channelFactory; //!< Channel Factory
    ObjectFactory m_deviceFactory;  //!< Device Factory
    bool m_enableFlowControl;       //!< whether to enable flow control
};

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

template <typename... Ts>
void
PointToPointHelper::SetQueue(std::string type, Ts&&... args)
{
    QueueBase::AppendItemTypeIfNotPresent(type, "Packet");

    m_queueFactory.SetTypeId(type);
    m_queueFactory.Set(std::forward<Ts>(args)...);
}

} // namespace ns3

#endif /* POINT_TO_POINT_HELPER_H */
