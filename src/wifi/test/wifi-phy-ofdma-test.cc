/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 University of Washington
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

#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-ppdu.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiPhyOfdmaTest");

class OfdmaSpectrumWifiPhy : public SpectrumWifiPhy
{
public:
  /**
   * Constructor
   *
   * \param staId the ID of the STA to which this PHY belongs to
   */
  OfdmaSpectrumWifiPhy (uint16_t staId);
  virtual ~OfdmaSpectrumWifiPhy ();

private:
  /**
   * Return the STA ID that has been assigned to the station this PHY belongs to.
   * This is typically called for MU PPDUs, in order to pick the correct PSDU.
   *
   * \return the STA ID
   */
  uint16_t GetStaId (void) const override;

  uint16_t m_staId; ///< ID of the STA to which this PHY belongs to
};

OfdmaSpectrumWifiPhy::OfdmaSpectrumWifiPhy (uint16_t staId)
  : SpectrumWifiPhy (),
    m_staId (staId)
{
}

OfdmaSpectrumWifiPhy::~OfdmaSpectrumWifiPhy()
{
}

uint16_t
OfdmaSpectrumWifiPhy::GetStaId (void) const
{
  return m_staId;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief DL-OFDMA PHY reception test
 */
class TestDlOfdmaReception : public TestCase
{
public:
  TestDlOfdmaReception ();
  virtual ~TestDlOfdmaReception ();

  /**
   * Receive success function for STA 1
   * \param psdu the PSDU
   * \param snr the SNR
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void RxSuccessSta1 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu);
  /**
   * Receive success function for STA 2
   * \param psdu the PSDU
   * \param snr the SNR
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void RxSuccessSta2 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu);
  /**
   * Receive success function for STA 3
   * \param psdu the PSDU
   * \param snr the SNR
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void RxSuccessSta3 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu);

  /**
   * Check the results
   */
  void CheckResults ();

  uint32_t m_countRxPacketsSta1; ///< count RX packets for STA 1
  uint32_t m_countRxPacketsSta2; ///< count RX packets for STA 2
  uint32_t m_countRxPacketsSta3; ///< count RX packets for STA 3
  uint32_t m_countRxBytesSta1;   ///< count RX bytes for STA 1
  uint32_t m_countRxBytesSta2;   ///< count RX bytes for STA 2
  uint32_t m_countRxBytesSta3;   ///< count RX bytes for STA 3

protected:
  virtual void DoSetup (void);
  Ptr<SpectrumWifiPhy> m_phyAp;   ///< PHY of AP
  Ptr<OfdmaSpectrumWifiPhy> m_phySta1; ///< PHY of STA 1
  Ptr<OfdmaSpectrumWifiPhy> m_phySta2; ///< PHY of STA 2
  Ptr<OfdmaSpectrumWifiPhy> m_phySta3; ///< PHY of STA 3
  /**
   * Send MU-PPDU function
   * \param rxStaId1 the ID of the recipient STA for the first PSDU
   * \param rxStaId2 the ID of the recipient STA for the second PSDU
   */
  void SendMuPpdu (uint16_t rxStaId1, uint16_t rxStaId2);

private:
  virtual void DoRun (void);

  /**
   * Schedule now to check  the PHY state
   * \param phy the PHY
   * \param expectedState the expected state of the PHY
   */
  void CheckPhyState (Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);
  /**
   * Check the PHY state now
   * \param phy the PHY
   * \param expectedState the expected state of the PHY
   */
  void DoCheckPhyState (Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState);
};

TestDlOfdmaReception::TestDlOfdmaReception ()
  : TestCase ("DL-OFDMA PHY reception test"),
    m_countRxPacketsSta1 (0),
    m_countRxPacketsSta2 (0),
    m_countRxPacketsSta3 (0),
    m_countRxBytesSta1 (0),
    m_countRxBytesSta2 (0),
    m_countRxBytesSta3 (0)
{
}

void
TestDlOfdmaReception::SendMuPpdu (uint16_t rxStaId1, uint16_t rxStaId2)
{
  NS_LOG_FUNCTION (this << rxStaId1 << rxStaId2);
  WifiConstPsduMap psdus;
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_MU, 800, 1, 1, 0, 20, false, false);

  HeRu::RuSpec ru1;
  ru1.primary80MHz = false;
  ru1.ruType = HeRu::RU_106_TONE;
  ru1.index = 1;
  txVector.SetRu (ru1, rxStaId1);
  txVector.SetMode (WifiPhy::GetHeMcs7 (), rxStaId1);
  txVector.SetNss (1, rxStaId1);

  HeRu::RuSpec ru2;
  ru2.primary80MHz = false;
  ru2.ruType = HeRu::RU_106_TONE;
  ru2.index = 2;
  txVector.SetRu (ru2, rxStaId2);
  txVector.SetMode (WifiPhy::GetHeMcs9 (), rxStaId2);
  txVector.SetNss (1, rxStaId2);

  Ptr<Packet> pkt1 = Create<Packet> (1000);
  WifiMacHeader hdr1;
  hdr1.SetType (WIFI_MAC_QOSDATA);
  hdr1.SetQosTid (0);
  hdr1.SetAddr1 (Mac48Address ("00:00:00:00:00:01"));
  hdr1.SetSequenceNumber (1);
  Ptr<WifiPsdu> psdu1 = Create<WifiPsdu> (pkt1, hdr1);
  psdus.insert (std::make_pair (rxStaId1, psdu1));

  Ptr<Packet> pkt2 = Create<Packet> (1500);
  WifiMacHeader hdr2;
  hdr2.SetType (WIFI_MAC_QOSDATA);
  hdr2.SetQosTid (0);
  hdr2.SetAddr1 (Mac48Address ("00:00:00:00:00:02"));
  hdr2.SetSequenceNumber (2);
  Ptr<WifiPsdu> psdu2 = Create<WifiPsdu> (pkt2, hdr2);
  psdus.insert (std::make_pair (rxStaId2, psdu2));

  m_phyAp->Send (psdus, txVector);
}

