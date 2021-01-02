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
#include "ns3/ap-wifi-mac.h"
#include "ns3/threshold-preamble-detection-model.h"
#include "ns3/waveform-generator.h"
#include "ns3/non-communicating-net-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiPhyOfdmaTest");

static const uint8_t DEFAULT_CHANNEL_NUMBER = 36;
static const uint32_t DEFAULT_FREQUENCY = 5180; // MHz
static const uint16_t DEFAULT_CHANNEL_WIDTH = 20; // MHz
static const uint16_t DEFAULT_GUARD_WIDTH = DEFAULT_CHANNEL_WIDTH; // MHz (expanded to channel width to model spectrum mask)

class OfdmaSpectrumWifiPhy : public SpectrumWifiPhy
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  /**
   * Constructor
   *
   * \param staId the ID of the STA to which this PHY belongs to
   */
  OfdmaSpectrumWifiPhy (uint16_t staId);
  virtual ~OfdmaSpectrumWifiPhy ();

  using WifiPhy::Reset;

  /**
   * TracedCallback signature for UID of transmitted PPDU.
   *
   * \param uid the UID of the transmitted PPDU
   */
  typedef void (*TxPpduUidCallback)(uint64_t uid);

  /**
   * \param ppdu the PPDU to send
   */
  void StartTx (Ptr<WifiPpdu> ppdu) override;

  /**
   * Set the global PPDU UID counter.
   *
   * \param uid the value to which the global PPDU UID counter should be set
   */
  void SetPpduUid (uint64_t uid);

  /**
   * \return the current preamble events map
   */
  std::map <std::pair<uint64_t, WifiPreamble>, Ptr<Event> > & GetCurrentPreambleEvents (void);


private:
  // Inherited
  uint16_t GetStaId (const Ptr<const WifiPpdu> ppdu) const override;

  uint16_t m_staId; ///< ID of the STA to which this PHY belongs to
  TracedCallback<uint64_t> m_phyTxPpduUidTrace; //!< Callback providing UID of the PPDU that is about to be transmitted
};

TypeId
OfdmaSpectrumWifiPhy::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::OfdmaSpectrumWifiPhy")
    .SetParent<SpectrumWifiPhy> ()
    .SetGroupName ("Wifi")
    .AddTraceSource ("TxPpduUid",
                     "UID of the PPDU to be transmitted",
                     MakeTraceSourceAccessor (&OfdmaSpectrumWifiPhy::m_phyTxPpduUidTrace),
                     "ns3::OfdmaSpectrumWifiPhy::TxPpduUidCallback")
  ;
  return tid;
}

OfdmaSpectrumWifiPhy::OfdmaSpectrumWifiPhy (uint16_t staId)
  : SpectrumWifiPhy (),
    m_staId (staId)
{
}

OfdmaSpectrumWifiPhy::~OfdmaSpectrumWifiPhy()
{
}

uint16_t
OfdmaSpectrumWifiPhy::GetStaId (const Ptr<const WifiPpdu> ppdu) const
{
  if (ppdu->IsDlMu ())
    {
      return m_staId;
    }
  return SpectrumWifiPhy::GetStaId (ppdu);
}

void
OfdmaSpectrumWifiPhy::SetPpduUid (uint64_t uid)
{
  m_globalPpduUid = uid;
  m_previouslyRxPpduUid = uid;
}

void
OfdmaSpectrumWifiPhy::StartTx (Ptr<WifiPpdu> ppdu)
{
  m_phyTxPpduUidTrace (ppdu->GetUid ());
  SpectrumWifiPhy::StartTx (ppdu);
}

