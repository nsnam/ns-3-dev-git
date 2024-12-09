/*
 * Copyright (c) 2023 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-helper.h"

#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/zigbee-stack.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ZigbeeHelper");

ZigbeeHelper::ZigbeeHelper()
{
    NS_LOG_FUNCTION(this);
    m_stackFactory.SetTypeId("ns3::zigbee::ZigbeeStack");
}

void
ZigbeeHelper::SetDeviceAttribute(std::string n1, const AttributeValue& v1)
{
    NS_LOG_FUNCTION(this);
    m_stackFactory.Set(n1, v1);
}

zigbee::ZigbeeStackContainer
ZigbeeHelper::Install(const NetDeviceContainer c)
{
    NS_LOG_FUNCTION(this);

    zigbee::ZigbeeStackContainer zigbeeStackContainer;

    for (uint32_t i = 0; i < c.GetN(); ++i)
    {
        Ptr<NetDevice> device = c.Get(i);

        NS_ASSERT_MSG(device, "NetDevice not found in the node " << i);

        NS_LOG_LOGIC("Installing Zigbee on node " << device->GetNode()->GetId());

        Ptr<zigbee::ZigbeeStack> zigbeeStack = m_stackFactory.Create<zigbee::ZigbeeStack>();
        zigbeeStackContainer.Add(zigbeeStack);
        device->GetNode()->AggregateObject(zigbeeStack);
        zigbeeStack->SetNetDevice(device);
    }
    return zigbeeStackContainer;
}

} // namespace ns3
