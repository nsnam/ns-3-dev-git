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
      LogComponentEnable ("DhcpClient", LOG_LEVEL_ALL);
      LogComponentEnable ("DhcpServer", LOG_LEVEL_ALL);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
    }

  Time stopTime = Seconds (20);

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

  uint32_t ifIndex;
  Ptr<Ipv4> ipv4Router = net.Get (3)->GetObject<Ipv4> ();
  ifIndex = ipv4Router->AddInterface (dev_net.Get (3));
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.12"), Ipv4Mask ("/24")));
  // ipv4Router->SetForwarding (ifIndex, true);
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
  DhcpServerHelper dhcpServer (Ipv4Address ("172.30.0.0"), Ipv4Mask ("/24"),
                               Ipv4Address ("172.30.0.10"), Ipv4Address ("172.30.0.15"),
                               Ipv4Address ("172.30.0.17"));

  ApplicationContainer ap_dhcp_server = dhcpServer.Install (router.Get (0));
  ap_dhcp_server.Start (Seconds (1.0));
  ap_dhcp_server.Stop (stopTime);

  DhcpClientHelper dhcpClient;
  NetDeviceContainer dhcpClientNetDevs;
  dhcpClientNetDevs.Add (dev_net.Get (0));
  dhcpClientNetDevs.Add (dev_net.Get (1));
  dhcpClientNetDevs.Add (dev_net.Get (2));

  ApplicationContainer dhcpClients = dhcpClient.Install (dhcpClientNetDevs);
  dhcpClients.Start (Seconds (10.0));
  dhcpClients.Stop (stopTime);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (p2pNodes.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (stopTime);

  UdpEchoClientHelper echoClient (p2pInterfaces.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (100));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes.Get (1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (stopTime);

  Simulator::Stop (stopTime + Seconds (10.0));

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
