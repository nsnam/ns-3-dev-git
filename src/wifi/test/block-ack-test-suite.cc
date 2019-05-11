/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/test.h"
#include "ns3/string.h"
#include "ns3/qos-utils.h"
#include "ns3/ctrl-headers.h"
#include "ns3/packet.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/mobility-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/config.h"
#include "ns3/pointer.h"

using namespace ns3;

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Packet Buffering Case A
 *
 * This simple test verifies the correctness of buffering for packets received
 * under block ack. In order to completely understand this example is important to cite
 * section 9.10.3 in IEEE802.11 standard:
 *
 * "[...] The sequence number space is considered divided into two parts, one of which
 * is “old” and one of which is “new” by means of a boundary created by adding half the
 * sequence number range to the current start of receive window (modulo 2^12)."
 */
//-------------------------------------------------------------------------------------

/* ----- = old packets
 * +++++ = new packets
 *
 *  CASE A: startSeq < endSeq
 *                        -  -   +
 *  initial buffer state: 0 16 56000
 *
 *
 *    0                            4095
 *    |------|++++++++++++++++|-----|
 *           ^                ^
 *           | startSeq       | endSeq = 4000
 *
 *  first received packet's sequence control = 64016 (seqNum = 4001, fragNum = 0) -
 *  second received packet's sequence control = 63984 (seqNum = 3999, fragNum = 0) +
 *  4001 is older seq number so this packet should be inserted at the buffer's begin.
 *  3999 is previous element of older of new packets: it should be inserted at the end of buffer.
 *
 *  expected buffer state: 64016 0 16 56000 63984
 *
 */
class PacketBufferingCaseA : public TestCase
{
public:
  PacketBufferingCaseA ();
  virtual ~PacketBufferingCaseA ();
private:
  virtual void DoRun (void);
  std::list<uint16_t> m_expectedBuffer; ///< expected test buffer
};

PacketBufferingCaseA::PacketBufferingCaseA ()
  : TestCase ("Check correct order of buffering when startSequence < endSeq")
{
  m_expectedBuffer.push_back (64016);
  m_expectedBuffer.push_back (0);
  m_expectedBuffer.push_back (16);
  m_expectedBuffer.push_back (56000);
  m_expectedBuffer.push_back (63984);
}

PacketBufferingCaseA::~PacketBufferingCaseA ()
{
}

void
PacketBufferingCaseA::DoRun (void)
{
  std::list<uint16_t> m_buffer;
  std::list<uint16_t>::iterator i,j;
  m_buffer.push_back (0);
  m_buffer.push_back (16);
  m_buffer.push_back (56000);

  uint16_t endSeq = 4000;

  uint16_t receivedSeq = 4001 * 16;
  uint32_t mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  receivedSeq = 3999 * 16;
  mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  for (i = m_buffer.begin (), j = m_expectedBuffer.begin (); i != m_buffer.end (); i++, j++)
    {
      NS_TEST_EXPECT_MSG_EQ (*i, *j, "error in buffer order");
    }
}


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Packet Buffering Case B
 *
 * ----- = old packets
 * +++++ = new packets
 *
 *  CASE B: startSeq > endSeq
 *                         -    +    +
 *  initial buffer state: 256 64000 16
 *
 *
 *    0                            4095
 *    |++++++|----------------|++++++|
 *           ^                ^
 *           | endSeq = 10    | startSeq
 *
 *  first received packet's sequence control = 240 (seqNum = 15, fragNum = 0)  -
 *  second received packet's sequence control = 241 (seqNum = 15, fragNum = 1) -
 *  third received packet's sequence control = 64800 (seqNum = 4050, fragNum = 0) +
 *  240 is an old packet should be inserted at the buffer's begin.
 *  241 is an old packet: second segment of the above packet.
 *  4050 is a new packet: it should be inserted between 64000 and 16.
 *
 *  expected buffer state: 240 241 256 64000 64800 16
 *
 */
class PacketBufferingCaseB : public TestCase
{
public:
  PacketBufferingCaseB ();
  virtual ~PacketBufferingCaseB ();
private:
  virtual void DoRun (void);
  std::list<uint16_t> m_expectedBuffer; ///< expected test buffer
};

PacketBufferingCaseB::PacketBufferingCaseB ()
  : TestCase ("Check correct order of buffering when startSequence > endSeq")
{
  m_expectedBuffer.push_back (240);
  m_expectedBuffer.push_back (241);
  m_expectedBuffer.push_back (256);
  m_expectedBuffer.push_back (64000);
  m_expectedBuffer.push_back (64800);
  m_expectedBuffer.push_back (16);
}

PacketBufferingCaseB::~PacketBufferingCaseB ()
{
}

