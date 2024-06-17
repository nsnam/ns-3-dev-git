/*
 * Copyright (c) 2009 INRIA
 * Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
 *                    (added timestamp and size fields)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "seq-ts-size-header.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SeqTsSizeHeader");

NS_OBJECT_ENSURE_REGISTERED(SeqTsSizeHeader);

SeqTsSizeHeader::SeqTsSizeHeader()
    : SeqTsHeader()
{
    NS_LOG_FUNCTION(this);
}

TypeId
SeqTsSizeHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SeqTsSizeHeader")
                            .SetParent<SeqTsHeader>()
                            .SetGroupName("Applications")
                            .AddConstructor<SeqTsSizeHeader>();
    return tid;
}

TypeId
SeqTsSizeHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SeqTsSizeHeader::SetSize(uint64_t size)
{
    m_size = size;
}

uint64_t
SeqTsSizeHeader::GetSize() const
{
    return m_size;
}

void
SeqTsSizeHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(size=" << m_size << ") AND ";
    SeqTsHeader::Print(os);
}

uint32_t
SeqTsSizeHeader::GetSerializedSize() const
{
    return SeqTsHeader::GetSerializedSize() + 8;
}

void
SeqTsSizeHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU64(m_size);
    SeqTsHeader::Serialize(i);
}

uint32_t
SeqTsSizeHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_size = i.ReadNtohU64();
    SeqTsHeader::Deserialize(i);
    return GetSerializedSize();
}

} // namespace ns3
