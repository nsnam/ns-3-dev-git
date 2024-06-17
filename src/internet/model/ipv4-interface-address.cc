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
    : m_scope(GLOBAL),
      m_secondary(false)
{
    NS_LOG_FUNCTION(this);
}

Ipv4InterfaceAddress::Ipv4InterfaceAddress(Ipv4Address local, Ipv4Mask mask)
    : m_scope(GLOBAL),
      m_secondary(false)
{
    NS_LOG_FUNCTION(this << local << mask);
    m_local = local;
    if (m_local == Ipv4Address::GetLoopback())
    {
        m_scope = HOST;
    }
    m_mask = mask;
    m_broadcast = Ipv4Address(local.Get() | (~mask.Get()));
}

Ipv4InterfaceAddress::Ipv4InterfaceAddress(const Ipv4InterfaceAddress& o)
    : m_local(o.m_local),
      m_mask(o.m_mask),
      m_broadcast(o.m_broadcast),
      m_scope(o.m_scope),
      m_secondary(o.m_secondary)
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
    m_mask = mask;
}

Ipv4Mask
Ipv4InterfaceAddress::GetMask() const
{
    NS_LOG_FUNCTION(this);
    return m_mask;
}

void
Ipv4InterfaceAddress::SetBroadcast(Ipv4Address broadcast)
{
    NS_LOG_FUNCTION(this << broadcast);
    m_broadcast = broadcast;
}

Ipv4Address
Ipv4InterfaceAddress::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return m_broadcast;
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
    Ipv4Address aAddr = m_local;
    aAddr = aAddr.CombineMask(m_mask);
    Ipv4Address bAddr = b;
    bAddr = bAddr.CombineMask(m_mask);

    return (aAddr == bAddr);
}

bool
Ipv4InterfaceAddress::IsSecondary() const
{
    NS_LOG_FUNCTION(this);
    return m_secondary;
}

void
Ipv4InterfaceAddress::SetSecondary()
{
    NS_LOG_FUNCTION(this);
    m_secondary = true;
}

void
Ipv4InterfaceAddress::SetPrimary()
{
    NS_LOG_FUNCTION(this);
    m_secondary = false;
}

std::ostream&
operator<<(std::ostream& os, const Ipv4InterfaceAddress& addr)
{
    os << "m_local=" << addr.GetLocal() << "; m_mask=" << addr.GetMask()
       << "; m_broadcast=" << addr.GetBroadcast() << "; m_scope=" << addr.GetScope()
       << "; m_secondary=" << addr.IsSecondary();
    return os;
}

} // namespace ns3
