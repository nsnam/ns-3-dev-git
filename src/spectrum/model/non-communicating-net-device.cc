/*
 * Copyright (c) 2010 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "non-communicating-net-device.h"

#include "ns3/boolean.h"
#include "ns3/channel.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NonCommunicatingNetDevice");

NS_OBJECT_ENSURE_REGISTERED(NonCommunicatingNetDevice);

TypeId
NonCommunicatingNetDevice::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NonCommunicatingNetDevice")
                            .SetParent<NetDevice>()
                            .SetGroupName("Spectrum")
                            .AddConstructor<NonCommunicatingNetDevice>()
                            .AddAttribute("Phy",
                                          "The PHY layer attached to this device.",
                                          PointerValue(),
                                          MakePointerAccessor(&NonCommunicatingNetDevice::GetPhy,
                                                              &NonCommunicatingNetDevice::SetPhy),
                                          MakePointerChecker<Object>());
    return tid;
}

NonCommunicatingNetDevice::NonCommunicatingNetDevice()
{
    NS_LOG_FUNCTION(this);
}

NonCommunicatingNetDevice::~NonCommunicatingNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
NonCommunicatingNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_node = nullptr;
    m_channel = nullptr;
    m_phy = nullptr;
    NetDevice::DoDispose();
}

void
NonCommunicatingNetDevice::SetIfIndex(const uint32_t index)
{
    NS_LOG_FUNCTION(index);
    m_ifIndex = index;
}

uint32_t
NonCommunicatingNetDevice::GetIfIndex() const
{
    NS_LOG_FUNCTION(this);
    return m_ifIndex;
}

bool
NonCommunicatingNetDevice::SetMtu(uint16_t mtu)
{
    NS_LOG_FUNCTION(mtu);
    return (mtu == 0);
}

uint16_t
NonCommunicatingNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION(this);
    return 0;
}

void
NonCommunicatingNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this);
}

Address
NonCommunicatingNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION(this);
    return Address();
}

bool
NonCommunicatingNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

Address
NonCommunicatingNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return Address();
}

bool
NonCommunicatingNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

Address
NonCommunicatingNetDevice::GetMulticast(Ipv4Address addr) const
{
    NS_LOG_FUNCTION(addr);
    return Address();
}

Address
NonCommunicatingNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(addr);
    return Address();
}

bool
NonCommunicatingNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
NonCommunicatingNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

Ptr<Node>
NonCommunicatingNetDevice::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
NonCommunicatingNetDevice::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(node);

    m_node = node;
}

void
NonCommunicatingNetDevice::SetPhy(Ptr<Object> phy)
{
    NS_LOG_FUNCTION(this << phy);
    m_phy = phy;
}

Ptr<Object>
NonCommunicatingNetDevice::GetPhy() const
{
    NS_LOG_FUNCTION(this);
    return m_phy;
}

void
NonCommunicatingNetDevice::SetChannel(Ptr<Channel> c)
{
    NS_LOG_FUNCTION(this << c);
    m_channel = c;
}

Ptr<Channel>
NonCommunicatingNetDevice::GetChannel() const
{
    NS_LOG_FUNCTION(this);
    return m_channel;
}

bool
NonCommunicatingNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
NonCommunicatingNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

void
NonCommunicatingNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(&callback);
}

void
NonCommunicatingNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    NS_LOG_FUNCTION(&cb);
}

void
NonCommunicatingNetDevice::SetPromiscReceiveCallback(NetDevice::PromiscReceiveCallback cb)
{
    NS_LOG_FUNCTION(&cb);
}

bool
NonCommunicatingNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
NonCommunicatingNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(packet << dest << protocolNumber);
    return false;
}

bool
NonCommunicatingNetDevice::SendFrom(Ptr<Packet> packet,
                                    const Address& src,
                                    const Address& dest,
                                    uint16_t protocolNumber)
{
    NS_LOG_FUNCTION(packet << src << dest << protocolNumber);
    return false;
}

} // namespace ns3
