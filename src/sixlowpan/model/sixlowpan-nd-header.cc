/*
 * Copyright (c) 2015 Università di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *
 *
 * Author: Alessio Bonadio <alessio.bonadio@gmail.com>
 *         Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *         Adnan Rashid <adnanrashidpk@gmail.com>
 */

#include "sixlowpan-nd-header.h"

#include "ns3/address-utils.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include <ios>
#include <utility>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanNdHeader");

NS_OBJECT_ENSURE_REGISTERED(Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf);

Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf()
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_REQUEST);
    SetChecksum(0);
    m_status = 0;
    m_regTime = 0;
    m_regAddress = Ipv6Address("::");
}

Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf(
    bool request)
{
    NS_LOG_FUNCTION(this << request);
    SetType(request ? Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_REQUEST
                    : Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_CONFIRM);
    SetChecksum(0);
    m_status = 0;
    m_tid = 0;
    m_regTime = 0;
    m_regAddress = Ipv6Address("::");
}

Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf(
    uint16_t time,
    std::vector<uint8_t> rovr,
    Ipv6Address address)
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_REQUEST);
    SetChecksum(0);
    m_status = 0;
    m_regTime = time;
    SetRovr(rovr);
    m_regAddress = address;
}

Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf(
    uint8_t status,
    uint16_t time,
    std::vector<uint8_t> rovr,
    Ipv6Address address)
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_ND_DUPLICATE_ADDRESS_CONFIRM);
    SetChecksum(0);
    m_status = status;
    m_regTime = time;
    SetRovr(rovr);
    m_regAddress = address;
}

Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::
    ~Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf()
{
    NS_LOG_FUNCTION(this);
}

TypeId
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Icmpv6DuplicateAddress")
                            .SetParent<Icmpv6Header>()
                            .SetGroupName("Internet")
                            .AddConstructor<Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf>();
    return tid;
}

TypeId
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::GetInstanceTypeId() const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

uint8_t
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::GetStatus() const
{
    NS_LOG_FUNCTION(this);
    return m_status;
}

void
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::SetStatus(uint8_t status)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(status));
    m_status = status;
}

uint16_t
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::GetRegTime() const
{
    NS_LOG_FUNCTION(this);
    return m_regTime;
}

void
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::SetRegTime(uint16_t time)
{
    NS_LOG_FUNCTION(this << time);
    m_regTime = time;
}

std::vector<uint8_t>
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::GetRovr() const
{
    NS_LOG_FUNCTION(this);
    return m_rovr;
}

void
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::SetRovr(const std::vector<uint8_t>& rovr)
{
    NS_LOG_FUNCTION(this);

    uint8_t rovrLength = rovr.size();
    NS_ASSERT_MSG((rovrLength == 8) || (rovrLength == 16) || (rovrLength == 24) ||
                      (rovrLength == 32),
                  "ROVR length must be 64, 128, 192, or 256 bits");

    m_rovr = std::move(rovr);
}

Ipv6Address
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::GetRegAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_regAddress;
}

void
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::SetRegAddress(Ipv6Address registered)
{
    NS_LOG_FUNCTION(this << registered);
    m_regAddress = registered;
}

void
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);

    std::ios oldState(nullptr);
    oldState.copyfmt(os);
    uint8_t rovrLength = m_rovr.size();

    os << "( type = " << (uint32_t)GetType() << " status " << (uint32_t)m_status << " TID " << m_tid
       << " lifetime " << m_regTime << " ROVR len " << rovrLength;
    for (uint8_t index = 0; index < rovrLength; index++)
    {
        os << std::hex << m_rovr[index];
    }
    os << ")";

    os.copyfmt(oldState);
}

uint32_t
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 8 + m_rovr.size();
}

void
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    i.WriteU8(GetType());
    i.WriteU8(m_status);

    // note: codePfx is always zero, ROVR size is a multiple of 8 bytes, and less than 32 bytes.
    uint8_t codeSfx = m_rovr.size() / 8;
    i.WriteU8(codeSfx);

    i.WriteU16(m_checksum);
    i.WriteU8(m_status);
    i.WriteU8(m_tid);
    i.WriteU16(m_regTime);

    for (const auto& rovr : m_rovr)
    {
        i.WriteU8(rovr);
    }
}

