/*
 * Copyright 2007 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Tom Henderson (tomhend@u.washington.edu)
 */

#include "packet-sink.h"

#include "ns3/address-utils.h"
#include "ns3/boolean.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/udp-socket.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketSink");

NS_OBJECT_ENSURE_REGISTERED(PacketSink);

TypeId
PacketSink::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PacketSink")
            .SetParent<SinkApplication>()
            .SetGroupName("Applications")
            .AddConstructor<PacketSink>()
            .AddAttribute("Protocol",
                          "The type id of the protocol to use for the rx socket.",
                          TypeIdValue(UdpSocketFactory::GetTypeId()),
                          MakeTypeIdAccessor(&PacketSink::m_tid),
                          MakeTypeIdChecker())
            .AddAttribute("EnableSeqTsSizeHeader",
                          "Enable optional header tracing of SeqTsSizeHeader",
                          BooleanValue(false),
                          MakeBooleanAccessor(&PacketSink::m_enableSeqTsSizeHeader),
                          MakeBooleanChecker())
            .AddTraceSource("Rx",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&PacketSink::m_rxTrace),
                            "ns3::Packet::AddressTracedCallback")
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&PacketSink::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback")
            .AddTraceSource("RxWithSeqTsSize",
                            "A packet with SeqTsSize header has been received",
                            MakeTraceSourceAccessor(&PacketSink::m_rxTraceWithSeqTsSize),
                            "ns3::PacketSink::SeqTsSizeCallback");
    return tid;
}

PacketSink::PacketSink()
    : m_buffer{},
      m_socket{nullptr},
      m_socket6{nullptr},
      m_socketList{},
      m_totalRx{0},
      m_enableSeqTsSizeHeader{false}
{
    NS_LOG_FUNCTION(this);
}

PacketSink::~PacketSink()
{
    NS_LOG_FUNCTION(this);
}

uint64_t
PacketSink::GetTotalRx() const
{
    NS_LOG_FUNCTION(this);
    return m_totalRx;
}

Ptr<Socket>
PacketSink::GetListeningSocket() const
{
    NS_LOG_FUNCTION(this);
    return m_socket;
}

std::list<Ptr<Socket>>
PacketSink::GetAcceptedSockets() const
{
    NS_LOG_FUNCTION(this);
    return m_socketList;
}

void
PacketSink::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_socket = nullptr;
    m_socketList.clear();

    // chain up
    Application::DoDispose();
}

