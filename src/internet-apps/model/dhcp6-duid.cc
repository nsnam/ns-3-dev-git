/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#include "dhcp6-duid.h"

#include "ns3/address.h"
#include "ns3/assert.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/loopback-net-device.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Duid");

bool
Duid::IsInvalid() const
{
    return m_blob.empty();
}

uint8_t
Duid::GetLength() const
{
    return m_blob.size();
}

std::vector<uint8_t>
Duid::GetIdentifier() const
{
    return m_blob;
}

Duid::Type
Duid::GetDuidType() const
{
    return m_duidType;
}

void
Duid::InitializeLL(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);

    m_duidType = Type::LL;
    Address duidAddress = FindSuitableNetDeviceAddress(node);

    uint8_t idLen = duidAddress.GetLength();
    NS_ASSERT_MSG(idLen == 6 || idLen == 8, "Duid: Invalid identifier length.");

    m_blob.resize(idLen + Offset::LL);

    // Ethernet (48 bit) - HardwareType 1
    // EUI-64 (64 bit) - HardwareType 27
    m_blob[0] = 0;
    m_blob[1] = idLen == 6 ? 1 : 27;

    duidAddress.CopyTo(&m_blob[Offset::LL]);
}

void
Duid::InitializeLLT(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);

    m_duidType = Type::LLT;
    Address duidAddress = FindSuitableNetDeviceAddress(node);

    uint8_t idLen = duidAddress.GetLength();
    NS_ASSERT_MSG(idLen == 6 || idLen == 8, "Duid: Invalid identifier length.");

    m_blob.resize(idLen + Offset::LLT);

    // Ethernet (48 bit) - HardwareType 1
    // EUI-64 (64 bit) - HardwareType 27
    m_blob[0] = 0;
    m_blob[1] = idLen == 6 ? 1 : 27;

    // This should be (according to RFC 8415)
    // seconds since midnight (UTC), January 1, 2000, modulo 2 ^ 32
    // We do not have an (easy) way to measure the difference.

    uint32_t time = std::floor(std::fmod(Simulator::Now().GetSeconds(), std::pow(2, 32)));
    HtoN(time, m_blob.begin() + Offset::LL);

    duidAddress.CopyTo(&m_blob[Offset::LLT]);
}

void
Duid::InitializeEN(uint32_t enNumber, const std::vector<uint8_t>& identifier)
{
    NS_LOG_FUNCTION(this << enNumber << identifier);

    m_duidType = Type::EN;

    // The DUID is going to be embedded in an option, which
    // have a DUID length expressed 16-bit
    // this leaves 65535 - 2 - 4 bytes for the identifier.
    NS_ASSERT_MSG(identifier.size() < 65535 - 6,
                  "Duid: Invalid identifier length (max length is 65529 bytes).");

    m_blob.resize(identifier.size() + Offset::EN);

    HtoN(enNumber, m_blob.begin());

    std::move(identifier.cbegin(), identifier.cend(), &m_blob[Offset::EN]);
}

void
Duid::InitializeUUID(const std::vector<uint8_t>& uuid)
{
    NS_LOG_FUNCTION(this << uuid);

    m_duidType = Type::UUID;

    NS_ASSERT_MSG(uuid.size() == 16, "Duid: Invalid identifier length (UUID must be 128 bits).");

    m_blob.resize(uuid.size());

    std::move(uuid.cbegin(), uuid.cend(), m_blob.begin());
}

void
Duid::Initialize(Type duidType, Ptr<Node> node, uint32_t enIdentifierLength)
{
    NS_LOG_FUNCTION(this << std::to_underlying(duidType) << node << enIdentifierLength);

    switch (duidType)
    {
    case Duid::Type::LLT: {
        InitializeLLT(node);
        break;
    }
    case Duid::Type::EN: {
        // We use enterprise-number 0xf00dcafe (totally arbitrary)
        // The list of numbers is at
        // https://www.iana.org/assignments/enterprise-numbers/
        std::vector<uint8_t> identifier(enIdentifierLength);
        HtoN(node->GetId(), identifier.begin());
        InitializeEN(0xf00dcafe, identifier);
        break;
    }
    case Duid::Type::LL: {
        InitializeLL(node);
        break;
    }
    case Duid::Type::UUID: {
        std::vector<uint8_t> identifier(16);
        HtoN(node->GetId(), identifier.begin());
        InitializeUUID(identifier);
        break;
    }
    }
}

