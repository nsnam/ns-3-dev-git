/*
 * Copyright (c) 2005,2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ipv4-interface.h"

#include "arp-cache.h"
#include "arp-l3-protocol.h"
#include "ipv4-l3-protocol.h"
#include "ipv4-queue-disc-item.h"
#include "loopback-net-device.h"

#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/traffic-control-layer.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Ipv4Interface");

NS_OBJECT_ENSURE_REGISTERED(Ipv4Interface);

TypeId
Ipv4Interface::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Ipv4Interface")
                            .SetParent<Object>()
                            .SetGroupName("Internet")
                            .AddAttribute("ArpCache",
                                          "The arp cache for this ipv4 interface",
                                          PointerValue(nullptr),
                                          MakePointerAccessor(&Ipv4Interface::SetArpCache,
                                                              &Ipv4Interface::GetArpCache),
                                          MakePointerChecker<ArpCache>());
    ;
    return tid;
}

/**
 * By default, Ipv4 interface are created in the "down" state
 *  with no IP addresses.  Before becoming usable, the user must
 * invoke SetUp on them once an Ipv4 address and mask have been set.
 */
Ipv4Interface::Ipv4Interface()
    : m_ifup(false),
      m_forwarding(true),
      m_metric(1),
      m_node(nullptr),
      m_device(nullptr),
      m_tc(nullptr),
      m_cache(nullptr)
{
    NS_LOG_FUNCTION(this);
}

Ipv4Interface::~Ipv4Interface()
{
    NS_LOG_FUNCTION(this);
}

void
Ipv4Interface::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_node = nullptr;
    m_device = nullptr;
    m_tc = nullptr;
    m_cache = nullptr;
    Object::DoDispose();
}

void
Ipv4Interface::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this << node);
    m_node = node;
    DoSetup();
}

void
Ipv4Interface::SetDevice(Ptr<NetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    m_device = device;
    DoSetup();
}

void
Ipv4Interface::SetTrafficControl(Ptr<TrafficControlLayer> tc)
{
    NS_LOG_FUNCTION(this << tc);
    m_tc = tc;
}

void
Ipv4Interface::DoSetup()
{
    NS_LOG_FUNCTION(this);
    if (!m_node || !m_device)
    {
        return;
    }
    if (!m_device->NeedsArp())
    {
        return;
    }
    Ptr<ArpL3Protocol> arp = m_node->GetObject<ArpL3Protocol>();
    m_cache = arp->CreateCache(m_device, this);
}

Ptr<NetDevice>
Ipv4Interface::GetDevice() const
{
    NS_LOG_FUNCTION(this);
    return m_device;
}

void
Ipv4Interface::SetMetric(uint16_t metric)
{
    NS_LOG_FUNCTION(this << metric);
    m_metric = metric;
}

uint16_t
Ipv4Interface::GetMetric() const
{
    NS_LOG_FUNCTION(this);
    return m_metric;
}

void
Ipv4Interface::SetArpCache(Ptr<ArpCache> a)
{
    NS_LOG_FUNCTION(this << a);
    m_cache = a;
}

Ptr<ArpCache>
Ipv4Interface::GetArpCache() const
{
    NS_LOG_FUNCTION(this);
    return m_cache;
}

/**
 * These are IP interface states and may be distinct from
 * NetDevice states, such as found in real implementations
 * (where the device may be down but IP interface state is still up).
 */
bool
Ipv4Interface::IsUp() const
{
    NS_LOG_FUNCTION(this);
    return m_ifup;
}

bool
Ipv4Interface::IsDown() const
{
    NS_LOG_FUNCTION(this);
    return !m_ifup;
}

void
Ipv4Interface::SetUp()
{
    NS_LOG_FUNCTION(this);
    m_ifup = true;
}

void
Ipv4Interface::SetDown()
{
    NS_LOG_FUNCTION(this);
    m_ifup = false;
}

bool
Ipv4Interface::IsForwarding() const
{
    NS_LOG_FUNCTION(this);
    return m_forwarding;
}

void
Ipv4Interface::SetForwarding(bool val)
{
    NS_LOG_FUNCTION(this << val);
    m_forwarding = val;
}

