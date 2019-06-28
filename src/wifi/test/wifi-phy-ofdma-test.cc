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
#include "ns3/waveform-generator.h"
#include "ns3/non-communicating-net-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiPhyOfdmaTest");

static const uint32_t DEFAULT_FREQUENCY = 5180; // MHz
static const uint16_t DEFAULT_CHANNEL_WIDTH = 20; // MHz

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
 * \brief DL-OFDMA PHY test
 */
class TestDlOfdmaPhyTransmission : public TestCase
{
public:
  TestDlOfdmaPhyTransmission ();
  virtual ~TestDlOfdmaPhyTransmission ();

private:
  virtual void DoSetup (void);
  virtual void DoRun (void);

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
   * Receive failure function for STA 1
   * \param psdu the PSDU
   */
  void RxFailureSta1 (Ptr<WifiPsdu> psdu);
  /**
   * Receive failure function for STA 2
   * \param psdu the PSDU
   */
  void RxFailureSta2 (Ptr<WifiPsdu> psdu);
  /**
   * Receive failure function for STA 3
   * \param psdu the PSDU
   */
  void RxFailureSta3 (Ptr<WifiPsdu> psdu);

  /**
   * Check the results for STA 1
   * \param expectedRxSuccess the expected number of RX success
   * \param expectedRxFailure the expected number of RX failures
   * \param expectedRxBytes the expected number of RX bytes
   */
  void CheckResultsSta1 (uint32_t expectedRxSuccess, uint32_t expectedRxFailure, uint32_t expectedRxBytes);
  /**
   * Check the results for STA 2
   * \param expectedRxSuccess the expected number of RX success
   * \param expectedRxFailure the expected number of RX failures
   * \param expectedRxBytes the expected number of RX bytes
   */
  void CheckResultsSta2 (uint32_t expectedRxSuccess, uint32_t expectedRxFailure, uint32_t expectedRxBytes);
  /**
   * Check the results for STA 3
   * \param expectedRxSuccess the expected number of RX success
   * \param expectedRxFailure the expected number of RX failures
   * \param expectedRxBytes the expected number of RX bytes
   */
  void CheckResultsSta3 (uint32_t expectedRxSuccess, uint32_t expectedRxFailure, uint32_t expectedRxBytes);

  /**
   * Reset the results
   */
  void ResetResults ();

  /**
   * Send MU-PPDU function
   * \param rxStaId1 the ID of the recipient STA for the first PSDU
   * \param rxStaId2 the ID of the recipient STA for the second PSDU
   */
  void SendMuPpdu (uint16_t rxStaId1, uint16_t rxStaId2);

  /**
   * Generate interference function
   * \param interferencePsd the PSD of the interference to be generated
   * \param duration the duration of the interference
   */
  void GenerateInterference (Ptr<SpectrumValue> interferencePsd, Time duration);
  /**
   * Stop interference function
   */
  void StopInterference (void);

  /**
   * Run one function
   */
  void RunOne ();

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

  uint32_t m_countRxSuccessSta1; ///< count RX success for STA 1
  uint32_t m_countRxSuccessSta2; ///< count RX success for STA 2
  uint32_t m_countRxSuccessSta3; ///< count RX success for STA 3
  uint32_t m_countRxFailureSta1; ///< count RX failure for STA 1
  uint32_t m_countRxFailureSta2; ///< count RX failure for STA 2
  uint32_t m_countRxFailureSta3; ///< count RX failure for STA 3
  uint32_t m_countRxBytesSta1;   ///< count RX bytes for STA 1
  uint32_t m_countRxBytesSta2;   ///< count RX bytes for STA 2
  uint32_t m_countRxBytesSta3;   ///< count RX bytes for STA 3

  Ptr<SpectrumWifiPhy> m_phyAp;           ///< PHY of AP
  Ptr<OfdmaSpectrumWifiPhy> m_phySta1;    ///< PHY of STA 1
  Ptr<OfdmaSpectrumWifiPhy> m_phySta2;    ///< PHY of STA 2
  Ptr<OfdmaSpectrumWifiPhy> m_phySta3;    ///< PHY of STA 3
  Ptr<WaveformGenerator> m_phyInterferer; ///< PHY of interferer

