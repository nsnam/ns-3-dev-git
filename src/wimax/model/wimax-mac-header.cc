/*
 * Copyright (c) 2007,2008, 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jahanzeb Farooq <jahanzeb.farooq@sophia.inria.fr>
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */

#include "wimax-mac-header.h"

#include "crc8.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(MacHeaderType);

MacHeaderType::MacHeaderType()
    : m_type(0)
{
}

MacHeaderType::MacHeaderType(uint8_t type)
    : m_type(type)
{
}

MacHeaderType::~MacHeaderType()
{
}

void
MacHeaderType::SetType(uint8_t type)
{
    m_type = type;
}

uint8_t
MacHeaderType::GetType() const
{
    return m_type;
}

std::string
MacHeaderType::GetName() const
{
    return "MAC Header Type";
}

TypeId
MacHeaderType::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MacHeaderType")
                            .SetParent<Header>()
                            .SetGroupName("Wimax")
                            .AddConstructor<MacHeaderType>();
    return tid;
}

TypeId
MacHeaderType::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MacHeaderType::Print(std::ostream& os) const
{
    os << " header type = " << (uint32_t)m_type;
}

uint32_t
MacHeaderType::GetSerializedSize() const
{
    return 0;
}

void
MacHeaderType::Serialize(Buffer::Iterator start) const
{
}

uint32_t
MacHeaderType::Deserialize(Buffer::Iterator start)
{
    return 0;
}

// ----------------------------------------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(GenericMacHeader);

GenericMacHeader::GenericMacHeader()
    : m_ht(0),
      m_ec(0),
      m_type(0),
      m_ci(0),
      m_eks(0),
      m_len(0),
      m_cid(Cid())
{
    m_esf = 0;
    m_hcs = 0;
    m_rsv1 = 0;
    c_hcs = 0;
}

GenericMacHeader::~GenericMacHeader()
{
}

void
GenericMacHeader::SetHt(uint8_t ht)
{
    m_ht = ht;
}

void
GenericMacHeader::SetEc(uint8_t ec)
{
    m_ec = ec;
}

void
GenericMacHeader::SetType(uint8_t type)
{
    m_type = type;
}

void
GenericMacHeader::SetCi(uint8_t ci)
{
    m_ci = ci;
}

void
GenericMacHeader::SetEks(uint8_t eks)
{
    m_eks = eks;
}

void
GenericMacHeader::SetLen(uint16_t len)
{
    m_len = len;
}

void
GenericMacHeader::SetCid(Cid cid)
{
    m_cid = cid;
}

void
GenericMacHeader::SetHcs(uint8_t hcs)
{
    m_hcs = hcs;
}

uint8_t
GenericMacHeader::GetHt() const
{
    return m_ht;
}

uint8_t
GenericMacHeader::GetEc() const
{
    return m_ec;
}

uint8_t
GenericMacHeader::GetType() const
{
    return m_type;
}

uint8_t
GenericMacHeader::GetCi() const
{
    return m_ci;
}

uint8_t
GenericMacHeader::GetEks() const
{
    return m_eks;
}

uint16_t
GenericMacHeader::GetLen() const
{
    return m_len;
}

Cid
GenericMacHeader::GetCid() const
{
    return m_cid;
}

uint8_t
GenericMacHeader::GetHcs() const
{
    return m_hcs;
}

std::string
GenericMacHeader::GetName() const
{
    return "Generic Mac Header";
}

TypeId
GenericMacHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GenericMacHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wimax")
                            .AddConstructor<GenericMacHeader>();
    return tid;
}

TypeId
GenericMacHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
GenericMacHeader::Print(std::ostream& os) const
{
    os << " ec (encryption control) = " << (uint32_t)m_ec << ", type = " << (uint32_t)m_type
       << ", ci (crc indicator) = " << (uint32_t)m_ci
       << ", eks (encryption key sequence) = " << (uint32_t)m_eks << ", len (length) = " << m_len
       << ", cid = " << m_cid << ", hcs (header check sequence) = " << (uint32_t)m_hcs;
}

