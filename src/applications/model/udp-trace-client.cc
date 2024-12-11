/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */
#include "udp-trace-client.h"

#include "seq-ts-header.h"

#include "ns3/address-utils.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UdpTraceClient");

NS_OBJECT_ENSURE_REGISTERED(UdpTraceClient);

/**
 * @brief Default trace to send
 */
UdpTraceClient::TraceEntry UdpTraceClient::g_defaultEntries[] = {
    {0, 534, 'I'},
    {40, 1542, 'P'},
    {120, 134, 'B'},
    {80, 390, 'B'},
    {240, 765, 'P'},
    {160, 407, 'B'},
    {200, 504, 'B'},
    {360, 903, 'P'},
    {280, 421, 'B'},
    {320, 587, 'B'},
};

TypeId
UdpTraceClient::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UdpTraceClient")
            .SetParent<SourceApplication>()
            .SetGroupName("Applications")
            .AddConstructor<UdpTraceClient>()
            .AddAttribute("RemoteAddress",
                          "The destination Address of the outbound packets",
                          AddressValue(),
                          MakeAddressAccessor(
                              (void(UdpTraceClient::*)(const Address&)) &
                              UdpTraceClient::SetRemote), // this is needed to indicate which
                                                          // version of the function overload to use
                          MakeAddressChecker(),
                          TypeId::SupportLevel::DEPRECATED,
                          "Replaced by Remote in ns-3.44.")
            .AddAttribute(
                "RemotePort",
                "The destination port of the outbound packets",
                UintegerValue(UdpTraceClient::DEFAULT_PORT),
                MakeAddressAccessor(
                    (void(UdpTraceClient::*)(const Address&)) &
                        UdpTraceClient::SetRemote, // this is needed to indicate which version of
                                                   // the function overload to use
                    &UdpTraceClient::GetRemote),
                MakeUintegerChecker<uint16_t>(),
                TypeId::SupportLevel::DEPRECATED,
                "Replaced by Remote in ns-3.44.")
            .AddAttribute("MaxPacketSize",
                          "The maximum size of a packet (including the SeqTsHeader, 12 bytes).",
                          UintegerValue(1024),
                          MakeUintegerAccessor(&UdpTraceClient::m_maxPacketSize),
                          MakeUintegerChecker<uint32_t>())
            .AddAttribute("TraceFilename",
                          "Name of file to load a trace from. By default, uses a hardcoded trace.",
                          StringValue(""),
                          MakeStringAccessor(&UdpTraceClient::SetTraceFile),
                          MakeStringChecker())
            .AddAttribute("TraceLoop",
                          "Loops through the trace file, starting again once it is over.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&UdpTraceClient::SetTraceLoop),
                          MakeBooleanChecker())

        ;
    return tid;
}

UdpTraceClient::UdpTraceClient()
    : m_sent{0},
      m_socket{nullptr},
      m_peerPort{},
      m_sendEvent{},
      m_currentEntry{0}
{
    NS_LOG_FUNCTION(this);
}

UdpTraceClient::~UdpTraceClient()
{
    NS_LOG_FUNCTION(this);
}

void
UdpTraceClient::SetRemote(const Address& ip, uint16_t port)
{
    NS_LOG_FUNCTION(this << ip << port);
    SetRemote(ip);
    SetPort(port);
}

void
UdpTraceClient::SetRemote(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    if (!addr.IsInvalid())
    {
        m_peer = addr;
        if (m_peerPort)
        {
            SetPort(*m_peerPort);
        }
        LoadTrace();
    }
}

Address
UdpTraceClient::GetRemote() const
{
    return m_peer;
}

void
UdpTraceClient::SetPort(uint16_t port)
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
UdpTraceClient::GetPort() const
{
    if (m_peer.IsInvalid())
    {
        return m_peerPort.value_or(UdpTraceClient::DEFAULT_PORT);
    }
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        return InetSocketAddress::ConvertFrom(m_peer).GetPort();
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        return Inet6SocketAddress::ConvertFrom(m_peer).GetPort();
    }
    return UdpTraceClient::DEFAULT_PORT;
}

void
UdpTraceClient::SetTraceFile(const std::string& traceFile)
{
    NS_LOG_FUNCTION(this << traceFile);
    m_traceFile = traceFile;
    LoadTrace();
}

void
UdpTraceClient::SetMaxPacketSize(uint16_t maxPacketSize)
{
    NS_LOG_FUNCTION(this << maxPacketSize);
    m_maxPacketSize = maxPacketSize;
}