std::map <std::pair<uint64_t, WifiPreamble>, Ptr<Event> > &
OfdmaSpectrumWifiPhy::GetCurrentPreambleEvents (void)
{
  return m_currentPreambleEvents;
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
 * \brief UL-OFDMA PPDU UID attribution test
 */
class TestUlOfdmaPpduUid : public TestCase
{
public:
  TestUlOfdmaPpduUid ();
  virtual ~TestUlOfdmaPpduUid ();

private:
  virtual void DoSetup (void);
  virtual void DoRun (void);

  /**
   * Transmitted PPDU information function for AP
   * \param uid the UID of the transmitted PPDU
   */
  void TxPpduAp (uint64_t uid);
  /**
   * Transmitted PPDU information function for STA 1
   * \param uid the UID of the transmitted PPDU
   */
  void TxPpduSta1 (uint64_t uid);
  /**
   * Transmitted PPDU information function for STA 2
   * \param uid the UID of the transmitted PPDU
   */
  void TxPpduSta2 (uint64_t uid);
  /**
   * Reset the global PPDU UID counter in WifiPhy
   */
  void ResetPpduUid (void);

  /**
   * Send MU-PPDU toward both STAs.
   */
  void SendMuPpdu (void);
  /**
   * Send TB-PPDU from both STAs.
   */
  void SendTbPpdu (void);
  /**
   * Send SU-PPDU function
   * \param txStaId the ID of the sending STA
   */
  void SendSuPpdu (uint16_t txStaId);

  /**
   * Check the UID of the transmitted PPDU
   * \param staId the STA-ID of the PHY (0 for AP)
   * \param expectedUid the expected UID
   */
  void CheckUid (uint16_t staId, uint64_t expectedUid);

  Ptr<OfdmaSpectrumWifiPhy> m_phyAp;   ///< PHY of AP
  Ptr<OfdmaSpectrumWifiPhy> m_phySta1; ///< PHY of STA 1
  Ptr<OfdmaSpectrumWifiPhy> m_phySta2; ///< PHY of STA 2

  uint64_t m_ppduUidAp; ///< UID of PPDU transmitted by AP
  uint64_t m_ppduUidSta1; ///< UID of PPDU transmitted by STA1
  uint64_t m_ppduUidSta2; ///< UID of PPDU transmitted by STA2
};

TestUlOfdmaPpduUid::TestUlOfdmaPpduUid ()
  : TestCase ("UL-OFDMA PPDU UID attribution test"),
    m_ppduUidAp (UINT64_MAX),
    m_ppduUidSta1 (UINT64_MAX),
    m_ppduUidSta2 (UINT64_MAX)
{
}

TestUlOfdmaPpduUid::~TestUlOfdmaPpduUid ()
{
  m_phyAp = 0;
  m_phySta1 = 0;
  m_phySta2 = 0;
}

void
TestUlOfdmaPpduUid::DoSetup (void)
{
  Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel> ();
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel> ();
  lossModel->SetFrequency (DEFAULT_FREQUENCY);
  spectrumChannel->AddPropagationLossModel (lossModel);
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  spectrumChannel->SetPropagationDelayModel (delayModel);

  Ptr<Node> apNode = CreateObject<Node> ();
  Ptr<WifiNetDevice> apDev = CreateObject<WifiNetDevice> ();
  m_phyAp = CreateObject<OfdmaSpectrumWifiPhy> (0);
  m_phyAp->CreateWifiSpectrumPhyInterface (apDev);
  m_phyAp->ConfigureStandardAndBand (WIFI_PHY_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phyAp->SetErrorRateModel (error);
  m_phyAp->SetFrequency (DEFAULT_FREQUENCY);
  m_phyAp->SetChannelWidth (DEFAULT_CHANNEL_WIDTH);
  m_phyAp->SetDevice (apDev);
  m_phyAp->SetChannel (spectrumChannel);
  m_phyAp->TraceConnectWithoutContext ("TxPpduUid", MakeCallback (&TestUlOfdmaPpduUid::TxPpduAp, this));
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
  m_phySta1->SetFrequency (DEFAULT_FREQUENCY);
  m_phySta1->SetChannelWidth (DEFAULT_CHANNEL_WIDTH);
  m_phySta1->SetDevice (sta1Dev);
  m_phySta1->SetChannel (spectrumChannel);
  m_phySta1->TraceConnectWithoutContext ("TxPpduUid", MakeCallback (&TestUlOfdmaPpduUid::TxPpduSta1, this));
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
  m_phySta2->SetFrequency (DEFAULT_FREQUENCY);
  m_phySta2->SetChannelWidth (DEFAULT_CHANNEL_WIDTH);
  m_phySta2->SetDevice (sta2Dev);
  m_phySta2->SetChannel (spectrumChannel);
  m_phySta2->TraceConnectWithoutContext ("TxPpduUid", MakeCallback (&TestUlOfdmaPpduUid::TxPpduSta2, this));
  Ptr<ConstantPositionMobilityModel> sta2Mobility = CreateObject<ConstantPositionMobilityModel> ();
  m_phySta2->SetMobility (sta2Mobility);
  sta2Dev->SetPhy (m_phySta2);
  sta2Node->AggregateObject (sta2Mobility);
  sta2Node->AddDevice (sta2Dev);
}

void
TestUlOfdmaPpduUid::CheckUid (uint16_t staId, uint64_t expectedUid)
{
  uint64_t uid;
  std::string device;
  switch (staId)
    {
      case 0:
        uid = m_ppduUidAp;
        device = "AP";
        break;
      case 1:
        uid = m_ppduUidSta1;
        device = "STA1";
        break;
      case 2:
        uid = m_ppduUidSta2;
        device = "STA2";
        break;
      default:
        NS_ABORT_MSG ("Unexpected STA-ID");
    }
  NS_TEST_ASSERT_MSG_EQ (uid, expectedUid, "UID " << uid << " does not match expected one " << expectedUid << " for " << device << " at " << Simulator::Now ());
}

void
TestUlOfdmaPpduUid::TxPpduAp (uint64_t uid)
{
  NS_LOG_FUNCTION (this << uid);
  m_ppduUidAp = uid;
}

void
TestUlOfdmaPpduUid::TxPpduSta1 (uint64_t uid)
{
  NS_LOG_FUNCTION (this << uid);
  m_ppduUidSta1 = uid;
}

void
TestUlOfdmaPpduUid::TxPpduSta2 (uint64_t uid)
{
  NS_LOG_FUNCTION (this << uid);
  m_ppduUidSta2 = uid;
}

void
TestUlOfdmaPpduUid::ResetPpduUid (void)
{
  NS_LOG_FUNCTION (this);
  m_phyAp->SetPpduUid (0); //one is enough since it's a global attribute
  return;
}

void
TestUlOfdmaPpduUid::SendMuPpdu (void)
{
  WifiConstPsduMap psdus;
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_MU, 800, 1, 1, 0, DEFAULT_CHANNEL_WIDTH, false, false);

  uint16_t rxStaId1 = 1;
  HeRu::RuSpec ru1;
  ru1.primary80MHz = false;
  ru1.ruType = HeRu::RU_106_TONE;
  ru1.index = 1;
  txVector.SetRu (ru1, rxStaId1);
  txVector.SetMode (WifiPhy::GetHeMcs7 (), rxStaId1);
  txVector.SetNss (1, rxStaId1);

  uint16_t rxStaId2 = 2;
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

void
TestUlOfdmaPpduUid::SendTbPpdu (void)
{
  WifiConstPsduMap psdus1;
  WifiConstPsduMap psdus2;
  WifiTxVector txVector1 = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_TB, 800, 1, 1, 0, DEFAULT_CHANNEL_WIDTH, false, false);
  WifiTxVector txVector2 = txVector1;

  uint16_t rxStaId1 = 1;
  HeRu::RuSpec ru1;
  ru1.primary80MHz = false;
  ru1.ruType = HeRu::RU_106_TONE;
  ru1.index = 1;
  txVector1.SetRu (ru1, rxStaId1);
  txVector1.SetMode (WifiPhy::GetHeMcs7 (), rxStaId1);
  txVector1.SetNss (1, rxStaId1);

  Ptr<Packet> pkt1 = Create<Packet> (1000);
  WifiMacHeader hdr1;
  hdr1.SetType (WIFI_MAC_QOSDATA);
  hdr1.SetQosTid (0);
  hdr1.SetAddr1 (Mac48Address ("00:00:00:00:00:00"));
  hdr1.SetSequenceNumber (1);
  Ptr<WifiPsdu> psdu1 = Create<WifiPsdu> (pkt1, hdr1);
  psdus1.insert (std::make_pair (rxStaId1, psdu1));

  uint16_t rxStaId2 = 2;
  HeRu::RuSpec ru2;
  ru2.primary80MHz = false;
  ru2.ruType = HeRu::RU_106_TONE;
  ru2.index = 2;
  txVector2.SetRu (ru2, rxStaId2);
  txVector2.SetMode (WifiPhy::GetHeMcs9 (), rxStaId2);
  txVector2.SetNss (1, rxStaId2);

  Ptr<Packet> pkt2 = Create<Packet> (1500);
  WifiMacHeader hdr2;
  hdr2.SetType (WIFI_MAC_QOSDATA);
  hdr2.SetQosTid (0);
  hdr2.SetAddr1 (Mac48Address ("00:00:00:00:00:00"));
  hdr2.SetSequenceNumber (2);
  Ptr<WifiPsdu> psdu2 = Create<WifiPsdu> (pkt2, hdr2);
  psdus2.insert (std::make_pair (rxStaId2, psdu2));

  Time txDuration1 = m_phySta1->CalculateTxDuration (psdu1->GetSize (), txVector1,
                                                     m_phySta1->GetPhyBand (), rxStaId1);
  Time txDuration2 = m_phySta2->CalculateTxDuration (psdu2->GetSize (), txVector2,
                                                     m_phySta1->GetPhyBand (), rxStaId2);
  Time txDuration = std::max (txDuration1, txDuration2);

  txVector1.SetLength (m_phySta1->ConvertHeTbPpduDurationToLSigLength (txDuration, m_phySta1->GetPhyBand ()));
  txVector2.SetLength (m_phySta2->ConvertHeTbPpduDurationToLSigLength (txDuration, m_phySta2->GetPhyBand ()));

  m_phySta1->Send (psdus1, txVector1);
  m_phySta2->Send (psdus2, txVector2);
}

void
TestUlOfdmaPpduUid::SendSuPpdu (uint16_t txStaId)
{
  WifiConstPsduMap psdus;
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, DEFAULT_CHANNEL_WIDTH, false, false);

  Ptr<Packet> pkt = Create<Packet> (1000);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  hdr.SetAddr1 (Mac48Address::GetBroadcast ());
  hdr.SetSequenceNumber (1);
  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (pkt, hdr);
  psdus.insert (std::make_pair (SU_STA_ID, psdu));

  switch (txStaId)
    {
      case 0:
        m_phyAp->Send (psdus, txVector);
        break;
      case 1:
        m_phySta1->Send (psdus, txVector);
        break;
      case 2:
        m_phySta2->Send (psdus, txVector);
        break;
      default:
        NS_ABORT_MSG ("Unexpected STA-ID");
    }
}

