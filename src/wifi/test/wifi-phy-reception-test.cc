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
#include "ns3/pointer.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-trailer.h"
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
   */
  void RxFailure (void);
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
TestThresholdPreambleDetectionWithoutFrameCapture::RxFailure (void)
{
  NS_LOG_FUNCTION (this);
  m_countRxFailure++;
}

TestThresholdPreambleDetectionWithoutFrameCapture::~TestThresholdPreambleDetectionWithoutFrameCapture ()
{
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
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);
}

// Test that the expected number of packet receptions occur.
void
TestThresholdPreambleDetectionWithoutFrameCapture::DoRun (void)
{
  double txPowerDbm = -30;

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
   */
  void RxFailure (void);
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
TestThresholdPreambleDetectionWithFrameCapture::RxFailure (void)
{
  NS_LOG_FUNCTION (this);
  m_countRxFailure++;
}

TestThresholdPreambleDetectionWithFrameCapture::~TestThresholdPreambleDetectionWithFrameCapture ()
{
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
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);

  Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel> ();
  m_phy->SetFrameCaptureModel (frameCaptureModel);
}

// Test that the expected number of packet receptions occur.
void
TestThresholdPreambleDetectionWithFrameCapture::DoRun (void)
{
  double txPowerDbm = -30;
  
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
   * \param rxPower the rx power (W)
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
  m_phy->SetPreambleDetectionModel (preambleDetectionModel);
  
  Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel> ();
  m_phy->SetFrameCaptureModel (frameCaptureModel);
}

// Test that the expected number of packet receptions occur.
void
TestSimpleFrameCaptureModel::DoRun (void)
{
  double txPowerDbm = -30;
  
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
}

static WifiPhyReceptionTestSuite wifiPhyReceptionTestSuite; ///< the test suite