TestDlOfdmaReception::~TestDlOfdmaReception ()
{
  m_phyAp = 0;
  m_phySta1 = 0;
  m_phySta2 = 0;
  m_phySta3 = 0;
}

void
TestDlOfdmaReception::RxSuccessSta1 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> /*statusPerMpdu*/)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector);
  m_countRxPacketsSta1++;
  m_countRxBytesSta1 += (psdu->GetSize () - 30);
}

void
TestDlOfdmaReception::RxSuccessSta2 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> /*statusPerMpdu*/)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector);
  m_countRxPacketsSta2++;
  m_countRxBytesSta2 += (psdu->GetSize () - 30);
}

void
TestDlOfdmaReception::RxSuccessSta3 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> /*statusPerMpdu*/)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector);
  m_countRxPacketsSta3++;
  m_countRxBytesSta3 += (psdu->GetSize () - 30);
}

void
TestDlOfdmaReception::CheckResults ()
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxPacketsSta1, 2, "The number of packets received by STA1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxPacketsSta2, 1, "The number of packets received by STA2 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxPacketsSta3, 1, "The number of packets received by STA3 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxBytesSta1, 2000, "The number of bytes received by STA1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxBytesSta2, 1500, "The number of bytes received by STA2 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxBytesSta3, 1500, "The number of bytes received by STA3 is not correct!");
}

void
TestDlOfdmaReception::CheckPhyState (Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
  //This is needed to make sure PHY state will be checked as the last event if a state change occured at the exact same time as the check
  Simulator::ScheduleNow (&TestDlOfdmaReception::DoCheckPhyState, this, phy, expectedState);
}

