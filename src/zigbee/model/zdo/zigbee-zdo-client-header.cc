/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-zdo-client-header.h"

#include "ns3/address-utils.h"

namespace ns3
{
namespace zigbee
{

NwkAddrRequestCmd::NwkAddrRequestCmd()
    : m_tsn(0),
      m_requestType(0),
      m_startIndex(0)
{
}

NwkAddrRequestCmd::~NwkAddrRequestCmd()
{
}

void
NwkAddrRequestCmd::SetTsn(uint8_t tsn)
{
    m_tsn = tsn;
}

uint8_t
NwkAddrRequestCmd::GetTsn() const
{
    return m_tsn;
}

void
NwkAddrRequestCmd::SetIeeeAddress(Mac64Address ieeeAddress)
{
    m_ieeeAddress = ieeeAddress;
}

Mac64Address
NwkAddrRequestCmd::GetIeeeAddress()
{
    return m_ieeeAddress;
}

void
NwkAddrRequestCmd::SetRequestType(uint8_t type)
{
    m_requestType = type;
}

uint8_t
NwkAddrRequestCmd::GetRequestType() const
{
    return m_requestType;
}

void
NwkAddrRequestCmd::SetStartIndex(uint8_t index)
{
    m_startIndex = index;
}

uint8_t
NwkAddrRequestCmd::GetStartIndex() const
{
    return m_startIndex;
}

void
NwkAddrRequestCmd::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU8(m_tsn);
    WriteTo(i, m_ieeeAddress);
    i.WriteU8(m_requestType);
    i.WriteU8(m_startIndex);
}

void
NwkAddrRequestCmd::Print(std::ostream& os) const
{
    os << "NwkAddrRequestCmd: "
       << "TSN: " << static_cast<uint32_t>(m_tsn) << ", "
       << "IEEE Address: " << m_ieeeAddress << ", "
       << "Extended Response: " << m_requestType << ", "
       << "Start Index: " << static_cast<uint32_t>(m_startIndex);
}

uint32_t
NwkAddrRequestCmd::GetSerializedSize() const
{
    return 11; // TSN (1) + IEEE address (8) + request type (1) + start index (1)
}

uint32_t
NwkAddrRequestCmd::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tsn = i.ReadU8();
    ReadFrom(i, m_ieeeAddress);
    m_requestType = i.ReadU8();
    m_startIndex = i.ReadU8();
    return i.GetDistanceFrom(start);
}

TypeId
NwkAddrRequestCmd::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::NwkAddrRequestCmd")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<NwkAddrRequestCmd>();
    return tid;
}

TypeId
NwkAddrRequestCmd::GetInstanceTypeId() const
{
    return GetTypeId();
}

} // namespace zigbee
} // namespace ns3
