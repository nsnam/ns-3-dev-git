/*
 * Copyright (c) 2007 INRIA
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "mac16-address.h"

#include "ns3/address.h"
#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <cstring>
#include <iomanip>
#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Mac16Address");

ATTRIBUTE_HELPER_CPP(Mac16Address);

uint64_t Mac16Address::m_allocationIndex = 0;

Mac16Address::Mac16Address(const char* str)
{
    NS_LOG_FUNCTION(this << str);
    NS_ASSERT_MSG(strlen(str) <= 5, "Mac16Address: illegal string (too long) " << str);

    unsigned int bytes[2];
    int charsRead = 0;

    int i = sscanf(str, "%02x:%02x%n", bytes, bytes + 1, &charsRead);
    NS_ASSERT_MSG(i == 2 && !str[charsRead], "Mac16Address: illegal string " << str);

    std::copy(std::begin(bytes), std::end(bytes), std::begin(m_address));
}

Mac16Address::Mac16Address(uint16_t addr)
{
    NS_LOG_FUNCTION(this);
    m_address[1] = addr & 0xFF;
    m_address[0] = (addr >> 8) & 0xFF;
}

void
Mac16Address::CopyFrom(const uint8_t buffer[2])
{
    NS_LOG_FUNCTION(this << &buffer);
    memcpy(m_address, buffer, 2);
}

void
Mac16Address::CopyTo(uint8_t buffer[2]) const
{
    NS_LOG_FUNCTION(this << &buffer);
    memcpy(buffer, m_address, 2);
}

bool
Mac16Address::IsMatchingType(const Address& address)
{
    NS_LOG_FUNCTION(&address);
    return address.CheckCompatible(GetType(), 2);
}

Mac16Address::operator Address() const
{
    return ConvertTo();
}

Mac16Address
Mac16Address::ConvertFrom(const Address& address)
{
    NS_LOG_FUNCTION(address);
    NS_ASSERT(address.CheckCompatible(GetType(), 2));
    Mac16Address retval;
    address.CopyTo(retval.m_address);
    return retval;
}

Address
Mac16Address::ConvertTo() const
{
    NS_LOG_FUNCTION(this);
    return Address(GetType(), m_address, 2);
}

uint16_t
Mac16Address::ConvertToInt() const
{
    uint16_t addr = m_address[1] & (0xFF);
    addr |= (m_address[0] << 8) & (0xFF << 8);

    return addr;
}

Mac16Address
Mac16Address::Allocate()
{
    NS_LOG_FUNCTION_NOARGS();

    if (m_allocationIndex == 0)
    {
        Simulator::ScheduleDestroy(Mac16Address::ResetAllocationIndex);
    }

    m_allocationIndex++;
    Mac16Address address;
    address.m_address[0] = (m_allocationIndex >> 8) & 0xff;
    address.m_address[1] = m_allocationIndex & 0xff;
    return address;
}

void
Mac16Address::ResetAllocationIndex()
{
    NS_LOG_FUNCTION_NOARGS();
    m_allocationIndex = 0;
}

uint8_t
Mac16Address::GetType()
{
    NS_LOG_FUNCTION_NOARGS();

    static uint8_t type = Address::Register();
    return type;
}

Mac16Address
Mac16Address::GetBroadcast()
{
    NS_LOG_FUNCTION_NOARGS();

    static Mac16Address broadcast("ff:ff");
    return broadcast;
}

Mac16Address
Mac16Address::GetMulticast(Ipv6Address address)
{
    NS_LOG_FUNCTION(address);

    uint8_t ipv6AddrBuf[16];
    address.GetBytes(ipv6AddrBuf);

    uint8_t addrBuf[2];

    addrBuf[0] = 0x80 | (ipv6AddrBuf[14] & 0x1F);
    addrBuf[1] = ipv6AddrBuf[15];

    Mac16Address multicastAddr;
    multicastAddr.CopyFrom(addrBuf);

    return multicastAddr;
}

bool
Mac16Address::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return m_address[0] == 0xff && m_address[1] == 0xff;
}

bool
Mac16Address::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    uint8_t val = m_address[0];
    val >>= 5;
    return val == 0x4;
}

std::ostream&
operator<<(std::ostream& os, const Mac16Address& address)
{
    uint8_t ad[2];
    address.CopyTo(ad);

    os.setf(std::ios::hex, std::ios::basefield);
    os.fill('0');
    for (uint8_t i = 0; i < 1; i++)
    {
        os << std::setw(2) << (uint32_t)ad[i] << ":";
    }
    // Final byte not suffixed by ":"
    os << std::setw(2) << (uint32_t)ad[1];
    os.setf(std::ios::dec, std::ios::basefield);
    os.fill(' ');
    return os;
}

std::istream&
operator>>(std::istream& is, Mac16Address& address)
{
    std::string v;
    is >> v;

    std::string::size_type col = 0;
    for (uint8_t i = 0; i < 2; ++i)
    {
        std::string tmp;
        std::string::size_type next;
        next = v.find(':', col);
        if (next == std::string::npos)
        {
            tmp = v.substr(col, v.size() - col);
            address.m_address[i] = std::stoul(tmp, nullptr, 16);
            break;
        }
        else
        {
            tmp = v.substr(col, next - col);
            address.m_address[i] = std::stoul(tmp, nullptr, 16);
            col = next + 1;
        }
    }
    return is;
}

} // namespace ns3
