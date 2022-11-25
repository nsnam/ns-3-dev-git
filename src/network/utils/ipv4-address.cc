/*
 * Copyright (c) 2005 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv4-address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <cstdlib>

#ifdef __WIN32__
#include <WS2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4Address");

Ipv4Mask::Ipv4Mask()
    : m_mask(0x66666666)
{
    NS_LOG_FUNCTION(this);
}

Ipv4Mask::Ipv4Mask(uint32_t mask)
    : m_mask(mask)
{
    NS_LOG_FUNCTION(this << mask);
}

Ipv4Mask::Ipv4Mask(const char* mask)
{
    NS_LOG_FUNCTION(this << mask);
    if (*mask == '/')
    {
        uint32_t plen = static_cast<uint32_t>(std::atoi(++mask));
        NS_ASSERT(plen <= 32);
        if (plen > 0)
        {
            m_mask = 0xffffffff << (32 - plen);
        }
        else
        {
            m_mask = 0;
        }
    }
    else
    {
        if (inet_pton(AF_INET, mask, &m_mask) <= 0)
        {
            NS_ABORT_MSG("Error, can not build an IPv4 mask from an invalid string: " << mask);
        }
        m_mask = ntohl(m_mask);
    }
}

bool
Ipv4Mask::IsMatch(Ipv4Address a, Ipv4Address b) const
{
    NS_LOG_FUNCTION(this << a << b);
    if ((a.Get() & m_mask) == (b.Get() & m_mask))
    {
        return true;
    }
    else
    {
        return false;
    }
}

uint32_t
Ipv4Mask::Get() const
{
    NS_LOG_FUNCTION(this);
    return m_mask;
}

void
Ipv4Mask::Set(uint32_t mask)
{
    NS_LOG_FUNCTION(this << mask);
    m_mask = mask;
}

uint32_t
Ipv4Mask::GetInverse() const
{
    NS_LOG_FUNCTION(this);
    return ~m_mask;
}

void
Ipv4Mask::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this << &os);
    os << ((m_mask >> 24) & 0xff) << "." << ((m_mask >> 16) & 0xff) << "." << ((m_mask >> 8) & 0xff)
       << "." << ((m_mask >> 0) & 0xff);
}

Ipv4Mask
Ipv4Mask::GetLoopback()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv4Mask loopback = Ipv4Mask("255.0.0.0");
    return loopback;
}

Ipv4Mask
Ipv4Mask::GetZero()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv4Mask zero = Ipv4Mask("0.0.0.0");
    return zero;
}

Ipv4Mask
Ipv4Mask::GetOnes()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv4Mask ones = Ipv4Mask("255.255.255.255");
    return ones;
}

uint16_t
Ipv4Mask::GetPrefixLength() const
{
    NS_LOG_FUNCTION(this);
    uint16_t tmp = 0;
    uint32_t mask = m_mask;
    while (mask != 0)
    {
        mask = mask << 1;
        tmp++;
    }
    return tmp;
}

/**
 *  Value of a not-yet-initialized IPv4 address, corresponding to 102.102.102.102.
 *  This is totally arbitrary.
 */
static constexpr uint32_t UNINITIALIZED = 0x66666666U;

Ipv4Address::Ipv4Address()
    : m_address(UNINITIALIZED),
      m_initialized(false)
{
    NS_LOG_FUNCTION(this);
}

Ipv4Address::Ipv4Address(uint32_t address)
{
    NS_LOG_FUNCTION(this << address);
    m_address = address;
    m_initialized = true;
}

Ipv4Address::Ipv4Address(const char* address)
{
    NS_LOG_FUNCTION(this << address);

    if (inet_pton(AF_INET, address, &m_address) <= 0)
    {
        NS_LOG_LOGIC("Error, can not build an IPv4 address from an invalid string: " << address);
        m_address = 0;
        m_initialized = false;
        return;
    }
    m_initialized = true;
    m_address = ntohl(m_address);
}

uint32_t
Ipv4Address::Get() const
{
    NS_LOG_FUNCTION(this);
    return m_address;
}

void
Ipv4Address::Set(uint32_t address)
{
    NS_LOG_FUNCTION(this << address);
    m_address = address;
    m_initialized = true;
}

void
Ipv4Address::Set(const char* address)
{
    NS_LOG_FUNCTION(this << address);
    if (inet_pton(AF_INET, address, &m_address) <= 0)
    {
        NS_LOG_LOGIC("Error, can not build an IPv4 address from an invalid string: " << address);
        m_address = 0;
        m_initialized = false;
        return;
    }
    m_initialized = true;
    m_address = ntohl(m_address);
}

Ipv4Address
Ipv4Address::CombineMask(const Ipv4Mask& mask) const
{
    NS_LOG_FUNCTION(this << mask);
    return Ipv4Address(Get() & mask.Get());
}

Ipv4Address
Ipv4Address::GetSubnetDirectedBroadcast(const Ipv4Mask& mask) const
{
    NS_LOG_FUNCTION(this << mask);
    if (mask == Ipv4Mask::GetOnes())
    {
        NS_ASSERT_MSG(false,
                      "Trying to get subnet-directed broadcast address with an all-ones netmask");
    }
    return Ipv4Address(Get() | mask.GetInverse());
}

