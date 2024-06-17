/*
 * Copyright (c) 2015 Université Pierre et Marie Curie
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Matthieu Coudron <matthieu.coudron@lip6.fr>
 */
#include "sll-header.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SllHeader");

NS_OBJECT_ENSURE_REGISTERED(SllHeader);

SllHeader::SllHeader()
    : m_packetType(UNICAST_FROM_PEER_TO_ME),
      m_arphdType(0),
      m_addressLength(0),
      m_address(0),
      m_protocolType(0)
{
    NS_LOG_FUNCTION(this);
}

SllHeader::~SllHeader()
{
    NS_LOG_FUNCTION(this);
}

TypeId
SllHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SllHeader")
                            .SetParent<Header>()
                            .SetGroupName("Network")
                            .AddConstructor<SllHeader>();
    return tid;
}

TypeId
SllHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint16_t
SllHeader::GetArpType() const
{
    return m_arphdType;
}

void
SllHeader::SetArpType(uint16_t arphdType)
{
    NS_LOG_FUNCTION(arphdType);
    m_arphdType = arphdType;
}

SllHeader::PacketType
SllHeader::GetPacketType() const
{
    return m_packetType;
}

void
SllHeader::SetPacketType(PacketType type)
{
    NS_LOG_FUNCTION(type);
    m_packetType = type;
}

void
SllHeader::Print(std::ostream& os) const
{
    os << "SLLHeader packetType=" << m_packetType << " protocol=" << m_protocolType;
}

uint32_t
SllHeader::GetSerializedSize() const
{
    return 2 + 2 + 2 + 8 + 2;
}

void
SllHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU16(m_packetType);
    i.WriteHtonU16(m_arphdType);
    i.WriteHtonU16(m_addressLength);
    i.WriteHtonU64(m_address);
    i.WriteHtonU16(m_protocolType);
}

uint32_t
SllHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_packetType = static_cast<PacketType>(i.ReadNtohU16());
    m_arphdType = i.ReadNtohU16();
    m_addressLength = i.ReadNtohU16();
    m_address = i.ReadNtohU64();
    m_protocolType = i.ReadNtohU16();

    return GetSerializedSize();
}

} // namespace ns3
