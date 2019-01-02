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
   * \param txPowerDbm the transmit power in dBm
   */
  void SendPacket (double txPowerDbm);
  /**
   * Spectrum wifi receive success function
   * \param p the packet
   * \param snr the SNR
   * \param txVector the transmit vector
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector);
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
TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket (double txPowerDbm)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs11 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);
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
  WifiPhyTag tag (txVector, mpdutype, 1);
  pkt->AddPacketTag (tag);
  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (txPowerDbm), GUARD_WIDTH);
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
  WifiPhyState currentState;
  PointerValue ptr;
  m_phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount (uint32_t expectedSuccessCount, uint32_t expectedFailureCount)
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxSuccess, expectedSuccessCount, "Didn't receive right number of successful packets");
  NS_TEST_ASSERT_MSG_EQ (m_countRxFailure, expectedFailureCount, "Didn't receive right number of unsuccessful packets");
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector)
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
  preambleDetectionModel->SetAttribute ("Threshold", DoubleValue (2));
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);
}

// Test that the expected number of packet receptions occur.
void
TestThresholdPreambleDetectionWithoutFrameCapture::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 0;
  double txPowerDbm = -30;
  m_phy->AssignStreams (streamNumber);

  //CASE 1: send one packet and check PHY state: packet reception should succeed

  Simulator::Schedule (Seconds (1.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm);
  // At 4us, STA PHY STATE should be IDLE
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // At 5us, STA PHY STATE should be RX
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Packet should have been successfully received
  Simulator::Schedule (Seconds (1.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 0);

  //CASE 2: send two packets with same power within the 4us window and check PHY state: PHY preamble detection should fail

  Simulator::Schedule (Seconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (2.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 0);

  //CASE 3: send two packets with second one 3 dB weaker within the 4us window and check PHY state: PHY preamble detection should succeed and payload reception should fail

  Simulator::Schedule (Seconds (3.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm - 3);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // In this case, the first packet should be marked as a failure
  Simulator::Schedule (Seconds (3.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 1);

  //CASE 4: send two packets with second one 3 dB higher within the 4us window and check PHY state: PHY preamble detection should fail and no packets should enter the reception stage
  Simulator::Schedule (Seconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm + 3);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (4.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 1);

  //CASE 5: idem but send the second packet after the 4us window: PHY preamble detection should succeed and payload reception should fail

  Simulator::Schedule (Seconds (5.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (6.0), &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket, this, txPowerDbm + 3);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (5.1), &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount, this, 1, 2);

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
   * \param txPowerDbm the transmit power in dBm
   */
  void SendPacket (double txPowerDbm);
  /**
   * Spectrum wifi receive success function
   * \param p the packet
   * \param snr the SNR
   * \param txVector the transmit vector
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector);
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
TestThresholdPreambleDetectionWithFrameCapture::SendPacket (double txPowerDbm)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs11 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);
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
  WifiPhyTag tag (txVector, mpdutype, 1);
  pkt->AddPacketTag (tag);
  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (txPowerDbm), GUARD_WIDTH);
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
  WifiPhyState currentState;
  PointerValue ptr;
  m_phy->GetAttribute ("State", ptr);
  Ptr <WifiPhyStateHelper> state = DynamicCast <WifiPhyStateHelper> (ptr.Get<WifiPhyStateHelper> ());
  currentState = state->GetState ();
  NS_TEST_ASSERT_MSG_EQ (currentState, expectedState, "PHY State " << currentState << " does not match expected state " << expectedState << " at " << Simulator::Now ());
}

void
TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount (uint32_t expectedSuccessCount, uint32_t expectedFailureCount)
{
  NS_TEST_ASSERT_MSG_EQ (m_countRxSuccess, expectedSuccessCount, "Didn't receive right number of successful packets");
  NS_TEST_ASSERT_MSG_EQ (m_countRxFailure, expectedFailureCount, "Didn't receive right number of unsuccessful packets");
}

void
TestThresholdPreambleDetectionWithFrameCapture::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector)
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
  preambleDetectionModel->SetAttribute ("Threshold", DoubleValue (2));
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);

  Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel> ();
  frameCaptureModel->SetAttribute ("Margin", DoubleValue (5));
  frameCaptureModel->SetAttribute ("CaptureWindow", TimeValue (MicroSeconds (16)));
  m_phy->SetFrameCaptureModel (frameCaptureModel);
}

// Test that the expected number of packet receptions occur.
void
TestThresholdPreambleDetectionWithFrameCapture::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 1;
  double txPowerDbm = -30;
  m_phy->AssignStreams (streamNumber);
  
  //CASE 1: send one packet and check PHY state: packet reception should succeed
  
  Simulator::Schedule (Seconds (1.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm);
  // At 4us, STA PHY STATE should be IDLE
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // At 5us, STA PHY STATE should be RX
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // Packet should have been successfully received
  Simulator::Schedule (Seconds (1.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 0);
  
  //CASE 2: send two packets with same power within the 4us window and check PHY state: PHY preamble detection should fail
  
  Simulator::Schedule (Seconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  // No more packet should have been successfully received, and since preamble detection did not pass the packet should not have been counted as a failure
  Simulator::Schedule (Seconds (2.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 0);
  
  //CASE 3: send two packets with second one 3 dB weaker within the 4us window and check PHY state: PHY preamble detection should succeed and payload reception should fail
  
  Simulator::Schedule (Seconds (3.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm - 3);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  // In this case, the first packet should be marked as a failure
  Simulator::Schedule (Seconds (3.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 1);
  
  //CASE 4: send two packets with second one 3 dB higher within the 4us window and check PHY state: PHY preamble detection should switch and succeed for the second packet, payload reception should fail
  Simulator::Schedule (Seconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (2.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm + 3);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (4.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 2);

  //CASE 5: idem but send the second packet after the 4us window: PHY preamble detection should succeed and payload reception should fail
  
  Simulator::Schedule (Seconds (5.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (6.0), &TestThresholdPreambleDetectionWithFrameCapture::SendPacket, this, txPowerDbm + 3);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (4.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::IDLE);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (5.0), &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState, this, WifiPhyState::RX);
  Simulator::Schedule (Seconds (5.1), &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount, this, 1, 3);

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
   * \param txPowerDbm the transmit power in dBm
   * \param packetSize the size of the packet in bytes
   */
  void SendPacket (double txPowerDbm, uint32_t packetSize);
  /**
   * Spectrum wifi receive success function
   * \param p the packet
   * \param snr the SNR
   * \param txVector the transmit vector
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector);
  /**
   * RX packet dropped function
   * \param p the packet
   */
  void RxDropped (Ptr<const Packet> p);

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
TestSimpleFrameCaptureModel::SendPacket (double txPowerDbm, uint32_t packetSize)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs0 (), 0, WIFI_PREAMBLE_HE_SU, 800, 1, 1, 0, 20, false, false);
  MpduType mpdutype = NORMAL_MPDU;
  
  Ptr<Packet> pkt = Create<Packet> (packetSize);
  WifiMacHeader hdr;
  WifiMacTrailer trailer;
  
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint32_t size = pkt->GetSize () + hdr.GetSize () + trailer.GetSerializedSize ();
  Time txDuration = m_phy->CalculateTxDuration (size, txVector, m_phy->GetFrequency (), mpdutype, 0);
  hdr.SetDuration (txDuration);
  
  pkt->AddHeader (hdr);
  pkt->AddTrailer (trailer);
  WifiPhyTag tag (txVector, mpdutype, 1);
  pkt->AddPacketTag (tag);
  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (txPowerDbm), GUARD_WIDTH);
  Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters> ();
  txParams->psd = txPowerSpectrum;
  txParams->txPhy = 0;
  txParams->duration = txDuration;
  txParams->packet = pkt;
  
  m_phy->StartRx (txParams);
}

