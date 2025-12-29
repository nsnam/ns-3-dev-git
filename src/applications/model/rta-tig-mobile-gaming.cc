/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "rta-tig-mobile-gaming.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/udp-socket-factory.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("RtaTigMobileGaming");

NS_OBJECT_ENSURE_REGISTERED(RtaTigMobileGaming);

TypeId
RtaTigMobileGaming::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RtaTigMobileGaming")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<RtaTigMobileGaming>()
            .AddAttribute("ModelPresets",
                          "The model presets to use (Custom for custom settings)",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          EnumValue(RtaTigMobileGaming::ModelPresets::CUSTOM),
                          MakeEnumAccessor<ModelPresets>(&RtaTigMobileGaming::m_modelPresets),
                          MakeEnumChecker(RtaTigMobileGaming::ModelPresets::CUSTOM,
                                          "Custom",
                                          RtaTigMobileGaming::ModelPresets::STATUS_SYNC_DL,
                                          "StatusSyncDl",
                                          RtaTigMobileGaming::ModelPresets::STATUS_SYNC_UL,
                                          "StatusSyncUl",
                                          RtaTigMobileGaming::ModelPresets::LOCKSTEP_DL,
                                          "LockstepDl",
                                          RtaTigMobileGaming::ModelPresets::LOCKSTEP_UL,
                                          "LockstepUl"))
            .AddAttribute(
                "Protocol",
                "The type of protocol to use. This should be a subclass of ns3::SocketFactory",
                TypeIdValue(UdpSocketFactory::GetTypeId()),
                MakeTypeIdAccessor(&RtaTigMobileGaming::m_protocolTid),
                MakeTypeIdChecker())
            .AddAttribute(
                "CustomInitialPacketSize",
                "A uniform random variable to generate size in bytes for initial packet payload.",
                StringValue("ns3::UniformRandomVariable[Min=0|Max=20]"),
                MakePointerAccessor(&RtaTigMobileGaming::m_initialSizeUniform),
                MakePointerChecker<UniformRandomVariable>())
            .AddAttribute(
                "CustomEndPacketSize",
                "A uniform random variable to generate size in bytes for end packet payload.",
                StringValue("ns3::UniformRandomVariable[Min=500|Max=600]"),
                MakePointerAccessor(&RtaTigMobileGaming::m_endSizeUniform),
                MakePointerChecker<UniformRandomVariable>())
            .AddAttribute(
                "CustomPacketSizeLev",
                "A largest extreme value random variable to calculate packet sizes in bytes.",
                StringValue("ns3::LargestExtremeValueRandomVariable[Location=50|Scale=11.0]"),
                MakePointerAccessor(&RtaTigMobileGaming::m_levSizes),
                MakePointerChecker<LargestExtremeValueRandomVariable>())
            .AddAttribute(
                "CustomPacketArrivalLev",
                "A largest extreme value random variable to calculate packet packet arrivals in "
                "microseconds.",
                StringValue("ns3::LargestExtremeValueRandomVariable[Location=13000|Scale=3700]"),
                MakePointerAccessor(&RtaTigMobileGaming::m_levArrivals),
                MakePointerChecker<LargestExtremeValueRandomVariable>())
            .AddTraceSource("TxWithStage",
                            "A packet is sent, this trace also reports the current stage",
                            MakeTraceSourceAccessor(&RtaTigMobileGaming::m_txStageTrace),
                            "ns3::RtaTigMobileGaming::TxTracedCallback");
    return tid;
}

RtaTigMobileGaming::RtaTigMobileGaming()
{
    NS_LOG_FUNCTION(this);
}

RtaTigMobileGaming::~RtaTigMobileGaming()
{
    NS_LOG_FUNCTION(this);
}

int64_t
RtaTigMobileGaming::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    m_initialSizeUniform->SetStream(currentStream++);
    m_endSizeUniform->SetStream(currentStream++);
    m_levArrivals->SetStream(currentStream++);
    m_levSizes->SetStream(currentStream++);
    return (currentStream - stream);
}

