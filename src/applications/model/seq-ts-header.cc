/*
 * Copyright (c) 2009 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "seq-ts-header.h"

#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SeqTsHeader");

NS_OBJECT_ENSURE_REGISTERED(SeqTsHeader);

SeqTsHeader::SeqTsHeader()
    : m_seq(0),
      m_ts(Simulator::Now().GetTimeStep())
{
    NS_LOG_FUNCTION(this);
}

void
SeqTsHeader::SetSeq(uint32_t seq)
{
    NS_LOG_FUNCTION(this << seq);
    m_seq = seq;
}

uint32_t
SeqTsHeader::GetSeq() const
{
    NS_LOG_FUNCTION(this);
    return m_seq;
}

Time
SeqTsHeader::GetTs() const
{
    NS_LOG_FUNCTION(this);
    return TimeStep(m_ts);
}

TypeId
SeqTsHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SeqTsHeader")
                            .SetParent<Header>()
                            .SetGroupName("Applications")
                            .AddConstructor<SeqTsHeader>();
    return tid;
}

TypeId
SeqTsHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SeqTsHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(seq=" << m_seq << " time=" << TimeStep(m_ts).As(Time::S) << ")";
}

uint32_t
SeqTsHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 4 + 8;
}

void
SeqTsHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU32(m_seq);
    i.WriteHtonU64(m_ts);
}

uint32_t
SeqTsHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_seq = i.ReadNtohU32();
    m_ts = i.ReadNtohU64();
    return GetSerializedSize();
}

} // namespace ns3