uint32_t
Icmpv6SixLowPanExtendedDuplicateAddressReqOrConf::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    SetType(i.ReadU8());
    uint8_t codePSfx = i.ReadU8();
    if (codePSfx > 4)
    {
        NS_LOG_LOGIC("Invalid CodeSfx or CodePfx value (" << codePSfx << "), discarding message");
        return 0;
    }
    uint8_t rovrLength = codePSfx * 8;

    m_status = i.ReadU8();
    i.Next(3);
    m_regTime = i.ReadU16();

    m_rovr.clear();
    for (uint8_t index = 0; index < rovrLength; index++)
    {
        m_rovr.push_back(i.ReadU8());
    }

    return GetSerializedSize();
}

NS_OBJECT_ENSURE_REGISTERED(Icmpv6OptionSixLowPanExtendedAddressRegistration);

Icmpv6OptionSixLowPanExtendedAddressRegistration::Icmpv6OptionSixLowPanExtendedAddressRegistration()
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_OPT_EXTENDED_ADDRESS_REGISTRATION);
    SetLength(1);
    m_status = 0;
    m_opaque = 0;
    m_i = 0;
    m_flagR = false;
    m_tid = 0;
    m_regTime = 0;
}

Icmpv6OptionSixLowPanExtendedAddressRegistration::Icmpv6OptionSixLowPanExtendedAddressRegistration(
    uint16_t time,
    const std::vector<uint8_t>& rovr,
    uint8_t tid)
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_OPT_EXTENDED_ADDRESS_REGISTRATION);
    SetLength(1 + rovr.size() / 8);
    m_status = 0;
    m_opaque = 0;
    m_i = 0;
    m_flagR = false;
    m_tid = tid;
    m_regTime = time;
    SetRovr(rovr);
}

Icmpv6OptionSixLowPanExtendedAddressRegistration::Icmpv6OptionSixLowPanExtendedAddressRegistration(
    uint8_t status,
    uint16_t time,
    const std::vector<uint8_t>& rovr,
    uint8_t tid)
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_OPT_EXTENDED_ADDRESS_REGISTRATION);
    SetLength(1 + rovr.size() / 8);
    m_status = status;
    m_opaque = 0;
    m_i = 0;
    m_flagR = false;
    m_tid = tid;
    m_regTime = time;
    SetRovr(rovr);
}

Icmpv6OptionSixLowPanExtendedAddressRegistration::
    ~Icmpv6OptionSixLowPanExtendedAddressRegistration()
{
    NS_LOG_FUNCTION(this);
}

TypeId
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Icmpv6OptionAddressRegistration")
                            .SetParent<Icmpv6OptionHeader>()
                            .SetGroupName("Internet")
                            .AddConstructor<Icmpv6OptionSixLowPanExtendedAddressRegistration>();
    return tid;
}

TypeId
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetInstanceTypeId() const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

uint8_t
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetStatus() const
{
    NS_LOG_FUNCTION(this);
    return m_status;
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::SetStatus(uint8_t status)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(status));
    m_status = status;
}

uint8_t
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetOpaque() const
{
    NS_LOG_FUNCTION(this);
    return m_opaque;
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::SetOpaque(uint8_t opaque)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(opaque));
    m_opaque = opaque;
}

uint8_t
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetI() const

