/*
 * Copyright (c) 2007, 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: George Riley <riley@ece.gatech.edu>
 */

#include "point-to-point-remote-channel.h"

#include "point-to-point-net-device.h"

#include "ns3/log.h"
#include "ns3/mpi-interface.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PointToPointRemoteChannel");

NS_OBJECT_ENSURE_REGISTERED(PointToPointRemoteChannel);

TypeId
PointToPointRemoteChannel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PointToPointRemoteChannel")
                            .SetParent<PointToPointChannel>()
                            .SetGroupName("PointToPoint")
                            .AddConstructor<PointToPointRemoteChannel>();
    return tid;
}

PointToPointRemoteChannel::PointToPointRemoteChannel()
    : PointToPointChannel()
{
}

PointToPointRemoteChannel::~PointToPointRemoteChannel()
{
}

bool
PointToPointRemoteChannel::TransmitStart(Ptr<const Packet> p,
                                         Ptr<PointToPointNetDevice> src,
                                         Time txTime)
{
    NS_LOG_FUNCTION(this << p << src);
    NS_LOG_LOGIC("UID is " << p->GetUid() << ")");

    IsInitialized();

    uint32_t wire = src == GetSource(0) ? 0 : 1;
    Ptr<PointToPointNetDevice> dst = GetDestination(wire);

    // Calculate the rxTime (absolute)
    Time rxTime = Simulator::Now() + txTime + GetDelay();
    MpiInterface::SendPacket(p->Copy(), rxTime, dst->GetNode()->GetId(), dst->GetIfIndex());
    return true;
}

} // namespace ns3