  uint16_t m_frequency;        ///< frequency in MHz
  uint16_t m_channelWidth;     ///< channel width in MHz
  Time m_expectedPpduDuration; ///< expected duration to send MU PPDU
};

TestDlOfdmaPhyTransmission::TestDlOfdmaPhyTransmission ()
  : TestCase ("DL-OFDMA PHY test"),
    m_countRxSuccessSta1 (0),
    m_countRxSuccessSta2 (0),
    m_countRxSuccessSta3 (0),
    m_countRxFailureSta1 (0),
    m_countRxFailureSta2 (0),
    m_countRxFailureSta3 (0),
    m_countRxBytesSta1 (0),
    m_countRxBytesSta2 (0),
    m_countRxBytesSta3 (0),
    m_frequency (DEFAULT_FREQUENCY),
    m_channelWidth (DEFAULT_CHANNEL_WIDTH),
    m_expectedPpduDuration (NanoSeconds (306400))
{
}

void
TestDlOfdmaPhyTransmission::ResetResults (void)
{
  m_countRxSuccessSta1 = 0;
  m_countRxSuccessSta2 = 0;
  m_countRxSuccessSta3 = 0;
  m_countRxFailureSta1 = 0;
  m_countRxFailureSta2 = 0;
  m_countRxFailureSta3 = 0;
  m_countRxBytesSta1 = 0;
  m_countRxBytesSta2 = 0;
  m_countRxBytesSta3 = 0;
}

void
TestDlOfdmaPhyTransmission::SendMuPpdu (uint16_t rxStaId1, uint16_t rxStaId2)
{
  NS_LOG_FUNCTION (this << rxStaId1 << rxStaId2);
  WifiConstPsduMap psdus;
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_MU, 800, 1, 1, 0, m_channelWidth, false, false);
  HeRu::RuType ruType = HeRu::RU_106_TONE;
  if (m_channelWidth == 20)
    {
      ruType = HeRu::RU_106_TONE;
    }
  else if (m_channelWidth == 40)
    {
      ruType = HeRu::RU_242_TONE;
    }
  else if (m_channelWidth == 80)
    {
      ruType = HeRu::RU_484_TONE;
    }
  else if (m_channelWidth == 160)
    {
      ruType = HeRu::RU_996_TONE;
    }
  else
    {
      NS_ASSERT_MSG (false, "Unsupported channel width");
    }
  
  HeRu::RuSpec ru1;
  ru1.primary80MHz = true;
  ru1.ruType = ruType;
  ru1.index = 1;
  txVector.SetRu (ru1, rxStaId1);
  txVector.SetMode (WifiPhy::GetHeMcs7 (), rxStaId1);
  txVector.SetNss (1, rxStaId1);

  HeRu::RuSpec ru2;
  ru2.primary80MHz = (m_channelWidth == 160) ? false : true;
  ru2.ruType = ruType;
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

void
TestDlOfdmaPhyTransmission::GenerateInterference (Ptr<SpectrumValue> interferencePsd, Time duration)
{
  m_phyInterferer->SetTxPowerSpectralDensity (interferencePsd);
  m_phyInterferer->SetPeriod (duration);
  m_phyInterferer->Start ();
  Simulator::Schedule (duration, &TestDlOfdmaPhyTransmission::StopInterference, this);
}

void
TestDlOfdmaPhyTransmission::StopInterference (void)
{
  m_phyInterferer->Stop();
}

TestDlOfdmaPhyTransmission::~TestDlOfdmaPhyTransmission ()
{
  m_phyAp = 0;
  m_phySta1 = 0;
  m_phySta2 = 0;
  m_phySta3 = 0;
  m_phyInterferer = 0;
}

void
TestDlOfdmaPhyTransmission::RxSuccessSta1 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> /*statusPerMpdu*/)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector);
  m_countRxSuccessSta1++;
  m_countRxBytesSta1 += (psdu->GetSize () - 30);
}