void
TestDlOfdmaReception::DoCheckPhyState (Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
  WifiPhyState currentState;
  PointerValue ptr;
  phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_LOG_FUNCTION (this << currentState);
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}

void
TestDlOfdmaReception::DoSetup (void)
{
  uint16_t channelWidth = 20; // MHz
  uint32_t frequency = 5180; // MHz

  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel> ();
  lossModel->SetFrequency (frequency * 1e6);
  spectrumChannel->AddPropagationLossModel (lossModel);
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  spectrumChannel->SetPropagationDelayModel (delayModel);
  
  Ptr<Node> apNode = CreateObject<Node> ();
  Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice> ();
  m_phyAp = CreateObject<SpectrumWifiPhy> ();
  m_phyAp->CreateWifiSpectrumPhyInterface (apDev);
  m_phyAp->ConfigureStandardAndBand (WIFI_PHY_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phyAp->SetErrorRateModel (error);
  m_phyAp->SetFrequency (frequency);
  m_phyAp->SetChannelWidth (channelWidth);
  m_phyAp->SetDevice (apDev);
  m_phyAp->SetChannel (spectrumChannel);
  Ptr<ConstantPositionMobilityModel> apMobility = CreateObject<ConstantPositionMobilityModel> ();
  m_phyAp->SetMobility (apMobility);
  apDev->SetPhy (m_phyAp);
  apNode->AggregateObject (apMobility);
  apNode->AddDevice (apDev);

  Ptr<Node> sta1Node = CreateObject<Node> ();
  Ptr<WifiNetDevice> sta1Dev = CreateObject<WifiNetDevice> ();
  m_phySta1 = CreateObject<OfdmaSpectrumWifiPhy> (1);
  m_phySta1->CreateWifiSpectrumPhyInterface (sta1Dev);
  m_phySta1->ConfigureStandardAndBand (WIFI_PHY_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);
  m_phySta1->SetErrorRateModel (error);
  m_phySta1->SetFrequency (frequency);
  m_phySta1->SetChannelWidth (channelWidth);
  m_phySta1->SetDevice (sta1Dev);
  m_phySta1->SetChannel (spectrumChannel);
  m_phySta1->SetReceiveOkCallback (MakeCallback (&TestDlOfdmaReception::RxSuccessSta1, this));
  Ptr<ConstantPositionMobilityModel> sta1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  m_phySta1->SetMobility (sta1Mobility);
  sta1Dev->SetPhy (m_phySta1);
  sta1Node->AggregateObject (sta1Mobility);
  sta1Node->AddDevice (sta1Dev);

  Ptr<Node> sta2Node = CreateObject<Node> ();
  Ptr<WifiNetDevice> sta2Dev = CreateObject<WifiNetDevice> ();
  m_phySta2 = CreateObject<OfdmaSpectrumWifiPhy> (2);
  m_phySta2->CreateWifiSpectrumPhyInterface (sta2Dev);
  m_phySta2->ConfigureStandardAndBand (WIFI_PHY_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);
  m_phySta2->SetErrorRateModel (error);
  m_phySta2->SetFrequency (frequency);
  m_phySta2->SetChannelWidth (channelWidth);
  m_phySta2->SetDevice (sta2Dev);
  m_phySta2->SetChannel (spectrumChannel);
  m_phySta2->SetReceiveOkCallback (MakeCallback (&TestDlOfdmaReception::RxSuccessSta2, this));
  Ptr<ConstantPositionMobilityModel> sta2Mobility = CreateObject<ConstantPositionMobilityModel> ();
  m_phySta2->SetMobility (sta2Mobility);
  sta2Dev->SetPhy (m_phySta2);
  sta2Node->AggregateObject (sta2Mobility);
  sta2Node->AddDevice (sta2Dev);

  Ptr<Node> sta3Node = CreateObject<Node> ();
  Ptr<WifiNetDevice> sta3Dev = CreateObject<WifiNetDevice> ();
  m_phySta3 = CreateObject<OfdmaSpectrumWifiPhy> (3);
  m_phySta3->CreateWifiSpectrumPhyInterface (sta3Dev);
  m_phySta3->ConfigureStandardAndBand (WIFI_PHY_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);
  m_phySta3->SetErrorRateModel (error);
  m_phySta3->SetFrequency (frequency);
  m_phySta3->SetChannelWidth (channelWidth);
  m_phySta3->SetDevice (sta3Dev);
  m_phySta3->SetChannel (spectrumChannel);
  m_phySta3->SetReceiveOkCallback (MakeCallback (&TestDlOfdmaReception::RxSuccessSta3, this));
  Ptr<ConstantPositionMobilityModel> sta3Mobility = CreateObject<ConstantPositionMobilityModel> ();
  m_phySta3->SetMobility (sta3Mobility);
  sta3Dev->SetPhy (m_phySta3);
  sta3Node->AggregateObject (sta3Mobility);
  sta3Node->AddDevice (sta3Dev);
}

void
TestDlOfdmaReception::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 0;
  m_phyAp->AssignStreams (streamNumber);
  m_phySta1->AssignStreams (streamNumber);
  m_phySta2->AssignStreams (streamNumber);
  m_phySta3->AssignStreams (streamNumber);

  //Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
  //Each STA should receive its PSDU.
  Simulator::Schedule (Seconds (1.0), &TestDlOfdmaReception::SendMuPpdu, this, 1, 2);

  //Since it takes 306.4us to transmit the largest packet,
  //all 3 PHYs should be back to IDLE at the same time time 306.4us
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (306399), &TestDlOfdmaReception::CheckPhyState, this, m_phySta1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (306399), &TestDlOfdmaReception::CheckPhyState, this, m_phySta2, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (306399), &TestDlOfdmaReception::CheckPhyState, this, m_phySta3, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (306400), &TestDlOfdmaReception::CheckPhyState, this, m_phySta1, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (306400), &TestDlOfdmaReception::CheckPhyState, this, m_phySta2, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (306400), &TestDlOfdmaReception::CheckPhyState, this, m_phySta3, WifiPhyState::IDLE);

  //Send MU PPDU with two PSDUs addressed to STA 1 and STA 3:
  //STA 1 should receive its PSDU, whereas STA 2 should not receive any PSDU
  //but should keep its PHY busy during all PPDU duration.
  Simulator::Schedule (Seconds (2.0), &TestDlOfdmaReception::SendMuPpdu, this, 1, 3);

  //Since it takes 306.4us to transmit the largest packet,
  //all 3 PHYs should be back to IDLE at the same time time 306.4us,
  //even the PHY that has no PSDU addressed to it.
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (306399), &TestDlOfdmaReception::CheckPhyState, this, m_phySta1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (306399), &TestDlOfdmaReception::CheckPhyState, this, m_phySta2, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (306399), &TestDlOfdmaReception::CheckPhyState, this, m_phySta3, WifiPhyState::RX);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (306400), &TestDlOfdmaReception::CheckPhyState, this, m_phySta1, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (306400), &TestDlOfdmaReception::CheckPhyState, this, m_phySta2, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (306400), &TestDlOfdmaReception::CheckPhyState, this, m_phySta3, WifiPhyState::IDLE);

  Simulator::Run ();
  Simulator::Destroy ();

  CheckResults ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi PHY OFDMA Test Suite
 */
class WifiPhyOfdmaTestSuite : public TestSuite
{
public:
  WifiPhyOfdmaTestSuite ();
};

WifiPhyOfdmaTestSuite::WifiPhyOfdmaTestSuite ()
  : TestSuite ("wifi-phy-ofdma", UNIT)
{
  AddTestCase (new TestDlOfdmaReception, TestCase::QUICK);
}

static WifiPhyOfdmaTestSuite wifiPhyOfdmaTestSuite; ///< the test suite
