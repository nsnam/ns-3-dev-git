/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "tgax-video-traffic.h"

#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/uinteger.h"

#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TgaxVideoTraffic");

NS_OBJECT_ENSURE_REGISTERED(TgaxVideoTraffic);

// clang-format off
const TgaxVideoTraffic::TrafficModels TgaxVideoTraffic::m_trafficModels {
    {
        {TrafficModelClassIdentifier::BUFFERED_VIDEO_1, {{"2Mbps"}, 6950, 0.8099}},
        {TrafficModelClassIdentifier::BUFFERED_VIDEO_2, {{"4Mbps"}, 13900, 0.8099}},
        {TrafficModelClassIdentifier::BUFFERED_VIDEO_3, {{"6Mbps"}, 20850, 0.8099}},
        {TrafficModelClassIdentifier::BUFFERED_VIDEO_4, {{"8Mbps"}, 27800, 0.8099}},
        {TrafficModelClassIdentifier::BUFFERED_VIDEO_5, {{"10Mbps"}, 34750, 0.8099}},
        {TrafficModelClassIdentifier::BUFFERED_VIDEO_6, {{"15600Kbps"}, 54210, 0.8099}},
        {TrafficModelClassIdentifier::MULTICAST_VIDEO_1, {{"3Mbps"}, 10425, 0.8099}},
        {TrafficModelClassIdentifier::MULTICAST_VIDEO_2, {{"6Mbps"}, 20850, 0.8099}}
    }
};

// clang-format on

TypeId
TgaxVideoTraffic::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::TgaxVideoTraffic")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<TgaxVideoTraffic>()
            .AddAttribute(
                "TrafficModelClassIdentifier",
                "The Traffic Model Class Identifier to use (use Custom for custom settings)",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                EnumValue(TgaxVideoTraffic::TrafficModelClassIdentifier::CUSTOM),
                MakeEnumAccessor<TrafficModelClassIdentifier>(
                    &TgaxVideoTraffic::m_trafficModelClassId),
                MakeEnumChecker(TgaxVideoTraffic::TrafficModelClassIdentifier::CUSTOM,
                                "Custom",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_1,
                                "BV1",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_2,
                                "BV2",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_3,
                                "BV3",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_4,
                                "BV4",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_5,
                                "BV5",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_6,
                                "BV6",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_1,
                                "MC1",
                                TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_2,
                                "MC2"))
            .AddAttribute(
                "Protocol",
                "The type of protocol to use. This should be a subclass of ns3::SocketFactory",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                TypeIdValue(TcpSocketFactory::GetTypeId()),
                MakeTypeIdAccessor(&TgaxVideoTraffic::m_protocolTid),
                MakeTypeIdChecker())
            .AddAttribute("CustomVideoBitRate",
                          "The video bit rate (if TrafficModelClassIdentifier is Custom).",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DataRateValue(DataRate("2Mbps")),
                          MakeDataRateAccessor(&TgaxVideoTraffic::m_bitRate),
                          MakeDataRateChecker())
            .AddAttribute("CustomFrameSizeScale",
                          "Scale parameter for the Weibull distribution to calculate the video "
                          "frame size in bytes (if TrafficModelClassIdentifier is Custom).",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(6950),
                          MakeDoubleAccessor(&TgaxVideoTraffic::m_frameSizeBytesScale),
                          MakeDoubleChecker<double>())
            .AddAttribute("CustomFrameSizeShape",
                          "Shape parameter for the Weibull distribution to calculate the video "
                          "frame size in bytes (if TrafficModelClassIdentifier is Custom).",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(0.8099),
                          MakeDoubleAccessor(&TgaxVideoTraffic::m_frameSizeBytesShape),
                          MakeDoubleChecker<double>())
            .AddAttribute("LatencyShape",
                          "Shape parameter for the Gamma distribution to calculate the network "
                          "latency in milliseconds.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(0.2463),
                          MakeDoubleAccessor(&TgaxVideoTraffic::m_latencyMsShape),
                          MakeDoubleChecker<double>())
            .AddAttribute("LatencyScale",
                          "Rate parameter for the Gamma distribution to calculate the network "
                          "latency in milliseconds. If set to 0 (default), no latency is added."
                          "In the reference model, this is set to 60.227ms because it uses a link "
                          "simulator that doesn't actually have queues and TCP models.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(0),
                          MakeDoubleAccessor(&TgaxVideoTraffic::m_latencyMsScale),
                          MakeDoubleChecker<double>())
            .AddTraceSource(
                "TxWithLatency",
                "A video packet is sent, this trace also reports the latency applied to the packet",
                MakeTraceSourceAccessor(&TgaxVideoTraffic::m_txLatencyTrace),
                "ns3::TgaxVideoTraffic::TxTracedCallback")
            .AddTraceSource("VideoFrameGenerated",
                            "A video frame is generated, this trace reports the amount of payload "
                            "bytes in the generated frame",
                            MakeTraceSourceAccessor(&TgaxVideoTraffic::m_frameGeneratedTrace),
                            "ns3::TracedValueCallback::Uint32");
    return tid;
}

