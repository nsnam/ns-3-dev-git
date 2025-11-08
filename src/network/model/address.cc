/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modified by: Dhiraj Lokesh <dhirajlokesh@gmail.com>
 */

#include "address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

#include <cstring>
#include <iomanip>
#include <iostream>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Address");

Address::KindTypeRegistry Address::m_typeRegistry;

Address::Address(uint8_t type, const uint8_t* buffer, uint8_t len)
    : m_type(type),
      m_len(len)
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(type) << &buffer << static_cast<uint32_t>(len));
    NS_ASSERT_MSG(m_len <= MAX_SIZE, "Address length too large");
    NS_ASSERT_MSG(type != UNASSIGNED_TYPE, "Type 0 is reserved for uninitialized addresses.");
    std::copy(buffer, buffer + len, m_data.begin());
}

void
Address::SetType(const std::string& kind, uint8_t length)
{
    NS_LOG_FUNCTION(this << kind << static_cast<uint32_t>(length));
    auto search = m_typeRegistry.find({kind, length});
    NS_ASSERT_MSG(search != m_typeRegistry.end(),
                  "No address with the given kind and length is registered.");

    if (m_type != search->second)
    {
        NS_ASSERT_MSG(m_type == UNASSIGNED_TYPE,
                      "You can only change the type of a type-0 address."
                          << static_cast<uint32_t>(m_type));

        m_type = search->second;
    }
}

bool
Address::IsInvalid() const
{
    NS_LOG_FUNCTION(this);
    return m_len == 0 && m_type == UNASSIGNED_TYPE;
}

uint8_t
Address::GetLength() const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_len <= MAX_SIZE);
    return m_len;
}

uint32_t
Address::CopyTo(uint8_t buffer[MAX_SIZE]) const
{
    NS_LOG_FUNCTION(this << &buffer);
    NS_ASSERT(m_len <= MAX_SIZE);
    std::copy(m_data.begin(), m_data.begin() + m_len, buffer);
    return m_len;
}

uint32_t
Address::CopyAllTo(uint8_t* buffer, uint8_t len) const
{
    NS_LOG_FUNCTION(this << &buffer << static_cast<uint32_t>(len));
    NS_ASSERT(len - m_len > 1);
    buffer[0] = m_type;
    buffer[1] = m_len;
    std::copy(m_data.begin(), m_data.begin() + m_len, buffer + 2);
    return m_len + 2;
}

uint32_t
Address::CopyFrom(const uint8_t* buffer, uint8_t len)
{
    NS_LOG_FUNCTION(this << &buffer << static_cast<uint32_t>(len));
    NS_ASSERT_MSG(m_len <= MAX_SIZE, "Address length too large");
    NS_ASSERT_MSG(m_type != UNASSIGNED_TYPE,
                  "Type-0 addresses are reserved. Please use SetType before using CopyFrom.");
    std::copy(buffer, buffer + len, m_data.begin());
    m_len = len;
    return m_len;
}

uint32_t
Address::CopyAllFrom(const uint8_t* buffer, uint8_t len)
{
    NS_LOG_FUNCTION(this << &buffer << static_cast<uint32_t>(len));
    NS_ASSERT(len >= 2);
    m_type = buffer[0];
    m_len = buffer[1];

    NS_ASSERT(len - m_len > 1);
    std::copy(buffer + 2, buffer + 2 + m_len, m_data.begin());
    return m_len + 2;
}

bool
Address::CheckCompatible(uint8_t type, uint8_t len) const
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(type) << static_cast<uint32_t>(len));
    NS_ASSERT(len <= MAX_SIZE);
    return (m_len == len && m_type == type);
}

bool
Address::IsMatchingType(uint8_t type) const
{
    NS_LOG_FUNCTION(this << static_cast<uint32_t>(type));
    return m_type == type;
}

uint8_t
Address::Register(const std::string& kind, uint8_t length)
{
    NS_LOG_FUNCTION(kind << length);
    static uint8_t lastRegisteredType = UNASSIGNED_TYPE;
    NS_ASSERT_MSG(m_typeRegistry.find({kind, length}) == m_typeRegistry.end(),
                  "An address of the same kind and length is already registered.");
    lastRegisteredType++;
    m_typeRegistry[{kind, length}] = lastRegisteredType;
    return lastRegisteredType;
}

uint32_t
Address::GetSerializedSize() const
{
    NS_LOG_FUNCTION(this);
    return 1 + 1 + m_len;
}

void
Address::Serialize(TagBuffer buffer) const
{
    NS_LOG_FUNCTION(this << &buffer);
    buffer.WriteU8(m_type);
    buffer.WriteU8(m_len);
    buffer.Write(m_data.data(), m_len);
}

void
Address::Deserialize(TagBuffer buffer)
{
    NS_LOG_FUNCTION(this << &buffer);
    m_type = buffer.ReadU8();
    m_len = buffer.ReadU8();
    NS_ASSERT(m_len <= MAX_SIZE);
    buffer.Read(m_data.data(), m_len);
}

ATTRIBUTE_HELPER_CPP(Address);

std::ostream&
operator<<(std::ostream& os, const Address& address)
{
    if (address.m_len == 0)
    {
        return os;
    }
    std::ios_base::fmtflags ff = os.flags();
    auto fill = os.fill('0');
    os.setf(std::ios::hex, std::ios::basefield);
    os << std::setw(2) << (uint32_t)address.m_type << "-" << std::setw(2) << (uint32_t)address.m_len
       << "-";
    for (uint8_t i = 0; i < (address.m_len - 1); ++i)
    {
        os << std::setw(2) << (uint32_t)address.m_data[i] << ":";
    }
    // Final byte not suffixed by ":"
    os << std::setw(2) << (uint32_t)address.m_data[address.m_len - 1];
    os.flags(ff); // Restore stream flags
    os.fill(fill);
    return os;
}

std::istream&
operator>>(std::istream& is, Address& address)
{
    std::string v;
    is >> v;
    std::string::size_type firstDash;
    std::string::size_type secondDash;
    firstDash = v.find('-');
    secondDash = v.find('-', firstDash + 1);
    std::string type = v.substr(0, firstDash);
    std::string len = v.substr(firstDash + 1, secondDash - (firstDash + 1));

    address.m_type = std::stoul(type, nullptr, 16);
    address.m_len = std::stoul(len, nullptr, 16);
    NS_ASSERT(address.m_len <= Address::MAX_SIZE);

    std::string::size_type col = secondDash + 1;
    for (uint8_t i = 0; i < address.m_len; ++i)
    {
        std::string tmp;
        std::string::size_type next;
        next = v.find(':', col);
        if (next == std::string::npos)
        {
            tmp = v.substr(col, v.size() - col);
            address.m_data[i] = std::stoul(tmp, nullptr, 16);
            break;
        }
        else
        {
            tmp = v.substr(col, next - col);
            address.m_data[i] = std::stoul(tmp, nullptr, 16);
            col = next + 1;
        }
    }
    return is;
}

} // namespace ns3
