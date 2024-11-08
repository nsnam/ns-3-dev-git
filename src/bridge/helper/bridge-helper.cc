/*
 * Copyright (c)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "bridge-helper.h"

#include "ns3/bridge-net-device.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node.h"

/**
 * @file
 * @ingroup bridge
 * ns3::BridgeHelper implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BridgeHelper");

BridgeHelper::BridgeHelper()
{
    NS_LOG_FUNCTION_NOARGS();
    m_deviceFactory.SetTypeId("ns3::BridgeNetDevice");
}

void
BridgeHelper::SetDeviceAttribute(std::string n1, const AttributeValue& v1)
{
    NS_LOG_FUNCTION_NOARGS();
    m_deviceFactory.Set(n1, v1);
}

NetDeviceContainer
BridgeHelper::Install(Ptr<Node> node, NetDeviceContainer c)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_LOGIC("**** Install bridge device on node " << node->GetId());

    NetDeviceContainer devs;
    Ptr<BridgeNetDevice> dev = m_deviceFactory.Create<BridgeNetDevice>();
    devs.Add(dev);
    node->AddDevice(dev);

    for (auto i = c.Begin(); i != c.End(); ++i)
    {
        NS_LOG_LOGIC("**** Add BridgePort " << *i);
        dev->AddBridgePort(*i);
    }
    return devs;
}

NetDeviceContainer
BridgeHelper::Install(std::string nodeName, NetDeviceContainer c)
{
    NS_LOG_FUNCTION_NOARGS();
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node, c);
}

} // namespace ns3
