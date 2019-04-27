/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Washington
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
 * Authors: Sébastien Deronne <sebastien.deronne@gmail.com>
 *          Scott Carpenter <scarpenter44@windstream.net>
 */

#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/config.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/constant-obss-pd-algorithm.h"
#include "ns3/he-configuration.h"
#include "ns3/wifi-utils.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("InterBssTestSuite");

uint32_t
ConvertContextToNodeId (std::string context)
{
  std::string sub = context.substr (10);
  uint32_t pos = sub.find ("/Device");
  uint32_t nodeId = atoi (sub.substr (0, pos).c_str ());
  return nodeId;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Wifi Test
 *
 * This test case tests the transmission of inter-BSS cases
 * and verify behavior of 11ax OBSS_PD spatial reuse.
 *
 * The topology for this test case is made of two networks, each with one AP and one STA:
 *
 *  STA1  --d1--  AP1  --d2--  AP2  --d3-- STA2
 *  RX1           TX1          TX2         RX2
 *
 * Main parameters:
 *  OBSS_PD level = -72dbm
 *  Received Power by TX1 from TX2 = [-62dbm, -82dbm]
 *  Received SINR by RX1 from TX1 > 3dB (enough to pass MCS0 reception)
 *  Received SINR by RX2 from TX2 > 3dB (enough to pass MCS0 reception)
 *  TX1/RX1 BSS Color = 1
 *  TX2/RX2 transmission PPDU BSS Color = [2 0]
 *  PHY = 11ax, MCS 0, 80MHz
 */

class TestInterBssConstantObssPdAlgo : public TestCase
{
public:
  TestInterBssConstantObssPdAlgo ();

  virtual void DoRun (void);

private:
  /**
   * Send one packet function
   * \param tx_dev the transmitting device
   * \param rx_dev the receiving device
   * \param payloadSize the payload size
   */
  void SendOnePacket (Ptr<WifiNetDevice> tx_dev, Ptr<WifiNetDevice> rx_dev, uint32_t payloadSize);

  /**
   * Allocate the node positions
   * \param d1 distance d1 (in meters)
   * \param d2 distance d2 (in meters)
   * \param d3 distance d3 (in meters)
   */
  Ptr<ListPositionAllocator> AllocatePositions (double d1, double d2, double d3);

  void SetExpectedTxPower (double txPowerDbm);

  /**
   * Setup the simulation
   */
  void SetupSimulation ();

  /**
   * Check the results
   */
  void CheckResults ();

  /**
   * Reset the results
   */
  void ResetResults ();

  /**
   * Run one function
   */
  void RunOne ();

  /**
   * Check if the Phy State for a device is an expected value
   */
  void CheckPhyState (Ptr<WifiNetDevice> device, WifiPhyState expectedState);

  /**
   * Notify Phy transmit begin
   * \param context the context
   * \param p the packet
   * \param txPowerW the tx power
   */
  void NotifyPhyTxBegin (std::string context, Ptr<const Packet> p, double txPowerW);

  /**
   * Notify Phy receive endsn
   * \param context the context
   * \param p the packet
   */
  void NotifyPhyRxEnd (std::string context, Ptr<const Packet> p);

  unsigned int m_numSta1PacketsSent; ///< number of sent packets from STA1
  unsigned int m_numSta2PacketsSent; ///< number of sent packets from STA2
  unsigned int m_numAp1PacketsSent; ///< number of sent packets from AP1
  unsigned int m_numAp2PacketsSent; ///< number of sent packets from AP2

  unsigned int m_numSta1PacketsReceived; ///< number of received packets from STA1
  unsigned int m_numSta2PacketsReceived; ///< number of received packets from STA2
  unsigned int m_numAp1PacketsReceived; ///< number of received packets from AP1
  unsigned int m_numAp2PacketsReceived; ///< number of received packets from AP2

  unsigned int m_payloadSize1; ///< size in bytes of packet payload in BSS 1
  unsigned int m_payloadSize2; ///< size in bytes of packet payload in BSS 2

  NetDeviceContainer m_staDevices;
  NetDeviceContainer m_apDevices;

  double m_txPowerDbm;
  double m_obssPdLevelDbm;
  double m_obssRxPowerDbm;
  double m_expectedTxPowerDbm;

  uint8_t m_bssColor1;
  uint8_t m_bssColor2;
};

TestInterBssConstantObssPdAlgo::TestInterBssConstantObssPdAlgo ()
  : TestCase ("InterBssConstantObssPd"),
    m_numSta1PacketsSent (0),
    m_numSta2PacketsSent (0),
    m_numAp1PacketsSent (0),
    m_numAp2PacketsSent (0),
    m_numSta1PacketsReceived (0),
    m_numSta2PacketsReceived (0),
    m_numAp1PacketsReceived (0),
    m_numAp2PacketsReceived (0),
    m_payloadSize1 (1000),
    m_payloadSize2 (1500),
    m_txPowerDbm (15),
    m_obssPdLevelDbm (-72),
    m_obssRxPowerDbm (-82),
    m_expectedTxPowerDbm (15),
    m_bssColor1 (1),
    m_bssColor2 (2)
{
}

Ptr<ListPositionAllocator>
TestInterBssConstantObssPdAlgo::AllocatePositions (double d1, double d2, double d3)
{
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (d1, 0.0, 0.0));  // AP1
  positionAlloc->Add (Vector (d1 + d2, 0.0, 0.0));  // AP2
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));  // STA1
  positionAlloc->Add (Vector (d1 + d2 + d3, 0.0, 0.0));  // STA2

  return positionAlloc;
}

