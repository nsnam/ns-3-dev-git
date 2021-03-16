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
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"

/**
 * This program demonstrates two types of
 * trace output that are available:
 * 1) Print Routing Table for all the nodes.
 * 2) Print Routing Path, given source and destination.
 *
 * Simple point to point links:
 * \verbatim
       ________
      /        \
    n0 -- n1 -- n2 -- n3

    n0 IP: 10.1.1.1, 10.1.4.1
    n1 IP: 10.1.1.2, 10.1.2.1
    n2 IP: 10.1.2.2, 10.1.3.1, 10.1.4.2
    n3 IP: 10.1.3.2
   \endverbatim
 *
 * Route Path for considered cases:
 * - Source (n0) and Destination (n3)
 *   It goes from n0 -> n2 -> n3
 * - Source (n1) and Destination (n3)
 *   It goes from n1 -> n2 -> n3
 * - Source (n2) and Destination (n0)
 *   It goes from n2 -> n0
 * - Source (n1) and Destination (n1)
 *   It goes from n1 -> n1
 * .
 * \verbatim
   Expected Routing Path output for above
   cases (in the output stream):
   Time: +3s, Nix Routing
   Route Path: (Node 0 to Node 3, Nix Vector: 101)
   10.1.4.1 (Node 0)   ---->   10.1.4.2 (Node 2)
   10.1.3.1 (Node 2)   ---->   10.1.3.2 (Node 3)

   Time: +5s, Nix Routing
   Route Path: (Node 1 to Node 3, Nix Vector: 101)
   10.1.2.1 (Node 1)   ---->   10.1.2.2 (Node 2)
   10.1.3.1 (Node 2)   ---->   10.1.3.2 (Node 3)

   Time: +6s, Nix Routing
   Route Path: (Node 2 to Node 0, Nix Vector: 10)
   10.1.4.2 (Node 2)   ---->   10.1.4.1 (Node 0)

   Time: +7s, Nix Routing
   Route Path: (Node 1 to Node 1, Nix Vector: )
   10.1.1.2 (Node 1)   ---->   10.1.1.2 (Node 1)

   Node: 0, Time: +8s, Local time: +8s, Nix Routing
   NixCache:
   Destination     NixVector
   10.1.3.2        101
   Ipv4RouteCache:
   Destination     Gateway         Source            OutputDevice
   10.1.3.2        10.1.4.2        10.1.4.1          2

   Node: 1, Time: +8s, Local time: +8s, Nix Routing
   NixCache:
   Destination     NixVector
   10.1.3.2        101
   Ipv4RouteCache:
   Destination     Gateway         Source            OutputDevice
   10.1.3.2        10.1.2.2        10.1.2.1          2

   Node: 2, Time: +8s, Local time: +8s, Nix Routing
   NixCache:
   Destination     NixVector
   10.1.1.1        10
   Ipv4RouteCache:
   Destination     Gateway         Source            OutputDevice
   10.1.1.1        10.1.4.1        10.1.4.2          3
   10.1.3.2        10.1.3.2        10.1.3.1          2
   10.1.4.1        10.1.4.1        10.1.4.2          3

   Node: 3, Time: +8s, Local time: +8s, Nix Routing
   NixCache:
   Destination     NixVector
   10.1.4.1        010
   Ipv4RouteCache:
   Destination     Gateway         Source            OutputDevice
   10.1.4.1        10.1.3.1        10.1.3.2          1
   \endverbatim
 */

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("NixSimpleExample");

int
main (int argc, char *argv[])
{
  CommandLine cmd (__FILE__);
  cmd.Parse (argc, argv);

  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NodeContainer nodes12;
  nodes12.Create (2);

  NodeContainer nodes23;
  nodes23.Add (nodes12.Get (1));
  nodes23.Create (1);

  NodeContainer nodes34;
  nodes34.Add (nodes23.Get (1));
  nodes34.Create (1);

  NodeContainer nodes13;
  nodes13.Add (nodes12.Get (0));
  nodes13.Add (nodes34.Get (0));

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));

  NodeContainer allNodes = NodeContainer (nodes12, nodes23.Get (1), nodes34.Get (1));

  // NixHelper to install nix-vector routing
  // on all nodes
  Ipv4NixVectorHelper nixRouting;
  InternetStackHelper stack;
  stack.SetRoutingHelper (nixRouting); // has effect on the next Install ()
  stack.Install (allNodes);

  NetDeviceContainer devices12;
  NetDeviceContainer devices23;
  NetDeviceContainer devices34;
  NetDeviceContainer devices13;
  devices12 = pointToPoint.Install (nodes12);
  devices23 = pointToPoint.Install (nodes23);
  devices34 = pointToPoint.Install (nodes34);
  devices13 = pointToPoint.Install (nodes13);

  Ipv4AddressHelper address1;
  address1.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4AddressHelper address2;
  address2.SetBase ("10.1.2.0", "255.255.255.0");
  Ipv4AddressHelper address3;
  address3.SetBase ("10.1.3.0", "255.255.255.0");
  Ipv4AddressHelper address4;
  address4.SetBase ("10.1.4.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces12 = address1.Assign (devices12);
  Ipv4InterfaceContainer interfaces23 = address2.Assign (devices23);
  Ipv4InterfaceContainer interfaces34 = address3.Assign (devices34);
  Ipv4InterfaceContainer interfaces13 = address4.Assign (devices13);

  UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (nodes34.Get (1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (interfaces34.GetAddress (1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (nodes12.Get (0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));

  // Trace routing paths for different source and destinations.
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("nix-simple.routes", std::ios::out);
  nixRouting.PrintRoutingPathAt (Seconds (3), nodes12.Get (0), interfaces34.GetAddress (1), routingStream);
  nixRouting.PrintRoutingPathAt (Seconds (5), nodes12.Get (1), interfaces34.GetAddress (1), routingStream);
  nixRouting.PrintRoutingPathAt (Seconds (6), nodes23.Get (1), interfaces12.GetAddress (0), routingStream);
  nixRouting.PrintRoutingPathAt (Seconds (7), nodes12.Get (1), interfaces12.GetAddress (1), routingStream);
  // Trace routing tables
  nixRouting.PrintRoutingTableAllAt (Seconds (8), routingStream);

  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
