/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "internet-static-setup-helper.h"

#include "ns3/arp-cache.h"
#include "ns3/assert.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node-container.h"
#include "ns3/node-list.h"
#include "ns3/node.h"
#include "ns3/object-factory.h"
#include "ns3/object-vector.h"
#include "ns3/pointer.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("InternetStaticSetupHelper");

const Time ONE_YEAR = Seconds(3600 * 365 * 24);

void
InternetStaticSetupHelper::PopulateArpCache()
{
    auto arp = CreateObject<ArpCache>();
    arp->SetAliveTimeout(ONE_YEAR);
    NodeContainer arpNodes;

    // Create an ARP cache that contains all IP->MAC address mapping for nodes with netdevices
    for (auto itNode = NodeList::Begin(); itNode != NodeList::End(); ++itNode)
    {
        auto ip = (*itNode)->GetObject<Ipv4L3Protocol>();
        if (!ip)
        {
            continue;
        }
        arpNodes.Add(*itNode);
        ObjectVectorValue interfaces;
        ip->GetAttribute("InterfaceList", interfaces);
        for (auto itIntf = interfaces.Begin(); itIntf != interfaces.End(); ++itIntf)
        {
            // Iterate all devices on the node, each device has a unique MAC address
            auto ipIface = (*itIntf).second->GetObject<Ipv4Interface>();
            NS_ASSERT(ipIface);
            ipIface->SetAttribute("ArpCache", PointerValue(arp));
            auto device = ipIface->GetDevice();
            NS_ASSERT(device);
            auto addr = device->GetAddress();
            for (size_t cntAddr = 0; cntAddr < ipIface->GetNAddresses(); ++cntAddr)
            {
                // Iterate all IP addresses associated with the device and
                // Create one ARP entry per IP
                auto ipAddr = ipIface->GetAddress(cntAddr).GetLocal();
                if (ipAddr == Ipv4Address::GetLoopback())
                {
                    continue;
                }
                if (auto [entry, wasAdded] = arp->TryAdd(ipAddr, addr); wasAdded)
                {
                    entry->MarkPermanent();
                    NS_LOG_INFO("ARP entry added for " << ipAddr << "->" << addr);
                }
            }
        }
    }
}

} // namespace ns3
