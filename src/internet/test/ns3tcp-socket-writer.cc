/*
 * Copyright (c) 2010 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3tcp-socket-writer.h"

#include "ns3/packet.h"
#include "ns3/tcp-socket-factory.h"

namespace ns3
{

SocketWriter::SocketWriter()
    : m_node(nullptr),
      m_socket(nullptr),
      m_isSetup(false),
      m_isConnected(false)
{
}

SocketWriter::~SocketWriter()
{
    m_socket = nullptr;
    m_node = nullptr;
}

/* static */
TypeId
SocketWriter::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SocketWriter")
                            .SetParent<Application>()
                            .SetGroupName("Stats")
                            .AddConstructor<SocketWriter>();
    return tid;
}

void
SocketWriter::StartApplication()
{
    m_socket = Socket::CreateSocket(m_node, TcpSocketFactory::GetTypeId());
    m_socket->Bind();
}

void
SocketWriter::StopApplication()
{
}

void
SocketWriter::Setup(Ptr<Node> node, Address peer)
{
    m_peer = peer;
    m_node = node;
    m_isSetup = true;
}

void
SocketWriter::Connect()
{
    if (!m_isSetup)
    {
        NS_FATAL_ERROR("Forgot to call Setup() first");
    }
    m_socket->Connect(m_peer);
    m_isConnected = true;
}

void
SocketWriter::Write(uint32_t numBytes)
{
    if (!m_isConnected)
    {
        Connect();
    }
    Ptr<Packet> packet = Create<Packet>(numBytes);
    m_socket->Send(packet);
}

void
SocketWriter::Close()
{
    m_socket->Close();
}
} // namespace ns3
