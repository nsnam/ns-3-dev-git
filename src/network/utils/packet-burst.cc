/*
 * Copyright (c) 2007,2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 */

#include "packet-burst.h"

#include "ns3/log.h"
#include "ns3/packet.h"

#include <list>
#include <stdint.h>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PacketBurst");

NS_OBJECT_ENSURE_REGISTERED(PacketBurst);

TypeId
PacketBurst::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PacketBurst")
                            .SetParent<Object>()
                            .SetGroupName("Network")
                            .AddConstructor<PacketBurst>();
    return tid;
}

PacketBurst::PacketBurst()
{
    NS_LOG_FUNCTION(this);
}

PacketBurst::~PacketBurst()
{
    NS_LOG_FUNCTION(this);
    for (auto iter = m_packets.begin(); iter != m_packets.end(); ++iter)
    {
        (*iter)->Unref();
    }
}

void
PacketBurst::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_packets.clear();
}

Ptr<PacketBurst>
PacketBurst::Copy() const
{
    NS_LOG_FUNCTION(this);
    Ptr<PacketBurst> burst = Create<PacketBurst>();

    for (auto iter = m_packets.begin(); iter != m_packets.end(); ++iter)
    {
        Ptr<Packet> packet = (*iter)->Copy();
        burst->AddPacket(packet);
    }
    return burst;
}

void
PacketBurst::AddPacket(Ptr<Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    if (packet)
    {
        m_packets.push_back(packet);
    }
}

std::list<Ptr<Packet>>
PacketBurst::GetPackets() const
{
    NS_LOG_FUNCTION(this);
    return m_packets;
}

uint32_t
PacketBurst::GetNPackets() const
{
    NS_LOG_FUNCTION(this);
    return m_packets.size();
}

uint32_t
PacketBurst::GetSize() const
{
    NS_LOG_FUNCTION(this);
    uint32_t size = 0;
    for (auto iter = m_packets.begin(); iter != m_packets.end(); ++iter)
    {
        Ptr<Packet> packet = *iter;
        size += packet->GetSize();
    }
    return size;
}

std::list<Ptr<Packet>>::const_iterator
PacketBurst::Begin() const
{
    NS_LOG_FUNCTION(this);
    return m_packets.begin();
}

std::list<Ptr<Packet>>::const_iterator
PacketBurst::End() const
{
    NS_LOG_FUNCTION(this);
    return m_packets.end();
}

} // namespace ns3
