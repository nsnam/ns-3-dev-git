/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Tom Henderson (tomh@tomh.org)
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
 */

#include "ns3/test.h"
#include "ns3/nstime.h"
#include "ns3/node.h"
#include "ns3/traced-callback.h"
#include "ns3/node-container.h"
#include "ns3/application-container.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/bulk-send-application.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"

using namespace ns3;

class BulkSendBasicTestCase : public TestCase
{
public:
  BulkSendBasicTestCase ();
  virtual ~BulkSendBasicTestCase ();

private:
  virtual void DoRun (void);
  void SendTx (Ptr<const Packet> p);
  void ReceiveRx (Ptr<const Packet> p, const Address& addr); 
  uint64_t m_sent {0};
  uint64_t m_received {0};
};

BulkSendBasicTestCase::BulkSendBasicTestCase ()
  : TestCase ("Check a basic 300KB transfer")
{
}

BulkSendBasicTestCase::~BulkSendBasicTestCase ()
{
}

void
BulkSendBasicTestCase::SendTx (Ptr<const Packet> p)
{
  m_sent += p->GetSize ();
}

void
BulkSendBasicTestCase::ReceiveRx (Ptr<const Packet> p, const Address& addr) 
{
  m_received += p->GetSize ();
}

void
BulkSendBasicTestCase::DoRun (void)
{
  Ptr<Node> sender = CreateObject<Node> ();
  Ptr<Node> receiver = CreateObject<Node> ();
  NodeContainer nodes;
  nodes.Add (sender);
  nodes.Add (receiver);
  SimpleNetDeviceHelper simpleHelper;
  simpleHelper.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  simpleHelper.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer devices;
  devices = simpleHelper.Install (nodes);
  InternetStackHelper internet;
  internet.Install (nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  uint16_t port = 9;
  BulkSendHelper sourceHelper ("ns3::TcpSocketFactory",
                               InetSocketAddress (i.GetAddress (1), port));
  sourceHelper.SetAttribute ("MaxBytes", UintegerValue (300000));
  ApplicationContainer sourceApp = sourceHelper.Install (nodes.Get (0));
  sourceApp.Start (Seconds (0.0));
  sourceApp.Stop (Seconds (10.0));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (1));
  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (10.0));

  Ptr<BulkSendApplication> source = DynamicCast<BulkSendApplication> (sourceApp.Get (0));
  Ptr<PacketSink> sink = DynamicCast<PacketSink> (sinkApp.Get (0));

  source->TraceConnectWithoutContext ("Tx", MakeCallback (&BulkSendBasicTestCase::SendTx, this));
  sink->TraceConnectWithoutContext ("Rx", MakeCallback (&BulkSendBasicTestCase::ReceiveRx, this));

  Simulator::Run ();
  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (m_sent, 300000, "Sent the full 300000 bytes");
  NS_TEST_ASSERT_MSG_EQ (m_received, 300000, "Received the full 300000 bytes");
}

// This test checks that the sequence number is sent and received in sequence
// despite the sending application having to pause and restart its sending
// due to a temporarily full transmit buffer.
class BulkSendSeqTsSizeTestCase : public TestCase
{
public:
  BulkSendSeqTsSizeTestCase ();
  virtual ~BulkSendSeqTsSizeTestCase ();

private:
  virtual void DoRun (void);
  void SendTx (Ptr<const Packet> p, const Address &from, const Address & to, const SeqTsSizeHeader &header);
  void ReceiveRx (Ptr<const Packet> p, const Address &from, const Address & to, const SeqTsSizeHeader &header);
  uint64_t m_sent {0};
  uint64_t m_received {0};
  uint64_t m_seqTxCounter {0};
  uint64_t m_seqRxCounter {0};
  Time m_lastTxTs {Seconds (0)};
  Time m_lastRxTs {Seconds (0)};
};

BulkSendSeqTsSizeTestCase::BulkSendSeqTsSizeTestCase ()
  : TestCase ("Check a 300KB transfer with SeqTsSize header enabled")
{
}

BulkSendSeqTsSizeTestCase::~BulkSendSeqTsSizeTestCase ()
{
}

