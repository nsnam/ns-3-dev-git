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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/test.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-trailer.h"
#include "ns3/ampdu-tag.h"
#include "ns3/wifi-phy-tag.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-utils.h"
#include "ns3/threshold-preamble-detection-model.h"
#include "ns3/simple-frame-capture-model.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-mac-queue-item.h"
#include "ns3/mpdu-aggregator.h"
#include "ns3/wifi-phy-header.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("WifiPhyReceptionTest");

static const uint8_t CHANNEL_NUMBER = 36;
static const uint32_t FREQUENCY = 5180; // MHz
static const uint16_t CHANNEL_WIDTH = 20; // MHz
static const uint16_t GUARD_WIDTH = CHANNEL_WIDTH; // MHz (expanded to channel width to model spectrum mask)

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Preamble detection test w/o frame capture
 */
class TestThresholdPreambleDetectionWithoutFrameCapture : public TestCase
{
public:
  TestThresholdPreambleDetectionWithoutFrameCapture ();
  virtual ~TestThresholdPreambleDetectionWithoutFrameCapture ();

protected:
  virtual void DoSetup (void);
  Ptr<SpectrumWifiPhy> m_phy; ///< Phy
  /**
   * Send packet function
   * \param rxPowerDbm the transmit power in dBm
   */
  void SendPacket (double rxPowerDbm);
  /**
   * Spectrum wifi receive success function
   * \param p the packet
   * \param snr the SNR
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu);
  /**
   * Spectrum wifi receive failure function
   * \param p the packet
   */
  void RxFailure (Ptr<Packet> p);
  uint32_t m_countRxSuccess; ///< count RX success
  uint32_t m_countRxFailure; ///< count RX failure

private:
  virtual void DoRun (void);

  /**
   * Check the PHY state
   * \param expectedState the expected PHY state
   */
  void CheckPhyState (WifiPhyState expectedState);
  void DoCheckPhyState (WifiPhyState expectedState);
  /**
   * Check the number of received packets
   * \param expectedSuccessCount the number of successfully received packets
   * \param expectedFailureCount the number of unsuccessfully received packets
   */
  void CheckRxPacketCount (uint32_t expectedSuccessCount, uint32_t expectedFailureCount);
};