void
TestSimpleFrameCaptureModel::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector)
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
TestSimpleFrameCaptureModel::RxDropped (Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (this << p);
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

// Test that the expected number of packet receptions occur.
void
TestSimpleFrameCaptureModel::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 2;
  double txPowerDbm = -30;
  m_phy->AssignStreams (streamNumber);
  
  //CASE 1: send two packets with same power within the capture window: should not switch reception because they have same power
  Simulator::Schedule (Seconds (1.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm, 1000);
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (10.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm, 1500);
  Simulator::Schedule (Seconds (1.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
  Reset ();

  //CASE 2: send two packets with second one 6 dB weaker within the capture window: should not switch reception because first one has higher power
  Simulator::Schedule (Seconds (2.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm, 1000);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (10.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm - 6, 1500);
  Simulator::Schedule (Seconds (2.1), &TestSimpleFrameCaptureModel::Expect1000BPacketReceived, this);
  Simulator::Schedule (Seconds (2.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
  Reset ();

  //CASE 3: send two packets with second one 6 dB higher within the capture window: should switch reception
  Simulator::Schedule (Seconds (3.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm, 1000);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (10.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm + 6, 1500);
  Simulator::Schedule (Seconds (3.1), &TestSimpleFrameCaptureModel::Expect1000BPacketDropped, this);
  Simulator::Schedule (Seconds (3.1), &TestSimpleFrameCaptureModel::Expect1500BPacketReceived, this);
  Reset ();

  //CASE 4: send two packets with second one 6 dB higher after the capture window: should not switch reception because capture window duration has elapsed when the second packet arrives
  Simulator::Schedule (Seconds (4.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm, 1000);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (25.0), &TestSimpleFrameCaptureModel::SendPacket, this, txPowerDbm + 6, 1500);
  Simulator::Schedule (Seconds (4.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
  Reset ();

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
   */
  void RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector);
  /**
   * RX failure function
   * \param p the packet
   */
  void RxFailure (Ptr<Packet> p);
  /**
   * RX dropped function
   * \param p the packet
   */
  void RxDropped (Ptr<const Packet> p);

  /**
   * Reset bitmaps function
   */
  void ResetBitmaps();

  /**
   * schedule send MPDU function
   * \param txPowerDbm the transmit power in dBm
   * \param packetSize the size of the packet in bytes
   * \param preamble the preamble
   * \param mpdutype the MPDU type
   * \param remainingNbOfMpdus the remaining number of MPDUs to send after this MPDU
   */
  void ScheduleSendMpdu (double txPowerDbm, uint32_t packetSize, WifiPreamble preamble, MpduType mpdutype, uint8_t remainingNbOfMpdus);
  /**
   * Send MPDU function
   * \param txPowerDbm the transmit power in dBm
   * \param packetSize the size of the packet in bytes
   * \param preamble the preamble
   * \param mpdutype the MPDU type
   * \param remainingNbOfMpdus the remaining number of MPDUs to send after this MPDU
   */
  void SendMpdu (double txPowerDbm, uint32_t packetSize, WifiPreamble preamble, MpduType mpdutype, uint8_t remainingNbOfMpdus);

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
TestAmpduReception::RxSuccess (Ptr<Packet> p, double snr, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << p << snr << txVector);
  if (p->GetSize () == 1030) //A-MPDU 1 - MPDU #1
    {
      m_rxSuccessBitmapAmpdu1 |= 1;
    }
  else if (p->GetSize () == 1130) //A-MPDU 1 - MPDU #2
    {
      m_rxSuccessBitmapAmpdu1 |= (1 << 1);
    }
  else if (p->GetSize () == 1230) //A-MPDU 1 - MPDU #3
    {
      m_rxSuccessBitmapAmpdu1 |= (1 << 2);
    }
  else if (p->GetSize () == 1330) //A-MPDU 2 - MPDU #1
    {
      m_rxSuccessBitmapAmpdu2 |= 1;
    }
  else if (p->GetSize () == 1430) //A-MPDU 2 - MPDU #2
    {
      m_rxSuccessBitmapAmpdu2 |= (1 << 1);
    }
  else if (p->GetSize () == 1530) //A-MPDU 2 - MPDU #3
    {
      m_rxSuccessBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::RxFailure (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  if (p->GetSize () == 1030) //A-MPDU 1 - MPDU #1
    {
      m_rxFailureBitmapAmpdu1 |= 1;
    }
  else if (p->GetSize () == 1130) //A-MPDU 1 - MPDU #2
    {
      m_rxFailureBitmapAmpdu1 |= (1 << 1);
    }
  else if (p->GetSize () == 1230) //A-MPDU 1 - MPDU #3
    {
      m_rxFailureBitmapAmpdu1 |= (1 << 2);
    }
  else if (p->GetSize () == 1330) //A-MPDU 2 - MPDU #1
    {
      m_rxFailureBitmapAmpdu2 |= 1;
    }
  else if (p->GetSize () == 1430) //A-MPDU 2 - MPDU #2
    {
      m_rxFailureBitmapAmpdu2 |= (1 << 1);
    }
  else if (p->GetSize () == 1530) //A-MPDU 2 - MPDU #3
    {
      m_rxFailureBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::RxDropped (Ptr<const Packet> p)
{
  NS_LOG_FUNCTION (this << p);
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
TestAmpduReception::ScheduleSendMpdu (double txPowerDbm, uint32_t packetSize, WifiPreamble preamble, MpduType mpdutype, uint8_t remainingNbOfMpdus)
{
  //This is needed to make sure this is the last scheduled event and to avoid the start of an MPDU is triggered before the end of the previous MPDU.
  Simulator::ScheduleNow (&TestAmpduReception::SendMpdu, this, txPowerDbm, packetSize, preamble, mpdutype, remainingNbOfMpdus);
}

void
TestAmpduReception::SendMpdu (double txPowerDbm, uint32_t packetSize, WifiPreamble preamble, MpduType mpdutype, uint8_t remainingNbOfMpdus)
{
  WifiTxVector txVector = WifiTxVector (WifiPhy::GetHeMcs0 (), 0, preamble, 800, 1, 1, 0, 20, false, false);
  
  Ptr<Packet> pkt = Create<Packet> (packetSize);
  WifiMacHeader hdr;
  WifiMacTrailer trailer;
  
  hdr.SetType (WIFI_MAC_QOSDATA);
  hdr.SetQosTid (0);
  uint32_t size = pkt->GetSize () + hdr.GetSize () + trailer.GetSerializedSize ();
  Time txDuration = m_phy->CalculateTxDuration (size, txVector, m_phy->GetFrequency (), mpdutype, 0);
  hdr.SetDuration (txDuration);
  
  pkt->AddHeader (hdr);
  pkt->AddTrailer (trailer);
  AmpduTag ampdutag;
  ampdutag.SetRemainingNbOfMpdus (remainingNbOfMpdus);
  pkt->AddPacketTag (ampdutag);
  WifiPhyTag tag (txVector, mpdutype, 1);
  pkt->AddPacketTag (tag);
  Ptr<SpectrumValue> txPowerSpectrum = WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity (FREQUENCY, CHANNEL_WIDTH, DbmToW (txPowerDbm), GUARD_WIDTH);
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

// Test that the expected number of packet receptions occur.
void
TestAmpduReception::DoRun (void)
{
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (1);
  int64_t streamNumber = 3;
  double txPowerDbm = -30;
  m_phy->AssignStreams (streamNumber);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 1: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  // A-MPDU 1
  Simulator::Schedule (Seconds (1.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (1.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (2), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (2) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (1.0) + MicroSeconds (2) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);
  
  // All MPDUs of A-MPDU 2 should have been received.
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (1.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);
  
  Simulator::Schedule (Seconds (1.2), &TestAmpduReception::ResetBitmaps, this);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 2: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  // A-MPDU 1
  Simulator::Schedule (Seconds (2.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (2.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (2.0) + MicroSeconds (2) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (3.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (3.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (10), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (10) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (3.0) + MicroSeconds (10) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);
  
  // All MPDUs of A-MPDU 2 should have been received.
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (3.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);
  
  Simulator::Schedule (Seconds (3.2), &TestAmpduReception::ResetBitmaps, this);

  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 4: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame capture window).
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  // A-MPDU 1
  Simulator::Schedule (Seconds (4.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (4.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (10), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (10) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (4.0) + MicroSeconds (10) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (5.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (5.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (100), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (100) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (5.0) + MicroSeconds (100) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);
  
  // All MPDUs of A-MPDU 2 should have been received.
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (5.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);
  
  Simulator::Schedule (Seconds (5.2), &TestAmpduReception::ResetBitmaps, this);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 6: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. after the frame capture window, during the payload of MPDU #1).
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (6.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (6.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (100), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (100) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (6.0) + MicroSeconds (100) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (7.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (1100000), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (1100000) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (7.0) + NanoSeconds (1100000) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // All MPDUs of A-MPDU 1 should have been ignored.
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000000);
  
  // All MPDUs of A-MPDU 2 should have been received.
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000111);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000000);
  Simulator::Schedule (Seconds (7.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000000);
  
  Simulator::Schedule (Seconds (7.2), &TestAmpduReception::ResetBitmaps, this);
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 8: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received with power under RX sensitivity.
  // The second A-MPDU is received during the payload of MPDU #2.
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (8.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (1100000), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (1100000) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (8.0) + NanoSeconds (1100000) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm - 100, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (9.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (9.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (9.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (9.0) + MicroSeconds (2), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (9.0) + MicroSeconds (2) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (9.0) + MicroSeconds (2) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);

  // All MPDUs of A-MPDU 1 should have been dropped.
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu1, this, 0b00000111);
  
  // MPDUs #1 and #2 of A-MPDU 2 should have been received with errors. M/PDU #3 should have been successfully received (it depends on random stream!)
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu2, this, 0b00000100);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu2, this, 0b00000011);
  Simulator::Schedule (Seconds (9.1), &TestAmpduReception::CheckRxDroppedBitmapAmpdu2, this, 0b00000011);
  
  Simulator::Schedule (Seconds (9.2), &TestAmpduReception::ResetBitmaps, this);

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  // CASE 10: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
  // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble detection).
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////

  // A-MPDU 1
  Simulator::Schedule (Seconds (10.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (10.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (10.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (10.0) + MicroSeconds (2), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (10.0) + MicroSeconds (2) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (10.0) + MicroSeconds (2) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (11.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (11.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (11.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (11.0) + MicroSeconds (2), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (11.0) + MicroSeconds (2) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (11.0) + MicroSeconds (2) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (12.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (12.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (12.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (12.0) + MicroSeconds (10), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (12.0) + MicroSeconds (10) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (12.0) + MicroSeconds (10) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);

 // All MPDUs of A-MPDU 1 should have been dropped (PHY header reception failed).
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (12.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000001);
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
  Simulator::Schedule (Seconds (13.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (13.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (13.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (13.0) + MicroSeconds (10), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (13.0) + MicroSeconds (10) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (13.0) + MicroSeconds (10) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // All MPDUs of A-MPDU 1 should have been dropped (PHY header reception failed).
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (13.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000001);
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
  Simulator::Schedule (Seconds (14.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (14.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (14.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 3, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (14.0) + MicroSeconds (10), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (14.0) + MicroSeconds (10) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (14.0) + MicroSeconds (10) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (15.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (15.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (15.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (15.0) + MicroSeconds (10), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (15.0) + MicroSeconds (10) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (15.0) + MicroSeconds (10) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (16.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (16.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (16.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (16.0) + MicroSeconds (10), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (16.0) + MicroSeconds (10) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (16.0) + MicroSeconds (10) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (17.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (17.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (17.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (17.0) + MicroSeconds (25), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (17.0) + MicroSeconds (25) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (17.0) + MicroSeconds (25) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (17.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000111);
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
  Simulator::Schedule (Seconds (18.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (18.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (18.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (18.0) + MicroSeconds (25), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (18.0) + MicroSeconds (25) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (18.0) + MicroSeconds (25) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (19.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (19.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (19.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (19.0) + MicroSeconds (25), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (19.0) + MicroSeconds (25) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (19.0) + MicroSeconds (25) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // All MPDUs of A-MPDU 1 should have been received with errors.
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxSuccessBitmapAmpdu1, this, 0b00000000);
  Simulator::Schedule (Seconds (19.1), &TestAmpduReception::CheckRxFailureBitmapAmpdu1, this, 0b00000111);
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
  Simulator::Schedule (Seconds (20.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (20.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (20.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (20.0) + MicroSeconds (100), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (20.0) + MicroSeconds (100) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (20.0) + MicroSeconds (100) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (21.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (21.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (21.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm + 6, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (21.0) + MicroSeconds (100), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (21.0) + MicroSeconds (100) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (21.0) + MicroSeconds (100) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (22.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (22.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (22.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (22.0) + MicroSeconds (100), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (22.0) + MicroSeconds (100) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (22.0) + MicroSeconds (100) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  Simulator::Schedule (Seconds (23.0), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1000, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (23.0) + NanoSeconds (1004369), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1100, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (23.0) + NanoSeconds (2055172), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1200, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
  // A-MPDU 2
  Simulator::Schedule (Seconds (23.0) + NanoSeconds (1100000), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1300, WIFI_PREAMBLE_HE_SU, MPDU_IN_AGGREGATE, 2);
  Simulator::Schedule (Seconds (23.0) + NanoSeconds (1100000) + NanoSeconds (1283343), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1400, WIFI_PREAMBLE_NONE, MPDU_IN_AGGREGATE, 1);
  Simulator::Schedule (Seconds (23.0) + NanoSeconds (1100000) + NanoSeconds (2613120), &TestAmpduReception::ScheduleSendMpdu, this, txPowerDbm, 1500, WIFI_PREAMBLE_NONE, LAST_MPDU_IN_AGGREGATE, 0);
  
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
  AddTestCase (new TestAmpduReception, TestCase::QUICK);
}

static WifiPhyReceptionTestSuite wifiPhyReceptionTestSuite; ///< the test suite