void
TestInterBssConstantObssPdAlgo::SetupSimulation ()
{
  Ptr<WifiNetDevice> ap_device1 = DynamicCast<WifiNetDevice> (m_apDevices.Get (0));
  Ptr<WifiNetDevice> ap_device2 = DynamicCast<WifiNetDevice> (m_apDevices.Get (1));
  Ptr<WifiNetDevice> sta_device1 = DynamicCast<WifiNetDevice> (m_staDevices.Get (0));
  Ptr<WifiNetDevice> sta_device2 = DynamicCast<WifiNetDevice> (m_staDevices.Get (1));

  bool expectPhyReset = (m_bssColor1 != 0) && (m_bssColor2 != 0) && (m_obssPdLevelDbm >= m_obssRxPowerDbm);

  // AP1 sends packet #1 after 0.25s. The purpose is to have addba handshake established.
  Simulator::Schedule (Seconds (0.25), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, ap_device1, sta_device1, m_payloadSize1);
  // STA1 sends packet #2 after 0.5s. The purpose is to have addba handshake established.
  Simulator::Schedule (Seconds (0.5), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, sta_device1, ap_device1, m_payloadSize1);

  // AP2 sends packet #3 after 0.75s. The purpose is to have addba handshake established.
  Simulator::Schedule (Seconds (0.75), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, ap_device2, sta_device2, m_payloadSize2);
  // STA2 sends packet #4 after 1.0s. The purpose is to have addba handshake established.
  Simulator::Schedule (Seconds (1), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, sta_device2, ap_device2, m_payloadSize2);

  // AP2 sends packet #5 0.5s later.
  Simulator::Schedule (Seconds (1.5), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, ap_device2, sta_device2, m_payloadSize2);
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (1), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, ap_device2, WifiPhyState::TX);
  // All other PHYs should have stay idle until 4us (preamble detection time).
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (2), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device1, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (2), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device2, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (2), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, ap_device1, WifiPhyState::IDLE);
  // All PHYs should be reeiving the PHY header if preamble has been detected (always the case in this test).
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (10), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (10), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device2, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (10), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, ap_device1, WifiPhyState::RX);
  // PHYs of AP1 and STA1 should be idle if it was reset by OBSS PD, otherwise they should be receiving
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (50), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device1, expectPhyReset ? WifiPhyState::IDLE : WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (50), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, ap_device1, expectPhyReset ? WifiPhyState::IDLE : WifiPhyState::RX);
  // STA2 should be receiving
  Simulator::Schedule (Seconds (1.5) + MicroSeconds (50), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device2, WifiPhyState::RX);

  // AP2 sends another packet #6 0.1s later.
  Simulator::Schedule (Seconds (1.6), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, ap_device2, sta_device2, m_payloadSize2);
  // STA1 sends a packet #7 100us later. Even though AP2 is still transmitting, STA1 can transmit simultaneously if it's PHY was reset by OBSS PD SR.
  Simulator::Schedule (Seconds (1.6) + MicroSeconds (100), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, sta_device1, ap_device1, m_payloadSize1);
  if (expectPhyReset)
    {
      // In this case, we check the TX power is restricted
      double expectedTxPower = std::min (m_txPowerDbm, 21 - (m_obssPdLevelDbm + 82));
      Simulator::Schedule (Seconds (1.6) + MicroSeconds (100), &TestInterBssConstantObssPdAlgo::SetExpectedTxPower, this, expectedTxPower);
    }
  // Check simultaneous transmissions
  Simulator::Schedule (Seconds (1.6) + MicroSeconds (350), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device1, expectPhyReset ? WifiPhyState::TX : WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.6) + MicroSeconds (350), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, ap_device1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.6) + MicroSeconds (350), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, sta_device2, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.6) + MicroSeconds (350), &TestInterBssConstantObssPdAlgo::CheckPhyState, this, ap_device2, WifiPhyState::TX);

  // AP2 sends another packet #8 0.1s later.
  Simulator::Schedule (Seconds (1.7), &TestInterBssConstantObssPdAlgo::SetExpectedTxPower, this, m_txPowerDbm);
  Simulator::Schedule (Seconds (1.7), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, ap_device2, sta_device2, m_payloadSize2);
  // STA1 sends a packet #9 0.1S later. Power retriction should not be applied.
  Simulator::Schedule (Seconds (1.8), &TestInterBssConstantObssPdAlgo::SendOnePacket, this, sta_device1, ap_device1, m_payloadSize1);

  Simulator::Stop (Seconds (1.9));
}