bool
Ipv4Address::IsSubnetDirectedBroadcast(const Ipv4Mask& mask) const
{
    NS_LOG_FUNCTION(this << mask);
    if (mask == Ipv4Mask::GetOnes())
    {
        // If the mask is 255.255.255.255, there is no subnet directed
        // broadcast for this address.
        return false;
    }
    return ((Get() | mask.Get()) == Ipv4Address::GetBroadcast().Get());
}

bool
Ipv4Address::IsInitialized() const
{
    NS_LOG_FUNCTION(this);
    return (m_initialized);
}

bool
Ipv4Address::IsAny() const
{
    NS_LOG_FUNCTION(this);
    return (m_address == 0x00000000U);
}

bool
Ipv4Address::IsLocalhost() const
{
    NS_LOG_FUNCTION(this);
    return (m_address == 0x7f000001U);
}

bool
Ipv4Address::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return (m_address == 0xffffffffU);
}

bool
Ipv4Address::IsMulticast() const
{
    //
    // Multicast addresses are defined as ranging from 224.0.0.0 through
    // 239.255.255.255 (which is E0000000 through EFFFFFFF in hex).
    //
    NS_LOG_FUNCTION(this);
    return (m_address >= 0xe0000000 && m_address <= 0xefffffff);
}

bool
Ipv4Address::IsLocalMulticast() const
{
    NS_LOG_FUNCTION(this);
    // Link-Local multicast address is 224.0.0.0/24
    return (m_address & 0xffffff00) == 0xe0000000;
}

void
Ipv4Address::Serialize(uint8_t buf[4]) const
{
    NS_LOG_FUNCTION(this << &buf);
    buf[0] = (m_address >> 24) & 0xff;
    buf[1] = (m_address >> 16) & 0xff;
    buf[2] = (m_address >> 8) & 0xff;
    buf[3] = (m_address >> 0) & 0xff;
}

Ipv4Address
Ipv4Address::Deserialize(const uint8_t buf[4])
{
    NS_LOG_FUNCTION(&buf);
    Ipv4Address ipv4;
    ipv4.m_address = 0;
    ipv4.m_address |= buf[0];
    ipv4.m_address <<= 8;
    ipv4.m_address |= buf[1];
    ipv4.m_address <<= 8;
    ipv4.m_address |= buf[2];
    ipv4.m_address <<= 8;
    ipv4.m_address |= buf[3];
    ipv4.m_initialized = true;

    return ipv4;
}

void
Ipv4Address::Print(std::ostream& os) const
{
    NS_LOG_FUNCTION(this);
    os << ((m_address >> 24) & 0xff) << "." << ((m_address >> 16) & 0xff) << "."
       << ((m_address >> 8) & 0xff) << "." << ((m_address >> 0) & 0xff);
}

bool
Ipv4Address::IsMatchingType(const Address& address)
{
    NS_LOG_FUNCTION(&address);
    return address.CheckCompatible(GetType(), 4);
}

Ipv4Address::operator Address() const
{
    return ConvertTo();
}

Address
Ipv4Address::ConvertTo() const
{
    NS_LOG_FUNCTION(this);
    uint8_t buf[4];
    Serialize(buf);
    return Address(GetType(), buf, 4);
}

Ipv4Address
Ipv4Address::ConvertFrom(const Address& address)
{
    NS_LOG_FUNCTION(&address);
    NS_ASSERT(address.CheckCompatible(GetType(), 4));
    uint8_t buf[4];
    address.CopyTo(buf);
    return Deserialize(buf);
}

uint8_t
Ipv4Address::GetType()
{
    NS_LOG_FUNCTION_NOARGS();
    static uint8_t type = Address::Register();
    return type;
}

Ipv4Address
Ipv4Address::GetZero()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv4Address zero("0.0.0.0");
    return zero;
}

Ipv4Address
Ipv4Address::GetAny()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv4Address any("0.0.0.0");
    return any;
}

Ipv4Address
Ipv4Address::GetBroadcast()
{
    NS_LOG_FUNCTION_NOARGS();
    static Ipv4Address broadcast("255.255.255.255");
    return broadcast;
}

Ipv4Address
Ipv4Address::GetLoopback()
{
    NS_LOG_FUNCTION_NOARGS();
    Ipv4Address loopback("127.0.0.1");
    return loopback;
}

size_t
Ipv4AddressHash::operator()(const Ipv4Address& x) const
{
    return std::hash<uint32_t>()(x.Get());
}

std::ostream&
operator<<(std::ostream& os, const Ipv4Address& address)
{
    address.Print(os);
    return os;
}

std::ostream&
operator<<(std::ostream& os, const Ipv4Mask& mask)
{
    mask.Print(os);
    return os;
}

std::istream&
operator>>(std::istream& is, Ipv4Address& address)
{
    std::string str;
    is >> str;
    address = Ipv4Address(str.c_str());
    return is;
}

std::istream&
operator>>(std::istream& is, Ipv4Mask& mask)
{
    std::string str;
    is >> str;
    mask = Ipv4Mask(str.c_str());
    return is;
}

ATTRIBUTE_HELPER_CPP(Ipv4Address);
ATTRIBUTE_HELPER_CPP(Ipv4Mask);

} // namespace ns3
