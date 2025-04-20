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

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Dhcp6Duid");

Duid::Duid()
{
    m_duidType = Duid::Type::LL;
    m_hardwareType = 0;
    m_time = Time();
    m_identifier = std::vector<uint8_t>();
}

bool
Duid::operator==(const Duid& o) const
{
    return (m_duidType == o.m_duidType && m_hardwareType == o.m_hardwareType &&
            m_identifier == o.m_identifier);
}

bool
operator<(const Duid& a, const Duid& b)
{
    if (a.m_duidType < b.m_duidType)
    {
        return true;
    }
    else if (a.m_duidType > b.m_duidType)
    {
        return false;
    }
    if (a.m_hardwareType < b.m_hardwareType)
    {
        return true;
    }
    else if (a.m_hardwareType > b.m_hardwareType)
    {
        return false;
    }
    NS_ASSERT(a.GetLength() == b.GetLength());
    return a.m_identifier < b.m_identifier;
}

bool
Duid::IsInvalid() const
{
    return m_identifier.empty();
}

uint8_t
Duid::GetLength() const
{
    return m_identifier.size();
}

std::vector<uint8_t>
Duid::GetIdentifier() const
{
    return m_identifier;
}

Duid::Type
Duid::GetDuidType() const
{
    return m_duidType;
}

void
Duid::SetDuidType(Duid::Type duidType)
{
    m_duidType = duidType;
}

uint16_t
Duid::GetHardwareType() const
{
    return m_hardwareType;
}

void
Duid::SetHardwareType(uint16_t hardwareType)
{
    NS_LOG_FUNCTION(this << hardwareType);
    m_hardwareType = hardwareType;
}

void
Duid::SetDuid(std::vector<uint8_t> identifier)
{
    NS_LOG_FUNCTION(this << identifier);

    m_duidType = Type::LL;
    uint8_t idLen = identifier.size();

    NS_ASSERT_MSG(idLen == 6 || idLen == 8, "Duid: Invalid identifier length.");

    switch (idLen)
    {
    case 6:
        // Ethernet - 48 bit length
        SetHardwareType(1);
        break;
    case 8:
        // EUI-64 - 64 bit length
        SetHardwareType(27);
        break;
    }

    m_identifier.resize(idLen);
    m_identifier = identifier;
}

void
Duid::Initialize(Ptr<Node> node)
{
    Ptr<Ipv6L3Protocol> ipv6 = node->GetObject<Ipv6L3Protocol>();
    uint32_t nInterfaces = ipv6->GetNInterfaces();

    uint32_t maxAddressLength = 0;
    Address duidAddress;

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

    // Consider the link-layer address of the first NetDevice in the list.
    uint8_t buffer[16];
    duidAddress.CopyTo(buffer);

    std::vector<uint8_t> identifier(duidAddress.GetLength());
    std::copy(buffer, buffer + duidAddress.GetLength(), identifier.begin());
    SetDuid(identifier);
}

Time
Duid::GetTime() const
{
    return m_time;
}

void
Duid::SetTime(Time time)
{
    NS_LOG_FUNCTION(this << time);
    m_time = time;
}

uint32_t
Duid::GetSerializedSize() const
{
    return 4 + m_identifier.size();
}

void
Duid::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU16(static_cast<std::underlying_type_t<Duid::Type>>(m_duidType));
    i.WriteHtonU16(m_hardwareType);

    for (const auto& byte : m_identifier)
    {
        i.WriteU8(byte);
    }
}

uint32_t
Duid::Deserialize(Buffer::Iterator start, uint32_t len)
{
    Buffer::Iterator i = start;

    m_duidType = static_cast<Duid::Type>(i.ReadNtohU16());
    m_hardwareType = i.ReadNtohU16();
    m_identifier.resize(len);

    for (uint32_t j = 0; j < len; j++)
    {
        m_identifier[j] = i.ReadU8();
    }

    return 4 + m_identifier.size();
}

size_t
Duid::DuidHash::operator()(const Duid& x) const noexcept
{
    uint8_t duidLen = x.GetLength();
    std::vector<uint8_t> buffer = x.GetIdentifier();

    std::string s(buffer.begin(), buffer.begin() + duidLen);
    return std::hash<std::string>{}(s);
}
} // namespace ns3