TgaxVideoTraffic::TgaxVideoTraffic()
    : m_frameSizeBytes{CreateObject<WeibullRandomVariable>()},
      m_latencyMs{CreateObject<GammaRandomVariable>()}
{
    NS_LOG_FUNCTION(this);
}

TgaxVideoTraffic::~TgaxVideoTraffic()
{
    NS_LOG_FUNCTION(this);
}

int64_t
TgaxVideoTraffic::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    m_frameSizeBytes->SetStream(currentStream++);
    m_latencyMs->SetStream(currentStream++);
    return (currentStream - stream);
}

void
TgaxVideoTraffic::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    SourceApplication::DoInitialize();

    if (m_trafficModelClassId != TrafficModelClassIdentifier::CUSTOM)
    {
        const auto it = m_trafficModels.find(m_trafficModelClassId);
        NS_ASSERT(it != m_trafficModels.cend());
        NS_ABORT_MSG_IF(
            ((m_trafficModelClassId == TrafficModelClassIdentifier::MULTICAST_VIDEO_1 ||
              m_trafficModelClassId == TrafficModelClassIdentifier::MULTICAST_VIDEO_2) &&
             m_protocolTid == TcpSocketFactory::GetTypeId()),
            "Cannot use TCP protocol with multicast video traffic model");
        m_bitRate = it->second.bitRate;
        m_frameSizeBytesScale = it->second.frameSizeBytesScale;
        m_frameSizeBytesShape = it->second.frameSizeBytesShape;
    }

    m_frameSizeBytes->SetAttribute("Scale", DoubleValue(m_frameSizeBytesScale));
    m_frameSizeBytes->SetAttribute("Shape", DoubleValue(m_frameSizeBytesShape));
    m_latencyMs->SetAttribute("Alpha", DoubleValue(m_latencyMsScale));
    m_latencyMs->SetAttribute("Beta", DoubleValue(m_latencyMsShape));

    auto averageFrameSize = static_cast<uint32_t>(m_frameSizeBytes->GetMean());
    m_interArrival = m_bitRate.CalculateBytesTxTime(averageFrameSize);
}

void
TgaxVideoTraffic::DoStartApplication()
{
    NS_LOG_FUNCTION(this);

    if (m_protocolTid == TcpSocketFactory::GetTypeId())
    {
        UintegerValue uintegerValue;
        m_socket->GetAttribute("SegmentSize", uintegerValue);
        m_maxSize = uintegerValue.Get();
    }

    m_socket->SetDataSentCallback(MakeCallback(&TgaxVideoTraffic::TxDone, this));
    m_socket->SetSendCallback(MakeCallback(&TgaxVideoTraffic::TxAvailable, this));
    m_socket->SetAllowBroadcast(true);
    m_socket->ShutdownRecv();

    if (m_connected)
    {
        ScheduleNextFrame();
    }
}

void
TgaxVideoTraffic::CancelEvents()
{
    NS_LOG_FUNCTION(this);
    m_generateFrameEvent.Cancel();
    for (auto& sendEvent : m_sendEvents)
    {
        sendEvent.second.Cancel();
    }
    m_sendEvents.clear();
    m_unsentPackets.clear();
}

void
TgaxVideoTraffic::ScheduleNextFrame()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_generateFrameEvent.IsExpired());
    NS_ASSERT(m_interArrival.IsStrictlyPositive());
    m_generateFrameEvent =
        Simulator::Schedule(m_interArrival, &TgaxVideoTraffic::GenerateVideoFrame, this);
}