void
TestInterBssConstantObssPdAlgo::ResetResults ()
{
  m_numSta1PacketsSent = 0;
  m_numSta2PacketsSent = 0;
  m_numAp1PacketsSent = 0;
  m_numAp2PacketsSent = 0;
  m_numSta1PacketsReceived = 0;
  m_numSta2PacketsReceived = 0;
  m_numAp1PacketsReceived = 0;
  m_numAp2PacketsReceived = 0;
  m_expectedTxPowerDbm = m_txPowerDbm;
}

void
TestInterBssConstantObssPdAlgo::CheckResults ()
{
  NS_TEST_ASSERT_MSG_EQ (m_numSta1PacketsSent, 3, "The number of packets sent by STA1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_numSta2PacketsSent, 1, "The number of packets sent by STA2 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_numAp1PacketsSent, 1, "The number of packets sent by AP1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_numAp2PacketsSent, 4, "The number of packets sent by AP2 is not correct!");

  NS_TEST_ASSERT_MSG_EQ (m_numSta1PacketsReceived, 1, "The number of packets received by STA1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_numSta2PacketsReceived, 4, "The number of packets received by STA2 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_numAp1PacketsReceived, 3, "The number of packets received by AP1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_numAp2PacketsReceived, 1, "The number of packets received by AP2 is not correct!");
}

void
TestInterBssConstantObssPdAlgo::NotifyPhyTxBegin (std::string context, Ptr<const Packet> p, double txPowerW)
{
  uint32_t idx = ConvertContextToNodeId (context);
  uint32_t pktSize = p->GetSize () - 38;
  if ((idx == 0) && (pktSize == m_payloadSize1))
    {
      m_numSta1PacketsSent++;
      NS_TEST_EXPECT_MSG_EQ (TestDoubleIsEqual (WToDbm (txPowerW), m_expectedTxPowerDbm, 1e-12), true, "Tx power is not correct!");
    }
  else if ((idx == 1) && (pktSize == m_payloadSize2))
    {
      m_numSta2PacketsSent++;
      NS_TEST_EXPECT_MSG_EQ (TestDoubleIsEqual (WToDbm (txPowerW), m_expectedTxPowerDbm, 1e-12), true, "Tx power is not correct!");
    }
  else if ((idx == 2) && (pktSize == m_payloadSize1))
    {
      m_numAp1PacketsSent++;
      NS_TEST_EXPECT_MSG_EQ (TestDoubleIsEqual (WToDbm (txPowerW), m_expectedTxPowerDbm, 1e-12), true, "Tx power is not correct!");
    }
  else if ((idx == 3) && (pktSize == m_payloadSize2))
    {
      m_numAp2PacketsSent++;
      NS_TEST_EXPECT_MSG_EQ (TestDoubleIsEqual (WToDbm (txPowerW), m_expectedTxPowerDbm, 1e-12), true, "Tx power is not correct!");
    }
}

void
TestInterBssConstantObssPdAlgo::NotifyPhyRxEnd (std::string context, Ptr<const Packet> p)
{
  uint32_t idx = ConvertContextToNodeId (context);
  uint32_t pktSize = p->GetSize () - 38;
  if ((idx == 0) && (pktSize == m_payloadSize1))
    {
      m_numSta1PacketsReceived++;
    }
  else if ((idx == 1) && (pktSize == m_payloadSize2))
    {
      m_numSta2PacketsReceived++;
    }
  else if ((idx == 2) && (pktSize == m_payloadSize1))
    {
      m_numAp1PacketsReceived++;
    }
  else if ((idx == 3) && (pktSize == m_payloadSize2))
    {
      m_numAp2PacketsReceived++;
    }
}

void
TestInterBssConstantObssPdAlgo::SendOnePacket (Ptr<WifiNetDevice> tx_dev, Ptr<WifiNetDevice> rx_dev, uint32_t payloadSize)
{
  Ptr<Packet> p = Create<Packet> (payloadSize);
  tx_dev->Send (p, rx_dev->GetAddress (), 1);
}

void
TestInterBssConstantObssPdAlgo::SetExpectedTxPower (double txPowerDbm)
{
  m_expectedTxPowerDbm = txPowerDbm;
}

void
TestInterBssConstantObssPdAlgo::CheckPhyState (Ptr<WifiNetDevice> device, WifiPhyState expectedState)
{
  WifiPhyState currentState;
  PointerValue ptr;
  Ptr<WifiPhy> phy = DynamicCast<WifiPhy> (device->GetPhy ());
  phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}

void
TestInterBssConstantObssPdAlgo::RunOne (void)
{
  ResetResults ();

  Config::Set ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/BE_MaxAmpduSize", UintegerValue (0));

  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (2);

  NodeContainer wifiApNodes;
  wifiApNodes.Create (2);

  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)

  SpectrumWifiPhyHelper phy = SpectrumWifiPhyHelper::Default ();
  Ptr<MultiModelSpectrumChannel> channel = CreateObject<MultiModelSpectrumChannel> ();
  channel->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());
  channel->AddPropagationLossModel (lossModel);
  phy.SetChannel (channel);
  phy.Set ("TxPowerStart", DoubleValue (m_txPowerDbm));
  phy.Set ("TxPowerEnd", DoubleValue (m_txPowerDbm));

  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue ("HeMcs0"),
                                "ControlMode", StringValue ("HeMcs0"));

  wifi.SetObssPdAlgorithm ("ns3::ConstantObssPdAlgorithm",
                           "ObssPdLevel", DoubleValue (m_obssPdLevelDbm));

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid));
  m_staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));
  m_apDevices = wifi.Install (phy, mac, wifiApNodes);

  for (uint32_t i = 0; i < m_apDevices.GetN (); i++)
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_apDevices.Get (i));
      Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
      if (i == 0)
        {
          heConfiguration->SetAttribute ("BssColor", UintegerValue (m_bssColor1));
        }
      else
        {
          heConfiguration->SetAttribute ("BssColor", UintegerValue (m_bssColor2));
        }
    }
  for (uint32_t i = 0; i < m_staDevices.GetN (); i++)
    {
      Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (m_staDevices.Get (i));
      Ptr<HeConfiguration> heConfiguration = device->GetHeConfiguration ();
    }

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = AllocatePositions (10, 50, 10); //distances do not really matter since we set RSSI per TX-RX pair to have full control
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNodes);
  mobility.Install (wifiStaNodes);

  lossModel->SetLoss (wifiStaNodes.Get (0)->GetObject<MobilityModel> (), wifiApNodes.Get (0)->GetObject<MobilityModel> (), m_txPowerDbm + 30); //Low attenuation for IBSS transmissions
  lossModel->SetLoss (wifiStaNodes.Get (1)->GetObject<MobilityModel> (), wifiApNodes.Get (1)->GetObject<MobilityModel> (), m_txPowerDbm + 30); //Low attenuation for IBSS transmissions
  lossModel->SetLoss (wifiStaNodes.Get (1)->GetObject<MobilityModel> (), wifiApNodes.Get (0)->GetObject<MobilityModel> (), m_txPowerDbm - m_obssRxPowerDbm); //Force received RSSI to be equal to m_obssRxPowerDbm
  lossModel->SetLoss (wifiStaNodes.Get (0)->GetObject<MobilityModel> (), wifiApNodes.Get (1)->GetObject<MobilityModel> (), m_txPowerDbm - m_obssRxPowerDbm); //Force received RSSI to be equal to m_obssRxPowerDbm
  lossModel->SetLoss (wifiApNodes.Get (0)->GetObject<MobilityModel> (), wifiApNodes.Get (1)->GetObject<MobilityModel> (), m_txPowerDbm - m_obssRxPowerDbm); //Force received RSSI to be equal to m_obssRxPowerDbm

  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxBegin", MakeCallback (&TestInterBssConstantObssPdAlgo::NotifyPhyTxBegin, this));
  Config::Connect ("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxEnd", MakeCallback (&TestInterBssConstantObssPdAlgo::NotifyPhyRxEnd, this));

  SetupSimulation ();

  Simulator::Run ();
  Simulator::Destroy ();

  CheckResults ();
}