void
TestUlOfdmaPpduUid::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 0;
  m_phyAp->AssignStreams (streamNumber);
  m_phySta1->AssignStreams (streamNumber);
  m_phySta2->AssignStreams (streamNumber);

  //Reset PPDU UID so as not to be dependent on previously executed test cases,
  //since global attribute will be changed).
  ResetPpduUid ();

  //Send HE MU PPDU with two PSDUs addressed to STA 1 and STA 2.
  //PPDU UID should be equal to 0 (the first counter value).
  Simulator::Schedule (Seconds (1.0), &TestUlOfdmaPpduUid::SendMuPpdu, this);
  Simulator::Schedule (Seconds (1.0), &TestUlOfdmaPpduUid::CheckUid, this, 0, 0);

  //Send HE SU PPDU from AP.
  //PPDU UID should be incremented since this is a new PPDU.
  Simulator::Schedule (Seconds (1.1), &TestUlOfdmaPpduUid::SendSuPpdu, this, 0);
  Simulator::Schedule (Seconds (1.1), &TestUlOfdmaPpduUid::CheckUid, this, 0, 1);

  //Send HE TB PPDU from STAs to AP.
  //PPDU UID should NOT be incremented since HE TB PPDUs reuse the UID of the immediately
  //preceding correctly received PPDU (which normally contains the trigger frame).
  Simulator::Schedule (Seconds (1.15), &TestUlOfdmaPpduUid::SendTbPpdu, this);
  Simulator::Schedule (Seconds (1.15), &TestUlOfdmaPpduUid::CheckUid, this, 1, 1);
  Simulator::Schedule (Seconds (1.15), &TestUlOfdmaPpduUid::CheckUid, this, 2, 1);

  //Send HE SU PPDU from STA1.
  //PPDU UID should be incremented since this is a new PPDU.
  Simulator::Schedule (Seconds (1.2), &TestUlOfdmaPpduUid::SendSuPpdu, this, 1);
  Simulator::Schedule (Seconds (1.2), &TestUlOfdmaPpduUid::CheckUid, this, 1, 2);

  Simulator::Run ();
  Simulator::Destroy ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief UL-OFDMA multiple RX events test
 */