void
TgaxVideoTraffic::GenerateVideoFrame()
{
    NS_LOG_FUNCTION(this);

    uint32_t frameSize{0};
    while (frameSize == 0)
    {
        frameSize = static_cast<uint32_t>(m_frameSizeBytes->GetValue());
    }

    m_frameGeneratedTrace(frameSize);
    m_generatedFrames.push_back(frameSize);

    SendWithLatency();

    ScheduleNextFrame();
}

uint32_t
TgaxVideoTraffic::GetNextPayloadSize()
{
    auto frameSize = m_remainingSize;
    if (frameSize == 0)
    {
        NS_ASSERT(!m_generatedFrames.empty());
        frameSize = m_generatedFrames.front();
        m_generatedFrames.pop_front();
    }

    if (const auto limit = m_maxSize.value_or(m_socket->GetTxAvailable()); frameSize > limit)
    {
        m_remainingSize = frameSize - limit;
        frameSize = limit;
    }
    else
    {
        m_remainingSize = 0;
    }

    return frameSize;
}

void
TgaxVideoTraffic::SendWithLatency()
{
    NS_LOG_FUNCTION(this);

    auto size = GetNextPayloadSize();
    NS_ASSERT(size > 0);

    auto latency{Time::FromDouble(m_latencyMs->GetValue(), Time::MS)};
    NS_LOG_INFO("At time " << Simulator::Now().As(Time::S)
                           << " video traffic source schedule to sent " << size
                           << " bytes after latency of " << latency.As(Time::US));

    m_sendEvents[m_nextEventId] =
        Simulator::Schedule(latency, &TgaxVideoTraffic::Send, this, m_nextEventId, size, latency);
    ++m_nextEventId;
}

void
TgaxVideoTraffic::Send(uint64_t eventId, uint32_t size, Time latency)
{
    NS_LOG_FUNCTION(this << eventId << size << latency << m_unsentPackets.size());

    auto it = m_sendEvents.find(eventId);
    NS_ASSERT(it != m_sendEvents.end());
    NS_ASSERT(it->second.IsExpired());

    auto packet = m_unsentPackets.empty() ? Create<Packet>(size) : m_unsentPackets.front().packet;
    const auto actual = static_cast<unsigned int>(m_socket->Send(packet));
    if (actual == size)
    {
        if (!m_unsentPackets.empty())
        {
            m_unsentPackets.pop_front();
        }
        m_txTrace(packet);
        m_txLatencyTrace(packet, latency);
        m_sendEvents.erase(it);
        if (InetSocketAddress::IsMatchingType(m_peer))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " video traffic source sent "
                                   << size << " bytes to "
                                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << " port "
                                   << InetSocketAddress::ConvertFrom(m_peer).GetPort());
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peer))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " video traffic source sent "
                                   << size << " bytes to "
                                   << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << " port "
                                   << Inet6SocketAddress::ConvertFrom(m_peer).GetPort());
        }
    }
    else
    {
        NS_LOG_DEBUG("Unable to send packet; actual " << actual << " size " << size
                                                      << "; caching for later attempt");
        if (m_unsentPackets.empty())
        {
            m_unsentPackets.push_back({eventId, packet, latency});
        }
    }
}

void
TgaxVideoTraffic::DoConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    ScheduleNextFrame();
}

void
TgaxVideoTraffic::TxDone(Ptr<Socket> socket, uint32_t size)
{
    NS_LOG_FUNCTION(this << socket << size);
    if (m_unsentPackets.empty() && !m_generatedFrames.empty() && m_socket->GetTxAvailable() > 0)
    {
        SendWithLatency();
    }
}

void
TgaxVideoTraffic::TxAvailable(Ptr<Socket> socket, uint32_t available)
{
    NS_LOG_FUNCTION(this << socket << available);
    if (m_unsentPackets.empty())
    {
        if (!m_generatedFrames.empty() && m_socket->GetTxAvailable() > 0)
        {
            SendWithLatency();
        }
        return;
    }
    const auto& unsentPacketInfo = m_unsentPackets.front();
    const auto size = unsentPacketInfo.packet->GetSize();
    if (available >= size)
    {
        // Do not add additional networking latency when this is another TX attempt
        NS_LOG_DEBUG("Retry packet " << unsentPacketInfo.id << " with size " << size);
        Send(unsentPacketInfo.id, size, unsentPacketInfo.latency);
    }
}

} // Namespace ns3
