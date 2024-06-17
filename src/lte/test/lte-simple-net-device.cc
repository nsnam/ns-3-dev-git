/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "lte-simple-net-device.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LteSimpleNetDevice");

NS_OBJECT_ENSURE_REGISTERED(LteSimpleNetDevice);

TypeId
LteSimpleNetDevice::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LteSimpleNetDevice")
                            .SetParent<SimpleNetDevice>()
                            .AddConstructor<LteSimpleNetDevice>();

    return tid;
}

LteSimpleNetDevice::LteSimpleNetDevice()
{
    NS_LOG_FUNCTION(this);
}

LteSimpleNetDevice::LteSimpleNetDevice(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    SetNode(node);
}

LteSimpleNetDevice::~LteSimpleNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
LteSimpleNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);
    SimpleNetDevice::DoDispose();
}

void
LteSimpleNetDevice::DoInitialize()
{
    NS_LOG_FUNCTION(this);
}

bool
LteSimpleNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(this << dest << protocolNumber);
    return SimpleNetDevice::Send(packet, dest, protocolNumber);
}

} // namespace ns3
