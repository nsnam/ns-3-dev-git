/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "video-traffic.h"

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

NS_LOG_COMPONENT_DEFINE("VideoTraffic");

NS_OBJECT_ENSURE_REGISTERED(VideoTraffic);

// clang-format off
const VideoTraffic::TrafficModels VideoTraffic::m_trafficModels {
    {
        {BUFFERED_VIDEO_1, {{"2Mbps"}, 6950, 0.8099}},
        {BUFFERED_VIDEO_2, {{"4Mbps"}, 13900, 0.8099}},
        {BUFFERED_VIDEO_3, {{"6Mbps"}, 20850, 0.8099}},
        {BUFFERED_VIDEO_4, {{"8Mbps"}, 27800, 0.8099}},
        {BUFFERED_VIDEO_5, {{"10Mbps"}, 34750, 0.8099}},
        {BUFFERED_VIDEO_6, {{"15600Kbps"}, 54210, 0.8099}},
        {MULTICAST_VIDEO_1, {{"3Mbps"}, 10425, 0.8099}},
        {MULTICAST_VIDEO_2, {{"6Mbps"}, 20850, 0.8099}}
    }
};

// clang-format on

TypeId
VideoTraffic::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VideoTraffic")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<VideoTraffic>()
            .AddAttribute(
                "TrafficModelClassIdentifier",
                "The Traffic Model Class Identifier to use (use Custom for custom settings)",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                EnumValue(VideoTraffic::CUSTOM),
                MakeEnumAccessor<TrafficModelClassIdentifier>(&VideoTraffic::m_trafficModelClassId),
                MakeEnumChecker(VideoTraffic::CUSTOM,
                                "Custom",
                                VideoTraffic::BUFFERED_VIDEO_1,
                                "BV1",
                                VideoTraffic::BUFFERED_VIDEO_2,
                                "BV2",
                                VideoTraffic::BUFFERED_VIDEO_3,
                                "BV3",
                                VideoTraffic::BUFFERED_VIDEO_4,
                                "BV4",
                                VideoTraffic::BUFFERED_VIDEO_5,
                                "BV5",
                                VideoTraffic::BUFFERED_VIDEO_6,
                                "BV6",
                                VideoTraffic::MULTICAST_VIDEO_1,
                                "MC1",
                                VideoTraffic::MULTICAST_VIDEO_2,
                                "MC2"))
            .AddAttribute(
                "Protocol",
                "The type of protocol to use. This should be a subclass of ns3::SocketFactory",
                TypeId::ATTR_GET | TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                TypeIdValue(UdpSocketFactory::GetTypeId()),
                MakeTypeIdAccessor(&VideoTraffic::m_tid),
                MakeTypeIdChecker())
            .AddAttribute("CustomVideoBitRate",
                          "The video bit rate (if TrafficModelClassIdentifier is Custom).",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DataRateValue(DataRate("2Mbps")),
                          MakeDataRateAccessor(&VideoTraffic::m_bitRate),
                          MakeDataRateChecker())
            .AddAttribute("CustomWeibullScale",
                          "Scale parameter for the Weibull distribution to calculate the video "
                          "frame size in bytes (if TrafficModelClassIdentifier is Custom).",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(6950),
                          MakeDoubleAccessor(&VideoTraffic::m_weibullScale),
                          MakeDoubleChecker<double>())
            .AddAttribute("CustomWeibullShape",
                          "Shape parameter for the Weibull distribution to calculate the video "
                          "frame size in bytes (if TrafficModelClassIdentifier is Custom).",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(0.8099),
                          MakeDoubleAccessor(&VideoTraffic::m_weibullShape),
                          MakeDoubleChecker<double>())
            .AddAttribute("GammaShape",
                          "Shape parameter for the Gamma distribution to calculate the network "
                          "latency in milliseconds.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(0.2463),
                          MakeDoubleAccessor(&VideoTraffic::m_gammaShape),
                          MakeDoubleChecker<double>())
            .AddAttribute("GammaScale",
                          "Rate parameter for the Gamma distribution to calculate the network "
                          "latency in milliseconds.",
                          TypeId::ATTR_GET |
                              TypeId::ATTR_CONSTRUCT, // prevent setting after construction
                          DoubleValue(60.227),
                          MakeDoubleAccessor(&VideoTraffic::m_gammaScale),
                          MakeDoubleChecker<double>())
            .AddTraceSource(
                "TxWithLatency",
                "A video packet is sent, this trace also reports the latency applied to the packet",
                MakeTraceSourceAccessor(&VideoTraffic::m_txLatencyTrace),
                "ns3::VideoTraffic::TxTracedCallback")
            .AddTraceSource("VideoFrameGenerated",
                            "A video frame is generated, this trace reports the amount of payload "
                            "bytes in the generated frame",
                            MakeTraceSourceAccessor(&VideoTraffic::m_frameGeneratedTrace),
                            "ns3::TracedValueCallback::Uint32");
    return tid;
}

VideoTraffic::VideoTraffic()
    : m_weibull{CreateObject<WeibullRandomVariable>()},
      m_gamma{CreateObject<GammaRandomVariable>()},
      m_socket{nullptr},
      m_connected{false},
      m_remainingSize{0},
      m_interArrival{Time(0)},
      m_nextEventId{0}
{
    NS_LOG_FUNCTION(this);
}

