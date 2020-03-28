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
#include "ns3/jellyfish-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-nix-vector-helper.h"
#include "ns3/packet-sink.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpBulkSendExample");

int
main (int argc, char *argv[])
{

  LogComponentEnable ("TcpBulkSendExample", LOG_LEVEL_INFO);
  LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);

  uint32_t nTopOfRackSwitch=250;
  uint32_t nPort = 8;
  uint32_t nServer = 3;
  uint32_t maxBytes = 0;

  CommandLine cmd;
  cmd.AddValue ("nTopOfRackSwitch", "Total Number Of Switches: ", nTopOfRackSwitch);
  cmd.AddValue ("nPort", "Total Number Of Ports in a Switch: ", nPort);
  cmd.AddValue ("nServer", "Total Number Of Servers in a Switch : ", nServer);
  cmd.Parse (argc, argv);  


  PointToPointHelper pointToPoint1;
  pointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint1.SetChannelAttribute ("Delay", StringValue ("5ms"));


  PointToPointHelper pointToPoint2;
  pointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("500Kbps"));
  pointToPoint2.SetChannelAttribute ("Delay", StringValue ("5ms"));

  JellyFishHelper jelly = JellyFishHelper(nTopOfRackSwitch, nPort, nServer, pointToPoint1, pointToPoint2);
  
  InternetStackHelper stack;
  Ipv4NixVectorHelper nixRouting;
  stack.SetRoutingHelper (nixRouting);
  jelly.InstallStack(stack);


  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.0.0");
  jelly.AssignIpv4Addresses(address);


  uint16_t port = 9;  // well-known echo port number


  BulkSendHelper source ("ns3::TcpSocketFactory",
                         InetSocketAddress (jelly.GetIpv4Address(4,0), port));
  // Set the amount of data to send in bytes.  Zero is unlimited.
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
  ApplicationContainer sourceApps = source.Install (jelly.GetServer(2,1));
  sourceApps.Start (Seconds (0.0));
  sourceApps.Stop (Seconds (10.0));

//
// Create a PacketSinkApplication and install it on node 1
//
  PacketSinkHelper sink ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApps = sink.Install (jelly.GetServer(4,0));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (10.0));


  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();

    // Trace routing tables
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("nix-simple.routes", std::ios::out);
  nixRouting.PrintRoutingTableAllAt (Seconds (8), routingStream);


  Ptr<PacketSink> sink1 = DynamicCast<PacketSink> (sinkApps.Get (0));
  std::cout << "Total Bytes Received: " << sink1->GetTotalRx () << std::endl;
  return 0;
}
