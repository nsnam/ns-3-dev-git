/*
 * Copyright (c) 2009 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "aloha-noack-mac-header.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AlohaNoackMacHeader");

NS_OBJECT_ENSURE_REGISTERED(AlohaNoackMacHeader);

TypeId
AlohaNoackMacHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::AlohaNoackMacHeader")
                            .SetParent<Header>()
                            .SetGroupName("Spectrum")
                            .AddConstructor<AlohaNoackMacHeader>();
    return tid;
}

TypeId
AlohaNoackMacHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
AlohaNoackMacHeader::GetSerializedSize() const
{
    return 12;
}

void
AlohaNoackMacHeader::Serialize(Buffer::Iterator start) const
{
    WriteTo(start, m_destination);
    WriteTo(start, m_source);
}

uint32_t
AlohaNoackMacHeader::Deserialize(Buffer::Iterator start)
{
    ReadFrom(start, m_destination);
    ReadFrom(start, m_source);
    return GetSerializedSize();
}

void
AlohaNoackMacHeader::Print(std::ostream& os) const
{
    os << "src=" << m_source << "dst=" << m_destination;
}

void
AlohaNoackMacHeader::SetSource(Mac48Address source)
{
    m_source = source;
}

Mac48Address
AlohaNoackMacHeader::GetSource() const
{
    return m_source;
}

void
AlohaNoackMacHeader::SetDestination(Mac48Address dst)
{
    m_destination = dst;
}

Mac48Address
AlohaNoackMacHeader::GetDestination() const
{
    return m_destination;
}

} // namespace ns3