class TestMultipleHeTbPreambles : public TestCase
{
public:
  TestMultipleHeTbPreambles ();
  virtual ~TestMultipleHeTbPreambles ();

private:
  virtual void DoSetup (void);
  virtual void DoRun (void);

  /**
   * Receive HE TB PPDU function.
   *
   * \param uid the UID used to identify a set of HE TB PPDUs belonging to the same UL-MU transmission
   * \param staId the STA ID
   * \param txPowerWatts the TX power in watts
   * \param payloadSize the size of the payload in bytes
   */
  void RxHeTbPpdu (uint64_t uid, uint16_t staId, double txPowerWatts, size_t payloadSize);

  /**
   * RX dropped function
   * \param p the packet
   * \param reason the reason
   */
  void RxDropped (Ptr<const Packet> p, WifiPhyRxfailureReason reason);

  /**
   * Reset function
   */
  void Reset (void);

  /**
   * Check the received HE TB preambles
   * \param nEvents the number of events created by the PHY
   * \param uids the vector of expected UIDs
   */
  void CheckHeTbPreambles (size_t nEvents, std::vector <uint64_t> uids);

  /**
   * Check the number of bytes dropped
   * \param expectedBytesDropped the expected number of bytes dropped
   */
  void CheckBytesDropped (size_t expectedBytesDropped);

