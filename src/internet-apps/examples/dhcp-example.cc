/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UPB
 * Copyright (c) 2017 NITK Surathkal
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
 * Author: Radu Lupu <rlupu@elcom.pub.ro>
 *         Ankit Deepak <adadeepak8@gmail.com>
 *         Deepti Rajagopal <deeptir96@gmail.com>
 *
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DhcpExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd;

  bool verbose = false;
  bool tracing = false;
  cmd.AddValue ("verbose", "turn on the logs", verbose);
  cmd.AddValue ("tracing", "turn on the tracing", tracing);

  cmd.Parse (argc, argv);

  // GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

 if (verbose)
   {
     LogComponentEnable ("DhcpClient", LOG_LEVEL_INFO);
     LogComponentEnable ("DhcpServer", LOG_LEVEL_INFO);
     LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
     LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
   }

  NS_LOG_INFO ("Create nodes.");
  NodeContainer nodes;
  NodeContainer router;
  nodes.Create (3);
  router.Create (2);

  NodeContainer net (nodes, router);

  NS_LOG_INFO ("Create channels.");
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  NetDeviceContainer dev_net = csma.Install (net);

  NodeContainer p2pNodes;
  p2pNodes.Add (net.Get (4));
  p2pNodes.Create (1);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (p2pNodes);

  InternetStackHelper tcpip;
  tcpip.Install (nodes);
  tcpip.Install (router);
  tcpip.Install (p2pNodes.Get (1));

  Ipv4AddressHelper address;
  address.SetBase ("172.30.1.0", "255.255.255.0");
  Ipv4InterfaceContainer p2pInterfaces;
  p2pInterfaces = address.Assign (p2pDevices);

  // manually add a routing entry because we don't want to add a dynamic routing
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4> ipv4Ptr = p2pNodes.Get (1)->GetObject<Ipv4> ();
  Ptr<Ipv4StaticRouting> staticRoutingA = ipv4RoutingHelper.GetStaticRouting (ipv4Ptr);
  staticRoutingA->AddNetworkRouteTo (Ipv4Address ("172.30.0.0"), Ipv4Mask ("/24"),
                                     Ipv4Address ("172.30.1.1"), 1);


  Ptr<Ipv4> ipv4MN = net.Get (0)->GetObject<Ipv4> ();
  uint32_t ifIndex = ipv4MN->AddInterface (dev_net.Get (0));
  ipv4MN->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/32")));
  ipv4MN->SetForwarding (ifIndex, true);
  ipv4MN->SetUp (ifIndex);

  Ptr<Ipv4> ipv4MN1 = net.Get (1)->GetObject<Ipv4> ();
  uint32_t ifIndex1 = ipv4MN1->AddInterface (dev_net.Get (1));
  ipv4MN1->AddAddress (ifIndex1, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/32")));
  ipv4MN1->SetForwarding (ifIndex1, true);
  ipv4MN1->SetUp (ifIndex1);

  Ptr<Ipv4> ipv4MN2 = net.Get (2)->GetObject<Ipv4> ();
  uint32_t ifIndex2 = ipv4MN2->AddInterface (dev_net.Get (2));
  ipv4MN2->AddAddress (ifIndex2, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/32")));
  ipv4MN2->SetForwarding (ifIndex2, true);
  ipv4MN2->SetUp (ifIndex2);

  Ptr<Ipv4> ipv4Router = net.Get (3)->GetObject<Ipv4> ();
  ifIndex = ipv4Router->AddInterface (dev_net.Get (3));
   ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.12"), Ipv4Mask ("/0"))); // need to remove this workaround
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.12"), Ipv4Mask ("/24")));
  ipv4Router->SetForwarding (ifIndex, true);
  ipv4Router->SetUp (ifIndex);

  Ptr<Ipv4> ipv4Router1 = net.Get (4)->GetObject<Ipv4> ();
  ifIndex = ipv4Router1->AddInterface (dev_net.Get (4));
  ipv4Router1->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.17"), Ipv4Mask ("/24")));
  ipv4Router1->SetForwarding (ifIndex, true);
  ipv4Router1->SetUp (ifIndex);

  NS_LOG_INFO ("Create Applications.");

  /*
   * DhcpServerHelper (Ipv4Address pool_addr, Ipv4Mask pool_mask,
   *                   Ipv4Address serv_addr,
   *                   Ipv4Address min_addr, Ipv4Address max_addr,
   *                   Ipv4Address gateway = Ipv4Address ());
   *
   */
  DhcpServerHelper dhcp_server (Ipv4Address ("172.30.0.0"), Ipv4Mask ("/24"),
                                Ipv4Address ("172.30.0.10"), Ipv4Address ("172.30.0.15"),
                                Ipv4Address ("172.30.0.17"));

  ApplicationContainer ap_dhcp_server = dhcp_server.Install (router.Get (0));
  ap_dhcp_server.Start (Seconds (1.0));
  ap_dhcp_server.Stop (Seconds (100.0));

  DhcpClientHelper dhcp_client (0);
  ApplicationContainer ap_dhcp_client = dhcp_client.Install (nodes.Get (0));
  ap_dhcp_client.Start (Seconds (1.0));
  ap_dhcp_client.Stop (Seconds (100.0));

  DhcpClientHelper dhcp_client1 (0);
  ApplicationContainer ap_dhcp_client1 = dhcp_client1.Install (nodes.Get (1));
  ap_dhcp_client1.Start (Seconds (1.0));
  ap_dhcp_client1.Stop (Seconds (100.0));

  DhcpClientHelper dhcp_client2 (0);
  ApplicationContainer ap_dhcp_client2 = dhcp_client2.Install (nodes.Get (2));
  ap_dhcp_client2.Start (Seconds (1.0));
  ap_dhcp_client2.Stop (Seconds (100.0));

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (p2pNodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (100.0));

  UdpEchoClientHelper echoClient (p2pInterfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (100.0));

  Simulator::Stop (Seconds (110.0));

  if (tracing)
    {
      csma.EnablePcapAll ("dhcp-csma");
      pointToPoint.EnablePcapAll ("dhcp-p2p");
    }

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
