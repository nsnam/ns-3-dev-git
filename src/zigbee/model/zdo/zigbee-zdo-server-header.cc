/*
 * Copyright (c) 2026 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-zdo-server-header.h"

#include "ns3/address-utils.h"

namespace ns3
{
namespace zigbee
{

NwkAddrResponseCmd::NwkAddrResponseCmd()
    : m_tsn(0),
      m_status(0)
{
}

NwkAddrResponseCmd::~NwkAddrResponseCmd()
{
}

void
NwkAddrResponseCmd::SetTsn(uint8_t tsn)
{
    m_tsn = tsn;
}

uint8_t
NwkAddrResponseCmd::GetTsn() const
{
    return m_tsn;
}

void
NwkAddrResponseCmd::SetStatus(uint8_t status)
{
    m_status = status;
}

uint8_t
NwkAddrResponseCmd::GetStatus() const
{
    return m_status;
}

void
NwkAddrResponseCmd::SetIeeeAddrRemoteDev(Mac64Address ieeeAddrRemoteDev)
{
    m_ieeeAddrRemoteDev = ieeeAddrRemoteDev;
}

Mac64Address
NwkAddrResponseCmd::GetIeeeAddrRemoteDev()
{
    return m_ieeeAddrRemoteDev;
}

void
NwkAddrResponseCmd::SetNwkAddrRemoteDev(Mac16Address nwkAddrRemoteDev)
{
    m_nwkAddrRemoteDev = nwkAddrRemoteDev;
}

Mac16Address
NwkAddrResponseCmd::GetNwkAddrRemoteDev()
{
    return m_nwkAddrRemoteDev;
}

void
NwkAddrResponseCmd::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU8(m_tsn);
    i.WriteU8(m_status);
    WriteTo(i, m_ieeeAddrRemoteDev);
    WriteTo(i, m_nwkAddrRemoteDev);
}

void
NwkAddrResponseCmd::Print(std::ostream& os) const
{
    os << "NwkAddrResponseCmd: "
       << "TSN: " << static_cast<uint32_t>(m_tsn) << ", "
       << "Status: " << static_cast<uint32_t>(m_status) << ", "
       << "IEEE Address Remote Device: " << m_ieeeAddrRemoteDev << ", "
       << "NWK Address Remote Device: " << m_nwkAddrRemoteDev;
}

uint32_t
NwkAddrResponseCmd::GetSerializedSize() const
{
    return 12; // TSN (1) + Status (1) + IEEE address (8) + NWK address (2)
}

uint32_t
NwkAddrResponseCmd::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_tsn = i.ReadU8();
    m_status = i.ReadU8();
    ReadFrom(i, m_ieeeAddrRemoteDev);
    ReadFrom(i, m_nwkAddrRemoteDev);
    return i.GetDistanceFrom(start);
}

TypeId
NwkAddrResponseCmd::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::NwkAddrResponseCmd")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<NwkAddrResponseCmd>();
    return tid;
}

TypeId
NwkAddrResponseCmd::GetInstanceTypeId() const
{
    return GetTypeId();
}

} // namespace zigbee
} // namespace ns3