// Application Methods
void
PacketSink::StartApplication() // Called at time specified by Start
{
    NS_LOG_FUNCTION(this);

    // Create the socket if not already
    if (!m_socket)
    {
        m_socket = Socket::CreateSocket(GetNode(), m_tid);
        auto local = m_local;
        if (local.IsInvalid())
        {
            local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
            NS_LOG_INFO(this << " Binding on port " << m_port << " / " << local << ".");
        }
        else if (InetSocketAddress::IsMatchingType(local))
        {
            m_port = InetSocketAddress::ConvertFrom(local).GetPort();
            const auto ipv4 = InetSocketAddress::ConvertFrom(local).GetIpv4();
            NS_LOG_INFO(this << " Binding on " << ipv4 << " port " << m_port << " / " << local
                             << ".");
        }
        else if (Inet6SocketAddress::IsMatchingType(local))
        {
            m_port = Inet6SocketAddress::ConvertFrom(local).GetPort();
            const auto ipv6 = Inet6SocketAddress::ConvertFrom(local).GetIpv6();
            NS_LOG_INFO(this << " Binding on " << ipv6 << " port " << m_port << " / " << local
                             << ".");
        }
        if (m_socket->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket->Listen();
        m_socket->ShutdownSend();
        if (addressUtils::IsMulticast(local))
        {
            if (auto udpSocket = DynamicCast<UdpSocket>(m_socket))
            {
                // equivalent to setsockopt (MCAST_JOIN_GROUP)
                udpSocket->MulticastJoinGroup(0, local);
            }
            else
            {
                NS_FATAL_ERROR("Error: joining multicast on a non-UDP socket");
            }
        }
        m_socket->SetRecvCallback(MakeCallback(&PacketSink::HandleRead, this));
        m_socket->SetRecvPktInfo(true);
        m_socket->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                    MakeCallback(&PacketSink::HandleAccept, this));
        m_socket->SetCloseCallbacks(MakeCallback(&PacketSink::HandlePeerClose, this),
                                    MakeCallback(&PacketSink::HandlePeerError, this));
    }

    if (m_local.IsInvalid() && !m_socket6)
    {
        // local address is not specified, so create another socket to also listen to all IPv6
        // addresses
        m_socket6 = Socket::CreateSocket(GetNode(), m_tid);
        auto local = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
        if (m_socket6->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket6->Listen();
        m_socket6->ShutdownSend();
        if (addressUtils::IsMulticast(local))
        {
            if (auto udpSocket = DynamicCast<UdpSocket>(m_socket6))
            {
                // equivalent to setsockopt (MCAST_JOIN_GROUP)
                udpSocket->MulticastJoinGroup(0, local);
            }
            else
            {
                NS_FATAL_ERROR("Error: joining multicast on a non-UDP socket");
            }
        }
        m_socket6->SetRecvCallback(MakeCallback(&PacketSink::HandleRead, this));
        m_socket6->SetRecvPktInfo(true);
        m_socket6->SetAcceptCallback(MakeNullCallback<bool, Ptr<Socket>, const Address&>(),
                                     MakeCallback(&PacketSink::HandleAccept, this));
        m_socket6->SetCloseCallbacks(MakeCallback(&PacketSink::HandlePeerClose, this),
                                     MakeCallback(&PacketSink::HandlePeerError, this));
    }
}

void
PacketSink::StopApplication() // Called at time specified by Stop
{
    NS_LOG_FUNCTION(this);
    while (!m_socketList.empty()) // these are accepted sockets, close them
    {
        auto acceptedSocket = m_socketList.front();
        m_socketList.pop_front();
        acceptedSocket->Close();
    }
    if (m_socket)
    {
        m_socket->Close();
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
    if (m_socket6)
    {
        m_socket6->Close();
        m_socket6->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    }
}

void
PacketSink::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Address from;
    while (auto packet = socket->RecvFrom(from))
    {
        if (packet->GetSize() == 0)
        { // EOF
            break;
        }
        m_totalRx += packet->GetSize();
        if (InetSocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " packet sink received "
                                   << packet->GetSize() << " bytes from "
                                   << InetSocketAddress::ConvertFrom(from).GetIpv4() << " port "
                                   << InetSocketAddress::ConvertFrom(from).GetPort() << " total Rx "
                                   << m_totalRx << " bytes");
        }
        else if (Inet6SocketAddress::IsMatchingType(from))
        {
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " packet sink received "
                                   << packet->GetSize() << " bytes from "
                                   << Inet6SocketAddress::ConvertFrom(from).GetIpv6() << " port "
                                   << Inet6SocketAddress::ConvertFrom(from).GetPort()
                                   << " total Rx " << m_totalRx << " bytes");
        }

        if (!m_rxTrace.IsEmpty() || !m_rxTraceWithAddresses.IsEmpty() ||
            (!m_rxTraceWithSeqTsSize.IsEmpty() && m_enableSeqTsSizeHeader))
        {
            Address localAddress;
            Ipv4PacketInfoTag interfaceInfo;
            Ipv6PacketInfoTag interface6Info;
            if (packet->RemovePacketTag(interfaceInfo))
            {
                localAddress = InetSocketAddress(interfaceInfo.GetAddress(), m_port);
            }
            else if (packet->RemovePacketTag(interface6Info))
            {
                localAddress = Inet6SocketAddress(interface6Info.GetAddress(), m_port);
            }
            else
            {
                socket->GetSockName(localAddress);
            }
            m_rxTrace(packet, from);
            m_rxTraceWithAddresses(packet, from, localAddress);

            if (!m_rxTraceWithSeqTsSize.IsEmpty() && m_enableSeqTsSizeHeader)
            {
                PacketReceived(packet, from, localAddress);
            }
        }
    }
}

void
PacketSink::PacketReceived(const Ptr<Packet>& p, const Address& from, const Address& localAddress)
{
    auto itBuffer = m_buffer.find(from);
    if (itBuffer == m_buffer.end())
    {
        itBuffer = m_buffer.emplace(from, Create<Packet>(0)).first;
    }

    auto buffer = itBuffer->second;
    buffer->AddAtEnd(p);

    SeqTsSizeHeader header;
    buffer->PeekHeader(header);

    NS_ABORT_IF(header.GetSize() == 0);

    while (buffer->GetSize() >= header.GetSize())
    {
        NS_LOG_DEBUG("Removing packet of size " << header.GetSize() << " from buffer of size "
                                                << buffer->GetSize());
        auto complete = buffer->CreateFragment(0, static_cast<uint32_t>(header.GetSize()));
        buffer->RemoveAtStart(static_cast<uint32_t>(header.GetSize()));

        complete->RemoveHeader(header);

        m_rxTraceWithSeqTsSize(complete, from, localAddress, header);

        if (buffer->GetSize() > header.GetSerializedSize())
        {
            buffer->PeekHeader(header);
        }
        else
        {
            break;
        }
    }
}

void
PacketSink::HandlePeerClose(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

void
PacketSink::HandlePeerError(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

void
PacketSink::HandleAccept(Ptr<Socket> s, const Address& from)
{
    NS_LOG_FUNCTION(this << s << from);
    s->SetRecvCallback(MakeCallback(&PacketSink::HandleRead, this));
    m_socketList.push_back(s);
}

} // Namespace ns3