{
    NS_LOG_FUNCTION(this);
    return m_i;
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::SetI(uint8_t twobit)
{
    if (twobit > 3)
    {
        NS_LOG_FUNCTION(this << static_cast<uint32_t>(twobit));
        return;
    }
    m_i = twobit;
}

bool
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetFlagR() const
{
    NS_LOG_FUNCTION(this);
    return m_flagR;
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::SetFlagR(bool r)
{
    NS_LOG_FUNCTION(this << r);
    m_flagR = r;
}

uint8_t
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetTransactionId() const
{
    NS_LOG_FUNCTION(this);
    return m_tid;
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::SetTransactionId(uint8_t tid)
{
    NS_LOG_FUNCTION(this);
    m_tid = tid;
}

uint16_t
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetRegTime() const
{
    NS_LOG_FUNCTION(this);
    return m_regTime;
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::SetRegTime(uint16_t time)
{
    NS_LOG_FUNCTION(this << time);
    m_regTime = time;
}

std::vector<uint8_t>
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetRovr() const
{
    NS_LOG_FUNCTION(this);
    return m_rovr;
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::SetRovr(const std::vector<uint8_t>& rovr)
{
    NS_LOG_FUNCTION(this);

    uint8_t rovrLength = rovr.size();
    NS_ASSERT_MSG((rovrLength == 8) || (rovrLength == 16) || (rovrLength == 24) ||
                      (rovrLength == 32),
                  "ROVR length must be 64, 128, 192, or 256 bits");

    m_rovr = std::move(rovr);
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);

    std::ios oldState(nullptr);
    oldState.copyfmt(os);
    uint8_t rovrLength = m_rovr.size();

    os << "(type = " << (uint32_t)GetType() << " length = " << (uint32_t)GetLength() << " status "
       << (uint32_t)m_status << " lifetime " << m_regTime << " ROVR (" << +rovrLength << ") ";
    for (uint8_t index = 0; index < rovrLength; index++)
    {
        os << std::hex << +m_rovr[index];
    }
    os << ")";

    os.copyfmt(oldState);
}

uint32_t
Icmpv6OptionSixLowPanExtendedAddressRegistration::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 8 + m_rovr.size();
}

void
Icmpv6OptionSixLowPanExtendedAddressRegistration::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    i.WriteU8(GetType());
    i.WriteU8(GetLength());
    i.WriteU8(m_status);
    i.WriteU8(m_opaque);

    uint8_t flags;
    flags = m_i;
    flags <<= 1;
    if (m_flagR)
    {
        flags |= 1;
    }
    flags <<= 1;

    // Flag T *must* be set to comply with rfc 8505
    flags |= 1;

    i.WriteU8(flags);
    i.WriteU8(m_tid);
    i.WriteU16(m_regTime);

    for (const auto& rovr : m_rovr)
    {
        i.WriteU8(rovr);
    }
}

uint32_t
Icmpv6OptionSixLowPanExtendedAddressRegistration::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;

    SetType(i.ReadU8());
    SetLength(i.ReadU8());
    uint8_t rovrLength = (GetLength() - 1) * 8;

    m_status = i.ReadU8();
    m_opaque = i.ReadU8();

    uint8_t flags = i.ReadU8();
    if ((flags & 0x01) != 0x01)
    {
        NS_LOG_LOGIC("Received an EARO without the T flag set - ignoring");
        return 0;
    }
    if ((flags & 0x02) == 0x02)
    {
        m_flagR = true;
    }
    m_i = (flags >> 2) & 0x03;

    m_tid = i.ReadU8();
    m_regTime = i.ReadU16();

    m_rovr.clear();
    for (uint8_t index = 0; index < rovrLength; index++)
    {
        m_rovr.push_back(i.ReadU8());
    }

    return GetSerializedSize();
}

NS_OBJECT_ENSURE_REGISTERED(Icmpv6OptionSixLowPanContext);

Icmpv6OptionSixLowPanContext::Icmpv6OptionSixLowPanContext()
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_OPT_SIXLOWPAN_CONTEXT);
    SetLength(0);
    m_contextLen = 0;
    m_c = false;
    m_cid = 0;
    m_validTime = 0;
    m_prefix = Ipv6Prefix("::");
}

Icmpv6OptionSixLowPanContext::Icmpv6OptionSixLowPanContext(bool c,
                                                           uint8_t cid,
                                                           uint16_t time,
                                                           Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_OPT_SIXLOWPAN_CONTEXT);

    if (prefix.GetPrefixLength() > 64)
    {
        SetLength(3);
    }
    else
    {
        SetLength(2);
    }

    m_contextLen = prefix.GetPrefixLength();
    m_c = c;
    m_cid = cid;
    m_validTime = time;
    m_prefix = prefix;
}

Icmpv6OptionSixLowPanContext::~Icmpv6OptionSixLowPanContext()
{
    NS_LOG_FUNCTION(this);
}

