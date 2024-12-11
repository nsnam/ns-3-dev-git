/*
 * Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

#include "udp-client.h"

#include "seq-ts-header.h"

#include "ns3/address-utils.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

#include <cstdio>
#include <cstdlib>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpClient");

NS_OBJECT_ENSURE_REGISTERED(UdpClient);

TypeId
UdpClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UdpClient")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<UdpClient>()
            .AddAttribute(
                "MaxPackets",
                "The maximum number of packets the application will send (zero means infinite)",
                UintegerValue(100),
                MakeUintegerAccessor(&UdpClient::m_count),
                MakeUintegerChecker<uint32_t>())
            .AddAttribute("Interval",
                          "The time to wait between packets",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&UdpClient::m_interval),
                          MakeTimeChecker())
            .AddAttribute("RemoteAddress",
                          "The destination Address of the outbound packets",
                          AddressValue(),
                          MakeAddressAccessor(
                              (void(UdpClient::*)(const Address&)) &
                                  UdpClient::SetRemote, // this is needed to indicate which version
                                                        // of the function overload to use
                              &UdpClient::GetRemote),
                          MakeAddressChecker(),
                          TypeId::SupportLevel::DEPRECATED,
                          "Replaced by Remote in ns-3.44.")
            .AddAttribute("RemotePort",
                          "The destination port of the outbound packets",
                          UintegerValue(UdpClient::DEFAULT_PORT),
                          MakeUintegerAccessor(&UdpClient::SetPort, &UdpClient::GetPort),
                          MakeUintegerChecker<uint16_t>(),
                          TypeId::SupportLevel::DEPRECATED,
                          "Replaced by Remote in ns-3.44.")
            .AddAttribute("PacketSize",
                          "Size of packets generated. The minimum packet size is 12 bytes which is "
                          "the size of the header carrying the sequence number and the time stamp.",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&UdpClient::m_size),
                          MakeUintegerChecker<uint32_t>(12, 65507))
            .AddTraceSource("Tx",
                            "A new packet is created and sent",
                            MakeTraceSourceAccessor(&UdpClient::m_txTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("TxWithAddresses",
                            "A new packet is created and sent",
                            MakeTraceSourceAccessor(&UdpClient::m_txTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    return tid;
}

UdpClient::UdpClient()
    : m_sent{0},
      m_totalTx{0},
      m_socket{nullptr},
      m_peerPort{},
      m_sendEvent{}
{
    NS_LOG_FUNCTION(this);
}

UdpClient::~UdpClient()
{
    NS_LOG_FUNCTION(this);
}

void
UdpClient::SetRemote(const Address& ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    SetRemote(ip);
    SetPort(port);
}

void
UdpClient::SetRemote(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    if (!addr.IsInvalid())
    {
        m_peer = addr;
        if (m_peerPort)
        {
            SetPort(*m_peerPort);
        }
    }
}

Address
UdpClient::GetRemote() const
{
    return m_peer;
}

void
UdpClient::SetPort(uint16_t port)
{
    NS_LOG_FUNCTION(this << port);
    if (m_peer.IsInvalid())
    {
        // save for later
        m_peerPort = port;
        return;
    }
    if (Ipv4Address::IsMatchingType(m_peer) || Ipv6Address::IsMatchingType(m_peer))
    {
        m_peer = addressUtils::ConvertToSocketAddress(m_peer, port);
    }
}

uint16_t
UdpClient::GetPort() const
{
    if (m_peer.IsInvalid())
    {
        return m_peerPort.value_or(UdpClient::DEFAULT_PORT);
    }
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        return InetSocketAddress::ConvertFrom(m_peer).GetPort();
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        return Inet6SocketAddress::ConvertFrom(m_peer).GetPort();
    }
    return UdpClient::DEFAULT_PORT;
}

void
UdpClient::StartApplication()
{
    NS_LOG_FUNCTION(this);

    if (!m_socket)
    {
        auto tid = TypeId::LookupByName("ns3::UdpSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        NS_ABORT_MSG_IF(m_peer.IsInvalid(), "Remote address not properly set");
        if (!m_local.IsInvalid())
        {
            NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                             InetSocketAddress::IsMatchingType(m_local)) ||
                                (InetSocketAddress::IsMatchingType(m_peer) &&
                                 Inet6SocketAddress::IsMatchingType(m_local)),
                            "Incompatible peer and local address IP version");
            if (m_socket->Bind(m_local) == -1)
            {
                NS_FATAL_ERROR("Failed to bind socket");
            }
        }
        else
        {
            if (InetSocketAddress::IsMatchingType(m_peer))
            {
                if (m_socket->Bind() == -1)
                {
                    NS_FATAL_ERROR("Failed to bind socket");
                }
            }
            else if (Inet6SocketAddress::IsMatchingType(m_peer))
            {
                if (m_socket->Bind6() == -1)
                {
                    NS_FATAL_ERROR("Failed to bind socket");
                }
            }
            else
            {
                NS_ASSERT_MSG(false, "Incompatible address type: " << m_peer);
            }
        }
        m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
        m_socket->Connect(m_peer);
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        m_socket->SetAllowBroadcast(true);
    }

#ifdef NS3_LOG_ENABLE
    std::stringstream peerAddressStringStream;
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        peerAddressStringStream << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << ":"
                                << InetSocketAddress::ConvertFrom(m_peer).GetPort();
        ;
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        peerAddressStringStream << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << ":"
                                << Inet6SocketAddress::ConvertFrom(m_peer).GetPort();
    }
    m_peerString = peerAddressStringStream.str();
#endif // NS3_LOG_ENABLE

    m_sendEvent = Simulator::Schedule(Seconds(0), &UdpClient::Send, this);
}

void
UdpClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_sendEvent);
}

void
UdpClient::Send()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_sendEvent.IsExpired());

    Address from;
    Address to;
    m_socket->GetSockName(from);
    m_socket->GetPeerName(to);
    SeqTsHeader seqTs;
    seqTs.SetSeq(m_sent);
    NS_ABORT_IF(m_size < seqTs.GetSerializedSize());
    auto p = Create<Packet>(m_size - seqTs.GetSerializedSize());

    // Trace before adding header, for consistency with PacketSink
    m_txTrace(p);
    m_txTraceWithAddresses(p, from, to);

    p->AddHeader(seqTs);

    if ((m_socket->Send(p)) >= 0)
    {
        ++m_sent;
        m_totalTx += p->GetSize();
#ifdef NS3_LOG_ENABLE
        NS_LOG_INFO("TraceDelay TX " << m_size << " bytes to " << m_peerString << " Uid: "
                                     << p->GetUid() << " Time: " << (Simulator::Now()).As(Time::S));
#endif // NS3_LOG_ENABLE
    }
#ifdef NS3_LOG_ENABLE
    else
    {
        NS_LOG_INFO("Error while sending " << m_size << " bytes to " << m_peerString);
    }
#endif // NS3_LOG_ENABLE

    if (m_sent < m_count || m_count == 0)
    {
        m_sendEvent = Simulator::Schedule(m_interval, &UdpClient::Send, this);
    }
}

uint64_t
UdpClient::GetTotalTx() const
{
    return m_totalTx;
}

} // Namespace ns3
