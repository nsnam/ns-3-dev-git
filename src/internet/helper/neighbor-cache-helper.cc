/*
 * Copyright (c) 2022 ZHIHENG DONG
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Zhiheng Dong <dzh2077@gmail.com>
 */

#include "neighbor-cache-helper.h"

#include "ns3/assert.h"
#include "ns3/channel-list.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NeighborCacheHelper");

NeighborCacheHelper::NeighborCacheHelper()
{
    NS_LOG_FUNCTION(this);
}

NeighborCacheHelper::~NeighborCacheHelper()
{
    NS_LOG_FUNCTION(this);
}

void
NeighborCacheHelper::PopulateNeighborCache()
{
    NS_LOG_FUNCTION(this);
    m_globalNeighborCache = true;
    for (uint32_t i = 0; i < ChannelList::GetNChannels(); ++i)
    {
        Ptr<Channel> channel = ChannelList::GetChannel(i);
        PopulateNeighborCache(channel);
    }
}

void
NeighborCacheHelper::PopulateNeighborCache(Ptr<Channel> channel) const
{
    NS_LOG_FUNCTION(this << channel);
    for (std::size_t i = 0; i < channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> netDevice = channel->GetDevice(i);
        Ptr<Node> node = netDevice->GetNode();

        int32_t ipv4InterfaceIndex = -1;
        if (node->GetObject<Ipv4>())
        {
            ipv4InterfaceIndex = node->GetObject<Ipv4>()->GetInterfaceForDevice(netDevice);
        }
        int32_t ipv6InterfaceIndex = -1;
        if (node->GetObject<Ipv6>())
        {
            ipv6InterfaceIndex = node->GetObject<Ipv6>()->GetInterfaceForDevice(netDevice);
        }

        for (std::size_t j = 0; j < channel->GetNDevices(); ++j)
        {
            Ptr<NetDevice> neighborDevice = channel->GetDevice(j);
            Ptr<Node> neighborNode = neighborDevice->GetNode();

            int32_t ipv4NeighborInterfaceIndex = -1;
            if (neighborNode->GetObject<Ipv4>())
            {
                ipv4NeighborInterfaceIndex =
                    neighborNode->GetObject<Ipv4>()->GetInterfaceForDevice(neighborDevice);
            }
            int32_t ipv6NeighborInterfaceIndex = -1;
            if (neighborNode->GetObject<Ipv6>())
            {
                ipv6NeighborInterfaceIndex =
                    neighborNode->GetObject<Ipv6>()->GetInterfaceForDevice(neighborDevice);
            }

            if (neighborDevice != netDevice)
            {
                if (ipv4InterfaceIndex != -1)
                {
                    Ptr<Ipv4Interface> ipv4Interface =
                        node->GetObject<Ipv4L3Protocol>()->GetInterface(ipv4InterfaceIndex);
                    if (ipv4NeighborInterfaceIndex != -1)
                    {
                        Ptr<Ipv4Interface> ipv4NeighborInterface =
                            neighborNode->GetObject<Ipv4L3Protocol>()->GetInterface(
                                ipv4NeighborInterfaceIndex);
                        PopulateNeighborEntriesIpv4(ipv4Interface, ipv4NeighborInterface);
                    }
                }
                if (ipv6InterfaceIndex != -1)
                {
                    Ptr<Ipv6Interface> ipv6Interface =
                        node->GetObject<Ipv6L3Protocol>()->GetInterface(ipv6InterfaceIndex);
                    if (ipv6NeighborInterfaceIndex != -1)
                    {
                        Ptr<Ipv6Interface> ipv6NeighborInterface =
                            neighborNode->GetObject<Ipv6L3Protocol>()->GetInterface(
                                ipv6NeighborInterfaceIndex);
                        PopulateNeighborEntriesIpv6(ipv6Interface, ipv6NeighborInterface);
                    }
                }
            }
        }
    }
}

