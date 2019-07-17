/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/simulator.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-psdu.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/yans-wifi-phy.h"
#include "ns3/mac-tx-middle.h"
#include "ns3/mac-low.h"
#include "ns3/msdu-aggregator.h"
#include "ns3/mpdu-aggregator.h"
#include "ns3/wifi-net-device.h"
#include "ns3/ht-configuration.h"
#include "ns3/vht-configuration.h"
#include "ns3/he-configuration.h"

using namespace ns3;

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Ampdu Aggregation Test
 */
class AmpduAggregationTest : public TestCase
{
public:
  AmpduAggregationTest ();

private:
  virtual void DoRun (void);
  Ptr<WifiNetDevice> m_device; ///<WifiNetDevice
  Ptr<StaWifiMac> m_mac; ///< Mac
  Ptr<YansWifiPhy> m_phy; ///< Phy
  Ptr<WifiRemoteStationManager> m_manager; ///< remote station manager
  ObjectFactory m_factory; ///< factory
};

AmpduAggregationTest::AmpduAggregationTest ()
  : TestCase ("Check the correctness of MPDU aggregation operations")
{
}

void
AmpduAggregationTest::DoRun (void)
{
  /*
   * Create device and attach HT configuration.
   */
  m_device = CreateObject<WifiNetDevice> ();
  Ptr<HtConfiguration> htConfiguration = CreateObject<HtConfiguration> ();
  m_device->SetHtConfiguration (htConfiguration);

  /*
   * Create and configure phy layer.
   */
  m_phy = CreateObject<YansWifiPhy> ();
  m_phy->SetDevice (m_device);
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  m_device->SetPhy (m_phy);

  /*
   * Create and configure manager.
   */
  m_factory = ObjectFactory ();
  m_factory.SetTypeId ("ns3::ConstantRateWifiManager");
  m_factory.Set ("DataMode", StringValue ("HtMcs7"));
  m_manager = m_factory.Create<WifiRemoteStationManager> ();
  m_manager->SetupPhy (m_phy);
  m_device->SetRemoteStationManager (m_manager);

  /*
   * Create and configure mac layer.
   */
  m_mac = CreateObject<StaWifiMac> ();
  m_mac->SetDevice (m_device);
  m_mac->SetWifiPhy (m_phy);
  m_mac->SetWifiRemoteStationManager (m_manager);
  m_mac->SetAddress (Mac48Address ("00:00:00:00:00:01"));
  m_mac->ConfigureStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  m_device->SetMac (m_mac);

  /*
   * Configure MPDU aggregation.
   */
  m_mac->SetAttribute ("BE_MaxAmpduSize", UintegerValue (65535));
  HtCapabilities htCapabilities;
  htCapabilities.SetMaxAmpduLength (65535);
  m_manager->AddStationHtCapabilities (Mac48Address ("00:00:00:00:00:02"), htCapabilities);
  m_manager->AddStationHtCapabilities (Mac48Address ("00:00:00:00:00:03"), htCapabilities);

  /*
   * Create a dummy packet of 1500 bytes and fill mac header fields.
   */
  Ptr<const Packet> pkt = Create<Packet> (1500);
  Ptr<Packet> currentAggregatedPacket = Create<Packet> ();
  WifiMacHeader hdr;
  hdr.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
  hdr.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint16_t sequence = m_mac->m_txMiddle->GetNextSequenceNumberFor (&hdr);
  hdr.SetSequenceNumber (sequence);
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  /*
   * Establish agreement.
   */
  MgtAddBaRequestHeader reqHdr;
  reqHdr.SetImmediateBlockAck ();
  reqHdr.SetTid (0);
  reqHdr.SetBufferSize (64);
  reqHdr.SetTimeout (0);
  reqHdr.SetStartingSequence (0);
  m_mac->GetBEQueue ()->m_baManager->CreateAgreement (&reqHdr, hdr.GetAddr1 ());
  m_mac->GetBEQueue ()->m_baManager->NotifyAgreementEstablished (hdr.GetAddr1 (), 0, 0);

  //-----------------------------------------------------------------------------------------------------

  /*
   * Test behavior when no other packets are in the queue
   */
  WifiTxVector txVector = m_mac->GetBEQueue ()->GetLow ()->GetDataTxVector (Create<const WifiMacQueueItem> (pkt, hdr));

  auto mpduList = m_mac->GetBEQueue ()->GetLow ()->GetMpduAggregator ()->GetNextAmpdu (Create<WifiMacQueueItem> (pkt, hdr),
                                                                                       txVector);
  NS_TEST_EXPECT_MSG_EQ (mpduList.empty (), true, "a single packet should not result in an A-MPDU");

  //-----------------------------------------------------------------------------------------------------

  /*
   * Test behavior when 2 more packets are in the queue
   */
  Ptr<const Packet> pkt1 = Create<Packet> (1500);
  Ptr<const Packet> pkt2 = Create<Packet> (1500);
  WifiMacHeader hdr1, hdr2;

  hdr1.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
  hdr1.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr1.SetType (WIFI_MAC_QOSDATA);
  hdr1.SetQosTid (0);

  hdr2.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
  hdr2.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr2.SetType (WIFI_MAC_QOSDATA);
  hdr2.SetQosTid (0);

  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt1, hdr1));
  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt2, hdr2));

  mpduList = m_mac->GetBEQueue ()->GetLow ()->GetMpduAggregator ()->GetNextAmpdu (Create<WifiMacQueueItem> (pkt, hdr),
                                                                                  txVector);
  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (mpduList);

  NS_TEST_EXPECT_MSG_EQ (mpduList.empty (), false, "MPDU aggregation failed");
  NS_TEST_EXPECT_MSG_EQ (psdu->GetSize (), 4606, "A-MPDU size is not correct");
  NS_TEST_EXPECT_MSG_EQ (mpduList.size (), 3, "A-MPDU should contain 3 MPDUs");
  NS_TEST_EXPECT_MSG_EQ (m_mac->GetBEQueue ()->GetWifiMacQueue ()->GetNPackets (), 0, "queue should be empty");

  Ptr <WifiMacQueueItem> dequeuedItem;
  WifiMacHeader dequeuedHdr;
  for (uint32_t i = 0; i < psdu->GetNMpdus (); i++)
    {
      NS_TEST_EXPECT_MSG_EQ (psdu->GetHeader (i).GetSequenceNumber (), i, "wrong sequence number");
    }

  //-----------------------------------------------------------------------------------------------------

  /*
   * Test behavior when the 802.11n station and another non-QoS station are associated to the AP.
   * The AP sends an A-MPDU to the 802.11n station followed by the last retransmission of a non-QoS data frame to the non-QoS station.
   * This is used to reproduce bug 2224.
   */
  pkt1 = Create<Packet> (1500);
  pkt2 = Create<Packet> (1500);
  hdr1.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
  hdr1.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr1.SetType (WIFI_MAC_QOSDATA);
  hdr1.SetQosTid (0);
  hdr1.SetSequenceNumber (3);
  hdr2.SetAddr1 (Mac48Address ("00:00:00:00:00:03"));
  hdr2.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr2.SetType (WIFI_MAC_QOSDATA);
  hdr2.SetQosTid (0);

  Ptr<const Packet> pkt3 = Create<Packet> (1500);
  WifiMacHeader hdr3;
  hdr3.SetSequenceNumber (0);
  hdr3.SetAddr1 (Mac48Address ("00:00:00:00:00:03"));
  hdr3.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr3.SetType (WIFI_MAC_QOSDATA);
  hdr3.SetQosTid (0);

  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt3, hdr3));

  mpduList = m_mac->GetBEQueue ()->GetLow ()->GetMpduAggregator ()->GetNextAmpdu (Create<WifiMacQueueItem> (pkt1, hdr1),
                                                                                  txVector);
  NS_TEST_EXPECT_MSG_EQ (mpduList.empty (), true, "a single packet for this destination should not result in an A-MPDU");

  mpduList = m_mac->GetBEQueue ()->GetLow ()->GetMpduAggregator ()->GetNextAmpdu (Create<WifiMacQueueItem> (pkt2, hdr2),
                                                                                  txVector);
  NS_TEST_EXPECT_MSG_EQ (mpduList.empty (), true, "no MPDU aggregation should be performed if there is no agreement");

  m_manager->SetMaxSsrc (0); //set to 0 in order to fake that the maximum number of retries has been reached
  m_mac->GetBEQueue ()->m_currentHdr = hdr2;
  m_mac->GetBEQueue ()->m_currentPacket = pkt2->Copy ();
  m_mac->GetBEQueue ()->MissedAck ();

  NS_TEST_EXPECT_MSG_EQ (m_mac->GetBEQueue ()->m_currentPacket, 0, "packet should be discarded");
  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Remove (pkt3);

  Simulator::Destroy ();

  m_manager->Dispose ();
  m_manager = 0;

  m_device->Dispose ();
  m_device = 0;

  htConfiguration = 0;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Two Level Aggregation Test
 */
