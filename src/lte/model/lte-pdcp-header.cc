/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "lte-pdcp-header.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LtePdcpHeader");

NS_OBJECT_ENSURE_REGISTERED(LtePdcpHeader);

LtePdcpHeader::LtePdcpHeader()
    : m_dcBit(0xff),
      m_sequenceNumber(0xfffa)
{
}

LtePdcpHeader::~LtePdcpHeader()
{
    m_dcBit = 0xff;
    m_sequenceNumber = 0xfffb;
}

void
LtePdcpHeader::SetDcBit(uint8_t dcBit)
{
    m_dcBit = dcBit & 0x01;
}

void
LtePdcpHeader::SetSequenceNumber(uint16_t sequenceNumber)
{
    m_sequenceNumber = sequenceNumber & 0x0FFF;
}

uint8_t
LtePdcpHeader::GetDcBit() const
{
    return m_dcBit;
}

uint16_t
LtePdcpHeader::GetSequenceNumber() const
{
    return m_sequenceNumber;
}

TypeId
LtePdcpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::LtePdcpHeader")
                            .SetParent<Header>()
                            .SetGroupName("Lte")
                            .AddConstructor<LtePdcpHeader>();
    return tid;
}

TypeId
LtePdcpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
LtePdcpHeader::Print(std::ostream& os) const
{
    os << "D/C=" << (uint16_t)m_dcBit;
    os << " SN=" << m_sequenceNumber;
}

uint32_t
LtePdcpHeader::GetSerializedSize() const
{
    return 2;
}

void
LtePdcpHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU8((m_dcBit << 7) | (m_sequenceNumber & 0x0F00) >> 8);
    i.WriteU8(m_sequenceNumber & 0x00FF);
}

uint32_t
LtePdcpHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint8_t byte_1;
    uint8_t byte_2;

    byte_1 = i.ReadU8();
    byte_2 = i.ReadU8();
    m_dcBit = (byte_1 & 0x80) > 7;
    // For now, we just support DATA PDUs
    NS_ASSERT(m_dcBit == DATA_PDU);
    m_sequenceNumber = ((byte_1 & 0x0F) << 8) | byte_2;

    return GetSerializedSize();
}

}; // namespace ns3