void
TestInterBssConstantObssPdAlgo::DoRun (void)
{
  //Test case 1: CCA CS Threshold = m_obssRxPowerDbm < m_obssPdLevelDbm
  m_obssPdLevelDbm = -72;
  m_obssRxPowerDbm = -82;
  m_bssColor1 = 1;
  m_bssColor2 = 2;
  RunOne ();

  //Test case 2: CCA CS Threshold < m_obssPdLevelDbm < m_obssRxPowerDbm
  m_obssPdLevelDbm = -72;
  m_obssRxPowerDbm = -62;
  m_bssColor1 = 1;
  m_bssColor2 = 2;
  RunOne ();

  //Test case 3: CCA CS Threshold = < m_obssPdLevelDbm = m_obssRxPowerDbm
  m_obssPdLevelDbm = -72;
  m_obssRxPowerDbm = -72;
  m_bssColor1 = 1;
  m_bssColor2 = 2;
  RunOne ();

  //Test case 4: CCA CS Threshold = m_obssRxPowerDbm < m_obssPdLevelDbm with BSS color 2 set to 0
  m_obssPdLevelDbm = -72;
  m_obssRxPowerDbm = -82;
  m_bssColor1 = 1;
  m_bssColor2 = 0;
  RunOne ();

  //Test case 5: CCA CS Threshold = m_obssRxPowerDbm < m_obssPdLevelDbm with BSS color 1 set to 0
  m_obssPdLevelDbm = -72;
  m_obssRxPowerDbm = -82;
  m_bssColor1 = 0;
  m_bssColor2 = 2;
  RunOne ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Inter BSS Test Suite
 */

class InterBssTestSuite : public TestSuite
{
public:
  InterBssTestSuite ();
};

InterBssTestSuite::InterBssTestSuite ()
  : TestSuite ("wifi-inter-bss", UNIT)
{
  AddTestCase (new TestInterBssConstantObssPdAlgo, TestCase::QUICK);
}

// Do not forget to allocate an instance of this TestSuite
static InterBssTestSuite interBssTestSuite;
