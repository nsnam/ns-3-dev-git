/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli "Federico II"
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
 * Author: Pasquale Imputato <p.imputato@gmail.com>
 * Author: Stefano Avallone <stefano.avallone@unina.it>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/traffic-control-module.h"

// This simple example shows how to use TrafficContorlHelper to install a QueueDisc on a device.
// The default QueueDisc is a pfifo_fast with max number of packets equal to 1000 (as in Linux)
//
// Network topology
//
//       10.1.1.0
// n0 -------------- n1
//    point-to-point

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TrafficControlExample");

void
PacketsInQueueTrace (uint32_t oldValue, uint32_t newValue)
{
  std::cout << "PacketsInQueue " << oldValue << " to " << newValue << std::endl;
}

int
main (int argc, char *argv[])
{
  double simulationTime = 10; //seconds

  NodeContainer nodes;
  nodes.Create (2);

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
  pointToPoint.SetQueue ("ns3::DropTailQueue", "Mode", StringValue ("QUEUE_MODE_PACKETS"), "MaxPackets", UintegerValue (1000));

  NetDeviceContainer devices;
  devices = pointToPoint.Install (nodes);

  InternetStackHelper stack;
  stack.Install (nodes);

  QueueDiscHelper qdh;
  qdh.SetQueueDiscType ("ns3::PfifoFastQueueDisc");
  qdh.SetAttribute ("Band0", PointerValue (CreateObjectWithAttributes<DropTailQueue> ("MaxPackets", UintegerValue (1000))));
  qdh.SetAttribute ("Band1", PointerValue (CreateObjectWithAttributes<DropTailQueue> ("MaxPackets", UintegerValue (1000))));
  qdh.SetAttribute ("Band2", PointerValue (CreateObjectWithAttributes<DropTailQueue> ("MaxPackets", UintegerValue (1000))));
  QueueDiscContainer qdiscs = qdh.Install (devices);

  Ptr<QueueDisc> q = qdiscs.Get (0);
  q->TraceConnectWithoutContext ("PacketsInQueue", MakeCallback (&PacketsInQueueTrace));

  Ipv4AddressHelper address;
  address.SetBase ("10.1.1.0", "255.255.255.0");

  Ipv4InterfaceContainer interfaces = address.Assign (devices);

  //Flow
  uint16_t port = 7;
  Address localAddress (InetSocketAddress (Ipv4Address::GetAny (), port));
  PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", localAddress);
  ApplicationContainer sinkApp = packetSinkHelper.Install (nodes.Get (0));

  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (simulationTime + 0.1));

  uint32_t payloadSize = 1448;
  Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (payloadSize));

  OnOffHelper onoff ("ns3::TcpSocketFactory",Ipv4Address::GetAny ());
  onoff.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  onoff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  onoff.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  onoff.SetAttribute ("DataRate", DataRateValue (1000000000)); //bit/s
  ApplicationContainer apps;

  AddressValue remoteAddress (InetSocketAddress (interfaces.GetAddress (0), port));
  onoff.SetAttribute ("Remote", remoteAddress);
  apps.Add (onoff.Install (nodes.Get (1)));
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (simulationTime + 0.1));

  Simulator::Stop (Seconds (simulationTime + 0.1));
  Simulator::Run ();
  Simulator::Destroy ();

  double thr = 0;
  uint32_t totalPacketsThr = DynamicCast<PacketSink> (sinkApp.Get (0))->GetTotalRx ();
  thr = totalPacketsThr * 8 / (simulationTime * 1000000.0); //Mbit/s
  std::cout << thr << " Mbit/s" <<std::endl;

  return 0;
}
