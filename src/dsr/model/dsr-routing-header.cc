/*
 * Copyright (c) 2011 Yufei Cheng
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Yufei Cheng   <yfcheng@ittc.ku.edu>
 * Modified by: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *              Lorenzo Bartolini <l.bartolini02@gmail.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#include "dsr-routing-header.h"

#include "dsr-options.h"
#include "dsr-routing.h"

#include "ns3/assert.h"
#include "ns3/header.h"
#include "ns3/log.h"

#include <vector>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DsrRoutingHeader");

namespace dsr
{

NS_OBJECT_ENSURE_REGISTERED(DsrRoutingHeader);

TypeId
DsrRoutingHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DsrRoutingHeader")
                            .AddConstructor<DsrRoutingHeader>()
                            .SetParent<Header>()
                            .SetGroupName("Dsr");
    return tid;
}

TypeId
DsrRoutingHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

DsrRoutingHeader::DsrRoutingHeader()
    : m_optionData(0),
      m_nextHeader(DsrRouting::NO_NEXT_HEADER)
{
}

DsrRoutingHeader::~DsrRoutingHeader()
{
}

void
DsrRoutingHeader::Print(std::ostream& os) const
{
    os << " nextHeader: " << (uint32_t)GetNextHeader()
       << " length: " << (uint32_t)GetPayloadLength();
}

void
DsrRoutingHeader::SetNextHeader(uint8_t protocol)
{
    m_nextHeader = protocol;
}

uint8_t
DsrRoutingHeader::GetNextHeader() const
{
    return m_nextHeader;
}

uint32_t
DsrRoutingHeader::GetSerializedSize() const
{
    // 4 bytes is the DsrFsHeader length: 1 byte for next header, 1 byte reserved, 2 bytes for
    // payload length

    DsrOptionHeader::Alignment align = {4, 0};
    return 4 + m_optionData.GetSize() + CalculatePad(align);
}

void
DsrRoutingHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteU8(m_nextHeader);
    i.WriteU8(0); // reserved field
    i.WriteHtonU16(GetPayloadLength());

    SerializeOptions(i);
}

uint32_t
DsrRoutingHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    SetNextHeader(i.ReadU8());
    i.ReadU8(); // reserved field
    uint16_t payloadLength = i.ReadNtohU16();

    DeserializeOptions(i, payloadLength);

    return GetSerializedSize();
}

uint16_t
DsrRoutingHeader::GetPayloadLength() const
{
    // Return the actual size of the options field including padding
    // This is the correct value for the Payload Length field per RFC 4728
    return static_cast<uint16_t>(GetSerializedSize() - 4);
}

void
DsrRoutingHeader::SerializeOptions(Buffer::Iterator start) const
{
    start.Write(m_optionData.Begin(), m_optionData.End());
    DsrOptionHeader::Alignment align = {4, 0};
    uint32_t fill = CalculatePad(align);
    NS_LOG_LOGIC("fill with " << fill << " bytes padding");
    switch (fill)
    {
    case 0:
        return;
    case 1:
        DsrOptionPad1Header().Serialize(start);
        return;
    default:
        DsrOptionPadnHeader(fill).Serialize(start);
        return;
    }
}

uint32_t
DsrRoutingHeader::DeserializeOptions(Buffer::Iterator start, uint32_t length)
{
    std::vector<uint8_t> buf(length);
    start.Read(buf.data(), length);
    m_optionData = Buffer();
    m_optionData.AddAtEnd(length);
    m_optionData.Begin().Write(buf.data(), buf.size());
    return length;
}

void
DsrRoutingHeader::AddDsrOption(const DsrOptionHeader& option)
{
    NS_LOG_FUNCTION_NOARGS();

    uint32_t pad = CalculatePad(option.GetAlignment());
    NS_LOG_LOGIC("need " << pad << " bytes padding");
    switch (pad)
    {
    case 0:
        break; // no padding needed
    case 1:
        AddDsrOption(DsrOptionPad1Header());
        break;
    default:
        AddDsrOption(DsrOptionPadnHeader(pad));
        break;
    }

    m_optionData.AddAtEnd(option.GetSerializedSize());
    Buffer::Iterator it = m_optionData.End();
    it.Prev(option.GetSerializedSize());
    option.Serialize(it);
}

uint32_t
DsrRoutingHeader::GetDsrOptionsOffset() const
{
    return 4;
}

uint32_t
DsrRoutingHeader::CalculatePad(DsrOptionHeader::Alignment alignment) const
{
    return (alignment.offset - (m_optionData.GetSize() + 4)) % alignment.factor;
}

} /* namespace dsr */
} /* namespace ns3 */