class TwoLevelAggregationTest : public TestCase
{
public:
  TwoLevelAggregationTest ();

private:
  virtual void DoRun (void);
  Ptr<WifiNetDevice> m_device; ///<WifiNetDevice
  Ptr<StaWifiMac> m_mac; ///< Mac
  Ptr<YansWifiPhy> m_phy; ///< Phy
  Ptr<WifiRemoteStationManager> m_manager; ///< remote station manager
  ObjectFactory m_factory; ///< factory
};

TwoLevelAggregationTest::TwoLevelAggregationTest ()
  : TestCase ("Check the correctness of two-level aggregation operations")
{
}

void
TwoLevelAggregationTest::DoRun (void)
{
  /*
   * Create device and attach HT configuration.
   */
  m_device = CreateObject<WifiNetDevice> ();
  Ptr<HtConfiguration> htConfiguration = CreateObject<HtConfiguration> ();
  m_device->SetHtConfiguration (htConfiguration);

  /*
   * Create and configure phy layer.
   */
  m_phy = CreateObject<YansWifiPhy> ();
  m_phy->SetDevice (m_device);
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  m_device->SetPhy (m_phy);

  /*
   * Create and configure manager.
   */
  m_factory = ObjectFactory ();
  m_factory.SetTypeId ("ns3::ConstantRateWifiManager");
  m_factory.Set ("DataMode", StringValue ("HtMcs7"));
  m_manager = m_factory.Create<WifiRemoteStationManager> ();
  m_manager->SetupPhy (m_phy);
  m_device->SetRemoteStationManager (m_manager);

  /*
   * Create and configure mac layer.
   */
  m_mac = CreateObject<StaWifiMac> ();
  m_mac->SetDevice (m_device);
  m_mac->SetWifiPhy (m_phy);
  m_mac->SetWifiRemoteStationManager (m_manager);
  m_mac->SetAddress (Mac48Address ("00:00:00:00:00:01"));
  m_mac->ConfigureStandard (WIFI_PHY_STANDARD_80211n_5GHZ);
  m_device->SetMac (m_mac);

  /*
   * Configure aggregation.
   */
  m_mac->SetAttribute ("BE_MaxAmsduSize", UintegerValue (4095));
  m_mac->SetAttribute ("BE_MaxAmpduSize", UintegerValue (65535));
  HtCapabilities htCapabilities;
  htCapabilities.SetMaxAmsduLength (7935);
  htCapabilities.SetMaxAmpduLength (65535);
  m_manager->AddStationHtCapabilities (Mac48Address ("00:00:00:00:00:02"), htCapabilities);

  /*
   * Create dummy packets of 1500 bytes and fill mac header fields that will be used for the tests.
   */
  Ptr<const Packet> pkt = Create<Packet> (1500);
  Ptr<Packet> currentAggregatedPacket = Create<Packet> ();
  WifiMacHeader hdr;
  hdr.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
  hdr.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);

  //-----------------------------------------------------------------------------------------------------

  /*
   * Test MSDU aggregation of two packets using MsduAggregator::GetNextAmsdu.
   * It checks whether aggregation succeeded:
   *      - returned packet should be different from 0;
   *      - A-MSDU frame size should be 3030 bytes (= 2 packets + headers + padding);
   *      - one packet should be removed from the queue (the other packet is removed later in MacLow::AggregateToAmpdu) .
   */
  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt, hdr));
  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt, hdr));

  WifiTxVector txVector = m_mac->GetBEQueue ()->GetLow ()->GetDataTxVector (Create<const WifiMacQueueItem> (pkt, hdr));

  Ptr<WifiMacQueueItem> item;
  item = m_mac->GetBEQueue ()->GetLow ()->GetMsduAggregator ()->GetNextAmsdu (hdr.GetAddr1 (), 0, txVector,
                                                                              currentAggregatedPacket->GetSize ());
  bool result = (item != 0);
  NS_TEST_EXPECT_MSG_EQ (result, true, "aggregation failed");
  NS_TEST_EXPECT_MSG_EQ (item->GetPacket ()->GetSize (), 3030, "wrong packet size");
  NS_TEST_EXPECT_MSG_EQ (m_mac->GetBEQueue ()->GetWifiMacQueue ()->GetNPackets (), 0, "aggregated packets not removed from the queue");

  //-----------------------------------------------------------------------------------------------------

  /*
   * Aggregation is refused when the maximum size is reached.
   * It checks whether MSDU aggregation has been rejected because the maximum MPDU size is set to 0 (returned packet should be equal to 0).
   * This test is needed to ensure that no packets are removed from the queue in
   * MsduAggregator::GetNextAmsdu, since aggregation will no occur in MacLow::AggregateToAmpdu.
   */
  m_mac->SetAttribute ("BE_MaxAmpduSize", UintegerValue (65535));

  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt, hdr));

  item = m_mac->GetBEQueue ()->GetLow ()->GetMsduAggregator ()->GetNextAmsdu (hdr.GetAddr1 (), 0, txVector,
                                                                              currentAggregatedPacket->GetSize ());
  result = (item != 0);
  NS_TEST_EXPECT_MSG_EQ (result, false, "maximum aggregated frame size check failed");

  //-----------------------------------------------------------------------------------------------------

  /*
   * Aggregation does not occur when there is no more packets in the queue.
   * It checks whether MSDU aggregation has been rejected because there is no packets ready in the queue (returned packet should be equal to 0).
   * This test is needed to ensure that there is no issue when the queue is empty.
   */
  m_mac->SetAttribute ("BE_MaxAmpduSize", UintegerValue (4095));

  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Remove (pkt);
  m_mac->GetBEQueue ()->GetWifiMacQueue ()->Remove (pkt);

  item = m_mac->GetBEQueue ()->GetLow ()->GetMsduAggregator ()->GetNextAmsdu (hdr.GetAddr1 (), 0, txVector,
                                                                              currentAggregatedPacket->GetSize ());

  result = (item != 0);
  NS_TEST_EXPECT_MSG_EQ (result, false, "aggregation failed to stop as queue is empty");

  //-----------------------------------------------------------------------------------------------------

  /*
   * Aggregation of MPDUs is stopped to prevent that the PPDU duration exceeds the TXOP limit.
   * In this test, the VI AC is used, which has a default TXOP limit of 3008 microseconds.
   */

  // Establish agreement.
  uint8_t tid = 5;
  MgtAddBaRequestHeader reqHdr;
  reqHdr.SetImmediateBlockAck ();
  reqHdr.SetTid (tid);
  reqHdr.SetBufferSize (64);
  reqHdr.SetTimeout (0);
  reqHdr.SetStartingSequence (0);
  m_mac->GetVIQueue ()->m_baManager->CreateAgreement (&reqHdr, hdr.GetAddr1 ());
  m_mac->GetVIQueue ()->m_baManager->NotifyAgreementEstablished (hdr.GetAddr1 (), tid, 0);

  m_mac->SetAttribute ("VI_MaxAmsduSize", UintegerValue (3050));  // max 2 MSDUs per A-MSDU
  m_mac->SetAttribute ("VI_MaxAmpduSize", UintegerValue (65535));
  m_manager->SetAttribute ("DataMode", StringValue ("HtMcs2"));  // 19.5Mbps

  pkt = Create<Packet> (1400);
  hdr.SetQosTid (tid);

  // Add 10 MSDUs to the EDCA queue
  for (uint8_t i = 0; i < 10; i++)
    {
      m_mac->GetVIQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt, hdr));
    }

  txVector = m_mac->GetVIQueue ()->GetLow ()->GetDataTxVector (Create<const WifiMacQueueItem> (pkt, hdr));
  Time txopLimit = m_mac->GetVIQueue ()->GetTxopLimit ();   // 3.008 ms

  // Compute the first MPDU to be aggregated in an A-MPDU. It must contain an A-MSDU
  // aggregating two MSDUs
  Ptr<WifiMacQueueItem> mpdu = m_mac->GetVIQueue ()->GetLow ()->GetMsduAggregator ()->GetNextAmsdu (hdr.GetAddr1 (), tid,
                                                                                                    txVector, 0, txopLimit);
  NS_TEST_EXPECT_MSG_EQ (m_mac->GetVIQueue ()->GetWifiMacQueue ()->GetNPackets (), 8, "There must be 8 MSDUs left in EDCA queue");

  auto mpduList = m_mac->GetVIQueue ()->GetLow ()->GetMpduAggregator ()->GetNextAmpdu (mpdu, txVector, txopLimit);

  // The maximum number of bytes that can be transmitted in a TXOP is (approximately, as we
  // do not consider that the preamble is transmitted at a different rate):
  // 19.5 Mbps * 3.008 ms = 7332 bytes
  // Given that the max A-MSDU size is set to 3050, an A-MSDU will contain two MSDUs and have
  // a size of 2 * 1400 (MSDU size) + 2 * 14 (A-MSDU subframe header size) + 2 (one padding field) = 2830 bytes
  // Hence, we expect that the A-MPDU will consist of:
  // - 2 MPDUs containing each an A-MSDU. The size of each MPDU is 2830 (A-MSDU) + 30 (header+trailer) = 2860
  // - 1 MPDU containing a single MSDU. The size of such MPDU is 1400 (MSDU) + 30 (header+trailer) = 1430
  // The size of the A-MPDU is 4 + 2860 + 4 + 2860 + 4 + 1430 = 7162
  NS_TEST_EXPECT_MSG_EQ (mpduList.empty (), false, "aggregation failed");
  NS_TEST_EXPECT_MSG_EQ (mpduList.size (), 3, "Unexpected number of MPDUs in the A-MPDU");
  NS_TEST_EXPECT_MSG_EQ (mpduList.at (0)->GetSize (), 2860, "Unexpected size of the first MPDU");
  NS_TEST_EXPECT_MSG_EQ (mpduList.at (1)->GetSize (), 2860, "Unexpected size of the second MPDU");
  NS_TEST_EXPECT_MSG_EQ (mpduList.at (2)->GetSize (), 1430, "Unexpected size of the first MPDU");
  NS_TEST_EXPECT_MSG_EQ (m_mac->GetVIQueue ()->GetWifiMacQueue ()->GetNPackets (), 5,
                         "Unexpected number of MSDUs left in the EDCA queue");

  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (mpduList);
  NS_TEST_EXPECT_MSG_EQ (psdu->GetSize (), 7162, "Unexpected size of the A-MPDU");

  Simulator::Destroy ();

  m_device->Dispose ();
  m_device = 0;
  htConfiguration = 0;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief 802.11ax aggregation test which permits 64 or 256 MPDUs in A-MPDU according to the negociated buffer size.
 */
