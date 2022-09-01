/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include "ns3/test.h"
#include "ns3/socket-factory.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/simulator.h"
#include "ns3/simple-channel.h"
#include "ns3/simple-net-device.h"
#include "ns3/socket.h"

#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/icmpv4-l4-protocol.h"
#include "ns3/ipv6-address-helper.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/udp-l4-protocol.h"
#include "ns3/ipv4-routing-helper.h"
#include "ns3/ipv6-routing-helper.h"
#include "ns3/neighbor-cache-helper.h"

using namespace ns3;

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief Neighbor cache Test
 */
class NeighborCacheTest : public TestCase
{
  Ptr<Packet> m_receivedPacket; //!< Received packet

  /**
   * \brief Send data immediately after being called.
   * \param socket The sending socket.
   * \param to IPv4 Destination address.
   */
  void DoSendDatav4 (Ptr<Socket> socket, Ipv4Address to);

  /**
   * \brief Send data immediately after being called.
   * \param socket The sending socket.
   * \param to IPv6 Destination address.
   */
  void DoSendDatav6 (Ptr<Socket> socket, Ipv6Address to);

  /**
   * \brief Schedules the DoSendData () function to send the data.
   * \param socket The sending socket.
   * \param to IPv4 Destination address.
   */
  void SendData (Ptr<Socket> socket, Ipv4Address to);

  /**
   * \brief Schedules the DoSendData () function to send the data.
   * \param socket The sending socket.
   * \param to IPv6 Destination address.
   */
  void SendData (Ptr<Socket> socket, Ipv6Address to);

public:
  virtual void DoRun (void);

  NeighborCacheTest ();

  /**
   * \brief Receive data.
   * \param socket The receiving socket.
   */
  void ReceivePkt (Ptr<Socket> socket);

  std::vector<uint32_t> m_receivedPacketSizes; //!< Received packet sizes
};

NeighborCacheTest::NeighborCacheTest ()
  : TestCase ("NeighborCache")
{}

void NeighborCacheTest::ReceivePkt (Ptr<Socket> socket)
{
  [[maybe_unused]] uint32_t availableData;
  availableData = socket->GetRxAvailable ();
  m_receivedPacket = socket->Recv (std::numeric_limits<uint32_t>::max (), 0);
  NS_TEST_ASSERT_MSG_EQ (availableData, m_receivedPacket->GetSize (), "Received Packet size is not equal to the Rx buffer size");
  m_receivedPacketSizes.push_back (m_receivedPacket->GetSize ());
}

void
NeighborCacheTest::DoSendDatav4 (Ptr<Socket> socket, Ipv4Address to)
{
  Address realTo = InetSocketAddress (to, 1234);
  NS_TEST_EXPECT_MSG_EQ (socket->SendTo (Create<Packet> (123), 0, realTo),
                         123, "100");
}

void
NeighborCacheTest::DoSendDatav6 (Ptr<Socket> socket, Ipv6Address to)
{
  Address realTo = Inet6SocketAddress (to, 1234);
  NS_TEST_EXPECT_MSG_EQ (socket->SendTo (Create<Packet> (123), 0, realTo),
                         123, "100");
}

void
NeighborCacheTest::SendData (Ptr<Socket> socket, Ipv4Address to)
{
  m_receivedPacket = Create<Packet> ();
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), Seconds (60),
                                  &NeighborCacheTest::DoSendDatav4, this, socket, to);
}

void
NeighborCacheTest::SendData (Ptr<Socket> socket, Ipv6Address to)
{
  m_receivedPacket = Create<Packet> ();
  Simulator::ScheduleWithContext (socket->GetNode ()->GetId (), Seconds (60),
                                  &NeighborCacheTest::DoSendDatav6, this, socket, to);
}

