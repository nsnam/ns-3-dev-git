/*
 * Copyright (c) 2007-2008 Louis Pasteur University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include "inet6-socket-address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Inet6SocketAddress");

Inet6SocketAddress::Inet6SocketAddress(Ipv6Address ipv6, uint16_t port)
    : m_ipv6(ipv6),
      m_port(port)
{
    NS_LOG_FUNCTION(this << ipv6 << port);
}

Inet6SocketAddress::Inet6SocketAddress(Ipv6Address ipv6)
    : m_ipv6(ipv6),
      m_port(0)
{
    NS_LOG_FUNCTION(this << ipv6);
}

Inet6SocketAddress::Inet6SocketAddress(const char* ipv6, uint16_t port)
    : m_ipv6(Ipv6Address(ipv6)),
      m_port(port)
{
    NS_LOG_FUNCTION(this << ipv6 << port);
}

Inet6SocketAddress::Inet6SocketAddress(const char* ipv6)
    : m_ipv6(Ipv6Address(ipv6)),
      m_port(0)
{
    NS_LOG_FUNCTION(this << ipv6);
}

Inet6SocketAddress::Inet6SocketAddress(uint16_t port)
    : m_ipv6(Ipv6Address::GetAny()),
      m_port(port)
{
    NS_LOG_FUNCTION(this << port);
}

uint16_t
Inet6SocketAddress::GetPort() const
{
    NS_LOG_FUNCTION(this);
    return m_port;
}

void
Inet6SocketAddress::SetPort(uint16_t port)
{
    NS_LOG_FUNCTION(this << port);
    m_port = port;
}

Ipv6Address
Inet6SocketAddress::GetIpv6() const
{
    NS_LOG_FUNCTION(this);
    return m_ipv6;
}

void
Inet6SocketAddress::SetIpv6(Ipv6Address ipv6)
{
    NS_LOG_FUNCTION(this << ipv6);
    m_ipv6 = ipv6;
}

bool
Inet6SocketAddress::IsMatchingType(const Address& addr)
{
    NS_LOG_FUNCTION(&addr);
    return addr.CheckCompatible(GetType(), 18); /* 16 (address) + 2  (port) */
}

Inet6SocketAddress::operator Address() const
{
    return ConvertTo();
}

Address
Inet6SocketAddress::ConvertTo() const
{
    NS_LOG_FUNCTION(this);
    uint8_t buf[18];
    m_ipv6.Serialize(buf);
    buf[16] = m_port & 0xff;
    buf[17] = (m_port >> 8) & 0xff;
    return Address(GetType(), buf, 18);
}

Inet6SocketAddress
Inet6SocketAddress::ConvertFrom(const Address& addr)
{
    NS_LOG_FUNCTION(&addr);
    NS_ASSERT(addr.CheckCompatible(GetType(), 18));
    uint8_t buf[18];
    addr.CopyTo(buf);
    Ipv6Address ipv6 = Ipv6Address::Deserialize(buf);
    uint16_t port = buf[16] | (buf[17] << 8);
    return Inet6SocketAddress(ipv6, port);
}

uint8_t
Inet6SocketAddress::GetType()
{
    NS_LOG_FUNCTION_NOARGS();
    static uint8_t type = Address::Register();
    return type;
}

} /* namespace ns3 */