uint16_t
UdpTraceClient::GetMaxPacketSize()
{
    NS_LOG_FUNCTION(this);
    return m_maxPacketSize;
}

void
UdpTraceClient::LoadTrace()
{
    NS_LOG_FUNCTION(this);
    m_entries.clear();
    m_currentEntry = 0;

    if (m_traceFile.empty())
    {
        LoadDefaultTrace();
        return;
    }

    std::ifstream ifTraceFile;
    ifTraceFile.open(m_traceFile, std::ifstream::in);
    if (!ifTraceFile.good())
    {
        LoadDefaultTrace();
        return;
    }

    uint32_t oldIndex = 0;
    uint32_t prevTime = 0;
    while (ifTraceFile.good())
    {
        uint32_t index = 0;
        char frameType;
        uint32_t time = 0;
        uint32_t size = 0;
        ifTraceFile >> index >> frameType >> time >> size;

        if (index == oldIndex)
        {
            continue;
        }

        TraceEntry entry{};
        if (frameType == 'B')
        {
            entry.timeToSend = 0;
        }
        else
        {
            entry.timeToSend = time - prevTime;
            prevTime = time;
        }
        entry.packetSize = size;
        entry.frameType = frameType;
        m_entries.push_back(entry);
        oldIndex = index;
    }

    ifTraceFile.close();
    NS_ASSERT_MSG(prevTime != 0, "A trace file can not contain B frames only.");
}

void
UdpTraceClient::LoadDefaultTrace()
{
    NS_LOG_FUNCTION(this);
    uint32_t prevTime = 0;
    for (uint32_t i = 0; i < (sizeof(g_defaultEntries) / sizeof(TraceEntry)); i++)
    {
        TraceEntry entry = g_defaultEntries[i];
        if (entry.frameType == 'B')
        {
            entry.timeToSend = 0;
        }
        else
        {
            uint32_t tmp = entry.timeToSend;
            entry.timeToSend -= prevTime;
            prevTime = tmp;
        }
        m_entries.push_back(entry);
    }
}

void
UdpTraceClient::StartApplication()
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
    m_sendEvent = Simulator::ScheduleNow(&UdpTraceClient::Send, this);
}

void
UdpTraceClient::StopApplication()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_sendEvent);
}

void
UdpTraceClient::SendPacket(uint32_t size)
{
    NS_LOG_FUNCTION(this << size);
    uint32_t packetSize;
    if (size > 12)
    {
        packetSize = size - 12; // 12 is the size of the SeqTsHeader
    }
    else
    {
        packetSize = 0;
    }
    auto p = Create<Packet>(packetSize);
    SeqTsHeader seqTs;
    seqTs.SetSeq(m_sent);
    p->AddHeader(seqTs);

    std::stringstream addressString{};
#ifdef NS3_LOG_ENABLE
    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        addressString << InetSocketAddress::ConvertFrom(m_peer).GetIpv4() << ":"
                      << InetSocketAddress::ConvertFrom(m_peer).GetPort();
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        addressString << Inet6SocketAddress::ConvertFrom(m_peer).GetIpv6() << ":"
                      << Inet6SocketAddress::ConvertFrom(m_peer).GetPort();
    }
#endif

    if ((m_socket->Send(p)) >= 0)
    {
        ++m_sent;
        NS_LOG_INFO("Sent " << size << " bytes to " << addressString.str());
    }
    else
    {
        NS_LOG_INFO("Error while sending " << size << " bytes to " << addressString.str());
    }
}

void
UdpTraceClient::Send()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_sendEvent.IsExpired());

    bool cycled = false;
    Ptr<Packet> p;
    TraceEntry* entry = &m_entries[m_currentEntry];
    do
    {
        for (uint32_t i = 0; i < entry->packetSize / m_maxPacketSize; i++)
        {
            SendPacket(m_maxPacketSize);
        }

        auto sizetosend = entry->packetSize % m_maxPacketSize;
        SendPacket(sizetosend);

        m_currentEntry++;
        if (m_currentEntry >= m_entries.size())
        {
            m_currentEntry = 0;
            cycled = true;
        }
        entry = &m_entries[m_currentEntry];
    } while (entry->timeToSend == 0);

    if (!cycled || m_traceLoop)
    {
        m_sendEvent =
            Simulator::Schedule(MilliSeconds(entry->timeToSend), &UdpTraceClient::Send, this);
    }
}

void
UdpTraceClient::SetTraceLoop(bool traceLoop)
{
    m_traceLoop = traceLoop;
}

} // Namespace ns3