uint32_t
GenericMacHeader::GetSerializedSize() const
{
    return 6;
}

void
GenericMacHeader::Serialize(Buffer::Iterator start) const
{
    /*
     * AI:Serialize function according to the
     * IEEE 8002.16e.
     * Please send bug and comments to
     * amine.ismail@udcast.com
     * amine.ismail@sophia.inria.fr
     */

    Buffer::Iterator i = start;

    uint8_t headerBuffer[6];
    memset(headerBuffer, 0, 6);

    headerBuffer[0] = ((m_ht << 7) & 0x80) | ((m_ec << 6) & 0x40) | (m_type & 0x3F);
    headerBuffer[1] = ((m_esf << 7) & 0x80) | ((m_ci << 6) & 0x40) | ((m_eks << 4) & 0x30) |
                      ((m_rsv1 << 3) & 0x08) | (((uint8_t)(m_len >> 8)) & 0x07);
    headerBuffer[2] = (uint8_t)(m_len);
    headerBuffer[3] = (uint8_t)(m_cid.GetIdentifier() >> 8);
    headerBuffer[4] = (uint8_t)(m_cid.GetIdentifier());
    uint8_t crc = CRC8Calculate(headerBuffer, 5);
    headerBuffer[5] = crc;
    for (int j = 0; j < 6; j++)
    {
        i.WriteU8(headerBuffer[j]);
    }
}

uint32_t
GenericMacHeader::Deserialize(Buffer::Iterator start)
{
    /*
     * AI:Deserialize function according to the
     * IEEE 8002.16e.
     * Please send bug and comments to
     * amine.ismail@udcast.com
     * amine.ismail@sophia.inria.fr
     */

    Buffer::Iterator i = start;

    uint8_t headerBuffer[6];
    for (int j = 0; j < 6; j++)
    {
        headerBuffer[j] = i.ReadU8();
    }
    m_ht = (headerBuffer[0] >> 7) & 0x01;
    m_ec = (headerBuffer[0] >> 6) & 0x01;
    m_type = (headerBuffer[0]) & 0x3F;
    m_esf = (headerBuffer[1] >> 7) & 0x01;
    m_ci = (headerBuffer[1] >> 6) & 0x01;
    m_eks = (headerBuffer[1] >> 4) & 0x03;
    m_rsv1 = (headerBuffer[1] >> 3) & 0x01;
    uint16_t lenmsb = (headerBuffer[1] & 0x07);
    uint16_t lenlsb = headerBuffer[2];
    m_len = ((lenmsb << 8) & 0x0700) | (lenlsb & 0x00FF);
    uint16_t cidmsb = headerBuffer[3];
    uint16_t cidlsb = headerBuffer[4];
    uint16_t cid = ((cidmsb << 8) & 0xFF00) | (cidlsb & 0x00FF);
    m_cid = Cid(cid);
    m_hcs = headerBuffer[5];
    c_hcs = CRC8Calculate(headerBuffer, 5);
    return i.GetDistanceFrom(start);
}

bool
GenericMacHeader::check_hcs() const
{
    return m_hcs == c_hcs;
}

// ----------------------------------------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(BandwidthRequestHeader);

BandwidthRequestHeader::BandwidthRequestHeader()
    : m_ht(1),
      m_ec(0),
      m_type(0),
      m_br(0),
      m_cid(Cid()),
      m_hcs(0)
{
}

BandwidthRequestHeader::~BandwidthRequestHeader()
{
}

void
BandwidthRequestHeader::SetHt(uint8_t ht)
{
    m_ht = ht;
}

void
BandwidthRequestHeader::SetEc(uint8_t ec)
{
    m_ec = ec;
}

void
BandwidthRequestHeader::SetType(uint8_t type)
{
    m_type = type;
}

void
BandwidthRequestHeader::SetBr(uint32_t br)
{
    m_br = br;
}

void
BandwidthRequestHeader::SetCid(Cid cid)
{
    m_cid = cid;
}