class HeAggregationTest : public TestCase
{
public:
  HeAggregationTest ();

private:
  void DoRun (void);
  void DoRunSubTest (uint16_t bufferSize);
  Ptr<WifiNetDevice> m_device; ///<WifiNetDevice
  Ptr<StaWifiMac> m_mac; ///< Mac
  Ptr<YansWifiPhy> m_phy; ///< Phy
  Ptr<WifiRemoteStationManager> m_manager; ///< remote station manager
  ObjectFactory m_factory; ///< factory
};

HeAggregationTest::HeAggregationTest ()
  : TestCase ("Check the correctness of 802.11ax aggregation operations")
{
}

void
HeAggregationTest::DoRunSubTest (uint16_t bufferSize)
{
  /*
   * Create device and attach configurations.
   */
  m_device = CreateObject<WifiNetDevice> ();
  Ptr<HtConfiguration> htConfiguration = CreateObject<HtConfiguration> ();
  m_device->SetHtConfiguration (htConfiguration);
  Ptr<VhtConfiguration> vhtConfiguration = CreateObject<VhtConfiguration> ();
  m_device->SetVhtConfiguration (vhtConfiguration);
  Ptr<HeConfiguration> heConfiguration = CreateObject<HeConfiguration> ();
  m_device->SetHeConfiguration (heConfiguration);

  /*
   * Create and configure phy layer.
   */
  m_phy = CreateObject<YansWifiPhy> ();
  m_phy->SetDevice (m_device);
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  m_device->SetPhy (m_phy);

  /*
   * Create and configure manager.
   */
  m_factory = ObjectFactory ();
  m_factory.SetTypeId ("ns3::ConstantRateWifiManager");
  m_factory.Set ("DataMode", StringValue ("HeMcs11"));
  m_manager = m_factory.Create<WifiRemoteStationManager> ();
  m_manager->SetupPhy (m_phy);
  m_device->SetRemoteStationManager (m_manager);

  /*
   * Create and configure mac layer.
   */
  m_mac = CreateObject<StaWifiMac> ();
  m_mac->SetDevice (m_device);
  m_mac->SetWifiPhy (m_phy);
  m_mac->SetWifiRemoteStationManager (m_manager);
  m_mac->SetAddress (Mac48Address ("00:00:00:00:00:01"));
  m_mac->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  m_device->SetMac (m_mac);

  /*
   * Configure aggregation.
   */
  HeCapabilities heCapabilities;
  m_manager->AddStationHeCapabilities (Mac48Address ("00:00:00:00:00:02"), heCapabilities);

  /*
   * Create a dummy packet of 100 bytes and fill mac header fields.
   */
  Ptr<const Packet> pkt = Create<Packet> (100);
  Ptr<Packet> currentAggregatedPacket = Create<Packet> ();
  WifiMacHeader hdr;
  hdr.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
  hdr.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint16_t sequence = m_mac->m_txMiddle->GetNextSequenceNumberFor (&hdr);
  hdr.SetSequenceNumber (sequence);
  hdr.SetFragmentNumber (0);
  hdr.SetNoMoreFragments ();
  hdr.SetNoRetry ();

  /*
   * Establish agreement.
   */
  MgtAddBaRequestHeader reqHdr;
  reqHdr.SetImmediateBlockAck ();
  reqHdr.SetTid (0);
  reqHdr.SetBufferSize (bufferSize);
  reqHdr.SetTimeout (0);
  reqHdr.SetStartingSequence (0);
  m_mac->GetBEQueue ()->m_baManager->CreateAgreement (&reqHdr, hdr.GetAddr1 ());
  m_mac->GetBEQueue ()->m_baManager->NotifyAgreementEstablished (hdr.GetAddr1 (), 0, 0);

  /*
   * Test behavior when 300 packets are ready for transmission but negociated buffer size is 64
   */
  for (uint16_t i = 0; i < 300; i++)
    {
      Ptr<const Packet> pkt = Create<Packet> (100);
      WifiMacHeader hdr;

      hdr.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
      hdr.SetAddr2 (Mac48Address ("00:00:00:00:00:01"));
      hdr.SetType (WIFI_MAC_QOSDATA);
      hdr.SetQosTid (0);

      m_mac->GetBEQueue ()->GetWifiMacQueue ()->Enqueue (Create<WifiMacQueueItem> (pkt, hdr));
  }

  WifiTxVector txVector = m_mac->GetBEQueue ()->GetLow ()->GetDataTxVector (Create<const WifiMacQueueItem> (pkt, hdr));

  auto mpduList = m_mac->GetBEQueue ()->GetLow ()->GetMpduAggregator ()-> GetNextAmpdu (Create<WifiMacQueueItem> (pkt, hdr),
                                                                                        txVector);
  NS_TEST_EXPECT_MSG_EQ (mpduList.empty (), false, "MPDU aggregation failed");
  NS_TEST_EXPECT_MSG_EQ (mpduList.size (), bufferSize, "A-MPDU should countain " << bufferSize << " MPDUs");
  uint16_t expectedRemainingPacketsInQueue = 300 - bufferSize + 1;
  NS_TEST_EXPECT_MSG_EQ (m_mac->GetBEQueue ()->GetWifiMacQueue ()->GetNPackets (), expectedRemainingPacketsInQueue, "queue should contain 300 - "<< bufferSize - 1 << " = "<< expectedRemainingPacketsInQueue << " packets");

  Simulator::Destroy ();

  m_manager->Dispose ();
  m_manager = 0;

  m_device->Dispose ();
  m_device = 0;

  htConfiguration = 0;
  vhtConfiguration = 0;
  heConfiguration = 0;
}

void
HeAggregationTest::DoRun ()
{
  DoRunSubTest (64);
  DoRunSubTest (256);
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Wifi Aggregation Test Suite
 */
class WifiAggregationTestSuite : public TestSuite
{
public:
  WifiAggregationTestSuite ();
};

WifiAggregationTestSuite::WifiAggregationTestSuite ()
  : TestSuite ("wifi-aggregation", UNIT)
{
  AddTestCase (new AmpduAggregationTest, TestCase::QUICK);
  AddTestCase (new TwoLevelAggregationTest, TestCase::QUICK);
  AddTestCase (new HeAggregationTest, TestCase::QUICK);
}

static WifiAggregationTestSuite g_wifiAggregationTestSuite; ///< the test suite
