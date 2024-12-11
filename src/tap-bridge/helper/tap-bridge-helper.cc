/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "tap-bridge-helper.h"

#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/node.h"
#include "ns3/tap-bridge.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TapBridgeHelper");

TapBridgeHelper::TapBridgeHelper()
{
    NS_LOG_FUNCTION_NOARGS();
    m_deviceFactory.SetTypeId("ns3::TapBridge");
}

TapBridgeHelper::TapBridgeHelper(Ipv4Address gateway)
{
    NS_LOG_FUNCTION_NOARGS();
    m_deviceFactory.SetTypeId("ns3::TapBridge");
    SetAttribute("Mode", EnumValue(TapBridge::CONFIGURE_LOCAL));
}

void
TapBridgeHelper::SetAttribute(std::string n1, const AttributeValue& v1)
{
    NS_LOG_FUNCTION(n1 << &v1);
    m_deviceFactory.Set(n1, v1);
}

Ptr<NetDevice>
TapBridgeHelper::Install(Ptr<Node> node, Ptr<NetDevice> nd, const AttributeValue& v1)
{
    NS_LOG_FUNCTION(node << nd << &v1);
    m_deviceFactory.Set("DeviceName", v1);
    return Install(node, nd);
}

Ptr<NetDevice>
TapBridgeHelper::Install(Ptr<Node> node, Ptr<NetDevice> nd)
{
    NS_LOG_FUNCTION(node << nd);
    NS_LOG_LOGIC("Install TapBridge on node " << node->GetId() << " bridging net device " << nd);

    Ptr<TapBridge> bridge = m_deviceFactory.Create<TapBridge>();
    node->AddDevice(bridge);
    bridge->SetBridgedNetDevice(nd);

    return bridge;
}

Ptr<NetDevice>
TapBridgeHelper::Install(std::string nodeName, Ptr<NetDevice> nd)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    return Install(node, nd);
}

Ptr<NetDevice>
TapBridgeHelper::Install(Ptr<Node> node, std::string ndName)
{
    Ptr<NetDevice> nd = Names::Find<NetDevice>(ndName);
    return Install(node, nd);
}

Ptr<NetDevice>
TapBridgeHelper::Install(std::string nodeName, std::string ndName)
{
    Ptr<Node> node = Names::Find<Node>(nodeName);
    Ptr<NetDevice> nd = Names::Find<NetDevice>(ndName);
    return Install(node, nd);
}

} // namespace ns3
