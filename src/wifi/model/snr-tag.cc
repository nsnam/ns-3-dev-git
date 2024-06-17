/*
 * Copyright (c) 2005,2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 * Copyright (c) 2013 University of Surrey
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Konstantinos Katsaros <dinos.katsaros@gmail.com>
 */

#include "snr-tag.h"

#include "ns3/double.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(SnrTag);

TypeId
SnrTag::GetTypeId()
{
    static TypeId tid = TypeId("ns3::SnrTag")
                            .SetParent<Tag>()
                            .SetGroupName("Wifi")
                            .AddConstructor<SnrTag>()
                            .AddAttribute("Snr",
                                          "The SNR of the last packet received",
                                          DoubleValue(0.0),
                                          MakeDoubleAccessor(&SnrTag::Get),
                                          MakeDoubleChecker<double>());
    return tid;
}

TypeId
SnrTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

SnrTag::SnrTag()
    : m_snr(0)
{
}

uint32_t
SnrTag::GetSerializedSize() const
{
    return sizeof(double);
}

void
SnrTag::Serialize(TagBuffer i) const
{
    i.WriteDouble(m_snr);
}

void
SnrTag::Deserialize(TagBuffer i)
{
    m_snr = i.ReadDouble();
}

void
SnrTag::Print(std::ostream& os) const
{
    os << "Snr=" << m_snr;
}

void
SnrTag::Set(double snr)
{
    m_snr = snr;
}

double
SnrTag::Get() const
{
    return m_snr;
}

} // namespace ns3
