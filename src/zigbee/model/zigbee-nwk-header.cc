/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Ryo Okuda  <c611901200@tokushima-u.ac.jp>
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-nwk-header.h"

#include "ns3/address-utils.h"

namespace ns3
{
namespace zigbee
{

ZigbeeNwkHeader::ZigbeeNwkHeader()
{
    SetFrameType(DATA);
    SetProtocolVer(3);
    SetDiscoverRoute(DiscoverRouteType::SUPPRESS_ROUTE_DISCOVERY);
    m_fctrlMcst = false;
    m_fctrlSecurity = false;
    m_fctrlSrcRoute = false;
    m_fctrlDstIEEEAddr = false;
    m_fctrlSrcIEEEAddr = false;
    m_fctrlEndDevIni = false;
}

ZigbeeNwkHeader::~ZigbeeNwkHeader()
{
}

void
ZigbeeNwkHeader::SetFrameType(enum NwkType nwkType)
{
    m_fctrlFrmType = nwkType;
}

NwkType
ZigbeeNwkHeader::GetFrameType() const
{
    return m_fctrlFrmType;
}

void
ZigbeeNwkHeader::SetProtocolVer(uint8_t ver)
{
    m_fctrlPrtVer = ver;
}

uint8_t
ZigbeeNwkHeader::GetProtocolVer() const
{
    return (m_fctrlPrtVer);
}

void
ZigbeeNwkHeader::SetDiscoverRoute(enum DiscoverRouteType discoverRoute)
{
    m_fctrlDscvRoute = discoverRoute;
}

DiscoverRouteType
ZigbeeNwkHeader::GetDiscoverRoute() const
{
    return m_fctrlDscvRoute;
}

void
ZigbeeNwkHeader::SetMulticast()
{
    m_fctrlMcst = true;
}

bool
ZigbeeNwkHeader::IsMulticast() const
{
    return m_fctrlMcst;
}

bool
ZigbeeNwkHeader::IsSecurityEnabled() const
{
    return m_fctrlSecurity;
}

void
ZigbeeNwkHeader::SetEndDeviceInitiator()
{
    m_fctrlEndDevIni = true;
}

bool
ZigbeeNwkHeader::GetEndDeviceInitiator() const
{
    return m_fctrlEndDevIni;
}

void
ZigbeeNwkHeader::SetDstAddr(Mac16Address addr)
{
    m_dstAddr = addr;
}

Mac16Address
ZigbeeNwkHeader::GetDstAddr() const
{
    return (m_dstAddr);
}

void
ZigbeeNwkHeader::SetSrcAddr(Mac16Address addr)
{
    m_srcAddr = addr;
}

Mac16Address
ZigbeeNwkHeader::GetSrcAddr() const
{
    return (m_srcAddr);
}

void
ZigbeeNwkHeader::SetRadius(uint8_t radius)
{
    m_radius = radius;
}

uint8_t
ZigbeeNwkHeader::GetRadius() const
{
    return (m_radius);
}

void
ZigbeeNwkHeader::SetSeqNum(uint8_t seqNum)
{
    m_seqNum = seqNum;
}

uint8_t
ZigbeeNwkHeader::GetSeqNum() const
{
    return (m_seqNum);
}

void
ZigbeeNwkHeader::SetDstIeeeAddr(Mac64Address dst)
{
    m_fctrlDstIEEEAddr = true;
    m_dstIeeeAddr = dst;
}

Mac64Address
ZigbeeNwkHeader::GetDstIeeeAddr() const
{
    return (m_dstIeeeAddr);
}

void
ZigbeeNwkHeader::SetSrcIeeeAddr(Mac64Address src)
{
    m_fctrlSrcIEEEAddr = true;
    m_srcIeeeAddr = src;
}

Mac64Address
ZigbeeNwkHeader::GetSrcIeeeAddr() const
{
    return (m_srcIeeeAddr);
}

void
ZigbeeNwkHeader::SetFrameControl(uint16_t frameControl)
{
    m_fctrlFrmType = static_cast<NwkType>((frameControl) & (0x03));                  // Bit 0-1
    m_fctrlPrtVer = (frameControl >> 2) & (0x0F);                                    // Bit 2-5
    m_fctrlDscvRoute = static_cast<DiscoverRouteType>((frameControl >> 6) & (0x03)); // Bit 6-7
    m_fctrlMcst = (frameControl >> 8) & (0x01);                                      // Bit 8
    m_fctrlSecurity = (frameControl >> 9) & (0x01);                                  // Bit 9
    m_fctrlSrcRoute = (frameControl >> 10) & (0x01);                                 // Bit 10
    m_fctrlDstIEEEAddr = (frameControl >> 11) & (0x01);                              // Bit 11
    m_fctrlSrcIEEEAddr = (frameControl >> 12) & (0x01);                              // Bit 12
    m_fctrlEndDevIni = (frameControl >> 13) & (0x01);                                // Bit 13
}

uint16_t
ZigbeeNwkHeader::GetFrameControl() const
{
    uint16_t val = 0;

    val = m_fctrlFrmType & (0x03);                    // Bit 0-1
    val |= (m_fctrlPrtVer << 2) & (0x0F << 2);        // Bit 2-5
    val |= (m_fctrlDscvRoute << 6) & (0x03 << 6);     // Bit 6-7
    val |= (m_fctrlMcst << 8) & (0x01 << 8);          // Bit 8
    val |= (m_fctrlSecurity << 9) & (0x01 << 9);      // Bit 9
    val |= (m_fctrlSrcRoute << 10) & (0x01 << 10);    // Bit 10
    val |= (m_fctrlDstIEEEAddr << 11) & (0x01 << 11); // Bit 11
    val |= (m_fctrlSrcIEEEAddr << 12) & (0x01 << 12); // Bit 12
    val |= (m_fctrlEndDevIni << 13) & (0x01 << 13);   // Bit 13

    return val;
}

void
ZigbeeNwkHeader::SetMulticastControl(uint8_t multicastControl)
{
    m_mcstMode = static_cast<MulticastMode>((multicastControl) & (0x03)); // Bit 0-1
    m_nonMemberRadius = (multicastControl >> 2) & (0x07);                 // Bit 2-4
    m_maxNonMemberRadius = (multicastControl >> 5) & (0x07);              // Bit 5-7
}

uint8_t
ZigbeeNwkHeader::GetMulticastControl() const
{
    uint8_t val = 0;

    val = m_mcstMode & (0x03);                        // Bit 0-1
    val |= (m_nonMemberRadius << 2) & (0x07 << 2);    // Bit 2-4
    val |= (m_maxNonMemberRadius << 5) & (0x07 << 5); // Bit 5-7

    return val;
}

void
ZigbeeNwkHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    uint16_t frameControl = GetFrameControl();
    i.WriteHtolsbU16(frameControl);