void
TestDlOfdmaPhyTransmission::RxSuccessSta2 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> /*statusPerMpdu*/)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector);
  m_countRxSuccessSta2++;
  m_countRxBytesSta2 += (psdu->GetSize () - 30);
}

void
TestDlOfdmaPhyTransmission::RxSuccessSta3 (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> /*statusPerMpdu*/)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector);
  m_countRxSuccessSta3++;
  m_countRxBytesSta3 += (psdu->GetSize () - 30);
}

void
TestDlOfdmaPhyTransmission::RxFailureSta1 (Ptr<WifiPsdu> psdu)
{
  NS_LOG_FUNCTION (this << *psdu);
  m_countRxFailureSta1++;
}

void
TestDlOfdmaPhyTransmission::RxFailureSta2 (Ptr<WifiPsdu> psdu)
{
  NS_LOG_FUNCTION (this << *psdu);
  m_countRxFailureSta2++;
}

void
TestDlOfdmaPhyTransmission::RxFailureSta3 (Ptr<WifiPsdu> psdu)
{
  NS_LOG_FUNCTION (this << *psdu);
  m_countRxFailureSta3++;
}

void
TestDlOfdmaPhyTransmission::CheckResultsSta1 (uint32_t expectedRxSuccess, uint32_t expectedRxFailure, uint32_t expectedRxBytes)
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxSuccessSta1, expectedRxSuccess, "The number of successfully received packets by STA 1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxFailureSta1, expectedRxFailure, "The number of unsuccessfully received packets by STA 1 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxBytesSta1, expectedRxBytes, "The number of bytes received by STA 1 is not correct!");
}

void
TestDlOfdmaPhyTransmission::CheckResultsSta2 (uint32_t expectedRxSuccess, uint32_t expectedRxFailure, uint32_t expectedRxBytes)
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxSuccessSta2, expectedRxSuccess, "The number of successfully received packets by STA 2 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxFailureSta2, expectedRxFailure, "The number of unsuccessfully received packets by STA 2 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxBytesSta2, expectedRxBytes, "The number of bytes received by STA 2 is not correct!");
}

void
TestDlOfdmaPhyTransmission::CheckResultsSta3 (uint32_t expectedRxSuccess, uint32_t expectedRxFailure, uint32_t expectedRxBytes)
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxSuccessSta3, expectedRxSuccess, "The number of successfully received packets by STA 3 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxFailureSta3, expectedRxFailure, "The number of unsuccessfully received packets by STA 3 is not correct!");
  NS_TEST_ASSERT_MSG_EQ (m_countRxBytesSta3, expectedRxBytes, "The number of bytes received by STA 3 is not correct!");
}

