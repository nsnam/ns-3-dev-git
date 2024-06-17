/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "simple-net-device-helper.h"

#include "trace-helper.h"

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/net-device-queue-interface.h"
#include "ns3/object-factory.h"
#include "ns3/packet.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"

#include <string>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SimpleNetDeviceHelper");

SimpleNetDeviceHelper::SimpleNetDeviceHelper()
{
    m_queueFactory.SetTypeId("ns3::DropTailQueue<Packet>");
    m_deviceFactory.SetTypeId("ns3::SimpleNetDevice");
    m_channelFactory.SetTypeId("ns3::SimpleChannel");
    m_pointToPointMode = false;
    m_enableFlowControl = true;
}

void
SimpleNetDeviceHelper::SetDeviceAttribute(std::string n1, const AttributeValue& v1)
{
    m_deviceFactory.Set(n1, v1);
}

void
SimpleNetDeviceHelper::SetChannelAttribute(std::string n1, const AttributeValue& v1)
{
    m_channelFactory.Set(n1, v1);
}

void
SimpleNetDeviceHelper::SetNetDevicePointToPointMode(bool pointToPointMode)
{
    m_pointToPointMode = pointToPointMode;
}

void
SimpleNetDeviceHelper::DisableFlowControl()
{
    m_enableFlowControl = false;
}

NetDeviceContainer
SimpleNetDeviceHelper::Install(Ptr<Node> node) const
{
    Ptr<SimpleChannel> channel = m_channelFactory.Create<SimpleChannel>();
    return Install(node, channel);
}

NetDeviceContainer
SimpleNetDeviceHelper::Install(Ptr<Node> node, Ptr<SimpleChannel> channel) const
{
    return NetDeviceContainer(InstallPriv(node, channel));
}

NetDeviceContainer
SimpleNetDeviceHelper::Install(const NodeContainer& c) const
{
    Ptr<SimpleChannel> channel = m_channelFactory.Create<SimpleChannel>();

    return Install(c, channel);
}

NetDeviceContainer
SimpleNetDeviceHelper::Install(const NodeContainer& c, Ptr<SimpleChannel> channel) const
{
    NetDeviceContainer devs;

    for (auto i = c.Begin(); i != c.End(); i++)
    {
        devs.Add(InstallPriv(*i, channel));
    }

    return devs;
}

Ptr<NetDevice>
SimpleNetDeviceHelper::InstallPriv(Ptr<Node> node, Ptr<SimpleChannel> channel) const
{
    Ptr<SimpleNetDevice> device = m_deviceFactory.Create<SimpleNetDevice>();
    device->SetAttribute("PointToPointMode", BooleanValue(m_pointToPointMode));
    device->SetAddress(Mac48Address::Allocate());
    node->AddDevice(device);
    device->SetChannel(channel);
    Ptr<Queue<Packet>> queue = m_queueFactory.Create<Queue<Packet>>();
    device->SetQueue(queue);
    NS_ASSERT_MSG(!m_pointToPointMode || (channel->GetNDevices() <= 2),
                  "Device set to PointToPoint and more than 2 devices on the channel.");
    if (m_enableFlowControl)
    {
        // Aggregate a NetDeviceQueueInterface object
        Ptr<NetDeviceQueueInterface> ndqi = CreateObject<NetDeviceQueueInterface>();
        ndqi->GetTxQueue(0)->ConnectQueueTraces(queue);
        device->AggregateObject(ndqi);
    }
    return device;
}

} // namespace ns3