TypeId
Icmpv6OptionSixLowPanContext::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Icmpv6OptionSixLowPanContext")
                            .SetParent<Icmpv6OptionHeader>()
                            .SetGroupName("Internet")
                            .AddConstructor<Icmpv6OptionSixLowPanContext>();
    return tid;
}

TypeId
Icmpv6OptionSixLowPanContext::GetInstanceTypeId() const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

uint8_t
Icmpv6OptionSixLowPanContext::GetContextLen() const
{
    NS_LOG_FUNCTION(this);
    return m_contextLen;
}

bool
Icmpv6OptionSixLowPanContext::IsFlagC() const
{
    NS_LOG_FUNCTION(this);
    return m_c;
}

void
Icmpv6OptionSixLowPanContext::SetFlagC(bool c)
{
    NS_LOG_FUNCTION(this << c);
    m_c = c;
}

uint8_t
Icmpv6OptionSixLowPanContext::GetCid() const
{
    NS_LOG_FUNCTION(this);
    return m_cid;
}

void
Icmpv6OptionSixLowPanContext::SetCid(uint8_t cid)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(cid));
    NS_ASSERT(cid <= 15);
    m_cid = cid;
}

uint16_t
Icmpv6OptionSixLowPanContext::GetValidTime() const
{
    NS_LOG_FUNCTION(this);
    return m_validTime;
}

void
Icmpv6OptionSixLowPanContext::SetValidTime(uint16_t time)
{
    NS_LOG_FUNCTION(this << time);
    m_validTime = time;
}

Ipv6Prefix
Icmpv6OptionSixLowPanContext::GetContextPrefix() const
{
    NS_LOG_FUNCTION(this);
    return m_prefix;
}

void
Icmpv6OptionSixLowPanContext::SetContextPrefix(Ipv6Prefix prefix)
{
    NS_LOG_FUNCTION(this << prefix);
    m_prefix = prefix;
    m_contextLen = prefix.GetPrefixLength();

    if (prefix.GetPrefixLength() > 64)
    {
        SetLength(3);
    }
    else
    {
        SetLength(2);
    }
}

void
Icmpv6OptionSixLowPanContext::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "( type = " << (uint32_t)GetType() << " length = " << (uint32_t)GetLength()
       << " context length = " << (uint32_t)m_contextLen << " flag C = " << m_c
       << " CID = " << (uint32_t)m_cid << " lifetime = " << m_validTime
       << " context prefix = " << m_prefix.ConvertToIpv6Address() << "/"
       << +m_prefix.GetPrefixLength() << ")";
}

uint32_t
Icmpv6OptionSixLowPanContext::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    uint8_t nb = GetLength() * 8;
    return nb;
}

void
Icmpv6OptionSixLowPanContext::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    uint8_t buf[16];
    uint8_t bitfield = 0;

    memset(buf, 0x0000, sizeof(buf));

    i.WriteU8(GetType());
    i.WriteU8(GetLength());
    i.WriteU8(m_contextLen);

    bitfield |= m_cid;
    bitfield |= (uint8_t)(m_c << 4);

    i.WriteU8(bitfield);
    i.WriteU16(0);
    i.WriteU16(m_validTime);

    m_prefix.GetBytes(buf);
    if (m_contextLen <= 64)
    {
        i.Write(buf, 8);
    }
    else
    {
        i.Write(buf, 16);
    }
}

uint32_t
Icmpv6OptionSixLowPanContext::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    uint8_t buf[16];
    uint8_t bitfield;

    memset(buf, 0x0000, sizeof(buf));

    SetType(i.ReadU8());
    SetLength(i.ReadU8());
    m_contextLen = i.ReadU8();

    bitfield = i.ReadU8();
    m_c = false;
    if (bitfield & (uint8_t)(1 << 4))
    {
        m_c = true;
    }
    m_cid = bitfield & 0xF;
    i.Next(2);
    m_validTime = i.ReadNtohU16();

    if (GetContextLen() <= 64)
    {
        i.Read(buf, 8);
    }
    else
    {
        i.Read(buf, 16);
    }
    m_prefix = Ipv6Prefix(buf, m_contextLen);

    if (m_contextLen > 64)
    {
        SetLength(3);
    }
    else
    {
        SetLength(2);
    }

    return GetSerializedSize();
}