void
NeighborCacheHelper::PopulateNeighborCache(const NetDeviceContainer& c) const
{
    NS_LOG_FUNCTION(this);
    for (uint32_t i = 0; i < c.GetN(); ++i)
    {
        Ptr<NetDevice> netDevice = c.Get(i);
        Ptr<Channel> channel = netDevice->GetChannel();
        Ptr<Node> node = netDevice->GetNode();

        int32_t ipv4InterfaceIndex = -1;
        if (node->GetObject<Ipv4>())
        {
            ipv4InterfaceIndex = node->GetObject<Ipv4>()->GetInterfaceForDevice(netDevice);
        }
        int32_t ipv6InterfaceIndex = -1;
        if (node->GetObject<Ipv6>())
        {
            ipv6InterfaceIndex = node->GetObject<Ipv6>()->GetInterfaceForDevice(netDevice);
        }

        for (std::size_t j = 0; j < channel->GetNDevices(); ++j)
        {
            Ptr<NetDevice> neighborDevice = channel->GetDevice(j);
            Ptr<Node> neighborNode = neighborDevice->GetNode();

            int32_t ipv4NeighborInterfaceIndex = -1;
            if (neighborNode->GetObject<Ipv4>())
            {
                ipv4NeighborInterfaceIndex =
                    neighborNode->GetObject<Ipv4>()->GetInterfaceForDevice(neighborDevice);
            }
            int32_t ipv6NeighborInterfaceIndex = -1;
            if (neighborNode->GetObject<Ipv6>())
            {
                ipv6NeighborInterfaceIndex =
                    neighborNode->GetObject<Ipv6>()->GetInterfaceForDevice(neighborDevice);
            }

            if (neighborDevice != netDevice)
            {
                if (ipv4InterfaceIndex != -1)
                {
                    Ptr<Ipv4Interface> ipv4Interface =
                        node->GetObject<Ipv4L3Protocol>()->GetInterface(ipv4InterfaceIndex);
                    if (ipv4NeighborInterfaceIndex != -1)
                    {
                        Ptr<Ipv4Interface> ipv4NeighborInterface =
                            neighborNode->GetObject<Ipv4L3Protocol>()->GetInterface(
                                ipv4NeighborInterfaceIndex);
                        PopulateNeighborEntriesIpv4(ipv4Interface, ipv4NeighborInterface);
                    }
                }
                if (ipv6InterfaceIndex != -1)
                {
                    Ptr<Ipv6Interface> ipv6Interface =
                        node->GetObject<Ipv6L3Protocol>()->GetInterface(ipv6InterfaceIndex);
                    if (ipv6NeighborInterfaceIndex != -1)
                    {
                        Ptr<Ipv6Interface> ipv6NeighborInterface =
                            neighborNode->GetObject<Ipv6L3Protocol>()->GetInterface(
                                ipv6NeighborInterfaceIndex);
                        PopulateNeighborEntriesIpv6(ipv6Interface, ipv6NeighborInterface);
                    }
                }
            }
        }
    }
}

void
NeighborCacheHelper::PopulateNeighborCache(const Ipv4InterfaceContainer& c) const
{
    NS_LOG_FUNCTION(this);
    for (uint32_t i = 0; i < c.GetN(); ++i)
    {
        std::pair<Ptr<Ipv4>, uint32_t> returnValue = c.Get(i);
        Ptr<Ipv4> ipv4 = returnValue.first;
        uint32_t index = returnValue.second;
        Ptr<Ipv4Interface> ipv4Interface = DynamicCast<Ipv4L3Protocol>(ipv4)->GetInterface(index);
        if (ipv4Interface)
        {
            Ptr<NetDevice> netDevice = ipv4Interface->GetDevice();
            Ptr<Channel> channel = netDevice->GetChannel();
            for (std::size_t j = 0; j < channel->GetNDevices(); ++j)
            {
                Ptr<NetDevice> neighborDevice = channel->GetDevice(j);
                if (neighborDevice != netDevice)
                {
                    Ptr<Node> neighborNode = neighborDevice->GetNode();
                    int32_t ipv4NeighborInterfaceIndex =
                        neighborNode->GetObject<Ipv4>()->GetInterfaceForDevice(neighborDevice);
                    if (ipv4NeighborInterfaceIndex != -1)
                    {
                        Ptr<Ipv4Interface> ipv4NeighborInterface =
                            neighborNode->GetObject<Ipv4L3Protocol>()->GetInterface(
                                ipv4NeighborInterfaceIndex);
                        PopulateNeighborEntriesIpv4(ipv4Interface, ipv4NeighborInterface);
                    }
                }
            }
        }
    }
}