void
BulkSendSeqTsSizeTestCase::SendTx (Ptr<const Packet> p, const Address &from, const Address & to, const SeqTsSizeHeader &header)
{
  // The header is not serialized onto the packet in this trace
  m_sent += p->GetSize () + header.GetSerializedSize ();
  NS_TEST_ASSERT_MSG_EQ (header.GetSeq (), m_seqTxCounter, "Missing sequence number");
  m_seqTxCounter++;
  NS_TEST_ASSERT_MSG_GT_OR_EQ (header.GetTs (), m_lastTxTs, "Timestamp less than last time");
  m_lastTxTs = header.GetTs ();
}

void
BulkSendSeqTsSizeTestCase::ReceiveRx (Ptr<const Packet> p, const Address &from, const Address & to, const SeqTsSizeHeader &header)
{
  // The header is not serialized onto the packet in this trace
  m_received += p->GetSize () + header.GetSerializedSize ();
  NS_TEST_ASSERT_MSG_EQ (header.GetSeq (), m_seqRxCounter, "Missing sequence number");
  m_seqRxCounter++;
  NS_TEST_ASSERT_MSG_GT_OR_EQ (header.GetTs (), m_lastRxTs, "Timestamp less than last time");
  m_lastRxTs = header.GetTs ();
}

void
BulkSendSeqTsSizeTestCase::DoRun (void)
{
  Ptr<Node> sender = CreateObject<Node> ();
  Ptr<Node> receiver = CreateObject<Node> ();
  NodeContainer nodes;
  nodes.Add (sender);
  nodes.Add (receiver);
  SimpleNetDeviceHelper simpleHelper;
  simpleHelper.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  simpleHelper.SetChannelAttribute ("Delay", StringValue ("10ms"));
  NetDeviceContainer devices;
  devices = simpleHelper.Install (nodes);
  InternetStackHelper internet;
  internet.Install (nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);
  uint16_t port = 9;
  BulkSendHelper sourceHelper ("ns3::TcpSocketFactory",
                               InetSocketAddress (i.GetAddress (1), port));
  sourceHelper.SetAttribute ("MaxBytes", UintegerValue (300000));
  sourceHelper.SetAttribute ("EnableSeqTsSizeHeader", BooleanValue (true));
  ApplicationContainer sourceApp = sourceHelper.Install (nodes.Get (0));
  sourceApp.Start (Seconds (0.0));
  sourceApp.Stop (Seconds (10.0));
  PacketSinkHelper sinkHelper ("ns3::TcpSocketFactory",
                         InetSocketAddress (Ipv4Address::GetAny (), port));
  sinkHelper.SetAttribute ("EnableSeqTsSizeHeader", BooleanValue (true));
  ApplicationContainer sinkApp = sinkHelper.Install (nodes.Get (1));
  sinkApp.Start (Seconds (0.0));
  sinkApp.Stop (Seconds (10.0));

  Ptr<BulkSendApplication> source = DynamicCast<BulkSendApplication> (sourceApp.Get (0));
  Ptr<PacketSink> sink = DynamicCast<PacketSink> (sinkApp.Get (0));

  source->TraceConnectWithoutContext ("TxWithSeqTsSize", MakeCallback (&BulkSendSeqTsSizeTestCase::SendTx, this));
  sink->TraceConnectWithoutContext ("RxWithSeqTsSize", MakeCallback (&BulkSendSeqTsSizeTestCase::ReceiveRx, this));

  Simulator::Run ();
  Simulator::Destroy ();

  NS_TEST_ASSERT_MSG_EQ (m_sent, 300000, "Sent the full 300000 bytes");
  NS_TEST_ASSERT_MSG_EQ (m_received, 300000, "Received the full 300000 bytes");
}

class BulkSendTestSuite : public TestSuite
{
public:
  BulkSendTestSuite ();
};

BulkSendTestSuite::BulkSendTestSuite ()
  : TestSuite ("bulk-send-application", UNIT)
{
  AddTestCase (new BulkSendBasicTestCase, TestCase::QUICK);
  AddTestCase (new BulkSendSeqTsSizeTestCase, TestCase::QUICK);
}

static BulkSendTestSuite g_bulkSendTestSuite;