void
Ipv4Interface::Send(Ptr<Packet> p, const Ipv4Header& hdr, Ipv4Address dest)
{
    NS_LOG_FUNCTION(this << *p << dest);
    if (!IsUp())
    {
        return;
    }

    // Check for a loopback device, if it's the case we don't pass through
    // traffic control layer
    if (DynamicCast<LoopbackNetDevice>(m_device))
    {
        /// @todo additional checks needed here (such as whether multicast
        /// goes to loopback)?
        p->AddHeader(hdr);
        m_device->Send(p, m_device->GetBroadcast(), Ipv4L3Protocol::PROT_NUMBER);
        return;
    }

    NS_ASSERT(m_tc);

    // is this packet aimed at a local interface ?
    for (auto i = m_ifaddrs.begin(); i != m_ifaddrs.end(); ++i)
    {
        if (dest == (*i).GetLocal())
        {
            p->AddHeader(hdr);
            Simulator::ScheduleNow(&TrafficControlLayer::Receive,
                                   m_tc,
                                   m_device,
                                   p,
                                   Ipv4L3Protocol::PROT_NUMBER,
                                   m_device->GetBroadcast(),
                                   m_device->GetBroadcast(),
                                   NetDevice::PACKET_HOST);
            return;
        }
    }
    if (m_device->NeedsArp())
    {
        NS_LOG_LOGIC("Needs ARP " << dest);
        Ptr<ArpL3Protocol> arp = m_node->GetObject<ArpL3Protocol>();
        Address hardwareDestination;
        bool found = false;
        if (dest.IsBroadcast())
        {
            NS_LOG_LOGIC("All-network Broadcast");
            hardwareDestination = m_device->GetBroadcast();
            found = true;
        }
        else if (dest.IsMulticast())
        {
            NS_LOG_LOGIC("IsMulticast");
            NS_ASSERT_MSG(m_device->IsMulticast(),
                          "ArpIpv4Interface::SendTo (): Sending multicast packet over "
                          "non-multicast device");

            hardwareDestination = m_device->GetMulticast(dest);
            found = true;
        }
        else
        {
            for (auto i = m_ifaddrs.begin(); i != m_ifaddrs.end(); ++i)
            {
                if (dest.IsSubnetDirectedBroadcast((*i).GetMask()))
                {
                    NS_LOG_LOGIC("Subnetwork Broadcast");
                    hardwareDestination = m_device->GetBroadcast();
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                NS_LOG_LOGIC("ARP Lookup");
                found = arp->Lookup(p, hdr, dest, m_device, m_cache, &hardwareDestination);
            }
        }

        if (found)
        {
            NS_LOG_LOGIC("Address Resolved.  Send.");
            m_tc->Send(m_device,
                       Create<Ipv4QueueDiscItem>(p,
                                                 hardwareDestination,
                                                 Ipv4L3Protocol::PROT_NUMBER,
                                                 hdr));
        }
    }
    else
    {
        NS_LOG_LOGIC("Doesn't need ARP");
        m_tc->Send(m_device,
                   Create<Ipv4QueueDiscItem>(p,
                                             m_device->GetBroadcast(),
                                             Ipv4L3Protocol::PROT_NUMBER,
                                             hdr));
    }
}

uint32_t
Ipv4Interface::GetNAddresses() const
{
    NS_LOG_FUNCTION(this);
    return m_ifaddrs.size();
}

bool
Ipv4Interface::AddAddress(Ipv4InterfaceAddress addr)
{
    NS_LOG_FUNCTION(this << addr);
    m_ifaddrs.push_back(addr);
    if (!m_addAddressCallback.IsNull())
    {
        m_addAddressCallback(this, addr);
    }
    return true;
}

Ipv4InterfaceAddress
Ipv4Interface::GetAddress(uint32_t index) const
{
    NS_LOG_FUNCTION(this << index);
    if (index >= m_ifaddrs.size())
    {
        NS_FATAL_ERROR("index " << index << " out of bounds");
    }

    uint32_t tmp = 0;
    for (auto i = m_ifaddrs.begin(); i != m_ifaddrs.end(); i++)
    {
        if (tmp == index)
        {
            return *i;
        }
        ++tmp;
    }

    return {}; // quiet compiler
}

Ipv4InterfaceAddress
Ipv4Interface::RemoveAddress(uint32_t index)
{
    NS_LOG_FUNCTION(this << index);
    if (index >= m_ifaddrs.size())
    {
        NS_FATAL_ERROR("Bug in Ipv4Interface::RemoveAddress");
    }

    uint32_t tmp = 0;
    for (auto it = m_ifaddrs.begin(); it != m_ifaddrs.end(); it++)
    {
        if (tmp == index)
        {
            // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
            Ipv4InterfaceAddress addr = *it;

            m_ifaddrs.erase(it);
            if (!m_removeAddressCallback.IsNull())
            {
                m_removeAddressCallback(this, addr);
            }
            return addr;
        }
        ++tmp;
    }
    NS_FATAL_ERROR("Address " << index << " not found");
    return {}; // quiet compiler
}

Ipv4InterfaceAddress
Ipv4Interface::RemoveAddress(Ipv4Address address)
{
    NS_LOG_FUNCTION(this << address);

    if (address == Ipv4Address::GetLoopback())
    {
        NS_LOG_WARN("Cannot remove loopback address.");
        return Ipv4InterfaceAddress();
    }

    for (auto it = m_ifaddrs.begin(); it != m_ifaddrs.end(); it++)
    {
        if ((*it).GetLocal() == address)
        {
            // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
            Ipv4InterfaceAddress ifAddr = *it;

            m_ifaddrs.erase(it);
            if (!m_removeAddressCallback.IsNull())
            {
                m_removeAddressCallback(this, ifAddr);
            }
            return ifAddr;
        }
    }
    return {};
}

void
Ipv4Interface::RemoveAddressCallback(
    Callback<void, Ptr<Ipv4Interface>, Ipv4InterfaceAddress> removeAddressCallback)
{
    NS_LOG_FUNCTION(this << &removeAddressCallback);
    m_removeAddressCallback = removeAddressCallback;
}

void
Ipv4Interface::AddAddressCallback(
    Callback<void, Ptr<Ipv4Interface>, Ipv4InterfaceAddress> addAddressCallback)
{
    NS_LOG_FUNCTION(this << &addAddressCallback);
    m_addAddressCallback = addAddressCallback;
}

} // namespace ns3