void
NeighborCacheHelper::PopulateNeighborCache(const Ipv6InterfaceContainer& c) const
{
    NS_LOG_FUNCTION(this);
    for (uint32_t i = 0; i < c.GetN(); ++i)
    {
        std::pair<Ptr<Ipv6>, uint32_t> returnValue = c.Get(i);
        Ptr<Ipv6> ipv6 = returnValue.first;
        uint32_t index = returnValue.second;
        Ptr<Ipv6Interface> ipv6Interface = DynamicCast<Ipv6L3Protocol>(ipv6)->GetInterface(index);
        if (ipv6Interface)
        {
            Ptr<NetDevice> netDevice = ipv6Interface->GetDevice();
            Ptr<Channel> channel = netDevice->GetChannel();
            for (std::size_t j = 0; j < channel->GetNDevices(); ++j)
            {
                Ptr<NetDevice> neighborDevice = channel->GetDevice(j);
                if (neighborDevice != netDevice)
                {
                    Ptr<Node> neighborNode = neighborDevice->GetNode();
                    int32_t ipv6NeighborInterfaceIndex =
                        neighborNode->GetObject<Ipv6>()->GetInterfaceForDevice(neighborDevice);
                    if (ipv6NeighborInterfaceIndex != -1)
                    {
                        Ptr<Ipv6Interface> ipv6NeighborInterface =
                            neighborNode->GetObject<Ipv6L3Protocol>()->GetInterface(
                                ipv6NeighborInterfaceIndex);
                        PopulateNeighborEntriesIpv6(ipv6Interface, ipv6NeighborInterface);
                    }
                }
            }
        }
    }
}

void
NeighborCacheHelper::PopulateNeighborEntriesIpv4(Ptr<Ipv4Interface> ipv4Interface,
                                                 Ptr<Ipv4Interface> neighborDeviceInterface) const
{
    uint32_t netDeviceAddresses = ipv4Interface->GetNAddresses();
    uint32_t neighborDeviceAddresses = neighborDeviceInterface->GetNAddresses();
    if (m_dynamicNeighborCache)
    {
        ipv4Interface->RemoveAddressCallback(
            MakeCallback(&NeighborCacheHelper::UpdateCacheByIpv4AddressRemoved, this));
        if (m_globalNeighborCache)
        {
            ipv4Interface->AddAddressCallback(
                MakeCallback(&NeighborCacheHelper::UpdateCacheByIpv4AddressAdded, this));
        }
    }
    for (uint32_t n = 0; n < netDeviceAddresses; ++n)
    {
        Ipv4InterfaceAddress netDeviceIfAddr = ipv4Interface->GetAddress(n);
        for (uint32_t m = 0; m < neighborDeviceAddresses; ++m)
        {
            Ipv4InterfaceAddress neighborDeviceIfAddr = neighborDeviceInterface->GetAddress(m);
            if (netDeviceIfAddr.IsInSameSubnet(neighborDeviceIfAddr.GetLocal()))
            {
                Ptr<NetDevice> neighborDevice = neighborDeviceInterface->GetDevice();
                // Add Arp entry of neighbor interface to current interface's Arp cache
                AddEntry(ipv4Interface,
                         neighborDeviceIfAddr.GetAddress(),
                         neighborDevice->GetAddress());
            }
        }
    }
}