Address
Duid::FindSuitableNetDeviceAddress(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);

    Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol>();
    NS_ASSERT_MSG(ipv6, "DHCPv6: No IPv6 stack found.");
    uint32_t nInterfaces = ipv6->GetNInterfaces();

    uint32_t maxAddressLength = 0;
    Address duidAddress;

    // We select NetDevices with the longest MAC address length,
    // and among them, the one with the smallest index.
    for (uint32_t i = 0; i < nInterfaces; i++)
    {
        Ptr<NetDevice> device = ipv6->GetNetDevice(i);

        // Discard the loopback device.
        if (DynamicCast<LoopbackNetDevice>(device))
        {
            continue;
        }

        // Check if the NetDevice is up.
        if (device->IsLinkUp())
        {
            NS_LOG_DEBUG("Interface " << device->GetIfIndex() << " up on node " << node->GetId());
            Address address = device->GetAddress();
            if (address.GetLength() > maxAddressLength)
            {
                maxAddressLength = address.GetLength();
                duidAddress = address;
            }
        }
    }

    NS_ASSERT_MSG(!duidAddress.IsInvalid(), "Duid: No suitable NetDevice found for DUID.");

    NS_LOG_DEBUG("DUID of node " << node->GetId() << " is " << duidAddress);

    return duidAddress;
}

uint32_t
Duid::GetSerializedSize() const
{
    // The DUID type is 2 bytes.
    return 2 + m_blob.size();
}

void
Duid::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU16(static_cast<std::underlying_type_t<Duid::Type>>(m_duidType));

    i.Write(&m_blob[0], m_blob.size());
}

uint32_t
Duid::Deserialize(Buffer::Iterator start, uint32_t len)
{
    Buffer::Iterator i = start;

    m_duidType = static_cast<Duid::Type>(i.ReadNtohU16());
    len -= 2;

    m_blob.resize(len);
    i.Read(&m_blob[0], m_blob.size());

    return GetSerializedSize();
}

void
Duid::HtoN(uint32_t source, std::vector<uint8_t>::iterator dest)
{
    dest[0] = (source >> 24);
    dest[1] = (source >> 16) & 0xFF;
    dest[2] = (source >> 8) & 0xFF;
    dest[3] = source & 0xFF;
}

size_t
Duid::DuidHash::operator()(const Duid& x) const noexcept
{
    uint8_t duidLen = x.GetLength();
    std::vector<uint8_t> buffer = x.GetIdentifier();

    std::string s(buffer.begin(), buffer.begin() + duidLen);
    return std::hash<std::string>{}(s);
}

// Comparison operators are defined because gcc-13.3 raises an error in optimized builds.
// Other than that, the comparisons are semantically identical to the default ones.
bool
Duid::operator==(const Duid& other) const
{
    return std::is_eq(*this <=> other);
};

std::strong_ordering
operator<=>(const Duid& a, const Duid& b)
{
    if (a.m_duidType == b.m_duidType)
    {
        return a.m_blob <=> b.m_blob;
    }

    return a.m_duidType <=> b.m_duidType;
}

std::ostream&
operator<<(std::ostream& os, const Duid& duid)
{
    auto fold16 = [](std::vector<uint8_t>::const_iterator a) -> uint16_t {
        auto f = static_cast<uint16_t>(a[0]) << 8;
        f += static_cast<uint16_t>(a[1]);
        return f;
    };

    auto fold32 = [](std::vector<uint8_t>::const_iterator a) -> uint32_t {
        auto f = static_cast<uint32_t>(a[0]) << 24;
        f += static_cast<uint32_t>(a[1]) << 16;
        f += static_cast<uint32_t>(a[2]) << 8;
        f += static_cast<uint32_t>(a[3]);
        return f;
    };

    auto format = [](std::vector<uint8_t>::const_iterator start,
                     std::vector<uint8_t>::const_iterator end) -> std::string {
        std::ostringstream str;
        str << std::hex;
        std::for_each(start, end, [&](int i) { str << std::setfill('0') << std::setw(2) << i; });
        return str.str();
    };

    switch (duid.m_duidType)
    {
    case Duid::Type::LLT: {
        os << "DUID-LLT";
        os << " HWtype: " << fold16(duid.m_blob.cbegin());
        os << " Time: " << fold32(duid.m_blob.cbegin() + 2);
        auto identifierStart = duid.m_blob.cbegin() + Duid::Offset::LLT;
        os << " Identifier: 0x" << format(identifierStart, duid.m_blob.cend());
        break;
    }
    case Duid::Type::EN: {
        os << "DUID-EN";
        os << " EnNumber: 0x"
           << format(duid.m_blob.cbegin(), duid.m_blob.cbegin() + Duid::Offset::EN);
        auto identifierStart = duid.m_blob.cbegin() + Duid::Offset::EN;
        os << " Identifier: 0x" << format(identifierStart, duid.m_blob.cend());
        break;
    }
    case Duid::Type::LL: {
        os << "DUID-LL";
        os << " HWtype: " << fold16(duid.m_blob.cbegin());
        auto identifierStart = duid.m_blob.cbegin() + Duid::Offset::LL;
        os << " Identifier: 0x" << format(identifierStart, duid.m_blob.cend());
        break;
    }
    case Duid::Type::UUID: {
        os << "DUID-UUID";
        auto identifierStart = duid.m_blob.cbegin();
        os << " UUID: 0x" << format(identifierStart, duid.m_blob.cend());
        break;
    }
    }
    return os;
}

} // namespace ns3
