/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#include "packet-data-calculators.h"

#include "mac48-address.h"

#include "ns3/basic-data-calculators.h"
#include "ns3/log.h"
#include "ns3/packet.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PacketDataCalculators");

//--------------------------------------------------------------
//----------------------------------------------
PacketCounterCalculator::PacketCounterCalculator()
{
    NS_LOG_FUNCTION_NOARGS();
}

PacketCounterCalculator::~PacketCounterCalculator()
{
    NS_LOG_FUNCTION_NOARGS();
}

/* static */
TypeId
PacketCounterCalculator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PacketCounterCalculator")
                            .SetParent<CounterCalculator<uint32_t>>()
                            .SetGroupName("Network")
                            .AddConstructor<PacketCounterCalculator>();
    return tid;
}

void
PacketCounterCalculator::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();

    CounterCalculator<uint32_t>::DoDispose();
    // PacketCounterCalculator::DoDispose
}

void
PacketCounterCalculator::PacketUpdate(std::string path, Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION_NOARGS();

    CounterCalculator<uint32_t>::Update();

    // PacketCounterCalculator::Update
}

void
PacketCounterCalculator::FrameUpdate(std::string path,
                                     Ptr<const Packet> packet,
                                     Mac48Address realto)
{
    NS_LOG_FUNCTION_NOARGS();

    CounterCalculator<uint32_t>::Update();

    // PacketCounterCalculator::Update
}

//--------------------------------------------------------------
//----------------------------------------------
PacketSizeMinMaxAvgTotalCalculator::PacketSizeMinMaxAvgTotalCalculator()
{
    NS_LOG_FUNCTION_NOARGS();
}

PacketSizeMinMaxAvgTotalCalculator::~PacketSizeMinMaxAvgTotalCalculator()
{
    NS_LOG_FUNCTION_NOARGS();
}

/* static */
TypeId
PacketSizeMinMaxAvgTotalCalculator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PacketSizeMinMaxAvgTotalCalculator")
                            .SetParent<MinMaxAvgTotalCalculator<uint32_t>>()
                            .SetGroupName("Network")
                            .AddConstructor<PacketSizeMinMaxAvgTotalCalculator>();
    return tid;
}

void
PacketSizeMinMaxAvgTotalCalculator::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();

    MinMaxAvgTotalCalculator<uint32_t>::DoDispose();
    // end PacketSizeMinMaxAvgTotalCalculator::DoDispose
}

void
PacketSizeMinMaxAvgTotalCalculator::PacketUpdate(std::string path, Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION_NOARGS();

    MinMaxAvgTotalCalculator<uint32_t>::Update(packet->GetSize());

    // end PacketSizeMinMaxAvgTotalCalculator::Update
}

void
PacketSizeMinMaxAvgTotalCalculator::FrameUpdate(std::string path,
                                                Ptr<const Packet> packet,
                                                Mac48Address realto)
{
    NS_LOG_FUNCTION_NOARGS();

    MinMaxAvgTotalCalculator<uint32_t>::Update(packet->GetSize());

    // end PacketSizeMinMaxAvgTotalCalculator::Update
}
