/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jaume.nin@cttc.es>
 */

#include "lte-rlc-tag.h"

#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(RlcTag);

RlcTag::RlcTag()
    : m_senderTimestamp(Seconds(0))
{
    // Nothing to do here
}

RlcTag::RlcTag(Time senderTimestamp)
    : m_senderTimestamp(senderTimestamp)

{
    // Nothing to do here
}

TypeId
RlcTag::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::RlcTag").SetParent<Tag>().SetGroupName("Lte").AddConstructor<RlcTag>();
    return tid;
}

TypeId
RlcTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
RlcTag::GetSerializedSize() const
{
    return sizeof(Time);
}

void
RlcTag::Serialize(TagBuffer i) const
{
    int64_t senderTimestamp = m_senderTimestamp.GetNanoSeconds();
    i.Write((const uint8_t*)&senderTimestamp, sizeof(int64_t));
}

void
RlcTag::Deserialize(TagBuffer i)
{
    int64_t senderTimestamp;
    i.Read((uint8_t*)&senderTimestamp, 8);
    m_senderTimestamp = NanoSeconds(senderTimestamp);
}

void
RlcTag::Print(std::ostream& os) const
{
    os << m_senderTimestamp;
}

} // namespace ns3