NS_OBJECT_ENSURE_REGISTERED(Icmpv6OptionSixLowPanAuthoritativeBorderRouter);

Icmpv6OptionSixLowPanAuthoritativeBorderRouter::Icmpv6OptionSixLowPanAuthoritativeBorderRouter()
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_OPT_AUTHORITATIVE_BORDER_ROUTER);
    SetLength(3);
    m_version = 0;
    m_validTime = 0;
    m_routerAddress = Ipv6Address("::");
}

Icmpv6OptionSixLowPanAuthoritativeBorderRouter::Icmpv6OptionSixLowPanAuthoritativeBorderRouter(
    uint32_t version,
    uint16_t time,
    Ipv6Address address)
{
    NS_LOG_FUNCTION(this);
    SetType(Icmpv6Header::ICMPV6_OPT_AUTHORITATIVE_BORDER_ROUTER);
    SetLength(3);
    m_version = version;
    m_validTime = time;
    m_routerAddress = address;
}

Icmpv6OptionSixLowPanAuthoritativeBorderRouter::~Icmpv6OptionSixLowPanAuthoritativeBorderRouter()
{
    NS_LOG_FUNCTION(this);
}

TypeId
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Icmpv6OptionAuthoritativeBorderRouter")
                            .SetParent<Icmpv6OptionHeader>()
                            .SetGroupName("Internet")
                            .AddConstructor<Icmpv6OptionSixLowPanAuthoritativeBorderRouter>();
    return tid;
}

TypeId
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::GetInstanceTypeId() const
{
    NS_LOG_FUNCTION(this);
    return GetTypeId();
}

uint32_t
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::GetVersion() const
{
    NS_LOG_FUNCTION(this);
    return m_version;
}

void
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::SetVersion(uint32_t version)
{
    NS_LOG_FUNCTION(this << version);
    m_version = version;
}

uint16_t
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::GetValidLifeTime() const
{
    NS_LOG_FUNCTION(this);
    return m_validTime;
}

void
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::SetValidLifeTime(uint16_t time)
{
    NS_LOG_FUNCTION(this << time);
    m_validTime = time;
}

Ipv6Address
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::GetRouterAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_routerAddress;
}

void
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::SetRouterAddress(Ipv6Address router)
{
    NS_LOG_FUNCTION(this << router);
    m_routerAddress = router;
}

void
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << "( type = " << (uint32_t)GetType() << " length = " << (uint32_t)GetLength()
       << " version = " << m_version << " lifetime = " << m_validTime
       << " router address = " << m_routerAddress << ")";
}

uint32_t
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 24;
}

void
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::Serialize(Buffer::Iterator start) const
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    uint8_t buf[16];
    uint32_t versionL = m_version;
    uint32_t versionH = m_version;

    memset(buf, 0x0000, sizeof(buf));

    i.WriteU8(GetType());
    i.WriteU8(GetLength());

    versionL &= 0xFFFF;
    i.WriteU16((uint16_t)versionL);
    versionH >>= 16;
    versionH &= 0xFFFF;
    i.WriteU16((uint16_t)versionH);

    i.WriteU16(m_validTime);

    m_routerAddress.Serialize(buf);
    i.Write(buf, 16);
}

uint32_t
Icmpv6OptionSixLowPanAuthoritativeBorderRouter::Deserialize(Buffer::Iterator start)
{
    NS_LOG_FUNCTION(this << &start);
    Buffer::Iterator i = start;
    uint8_t buf[16];
    uint32_t versionL;
    uint32_t versionH;

    memset(buf, 0x0000, sizeof(buf));

    SetType(i.ReadU8());
    SetLength(i.ReadU8());

    versionL = (uint32_t)i.ReadU16();
    versionH = (uint32_t)i.ReadU16();
    versionH <<= 16;
    m_version = (versionL &= 0xFFFF) + (versionH &= 0xFFFF0000);

    m_validTime = i.ReadU16();

    i.Read(buf, 16);
    m_routerAddress = Ipv6Address::Deserialize(buf);

    return GetSerializedSize();
}

} /* namespace ns3 */