  Ptr<OfdmaSpectrumWifiPhy> m_phy; ///< Phy

  uint64_t m_totalBytesDropped; ///< total number of dropped bytes
};

TestMultipleHeTbPreambles::TestMultipleHeTbPreambles ()
  : TestCase ("UL-OFDMA multiple RX events test"),
    m_totalBytesDropped (0)
{
}

TestMultipleHeTbPreambles::~TestMultipleHeTbPreambles ()
{
  m_phy = 0;
}

void
TestMultipleHeTbPreambles::Reset (void)
{
  NS_LOG_FUNCTION (this);
  m_totalBytesDropped = 0;
  //We have to reset PHY here since we do not trigger OFDMA payload RX event in this test
  m_phy->Reset ();
}

void
TestMultipleHeTbPreambles::RxDropped (Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
  NS_LOG_FUNCTION (this << p << reason);
  m_totalBytesDropped += (p->GetSize () - 30);
}

void
TestMultipleHeTbPreambles::CheckHeTbPreambles (size_t nEvents, std::vector <uint64_t> uids)
{
  auto events = m_phy->GetCurrentPreambleEvents ();
  NS_TEST_ASSERT_MSG_EQ (events.size (), nEvents, "The number of UL MU events is not correct!");
  for (auto const& uid : uids)
    {
      auto pair = std::make_pair (uid, WIFI_PREAMBLE_HE_TB);
      auto it = events.find (pair);
      bool found = (it != events.end ());
      NS_TEST_ASSERT_MSG_EQ (found, true, "HE TB PPDU with UID " << uid << " has not been received!");
    }
}

void
TestMultipleHeTbPreambles::CheckBytesDropped (size_t expectedBytesDropped)
{
  NS_TEST_ASSERT_MSG_EQ (m_totalBytesDropped, expectedBytesDropped, "The number of dropped bytes is not correct!");
}

void
TestMultipleHeTbPreambles::RxHeTbPpdu (uint64_t uid, uint16_t staId, double txPowerWatts, size_t payloadSize)
{
  WifiConstPsduMap psdus;
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_TB, 800, 1, 1, 0, DEFAULT_CHANNEL_WIDTH, false, false);

  HeRu::RuSpec ru;
  ru.primary80MHz = false;
  ru.ruType = HeRu::RU_106_TONE;
  ru.index = staId;
  txVector.SetRu (ru, staId);
  txVector.SetMode (WifiPhy::GetHeMcs7 (), staId);
  txVector.SetNss (1, staId);

  Ptr<Packet> pkt = Create<Packet> (payloadSize);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  hdr.SetAddr1 (Mac48Address ("00:00:00:00:00:00"));
  hdr.SetSequenceNumber (1);
  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (pkt, hdr);
  psdus.insert (std::make_pair (staId, psdu));

  Time ppduDuration = m_phy->CalculateTxDuration (psdu->GetSize (), txVector, m_phy->GetPhyBand (), staId);
  Ptr<WifiPpdu> ppdu = Create<WifiPpdu> (psdus, txVector, ppduDuration, WIFI_PHY_BAND_5GHZ, uid);

  Ptr<SpectrumValue> rxPsd = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (DEFAULT_FREQUENCY, DEFAULT_CHANNEL_WIDTH, txPowerWatts, DEFAULT_GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> rxParams = Create<WifiSpectrumSignalParameters> ();
  rxParams->psd = rxPsd;
  rxParams->txPhy = 0;
  rxParams->duration = ppduDuration;
  rxParams->ppdu = ppdu;

  m_phy->StartRx (rxParams);
}