void
BandwidthRequestHeader::SetHcs(uint8_t hcs)
{
    m_hcs = hcs;
}

uint8_t
BandwidthRequestHeader::GetHt() const
{
    return m_ht;
}

uint8_t
BandwidthRequestHeader::GetEc() const
{
    return m_ec;
}

uint8_t
BandwidthRequestHeader::GetType() const
{
    return m_type;
}

uint32_t
BandwidthRequestHeader::GetBr() const
{
    return m_br;
}

Cid
BandwidthRequestHeader::GetCid() const
{
    return m_cid;
}

uint8_t
BandwidthRequestHeader::GetHcs() const
{
    return m_hcs;
}

std::string
BandwidthRequestHeader::GetName() const
{
    return "Bandwidth Request Header";
}

TypeId
BandwidthRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BandwidthRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wimax")
                            .AddConstructor<BandwidthRequestHeader>();
    return tid;
}

TypeId
BandwidthRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
BandwidthRequestHeader::Print(std::ostream& os) const
{
    os << " ec (encryption control) = " << (uint32_t)m_ec << ", type = " << (uint32_t)m_type
       << ", br (bandwidth request) = " << m_br << ", cid = ";
    m_cid.GetIdentifier();
    os << ", hcs (header check sequence) = " << (uint32_t)m_hcs;
}

uint32_t
BandwidthRequestHeader::GetSerializedSize() const
{
    /*
     * The size of the BandwidthRequest mac header is always 6 bytes
     */
    return 6;
}

void
BandwidthRequestHeader::Serialize(Buffer::Iterator start) const
{
    /*
     * AI:Serialize function according to the
     * IEEE 8002.16e.
     * please send bug and comments to
     * amine.ismail@udcast.com
     * amine.ismail@sophia.inria.fr
     */

    Buffer::Iterator i = start;
    uint8_t headerBuffer[6];
    uint8_t br_msb1 = (((uint32_t)m_br) >> 16) & 0x00000007;
    uint8_t br_msb2 = (((uint32_t)m_br) >> 8) & 0x000000FF;
    uint8_t br_lsb = m_br & 0x000000FF;
    headerBuffer[0] =
        ((m_ht << 7) & 0x80) | ((m_ec << 6) & 0x40) | ((m_type << 3) & 0x38) | br_msb1;
    headerBuffer[1] = br_msb2;
    headerBuffer[2] = br_lsb;
    headerBuffer[3] = (uint8_t)((m_cid.GetIdentifier() >> 8) & 0x00FF);
    headerBuffer[4] = (uint8_t)(m_cid.GetIdentifier() & 0x00FF);
    headerBuffer[5] = CRC8Calculate(headerBuffer, 5);

    for (int j = 0; j < 6; j++)
    {
        i.WriteU8(headerBuffer[j]);
    }
}

uint32_t
BandwidthRequestHeader::Deserialize(Buffer::Iterator start)
{
    /*
     * AI:Deserialize function according to the
     * IEEE 8002.16e.
     * Please send bug and comments to
     * amine.ismail@udcast.com
     * amine.ismail@sophia.inria.fr
     */

    Buffer::Iterator i = start;

    uint8_t headerBuffer[6];
    for (int j = 0; j < 6; j++)
    {
        headerBuffer[j] = i.ReadU8();
    }

    m_ht = (headerBuffer[0] >> 7) & 0x01;
    m_ec = (headerBuffer[0] >> 6) & 0x01;
    m_type = (headerBuffer[0] >> 3) & 0x07;
    uint32_t br_msb1 = headerBuffer[0] & 0x00000007;
    uint32_t br_msb2 = headerBuffer[1] & 0x000000FF;
    uint32_t br_lsb = headerBuffer[2] & 0x000000FF;
    m_br = ((uint32_t)br_msb1 << 14) | ((uint32_t)br_msb2 << 8) | br_lsb;
    uint16_t cidmsb = headerBuffer[3];
    uint16_t cidlsb = headerBuffer[4];
    uint16_t cid = ((cidmsb << 8) & 0xFF00) | (cidlsb & 0x00FF);
    m_cid = Cid(cid);
    m_hcs = headerBuffer[5];
    c_hcs = CRC8Calculate(headerBuffer, 5);

    return i.GetDistanceFrom(start);
}

