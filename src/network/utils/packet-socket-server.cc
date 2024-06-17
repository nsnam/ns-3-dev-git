/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "packet-socket-server.h"

#include "packet-socket-address.h"
#include "packet-socket-factory.h"
#include "packet-socket.h"

#include "ns3/abort.h"
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

NS_LOG_COMPONENT_DEFINE("PacketSocketServer");

NS_OBJECT_ENSURE_REGISTERED(PacketSocketServer);

TypeId
PacketSocketServer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PacketSocketServer")
                            .SetParent<Application>()
                            .SetGroupName("Network")
                            .AddConstructor<PacketSocketServer>()
                            .AddTraceSource("Rx",
                                            "A packet has been received",
                                            MakeTraceSourceAccessor(&PacketSocketServer::m_rxTrace),
                                            "ns3::Packet::AddressTracedCallback");
    return tid;
}

PacketSocketServer::PacketSocketServer()
{
    NS_LOG_FUNCTION(this);
    m_pktRx = 0;
    m_bytesRx = 0;
    m_socket = nullptr;
    m_localAddressSet = false;
}

PacketSocketServer::~PacketSocketServer()
{
    NS_LOG_FUNCTION(this);
}

void
PacketSocketServer::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Application::DoDispose();
}

void
PacketSocketServer::StartApplication()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT_MSG(m_localAddressSet, "Local address not set");

    if (!m_socket)
    {
        TypeId tid = TypeId::LookupByName("ns3::PacketSocketFactory");
        m_socket = Socket::CreateSocket(GetNode(), tid);
        m_socket->Bind(m_localAddress);
    }

    m_socket->SetRecvCallback(MakeCallback(&PacketSocketServer::HandleRead, this));
}

void
PacketSocketServer::StopApplication()
{
    NS_LOG_FUNCTION(this);
    m_socket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    m_socket->Close();
}

void
PacketSocketServer::SetLocal(PacketSocketAddress addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_localAddress = addr;
    m_localAddressSet = true;
}

void
PacketSocketServer::HandleRead(Ptr<Socket> socket)
{
    NS_LOG_FUNCTION(this << socket);
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        if (PacketSocketAddress::IsMatchingType(from))
        {
            m_pktRx++;
            m_bytesRx += packet->GetSize();
            NS_LOG_INFO("At time " << Simulator::Now().As(Time::S) << " packet sink received "
                                   << packet->GetSize() << " bytes from "
                                   << PacketSocketAddress::ConvertFrom(from) << " total Rx "
                                   << m_pktRx << " packets"
                                   << " and " << m_bytesRx << " bytes");
            m_rxTrace(packet, from);
        }
    }
}

} // Namespace ns3