VideoTraffic::~VideoTraffic()
{
    NS_LOG_FUNCTION(this);
}

int64_t
VideoTraffic::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    m_weibull->SetStream(currentStream++);
    m_gamma->SetStream(currentStream++);
    return (currentStream - stream);
}

void
VideoTraffic::DoDispose()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    m_socket = nullptr;
    Application::DoDispose();
}

void
VideoTraffic::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        if (m_trafficModelClassId != TrafficModelClassIdentifier::CUSTOM)
        {
            const auto it = m_trafficModels.find(m_trafficModelClassId);
            NS_ASSERT(it != m_trafficModels.cend());
            m_bitRate = it->second.bitRate;
            m_weibullScale = it->second.weibullScale;
            m_weibullShape = it->second.weibullShape;
        }

        auto averageFrameSize =
            static_cast<uint32_t>(WeibullRandomVariable::GetMean(m_weibullScale, m_weibullShape));
        m_interArrival = m_bitRate.CalculateBytesTxTime(averageFrameSize);

        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        if (m_tid == TcpSocketFactory::GetTypeId())
        {
            UintegerValue uintegerValue;
            m_socket->GetAttribute("SegmentSize", uintegerValue);
            m_maxSize = uintegerValue.Get();
        }

        int ret = -1;
        if (!m_local.IsInvalid())
        {
            NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                             InetSocketAddress::IsMatchingType(m_local)) ||
                                (InetSocketAddress::IsMatchingType(m_peer) &&
                                 Inet6SocketAddress::IsMatchingType(m_local)),
                            "Incompatible peer and local address IP version");
            ret = m_socket->Bind(m_local);
        }
        else
        {
            if (Inet6SocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind6();
            }
            else if (InetSocketAddress::IsMatchingType(m_peer) ||
                     PacketSocketAddress::IsMatchingType(m_peer))
            {
                ret = m_socket->Bind();
            }
        }

        if (ret == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }

        m_socket->SetConnectCallback(MakeCallback(&VideoTraffic::ConnectionSucceeded, this),
                                     MakeCallback(&VideoTraffic::ConnectionFailed, this));
        m_socket->SetDataSentCallback(MakeCallback(&VideoTraffic::TxDone, this));
        m_socket->SetSendCallback(MakeCallback(&VideoTraffic::TxAvailable, this));

        if (InetSocketAddress::IsMatchingType(m_peer))
        {
            m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
        }

        m_socket->Connect(m_peer);
        m_socket->SetAllowBroadcast(true);
        m_socket->ShutdownRecv();
    }

    CancelEvents();

    if (m_connected)
    {
        ScheduleNextFrame();
    }
}

void
VideoTraffic::StopApplication()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
VideoTraffic::CancelEvents()
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
VideoTraffic::ScheduleNextFrame()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_generateFrameEvent.IsExpired());
    NS_ASSERT(m_interArrival.IsStrictlyPositive());
    m_generateFrameEvent =
        Simulator::Schedule(m_interArrival, &VideoTraffic::GenerateVideoFrame, this);
}

void
VideoTraffic::GenerateVideoFrame()
{
    NS_LOG_FUNCTION(this);

    uint32_t frameSize{0};
    while (frameSize == 0)
    {
        frameSize = static_cast<uint32_t>(m_weibull->GetValue(m_weibullScale, m_weibullShape, 0));
    }

    m_frameGeneratedTrace(frameSize);
    m_generatedFrames.push_back(frameSize);

    SendWithLatency();

    ScheduleNextFrame();
}

uint32_t
VideoTraffic::GetNextPayloadSize()
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
VideoTraffic::SendWithLatency()
{
    NS_LOG_FUNCTION(this);

    auto size = GetNextPayloadSize();
    NS_ASSERT(size > 0);

    const auto millisecondsToMicroSeconds{1000};
    auto latency{
        MicroSeconds(millisecondsToMicroSeconds * m_gamma->GetValue(m_gammaScale, m_gammaShape))};
    NS_LOG_INFO("At time " << Simulator::Now().As(Time::S)
                           << " video traffic source schedule to sent " << size
                           << " bytes after latency of " << latency.As(Time::US));

    m_sendEvents[m_nextEventId] =
        Simulator::Schedule(latency, &VideoTraffic::Send, this, m_nextEventId, size, latency);
    ++m_nextEventId;
}

void
VideoTraffic::Send(uint64_t eventId, uint32_t size, Time latency)
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
VideoTraffic::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = true;
    ScheduleNextFrame();
}

void
VideoTraffic::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_FATAL_ERROR("Can't connect");
}

void
VideoTraffic::TxDone(Ptr<Socket> socket, uint32_t size)
{
    NS_LOG_FUNCTION(this << socket << size);
    if (m_unsentPackets.empty() && !m_generatedFrames.empty() && m_socket->GetTxAvailable() > 0)
    {
        SendWithLatency();
    }
}

void
VideoTraffic::TxAvailable(Ptr<Socket> socket, uint32_t available)
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
