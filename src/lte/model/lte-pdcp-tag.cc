/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jaume Nin <jaume.nin@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "lte-pdcp-tag.h"

#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(PdcpTag);

PdcpTag::PdcpTag()
    : m_senderTimestamp(Seconds(0))
{
    // Nothing to do here
}

PdcpTag::PdcpTag(Time senderTimestamp)
    : m_senderTimestamp(senderTimestamp)

{
    // Nothing to do here
}

TypeId
PdcpTag::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::PdcpTag").SetParent<Tag>().SetGroupName("Lte").AddConstructor<PdcpTag>();
    return tid;
}

TypeId
PdcpTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
PdcpTag::GetSerializedSize() const
{
    return sizeof(Time);
}

void
PdcpTag::Serialize(TagBuffer i) const
{
    int64_t senderTimestamp = m_senderTimestamp.GetNanoSeconds();
    i.Write((const uint8_t*)&senderTimestamp, sizeof(int64_t));
}

void
PdcpTag::Deserialize(TagBuffer i)
{
    int64_t senderTimestamp;
    i.Read((uint8_t*)&senderTimestamp, 8);
    m_senderTimestamp = NanoSeconds(senderTimestamp);
}

void
PdcpTag::Print(std::ostream& os) const
{
    os << m_senderTimestamp;
}

Time
PdcpTag::GetSenderTimestamp() const
{
    return m_senderTimestamp;
}

void
PdcpTag::SetSenderTimestamp(Time senderTimestamp)
{
    this->m_senderTimestamp = senderTimestamp;
}

} // namespace ns3
