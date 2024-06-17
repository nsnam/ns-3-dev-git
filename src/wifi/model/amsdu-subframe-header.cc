/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "amsdu-subframe-header.h"

#include "ns3/address-utils.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(AmsduSubframeHeader);

TypeId
AmsduSubframeHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::AmsduSubframeHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<AmsduSubframeHeader>();
    return tid;
}

TypeId
AmsduSubframeHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

AmsduSubframeHeader::AmsduSubframeHeader()
    : m_length(0)
{
}

AmsduSubframeHeader::~AmsduSubframeHeader()
{
}

uint32_t
AmsduSubframeHeader::GetSerializedSize() const
{
    return (6 + 6 + 2);
}

void
AmsduSubframeHeader::Serialize(Buffer::Iterator i) const
{
    WriteTo(i, m_da);
    WriteTo(i, m_sa);
    i.WriteHtonU16(m_length);
}

uint32_t
AmsduSubframeHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    ReadFrom(i, m_da);
    ReadFrom(i, m_sa);
    m_length = i.ReadNtohU16();
    return i.GetDistanceFrom(start);
}

void
AmsduSubframeHeader::Print(std::ostream& os) const
{
    os << "DA = " << m_da << ", SA = " << m_sa << ", length = " << m_length;
}

void
AmsduSubframeHeader::SetDestinationAddr(Mac48Address to)
{
    m_da = to;
}

void
AmsduSubframeHeader::SetSourceAddr(Mac48Address from)
{
    m_sa = from;
}

void
AmsduSubframeHeader::SetLength(uint16_t length)
{
    m_length = length;
}

Mac48Address
AmsduSubframeHeader::GetDestinationAddr() const
{
    return m_da;
}

Mac48Address
AmsduSubframeHeader::GetSourceAddr() const
{
    return m_sa;
}

uint16_t
AmsduSubframeHeader::GetLength() const
{
    return m_length;
}

} // namespace ns3