void
NeighborCacheTest::DoRun (void)
{
  Ptr<Node> txNode = CreateObject<Node> ();
  Ptr<Node> rxNode = CreateObject<Node> ();
  Ptr<Node> snifferNode = CreateObject<Node> ();

  NodeContainer all (txNode, rxNode, snifferNode);

  std::ostringstream stringStream1v4;
  Ptr<OutputStreamWrapper> arpStream = Create<OutputStreamWrapper> (&stringStream1v4);
  std::ostringstream stringStream1v6;
  Ptr<OutputStreamWrapper> ndiscStream = Create<OutputStreamWrapper> (&stringStream1v6);

  InternetStackHelper internetNodes;
  internetNodes.Install (all);

  NetDeviceContainer net;
  // Sender Node
  Ptr<SimpleNetDevice> txDev;
  {
    txDev = CreateObject<SimpleNetDevice> ();
    txDev->SetAddress (Mac48Address ("00:00:00:00:00:01"));
    txNode->AddDevice (txDev);
  }
  net.Add (txDev);

  // Recieve node
  Ptr<SimpleNetDevice> rxDev;
  {
    rxDev = CreateObject<SimpleNetDevice> ();
    rxDev->SetAddress (Mac48Address ("00:00:00:00:00:02"));
    rxNode->AddDevice (rxDev);
  }
  net.Add (rxDev);

  // Sniffer node
  Ptr<SimpleNetDevice> snifferDev;
  {
    snifferDev = CreateObject<SimpleNetDevice> ();
    snifferDev->SetAddress (Mac48Address ("00:00:00:00:00:03"));
    snifferNode->AddDevice (snifferDev);
  }
  net.Add (snifferDev);

  // link the channels
  Ptr<SimpleChannel> channel = CreateObject<SimpleChannel> ();
  txDev->SetChannel (channel);
  rxDev->SetChannel (channel);
  snifferDev->SetChannel (channel);

  // Setup IPv4 addresses
  Ipv4AddressHelper ipv4;
  ipv4.SetBase (Ipv4Address ("10.0.1.0"), Ipv4Mask ("255.255.255.0"));
  Ipv4InterfaceContainer icv4 = ipv4.Assign (net);

  //Setup IPv6 addresses
  Ipv6AddressHelper ipv6;
  ipv6.SetBase (Ipv6Address ("2001:0::"), Ipv6Prefix (64));
  Ipv6InterfaceContainer icv6 = ipv6.Assign (net);

  //Populate perfect caches.
  NeighborCacheHelper neighborCache;
  neighborCache.PopulateNeighborCache ();

  //Print cache.
  Ipv4RoutingHelper::PrintNeighborCacheAllAt (Seconds (0), arpStream);
  Ipv6RoutingHelper::PrintNeighborCacheAllAt (Seconds (0), ndiscStream);

  // Create the UDP sockets
  Ptr<SocketFactory> rxSocketFactory = rxNode->GetObject<UdpSocketFactory> ();
  Ptr<Socket> rxSocketv4 = rxSocketFactory->CreateSocket ();
  Ptr<Socket> rxSocketv6 = rxSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (rxSocketv4->Bind (InetSocketAddress (Ipv4Address ("10.0.1.2"), 1234)), 0, "trivial");
  NS_TEST_EXPECT_MSG_EQ (rxSocketv6->Bind (Inet6SocketAddress (Ipv6Address ("2001:0::200:ff:fe00:2"), 1234)), 0, "trivial");
  rxSocketv4->SetRecvCallback (MakeCallback (&NeighborCacheTest::ReceivePkt, this));
  rxSocketv6->SetRecvCallback (MakeCallback (&NeighborCacheTest::ReceivePkt, this));

  Ptr<SocketFactory> snifferSocketFactory = snifferNode->GetObject<UdpSocketFactory> ();
  Ptr<Socket> snifferSocketv4 = snifferSocketFactory->CreateSocket ();
  Ptr<Socket> snifferSocketv6 = snifferSocketFactory->CreateSocket ();
  NS_TEST_EXPECT_MSG_EQ (snifferSocketv4->Bind (InetSocketAddress (Ipv4Address ("10.0.1.3"), 1234)), 0, "trivial");
  NS_TEST_EXPECT_MSG_EQ (snifferSocketv6->Bind (Inet6SocketAddress (Ipv6Address ("2001:0::200:ff:fe00:3"), 1234)), 0, "trivial");
  snifferSocketv4->SetRecvCallback (MakeCallback (&NeighborCacheTest::ReceivePkt, this));
  snifferSocketv6->SetRecvCallback (MakeCallback (&NeighborCacheTest::ReceivePkt, this));

  Ptr<SocketFactory> txSocketFactory = txNode->GetObject<UdpSocketFactory> ();
  Ptr<Socket> txSocket = txSocketFactory->CreateSocket ();
  txSocket->SetAllowBroadcast (true);

  // ------ Now the tests ------------

  // Unicast test
  SendData (txSocket, Ipv4Address ("10.0.1.2"));
  SendData (txSocket, Ipv6Address ("2001:0::200:ff:fe00:2"));

  Simulator::Stop (Seconds (66));
  Simulator::Run ();

  NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSizes[0], 123, "Perfect Arp should work.");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSizes[1], 123, "Perfect Ndp should work.");
  NS_TEST_EXPECT_MSG_EQ (m_receivedPacketSizes.size (), 2, "Perfect Arp and perfect Ndp should have received only 1 packet.");

  // Test the  Cache
  constexpr auto arpCache = "ARP Cache of node 0 at time 0\n"
    "10.0.1.2 dev 1 lladdr 04-06-00:00:00:00:00:02 STATIC_AUTOGENERATED\n"
    "10.0.1.3 dev 1 lladdr 04-06-00:00:00:00:00:03 STATIC_AUTOGENERATED\n"
    "ARP Cache of node 1 at time 0\n"
    "10.0.1.1 dev 1 lladdr 04-06-00:00:00:00:00:01 STATIC_AUTOGENERATED\n"
    "10.0.1.3 dev 1 lladdr 04-06-00:00:00:00:00:03 STATIC_AUTOGENERATED\n"
    "ARP Cache of node 2 at time 0\n"
    "10.0.1.1 dev 1 lladdr 04-06-00:00:00:00:00:01 STATIC_AUTOGENERATED\n"
    "10.0.1.2 dev 1 lladdr 04-06-00:00:00:00:00:02 STATIC_AUTOGENERATED\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream1v4.str (), arpCache, "Arp cache is incorrect.");

  constexpr auto NdiscCache = "NDISC Cache of node 0 at time +0s\n"
    "2001::200:ff:fe00:2 dev 1 lladdr 04-06-00:00:00:00:00:02 STATIC_AUTOGENERATED\n"
    "2001::200:ff:fe00:3 dev 1 lladdr 04-06-00:00:00:00:00:03 STATIC_AUTOGENERATED\n"
    "fe80::200:ff:fe00:2 dev 1 lladdr 04-06-00:00:00:00:00:02 STATIC_AUTOGENERATED\n"
    "fe80::200:ff:fe00:3 dev 1 lladdr 04-06-00:00:00:00:00:03 STATIC_AUTOGENERATED\n"
    "NDISC Cache of node 1 at time +0s\n"
    "2001::200:ff:fe00:1 dev 1 lladdr 04-06-00:00:00:00:00:01 STATIC_AUTOGENERATED\n"
    "2001::200:ff:fe00:3 dev 1 lladdr 04-06-00:00:00:00:00:03 STATIC_AUTOGENERATED\n"
    "fe80::200:ff:fe00:1 dev 1 lladdr 04-06-00:00:00:00:00:01 STATIC_AUTOGENERATED\n"
    "fe80::200:ff:fe00:3 dev 1 lladdr 04-06-00:00:00:00:00:03 STATIC_AUTOGENERATED\n"
    "NDISC Cache of node 2 at time +0s\n"
    "2001::200:ff:fe00:1 dev 1 lladdr 04-06-00:00:00:00:00:01 STATIC_AUTOGENERATED\n"
    "2001::200:ff:fe00:2 dev 1 lladdr 04-06-00:00:00:00:00:02 STATIC_AUTOGENERATED\n"
    "fe80::200:ff:fe00:1 dev 1 lladdr 04-06-00:00:00:00:00:01 STATIC_AUTOGENERATED\n"
    "fe80::200:ff:fe00:2 dev 1 lladdr 04-06-00:00:00:00:00:02 STATIC_AUTOGENERATED\n";
  NS_TEST_EXPECT_MSG_EQ (stringStream1v6.str (), NdiscCache, "Ndisc cache is incorrect.");

  m_receivedPacket->RemoveAllByteTags ();

  Simulator::Destroy ();
}

/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief IPv4 RIP TestSuite
 */
class NeighborCacheTestSuite : public TestSuite
{
public:
  NeighborCacheTestSuite () : TestSuite ("neighbor-cache", UNIT)
  {
    AddTestCase (new NeighborCacheTest, TestCase::QUICK);
  }
};

static NeighborCacheTestSuite g_neighborcacheTestSuite; //!< Static variable for test initialization
