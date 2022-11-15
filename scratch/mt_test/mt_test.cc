/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-layer.h"
#include "ns3/modular-transport.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MTTest");

int 
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("Testing Modular Transport");

  NodeContainer nodes;
  nodes.Create(2);
  
  PointToPointHelper p2p;
  NetDeviceContainer devs = p2p.Install(nodes);

  // Install the stack
  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i){
    Ptr<Node> node = *i; 

    NS_LOG_UNCOND ("Installing ARP on Node " << node->GetId());
 
    Ptr<ArpL3Protocol> arp = CreateObject<ArpL3Protocol>();
    node->AggregateObject(arp);

    NS_LOG_UNCOND ("Installing IP on Node " << node->GetId());
    
    Ptr<Ipv4L3Protocol> ip = CreateObject<Ipv4L3Protocol>();
    node->AggregateObject(ip);

    
    NS_LOG_UNCOND ("Installing ICMP on Node " << node->GetId());
    
    Ptr<Icmpv4L4Protocol> icmp = CreateObject<Icmpv4L4Protocol>();
    node->AggregateObject(icmp);

    NS_LOG_UNCOND("Configuring Routing");

    Ipv4StaticRoutingHelper staticRouting;
    Ipv4GlobalRoutingHelper globalRouting;
    Ipv4ListRoutingHelper listRouting;
    listRouting.Add (staticRouting, 0);
    listRouting.Add (globalRouting, -10);
    
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ptr<Ipv4RoutingProtocol> ipv4Routing = listRouting.Create (node);
    ipv4->SetRoutingProtocol (ipv4Routing); 

    Ptr<TrafficControlLayer> tc = CreateObject<TrafficControlLayer>();
    node->AggregateObject(tc);
    arp->SetTrafficControl(tc);

    NS_LOG_UNCOND ("Installing Modular Transport on Node " << node->GetId());
    Ptr<ModularTransport> transport = CreateObject<ModularTransport>();
    node->AggregateObject(transport);
  }

  NS_LOG_UNCOND("");

  // Set up IP addresses
  NS_LOG_UNCOND ("Setting up IP Addresses");

  Ipv4AddressHelper ipv4_add;
  ipv4_add.SetBase("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer ipInterfs = ipv4_add.Assign(devs);

  for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i){
    Ptr<Node> node = *i;
    NS_LOG_UNCOND("Node " << node->GetId() << " Interfaces: ");
    Ptr<Ipv4L3Protocol> ipv4 = node->GetObject<Ipv4L3Protocol>();
    for (unsigned int f = 0; f < ipv4->GetNInterfaces(); f++){
      Ptr<Ipv4Interface> intf = ipv4->GetInterface(f);
      for (unsigned int a = 0; a < intf->GetNAddresses(); a++){
        NS_LOG_UNCOND(intf->GetAddress(a));
      }
    }

    NS_LOG_UNCOND("");
  } 

  
  // Send a packet
  Ptr<Node> src = *nodes.Begin();
  Ptr<Ipv4Interface> src_intf = src->GetObject<Ipv4L3Protocol>()->GetInterface(1); 
  Ipv4Address saddr = src_intf->GetAddress(0).GetAddress();
  NS_LOG_UNCOND("Source address: " << saddr);
  Ptr<Node> dst = *(nodes.End() - 1);
  Ipv4Address daddr = dst->GetObject<Ipv4L3Protocol>()->GetInterface(1)->GetAddress(0).GetAddress();
  NS_LOG_UNCOND("Destination address: " << daddr);

  Ptr<Packet> packet = Create<Packet> (100);
  MTHeader mth;
  mth.SetF1(2);
  Ptr<ModularTransport> transport = src->GetObject<ModularTransport>();
  Simulator::Schedule(Seconds(1), &ModularTransport::SendPacket, transport, packet, mth, saddr, daddr);
  

  Simulator::Run ();
  Simulator::Destroy ();
}
