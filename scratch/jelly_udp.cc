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


using namespace ns3;

int
main (int argc, char *argv[])
{
  uint32_t nTopOfRackSwitch=250;
  uint32_t nPort = 8;
  uint32_t nServer = 3;
 
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  CommandLine cmd;
  cmd.AddValue ("nTopOfRackSwitch", "Total Number Of Switches: ", nTopOfRackSwitch);
  cmd.AddValue ("nPort", "Total Number Of Ports in a Switch: ", nPort);
  cmd.AddValue ("nServer", "Total Number Of Servers in a Switch : ", nServer);
  cmd.Parse (argc, argv);  


  PointToPointHelper PointToPoint1;
  PointToPoint1.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  PointToPoint1.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));


  PointToPointHelper PointToPoint2;
  PointToPoint2.SetDeviceAttribute ("DataRate", StringValue ("100Mbps"));
  PointToPoint2.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

  JellyFishHelper jelly = JellyFishHelper(nTopOfRackSwitch, nPort, nServer, PointToPoint1, PointToPoint2);
  
  InternetStackHelper stack;
  Ipv4NixVectorHelper nixRouting;
  stack.SetRoutingHelper (nixRouting);
  jelly.InstallStack(stack);


  Ipv4AddressHelper address;
  address.SetBase ("10.1.0.0", "255.255.0.0");
  jelly.AssignIpv4Addresses(address);


  UdpEchoServerHelper echoServer (9);
  ApplicationContainer serverApps = echoServer.Install (jelly.GetServer(2,1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (jelly.GetIpv4Address(1,1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = echoClient.Install (jelly.GetServer(4,0));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));


    // Trace routing tables
  Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("nix-simple.routes", std::ios::out);
  nixRouting.PrintRoutingTableAllAt (Seconds (8), routingStream);


  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