void
PacketBufferingCaseB::DoRun (void)
{
  std::list<uint16_t> m_buffer;
  std::list<uint16_t>::iterator i,j;
  m_buffer.push_back (256);
  m_buffer.push_back (64000);
  m_buffer.push_back (16);

  uint16_t endSeq = 10;

  uint16_t receivedSeq = 15 * 16;
  uint32_t mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  receivedSeq = 15 * 16 + 1;
  mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  receivedSeq = 4050 * 16;
  mappedSeq = QosUtilsMapSeqControlToUniqueInteger (receivedSeq, endSeq);
  /* cycle to right position for this packet */
  for (i = m_buffer.begin (); i != m_buffer.end (); i++)
    {
      if (QosUtilsMapSeqControlToUniqueInteger ((*i), endSeq) >= mappedSeq)
        {
          //position found
          break;
        }
    }
  m_buffer.insert (i, receivedSeq);

  for (i = m_buffer.begin (), j = m_expectedBuffer.begin (); i != m_buffer.end (); i++, j++)
    {
      NS_TEST_EXPECT_MSG_EQ (*i, *j, "error in buffer order");
    }
}


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test for block ack header
 */
class CtrlBAckResponseHeaderTest : public TestCase
{
public:
  CtrlBAckResponseHeaderTest ();
private:
  virtual void DoRun ();
  CtrlBAckResponseHeader m_blockAckHdr; ///< block ack header
};

CtrlBAckResponseHeaderTest::CtrlBAckResponseHeaderTest ()
  : TestCase ("Check the correctness of block ack compressed bitmap")
{
}

void
CtrlBAckResponseHeaderTest::DoRun (void)
{
  m_blockAckHdr.SetType (COMPRESSED_BLOCK_ACK);

  //Case 1: startSeq < endSeq
  //          179        242
  m_blockAckHdr.SetStartingSequence (179);
  for (uint16_t i = 179; i < 220; i++)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  for (uint16_t i = 225; i <= 242; i++)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0xffffc1ffffffffffLL, "error in compressed bitmap");
  m_blockAckHdr.SetReceivedPacket (1500);
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0xffffc1ffffffffffLL, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (220), false, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (225), true, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (1500), false, "error in compressed bitmap");

  m_blockAckHdr.ResetBitmap ();

  //Case 2: startSeq > endSeq
  //          4090       58
  m_blockAckHdr.SetStartingSequence (4090);
  for (uint16_t i = 4090; i != 10; i = (i + 1) % 4096)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  for (uint16_t i = 22; i < 25; i++)
    {
      m_blockAckHdr.SetReceivedPacket (i);
    }
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0x00000000007000ffffLL, "error in compressed bitmap");
  m_blockAckHdr.SetReceivedPacket (80);
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.GetCompressedBitmap (), 0x00000000007000ffffLL, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (4090), true, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (4095), true, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (10), false, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (35), false, "error in compressed bitmap");
  NS_TEST_EXPECT_MSG_EQ (m_blockAckHdr.IsPacketReceived (80), false, "error in compressed bitmap");
}


/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test for Block Ack Policy with aggregation disabled
 *
 * This test aims to check the Block Ack policy with "legacy" 802.11, i.e., prior
 * to aggregation (802.11n). The block ack threshold is set to 2, hence a block ack
 * agreement is established when there are at least two packets in the EDCA queue.
 * Consequently, the first packet is sent with Normal Ack policy (because a BA agreement
 * has not been established yet), while all other packets are sent with Block Ack
 * policy and followed by a Block Ack Request and then a Block Ack.
 */
class BlockAckAggregationDisabledTest : public TestCase
{
  /**
  * Keeps the maximum duration among all TXOPs
  */
  struct TxopDurationTracer
  {
    void Trace (Time startTime, Time duration);
    Time m_max {Seconds (0)};
  };

public:
  BlockAckAggregationDisabledTest ();
  virtual ~BlockAckAggregationDisabledTest ();

  virtual void DoRun (void);


private:
  uint32_t m_received; ///< received packets
  uint16_t m_txTotal; ///< transmitted data packets
  uint16_t m_nBar; ///< transmitted BlockAckReq frames
  uint16_t m_nBa; ///< received BlockAck frames

  /**
   * Function to trace packets received by the server application
   * \param context the context
   * \param p the packet
   * \param adr the address
   */
  void L7Receive (std::string context, Ptr<const Packet> p, const Address &adr);
  /**
   * Callback invoked when PHY transmits a packet
   * \param context the context
   * \param p the packet
   * \param power the tx power
   */
  void Transmit (std::string context, Ptr<const Packet> p, double power);
  /**
   * Callback invoked when PHY receives a packet
   * \param context the context
   * \param p the packet
   */
  void Receive (std::string context, Ptr<const Packet> p);
};

void
BlockAckAggregationDisabledTest::TxopDurationTracer::Trace (Time startTime, Time duration)
{
  if (duration > m_max)
    {
      m_max = duration;
    }
}

BlockAckAggregationDisabledTest::BlockAckAggregationDisabledTest ()
  : TestCase ("Test case for Block Ack Policy with aggregation disabled"),
    m_received (0),
    m_txTotal (0),
    m_nBar (0),
    m_nBa (0)
{
}

BlockAckAggregationDisabledTest::~BlockAckAggregationDisabledTest ()
{
}

