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
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/applications-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"

#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("BandwidthEstimation");

static double TRACE_START_TIME = 0.05;

static void
EstimatedBWTracer (Ptr<OutputStreamWrapper> stream,
                   double oldval, double newval)
{
  NS_LOG_INFO (Simulator::Now ().GetSeconds () <<
                                               " Estimated bandwidth from " << oldval << " to " << newval);

  *stream->GetStream () << Simulator::Now ().GetSeconds () << " "
                        << newval << std::endl;
}

static void
TraceEstimatedBW (Ptr<OutputStreamWrapper> estimatedBWStream)
{
  Config::ConnectWithoutContext ("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionOps/$ns3::TcpWestwood/EstimatedBW",
                                 MakeBoundCallback (&EstimatedBWTracer, estimatedBWStream));
}

int
main (int argc, char *argv[])
{
  NS_LOG_UNCOND ("Figure 3: TCPW with concurrent UDP traffic: bandwidth estimation");
  int bw = 5; // Mbps
  int delay = 30; // milliseconds
  int time = 300; // seconds

  std::string bwStr = std::to_string(bw) + "Mbps";
  std::string delayStr = std::to_string(delay) + "ms";

  std::string dir = "outputs/";
  std::string estimatedBWStreamName = dir + "estimated-bw.tr";
  Ptr<OutputStreamWrapper> estimatedBWStream;
  AsciiTraceHelper asciiTraceHelper;
  estimatedBWStream = asciiTraceHelper.CreateFileStream (estimatedBWStreamName);

  NS_LOG_UNCOND("Creating Nodes...");

  NodeContainer nodes;
  nodes.Create(2);

  Ptr<Node> h1 = nodes.Get(0);
  Ptr<Node> h2 = nodes.Get(1);

  NS_LOG_UNCOND("Configuring Channels...");

  PointToPointHelper link;
  link.SetDeviceAttribute ("DataRate", StringValue (bwStr));
  link.SetChannelAttribute ("Delay", StringValue (delayStr));

  NS_LOG_UNCOND("Creating NetDevices...");

  NetDeviceContainer h1h2_NetDevices = link.Install (h1, h2);

  Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId()));
  Config::SetDefault("ns3::TcpWestwood::ProtocolType", EnumValue(TcpWestwood::WESTWOODPLUS));
  Config::SetDefault("ns3::TcpWestwood::FilterType", EnumValue(TcpWestwood::TUSTIN));

  NS_LOG_UNCOND("Installing Internet Stack...");

  InternetStackHelper stack;
  stack.InstallAll ();

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer h1h2_interfaces = address.Assign (h1h2_NetDevices);
  address.NewNetwork ();

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_UNCOND("Setting up the Application...");

  uint16_t receiverPort = 5001;
  AddressValue receiverAddress (InetSocketAddress (h1h2_interfaces.GetAddress (1),
                                                   receiverPort));
  PacketSinkHelper receiverHelper ("ns3::TcpSocketFactory",
                                   receiverAddress.Get());
  receiverHelper.SetAttribute ("Protocol",
                               TypeIdValue (TcpSocketFactory::GetTypeId ()));

  ApplicationContainer receiverApp = receiverHelper.Install (h2);
  receiverApp.Start (Seconds (0.0));
  receiverApp.Stop (Seconds ((double)time));

  BulkSendHelper ftp ("ns3::TcpSocketFactory", Address ());
  ftp.SetAttribute ("Protocol", TypeIdValue (TcpSocketFactory::GetTypeId ()));
  ftp.SetAttribute ("Remote", receiverAddress);

  ApplicationContainer sourceApp = ftp.Install (h1);
  sourceApp.Start (Seconds (0.0));
  sourceApp.Stop (Seconds ((double)time));

  Simulator::Schedule (Seconds (TRACE_START_TIME), &TraceEstimatedBW, estimatedBWStream);

  NS_LOG_UNCOND("Running the Simulation...");
  Simulator::Stop (Seconds ((double)time));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}

