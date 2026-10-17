/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv4-interface-address.h"

#include "ns3/assert.h"
#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4InterfaceAddress");

Ipv4InterfaceAddress::Ipv4InterfaceAddress()
    : m_scope(GLOBAL)
{
    NS_LOG_FUNCTION(this);
}

Ipv4InterfaceAddress::Ipv4InterfaceAddress(Ipv4Address local, Ipv4Mask mask)
    : m_scope(GLOBAL)
{
    NS_LOG_FUNCTION(this << local << mask);
    m_local = local;
    if (m_local == Ipv4Address::GetLoopback())
    {
        m_scope = HOST;
    }
    m_prefixLength = mask.GetPrefixLength();
}

Ipv4InterfaceAddress::Ipv4InterfaceAddress(const Ipv4InterfaceAddress& o)
    : m_local(o.m_local),
      m_prefixLength(o.m_prefixLength),
      m_scope(o.m_scope)
{
    NS_LOG_FUNCTION(this << &o);
}

void
Ipv4InterfaceAddress::SetLocal(Ipv4Address local)
{
    NS_LOG_FUNCTION(this << local);
    m_local = local;
}

void
Ipv4InterfaceAddress::SetAddress(Ipv4Address address)
{
    SetLocal(address);
}

Ipv4Address
Ipv4InterfaceAddress::GetLocal() const
{
    NS_LOG_FUNCTION(this);
    return m_local;
}

Ipv4Address
Ipv4InterfaceAddress::GetAddress() const
{
    return GetLocal();
}

void
Ipv4InterfaceAddress::SetMask(Ipv4Mask mask)
{
    NS_LOG_FUNCTION(this << mask);
    m_prefixLength = mask.GetPrefixLength();
}

Ipv4Mask
Ipv4InterfaceAddress::GetMask() const
{
    NS_LOG_FUNCTION(this);

    // Do not shift a number by its length.
    // The C++ standard says it's an undefined result.
    if (m_prefixLength == 0)
    {
        return Ipv4Mask::GetZero();
    }

    uint32_t mask = 0xffffffff << (32 - m_prefixLength);
    return Ipv4Mask(mask);
}

Ipv4Address
Ipv4InterfaceAddress::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);

    uint32_t inverseMask = m_prefixLength == 32 ? 0 : 0xffffffff >> m_prefixLength;
    return Ipv4Address(m_local.Get() | inverseMask);
}

void
Ipv4InterfaceAddress::SetScope(Ipv4InterfaceAddress::InterfaceAddressScope_e scope)
{
    NS_LOG_FUNCTION(this << scope);
    m_scope = scope;
}

Ipv4InterfaceAddress::InterfaceAddressScope_e
Ipv4InterfaceAddress::GetScope() const
{
    NS_LOG_FUNCTION(this);
    return m_scope;
}

bool
Ipv4InterfaceAddress::IsInSameSubnet(const Ipv4Address b) const
{
    Ipv4Mask mask = GetMask();

    Ipv4Address aAddr = m_local;
    aAddr = aAddr.CombineMask(mask);
    Ipv4Address bAddr = b;
    bAddr = bAddr.CombineMask(mask);

    return (aAddr == bAddr);
}

std::ostream&
operator<<(std::ostream& os, const Ipv4InterfaceAddress& addr)
{
    os << "m_local=" << addr.GetLocal() << "; m_mask=" << addr.GetMask()
       << "; m_broadcast=" << addr.GetBroadcast() << "; m_scope=" << addr.GetScope();
    return os;
}

} // namespace ns3