TestThresholdPreambleDetectionWithoutFrameCapture::TestThresholdPreambleDetectionWithoutFrameCapture ()
  : TestCase ("Threshold preamble detection model test when no frame capture model is applied"),
    m_countRxSuccess (0),
    m_countRxFailure (0)
{
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket (double rxPowerDbm)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);

  Ptr<Packet> pkt = Create<Packet> (1000);
  WifiMacHeader hdr;
  WifiMacTrailer trailer;

  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint32_t size = pkt->GetSize () + hdr.GetSize () + trailer.GetSerializedSize ();
  Time txDuration = m_phy->CalculateTxDuration (size, txVector, m_phy->GetFrequency ());
  hdr.SetDuration (txDuration);

  pkt->AddHeader (hdr);
  pkt->AddTrailer (trailer);

  HeSigHeader heSig;
  heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
  heSig.SetBssColor (txVector.GetBssColor ());
  heSig.SetChannelWidth (txVector.GetChannelWidth ());
  heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2);
  pkt->AddHeader (heSig);

  LSigHeader sig;
  pkt->AddHeader (sig);

  WifiPhyTag tag (txVector.GetPreambleType (), txVector.GetMode ().GetModulationClass (), 1);
  pkt->AddPacketTag (tag);

  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (rxPowerDbm), GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = 0;
  txParams->duration = txDuration;
  txParams->packet = pkt;

  m_phy->StartRx (txParams);
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState (WifiPhyState expectedState)
{
  //This is needed to make sure PHY state will be checked as the last event if a state change occured at the exact same time as the check
  Simulator::ScheduleNow (&TestThresholdPreambleDetectionWithoutFrameCapture::DoCheckPhyState, this, expectedState);
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::DoCheckPhyState (WifiPhyState expectedState)
{
  WifiPhyState currentState;
  PointerValue ptr;
  m_phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_LOG_FUNCTION (this << currentState);
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount (uint32_t expectedSuccessCount, uint32_t expectedFailureCount)
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxSuccess, expectedSuccessCount, "Didn't receive right number of successful packets");
  NS_TEST_ASSERT_MSG_EQ (m_countRxFailure, expectedFailureCount, "Didn't receive right number of unsuccessful packets");
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu)
{
  NS_LOG_FUNCTION (this << p << snr << txVector);
  m_countRxSuccess++;
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::RxFailure (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  m_countRxFailure++;
}

TestThresholdPreambleDetectionWithoutFrameCapture::~TestThresholdPreambleDetectionWithoutFrameCapture ()
{
  m_phy = 0;
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::DoSetup (void)
{
  m_phy = CreateObject<SpectrumWifiPhy> ();
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phy->SetErrorRateModel (error);
  m_phy->SetChannelNumber (CHANNEL_NUMBER);
  m_phy->SetFrequency (FREQUENCY);
  m_phy->SetReceiveOkCallback (MakeCallback (&TestThresholdPreambleDetectionWithoutFrameCapture::RxSuccess, this));
  m_phy->SetReceiveErrorCallback (MakeCallback (&TestThresholdPreambleDetectionWithoutFrameCapture::RxFailure, this));

  Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel> ();
  preambleDetectionModel->SetAttribute ("Threshold", DoubleValue (4));
  preambleDetectionModel->SetAttribute ("MinimumRssi", DoubleValue (-82));
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 0;
  m_phy->AssignStreams (streamNumber);

  //RX power > CCA-ED > CCA-PD
  double rxPowerDbm = -50;

  // CASE 1: send one packet and check PHY state:
  // All reception stages should succeed and PHY state should be RX for the duration of the packet minus the time to detect the preamble,
  // otherwise it should be IDLE.

  Simulator::Schedule (Seconds (1.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // Packet should have been successfully received
  Simulator::Schedule (Seconds (1.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 0);

  // CASE 2: send two packets with same power within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than the threshold of 4 dB),
  // and PHY state should be CCA_BUSY since the total energy is above CCA-ED (-62 dBm).
  // CCA_BUSY state should last for the duration of the two packets minus the time to detect the preamble.

  Simulator::Schedule (Seconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should move from IDLE to CCA_BUSY
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass, the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (2.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 0);

  // CASE 3: send two packets with second one 3 dB weaker within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around 3 dB, which is lower than the threshold of 4 dB),
  // and PHY state should be CCA_BUSY since the total energy is above CCA-ED (-62 dBm).
  // CCA_BUSY state should last for the duration of the two packets minus the time to detect the preamble.

  Simulator::Schedule (Seconds (3.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm - 3);
  // At 4us, STA PHY STATE should move from IDLE to CCA_BUSY
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (3.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 0);

  // CASE 4: send two packets with second one 6 dB weaker within the 4us window and check PHY state:
  // PHY preamble detection should succeed because SNR is high enough (around 6 dB, which is higher than the threshold of 4 dB),
  // but payload reception should fail (SNR too low to decode the modulation).

  Simulator::Schedule (Seconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm - 6);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
  // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY should first be seen as CCA_BUSY for 2us.
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // In this case, the first packet should be marked as a failure
  Simulator::Schedule (Seconds (4.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 1);

  // CASE 5: send two packets with second one 3 dB higher within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around -3 dB, which is lower than the threshold of 4 dB),
  // and PHY state should be CCA_BUSY since the total energy is above CCA-ED (-62 dBm).
  // CCA_BUSY state should last for the duration of the two packets minus the time to detect the preamble.

  Simulator::Schedule (Seconds (5.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm + 3);
  // At 4us, STA PHY STATE should move from IDLE to CCA_BUSY
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (5.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 1);

  // CCA-PD < RX power < CCA-ED
  rxPowerDbm = -70;

  // CASE 6: send one packet and check PHY state:
  // All reception stages should succeed and PHY state should be RX for the duration of the packet minus the time to detect the preamble,
  // otherwise it should be IDLE.

  Simulator::Schedule (Seconds (6.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // Packet should have been successfully received
  Simulator::Schedule (Seconds (6.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 2, 1);

  // CASE 7: send two packets with same power within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than the threshold of 4 dB),
  // and PHY state should stay IDLE since the total energy is below CCA-ED (-62 dBm).

  Simulator::Schedule (Seconds (7.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (7.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (7.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (7.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 2, 1);

  // CASE 8: send two packets with second one 3 dB weaker within the 4us window and check PHY state: PHY preamble detection should fail
  // PHY preamble detection should fail because SNR is too low (around 3 dB, which is lower than the threshold of 4 dB),
  // and PHY state should stay IDLE since the total energy is below CCA-ED (-62 dBm).

  Simulator::Schedule (Seconds (8.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (8.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm - 3);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (8.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (8.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 2, 1);

  // CASE 9: send two packets with second one 6 dB weaker within the 4us window and check PHY state:
  // PHY preamble detection should succeed because SNR is high enough (around 6 dB, which is higher than the threshold of 4 dB),
  // but payload reception should fail (SNR too low to decode the modulation).

  Simulator::Schedule (Seconds (9.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (9.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm - 6);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (9.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (9.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
  Simulator::Schedule (Seconds (9.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (9.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // In this case, the first packet should be marked as a failure
  Simulator::Schedule (Seconds (9.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 2, 2);

  // CASE 10: send two packets with second one 3 dB higher within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around -3 dB, which is lower than the threshold of 4 dB),
  // and PHY state should stay IDLE since the total energy is below CCA-ED (-62 dBm).

  Simulator::Schedule (Seconds (10.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (10.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm + 3);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (10.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (10.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 2, 2);

  // CASE 11: send one packet with a power slightly above the minimum RSSI needed for the preamble detection (-82 dBm) and check PHY state:
  // preamble detection should succeed and PHY state should move to RX.

  rxPowerDbm = -81;

  Simulator::Schedule (Seconds (11.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (11.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (11.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
  Simulator::Schedule (Seconds (11.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (11.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  
  // RX power < CCA-PD < CCA-ED
  rxPowerDbm = -83;

  //CASE 12: send one packet with a power slightly below the minimum RSSI needed for the preamble detection (-82 dBm) and check PHY state:
  //preamble detection should fail and PHY should be kept in IDLE state.

  Simulator::Schedule (Seconds (12.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY state should be IDLE
  Simulator::Schedule (Seconds (12.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);

  Simulator::Run ();
  Simulator::Destroy ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Preamble detection test w/o frame capture
 */
class TestThresholdPreambleDetectionWithFrameCapture : public TestCase
{
public:
  TestThresholdPreambleDetectionWithFrameCapture ();
  virtual ~TestThresholdPreambleDetectionWithFrameCapture ();

protected:
  virtual void DoSetup (void);
  Ptr<SpectrumWifiPhy> m_phy; ///< Phy
  /**
   * Send packet function
   * \param rxPowerDbm the transmit power in dBm
   */
  void SendPacket (double rxPowerDbm);
  /**
   * Spectrum wifi receive success function
   * \param p the packet
   * \param snr the SNR
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu);
  /**
   * Spectrum wifi receive failure function
   * \param p the packet
   */
  void RxFailure (Ptr<Packet> p);
  uint32_t m_countRxSuccess; ///< count RX success
  uint32_t m_countRxFailure; ///< count RX failure

private:
  virtual void DoRun (void);

  /**
   * Check the PHY state
   * \param expectedState the expected PHY state
   */
  void CheckPhyState (WifiPhyState expectedState);
  void DoCheckPhyState (WifiPhyState expectedState);
  /**
   * Check the number of received packets
   * \param expectedSuccessCount the number of successfully received packets
   * \param expectedFailureCount the number of unsuccessfully received packets
   */
  void CheckRxPacketCount (uint32_t expectedSuccessCount, uint32_t expectedFailureCount);
};

TestThresholdPreambleDetectionWithFrameCapture::TestThresholdPreambleDetectionWithFrameCapture ()
: TestCase ("Threshold preamble detection model test when simple frame capture model is applied"),
m_countRxSuccess (0),
m_countRxFailure (0)
{
}

void
TestThresholdPreambleDetectionWithFrameCapture::SendPacket (double rxPowerDbm)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);
  
  Ptr<Packet> pkt = Create<Packet> (1000);
  WifiMacHeader hdr;
  WifiMacTrailer trailer;

  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint32_t size = pkt->GetSize () + hdr.GetSize () + trailer.GetSerializedSize ();
  Time txDuration = m_phy->CalculateTxDuration (size, txVector, m_phy->GetFrequency ());
  hdr.SetDuration (txDuration);

  pkt->AddHeader (hdr);
  pkt->AddTrailer (trailer);

  HeSigHeader heSig;
  heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
  heSig.SetBssColor (txVector.GetBssColor ());
  heSig.SetChannelWidth (txVector.GetChannelWidth ());
  heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2);
  pkt->AddHeader (heSig);

  LSigHeader sig;
  pkt->AddHeader (sig);

  WifiPhyTag tag (txVector.GetPreambleType (), txVector.GetMode ().GetModulationClass (), 1);
  pkt->AddPacketTag (tag);

  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (rxPowerDbm), GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = 0;
  txParams->duration = txDuration;
  txParams->packet = pkt;

  m_phy->StartRx (txParams);
}

void
TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState (WifiPhyState expectedState)
{
  //This is needed to make sure PHY state will be checked as the last event if a state change occured at the exact same time as the check
  Simulator::ScheduleNow (&TestThresholdPreambleDetectionWithFrameCapture::DoCheckPhyState, this, expectedState);
}

void
TestThresholdPreambleDetectionWithFrameCapture::DoCheckPhyState (WifiPhyState expectedState)
{
  WifiPhyState currentState;
  PointerValue ptr;
  m_phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_LOG_FUNCTION (this << currentState);
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}

void
TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount (uint32_t expectedSuccessCount, uint32_t expectedFailureCount)
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxSuccess, expectedSuccessCount, "Didn't receive right number of successful packets");
  NS_TEST_ASSERT_MSG_EQ (m_countRxFailure, expectedFailureCount, "Didn't receive right number of unsuccessful packets");
}

void
TestThresholdPreambleDetectionWithFrameCapture::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu)
{
  NS_LOG_FUNCTION (this << p << txVector);
  m_countRxSuccess++;
}

void
TestThresholdPreambleDetectionWithFrameCapture::RxFailure (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  m_countRxFailure++;
}

TestThresholdPreambleDetectionWithFrameCapture::~TestThresholdPreambleDetectionWithFrameCapture ()
{
  m_phy = 0;
}

void
TestThresholdPreambleDetectionWithFrameCapture::DoSetup (void)
{
  m_phy = CreateObject<SpectrumWifiPhy> ();
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phy->SetErrorRateModel (error);
  m_phy->SetChannelNumber (CHANNEL_NUMBER);
  m_phy->SetFrequency (FREQUENCY);
  m_phy->SetReceiveOkCallback (MakeCallback (&TestThresholdPreambleDetectionWithFrameCapture::RxSuccess, this));
  m_phy->SetReceiveErrorCallback (MakeCallback (&TestThresholdPreambleDetectionWithFrameCapture::RxFailure, this));

  Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel> ();
  preambleDetectionModel->SetAttribute ("Threshold", DoubleValue (4));
  preambleDetectionModel->SetAttribute ("MinimumRssi", DoubleValue (-82));
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);

  Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel> ();
  frameCaptureModel->SetAttribute ("Margin", DoubleValue (5));
  frameCaptureModel->SetAttribute ("CaptureWindow", TimeValue (MicroSeconds (16)));
  m_phy->SetFrameCaptureModel (frameCaptureModel);
}

void
TestThresholdPreambleDetectionWithFrameCapture::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 1;
  m_phy->AssignStreams (streamNumber);

  //RX power > CCA-ED > CCA-PD
  double rxPowerDbm = -50;

  // CASE 1: send one packet and check PHY state:
  // All reception stages should succeed and PHY state should be RX for the duration of the packet minus the time to detect the preamble,
  // otherwise it should be IDLE.

  Simulator::Schedule (Seconds (1.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // Packet should have been successfully received
  Simulator::Schedule (Seconds (1.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 0);

  // CASE 2: send two packets with same power within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than the threshold of 4 dB),
  // and PHY state should be CCA_BUSY since the total energy is above CCA-ED (-62 dBm).
  // CCA_BUSY state should last for the duration of the two packets minus the time to detect the preamble.

  Simulator::Schedule (Seconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should move from IDLE to CCA_BUSY
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (2.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 0);

  // CASE 3: send two packets with second one 3 dB weaker within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around 3 dB, which is lower than the threshold of 4 dB),
  // and PHY state should be CCA_BUSY since the total energy is above CCA-ED (-62 dBm).
  // CCA_BUSY state should last for the duration of the two packets minus the time to detect the preamble.

  Simulator::Schedule (Seconds (3.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm - 3);
  // At 4us, STA PHY STATE should move from IDLE to CCA_BUSY
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (3.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 0);

  // CASE 4: send two packets with second one 6 dB weaker within the 4us window and check PHY state:
  // PHY preamble detection should succeed because SNR is high enough (around 6 dB, which is higher than the threshold of 4 dB),
  // but payload reception should fail (SNR too low to decode the modulation).

  Simulator::Schedule (Seconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm - 6);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
  // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY should first be seen as CCA_BUSY for 2us.
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // In this case, the first packet should be marked as a failure
  Simulator::Schedule (Seconds (4.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 1);

  // CASE 5: send two packets with second one 3 dB higher within the 4us window and check PHY state:
  // PHY preamble detection should switch because a higher packet is received within the 4us window,
  // but preamble detection should fail because SNR is too low (around 3 dB, which is lower than the threshold of 4 dB),
  // PHY state should be CCA_BUSY since the total energy is above CCA-ED (-62 dBm).

  Simulator::Schedule (Seconds (5.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm + 3);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // At 6us, STA PHY STATE should move from IDLE to CCA_BUSY
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (5999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (6000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (5.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 1);

  // CASE 6: send two packets with second one 6 dB higher within the 4us window and check PHY state:
  // PHY preamble detection should switch because a higher packet is received within the 4us window,
  // and preamble detection should succeed because SNR is high enough (around 6 dB, which is higher than the threshold of 4 dB),
  // Payload reception should fail (SNR too low to decode the modulation).

  Simulator::Schedule (Seconds (6.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm + 6);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // At 6us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (5999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (6000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // In this case, the second packet should be marked as a failure
  Simulator::Schedule (Seconds (6.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 2);

  // CCA-PD < RX power < CCA-ED
  rxPowerDbm = -70;

  // CASE 7: send one packet and check PHY state:
  // All reception stages should succeed and PHY state should be RX for the duration of the packet minus the time to detect the preamble,
  // otherwise it should be IDLE.

  Simulator::Schedule (Seconds (7.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // Packet should have been successfully received
  Simulator::Schedule (Seconds (7.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 2, 2);

  // CASE 8: send two packets with same power within the 4us window and check PHY state:
  // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than the threshold of 4 dB),
  // and PHY state should stay IDLE since the total energy is below CCA-ED (-62 dBm).

  Simulator::Schedule (Seconds (8.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (8.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (8.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (8.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 2, 2);

  // CASE 9: send two packets with second one 3 dB weaker within the 4us window and check PHY state: PHY preamble detection should fail
  // PHY preamble detection should fail because SNR is too low (around 3 dB, which is lower than the threshold of 4 dB),
  // and PHY state should stay IDLE since the total energy is below CCA-ED (-62 dBm).

  Simulator::Schedule (Seconds (9.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (9.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm - 3);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (9.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (9.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 2, 2);

  // CASE 10: send two packets with second one 6 dB weaker within the 4us window and check PHY state:
  // PHY preamble detection should succeed because SNR is high enough (around 6 dB, which is higher than the threshold of 4 dB),
  // but payload reception should fail (SNR too low to decode the modulation).

  Simulator::Schedule (Seconds (10.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (10.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm - 6);
  // At 4us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (10.0) + NanoSeconds (3999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (10.0) + NanoSeconds (4000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
  Simulator::Schedule (Seconds (10.0) + NanoSeconds (152799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (10.0) + NanoSeconds (152800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // In this case, the first packet should be marked as a failure
  Simulator::Schedule (Seconds (10.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 2, 3);

  // CASE 11: send two packets with second one 3 dB higher within the 4us window and check PHY state:
  // PHY preamble detection should switch because a higher packet is received within the 4us window,
  // but preamble detection should fail because SNR is too low (around 3 dB, which is lower than the threshold of 4 dB).
  // PHY state should stay IDLE since the total energy is below CCA-ED (-62 dBm).

  Simulator::Schedule (Seconds (11.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (11.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm + 3);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (11.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // At 6us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (11.0) + MicroSeconds (6.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (11.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 2, 3);

  // CASE 12: send two packets with second one 6 dB higher within the 4us window and check PHY state:
  // PHY preamble detection should switch because a higher packet is received within the 4us window,
  // and preamble detection should succeed because SNR is high enough (around 6 dB, which is higher than the threshold of 4 dB),
  // Payload reception should fail (SNR too low to decode the modulation).

  Simulator::Schedule (Seconds (12.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (12.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, rxPowerDbm + 6);
  // At 4us, STA PHY STATE should stay in IDLE
  Simulator::Schedule (Seconds (12.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // At 6us, STA PHY STATE should move from IDLE to RX
  Simulator::Schedule (Seconds (12.0) + NanoSeconds (5999), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (12.0) + NanoSeconds (6000), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2 = 154.8us
  Simulator::Schedule (Seconds (12.0) + NanoSeconds (154799), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (12.0) + NanoSeconds (154800), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // In this case, the second packet should be marked as a failure
  Simulator::Schedule (Seconds (12.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 2, 4);

  Simulator::Run ();
  Simulator::Destroy ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Simple frame capture model test
 */
class TestSimpleFrameCaptureModel : public TestCase
{
public:
  TestSimpleFrameCaptureModel ();
  virtual ~TestSimpleFrameCaptureModel ();

protected:
  virtual void DoSetup (void);

private:
  virtual void DoRun (void);

  /**
   * Reset function
   */
  void Reset (void);
  /**
   * Send packet function
   * \param rxPowerDbm the transmit power in dBm
   * \param packetSize the size of the packet in bytes
   */
  void SendPacket (double rxPowerDbm, uint32_t packetSize);
  /**
   * Spectrum wifi receive success function
   * \param p the packet
   * \param snr the SNR
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu);
  /**
   * RX dropped function
   * \param p the packet
   * \param reason the reason
   */
  void RxDropped (Ptr<const Packet> p, WifiPhyRxfailureReason reason);

  void Expect1000BPacketReceived ();
  void Expect1500BPacketReceived ();
  void Expect1000BPacketDropped ();
  void Expect1500BPacketDropped ();

  Ptr<SpectrumWifiPhy> m_phy; ///< Phy

  bool m_rxSuccess1000B; ///< count received packets with 1000B payload
  bool m_rxSuccess1500B; ///< count received packets with 1500B payload
  bool m_rxDropped1000B; ///< count dropped packets with 1000B payload
  bool m_rxDropped1500B; ///< count dropped packets with 1500B payload
};

TestSimpleFrameCaptureModel::TestSimpleFrameCaptureModel ()
: TestCase ("Simple frame capture model test"),
  m_rxSuccess1000B (false),
  m_rxSuccess1500B (false),
  m_rxDropped1000B (false),
  m_rxDropped1500B (false)
{
}

void
TestSimpleFrameCaptureModel::SendPacket (double rxPowerDbm, uint32_t packetSize)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs0 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);
  
  Ptr<Packet> pkt = Create<Packet> (packetSize);
  WifiMacHeader hdr;
  WifiMacTrailer trailer;
  
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint32_t size = pkt->GetSize () + hdr.GetSize () + trailer.GetSerializedSize ();
  Time txDuration = m_phy->CalculateTxDuration (size, txVector, m_phy->GetFrequency ());
  hdr.SetDuration (txDuration);
  
  pkt->AddHeader (hdr);
  pkt->AddTrailer (trailer);

  HeSigHeader heSig;
  heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
  heSig.SetBssColor (txVector.GetBssColor ());
  heSig.SetChannelWidth (txVector.GetChannelWidth ());
  heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2);
  pkt->AddHeader (heSig);
  
  LSigHeader sig;
  pkt->AddHeader (sig);
  
  WifiPhyTag tag (txVector.GetPreambleType (), txVector.GetMode ().GetModulationClass (), 1);
  pkt->AddPacketTag (tag);

  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (rxPowerDbm), GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = 0;
  txParams->duration = txDuration;
  txParams->packet = pkt;
  
  m_phy->StartRx (txParams);
}

void
TestSimpleFrameCaptureModel::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu)
{
  NS_LOG_FUNCTION (this << p << snr << txVector);
  if (p->GetSize () == 1030)
    {
      m_rxSuccess1000B = true;
    }
  else if (p->GetSize () == 1530)
    {
      m_rxSuccess1500B = true;
    }
}

void
TestSimpleFrameCaptureModel::RxDropped (Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
  NS_LOG_FUNCTION (this << p << reason);
  if (p->GetSize () == 1030)
    {
      m_rxDropped1000B = true;
    }
  else if (p->GetSize () == 1530)
    {
      m_rxDropped1500B = true;
    }
}

void
TestSimpleFrameCaptureModel::Reset ()
{
  m_rxSuccess1000B = false;
  m_rxSuccess1500B = false;
  m_rxDropped1000B = false;
  m_rxDropped1500B = false;
}

void
TestSimpleFrameCaptureModel::Expect1000BPacketReceived ()
{
  NS_TEST_ASSERT_MSG_EQ (m_rxSuccess1000B, true, "Didn't receive 1000B packet");
}

void
TestSimpleFrameCaptureModel::Expect1500BPacketReceived ()
{
  NS_TEST_ASSERT_MSG_EQ (m_rxSuccess1500B, true, "Didn't receive 1500B packet");
}
void
TestSimpleFrameCaptureModel::Expect1000BPacketDropped ()
{
  NS_TEST_ASSERT_MSG_EQ (m_rxDropped1000B, true, "Didn't drop 1000B packet");
}

void
TestSimpleFrameCaptureModel::Expect1500BPacketDropped ()
{
  NS_TEST_ASSERT_MSG_EQ (m_rxDropped1500B, true, "Didn't drop 1500B packet");
}

TestSimpleFrameCaptureModel::~TestSimpleFrameCaptureModel ()
{
  m_phy = 0;
}

void
TestSimpleFrameCaptureModel::DoSetup (void)
{
  m_phy = CreateObject<SpectrumWifiPhy> ();
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phy->SetErrorRateModel (error);
  m_phy->SetChannelNumber (CHANNEL_NUMBER);
  m_phy->SetFrequency (FREQUENCY);

  m_phy->SetReceiveOkCallback (MakeCallback (&TestSimpleFrameCaptureModel::RxSuccess, this));
  m_phy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&TestSimpleFrameCaptureModel::RxDropped, this));

  Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel> ();
  preambleDetectionModel->SetAttribute ("Threshold", DoubleValue (2));
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);

  Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel> ();
  frameCaptureModel->SetAttribute ("Margin", DoubleValue (5));
  frameCaptureModel->SetAttribute ("CaptureWindow", TimeValue (MicroSeconds (16)));
  m_phy->SetFrameCaptureModel (frameCaptureModel);
}

void
TestSimpleFrameCaptureModel::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 2;
  double rxPowerDbm = -30;
  m_phy->AssignStreams (streamNumber);

  // CASE 1: send two packets with same power within the capture window:
  // PHY should not switch reception because they have same power.

  Simulator::Schedule (Seconds (1.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm, 1000);
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (10.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm, 1500);
  Simulator::Schedule (Seconds (1.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
  Simulator::Schedule (Seconds (1.2), &TestSimpleFrameCaptureModel::Reset, this);

  // CASE 2: send two packets with second one 6 dB weaker within the capture window:
  // PHY should not switch reception because first one has higher power.

  Simulator::Schedule (Seconds (2.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm, 1000);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (10.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm - 6, 1500);
  Simulator::Schedule (Seconds (2.1), &TestSimpleFrameCaptureModel::Expect1000BPacketReceived, this);
  Simulator::Schedule (Seconds (2.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
  Simulator::Schedule (Seconds (2.2), &TestSimpleFrameCaptureModel::Reset, this);

  // CASE 3: send two packets with second one 6 dB higher within the capture window:
  // PHY should switch reception because the second one has a higher power.

  Simulator::Schedule (Seconds (3.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm, 1000);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (10.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm + 6, 1500);
  Simulator::Schedule (Seconds (3.1), &TestSimpleFrameCaptureModel::Expect1000BPacketDropped, this);
  Simulator::Schedule (Seconds (3.1), &TestSimpleFrameCaptureModel::Expect1500BPacketReceived, this);
  Simulator::Schedule (Seconds (3.2), &TestSimpleFrameCaptureModel::Reset, this);

  // CASE 4: send two packets with second one 6 dB higher after the capture window:
  // PHY should not switch reception because capture window duration has elapsed when the second packet arrives.

  Simulator::Schedule (Seconds (4.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm, 1000);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (25.0), &TestSimpleFrameCaptureModel::SendPacket, this, rxPowerDbm + 6, 1500);
  Simulator::Schedule (Seconds (4.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
  Simulator::Schedule (Seconds (4.2), &TestSimpleFrameCaptureModel::Reset, this);

  Simulator::Run ();
  Simulator::Destroy ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test PHY state upon success or failure of L-SIG and SIG-A
 */
class TestPhyHeadersReception : public TestCase
{
public:
  TestPhyHeadersReception ();
  virtual ~TestPhyHeadersReception ();

protected:
  virtual void DoSetup (void);
  Ptr<SpectrumWifiPhy> m_phy; ///< Phy
  /**
   * Send packet function
   * \param rxPowerDbm the transmit power in dBm
   */
  void SendPacket (double rxPowerDbm);

private:
  virtual void DoRun (void);

  /**
   * Check the PHY state
   * \param expectedState the expected PHY state
   */
  void CheckPhyState (WifiPhyState expectedState);
  void DoCheckPhyState (WifiPhyState expectedState);
};

TestPhyHeadersReception::TestPhyHeadersReception ()
: TestCase ("PHY headers reception test")
{
}

void
TestPhyHeadersReception::SendPacket (double rxPowerDbm)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs7 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);
  MpduType mpdutype = NORMAL_MPDU;

  Ptr<Packet> pkt = Create<Packet> (1000);
  WifiMacHeader hdr;
  WifiMacTrailer trailer;

  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint32_t size = pkt->GetSize () + hdr.GetSize () + trailer.GetSerializedSize ();
  Time txDuration = m_phy->CalculateTxDuration (size, txVector, m_phy->GetFrequency (), mpdutype, 0);
  hdr.SetDuration (txDuration);

  pkt->AddHeader (hdr);
  pkt->AddTrailer (trailer);

  HeSigHeader heSig;
  heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
  heSig.SetBssColor (txVector.GetBssColor ());
  heSig.SetChannelWidth (txVector.GetChannelWidth ());
  heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2);
  pkt->AddHeader (heSig);

  LSigHeader sig;
  pkt->AddHeader (sig);

  WifiPhyTag tag (txVector.GetPreambleType (), txVector.GetMode ().GetModulationClass (), 1);
  pkt->AddPacketTag (tag);

  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (rxPowerDbm), GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = 0;
  txParams->duration = txDuration;
  txParams->packet = pkt;

  m_phy->StartRx (txParams);
}

void
TestPhyHeadersReception::CheckPhyState (WifiPhyState expectedState)
{
  //This is needed to make sure PHY state will be checked as the last event if a state change occured at the exact same time as the check
  Simulator::ScheduleNow (&TestPhyHeadersReception::DoCheckPhyState, this, expectedState);
}

void
TestPhyHeadersReception::DoCheckPhyState (WifiPhyState expectedState)
{
  WifiPhyState currentState;
  PointerValue ptr;
  m_phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_LOG_FUNCTION (this << currentState);
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}


TestPhyHeadersReception::~TestPhyHeadersReception ()
{
  m_phy = 0;
}

void
TestPhyHeadersReception::DoSetup (void)
{
  m_phy = CreateObject<SpectrumWifiPhy> ();
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phy->SetErrorRateModel (error);
  m_phy->SetChannelNumber (CHANNEL_NUMBER);
  m_phy->SetFrequency (FREQUENCY);
}

void
TestPhyHeadersReception::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 0;
  m_phy->AssignStreams (streamNumber);

  // RX power > CCA-ED
  double rxPowerDbm = -50;
  
  // CASE 1: send one packet followed by a second one with same power between the end of the 4us preamble detection window
  // and the start of L-SIG of the first packet: reception should be aborted since L-SIG cannot be decoded (SNR too low).

  Simulator::Schedule (Seconds (1.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (10), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  // At 10 us, STA PHY STATE should be RX.
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (10.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // At 24us (end of L-SIG), STA PHY STATE should go to CCA_BUSY because L-SIG reception failed and the total energy is above CCA-ED.
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (23999), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (24000), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8 + 10 = 162.8us.
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (162799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (162800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  // CASE 2: send one packet followed by a second one 3 dB weaker between the end of the 4us preamble detection window
  // and the start of L-SIG of the first packet: reception should not be aborted since L-SIG can be decoded (SNR high enough).

  Simulator::Schedule (Seconds (2.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (10), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm - 3);
  // At 10 us, STA PHY STATE should be RX.
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (10.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // At 24us (end of L-SIG), STA PHY STATE should be unchanged because L-SIG reception should have succeeded.
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (24.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
  // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY should first be seen as CCA_BUSY for 10us.
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (152799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (152800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (162799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (162800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  // CASE 3: send one packet followed by a second one with same power between the end of L-SIG and the start of HE-SIG of the first packet:
  // PHY header reception should not succeed but PHY should stay in RX state for the duration estimated from L-SIG.

  Simulator::Schedule (Seconds (3.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (25), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  // At 44 us (end of HE-SIG), STA PHY STATE should be RX (even though reception of HE-SIG failed)
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (44.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // STA PHY STATE should move back to IDLE once the duration estimated from L-SIG has elapsed, i.e. at 152.8us.
  // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY should first be seen as CCA_BUSY for 25us.
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (152799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (152800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (177799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (177800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  // CASE 4: send one packet followed by a second one 3 dB weaker between the end of L-SIG and the start of HE-SIG of the first packet:
  // PHY header reception should succeed.
  
  Simulator::Schedule (Seconds (4.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (25), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm - 3);
  // At 44 us (end of HE-SIG), STA PHY STATE should be RX.
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (44.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // STA PHY STATE should move back to IDLE once the duration estimated from L-SIG has elapsed, i.e. at 152.8us.
  // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY should first be seen as CCA_BUSY for 25us.
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (152799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (152800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (177799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::CCA_BUSY);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (177800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  // RX power < CCA-ED
  rxPowerDbm = -70;

  // CASE 5: send one packet followed by a second one with same power between the end of the 4us preamble detection window
  // and the start of L-SIG of the first packet: reception should be aborted since L-SIG cannot be decoded (SNR too low).
  
  Simulator::Schedule (Seconds (5.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (10), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  // At 10 us, STA PHY STATE should be RX.
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (10.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // At 24us (end of L-SIG), STA PHY STATE should go to IDLE because L-SIG reception failed and the total energy is below CCA-ED.
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (23999), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (24000), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  // CASE 6: send one packet followed by a second one 3 dB weaker between the end of the 4us preamble detection window
  // and the start of L-SIG of the first packet: reception should not be aborted since L-SIG can be decoded (SNR high enough).

  Simulator::Schedule (Seconds (6.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (10), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm - 3);
  // At 10 us, STA PHY STATE should be RX.
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (10.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // At 24us (end of L-SIG), STA PHY STATE should be unchanged because L-SIG reception should have succeeded.
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (24.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (152799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (152800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  // CASE 7: send one packet followed by a second one with same power between the end of L-SIG and the start of HE-SIG of the first packet:
  // PHY header reception should not succeed but PHY should stay in RX state for the duration estimated from L-SIG.

  Simulator::Schedule (Seconds (7.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (7.0) + MicroSeconds (25), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  // At 44 us (end of HE-SIG), STA PHY STATE should be RX (even though reception of HE-SIG failed).
  Simulator::Schedule (Seconds (7.0) + MicroSeconds (44.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // STA PHY STATE should move back to IDLE once the duration estimated from L-SIG has elapsed, i.e. at 152.8us.
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (152799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (152800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  // CASE 8: send one packet followed by a second one 3 dB weaker between the end of L-SIG and the start of HE-SIG of the first packet:
  // PHY header reception should succeed.

  Simulator::Schedule (Seconds (8.0), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm);
  Simulator::Schedule (Seconds (8.0) + MicroSeconds (25), &TestPhyHeadersReception::SendPacket, this, rxPowerDbm - 3);
  // At 44 us (end of HE-SIG), STA PHY STATE should be RX.
  Simulator::Schedule (Seconds (8.0) + MicroSeconds (44.0), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  // STA PHY STATE should move back to IDLE once the duration estimated from L-SIG has elapsed, i.e. at 152.8us.
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (152799), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (152800), &TestPhyHeadersReception::CheckPhyState, this, WifiPhyState::IDLE);

  Simulator::Run ();
  Simulator::Destroy ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief A-MPDU reception test
 */
class TestAmpduReception : public TestCase
{
public:
  TestAmpduReception ();
  virtual ~TestAmpduReception ();

protected:
  virtual void DoSetup (void);

private:
  virtual void DoRun (void);

  /**
   * RX success function
   * \param p the packet
   * \param snr the SNR
   * \param txVector the transmit vector
   * \param statusPerMpdu reception status per MPDU
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu);
  /**
   * RX failure function
   * \param p the packet
   */
  void RxFailure (Ptr<Packet> p);
  /**
   * RX dropped function
   * \param p the packet
   * \param reason the reason
   */
  void RxDropped (Ptr<const Packet> p, WifiPhyRxfailureReason reason);
  /**
   * Increment reception success bitmap.
   * \param size the size of the received packet
   */
  void IncrementSuccessBitmap (uint32_t size);
  /**
   * Increment reception failure bitmap.
   * \param size the size of the received packet
   */
  void IncrementFailureBitmap (uint32_t size);

  /**
   * Reset bitmaps function
   */
  void ResetBitmaps();

  /**
   * Send A-MPDU with 3 MPDUs of different size (i-th MSDU will have 100 bytes more than (i-1)-th).
   * \param rxPowerDbm the transmit power in dBm
   * \param referencePacketSize the reference size of the packets in bytes (i-th MSDU will have 100 bytes more than (i-1)-th)
   */
  void SendAmpduWithThreeMpdus (double rxPowerDbm, uint32_t referencePacketSize);

  /**
   * Check the RX success bitmap for A-MPDU 1
   * \param expected the expected bitmap
   */
  void CheckRxSuccessBitmapAmpdu1 (uint8_t expected);
  /**
   * Check the RX success bitmap for A-MPDU 2
   * \param expected the expected bitmap
   */
  void CheckRxSuccessBitmapAmpdu2 (uint8_t expected);
  /**
   * Check the RX failure bitmap for A-MPDU 1
   * \param expected the expected bitmap
   */
  void CheckRxFailureBitmapAmpdu1 (uint8_t expected);
  /**
   * Check the RX failure bitmap for A-MPDU 2
   * \param expected the expected bitmap
   */
  void CheckRxFailureBitmapAmpdu2 (uint8_t expected);
  /**
   * Check the RX dropped bitmap for A-MPDU 1
   * \param expected the expected bitmap
   */
  void CheckRxDroppedBitmapAmpdu1 (uint8_t expected);
  /**
   * Check the RX dropped bitmap for A-MPDU 2
   * \param expected the expected bitmap
   */
  void CheckRxDroppedBitmapAmpdu2 (uint8_t expected);

  /**
   * Check the PHY state
   * \param expectedState the expected PHY state
   */
  void CheckPhyState (WifiPhyState expectedState);

  Ptr<SpectrumWifiPhy> m_phy; ///< Phy

  uint8_t m_rxSuccessBitmapAmpdu1;
  uint8_t m_rxSuccessBitmapAmpdu2;

  uint8_t m_rxFailureBitmapAmpdu1;
  uint8_t m_rxFailureBitmapAmpdu2;

  uint8_t m_rxDroppedBitmapAmpdu1;
  uint8_t m_rxDroppedBitmapAmpdu2;
};

TestAmpduReception::TestAmpduReception ()
: TestCase ("A-MPDU reception test"),
  m_rxSuccessBitmapAmpdu1 (0),
  m_rxSuccessBitmapAmpdu2 (0),
  m_rxFailureBitmapAmpdu1 (0),
  m_rxFailureBitmapAmpdu2 (0),
  m_rxDroppedBitmapAmpdu1 (0),
  m_rxDroppedBitmapAmpdu2 (0)
{
}

TestAmpduReception::~TestAmpduReception ()
{
  m_phy = 0;
}

void
TestAmpduReception::ResetBitmaps()
{
  m_rxSuccessBitmapAmpdu1 = 0;
  m_rxSuccessBitmapAmpdu2 = 0;
  m_rxFailureBitmapAmpdu1 = 0;
  m_rxFailureBitmapAmpdu2 = 0;
  m_rxDroppedBitmapAmpdu1 = 0;
  m_rxDroppedBitmapAmpdu2 = 0;
}

void
TestAmpduReception::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu)
{
  NS_LOG_FUNCTION (this << p << snr << txVector);
  if (IsAmpdu (p))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (p);
      NS_ABORT_MSG_IF (mpdus.size () != statusPerMpdu.size (), "Should have one receive status per MPDU");
      auto rxOkForMpdu = statusPerMpdu.begin ();
      for (const auto & mpdu : mpdus)
        {
          if (*rxOkForMpdu)
            {
              IncrementSuccessBitmap (mpdu->GetSize ());
            }
          else
            {
              IncrementFailureBitmap (mpdu->GetSize ());
            }
          ++rxOkForMpdu;
        }
    }
  else
    {
      IncrementSuccessBitmap (p->GetSize ());
    }
}

void
TestAmpduReception::IncrementSuccessBitmap (uint32_t size)
{
  if (size == 1030) //A-MPDU 1 - MPDU #1
    {
      m_rxSuccessBitmapAmpdu1 |= 1;
    }
  else if (size == 1130) //A-MPDU 1 - MPDU #2
    {
      m_rxSuccessBitmapAmpdu1 |= (1 << 1);
    }
  else if (size == 1230) //A-MPDU 1 - MPDU #3
    {
      m_rxSuccessBitmapAmpdu1 |= (1 << 2);
    }
  else if (size == 1330) //A-MPDU 2 - MPDU #1
    {
      m_rxSuccessBitmapAmpdu2 |= 1;
    }
  else if (size == 1430) //A-MPDU 2 - MPDU #2
    {
      m_rxSuccessBitmapAmpdu2 |= (1 << 1);
    }
  else if (size == 1530) //A-MPDU 2 - MPDU #3
    {
      m_rxSuccessBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::RxFailure (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  if (IsAmpdu (p))
    {
      std::list<Ptr<const Packet>> mpdus = MpduAggregator::PeekMpdus (p);
      for (const auto & mpdu : mpdus)
        {
          IncrementFailureBitmap (mpdu->GetSize ());
        }
    }
  else
    {
      IncrementFailureBitmap (p->GetSize ());
    }
}

void
TestAmpduReception::IncrementFailureBitmap (uint32_t size)
{
  if (size == 1030) //A-MPDU 1 - MPDU #1
    {
      m_rxFailureBitmapAmpdu1 |= 1;
    }
  else if (size == 1130) //A-MPDU 1 - MPDU #2
    {
      m_rxFailureBitmapAmpdu1 |= (1 << 1);
    }
  else if (size == 1230) //A-MPDU 1 - MPDU #3
    {
      m_rxFailureBitmapAmpdu1 |= (1 << 2);
    }
  else if (size == 1330) //A-MPDU 2 - MPDU #1
    {
      m_rxFailureBitmapAmpdu2 |= 1;
    }
  else if (size == 1430) //A-MPDU 2 - MPDU #2
    {
      m_rxFailureBitmapAmpdu2 |= (1 << 1);
    }
  else if (size == 1530) //A-MPDU 2 - MPDU #3
    {
      m_rxFailureBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::RxDropped (Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
  NS_LOG_FUNCTION (this << p << reason);
  if (p->GetSize () == 1030) //A-MPDU 1 - MPDU #1
    {
      m_rxDroppedBitmapAmpdu1 |= 1;
    }
  else if (p->GetSize () == 1130) //A-MPDU 1 - MPDU #2
    {
      m_rxDroppedBitmapAmpdu1 |= (1 << 1);
    }
  else if (p->GetSize () == 1230) //A-MPDU 1 - MPDU #3
    {
      m_rxDroppedBitmapAmpdu1 |= (1 << 2);
    }
  else if (p->GetSize () == 1330) //A-MPDU 2 - MPDU #1
    {
      m_rxDroppedBitmapAmpdu2 |= 1;
    }
  else if (p->GetSize () == 1430) //A-MPDU 2 - MPDU #2
    {
      m_rxDroppedBitmapAmpdu2 |= (1 << 1);
    }
  else if (p->GetSize () == 1530) //A-MPDU 2 - MPDU #3
    {
      m_rxDroppedBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::CheckRxSuccessBitmapAmpdu1 (uint8_t expected)
{
  NS_TEST_ASSERT_MSG_EQ (m_rxSuccessBitmapAmpdu1, expected, "RX success bitmap for A-MPDU 1 is not as expected");
}

void
TestAmpduReception::CheckRxSuccessBitmapAmpdu2 (uint8_t expected)
{
  NS_TEST_ASSERT_MSG_EQ (m_rxSuccessBitmapAmpdu2, expected, "RX success bitmap for A-MPDU 2 is not as expected");
}

void
TestAmpduReception::CheckRxFailureBitmapAmpdu1 (uint8_t expected)
{
  NS_TEST_ASSERT_MSG_EQ (m_rxFailureBitmapAmpdu1, expected, "RX failure bitmap for A-MPDU 1 is not as expected");
}

void
TestAmpduReception::CheckRxFailureBitmapAmpdu2 (uint8_t expected)
{
  NS_TEST_ASSERT_MSG_EQ (m_rxFailureBitmapAmpdu2, expected, "RX failure bitmap for A-MPDU 2 is not as expected");
}

void
TestAmpduReception::CheckRxDroppedBitmapAmpdu1 (uint8_t expected)
{
  NS_TEST_ASSERT_MSG_EQ (m_rxDroppedBitmapAmpdu1, expected, "RX dropped bitmap for A-MPDU 1 is not as expected");
}

void
TestAmpduReception::CheckRxDroppedBitmapAmpdu2 (uint8_t expected)
{
  NS_TEST_ASSERT_MSG_EQ (m_rxDroppedBitmapAmpdu2, expected, "RX dropped bitmap for A-MPDU 2 is not as expected");
}

void
TestAmpduReception::CheckPhyState (WifiPhyState expectedState)
{
  WifiPhyState currentState;
  PointerValue ptr;
  m_phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}

void
TestAmpduReception::SendAmpduWithThreeMpdus (double rxPowerDbm, uint32_t referencePacketSize)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs0 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, true, false);

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);

  std::vector<Ptr<WifiMacQueueItem>> mpduList;
  for (size_t i = 0; i < 3; ++i)
    {
      Ptr<Packet> p = Create<Packet> (referencePacketSize + i * 100);
      mpduList.push_back (Create<WifiMacQueueItem> (p, hdr));
    }
  Ptr<WifiPsdu> psdu = Create<WifiPsdu> (mpduList);
  
  Time txDuration = m_phy->CalculateTxDuration (psdu->GetSize (), txVector, m_phy->GetFrequency ());
  psdu->SetDuration (txDuration);
  Ptr<Packet> pkt = psdu->GetPacket ()->Copy ();

  HeSigHeader heSig;
  heSig.SetMcs (txVector.GetMode ().GetMcsValue ());
  heSig.SetBssColor (txVector.GetBssColor ());
  heSig.SetChannelWidth (txVector.GetChannelWidth ());
  heSig.SetGuardIntervalAndLtfSize (txVector.GetGuardInterval (), 2);
  pkt->AddHeader (heSig);
  
  LSigHeader sig;
  uint16_t length = ((ceil ((static_cast<double> (txDuration.GetNanoSeconds () - (20 * 1000)) / 1000) / 4.0) * 3) - 3 - 2);
  sig.SetLength (length);
  pkt->AddHeader (sig);

  WifiPhyTag tag (txVector.GetPreambleType (), txVector.GetMode ().GetModulationClass (), 1);
  pkt->AddPacketTag (tag);

  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (rxPowerDbm), GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = 0;
  txParams->duration = txDuration;
  txParams->packet = pkt;

  m_phy->StartRx (txParams);
}

void
TestAmpduReception::DoSetup (void)
{
  m_phy = CreateObject<SpectrumWifiPhy> ();
  m_phy->ConfigureStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel> ();
  m_phy->SetErrorRateModel (error);
  m_phy->SetChannelNumber (CHANNEL_NUMBER);
  m_phy->SetFrequency (FREQUENCY);

  m_phy->SetReceiveOkCallback (MakeCallback (&TestAmpduReception::RxSuccess, this));
  m_phy->SetReceiveErrorCallback (MakeCallback (&TestAmpduReception::RxFailure, this));
  m_phy->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&TestAmpduReception::RxDropped, this));

  Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel> ();
  preambleDetectionModel->SetAttribute ("Threshold", DoubleValue (2));
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);

  Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel> ();
  frameCaptureModel->SetAttribute ("Margin", DoubleValue (5));
  frameCaptureModel->SetAttribute ("CaptureWindow", TimeValue (MicroSeconds (16)));
  m_phy->SetFrameCaptureModel (frameCaptureModel);
}

void
TestAmpduReception::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (2);
  int64_t streamNumber = 1;
  double rxPowerDbm = -30;
  m_phy->AssignStreams (streamNumber);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 1: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (1.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (2), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been successfully received.
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (1.2), &TestAmpduReception::ResetBitmaps, this);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 2: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (2.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1300);

  // All MPDUs of A-MPDU 1 should have been received.
  Simulator::Schedule (Seconds (2.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (2.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (2.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been ignored.
  Simulator::Schedule (Seconds (2.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (2.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (2.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (2.2), &TestAmpduReception::ResetBitmaps, this);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 3: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (3.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (10), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been successfully received.
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (3.2), &TestAmpduReception::ResetBitmaps, this);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 4: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (4.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (10), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1300);

  // All MPDUs of A-MPDU 1 should have been received.
  Simulator::Schedule (Seconds (4.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (4.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (4.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been ignored.
  Simulator::Schedule (Seconds (4.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (4.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (4.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (4.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 5: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. after the frame capture window, during the payload of MPDU #1).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (5.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (100), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been successfully received.
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (5.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 6: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. after the frame capture window, during the payload of MPDU #1).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (6.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (100), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1300);

  // All MPDUs of A-MPDU 1 should have been received.
  Simulator::Schedule (Seconds (6.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (6.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (6.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been ignored.
  Simulator::Schedule (Seconds (6.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (6.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (6.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (6.2), &TestAmpduReception::ResetBitmaps, this);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 7: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received during the payload of MPDU #2.
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (7.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (1100000), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been successfully received.
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (7.2), &TestAmpduReception::ResetBitmaps, this);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 8: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received during the payload of MPDU #2.
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (8.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (1100000), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm - 100, 1300);

  // All MPDUs of A-MPDU 1 should have been received.
  Simulator::Schedule (Seconds (8.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (8.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (8.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been ignored.
  Simulator::Schedule (Seconds (8.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (8.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (8.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (8.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 9: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power 3 dB higher.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (9.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (9.0) + MicroSeconds (2), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 3, 1300);

  // All MPDUs of A-MPDU 1 should have been dropped.
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been received with errors.
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (9.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 10: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (10.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (10.0) + MicroSeconds (2), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been dropped (preamble detection failed).
  Simulator::Schedule (Seconds (10.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (10.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (10.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped as well.
  Simulator::Schedule (Seconds (10.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (10.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (10.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (10.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 11: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 3 dB higher.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (11.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 3, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (11.0) + MicroSeconds (2), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (11.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (11.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (11.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped.
  Simulator::Schedule (Seconds (11.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (11.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (11.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (11.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 12: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power 3 dB higher.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (12.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (12.0) + MicroSeconds (10), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 3, 1300);

 // All MPDUs of A-MPDU 1 should have been received with errors (PHY header reception failed and thus incorrect decoding of payload).
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped (even though TX power is higher, it is not high enough to get the PHY reception switched)
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (12.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 13: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (13.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (13.0) + MicroSeconds (10), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been received with errors (PHY header reception failed and thus incorrect decoding of payload).
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped as well.
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (13.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 14: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 3 dB higher.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (14.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 3, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (14.0) + MicroSeconds (10), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (14.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (14.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (14.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped.
  Simulator::Schedule (Seconds (14.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (14.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (14.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (14.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 15: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power 6 dB higher.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (15.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (15.0) + MicroSeconds (10), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 6, 1300);

  // All MPDUs of A-MPDU 1 should have been dropped because PHY reception switched to A-MPDU 2.
  Simulator::Schedule (Seconds (15.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (15.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (15.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been successfully received
  Simulator::Schedule (Seconds (15.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (15.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (15.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);

  Simulator::Schedule (Seconds (15.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 16: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 6 dB higher.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (16.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 6, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (16.0) + MicroSeconds (10), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been successfully received.
  Simulator::Schedule (Seconds (16.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (16.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (16.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been dropped.
  Simulator::Schedule (Seconds (16.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (16.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (16.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (16.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 17: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power 6 dB higher.
  // The second A-MPDU is received 25 microseconds after the first A-MPDU (i.e. after the frame capture window, but still during PHY header).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (17.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (17.0) + MicroSeconds (25), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 6, 1300);

  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped (no reception switch, MPDUs dropped because PHY is already in RX state).
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (17.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 18: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 6 dB higher.
  // The second A-MPDU is received 25 microseconds after the first A-MPDU (i.e. after the frame capture window, but still during PHY header).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (18.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 6, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (18.0) + MicroSeconds (25), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been successfully received.
  Simulator::Schedule (Seconds (18.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (18.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (18.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been dropped.
  Simulator::Schedule (Seconds (18.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (18.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (18.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (18.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 19: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
  // The second A-MPDU is received 25 microseconds after the first A-MPDU (i.e. after the frame capture window, but still during PHY header).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (19.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (19.0) + MicroSeconds (25), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped.
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (19.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 20: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power 6 dB higher.
  // The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. during the payload of MPDU #1).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (20.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (20.0) + MicroSeconds (100), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 6, 1300);

  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (20.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (20.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (20.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped (no reception switch, MPDUs dropped because PHY is already in RX state).
  Simulator::Schedule (Seconds (20.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (20.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (20.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (20.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 21: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 6 dB higher.
  // The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. during the payload of MPDU #1).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (21.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm + 6, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (21.0) + MicroSeconds (100), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been successfully received.
  Simulator::Schedule (Seconds (21.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (21.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (21.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);

  // All MPDUs of A-MPDU 2 should have been dropped.
  Simulator::Schedule (Seconds (21.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (21.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (21.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (21.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 22: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
  // The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. during the payload of MPDU #1).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (22.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (22.0) + MicroSeconds (100), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (22.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (22.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000111);
  Simulator::Schedule (Seconds (22.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);

  // All MPDUs of A-MPDU 2 should have been dropped.
  Simulator::Schedule (Seconds (22.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (22.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (22.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (22.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////
  // CASE 23: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
  // The second A-MPDU is received during the payload of MPDU #2.
  ///////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (23.0), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1000);

  // A-MPDU 2
  Simulator::Schedule (Seconds (23.0) + NanoSeconds (1100000), &TestAmpduReception::SendAmpduWithThreeMpdus, this, rxPowerDbm, 1300);

  // The first MPDU of A-MPDU 1 should have been successfully received (no interference).
  // The two other MPDUs failed due to interference and are marked as failure (and dropped).
  Simulator::Schedule (Seconds (23.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000001);
  Simulator::Schedule (Seconds (23.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000110);
  Simulator::Schedule (Seconds (23.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000110);

  // The two first MPDUs of A-MPDU 2 are dropped because PHY is already in RX state (receiving A-MPDU 1).
  // The last MPDU of A-MPDU 2 is interference free (A-MPDU 1 transmission is finished) but is dropped because its PHY preamble and header were not received.
  Simulator::Schedule (Seconds (23.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (23.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (23.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000111);

  Simulator::Schedule (Seconds (23.2), &TestAmpduReception::ResetBitmaps, this);

  Simulator::Run ();
  Simulator::Destroy ();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi PHY reception Test Suite
 */
class WifiPhyReceptionTestSuite : public TestSuite
{
public:
  WifiPhyReceptionTestSuite ();
};

WifiPhyReceptionTestSuite::WifiPhyReceptionTestSuite ()
  : TestSuite ("wifi-phy-reception", UNIT)
{
  AddTestCase (new TestThresholdPreambleDetectionWithoutFrameCapture, TestCase::QUICK);
  AddTestCase (new TestThresholdPreambleDetectionWithFrameCapture, TestCase::QUICK);
  AddTestCase (new TestSimpleFrameCaptureModel, TestCase::QUICK);
  AddTestCase (new TestPhyHeadersReception, TestCase::QUICK);
  AddTestCase (new TestAmpduReception, TestCase::QUICK);
}

static WifiPhyReceptionTestSuite wifiPhyReceptionTestSuite; ///< the test suite
