/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "virtual-desktop.h"

#include "ns3/attribute-container.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet.h"
#include "ns3/pair.h"
#include "ns3/pointer.h"
#include "ns3/random-variable-stream.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/string.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VirtualDesktop");

NS_OBJECT_ENSURE_REGISTERED(VirtualDesktop);

TypeId
VirtualDesktop::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VirtualDesktop")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<VirtualDesktop>()
            .AddAttribute(
                "Protocol",
                "The type of protocol to use. This should be a subclass of ns3::SocketFactory",
                TypeIdValue(TypeId::LookupByName("ns3::TcpSocketFactory")),
                MakeTypeIdAccessor(&VirtualDesktop::m_tid),
                MakeTypeIdChecker())
            .AddAttribute(
                "InitialPacketArrival",
                "A uniform random variable to generate the initial packet arrival in nanoseconds.",
                StringValue("ns3::UniformRandomVariable[Min=0|Max=20000000]"),
                MakePointerAccessor(&VirtualDesktop::m_initialArrivalUniform),
                MakePointerChecker<UniformRandomVariable>())
            .AddAttribute("InterPacketArrivals",
                          "An exponential random variable to generate the inter packet arrivals in "
                          "nanoseconds.",
                          StringValue("ns3::ExponentialRandomVariable[Mean=60226900]"),
                          MakePointerAccessor(&VirtualDesktop::m_interArrivalExponential),
                          MakePointerChecker<ExponentialRandomVariable>())
            .AddAttribute(
                "ParametersPacketSize",
                "The mean value and standard deviation for each mode of the multimodal normal "
                "distribution used to calculate packet sizes.",
                StringValue("41.0 3.2;1478.3 11.6"),
                MakeAttributeContainerAccessor<PairValue<DoubleValue, DoubleValue>, ';'>(
                    &VirtualDesktop::SetParametersPacketSize),
                MakeAttributeContainerChecker<PairValue<DoubleValue, DoubleValue>, ';'>(
                    MakePairChecker<DoubleValue, DoubleValue>(MakeDoubleChecker<double>(),
                                                              MakeDoubleChecker<double>())));
    return tid;
}

VirtualDesktop::VirtualDesktop()
    : m_socket{nullptr},
      m_connected{false},
      m_initialPacket{true},
      m_unsentPacket{nullptr}
{
    NS_LOG_FUNCTION(this);
}

VirtualDesktop::~VirtualDesktop()
{
    NS_LOG_FUNCTION(this);
}

int64_t
VirtualDesktop::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    auto currentStream = stream;
    m_initialArrivalUniform->SetStream(currentStream++);
    m_interArrivalExponential->SetStream(currentStream++);
    if (m_pktSizeDistributions.size() > 1)
    {
        m_dlModeSelection->SetStream(currentStream++);
    }
    for (auto& pktSizeDistribution : m_pktSizeDistributions)
    {
        pktSizeDistribution->SetStream(currentStream++);
    }
    return (currentStream - stream);
}

void
VirtualDesktop::DoDispose()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    m_socket = nullptr;
    SourceApplication::DoDispose();
}

void
VirtualDesktop::SetParametersPacketSize(const std::vector<std::pair<double, double>>& params)
{
    m_pktSizeDistributions.clear();
    NS_LOG_FUNCTION(this << params.size());
    for (const auto& [mean, std] : params)
    {
        auto normal = CreateObject<NormalRandomVariable>();
        normal->SetAttribute("Mean", DoubleValue(mean));
        normal->SetAttribute("Variance", DoubleValue(std::pow(std, 2)));
        m_pktSizeDistributions.push_back(normal);
    }
    if ((m_pktSizeDistributions.size() > 1) && !m_dlModeSelection)
    {
        m_dlModeSelection = CreateObject<UniformRandomVariable>();
    }
}

