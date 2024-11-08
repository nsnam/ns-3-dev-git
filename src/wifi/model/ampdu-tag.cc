/*
 * Copyright (c) 2013
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Ghada Badawy <gbadawy@gmail.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ampdu-tag.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(AmpduTag);

TypeId
AmpduTag::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AmpduTag").SetParent<Tag>().SetGroupName("Wifi").AddConstructor<AmpduTag>();
    return tid;
}

TypeId
AmpduTag::GetInstanceTypeId() const
{
    return GetTypeId();
}

AmpduTag::AmpduTag()
    : m_nbOfMpdus(0),
      m_duration()
{
}

void
AmpduTag::SetRemainingNbOfMpdus(uint8_t nbOfMpdus)
{
    m_nbOfMpdus = nbOfMpdus;
}

void
AmpduTag::SetRemainingAmpduDuration(Time duration)
{
    NS_ASSERT(m_duration <= MilliSeconds(10));
    m_duration = duration;
}

uint32_t
AmpduTag::GetSerializedSize() const
{
    return (1 + sizeof(Time));
}

void
AmpduTag::Serialize(TagBuffer i) const
{
    i.WriteU8(m_nbOfMpdus);
    int64_t duration = m_duration.GetTimeStep();
    i.Write((const uint8_t*)&duration, sizeof(int64_t));
}

void
AmpduTag::Deserialize(TagBuffer i)
{
    m_nbOfMpdus = i.ReadU8();
    int64_t duration;
    i.Read((uint8_t*)&duration, sizeof(int64_t));
    m_duration = Time(duration);
}

uint8_t
AmpduTag::GetRemainingNbOfMpdus() const
{
    return m_nbOfMpdus;
}

Time
AmpduTag::GetRemainingAmpduDuration() const
{
    return m_duration;
}

void
AmpduTag::Print(std::ostream& os) const
{
    os << "Remaining number of MPDUs=" << m_nbOfMpdus
       << " Remaining A-MPDU duration=" << m_duration;
}

} // namespace ns3
