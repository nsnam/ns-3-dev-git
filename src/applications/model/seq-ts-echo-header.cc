/*
 * Copyright (c) 2009 INRIA
 * Copyright (c) 2016 Universita' di Firenze (added echo fields)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "seq-ts-echo-header.h"

#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SeqTsEchoHeader");

NS_OBJECT_ENSURE_REGISTERED(SeqTsEchoHeader);

SeqTsEchoHeader::SeqTsEchoHeader()
    : m_seq(0),
      m_tsValue(Simulator::Now()),
      m_tsEchoReply()
{
    NS_LOG_FUNCTION(this);
}

void
SeqTsEchoHeader::SetSeq(uint32_t seq)
{
    NS_LOG_FUNCTION(this << seq);
    m_seq = seq;
}

uint32_t
SeqTsEchoHeader::GetSeq() const
{
    NS_LOG_FUNCTION(this);
    return m_seq;
}

void
SeqTsEchoHeader::SetTsValue(Time ts)
{
    NS_LOG_FUNCTION(this << ts);
    m_tsValue = ts;
}

Time
SeqTsEchoHeader::GetTsValue() const
{
    NS_LOG_FUNCTION(this);
    return m_tsValue;
}

void
SeqTsEchoHeader::SetTsEchoReply(Time ts)
{
    NS_LOG_FUNCTION(this << ts);
    m_tsEchoReply = ts;
}

Time
SeqTsEchoHeader::GetTsEchoReply() const
{
    NS_LOG_FUNCTION(this);
    return m_tsEchoReply;
}

TypeId
SeqTsEchoHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SeqTsEchoHeader")
                            .SetParent<Header>()
                            .SetGroupName("Applications")
                            .AddConstructor<SeqTsEchoHeader>();
    return tid;
}

TypeId
SeqTsEchoHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
SeqTsEchoHeader::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "(seq=" << m_seq << " Tx time=" << m_tsValue.As(Time::S)
       << " Rx time=" << m_tsEchoReply.As(Time::S) << ")";
}

uint32_t
SeqTsEchoHeader::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 4 + 8 + 8;
}

void
SeqTsEchoHeader::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    i.WriteHtonU32(m_seq);
    i.WriteHtonU64(m_tsValue.GetTimeStep());
    i.WriteHtonU64(m_tsEchoReply.GetTimeStep());
}

uint32_t
SeqTsEchoHeader::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    m_seq = i.ReadNtohU32();
    m_tsValue = TimeStep(i.ReadNtohU64());
    m_tsEchoReply = TimeStep(i.ReadNtohU64());
    return GetSerializedSize();
}

} // namespace ns3
