/*
 * Copyright (c) 2007-2009 Strasbourg University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#include "ipv6-end-point.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv6EndPoint");

Ipv6EndPoint::Ipv6EndPoint(Ipv6Address addr, uint16_t port)
    : m_localAddr(addr),
      m_localPort(port),
      m_peerAddr(Ipv6Address::GetAny()),
      m_peerPort(0),
      m_rxEnabled(true)
{
}

Ipv6EndPoint::~Ipv6EndPoint()
{
    if (!m_destroyCallback.IsNull())
    {
        m_destroyCallback();
    }
    m_rxCallback.Nullify();
    m_icmpCallback.Nullify();
    m_destroyCallback.Nullify();
}

Ipv6Address
Ipv6EndPoint::GetLocalAddress() const
{
    return m_localAddr;
}

void
Ipv6EndPoint::SetLocalAddress(Ipv6Address addr)
{
    m_localAddr = addr;
}

uint16_t
Ipv6EndPoint::GetLocalPort() const
{
    return m_localPort;
}

void
Ipv6EndPoint::SetLocalPort(uint16_t port)
{
    m_localPort = port;
}

Ipv6Address
Ipv6EndPoint::GetPeerAddress() const
{
    return m_peerAddr;
}

uint16_t
Ipv6EndPoint::GetPeerPort() const
{
    return m_peerPort;
}

void
Ipv6EndPoint::BindToNetDevice(Ptr<NetDevice> netdevice)
{
    m_boundnetdevice = netdevice;
}

Ptr<NetDevice>
Ipv6EndPoint::GetBoundNetDevice() const
{
    return m_boundnetdevice;
}

void
Ipv6EndPoint::SetPeer(Ipv6Address addr, uint16_t port)
{
    m_peerAddr = addr;
    m_peerPort = port;
}

void
Ipv6EndPoint::SetRxCallback(
    Callback<void, Ptr<Packet>, Ipv6Header, uint16_t, Ptr<Ipv6Interface>> callback)
{
    m_rxCallback = callback;
}

void
Ipv6EndPoint::SetIcmpCallback(
    Callback<void, Ipv6Address, uint8_t, uint8_t, uint8_t, uint32_t> callback)
{
    m_icmpCallback = callback;
}

void
Ipv6EndPoint::SetDestroyCallback(Callback<void> callback)
{
    m_destroyCallback = callback;
}

void
Ipv6EndPoint::ForwardUp(Ptr<Packet> p,
                        Ipv6Header header,
                        uint16_t port,
                        Ptr<Ipv6Interface> incomingInterface)
{
    if (!m_rxCallback.IsNull())
    {
        m_rxCallback(p, header, port, incomingInterface);
    }
}

void
Ipv6EndPoint::ForwardIcmp(Ipv6Address src, uint8_t ttl, uint8_t type, uint8_t code, uint32_t info)
{
    if (!m_icmpCallback.IsNull())
    {
        m_icmpCallback(src, ttl, type, code, info);
    }
}

void
Ipv6EndPoint::SetRxEnabled(bool enabled)
{
    m_rxEnabled = enabled;
}

bool
Ipv6EndPoint::IsRxEnabled() const
{
    return m_rxEnabled;
}

} /* namespace ns3 */