void
TestDlOfdmaPhyTransmission::CheckPhyState (Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
{
  //This is needed to make sure PHY state will be checked as the last event if a state change occured at the exact same time as the check
  Simulator::ScheduleNow (&TestDlOfdmaPhyTransmission::DoCheckPhyState, this, phy, expectedState);
}

void
TestDlOfdmaPhyTransmission::DoCheckPhyState (Ptr<OfdmaSpectrumWifiPhy> phy, WifiPhyState expectedState)
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
TestDlOfdmaPhyTransmission::DoSetup (void)
{
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel> ();
  lossModel->SetFrequency (m_frequency * 1e6);
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
  m_phySta1->SetDevice (sta1Dev);
  m_phySta1->SetChannel (spectrumChannel);
  m_phySta1->SetReceiveOkCallback (MakeCallback (&TestDlOfdmaPhyTransmission::RxSuccessSta1, this));
  m_phySta1->SetReceiveErrorCallback (MakeCallback (&TestDlOfdmaPhyTransmission::RxFailureSta1, this));
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
  m_phySta2->SetDevice (sta2Dev);
  m_phySta2->SetChannel (spectrumChannel);
  m_phySta2->SetReceiveOkCallback (MakeCallback (&TestDlOfdmaPhyTransmission::RxSuccessSta2, this));
  m_phySta2->SetReceiveErrorCallback (MakeCallback (&TestDlOfdmaPhyTransmission::RxFailureSta2, this));
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
  m_phySta3->SetDevice (sta3Dev);
  m_phySta3->SetChannel (spectrumChannel);
  m_phySta3->SetReceiveOkCallback (MakeCallback (&TestDlOfdmaPhyTransmission::RxSuccessSta3, this));
  m_phySta3->SetReceiveErrorCallback (MakeCallback (&TestDlOfdmaPhyTransmission::RxFailureSta3, this));
  Ptr<ConstantPositionMobilityModel> sta3Mobility = CreateObject<ConstantPositionMobilityModel> ();
  m_phySta3->SetMobility (sta3Mobility);
  sta3Dev->SetPhy (m_phySta3);
  sta3Node->AggregateObject (sta3Mobility);
  sta3Node->AddDevice (sta3Dev);

  Ptr<Node> interfererNode = CreateObject<Node> ();
  Ptr<NonCommunicatingNetDevice> interfererDev = CreateObject<NonCommunicatingNetDevice> ();
  m_phyInterferer = CreateObject<WaveformGenerator> ();
  m_phyInterferer->SetDevice (interfererDev);
  m_phyInterferer->SetChannel (spectrumChannel);
  m_phyInterferer->SetDutyCycle (1);
  interfererNode->AddDevice (interfererDev);
}

void
TestDlOfdmaPhyTransmission::RunOne (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 0;
  m_phyAp->AssignStreams (streamNumber);
  m_phySta1->AssignStreams (streamNumber);
  m_phySta2->AssignStreams (streamNumber);
  m_phySta3->AssignStreams (streamNumber);

  m_phyAp->SetFrequency (m_frequency);
  m_phyAp->SetChannelWidth (m_channelWidth);

  m_phySta1->SetFrequency (m_frequency);
  m_phySta1->SetChannelWidth (m_channelWidth);

  m_phySta2->SetFrequency (m_frequency);
  m_phySta2->SetChannelWidth (m_channelWidth);

  m_phySta3->SetFrequency (m_frequency);
  m_phySta3->SetChannelWidth (m_channelWidth);

  Simulator::Schedule (Seconds (0.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

  //Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
  //Each STA should receive its PSDU.
  Simulator::Schedule (Seconds (1.0), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

  //Since it takes m_expectedPpduDuration to transmit the PPDU,
  //all 3 PHYs should be back to IDLE at the same time,
  //even the PHY that has no PSDU addressed to it.
  Simulator::Schedule (Seconds (1.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (1.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::IDLE);

  //One PSDU of 1000 bytes should have been successfully received by STA 1
  Simulator::Schedule (Seconds (1.1), &TestDlOfdmaPhyTransmission::CheckResultsSta1, this, 1, 0, 1000);
  //One PSDU of 1500 bytes should have been successfully received by STA 2
  Simulator::Schedule (Seconds (1.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 1, 0, 1500);
  //No PSDU should have been received by STA 3
  Simulator::Schedule (Seconds (1.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

  Simulator::Schedule (Seconds (1.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

  //Send MU PPDU with two PSDUs addressed to STA 1 and STA 3:
  //STA 1 should receive its PSDU, whereas STA 2 should not receive any PSDU
  //but should keep its PHY busy during all PPDU duration.
  Simulator::Schedule (Seconds (2.0), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 3);

  //Since it takes m_expectedPpduDuration to transmit the PPDU,
  //all 3 PHYs should be back to IDLE at the same time,
  //even the PHY that has no PSDU addressed to it.
  Simulator::Schedule (Seconds (2.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (2.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (2.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::RX);
  Simulator::Schedule (Seconds (2.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::IDLE);

  //One PSDU of 1000 bytes should have been successfully received by STA 1
  Simulator::Schedule (Seconds (2.1), &TestDlOfdmaPhyTransmission::CheckResultsSta1, this, 1, 0, 1000);
  //No PSDU should have been received by STA 2
  Simulator::Schedule (Seconds (2.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 0, 0, 0);
  //One PSDU of 1500 bytes should have been successfully received by STA 3
  Simulator::Schedule (Seconds (2.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 1, 0, 1500);

  Simulator::Schedule (Seconds (2.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

  //Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
  Simulator::Schedule (Seconds (3.0), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

  //A strong non-wifi interference is generated on RU 1 during PSDU reception
  BandInfo bandInfo;
  bandInfo.fc = (m_frequency - (m_channelWidth / 4)) * 1e6;
  bandInfo.fl = bandInfo.fc - ((m_channelWidth / 4) * 1e6);
  bandInfo.fh = bandInfo.fc + ((m_channelWidth / 4) * 1e6);
  Bands bands;
  bands.push_back (bandInfo);

  Ptr<SpectrumModel> SpectrumInterferenceRu1 = Create<SpectrumModel> (bands);
  Ptr<SpectrumValue> interferencePsdRu1 = Create<SpectrumValue> (SpectrumInterferenceRu1);
  double interferencePower = 0.1; //watts
  *interferencePsdRu1 = interferencePower / ((m_channelWidth / 2) * 20e6);

  Simulator::Schedule (Seconds (3.0) + MicroSeconds (50), &TestDlOfdmaPhyTransmission::GenerateInterference, this, interferencePsdRu1, MilliSeconds (100));

  //Since it takes m_expectedPpduDuration to transmit the PPDU,
  //both PHYs should be back to CCA_BUSY (due to the interference) at the same time,
  //even the PHY that has no PSDU addressed to it.
  Simulator::Schedule (Seconds (3.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (3.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::RX);
  Simulator::Schedule (Seconds (3.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (3.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (3.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (3.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::CCA_BUSY);

  //One PSDU of 1000 bytes should have been unsuccessfully received by STA 1 (since interference occupies RU 1)
  Simulator::Schedule (Seconds (3.1), &TestDlOfdmaPhyTransmission::CheckResultsSta1, this, 0, 1, 0);
  //One PSDU of 1500 bytes should have been successfully received by STA 2
  Simulator::Schedule (Seconds (3.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 1, 0, 1500);
  //No PSDU should have been received by STA3
  Simulator::Schedule (Seconds (3.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

  Simulator::Schedule (Seconds (3.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

  //Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
  Simulator::Schedule (Seconds (4.0), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

  //A strong non-wifi interference is generated on RU 2 during PSDU reception
  bandInfo.fc = (m_frequency + (m_channelWidth / 4)) * 1e6;
  bandInfo.fl = bandInfo.fc - ((m_channelWidth / 4) * 1e6);
  bandInfo.fh = bandInfo.fc + ((m_channelWidth / 4) * 1e6);
  bands.clear ();
  bands.push_back (bandInfo);

  Ptr<SpectrumModel> SpectrumInterferenceRu2 = Create<SpectrumModel> (bands);
  Ptr<SpectrumValue> interferencePsdRu2 = Create<SpectrumValue> (SpectrumInterferenceRu2);
  *interferencePsdRu2 = interferencePower / ((m_channelWidth / 2) * 20e6);

  Simulator::Schedule (Seconds (4.0) + MicroSeconds (50), &TestDlOfdmaPhyTransmission::GenerateInterference, this, interferencePsdRu2, MilliSeconds (100));

  //Since it takes m_expectedPpduDuration to transmit the PPDU,
  //both PHYs should be back to IDLE (or CCA_BUSY if interference on the primary 20 MHz) at the same time,
  //even the PHY that has no PSDU addressed to it.
  Simulator::Schedule (Seconds (4.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (4.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::RX);
  Simulator::Schedule (Seconds (4.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, (m_channelWidth >= 40) ? WifiPhyState::IDLE : WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, (m_channelWidth >= 40) ? WifiPhyState::IDLE : WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, (m_channelWidth >= 40) ? WifiPhyState::IDLE : WifiPhyState::CCA_BUSY);

  //One PSDU of 1000 bytes should have been successfully received by STA 1
  Simulator::Schedule (Seconds (4.1), &TestDlOfdmaPhyTransmission::CheckResultsSta1, this, 1, 0, 1000);
  //One PSDU of 1500 bytes should have been unsuccessfully received by STA 2 (since interference occupies RU 2)
  Simulator::Schedule (Seconds (4.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 0, 1, 0);
  //No PSDU should have been received by STA3
  Simulator::Schedule (Seconds (4.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

  Simulator::Schedule (Seconds (4.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

  //Send MU PPDU with two PSDUs addressed to STA 1 and STA 2:
  Simulator::Schedule (Seconds (5.0), &TestDlOfdmaPhyTransmission::SendMuPpdu, this, 1, 2);

  //A strong non-wifi interference is generated on the full band during PSDU reception
  bandInfo.fc = m_frequency * 1e6;
  bandInfo.fl = bandInfo.fc - ((m_channelWidth / 2) * 1e6);
  bandInfo.fh = bandInfo.fc + ((m_channelWidth / 2) * 1e6);
  bands.clear ();
  bands.push_back (bandInfo);

  Ptr<SpectrumModel> SpectrumInterferenceAll = Create<SpectrumModel> (bands);
  Ptr<SpectrumValue> interferencePsdAll = Create<SpectrumValue> (SpectrumInterferenceAll);
  *interferencePsdAll = interferencePower / (m_channelWidth * 20e6);

  Simulator::Schedule (Seconds (5.0) + MicroSeconds (50), &TestDlOfdmaPhyTransmission::GenerateInterference, this, interferencePsdAll, MilliSeconds (100));

  //Since it takes m_expectedPpduDuration to transmit the PPDU,
  //both PHYs should be back to CCA_BUSY (due to the interference) at the same time,
  //even the PHY that has no PSDU addressed to it.
  Simulator::Schedule (Seconds (5.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::RX);
  Simulator::Schedule (Seconds (5.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::RX);
  Simulator::Schedule (Seconds (5.0) + m_expectedPpduDuration - NanoSeconds (1), &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (5.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta1, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (5.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta2, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (5.0) + m_expectedPpduDuration, &TestDlOfdmaPhyTransmission::CheckPhyState, this, m_phySta3, WifiPhyState::CCA_BUSY);

  //One PSDU of 1000 bytes should have been unsuccessfully received by STA 1 (since interference occupies RU 1)
  Simulator::Schedule (Seconds (5.1), &TestDlOfdmaPhyTransmission::CheckResultsSta1, this, 0, 1, 0);
  //One PSDU of 1500 bytes should have been unsuccessfully received by STA 2 (since interference occupies RU 2)
  Simulator::Schedule (Seconds (5.1), &TestDlOfdmaPhyTransmission::CheckResultsSta2, this, 0, 1, 0);
  //No PSDU should have been received by STA3
  Simulator::Schedule (Seconds (5.1), &TestDlOfdmaPhyTransmission::CheckResultsSta3, this, 0, 0, 0);

  Simulator::Schedule (Seconds (5.5), &TestDlOfdmaPhyTransmission::ResetResults, this);

  Simulator::Run ();
}

void
TestDlOfdmaPhyTransmission::DoRun (void)
{
  m_frequency = 5180;
  m_channelWidth = 20;
  m_expectedPpduDuration = NanoSeconds (306400);
  RunOne ();

  m_frequency = 5190;
  m_channelWidth = 40;
  m_expectedPpduDuration = NanoSeconds (156800);
  RunOne ();

  m_frequency = 5210;
  m_channelWidth = 80;
  m_expectedPpduDuration = NanoSeconds (102400);
  RunOne ();

  m_frequency = 5250;
  m_channelWidth = 160;
  m_expectedPpduDuration = NanoSeconds (75200);
  RunOne ();

  Simulator::Destroy ();
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
  AddTestCase (new TestDlOfdmaPhyTransmission, TestCase::QUICK);
}

static WifiPhyOfdmaTestSuite wifiPhyOfdmaTestSuite; ///< the test suite