void
BlockAckAggregationDisabledTest::L7Receive (std::string context, Ptr<const Packet> p, const Address &adr)
{
  if (p->GetSize () == 1400)
    {
      m_received++;
    }
}

void
BlockAckAggregationDisabledTest::Transmit (std::string context, Ptr<const Packet> p, double power)
{
  WifiMacHeader hdr;
  p->PeekHeader (hdr);

  if (hdr.IsQosData ())
    {
      m_txTotal++;
      NS_TEST_EXPECT_MSG_EQ ((m_txTotal == 1 || hdr.IsQosBlockAck ()), true, "Unexpected QoS ack policy");
    }
  else if (hdr.IsBlockAckReq ())
    {
      m_nBar++;
    }
}

void
BlockAckAggregationDisabledTest::Receive (std::string context, Ptr<const Packet> p)
{
  WifiMacHeader hdr;
  p->PeekHeader (hdr);

  if (hdr.IsBlockAck ())
    {
      m_nBa++;
    }
}

void
BlockAckAggregationDisabledTest::DoRun (void)
{
  NodeContainer wifiStaNode;
  wifiStaNode.Create (1);

  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  wifi.SetRemoteStationManager ("ns3::IdealWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "QosSupported", BooleanValue (true),
               "Ssid", SsidValue (ssid),
               /* setting blockack threshold for sta's BE queue */
               "BE_BlockAckThreshold", UintegerValue (2),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNode);

  mac.SetType ("ns3::ApWifiMac",
               "QosSupported", BooleanValue (true),
               "Ssid", SsidValue (ssid),
               "BeaconGeneration", BooleanValue (true));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();

  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (1.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);
  mobility.Install (wifiStaNode);

  Ptr<WifiNetDevice> ap_device = DynamicCast<WifiNetDevice> (apDevices.Get (0));
  Ptr<WifiNetDevice> sta_device = DynamicCast<WifiNetDevice> (staDevices.Get (0));

  // Disable A-MPDU aggregation
  sta_device->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
  TxopDurationTracer txopTracer;

  PacketSocketAddress socket;
  socket.SetSingleDevice (sta_device->GetIfIndex ());
  socket.SetPhysicalAddress (ap_device->GetAddress ());
  socket.SetProtocol (1);

  // give packet socket powers to nodes.
  PacketSocketHelper packetSocket;
  packetSocket.Install (wifiStaNode);
  packetSocket.Install (wifiApNode);

  Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient> ();
  client->SetAttribute ("PacketSize", UintegerValue (1400));
  client->SetAttribute ("MaxPackets", UintegerValue (14));
  client->SetAttribute ("Interval", TimeValue (MicroSeconds (0)));
  client->SetRemote (socket);
  wifiStaNode.Get (0)->AddApplication (client);
  client->SetStartTime (Seconds (1));
  client->SetStopTime (Seconds (3.0));

  Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer> ();
  server->SetLocal (socket);
  wifiApNode.Get (0)->AddApplication (server);
  server->SetStartTime (Seconds (0.0));
  server->SetStopTime (Seconds (4.0));

  Config::Connect ("/NodeList/*/ApplicationList/0/$ns3::PacketSocketServer/Rx", MakeCallback (&BlockAckAggregationDisabledTest::L7Receive, this));
  Config::Connect ("/NodeList/0/DeviceList/0/Phy/PhyTxBegin", MakeCallback (&BlockAckAggregationDisabledTest::Transmit, this));
  Config::Connect ("/NodeList/0/DeviceList/0/Phy/PhyRxBegin", MakeCallback (&BlockAckAggregationDisabledTest::Receive, this));

  Simulator::Stop (Seconds (5));
  Simulator::Run ();

  Simulator::Destroy ();

  // The client application generates 14 packets, so we expect that the wifi PHY
  // layer transmits 14 MPDUs, the server application receives 14 packets, and
  // a BAR is transmitted after each MPDU but the first one (because a BA agreement
  // is established before transmitting the second MPDU).
  NS_TEST_EXPECT_MSG_EQ (m_txTotal, 14, "Unexpected number of transmitted packets");
  NS_TEST_EXPECT_MSG_EQ (m_received, 14, "Unexpected number of received packets");
  NS_TEST_EXPECT_MSG_EQ (m_nBar, 13, "Unexpected number of Block Ack Requests");
  NS_TEST_EXPECT_MSG_EQ (m_nBa, 13, "Unexpected number of Block Ack Responses");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Block Ack Test Suite
 */
class BlockAckTestSuite : public TestSuite
{
public:
  BlockAckTestSuite ();
};

BlockAckTestSuite::BlockAckTestSuite ()
  : TestSuite ("wifi-block-ack", UNIT)
{
  AddTestCase (new PacketBufferingCaseA, TestCase::QUICK);
  AddTestCase (new PacketBufferingCaseB, TestCase::QUICK);
  AddTestCase (new CtrlBAckResponseHeaderTest, TestCase::QUICK);
  AddTestCase (new BlockAckAggregationDisabledTest, TestCase::QUICK);
}

static BlockAckTestSuite g_blockAckTestSuite; ///< the test suite
