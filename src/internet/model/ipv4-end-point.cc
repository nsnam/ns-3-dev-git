/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv4-end-point.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4EndPoint");

Ipv4EndPoint::Ipv4EndPoint(Ipv4Address address, uint16_t port)
    : m_localAddr(address),
      m_localPort(port),
      m_peerAddr(Ipv4Address::GetAny()),
      m_peerPort(0),
      m_rxEnabled(true)
{
    NS_LOG_FUNCTION(this << address << port);
}

Ipv4EndPoint::~Ipv4EndPoint()
{
    NS_LOG_FUNCTION(this);
    if (!m_destroyCallback.IsNull())
    {
        m_destroyCallback();
    }
    m_rxCallback.Nullify();
    m_icmpCallback.Nullify();
    m_destroyCallback.Nullify();
}

Ipv4Address
Ipv4EndPoint::GetLocalAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_localAddr;
}

void
Ipv4EndPoint::SetLocalAddress(Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);
    m_localAddr = address;
}

uint16_t
Ipv4EndPoint::GetLocalPort() const
{
    NS_LOG_FUNCTION(this);
    return m_localPort;
}

Ipv4Address
Ipv4EndPoint::GetPeerAddress() const
{
    NS_LOG_FUNCTION(this);
    return m_peerAddr;
}

uint16_t
Ipv4EndPoint::GetPeerPort() const
{
    NS_LOG_FUNCTION(this);
    return m_peerPort;
}

void
Ipv4EndPoint::SetPeer(Ipv4Address address, uint16_t port)
{
    NS_LOG_FUNCTION(this << address << port);
    m_peerAddr = address;
    m_peerPort = port;
}

void
Ipv4EndPoint::BindToNetDevice(Ptr<NetDevice> netdevice)
{
    NS_LOG_FUNCTION(this << netdevice);
    m_boundnetdevice = netdevice;
}

Ptr<NetDevice>
Ipv4EndPoint::GetBoundNetDevice() const
{
    NS_LOG_FUNCTION(this);
    return m_boundnetdevice;
}

void
Ipv4EndPoint::SetRxCallback(
    Callback<void, Ptr<Packet>, Ipv4Header, uint16_t, Ptr<Ipv4Interface>> callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_rxCallback = callback;
}

void
Ipv4EndPoint::SetIcmpCallback(
    Callback<void, Ipv4Address, uint8_t, uint8_t, uint8_t, uint32_t> callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_icmpCallback = callback;
}

void
Ipv4EndPoint::SetDestroyCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(this << &callback);
    m_destroyCallback = callback;
}

void
Ipv4EndPoint::ForwardUp(Ptr<Packet> p,
                        const Ipv4Header& header,
                        uint16_t sport,
                        Ptr<Ipv4Interface> incomingInterface)
{
    NS_LOG_FUNCTION(this << p << &header << sport << incomingInterface);

    if (!m_rxCallback.IsNull())
    {
        m_rxCallback(p, header, sport, incomingInterface);
    }
}

void
Ipv4EndPoint::ForwardIcmp(Ipv4Address icmpSource,
                          uint8_t icmpTtl,
                          uint8_t icmpType,
                          uint8_t icmpCode,
                          uint32_t icmpInfo)
{
    NS_LOG_FUNCTION(this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType
                         << (uint32_t)icmpCode << icmpInfo);
    if (!m_icmpCallback.IsNull())
    {
        m_icmpCallback(icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}

void
Ipv4EndPoint::SetRxEnabled(bool enabled)
{
    m_rxEnabled = enabled;
}

bool
Ipv4EndPoint::IsRxEnabled() const
{
    return m_rxEnabled;
}

} // namespace ns3