void
VirtualDesktop::StartApplication()
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

        m_socket->SetConnectCallback(MakeCallback(&VirtualDesktop::ConnectionSucceeded, this),
                                     MakeCallback(&VirtualDesktop::ConnectionFailed, this));
        m_socket->SetDataSentCallback(MakeCallback(&VirtualDesktop::TxDone, this));
        m_socket->SetSendCallback(MakeCallback(&VirtualDesktop::TxAvailable, this));

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
VirtualDesktop::StopApplication()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    if (m_socket)
    {
        m_socket->Close();
    }
}

void
VirtualDesktop::CancelEvents()
{
    NS_LOG_FUNCTION(this);
    m_txEvent.Cancel();
    if (m_unsentPacket)
    {
        NS_LOG_DEBUG("Discarding cached packet upon CancelEvents ()");
    }
    m_unsentPacket = nullptr;
    m_initialPacket = true;
}

Time
VirtualDesktop::GetInterArrival() const
{
    if (m_unsentPacket)
    {
        return Time();
    }
    if (m_initialPacket)
    {
        return NanoSeconds(m_initialArrivalUniform->GetValue());
    }
    return NanoSeconds(m_interArrivalExponential->GetValue());
}

uint32_t
VirtualDesktop::GetPacketSize() const
{
    const auto mode = (m_pktSizeDistributions.size() > 1)
                          ? m_dlModeSelection->GetInteger(0, m_pktSizeDistributions.size() - 1)
                          : 0;
    return m_pktSizeDistributions.at(mode)->GetValue();
}

void
VirtualDesktop::ScheduleNext()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(!m_txEvent.IsPending());
    m_txEvent = Simulator::Schedule(GetInterArrival(), &VirtualDesktop::SendPacket, this);
}

void
VirtualDesktop::SendPacket()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_txEvent.IsExpired());

    Ptr<Packet> packet;
    if (m_unsentPacket)
    {
        packet = m_unsentPacket;
    }
    else
    {
        packet = Create<Packet>(GetPacketSize());
    }

    unsigned int actualSize = m_socket->Send(packet);
    if (actualSize == packet->GetSize())
    {
        m_txTrace(packet);
        m_unsentPacket = nullptr;
        Address localAddress;
        m_socket->GetSockName(localAddress);
        if (InetSocketAddress::IsMatchingType(m_peer))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " VDI traffic source sent "
                                   << (m_initialPacket ? "initial packet of " : "packet of ")
                                   << packet->GetSize() << " bytes to "
                                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << " port "
                                   << InetSocketAddress::ConvertFrom(m_peer).GetPort());
        }
        else if (Inet6SocketAddress::IsMatchingType(m_peer))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " VDI traffic source sent "
                                   << (m_initialPacket ? "initial packet of " : "packet of ")
                                   << packet->GetSize() << " bytes to "
                                   << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << " port "
                                   << Inet6SocketAddress::ConvertFrom(m_peer).GetPort());
        }
    }
    else
    {
        NS_LOG_DEBUG("Unable to send VDI packet; actual " << actualSize << " size "
                                                          << packet->GetSize()
                                                          << "; caching for later attempt");
        m_unsentPacket = packet;
    }

    if (m_initialPacket)
    {
        m_initialPacket = false;
    }

    ScheduleNext();
}

void
VirtualDesktop::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = true;
    ScheduleNext();
}

void
VirtualDesktop::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    NS_FATAL_ERROR("Can't connect");
}

void
VirtualDesktop::TxDone(Ptr<Socket> socket, uint32_t size)
{
    NS_LOG_FUNCTION(this << socket << size);
    if (m_unsentPacket)
    {
        ScheduleNext();
    }
}

void
VirtualDesktop::TxAvailable(Ptr<Socket> socket, uint32_t available)
{
    NS_LOG_FUNCTION(this << socket << available);
    if (m_unsentPacket)
    {
        ScheduleNext();
    }
}

} // Namespace ns3
