/*
 * Copyright (c) 2013 Fraunhofer FKIE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 */
#include "lr-wpan-lqi-tag.h"

#include "ns3/integer.h"

namespace ns3
{
namespace lrwpan
{

NS_OBJECT_ENSURE_REGISTERED(LrWpanLqiTag);

TypeId
LrWpanLqiTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::lrwpan::LrWpanLqiTag")
                            .AddDeprecatedName("ns3::LrWpanLqiTag")
                            .SetParent<Tag>()
                            .SetGroupName("LrWpan")
                            .AddConstructor<LrWpanLqiTag>()
                            .AddAttribute("Lqi",
                                          "The lqi of the last packet received",
                                          IntegerValue(0),
                                          MakeIntegerAccessor(&LrWpanLqiTag::Get),
                                          MakeIntegerChecker<uint8_t>());
    return tid;
}

TypeId
LrWpanLqiTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

LrWpanLqiTag::LrWpanLqiTag()
    : m_lqi(0)
{
}

LrWpanLqiTag::LrWpanLqiTag(uint8_t lqi)
    : m_lqi(lqi)
{
}

uint32_t
LrWpanLqiTag::GetSerializedSize() const
{
    return sizeof(uint8_t);
}

void
LrWpanLqiTag::Serialize(TagBuffer i) const
{
    i.WriteU8(m_lqi);
}

void
LrWpanLqiTag::Deserialize(TagBuffer i)
{
    m_lqi = i.ReadU8();
}

void
LrWpanLqiTag::Print(std::ostream& os) const
{
    os << "Lqi = " << m_lqi;
}

void
LrWpanLqiTag::Set(uint8_t lqi)
{
    m_lqi = lqi;
}

uint8_t
LrWpanLqiTag::Get() const
{
    return m_lqi;
}

} // namespace lrwpan
} // namespace ns3
