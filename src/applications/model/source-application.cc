/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "source-application.h"

#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/log.h"
#include "ns3/packet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SourceApplication");

NS_OBJECT_ENSURE_REGISTERED(SourceApplication);

TypeId
SourceApplication::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SourceApplication")
            .SetParent<Application>()
            .SetGroupName("Applications")
            .AddAttribute(
                "Remote",
                "The address of the destination, made of the remote IP address and the "
                "destination port",
                AddressValue(),
                MakeAddressAccessor(&SourceApplication::SetRemote, &SourceApplication::GetRemote),
                MakeAddressChecker())
            .AddAttribute("Local",
                          "The Address on which to bind the socket. If not set, it is generated "
                          "automatically when needed by the application.",
                          AddressValue(),
                          MakeAddressAccessor(&SourceApplication::m_local),
                          MakeAddressChecker())
            .AddAttribute("Tos",
                          "The Type of Service used to send IPv4 packets. "
                          "All 8 bits of the TOS byte are set (including ECN bits).",
                          UintegerValue(0),
                          MakeUintegerAccessor(&SourceApplication::m_tos),
                          MakeUintegerChecker<uint8_t>())
            .AddTraceSource("Tx",
                            "A packet is sent",
                            MakeTraceSourceAccessor(&SourceApplication::m_txTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

SourceApplication::SourceApplication(bool allowPacketSocket)
    : m_allowPacketSocket{allowPacketSocket}
{
    NS_LOG_FUNCTION(this);
}

SourceApplication::~SourceApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SourceApplication::DoDispose()
{
    NS_LOG_FUNCTION(this);
    CancelEvents();
    m_socket = nullptr;
    Application::DoDispose();
}

void
SourceApplication::SetRemote(const Address& addr)
{
    NS_LOG_FUNCTION(this << addr);
    if (!addr.IsInvalid())
    {
        m_peer = addr;
    }
}

Address
SourceApplication::GetRemote() const
{
    return m_peer;
}

Ptr<Socket>
SourceApplication::GetSocket() const
{
    return m_socket;
}

void
SourceApplication::StartApplication()
{
    NS_LOG_FUNCTION(this);

    // note: it is currently not possible to restart an application

    NS_ABORT_MSG_IF(m_peer.IsInvalid(), "Remote address not properly set");
    if (!m_local.IsInvalid())
    {
        NS_ABORT_MSG_IF((Inet6SocketAddress::IsMatchingType(m_peer) &&
                         InetSocketAddress::IsMatchingType(m_local)) ||
                            (InetSocketAddress::IsMatchingType(m_peer) &&
                             Inet6SocketAddress::IsMatchingType(m_local)),
                        "Incompatible peer and local address IP version");
    }

    m_socket = Socket::CreateSocket(GetNode(), m_protocolTid);
    m_socket->SetConnectCallback(MakeCallback(&SourceApplication::ConnectionSucceeded, this),
                                 MakeCallback(&SourceApplication::ConnectionFailed, this));

    int ret{-1};
    if (InetSocketAddress::IsMatchingType(m_peer) ||
        (m_allowPacketSocket && PacketSocketAddress::IsMatchingType(m_peer)))
    {
        ret = m_socket->Bind();
    }
    else if (Inet6SocketAddress::IsMatchingType(m_peer))
    {
        ret = m_socket->Bind6();
    }
    else
    {
        NS_FATAL_ERROR("Incompatible address type: " << m_peer);
    }
    if (ret == -1)
    {
        NS_FATAL_ERROR("Failed to bind socket");
    }

    if (InetSocketAddress::IsMatchingType(m_peer))
    {
        m_socket->SetIpTos(m_tos); // Affects only IPv4 sockets.
    }

    m_socket->Connect(m_peer);

    CancelEvents();

    DoStartApplication();
}

void
SourceApplication::StopApplication()
{
    NS_LOG_FUNCTION(this);
    DoStopApplication();
    CancelEvents();
    CloseSocket();
}

bool
SourceApplication::CloseSocket()
{
    m_connected = false;
    if (m_socket)
    {
        const auto ret = m_socket->Close();
        m_socket->SetConnectCallback(MakeNullCallback<void, Ptr<Socket>>(),
                                     MakeNullCallback<void, Ptr<Socket>>());
        m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
        return (ret == 0);
    }
    return true;
}

void
SourceApplication::ConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = true;
    DoConnectionSucceeded(socket);
}

void
SourceApplication::ConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    m_connected = false;
    DoConnectionFailed(socket);
}

void
SourceApplication::DoStartApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SourceApplication::DoStopApplication()
{
    NS_LOG_FUNCTION(this);
}

void
SourceApplication::DoConnectionSucceeded(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

void
SourceApplication::DoConnectionFailed(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
}

} // Namespace ns3