void
RtaTigMobileGaming::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    SourceApplication::DoInitialize();

    if (m_modelPresets != ModelPresets::CUSTOM)
    {
        // Load model presets
        switch (m_modelPresets)
        {
        case ModelPresets::STATUS_SYNC_DL:
            m_initialSizeUniform->SetAttribute("Min", DoubleValue(0));
            m_initialSizeUniform->SetAttribute("Max", DoubleValue(20));
            m_endSizeUniform->SetAttribute("Min", DoubleValue(500));
            m_endSizeUniform->SetAttribute("Max", DoubleValue(600));
            m_levSizes->SetAttribute("Location", DoubleValue(50.0));
            m_levSizes->SetAttribute("Scale", DoubleValue(11.0));
            m_levArrivals->SetAttribute("Location", DoubleValue(13000.0));
            m_levArrivals->SetAttribute("Scale", DoubleValue(3700.0));
            break;
        case ModelPresets::STATUS_SYNC_UL:
            m_initialSizeUniform->SetAttribute("Min", DoubleValue(0));
            m_initialSizeUniform->SetAttribute("Max", DoubleValue(20));
            m_endSizeUniform->SetAttribute("Min", DoubleValue(400));
            m_endSizeUniform->SetAttribute("Max", DoubleValue(550));
            m_levSizes->SetAttribute("Location", DoubleValue(38.0));
            m_levSizes->SetAttribute("Scale", DoubleValue(3.7));
            m_levArrivals->SetAttribute("Location", DoubleValue(15000.0));
            m_levArrivals->SetAttribute("Scale", DoubleValue(5700.0));
            break;
        case ModelPresets::LOCKSTEP_DL:
            m_initialSizeUniform->SetAttribute("Min", DoubleValue(0));
            m_initialSizeUniform->SetAttribute("Max", DoubleValue(80));
            m_endSizeUniform->SetAttribute("Min", DoubleValue(1400));
            m_endSizeUniform->SetAttribute("Max", DoubleValue(1500));
            m_levSizes->SetAttribute("Location", DoubleValue(210.0));
            m_levSizes->SetAttribute("Scale", DoubleValue(35.0));
            m_levArrivals->SetAttribute("Location", DoubleValue(28000.0));
            m_levArrivals->SetAttribute("Scale", DoubleValue(4200.0));
            break;
        case ModelPresets::LOCKSTEP_UL:
            m_initialSizeUniform->SetAttribute("Min", DoubleValue(0));
            m_initialSizeUniform->SetAttribute("Max", DoubleValue(80));
            m_endSizeUniform->SetAttribute("Min", DoubleValue(500));
            m_endSizeUniform->SetAttribute("Max", DoubleValue(600));
            m_levSizes->SetAttribute("Location", DoubleValue(92.0));
            m_levSizes->SetAttribute("Scale", DoubleValue(38.0));
            m_levArrivals->SetAttribute("Location", DoubleValue(22000.0));
            m_levArrivals->SetAttribute("Scale", DoubleValue(3400.0));
            break;
        case ModelPresets::CUSTOM:
        default:
            break;
        }
    }
}

void
RtaTigMobileGaming::DoStartApplication()
{
    NS_LOG_FUNCTION(this);

    m_socket->SetAllowBroadcast(true);
    m_socket->ShutdownRecv();

    m_currentStage = TrafficModelStage::INITIAL;

    if (m_connected)
    {
        ScheduleNext();
    }
}

void
RtaTigMobileGaming::DoStopApplication()
{
    NS_LOG_FUNCTION(this);
    m_currentStage = TrafficModelStage::ENDING;
}

void
RtaTigMobileGaming::CancelEvents()
{
    NS_LOG_FUNCTION(this);
    if (m_currentStage == TrafficModelStage::ENDING)
    {
        // handled once ending packet is transmitted
        return;
    }
    m_txEvent.Cancel();
}

void
RtaTigMobileGaming::ScheduleNext()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_txEvent.IsPending());
    const auto delay = MicroSeconds(m_levArrivals->GetValue());
    m_txEvent = Simulator::Schedule(delay, &RtaTigMobileGaming::SendPacket, this);
}

void
RtaTigMobileGaming::SendPacket()
{
    NS_LOG_FUNCTION(this);

    uint32_t packetSize;
    const auto stage{m_currentStage};
    switch (stage)
    {
    case TrafficModelStage::INITIAL:
        packetSize = m_initialSizeUniform->GetInteger();
        break;
    case TrafficModelStage::ENDING:
        packetSize = m_endSizeUniform->GetInteger();
        break;
    case TrafficModelStage::GAMING:
    default:
        packetSize = std::round(m_levSizes->GetValue());
        break;
    }
    auto packet = Create<Packet>(packetSize);

    const auto actualSize = static_cast<uint32_t>(m_socket->Send(packet));
    NS_ABORT_MSG_IF(actualSize != packetSize,
                    "Sent size " << actualSize << " does not match expected size " << packetSize);
    m_txTrace(packet);
    m_txStageTrace(packet, stage);

    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " gaming traffic source sent "
                               << packetSize << " bytes during stage " << stage << " to "
                               << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << " port "
                               << InetSocketAddress::ConvertFrom(m_peer).GetPort());
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " gaming traffic source sent "
                               << packetSize << " bytes during stage " << stage << " to "
                               << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << " port "
                               << Inet6SocketAddress::ConvertFrom(m_peer).GetPort());
    }

    if (m_currentStage == TrafficModelStage::ENDING)
    {
        m_currentStage = TrafficModelStage::INITIAL;
        CancelEvents();
        CloseSocket();
    }
    else
    {
        if (m_currentStage == TrafficModelStage::INITIAL)
        {
            m_currentStage = TrafficModelStage::GAMING;
        }
        ScheduleNext();
    }
}

void
RtaTigMobileGaming::DoConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    ScheduleNext();
}

std::ostream&
operator<<(std::ostream& os, RtaTigMobileGaming::TrafficModelStage stage)
{
    switch (stage)
    {
    case RtaTigMobileGaming::TrafficModelStage::INITIAL:
        os << "Initial stage";
        break;
    case RtaTigMobileGaming::TrafficModelStage::GAMING:
        os << "Gaming stage";
        break;
    case RtaTigMobileGaming::TrafficModelStage::ENDING:
        os << "Ending stage";
        break;
    default:
        NS_FATAL_ERROR("Unknown gaming traffic model stage");
    }
    return os;
}

} // Namespace ns3