bool
BandwidthRequestHeader::check_hcs() const
{
    return m_hcs == c_hcs;
}

// ----------------------------------------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(GrantManagementSubheader);

GrantManagementSubheader::GrantManagementSubheader()
    : m_si(0),
      m_pm(0),
      m_pbr(0)
{
}

GrantManagementSubheader::~GrantManagementSubheader()
{
}

void
GrantManagementSubheader::SetSi(uint8_t si)
{
    m_si = si;
}

void
GrantManagementSubheader::SetPm(uint8_t pm)
{
    m_pm = pm;
}

void
GrantManagementSubheader::SetPbr(uint16_t pbr)
{
    m_pbr = pbr;
}

uint8_t
GrantManagementSubheader::GetSi() const
{
    return m_si;
}

uint8_t
GrantManagementSubheader::GetPm() const
{
    return m_pm;
}

uint16_t
GrantManagementSubheader::GetPbr() const
{
    return m_pbr;
}

std::string
GrantManagementSubheader::GetName() const
{
    return "Grant Management Subheader";
}

TypeId
GrantManagementSubheader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::GrantManagementSubheader")
                            .SetParent<Header>()
                            .SetGroupName("Wimax")
                            .AddConstructor<GrantManagementSubheader>();
    return tid;
}

TypeId
GrantManagementSubheader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
GrantManagementSubheader::Print(std::ostream& os) const
{
    os << " si (slip indicator) = " << (uint32_t)m_si << ", pm (poll me) = " << (uint32_t)m_pm
       << ", pbr (piggyback request) = " << m_pbr;
}

uint32_t
GrantManagementSubheader::GetSerializedSize() const
{
    return 1 + 1 + 2;
}

void
GrantManagementSubheader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_si);
    i.WriteU8(m_pm);
    i.WriteU16(m_pbr);
}

uint32_t
GrantManagementSubheader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_si = i.ReadU8();
    m_pm = i.ReadU8();
    m_pbr = i.ReadU16();

    return i.GetDistanceFrom(start);
}

// ----------------------------------------------------------------------------------------------------------

NS_OBJECT_ENSURE_REGISTERED(FragmentationSubheader);

FragmentationSubheader::FragmentationSubheader()
    : m_fc(0),
      m_fsn(0)
{
}

FragmentationSubheader::~FragmentationSubheader()
{
}

void
FragmentationSubheader::SetFc(uint8_t fc)
{
    m_fc = fc;
}

void
FragmentationSubheader::SetFsn(uint8_t fsn)
{
    m_fsn = fsn;
}

uint8_t
FragmentationSubheader::GetFc() const
{
    return m_fc;
}

uint8_t
FragmentationSubheader::GetFsn() const
{
    return m_fsn;
}

std::string
FragmentationSubheader::GetName() const
{
    return "Fragmentation Subheader";
}

TypeId
FragmentationSubheader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FragmentationSubheader")
                            .SetParent<Header>()
                            .SetGroupName("Wimax")
                            .AddConstructor<FragmentationSubheader>();
    return tid;
}

TypeId
FragmentationSubheader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
FragmentationSubheader::Print(std::ostream& os) const
{
    os << " fc (fragment control) = " << (uint32_t)m_fc
       << ", fsn (fragmentation sequence number) = " << (uint32_t)m_fsn << "\n";
}

uint32_t
FragmentationSubheader::GetSerializedSize() const
{
    return 2;
}

void
FragmentationSubheader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_fc);
    i.WriteU8(m_fsn);
}

uint32_t
FragmentationSubheader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_fc = i.ReadU8();
    m_fsn = i.ReadU8();

    return i.GetDistanceFrom(start);
}

} // namespace ns3