void
NeighborCacheHelper::PopulateNeighborEntriesIpv6(Ptr<Ipv6Interface> ipv6Interface,
                                                 Ptr<Ipv6Interface> neighborDeviceInterface) const
{
    uint32_t netDeviceAddresses = ipv6Interface->GetNAddresses();
    uint32_t neighborDeviceAddresses = neighborDeviceInterface->GetNAddresses();
    if (m_dynamicNeighborCache)
    {
        ipv6Interface->RemoveAddressCallback(
            MakeCallback(&NeighborCacheHelper::UpdateCacheByIpv6AddressRemoved, this));
        if (m_globalNeighborCache)
        {
            ipv6Interface->AddAddressCallback(
                MakeCallback(&NeighborCacheHelper::UpdateCacheByIpv6AddressAdded, this));
        }
    }
    for (uint32_t n = 0; n < netDeviceAddresses; ++n)
    {
        Ipv6InterfaceAddress netDeviceIfAddr = ipv6Interface->GetAddress(n);
        // Ignore if it is a linklocal address, which will be added along with the global address
        if (netDeviceIfAddr.GetScope() == Ipv6InterfaceAddress::LINKLOCAL ||
            netDeviceIfAddr.GetScope() == Ipv6InterfaceAddress::HOST)
        {
            NS_LOG_LOGIC("Skip the LINKLOCAL or LOCALHOST interface " << netDeviceIfAddr);
            continue;
        }
        for (uint32_t m = 0; m < neighborDeviceAddresses; ++m)
        {
            // Ignore if it is a linklocal address, which will be added along with the global
            // address
            Ipv6InterfaceAddress neighborDeviceIfAddr = neighborDeviceInterface->GetAddress(m);
            if (neighborDeviceIfAddr.GetScope() == Ipv6InterfaceAddress::LINKLOCAL ||
                neighborDeviceIfAddr.GetScope() == Ipv6InterfaceAddress::HOST)
            {
                NS_LOG_LOGIC("Skip the LINKLOCAL or LOCALHOST interface " << neighborDeviceIfAddr);
                continue;
            }
            if (netDeviceIfAddr.IsInSameSubnet(neighborDeviceIfAddr.GetAddress()))
            {
                Ptr<NetDevice> neighborDevice = neighborDeviceInterface->GetDevice();
                // Add neighbor's Ndisc entries of global address and linklocal address to current
                // interface's Ndisc cache
                AddEntry(ipv6Interface,
                         neighborDeviceIfAddr.GetAddress(),
                         neighborDevice->GetAddress());
                Ipv6InterfaceAddress neighborlinkLocalAddr =
                    neighborDeviceInterface->GetLinkLocalAddress();
                AddEntry(ipv6Interface,
                         neighborlinkLocalAddr.GetAddress(),
                         neighborDevice->GetAddress());
            }
        }
    }
}

void
NeighborCacheHelper::AddEntry(Ptr<Ipv4Interface> netDeviceInterface,
                              Ipv4Address ipv4Address,
                              Address macAddress) const
{
    NS_LOG_FUNCTION(this << netDeviceInterface << ipv4Address << macAddress);
    Ptr<ArpCache> arpCache = netDeviceInterface->GetArpCache();
    if (!arpCache)
    {
        NS_LOG_LOGIC(
            "ArpCache doesn't exist, might be a point-to-point NetDevice without ArpCache");
        return;
    }
    ArpCache::Entry* entry = arpCache->Lookup(ipv4Address);
    if (!entry)
    {
        NS_LOG_FUNCTION("ADD an ARP entry");
        entry = arpCache->Add(ipv4Address);
    }
    entry->SetMacAddress(macAddress);
    entry->MarkAutoGenerated();
}

void
NeighborCacheHelper::AddEntry(Ptr<Ipv6Interface> netDeviceInterface,
                              Ipv6Address ipv6Address,
                              Address macAddress) const
{
    NS_LOG_FUNCTION(this << netDeviceInterface << ipv6Address << macAddress);
    Ptr<NdiscCache> ndiscCache = netDeviceInterface->GetNdiscCache();
    if (!ndiscCache)
    {
        NS_LOG_LOGIC(
            "NdiscCache doesn't exist, might be a point-to-point NetDevice without NdiscCache");
        return;
    }
    NdiscCache::Entry* entry = ndiscCache->Lookup(ipv6Address);
    if (!entry)
    {
        NS_LOG_FUNCTION("ADD a NDISC entry");
        entry = ndiscCache->Add(ipv6Address);
    }
    entry->SetMacAddress(macAddress);
    entry->MarkAutoGenerated();
}

