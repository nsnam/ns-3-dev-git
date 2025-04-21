/*
 *  Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

#include "udp-server.h"

#include "packet-loss-counter.h"
#include "seq-ts-header.h"

#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpServer");

NS_OBJECT_ENSURE_REGISTERED(UdpServer);

TypeId
UdpServer::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UdpServer")
            .SetParent<SinkApplication>()
            .SetGroupName("Applications")
            .AddConstructor<UdpServer>()
            .AddAttribute("PacketWindowSize",
                          "The size of the window used to compute the packet loss. This value "
                          "should be a multiple of 8.",
                          UintegerValue(32),
                          MakeUintegerAccessor(&UdpServer::GetPacketWindowSize,
                                               &UdpServer::SetPacketWindowSize),
                          MakeUintegerChecker<uint16_t>(8, 256))
            .AddTraceSource("RxWithAddresses",
                            "A packet has been received",
                            MakeTraceSourceAccessor(&UdpServer::m_rxTraceWithAddresses),
                            "ns3::Packet::TwoAddressTracedCallback");
    return tid;
}

UdpServer::UdpServer()
    : SinkApplication(DEFAULT_PORT)
{
    NS_LOG_FUNCTION(this);
    m_protocolTid = TypeId::LookupByName("ns3::UdpSocketFactory");
}

UdpServer::~UdpServer()
{
    NS_LOG_FUNCTION(this);
}

uint16_t
UdpServer::GetPacketWindowSize() const
{
    return m_lossCounter.GetBitMapSize();
}

void
UdpServer::SetPacketWindowSize(uint16_t size)
{
    NS_LOG_FUNCTION(this << size);
    m_lossCounter.SetBitMapSize(size);
}

uint32_t
UdpServer::GetLost() const
{
    return m_lossCounter.GetLost();
}

uint64_t
UdpServer::GetReceived() const
{
    return m_received;
}

void
UdpServer::DoStartApplication()
{
    NS_LOG_FUNCTION(this);

    auto local = m_local;
    if (local.IsInvalid())
    {
        local = InetSocketAddress(Ipv4Address::GetAny(), m_port);
        NS_LOG_INFO(this << " Binding on port " << m_port << " / " << local << ".");
    }
    else if (InetSocketAddress::IsMatchingType(m_local))
    {
        const auto ipv4 = InetSocketAddress::ConvertFrom(m_local).GetIpv4();
        NS_LOG_INFO(this << " Binding on " << ipv4 << " port " << m_port << " / " << m_local
                         << ".");
    }
    else if (Ipv6Address::IsMatchingType(m_local))
    {
        const auto ipv6 = Inet6SocketAddress::ConvertFrom(m_local).GetIpv6();
        NS_LOG_INFO(this << " Binding on " << ipv6 << " port " << m_port << " / " << m_local
                         << ".");
    }
    if (m_socket->Bind(local) == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }
    m_socket->SetRecvCallback(MakeCallback(&UdpServer::HandleRead, this));

    if (m_local.IsInvalid())
    {
        auto local = Inet6SocketAddress(Ipv6Address::GetAny(), m_port);
        if (m_socket6->Bind(local) == -1)
        {
            NS_FATAL_ERROR("Failed to bind socket");
        }
        m_socket6->SetRecvCallback(MakeCallback(&UdpServer::HandleRead, this));
    }
}

void
UdpServer::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Address from;
    while (auto packet = socket->RecvFrom(from))
    {
        Address localAddress;
        socket->GetSockName(localAddress);
        m_rxTraceWithoutAddress(packet);
        m_rxTrace(packet, from);
        m_rxTraceWithAddresses(packet, from, localAddress);
        if (packet->GetSize() > 0)
        {
            const auto receivedSize = packet->GetSize();
            SeqTsHeader seqTs;
            packet->RemoveHeader(seqTs);
            const auto currentSequenceNumber = seqTs.GetSeq();
            if (InetSocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("TraceDelay: RX " << receivedSize << " bytes from "
                                              << InetSocketAddress::ConvertFrom(from).GetIpv4()
                                              << " Sequence Number: " << currentSequenceNumber
                                              << " Uid: " << packet->GetUid() << " TXtime: "
                                              << seqTs.GetTs() << " RXtime: " << Simulator::Now()
                                              << " Delay: " << Simulator::Now() - seqTs.GetTs());
            }
            else if (Inet6SocketAddress::IsMatchingType(from))
            {
                NS_LOG_INFO("TraceDelay: RX " << receivedSize << " bytes from "
                                              << Inet6SocketAddress::ConvertFrom(from).GetIpv6()
                                              << " Sequence Number: " << currentSequenceNumber
                                              << " Uid: " << packet->GetUid() << " TXtime: "
                                              << seqTs.GetTs() << " RXtime: " << Simulator::Now()
                                              << " Delay: " << Simulator::Now() - seqTs.GetTs());
            }

            m_lossCounter.NotifyReceived(currentSequenceNumber);
            m_received++;
        }
    }
}

} // Namespace ns3