    WriteTo(i, m_dstAddr);
    WriteTo(i, m_srcAddr);
    i.WriteU8(m_radius);
    i.WriteU8(m_seqNum);

    if (m_fctrlDstIEEEAddr)
    {
        WriteTo(i, m_dstIeeeAddr);
    }

    if (m_fctrlSrcIEEEAddr)
    {
        WriteTo(i, m_srcIeeeAddr);
    }

    if (m_fctrlMcst)
    {
        i.WriteU8(GetMulticastControl());
    }

    // TODO: Add Source route subframe when supported
}

uint32_t
ZigbeeNwkHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint16_t frameControl = i.ReadLsbtohU16();
    SetFrameControl(frameControl);
    ReadFrom(i, m_dstAddr);
    ReadFrom(i, m_srcAddr);
    SetRadius(i.ReadU8());
    SetSeqNum(i.ReadU8());

    if (m_fctrlDstIEEEAddr)
    {
        ReadFrom(i, m_dstIeeeAddr);
    }

    if (m_fctrlSrcIEEEAddr)
    {
        ReadFrom(i, m_srcIeeeAddr);
    }

    if (m_fctrlMcst)
    {
        uint8_t multicastControl = i.ReadU8();
        SetMulticastControl(multicastControl);
    }

    // TODO: Add Source route subframe when supported

    return i.GetDistanceFrom(start);
}

uint32_t
ZigbeeNwkHeader::GetSerializedSize() const
{
    /*
     * Each NWK header will have:
     *
     * Frame Control      : 2 octet
     *
     * Routing Fields:
     * Destination address      : 2 octet
     * Source address           : 2 octet
     * Radius                   : 1 octet
     * Sequence number          : 1 octet
     * Destination IEEE address : 0/8 octet
     * Source IEEE address      : 0/8 octet
     * Multicast control        : 0/1 octet
     * Source route subframe    : variable
     */

    uint32_t size = 8;

    if (m_fctrlDstIEEEAddr)
    {
        size += 8;
    }

    if (m_fctrlSrcIEEEAddr)
    {
        size += 8;
    }

    if (m_fctrlMcst)
    {
        size += 1;
    }

    // TODO: Add Source route subframe when supported.

    return (size);
}

TypeId
ZigbeeNwkHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeeNwkHeader")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeeNwkHeader>();
    return tid;
}

TypeId
ZigbeeNwkHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
ZigbeeNwkHeader::Print(std::ostream& os) const
{
    os << "\n";
    switch (m_fctrlFrmType)
    {
    case DATA:
        os << "Frame Type = DATA ";
        break;
    case NWK_COMMAND:
        os << "Frame Type = NWK_COMMAND ";
        break;
    case INTER_PAN:
        os << "Frame Type = INTER_PAN  ";
        break;
    }
    os << " | Protocol Version = " << static_cast<uint32_t>(m_fctrlPrtVer)
       << " | Security Enabled = " << static_cast<uint32_t>(m_fctrlSecurity)
       << " | Source Route = " << static_cast<uint32_t>(m_fctrlSrcRoute);

    switch (m_fctrlDscvRoute)
    {
    case DiscoverRouteType::SUPPRESS_ROUTE_DISCOVERY:
        os << " | Enable Route Discovery = SUPPRESS ";
        break;
    case DiscoverRouteType::ENABLE_ROUTE_DISCOVERY:
        os << " | Enable Route Discovery = ENABLED ";
        break;
    }

    os << "\nDst Addr = " << m_dstAddr << " | Src Addr  = " << m_srcAddr
       << " | Sequence Num = " << static_cast<uint32_t>(m_seqNum)
       << " | Radius = " << static_cast<uint32_t>(m_radius);

    if (m_fctrlDstIEEEAddr)
    {
        os << "\nDst IEEE Addr = " << m_dstIeeeAddr;
    }

    if (m_fctrlSrcIEEEAddr)
    {
        os << " | Src IEEE Addr  = " << m_srcIeeeAddr;
    }

    if (m_fctrlMcst)
    {
        switch (m_mcstMode)
        {
        case MEMBER:
            os << " | Mcst mode: MEMBER ";
            break;
        case NONMEMBER:
            os << " | Mcst mode: NONMEMBER ";
            break;
        }

        os << " | Non Member radius = " << static_cast<uint32_t>(m_nonMemberRadius)
           << " | Max Non Member radius = " << static_cast<uint32_t>(m_maxNonMemberRadius);
    }
    os << "\n";

    // TODO: Add Source route subframe when supported.
}
} // namespace zigbee
} // namespace ns3
