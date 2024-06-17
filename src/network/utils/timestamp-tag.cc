/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Joe Kopena <tjkopena@cs.drexel.edu>
 */

#include "timestamp-tag.h"

#include "ns3/nstime.h"
#include "ns3/tag-buffer.h"
#include "ns3/tag.h"
#include "ns3/type-id.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(TimestampTag);

TimestampTag::TimestampTag() = default;

TimestampTag::TimestampTag(Time timestamp)
    : m_timestamp(timestamp)
{
}

TypeId
TimestampTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TimestampTag")
                            .SetParent<Tag>()
                            .SetGroupName("Network")
                            .AddConstructor<TimestampTag>();
    return tid;
}

TypeId
TimestampTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
TimestampTag::GetSerializedSize() const
{
    return 8;
}

void
TimestampTag::Serialize(TagBuffer i) const
{
    i.WriteU64(m_timestamp.GetTimeStep());
}

void
TimestampTag::Deserialize(TagBuffer i)
{
    m_timestamp = TimeStep(i.ReadU64());
}

void
TimestampTag::Print(std::ostream& os) const
{
    os << "timestamp=" << m_timestamp.As(Time::S);
}

Time
TimestampTag::GetTimestamp() const
{
    return m_timestamp;
}

void
TimestampTag::SetTimestamp(Time timestamp)
{
    m_timestamp = timestamp;
}

} // namespace ns3