void
NeighborCacheHelper::FlushAutoGenerated() const
{
    NS_LOG_FUNCTION(this);
    for (uint32_t i = 0; i < NodeList::GetNNodes(); ++i)
    {
        Ptr<Node> node = NodeList::GetNode(i);
        for (uint32_t j = 0; j < node->GetNDevices(); ++j)
        {
            Ptr<NetDevice> netDevice = node->GetDevice(j);
            int32_t ipv4InterfaceIndex = node->GetObject<Ipv4>()->GetInterfaceForDevice(netDevice);
            int32_t ipv6InterfaceIndex = node->GetObject<Ipv6>()->GetInterfaceForDevice(netDevice);
            if (ipv4InterfaceIndex != -1)
            {
                Ptr<Ipv4Interface> ipv4Interface =
                    node->GetObject<Ipv4L3Protocol>()->GetInterface(ipv4InterfaceIndex);
                Ptr<ArpCache> arpCache = ipv4Interface->GetArpCache();
                if (arpCache)
                {
                    NS_LOG_FUNCTION("Remove an ARP entry");
                    arpCache->RemoveAutoGeneratedEntries();
                }
            }
            if (ipv6InterfaceIndex != -1)
            {
                Ptr<Ipv6Interface> ipv6Interface =
                    node->GetObject<Ipv6L3Protocol>()->GetInterface(ipv6InterfaceIndex);
                Ptr<NdiscCache> ndiscCache = ipv6Interface->GetNdiscCache();
                if (ndiscCache)
                {
                    NS_LOG_FUNCTION("Remove a NDISC entry");
                    ndiscCache->RemoveAutoGeneratedEntries();
                }
            }
        }
    }
}

void
NeighborCacheHelper::UpdateCacheByIpv4AddressRemoved(const Ptr<Ipv4Interface> interface,
                                                     const Ipv4InterfaceAddress ifAddr) const
{
    NS_LOG_FUNCTION(this);
    Ptr<NetDevice> netDevice = interface->GetDevice();
    Ptr<Channel> channel = netDevice->GetChannel();
    for (std::size_t i = 0; i < channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> neighborDevice = channel->GetDevice(i);
        Ptr<Node> neighborNode = neighborDevice->GetNode();
        int32_t neighborInterfaceIndex =
            neighborNode->GetObject<Ipv4>()->GetInterfaceForDevice(neighborDevice);
        if (neighborInterfaceIndex != -1)
        {
            Ptr<Ipv4Interface> neighborInterface =
                neighborNode->GetObject<Ipv4L3Protocol>()->GetInterface(neighborInterfaceIndex);
            Ptr<ArpCache> arpCache = neighborInterface->GetArpCache();
            if (!arpCache)
            {
                NS_LOG_LOGIC("ArpCache doesn't exist");
                return;
            }
            ArpCache::Entry* entry = arpCache->Lookup(ifAddr.GetLocal());
            if (entry)
            {
                arpCache->Remove(entry);
            }
        }
    }
}

