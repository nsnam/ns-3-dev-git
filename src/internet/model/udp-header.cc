/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "udp-header.h"

#include "ns3/address-utils.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(UdpHeader);

void
UdpHeader::EnableChecksums()
{
    m_calcChecksum = true;
}

void
UdpHeader::SetDestinationPort(uint16_t port)
{
    m_destinationPort = port;
}

void
UdpHeader::SetSourcePort(uint16_t port)
{
    m_sourcePort = port;
}

uint16_t
UdpHeader::GetSourcePort() const
{
    return m_sourcePort;
}

uint16_t
UdpHeader::GetDestinationPort() const
{
    return m_destinationPort;
}

void
UdpHeader::InitializeChecksum(Address source, Address destination, uint8_t protocol)
{
    m_source = source;
    m_destination = destination;
    m_protocol = protocol;
}

void
UdpHeader::InitializeChecksum(Ipv4Address source, Ipv4Address destination, uint8_t protocol)
{
    m_source = source;
    m_destination = destination;
    m_protocol = protocol;
}

void
UdpHeader::InitializeChecksum(Ipv6Address source, Ipv6Address destination, uint8_t protocol)
{
    m_source = source;
    m_destination = destination;
    m_protocol = protocol;
}

uint16_t
UdpHeader::CalculateHeaderChecksum(uint16_t size) const
{
    Buffer buf = Buffer((2 * Address::MAX_SIZE) + 8);
    buf.AddAtStart((2 * Address::MAX_SIZE) + 8);
    Buffer::Iterator it = buf.Begin();
    uint32_t hdrSize = 0;

    WriteTo(it, m_source);
    WriteTo(it, m_destination);
    if (Ipv4Address::IsMatchingType(m_source))
    {
        it.WriteU8(0);           /* protocol */
        it.WriteU8(m_protocol);  /* protocol */
        it.WriteU8(size >> 8);   /* length */
        it.WriteU8(size & 0xff); /* length */
        hdrSize = 12;
    }
    else if (Ipv6Address::IsMatchingType(m_source))
    {
        it.WriteU16(0);
        it.WriteU8(size >> 8);   /* length */
        it.WriteU8(size & 0xff); /* length */
        it.WriteU16(0);
        it.WriteU8(0);
        it.WriteU8(m_protocol); /* protocol */
        hdrSize = 40;
    }

    it = buf.Begin();
    /* we don't CompleteChecksum ( ~ ) now */
    return ~(it.CalculateIpChecksum(hdrSize));
}

bool
UdpHeader::IsChecksumOk() const
{
    return m_goodChecksum;
}

void
UdpHeader::ForceChecksum(uint16_t checksum)
{
    m_checksum = checksum;
}

void
UdpHeader::ForcePayloadSize(uint16_t payloadSize)
{
    m_forcedPayloadSize = payloadSize;
}

TypeId
UdpHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UdpHeader")
                            .SetParent<Header>()
                            .SetGroupName("Internet")
                            .AddConstructor<UdpHeader>();
    return tid;
}

TypeId
UdpHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
UdpHeader::Print(std::ostream& os) const
{
    os << "length: " << m_payloadSize + GetSerializedSize() << " " << m_sourcePort << " > "
       << m_destinationPort;
}

uint32_t
UdpHeader::GetSerializedSize() const
{
    return 8;
}

void
UdpHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteHtonU16(m_sourcePort);
    i.WriteHtonU16(m_destinationPort);
    if (m_forcedPayloadSize == 0)
    {
        i.WriteHtonU16(start.GetSize());
    }
    else
    {
        i.WriteHtonU16(m_forcedPayloadSize);
    }

    if (m_checksum == 0)
    {
        i.WriteU16(0);

        if (m_calcChecksum)
        {
            uint16_t headerChecksum = CalculateHeaderChecksum(start.GetSize());
            i = start;
            uint16_t checksum = i.CalculateIpChecksum(start.GetSize(), headerChecksum);

            i = start;
            i.Next(6);

            // RFC 768: If the computed checksum is zero, it is transmitted as all ones
            if (checksum == 0)
            {
                checksum = 0xffff;
            }
            i.WriteU16(checksum);
        }
    }
    else
    {
        i.WriteU16(m_checksum);
    }
}

uint32_t
UdpHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_sourcePort = i.ReadNtohU16();
    m_destinationPort = i.ReadNtohU16();
    m_payloadSize = i.ReadNtohU16() - GetSerializedSize();
    m_checksum = i.ReadU16();

    // RFC 768: An all zero transmitted checksum value means that the
    // transmitter generated  no checksum (for debugging or for higher
    // level protocols that don't care).
    //
    // This is common in IPv4, while IPv6 requires UDP to use its checksum.
    //
    // As strange as it might sound, flipping from 0x0000 to 0xffff does not
    // change anything in the verification.
    //
    // According to RFC 1141, the following holds:
    // ~C' = ~(C + (-m) + m') = ~C + (m - m') = ~C + m + ~m'
    // If ~C (the original CRC) is zero, m (the CRC field) is zero, and m' is 0xffff,
    // then, according to the formula, we have that ~C' is zero.
    // I.e., changing the CRC from 0 to 0xffff has no effect on the Rx verification.
    //
    // Fun fact: if you take an IPv4 header with an Identification field set to zero
    // and you change it to 0xffff, the checksum will not change (~_^)
    if (m_calcChecksum && m_checksum)
    {
        uint16_t headerChecksum = CalculateHeaderChecksum(start.GetSize());
        i = start;
        uint16_t checksum = i.CalculateIpChecksum(start.GetSize(), headerChecksum);

        m_goodChecksum = (checksum == 0);
    }

    return GetSerializedSize();
}

uint16_t
UdpHeader::GetChecksum() const
{
    return m_checksum;
}

} // namespace ns3
