/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#include "hwmp-tag.h"

namespace ns3
{
namespace dot11s
{

NS_OBJECT_ENSURE_REGISTERED(HwmpTag);

// Class HwmpTag:
HwmpTag::HwmpTag()
    : m_address(Mac48Address::GetBroadcast()),
      m_ttl(0),
      m_metric(0),
      m_seqno(0)
{
}

HwmpTag::~HwmpTag()
{
}

void
HwmpTag::SetAddress(Mac48Address retransmitter)
{
    m_address = retransmitter;
}

Mac48Address
HwmpTag::GetAddress()
{
    return m_address;
}

void
HwmpTag::SetTtl(uint8_t ttl)
{
    m_ttl = ttl;
}

uint8_t
HwmpTag::GetTtl() const
{
    return m_ttl;
}

void
HwmpTag::SetMetric(uint32_t metric)
{
    m_metric = metric;
}

uint32_t
HwmpTag::GetMetric() const
{
    return m_metric;
}

void
HwmpTag::SetSeqno(uint32_t seqno)
{
    m_seqno = seqno;
}

uint32_t
HwmpTag::GetSeqno() const
{
    return m_seqno;
}

/**
 * @brief Get the type ID.
 * @return the object TypeId
 */
TypeId
HwmpTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::dot11s::HwmpTag")
                            .SetParent<Tag>()
                            .SetGroupName("Mesh")
                            .AddConstructor<HwmpTag>();
    return tid;
}

TypeId
HwmpTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
HwmpTag::GetSerializedSize() const
{
    return 6    // address
           + 1  // ttl
           + 4  // metric
           + 4; // seqno
}

void
HwmpTag::Serialize(TagBuffer i) const
{
    uint8_t address[6];
    int j;
    m_address.CopyTo(address);
    i.WriteU8(m_ttl);
    i.WriteU32(m_metric);
    i.WriteU32(m_seqno);
    for (j = 0; j < 6; j++)
    {
        i.WriteU8(address[j]);
    }
}

void
HwmpTag::Deserialize(TagBuffer i)
{
    uint8_t address[6];
    int j;
    m_ttl = i.ReadU8();
    m_metric = i.ReadU32();
    m_seqno = i.ReadU32();
    for (j = 0; j < 6; j++)
    {
        address[j] = i.ReadU8();
    }
    m_address.CopyFrom(address);
}

void
HwmpTag::Print(std::ostream& os) const
{
    os << "address=" << m_address;
    os << "ttl=" << m_ttl;
    os << "metrc=" << m_metric;
    os << "seqno=" << m_seqno;
}

void
HwmpTag::DecrementTtl()
{
    m_ttl--;
}
} // namespace dot11s
} // namespace ns3