void
NeighborCacheHelper::UpdateCacheByIpv4AddressAdded(const Ptr<Ipv4Interface> interface,
                                                   const Ipv4InterfaceAddress ifAddr) const
{
    NS_LOG_FUNCTION(this);
    Ptr<NetDevice> netDevice = interface->GetDevice();
    Ptr<Channel> channel = netDevice->GetChannel();
    for (std::size_t i = 0; i < channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> neighborDevice = channel->GetDevice(i);
        if (neighborDevice != netDevice)
        {
            Ptr<Node> neighborNode = neighborDevice->GetNode();
            int32_t neighborInterfaceIndex =
                neighborNode->GetObject<Ipv4>()->GetInterfaceForDevice(neighborDevice);
            if (neighborInterfaceIndex != -1)
            {
                Ptr<Ipv4Interface> neighborInterface =
                    neighborNode->GetObject<Ipv4L3Protocol>()->GetInterface(neighborInterfaceIndex);
                uint32_t neighborDeviceAddresses = neighborInterface->GetNAddresses();
                for (uint32_t m = 0; m < neighborDeviceAddresses; ++m)
                {
                    Ipv4InterfaceAddress neighborDeviceIfAddr = neighborInterface->GetAddress(m);
                    if (ifAddr.IsInSameSubnet(neighborDeviceIfAddr.GetLocal()))
                    {
                        // Add Arp entity of current interface to its neighbor's Arp cache
                        AddEntry(neighborInterface, ifAddr.GetAddress(), netDevice->GetAddress());
                    }
                }
            }
        }
    }
}

void
NeighborCacheHelper::UpdateCacheByIpv6AddressRemoved(const Ptr<Ipv6Interface> interface,
                                                     const Ipv6InterfaceAddress ifAddr) const
{
    NS_LOG_FUNCTION(this);
    Ptr<NetDevice> netDevice = interface->GetDevice();
    Ptr<Channel> channel = netDevice->GetChannel();
    for (std::size_t i = 0; i < channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> neighborDevice = channel->GetDevice(i);
        Ptr<Node> neighborNode = neighborDevice->GetNode();
        int32_t neighborInterfaceIndex =
            neighborNode->GetObject<Ipv6>()->GetInterfaceForDevice(neighborDevice);
        if (neighborInterfaceIndex != -1)
        {
            Ptr<Ipv6Interface> neighborInterface =
                neighborNode->GetObject<Ipv6L3Protocol>()->GetInterface(neighborInterfaceIndex);
            Ptr<NdiscCache> ndiscCache = neighborInterface->GetNdiscCache();
            if (!ndiscCache)
            {
                NS_LOG_LOGIC("ndiscCache doesn't exist");
                return;
            }
            NdiscCache::Entry* entry = ndiscCache->Lookup(ifAddr.GetAddress());
            if (entry)
            {
                ndiscCache->Remove(entry);
            }
        }
    }
}

void
NeighborCacheHelper::UpdateCacheByIpv6AddressAdded(const Ptr<Ipv6Interface> interface,
                                                   const Ipv6InterfaceAddress ifAddr) const
{
    NS_LOG_FUNCTION(this);
    Ptr<NetDevice> netDevice = interface->GetDevice();
    Ptr<Channel> channel = netDevice->GetChannel();
    for (std::size_t i = 0; i < channel->GetNDevices(); ++i)
    {
        Ptr<NetDevice> neighborDevice = channel->GetDevice(i);
        if (neighborDevice != netDevice)
        {
            Ptr<Node> neighborNode = neighborDevice->GetNode();
            int32_t neighborInterfaceIndex =
                neighborNode->GetObject<Ipv6>()->GetInterfaceForDevice(neighborDevice);
            if (neighborInterfaceIndex != -1)
            {
                Ptr<Ipv6Interface> neighborInterface =
                    neighborNode->GetObject<Ipv6L3Protocol>()->GetInterface(neighborInterfaceIndex);
                uint32_t neighborDeviceAddresses = neighborInterface->GetNAddresses();
                for (uint32_t m = 0; m < neighborDeviceAddresses; ++m)
                {
                    Ipv6InterfaceAddress neighborDeviceIfAddr = neighborInterface->GetAddress(m);
                    if (ifAddr.IsInSameSubnet(neighborDeviceIfAddr.GetAddress()))
                    {
                        // Add Arp entity of current interface to its neighbor's Arp cache
                        AddEntry(neighborInterface, ifAddr.GetAddress(), netDevice->GetAddress());
                    }
                }
            }
        }
    }
}

void
NeighborCacheHelper::SetDynamicNeighborCache(bool enable)
{
    NS_LOG_FUNCTION(this);
    m_dynamicNeighborCache = enable;
}

} // namespace ns3