void
TestMultipleHeTbPreambles::DoSetup (void)
{
  Ptr<WifiNetDevice> dev = CreateObject<WifiNetDevice> ();
  m_phy = CreateObject<OfdmaSpectrumWifiPhy> (0);
  m_phy->ConfigureStandardAndBand (WIFI_PHY_STANDARD_80211ax, WIFI_PHY_BAND_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  Ptr<ApWifiMac> mac = CreateObject<ApWifiMac> ();
  mac->SetAttribute ("BeaconGeneration", BooleanValue (false));
  dev->SetMac (mac);
  m_phy->SetErrorRateModel (error);
  m_phy->SetChannelNumber (DEFAULT_CHANNEL_NUMBER);
  m_phy->SetFrequency (DEFAULT_FREQUENCY);
  m_phy->SetChannelWidth (DEFAULT_CHANNEL_WIDTH);
  m_phy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&TestMultipleHeTbPreambles::RxDropped, this));
  m_phy->SetDevice (dev);
  Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel> ();
  preambleDetectionModel->SetAttribute ("Threshold", DoubleValue (4));
  preambleDetectionModel->SetAttribute ("MinimumRssi", DoubleValue (-82));
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);
}

void
TestMultipleHeTbPreambles::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 0;
  m_phy->AssignStreams (streamNumber);

  double txPowerWatts = 0.01;

  {
    //Verify a single UL MU transmission with two stations belonging to the same BSS
    std::vector<uint64_t> uids {0};
    Simulator::Schedule (Seconds (1), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 1, txPowerWatts, 1001);
    Simulator::Schedule (Seconds (1) + NanoSeconds (100), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 2, txPowerWatts, 1002);
    //Check that we received a single UL MU transmission with the corresponding UID
    Simulator::Schedule (Seconds (1.0) + MicroSeconds (1), &TestMultipleHeTbPreambles::CheckHeTbPreambles, this, 1, uids);
    Simulator::Schedule (Seconds (1.5), &TestMultipleHeTbPreambles::Reset, this);
  }

  {
    //Verify the correct reception of 2 UL MU transmissions with two stations per BSS, where the second transmission
    //arrives during the preamble detection window and with half the power of the first transmission.
    std::vector<uint64_t> uids {1, 2};
    Simulator::Schedule (Seconds (2), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 1, txPowerWatts, 1001);
    Simulator::Schedule (Seconds (2) + NanoSeconds (100), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 2, txPowerWatts, 1002);
    Simulator::Schedule (Seconds (2) + NanoSeconds (200), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 1, txPowerWatts / 2, 1003);
    Simulator::Schedule (Seconds (2) + NanoSeconds (300), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 2, txPowerWatts / 2, 1004);
    //Check that we received the correct reception of 2 UL MU transmissions with the corresponding UIDs
    Simulator::Schedule (Seconds (2.0) + MicroSeconds (1), &TestMultipleHeTbPreambles::CheckHeTbPreambles, this, 2, uids);
    Simulator::Schedule (Seconds (2.5), &TestMultipleHeTbPreambles::Reset, this);
    //TODO: verify PPDUs from second UL MU transmission are dropped
  }

  {
    //Verify the correct reception of 2 UL MU transmissions with two stations per BSS, where the second transmission
    //arrives during the preamble detection window and with twice the power of the first transmission.
    std::vector<uint64_t> uids {3, 4};
    Simulator::Schedule (Seconds (3), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 1, txPowerWatts / 2, 1001);
    Simulator::Schedule (Seconds (3) + NanoSeconds (100), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 2, txPowerWatts / 2, 1002);
    Simulator::Schedule (Seconds (3) + NanoSeconds (200), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 1, txPowerWatts, 1003);
    Simulator::Schedule (Seconds (3) + NanoSeconds (300), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 2, txPowerWatts, 1004);
    //Check that we received the correct reception of 2 UL MU transmissions with the corresponding UIDs
    Simulator::Schedule (Seconds (3.0) + MicroSeconds (1), &TestMultipleHeTbPreambles::CheckHeTbPreambles, this, 2, uids);
    Simulator::Schedule (Seconds (3.5), &TestMultipleHeTbPreambles::Reset, this);
    //TODO: verify PPDUs from first UL MU transmission are dropped
  }

  {
    //Verify the correct reception of 2 UL MU transmissions with two stations per BSS, where the second transmission
    //arrives during PHY header reception and with the same power as the first transmission.
    std::vector<uint64_t> uids {5, 6};
    Simulator::Schedule (Seconds (4), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 1, txPowerWatts, 1001);
    Simulator::Schedule (Seconds (4) + NanoSeconds (100), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 2, txPowerWatts, 1002);
    Simulator::Schedule (Seconds (4) + MicroSeconds (5), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 1, txPowerWatts, 1003);
    Simulator::Schedule (Seconds (4) + MicroSeconds (5) + NanoSeconds (100), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 2, txPowerWatts, 1004);
    //Check that we received the correct reception of the first UL MU transmission with the corresponding UID (second one dropped)
    Simulator::Schedule (Seconds (4.0) + MicroSeconds (10), &TestMultipleHeTbPreambles::CheckHeTbPreambles, this, 1, std::vector<uint64_t> {uids[0]});
    //The packets of the second UL MU transmission should have been dropped
    Simulator::Schedule (Seconds (4.0) + MicroSeconds (10), &TestMultipleHeTbPreambles::CheckBytesDropped, this, 1003 + 1004);
    Simulator::Schedule (Seconds (4.5), &TestMultipleHeTbPreambles::Reset, this);
  }

  {
    //Verify the correct reception of one UL MU transmission out of 2 with two stations per BSS, where the second transmission
    //arrives during payload reception and with the same power as the first transmission.
    std::vector<uint64_t> uids {7, 8};
    Simulator::Schedule (Seconds (5), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 1, txPowerWatts, 1001);
    Simulator::Schedule (Seconds (5) + NanoSeconds (100), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 2, txPowerWatts, 1002);
    Simulator::Schedule (Seconds (5) + MicroSeconds (50), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 1, txPowerWatts, 1003);
    Simulator::Schedule (Seconds (5) + MicroSeconds (50) + NanoSeconds (100), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[1], 2, txPowerWatts, 1004);
    //Check that we received the correct reception of the first UL MU transmission with the corresponding UID (second one dropped)
    Simulator::Schedule (Seconds (5.0) + MicroSeconds (100), &TestMultipleHeTbPreambles::CheckHeTbPreambles, this, 1, std::vector<uint64_t> {uids[0]});
    //The packets of the second UL MU transmission should have been dropped
    Simulator::Schedule (Seconds (5.0) + MicroSeconds (100), &TestMultipleHeTbPreambles::CheckBytesDropped, this, 1003 + 1004);
    Simulator::Schedule (Seconds (5.5), &TestMultipleHeTbPreambles::Reset, this);
  }

  {
    //Verify the correct reception of a single UL MU transmission with two stations belonging to the same BSS,
    //and the second PPDU arrives 500ns after the first PPDU, i.e. it exceeds the delay spread of 400ns
    std::vector<uint64_t> uids {9};
    Simulator::Schedule (Seconds (6), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 1, txPowerWatts, 1001);
    Simulator::Schedule (Seconds (6) + NanoSeconds (500), &TestMultipleHeTbPreambles::RxHeTbPpdu, this, uids[0], 2, txPowerWatts, 1002);
    //Check that we received a single UL MU transmission with the corresponding UID
    Simulator::Schedule (Seconds (6.0) + MicroSeconds (1), &TestMultipleHeTbPreambles::CheckHeTbPreambles, this, 1, uids);
    //The first packet of 1001 bytes should be dropped because preamble is not detected after 4us (because the PPDU that arrived at 500ns is interfering):
    //the second HE TB PPDU is acting as interference since it arrived after the maximum allowed 400ns.
    //Obviously, that second packet of 1002 bytes is dropped as well.
    Simulator::Schedule (Seconds (6.0) + MicroSeconds (5), &TestMultipleHeTbPreambles::CheckBytesDropped, this, 1001 + 1002);
    Simulator::Schedule (Seconds (6.5), &TestMultipleHeTbPreambles::Reset, this);
  }

  Simulator::Run ();
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
  AddTestCase (new TestUlOfdmaPpduUid, TestCase::QUICK);
  AddTestCase (new TestMultipleHeTbPreambles, TestCase::QUICK);
}

static WifiPhyOfdmaTestSuite wifiPhyOfdmaTestSuite; ///< the test suite
