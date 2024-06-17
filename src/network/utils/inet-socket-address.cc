/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "inet-socket-address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("InetSocketAddress");

InetSocketAddress::InetSocketAddress(Ipv4Address ipv4, uint16_t port)
    : m_ipv4(ipv4),
      m_port(port)
{
    NS_LOG_FUNCTION(this << ipv4 << port);
}

InetSocketAddress::InetSocketAddress(Ipv4Address ipv4)
    : m_ipv4(ipv4),
      m_port(0)
{
    NS_LOG_FUNCTION(this << ipv4);
}

InetSocketAddress::InetSocketAddress(const char* ipv4, uint16_t port)
    : m_ipv4(Ipv4Address(ipv4)),
      m_port(port)
{
    NS_LOG_FUNCTION(this << ipv4 << port);
}

InetSocketAddress::InetSocketAddress(const char* ipv4)
    : m_ipv4(Ipv4Address(ipv4)),
      m_port(0)
{
    NS_LOG_FUNCTION(this << ipv4);
}

InetSocketAddress::InetSocketAddress(uint16_t port)
    : m_ipv4(Ipv4Address::GetAny()),
      m_port(port)
{
    NS_LOG_FUNCTION(this << port);
}

uint16_t
InetSocketAddress::GetPort() const
{
    NS_LOG_FUNCTION(this);
    return m_port;
}

Ipv4Address
InetSocketAddress::GetIpv4() const
{
    NS_LOG_FUNCTION(this);
    return m_ipv4;
}

void
InetSocketAddress::SetPort(uint16_t port)
{
    NS_LOG_FUNCTION(this << port);
    m_port = port;
}

void
InetSocketAddress::SetIpv4(Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_ipv4 = address;
}

bool
InetSocketAddress::IsMatchingType(const Address& address)
{
    NS_LOG_FUNCTION(&address);
    return address.CheckCompatible(GetType(), 6);
}

InetSocketAddress::operator Address() const
{
    return ConvertTo();
}

Address
InetSocketAddress::ConvertTo() const
{
    NS_LOG_FUNCTION(this);
    uint8_t buf[6];
    m_ipv4.Serialize(buf);
    buf[4] = m_port & 0xff;
    buf[5] = (m_port >> 8) & 0xff;
    return Address(GetType(), buf, 6);
}

InetSocketAddress
InetSocketAddress::ConvertFrom(const Address& address)
{
    NS_LOG_FUNCTION(&address);
    NS_ASSERT(address.CheckCompatible(GetType(), 6)); /* 4 (address) + 2  (port) */
    uint8_t buf[6];
    address.CopyTo(buf);
    Ipv4Address ipv4 = Ipv4Address::Deserialize(buf);
    uint16_t port = buf[4] | (buf[5] << 8);
    InetSocketAddress inet(ipv4, port);
    return inet;
}

uint8_t
InetSocketAddress::GetType()
{
    NS_LOG_FUNCTION_NOARGS();
    static uint8_t type = Address::Register();
    return type;
}

} // namespace ns3
