/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "mobile-gaming.h"

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

NS_LOG_COMPONENT_DEFINE("MobileGaming");

NS_OBJECT_ENSURE_REGISTERED(MobileGaming);

TypeId
MobileGaming::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MobileGaming")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<MobileGaming>()
            .AddAttribute(
                "Protocol",
                "The type of protocol to use. This should be a subclass of ns3::SocketFactory",
                TypeIdValue(UdpSocketFactory::GetTypeId()),
                MakeTypeIdAccessor(&MobileGaming::m_tid),
                MakeTypeIdChecker())
            .AddAttribute(
                "InitialPacketSize",
                "A uniform random variable to generate size in bytes for initial packet payload.",
                StringValue("ns3::UniformRandomVariable[Min=0|Max=20]"),
                MakePointerAccessor(&MobileGaming::m_initialSizeUniform),
                MakePointerChecker<UniformRandomVariable>())
            .AddAttribute(
                "EndPacketSize",
                "A uniform random variable to generate size in bytes for end packet payload.",
                StringValue("ns3::UniformRandomVariable[Min=500|Max=600]"),
                MakePointerAccessor(&MobileGaming::m_endSizeUniform),
                MakePointerChecker<UniformRandomVariable>())
            .AddAttribute(
                "PacketSizeLev",
                "A largest extreme value random variable to calculate packet sizes in bytes.",
                StringValue("ns3::LargestExtremeValueRandomVariable[Location=50|Scale=11.0]"),
                MakePointerAccessor(&MobileGaming::m_levSizes),
                MakePointerChecker<LargestExtremeValueRandomVariable>())
            .AddAttribute(
                "PacketArrivalLev",
                "A largest extreme value random variable to calculate packet packet arrivals in "
                "microseconds.",
                StringValue("ns3::LargestExtremeValueRandomVariable[Location=13000|Scale=3700]"),
                MakePointerAccessor(&MobileGaming::m_levArrivals),
                MakePointerChecker<LargestExtremeValueRandomVariable>())
            .AddTraceSource("Tx",
                            "A packet is sent",
                            MakeTraceSourceAccessor(&MobileGaming::m_txTrace),
                            "ns3::MobileGaming::TxTracedCallback");
    return tid;
}

MobileGaming::MobileGaming()
    : m_socket{nullptr},
      m_connected{false},
      m_currentStage{TrafficModelStage::INITIAL},
      m_txEvent{}
{
    NS_LOG_FUNCTION(this);
}

MobileGaming::~MobileGaming()
{
    NS_LOG_FUNCTION(this);
}

int64_t
MobileGaming::AssignStreams(int64_t stream)
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
MobileGaming::DoDispose()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    m_socket = nullptr;
    SourceApplication::DoDispose();
}

void
MobileGaming::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);

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

        m_socket->SetConnectCallback(MakeCallback(&MobileGaming::ConnectionSucceeded, this),
                                     MakeCallback(&MobileGaming::ConnectionFailed, this));

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
        ScheduleNext();
    }
}

void
MobileGaming::StopApplication()
{
    NS_LOG_FUNCTION(this);
    m_currentStage = TrafficModelStage::ENDING;
}

void
MobileGaming::CancelEvents()
{
    NS_LOG_FUNCTION(this);
    m_txEvent.Cancel();
    m_currentStage = TrafficModelStage::INITIAL;
}

void
MobileGaming::ScheduleNext()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_txEvent.IsPending());
    const auto delay = MicroSeconds(m_levArrivals->GetValue());
    m_txEvent = Simulator::Schedule(delay, &MobileGaming::SendPacket, this);
}

void
MobileGaming::SendPacket()
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
    NS_ABORT_IF(actualSize != packetSize);
    m_txTrace(packet, stage);

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
        CancelEvents();
        if (m_socket)
        {
            m_socket->Close();
        }
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
MobileGaming::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = true;
    ScheduleNext();
}

void
MobileGaming::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_FATAL_ERROR("Can't connect");
}

std::ostream&
operator<<(std::ostream& os, MobileGaming::TrafficModelStage stage)
{
    switch (stage)
    {
    case MobileGaming::TrafficModelStage::INITIAL:
        os << "Initial stage";
        break;
    case MobileGaming::TrafficModelStage::GAMING:
        os << "Gaming stage";
        break;
    case MobileGaming::TrafficModelStage::ENDING:
        os << "Ending stage";
        break;
    default:
        NS_FATAL_ERROR("Unknown gaming traffic model stage");
    }
    return os;
}

} // Namespace ns3
