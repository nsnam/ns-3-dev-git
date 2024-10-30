/*
 * Copyright (c) 2018 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/ampdu-tag.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/he-phy.h"
#include "ns3/he-ppdu.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mpdu-aggregator.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simple-frame-capture-model.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/test.h"
#include "ns3/threshold-preamble-detection-model.h"
#include "ns3/wifi-bandwidth-filter.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-phy-interface.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"

#include <optional>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyReceptionTest");

static const uint8_t CHANNEL_NUMBER = 36;
static const MHz_u FREQUENCY{5180};
static const MHz_u CHANNEL_WIDTH{20};
static const MHz_u GUARD_WIDTH = CHANNEL_WIDTH; // expanded to channel width to model spectrum mask

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Wifi Phy Reception Test base class
 */
class WifiPhyReceptionTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param test_name the test name
     */
    WifiPhyReceptionTest(std::string test_name);
    /**
     * Destructor
     */
    ~WifiPhyReceptionTest() override = default;

  protected:
    void DoSetup() override;
    void DoTeardown() override;

    /**
     * Send packet function
     * @param rxPower the transmit power
     * @param packetSize the size of the packet in bytes
     * @param mcs the MCS to transmit the packet
     */
    void SendPacket(dBm_u rxPower, uint32_t packetSize, uint8_t mcs);

    /**
     * Schedule now to check  the PHY state
     * @param expectedState the expected PHY state
     */
    void CheckPhyState(WifiPhyState expectedState);
    /**
     * Check the PHY state now
     * @param expectedState the expected PHY state
     */
    void DoCheckPhyState(WifiPhyState expectedState);

    Ptr<SpectrumWifiPhy> m_phy; //!< the PHY
    uint64_t m_uid{0};          //!< the UID to use for the PPDU
};

WifiPhyReceptionTest::WifiPhyReceptionTest(std::string test_name)
    : TestCase{test_name}
{
}

void
WifiPhyReceptionTest::SendPacket(dBm_u rxPower, uint32_t packetSize, uint8_t mcs)
{
    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs(mcs),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         false);

    Ptr<Packet> pkt = Create<Packet>(packetSize);
    WifiMacHeader hdr;

    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
    Time txDuration =
        SpectrumWifiPhy::CalculateTxDuration(psdu->GetSize(), txVector, m_phy->GetPhyBand());

    Ptr<WifiPpdu> ppdu =
        Create<HePpdu>(psdu, txVector, m_phy->GetOperatingChannel(), txDuration, m_uid++);

    Ptr<SpectrumValue> txPowerSpectrum =
        WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(FREQUENCY,
                                                                    CHANNEL_WIDTH,
                                                                    DbmToW(rxPower),
                                                                    GUARD_WIDTH);

    Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters>();
    txParams->psd = txPowerSpectrum;
    txParams->txPhy = nullptr;
    txParams->duration = txDuration;
    txParams->ppdu = ppdu;

    m_phy->StartRx(txParams, nullptr);
}

void
WifiPhyReceptionTest::CheckPhyState(WifiPhyState expectedState)
{
    // This is needed to make sure PHY state will be checked as the last event if a state change
    // occurred at the exact same time as the check
    Simulator::ScheduleNow(&WifiPhyReceptionTest::DoCheckPhyState, this, expectedState);
}

void
WifiPhyReceptionTest::DoCheckPhyState(WifiPhyState expectedState)
{
    WifiPhyState currentState;
    PointerValue ptr;
    m_phy->GetAttribute("State", ptr);
    Ptr<WifiPhyStateHelper> state = DynamicCast<WifiPhyStateHelper>(ptr.Get<WifiPhyStateHelper>());
    currentState = state->GetState();
    NS_LOG_FUNCTION(this << currentState);
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
WifiPhyReceptionTest::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<Node> node = CreateObject<Node>();
    Ptr<WifiNetDevice> dev = CreateObject<WifiNetDevice>();
    m_phy = CreateObject<SpectrumWifiPhy>();
    Ptr<InterferenceHelper> interferenceHelper = CreateObject<InterferenceHelper>();
    m_phy->SetInterferenceHelper(interferenceHelper);
    Ptr<ErrorRateModel> error = CreateObject<NistErrorRateModel>();
    m_phy->SetErrorRateModel(error);
    m_phy->SetDevice(dev);
    m_phy->AddChannel(spectrumChannel);
    m_phy->SetOperatingChannel(WifiPhy::ChannelTuple{CHANNEL_NUMBER, 0, WIFI_PHY_BAND_5GHZ, 0});
    m_phy->ConfigureStandard(WIFI_STANDARD_80211ax);
    dev->SetPhy(m_phy);
    node->AddDevice(dev);
}

void
WifiPhyReceptionTest::DoTeardown()
{
    m_phy->Dispose();
    m_phy = nullptr;
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Preamble detection test w/o frame capture
 */
class TestThresholdPreambleDetectionWithoutFrameCapture : public WifiPhyReceptionTest
{
  public:
    TestThresholdPreambleDetectionWithoutFrameCapture();

  protected:
    void DoSetup() override;

    /**
     * Spectrum wifi receive success function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);
    /**
     * Spectrum wifi receive failure function
     * @param psdu the PSDU
     */
    void RxFailure(Ptr<const WifiPsdu> psdu);
    uint32_t m_countRxSuccess{0}; ///< count RX success
    uint32_t m_countRxFailure{0}; ///< count RX failure

  private:
    void DoRun() override;

    /**
     * Check the number of received packets
     * @param expectedSuccessCount the number of successfully received packets
     * @param expectedFailureCount the number of unsuccessfully received packets
     */
    void CheckRxPacketCount(uint32_t expectedSuccessCount, uint32_t expectedFailureCount);
};

TestThresholdPreambleDetectionWithoutFrameCapture::
    TestThresholdPreambleDetectionWithoutFrameCapture()
    : WifiPhyReceptionTest(
          "Threshold preamble detection model test when no frame capture model is applied")
{
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount(uint32_t expectedSuccessCount,
                                                                      uint32_t expectedFailureCount)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccess,
                          expectedSuccessCount,
                          "Didn't receive right number of successful packets");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailure,
                          expectedFailureCount,
                          "Didn't receive right number of unsuccessful packets");
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::RxSuccess(Ptr<const WifiPsdu> psdu,
                                                             RxSignalInfo rxSignalInfo,
                                                             const WifiTxVector& txVector,
                                                             const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccess++;
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailure++;
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::DoSetup()
{
    WifiPhyReceptionTest::DoSetup();

    m_phy->SetReceiveOkCallback(
        MakeCallback(&TestThresholdPreambleDetectionWithoutFrameCapture::RxSuccess, this));
    m_phy->SetReceiveErrorCallback(
        MakeCallback(&TestThresholdPreambleDetectionWithoutFrameCapture::RxFailure, this));

    Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel =
        CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(4));
    preambleDetectionModel->SetAttribute("MinimumRssi", DoubleValue(-82));
    m_phy->SetPreambleDetectionModel(preambleDetectionModel);
}

void
TestThresholdPreambleDetectionWithoutFrameCapture::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phy->AssignStreams(streamNumber);

    // RX power > CCA-ED > CCA-PD
    dBm_u rxPower{-50};

    // CASE 1: send one packet and check PHY state:
    // All reception stages should succeed and PHY state should be RX for the duration of the packet
    // minus the time to detect the preamble, otherwise it should be IDLE.

    Simulator::Schedule(Seconds(1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(1) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(1) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(1) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(1) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
    Simulator::Schedule(Seconds(1) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // Packet should have been successfully received
    Simulator::Schedule(Seconds(1.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        0);

    // CASE 2: send two packets with same power within the 4us window and check PHY state:
    // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than
    // the threshold of 4 dB), and PHY state should be CCA_BUSY since the total energy is above
    // CCA-ED (-62 dBm). CCA_BUSY state should last for the duration of the two packets minus the
    // time to detect the preamble.

    Simulator::Schedule(Seconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(2) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, no preamble is successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(2) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(2) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(2) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(2) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass, the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(2.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        0);

    // CASE 3: send two packets with second one 3 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should fail because SNR is too low (around 3 dB, which is lower
    // than the threshold of 4 dB), and PHY state should be CCA_BUSY since the total energy is above
    // CCA-ED (-62 dBm). CCA_BUSY state should last for the duration of the two packets minus the
    // time to detect the preamble.

    Simulator::Schedule(Seconds(3),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(3) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 4us, no preamble is successfully detected, hence STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(3) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(3) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(3) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(3.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        0);

    // CASE 4: send two packets with second one 6 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should succeed because SNR is high enough (around 6 dB, which
    // is higher than the threshold of 4 dB), but payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(4),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(4) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{6},
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(4) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(4) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(4) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
    // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY
    // should first be seen as CCA_BUSY for 2us.
    Simulator::Schedule(Seconds(4) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // In this case, the first packet should be marked as a failure
    Simulator::Schedule(Seconds(4.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        1);

    // CASE 5: send two packets with second one 3 dB higher within the 4us window and check PHY
    // state: PHY preamble detection should fail because SNR is too low (around -3 dB, which is
    // lower than the threshold of 4 dB), and PHY state should be CCA_BUSY since the total energy is
    // above CCA-ED (-62 dBm). CCA_BUSY state should last for the duration of the two packets minus
    // the time to detect the preamble.

    Simulator::Schedule(Seconds(5),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(5) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{3.0},
                        1000,
                        7);
    // At 6us (hence 4us after the last signal is received), no preamble is successfully detected,
    // hence STA PHY STATE should move from IDLE to CCA_BUSY
    Simulator::Schedule(Seconds(5) + NanoSeconds(5999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(5) + NanoSeconds(6000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(5) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(5) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(5.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        1);

    // CCA-PD < RX power < CCA-ED
    rxPower = dBm_u{-70};

    // CASE 6: send one packet and check PHY state:
    // All reception stages should succeed and PHY state should be RX for the duration of the packet
    // minus the time to detect the preamble, otherwise it should be IDLE.

    Simulator::Schedule(Seconds(6),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(6) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(6) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(6) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(6) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
    Simulator::Schedule(Seconds(6) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(6) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // Packet should have been successfully received
    Simulator::Schedule(Seconds(6.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        1);

    // CASE 7: send two packets with same power within the 4us window and check PHY state:
    // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than
    // the threshold of 4 dB), and PHY state should be CCA_BUSY since it should detect the start of
    // a valid OFDM transmission at a receive level greater than or equal to the minimum modulation
    // and coding rate sensitivity (–82 dBm for 20 MHz channel spacing).

    Simulator::Schedule(Seconds(7),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(7) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(7) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(7.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        1);

    // CASE 8: send two packets with second one 3 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should fail PHY preamble detection should fail because SNR is
    // too low (around 3 dB, which is lower than the threshold of 4 dB), and PHY state should be
    // CCA_BUSY since it should detect the start of a valid OFDM transmission at a receive level
    // greater than or equal to the minimum modulation and coding rate sensitivity (–82 dBm for 20
    // MHz channel spacing).

    Simulator::Schedule(Seconds(8),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(8) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(8) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(8.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        1);

    // CASE 9: send two packets with second one 6 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should succeed because SNR is high enough (around 6 dB, which
    // is higher than the threshold of 4 dB), but payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(9),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(9) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{6},
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(9) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(9) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(9) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(9) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to CCA_BUSY at time
    // 152.8us.
    Simulator::Schedule(Seconds(9) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(9) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // In this case, the first packet should be marked as a failure
    Simulator::Schedule(Seconds(9.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        2);

    // CASE 10: send two packets with second one 3 dB higher within the 4us window and check PHY
    // state: PHY preamble detection should fail because SNR is too low (around -3 dB, which is
    // lower than the threshold of 4 dB), and PHY state should stay IDLE since the total energy is
    // below CCA-ED (-62 dBm).

    Simulator::Schedule(Seconds(10),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(10) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{3.0},
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(10) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(10.1),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        2);

    // CASE 11: send one packet with a power slightly above the minimum RSSI needed for the preamble
    // detection (-82 dBm) and check PHY state: preamble detection should succeed and PHY state
    // should move to RX.

    rxPower = dBm_u{-81};

    Simulator::Schedule(Seconds(11),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(11) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(11) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(11) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(11) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
    Simulator::Schedule(Seconds(11) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(11) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);

    // RX power < CCA-PD < CCA-ED
    rxPower = dBm_u{-83};

    // CASE 12: send one packet with a power slightly below the minimum RSSI needed for the preamble
    // detection (-82 dBm) and check PHY state: preamble detection should fail and PHY should be
    // kept in IDLE state.

    Simulator::Schedule(Seconds(12),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, STA PHY state should be IDLE
    Simulator::Schedule(Seconds(12) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithoutFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Preamble detection test w/o frame capture
 */
class TestThresholdPreambleDetectionWithFrameCapture : public WifiPhyReceptionTest
{
  public:
    TestThresholdPreambleDetectionWithFrameCapture();

  protected:
    void DoSetup() override;

    /**
     * Spectrum wifi receive success function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);
    /**
     * Spectrum wifi receive failure function
     * @param psdu the PSDU
     */
    void RxFailure(Ptr<const WifiPsdu> psdu);
    uint32_t m_countRxSuccess{0}; ///< count RX success
    uint32_t m_countRxFailure{0}; ///< count RX failure

  private:
    void DoRun() override;

    /**
     * Check the number of received packets
     * @param expectedSuccessCount the number of successfully received packets
     * @param expectedFailureCount the number of unsuccessfuly received packets
     */
    void CheckRxPacketCount(uint32_t expectedSuccessCount, uint32_t expectedFailureCount);
};

TestThresholdPreambleDetectionWithFrameCapture::TestThresholdPreambleDetectionWithFrameCapture()
    : WifiPhyReceptionTest(
          "Threshold preamble detection model test when simple frame capture model is applied")
{
}

void
TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount(uint32_t expectedSuccessCount,
                                                                   uint32_t expectedFailureCount)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccess,
                          expectedSuccessCount,
                          "Didn't receive right number of successful packets");
    NS_TEST_ASSERT_MSG_EQ(m_countRxFailure,
                          expectedFailureCount,
                          "Didn't receive right number of unsuccessful packets");
}

void
TestThresholdPreambleDetectionWithFrameCapture::RxSuccess(Ptr<const WifiPsdu> psdu,
                                                          RxSignalInfo rxSignalInfo,
                                                          const WifiTxVector& txVector,
                                                          const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << txVector);
    m_countRxSuccess++;
}

void
TestThresholdPreambleDetectionWithFrameCapture::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailure++;
}

void
TestThresholdPreambleDetectionWithFrameCapture::DoSetup()
{
    WifiPhyReceptionTest::DoSetup();

    m_phy->SetReceiveOkCallback(
        MakeCallback(&TestThresholdPreambleDetectionWithFrameCapture::RxSuccess, this));
    m_phy->SetReceiveErrorCallback(
        MakeCallback(&TestThresholdPreambleDetectionWithFrameCapture::RxFailure, this));

    Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel =
        CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(4));
    preambleDetectionModel->SetAttribute("MinimumRssi", DoubleValue(-82));
    m_phy->SetPreambleDetectionModel(preambleDetectionModel);

    Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel>();
    frameCaptureModel->SetAttribute("Margin", DoubleValue(5));
    frameCaptureModel->SetAttribute("CaptureWindow", TimeValue(MicroSeconds(16)));
    m_phy->SetFrameCaptureModel(frameCaptureModel);
}

void
TestThresholdPreambleDetectionWithFrameCapture::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 1;
    m_phy->AssignStreams(streamNumber);

    // RX power > CCA-ED > CCA-PD
    dBm_u rxPower{-50};

    // CASE 1: send one packet and check PHY state:
    // All reception stages should succeed and PHY state should be RX for the duration of the packet
    // minus the time to detect the preamble, otherwise it should be IDLE.

    Simulator::Schedule(Seconds(1),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(1) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(1) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(1) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(1) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
    Simulator::Schedule(Seconds(1) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(1) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // Packet should have been successfully received
    Simulator::Schedule(Seconds(1.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        0);

    // CASE 2: send two packets with same power within the 4us window and check PHY state:
    // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than
    // the threshold of 4 dB), and PHY state should be CCA_BUSY since the total energy is above
    // CCA-ED (-62 dBm). CCA_BUSY state should last for the duration of the two packets minus the
    // time to detect the preamble.

    Simulator::Schedule(Seconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(2) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, no preamble is successfully detected, hence STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(2) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(2) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(2) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(2) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(2.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        0);

    // CASE 3: send two packets with second one 3 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should fail because SNR is too low (around 3 dB, which is lower
    // than the threshold of 4 dB), and PHY state should be CCA_BUSY since the total energy is above
    // CCA-ED (-62 dBm). CCA_BUSY state should last for the duration of the two packets minus the
    // time to detect the preamble.

    Simulator::Schedule(Seconds(3),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(3) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 4us, no preamble is successfully detected, hence STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(3) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(3) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(3) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(3.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        0);

    // CASE 4: send two packets with second one 6 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should succeed because SNR is high enough (around 6 dB, which
    // is higher than the threshold of 4 dB), but payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(4),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(4) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{6},
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(4) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(4) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(4) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
    // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY
    // should first be seen as CCA_BUSY for 2us.
    Simulator::Schedule(Seconds(4) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // In this case, the first packet should be marked as a failure
    Simulator::Schedule(Seconds(4.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        1);

    // CASE 5: send two packets with second one 3 dB higher within the 4us window and check PHY
    // state: PHY preamble detection should switch because a higher packet is received within the
    // 4us window, but preamble detection should fail because SNR is too low (around 3 dB, which is
    // lower than the threshold of 4 dB), PHY state should be CCA_BUSY since the total energy is
    // above CCA-ED (-62 dBm).

    Simulator::Schedule(Seconds(5),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(5) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{3.0},
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(5) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // At 6us, STA PHY STATE should move from IDLE to CCA_BUSY
    Simulator::Schedule(Seconds(5) + NanoSeconds(5999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(5) + NanoSeconds(6000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(5) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(5) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(5.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        1);

    // CASE 6: send two packets with second one 6 dB higher within the 4us window and check PHY
    // state: PHY preamble detection should switch because a higher packet is received within the
    // 4us window, and preamble detection should succeed because SNR is high enough (around 6 dB,
    // which is higher than the threshold of 4 dB), Payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(6),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(6) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{6.0},
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(6) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // At 6us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(6) + NanoSeconds(5999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(6) + NanoSeconds(6000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 46us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(6) + NanoSeconds(45999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(6) + NanoSeconds(46000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(6) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(6) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // In this case, the second packet should be marked as a failure
    Simulator::Schedule(Seconds(6.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        2);

    // CASE 7: send two packets with same power at the exact same time and check PHY state:
    // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than
    // the threshold of 4 dB), and PHY state should be CCA_BUSY since the total energy is above
    // CCA-ED (-62 dBm). CCA_BUSY state should last for the duration of the two packets minus the
    // time to detect the preamble.

    Simulator::Schedule(Seconds(7),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(7),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, no preamble is successfully detected, hence STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(7) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(7) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(7) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(7) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(7.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        2);

    // CASE 8: send two packets with second one 3 dB weaker at the exact same time and check PHY
    // state: PHY preamble detection should fail because SNR is too low (around 3 dB, which is lower
    // than the threshold of 4 dB), and PHY state should be CCA_BUSY since the total energy is above
    // CCA-ED (-62 dBm). CCA_BUSY state should last for the duration of the two packets minus the
    // time to detect the preamble.

    Simulator::Schedule(Seconds(8),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(8),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 4us, no preamble is successfully detected, hence STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(8) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(8) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 us
    Simulator::Schedule(Seconds(8) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(8) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(8.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        2);

    // CASE 9: send two packets with second one 6 dB weaker at the exact same time and check PHY
    // state: PHY preamble detection should succeed because SNR is high enough (around 6 dB, which
    // is higher than the threshold of 4 dB), but payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(9),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(9),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{6},
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(9) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(9) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(9) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(9) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packets, PHY should be back to IDLE at time 152.8us.
    Simulator::Schedule(Seconds(9) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(9) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // In this case, the first packet should be marked as a failure
    Simulator::Schedule(Seconds(9.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        3);

    // CASE 10: send two packets with second one 3 dB higher at the exact same time and check PHY
    // state: PHY preamble detection should switch because a higher packet is received within the
    // 4us window, but preamble detection should fail because SNR is too low (around 3 dB, which is
    // lower than the threshold of 4 dB), PHY state should be CCA_BUSY since the total energy is
    // above CCA-ED (-62 dBm).

    Simulator::Schedule(Seconds(10),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(10),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{3.0},
                        1000,
                        7);
    // At 4us, no preamble is successfully detected, hence STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(10) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(10) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 us
    Simulator::Schedule(Seconds(10) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(10) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(10.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        3);

    // CASE 11: send two packets with second one 6 dB higher at the exact same time and check PHY
    // state: PHY preamble detection should switch because a higher packet is received within the
    // 4us window, and preamble detection should succeed because SNR is high enough (around 6 dB,
    // which is higher than the threshold of 4 dB), Payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(11),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(11),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{6.0},
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(11) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(11) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(11) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(11) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 us
    Simulator::Schedule(Seconds(11) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(11) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // In this case, the second packet should be marked as a failure
    Simulator::Schedule(Seconds(11.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        1,
                        4);

    // CCA-PD < RX power < CCA-ED
    rxPower = dBm_u{-70};

    // CASE 12: send one packet and check PHY state:
    // All reception stages should succeed and PHY state should be RX for the duration of the packet
    // minus the time to detect the preamble, otherwise it should be IDLE.

    Simulator::Schedule(Seconds(12),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(12) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(12) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(12) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(12) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us
    Simulator::Schedule(Seconds(12) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(12) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // Packet should have been successfully received
    Simulator::Schedule(Seconds(12.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        4);

    // CASE 13: send two packets with same power within the 4us window and check PHY state:
    // PHY preamble detection should fail because SNR is too low (around 0 dB, which is lower than
    // the threshold of 4 dB), and PHY state should be CCA_BUSY since it should detect the start of
    // a valid OFDM transmission at a receive level greater than or equal to the minimum modulation
    // and coding rate sensitivity (–82 dBm for 20 MHz channel spacing).

    Simulator::Schedule(Seconds(13),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(13) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(13) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(13.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        4);

    // CASE 14: send two packets with second one 3 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should fail PHY preamble detection should fail because SNR is
    // too low (around 3 dB, which is lower than the threshold of 4 dB), and PHY state should be
    // CCA_BUSY since it should detect the start of a valid OFDM transmission at a receive level
    // greater than or equal to the minimum modulation and coding rate sensitivity (–82 dBm for 20
    // MHz channel spacing).

    Simulator::Schedule(Seconds(14),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(14) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(14) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(14.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        4);

    // CASE 15: send two packets with second one 6 dB weaker within the 4us window and check PHY
    // state: PHY preamble detection should succeed because SNR is high enough (around 6 dB, which
    // is higher than the threshold of 4 dB), but payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(15),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(15) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower - dB_u{6},
                        1000,
                        7);
    // At 4us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(15) + NanoSeconds(3999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(15) + NanoSeconds(4000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(15) + NanoSeconds(43999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(15) + NanoSeconds(44000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to CCA_BUSY at time
    // 152.8us.
    Simulator::Schedule(Seconds(15) + NanoSeconds(152799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(15) + NanoSeconds(152800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // In this case, the first packet should be marked as a failure
    Simulator::Schedule(Seconds(15.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        5);

    // CASE 16: send two packets with second one 3 dB higher within the 4us window and check PHY
    // state: PHY preamble detection should switch because a higher packet is received within the
    // 4us window, but preamble detection should fail because SNR is too low (around 3 dB, which is
    // lower than the threshold of 4 dB). and PHY state should be CCA_BUSY since it should detect
    // the start of a valid OFDM transmission at a receive level greater than or equal to the
    // minimum modulation and coding rate sensitivity (–82 dBm for 20 MHz channel spacing).

    Simulator::Schedule(Seconds(16),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(16) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{3.0},
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(16) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // At 6us, STA PHY STATE should be CCA_BUSY
    Simulator::Schedule(Seconds(16) + MicroSeconds(6),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // No more packet should have been successfully received, and since preamble detection did not
    // pass the packet should not have been counted as a failure
    Simulator::Schedule(Seconds(16.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        5);

    // CASE 17: send two packets with second one 6 dB higher within the 4us window and check PHY
    // state: PHY preamble detection should switch because a higher packet is received within the
    // 4us window, and preamble detection should succeed because SNR is high enough (around 6 dB,
    // which is higher than the threshold of 4 dB), Payload reception should fail (SNR too low to
    // decode the modulation).

    Simulator::Schedule(Seconds(17),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(17) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{6.0},
                        1000,
                        7);
    // At 4us, STA PHY STATE should stay IDLE
    Simulator::Schedule(Seconds(17) + MicroSeconds(4),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // At 6us, preamble should be successfully detected and STA PHY STATE should move from IDLE to
    // CCA_BUSY
    Simulator::Schedule(Seconds(17) + NanoSeconds(5999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    Simulator::Schedule(Seconds(17) + NanoSeconds(6000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 46us, PHY header should be successfully received and STA PHY STATE should move from
    // CCA_BUSY to RX
    Simulator::Schedule(Seconds(17) + NanoSeconds(45999),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(17) + NanoSeconds(46000),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit each packet, PHY should be back to IDLE at time 152.8 + 2
    // = 154.8us
    Simulator::Schedule(Seconds(17) + NanoSeconds(154799),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(17) + NanoSeconds(154800),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);
    // In this case, the second packet should be marked as a failure
    Simulator::Schedule(Seconds(17.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        2,
                        6);

    rxPower = dBm_u{-50};
    // CASE 18: send two packets with second one 50 dB higher within the 4us window

    Simulator::Schedule(Seconds(18),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(18) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{50.0},
                        1000,
                        7);
    // The second packet should be received successfully
    Simulator::Schedule(Seconds(18.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        3,
                        6);

    // CASE 19: send two packets with second one 10 dB higher within the 4us window

    Simulator::Schedule(Seconds(19),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(19) + MicroSeconds(2),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{10.0},
                        1000,
                        7);
    // The second packet should be captured, but not decoded since SNR to low for used MCS
    Simulator::Schedule(Seconds(19.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        3,
                        7);

    // CASE 20: send two packets with second one 50 dB higher in the same time

    Simulator::Schedule(Seconds(20),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(20),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{50.0},
                        1000,
                        7);
    // The second packet should be received successfully, same as in CASE 13
    Simulator::Schedule(Seconds(20.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        4,
                        7);

    // CASE 21: send two packets with second one 10 dB higher in the same time

    Simulator::Schedule(Seconds(21),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    Simulator::Schedule(Seconds(21),
                        &TestThresholdPreambleDetectionWithFrameCapture::SendPacket,
                        this,
                        rxPower + dB_u{10.0},
                        1000,
                        7);
    // The second packet should be captured, but not decoded since SNR to low for used MCS, same as
    // in CASE 19
    Simulator::Schedule(Seconds(21.1),
                        &TestThresholdPreambleDetectionWithFrameCapture::CheckRxPacketCount,
                        this,
                        4,
                        8);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Simple frame capture model test
 */
class TestSimpleFrameCaptureModel : public WifiPhyReceptionTest
{
  public:
    TestSimpleFrameCaptureModel();

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Reset function
     */
    void Reset();
    /**
     * Spectrum wifi receive success function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);
    /**
     * RX dropped function
     * @param p the packet
     * @param reason the reason
     */
    void RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason);

    /**
     * Verify whether 1000 bytes packet has been received
     */
    void Expect1000BPacketReceived();
    /**
     * Verify whether 1500 bytes packet has been received
     */
    void Expect1500BPacketReceived();
    /**
     * Verify whether 1000 bytes packet has been dropped
     */
    void Expect1000BPacketDropped();
    /**
     * Verify whether 1500 bytes packet has been dropped
     */
    void Expect1500BPacketDropped();

    bool m_rxSuccess1000B{false}; ///< count received packets with 1000B payload
    bool m_rxSuccess1500B{false}; ///< count received packets with 1500B payload
    bool m_rxDropped1000B{false}; ///< count dropped packets with 1000B payload
    bool m_rxDropped1500B{false}; ///< count dropped packets with 1500B payload
};

TestSimpleFrameCaptureModel::TestSimpleFrameCaptureModel()
    : WifiPhyReceptionTest("Simple frame capture model test")
{
}

void
TestSimpleFrameCaptureModel::RxSuccess(Ptr<const WifiPsdu> psdu,
                                       RxSignalInfo rxSignalInfo,
                                       const WifiTxVector& txVector,
                                       const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    NS_ASSERT(!psdu->IsAggregate() || psdu->IsSingle());
    if (psdu->GetSize() == 1030)
    {
        m_rxSuccess1000B = true;
    }
    else if (psdu->GetSize() == 1530)
    {
        m_rxSuccess1500B = true;
    }
}

void
TestSimpleFrameCaptureModel::RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << p << reason);
    if (p->GetSize() == 1030)
    {
        m_rxDropped1000B = true;
    }
    else if (p->GetSize() == 1530)
    {
        m_rxDropped1500B = true;
    }
}

void
TestSimpleFrameCaptureModel::Reset()
{
    m_rxSuccess1000B = false;
    m_rxSuccess1500B = false;
    m_rxDropped1000B = false;
    m_rxDropped1500B = false;
}

void
TestSimpleFrameCaptureModel::Expect1000BPacketReceived()
{
    NS_TEST_ASSERT_MSG_EQ(m_rxSuccess1000B, true, "Didn't receive 1000B packet");
}

void
TestSimpleFrameCaptureModel::Expect1500BPacketReceived()
{
    NS_TEST_ASSERT_MSG_EQ(m_rxSuccess1500B, true, "Didn't receive 1500B packet");
}

void
TestSimpleFrameCaptureModel::Expect1000BPacketDropped()
{
    NS_TEST_ASSERT_MSG_EQ(m_rxDropped1000B, true, "Didn't drop 1000B packet");
}

void
TestSimpleFrameCaptureModel::Expect1500BPacketDropped()
{
    NS_TEST_ASSERT_MSG_EQ(m_rxDropped1500B, true, "Didn't drop 1500B packet");
}

void
TestSimpleFrameCaptureModel::DoSetup()
{
    WifiPhyReceptionTest::DoSetup();

    m_phy->SetReceiveOkCallback(MakeCallback(&TestSimpleFrameCaptureModel::RxSuccess, this));
    m_phy->TraceConnectWithoutContext("PhyRxDrop",
                                      MakeCallback(&TestSimpleFrameCaptureModel::RxDropped, this));

    Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel =
        CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(2));
    m_phy->SetPreambleDetectionModel(preambleDetectionModel);

    Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel>();
    frameCaptureModel->SetAttribute("Margin", DoubleValue(5));
    frameCaptureModel->SetAttribute("CaptureWindow", TimeValue(MicroSeconds(16)));
    m_phy->SetFrameCaptureModel(frameCaptureModel);
}

void
TestSimpleFrameCaptureModel::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 2;
    dBm_u rxPower{-30};
    m_phy->AssignStreams(streamNumber);

    // CASE 1: send two packets with same power within the capture window:
    // PHY should not switch reception because they have same power.

    Simulator::Schedule(Seconds(1),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower,
                        1000,
                        0);
    Simulator::Schedule(Seconds(1) + MicroSeconds(10),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower,
                        1500,
                        0);
    Simulator::Schedule(Seconds(1.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
    Simulator::Schedule(Seconds(1.2), &TestSimpleFrameCaptureModel::Reset, this);

    // CASE 2: send two packets with second one 6 dB weaker within the capture window:
    // PHY should not switch reception because first one has higher power.

    Simulator::Schedule(Seconds(2),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower,
                        1000,
                        0);
    Simulator::Schedule(Seconds(2) + MicroSeconds(10),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower - dB_u{6},
                        1500,
                        0);
    Simulator::Schedule(Seconds(2.1),
                        &TestSimpleFrameCaptureModel::Expect1000BPacketReceived,
                        this);
    Simulator::Schedule(Seconds(2.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
    Simulator::Schedule(Seconds(2.2), &TestSimpleFrameCaptureModel::Reset, this);

    // CASE 3: send two packets with second one 6 dB higher within the capture window:
    // PHY should switch reception because the second one has a higher power.

    Simulator::Schedule(Seconds(3),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower,
                        1000,
                        0);
    Simulator::Schedule(Seconds(3) + MicroSeconds(10),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower + dB_u{6.0},
                        1500,
                        0);
    Simulator::Schedule(Seconds(3.1), &TestSimpleFrameCaptureModel::Expect1000BPacketDropped, this);
    Simulator::Schedule(Seconds(3.1),
                        &TestSimpleFrameCaptureModel::Expect1500BPacketReceived,
                        this);
    Simulator::Schedule(Seconds(3.2), &TestSimpleFrameCaptureModel::Reset, this);

    // CASE 4: send two packets with second one 6 dB higher after the capture window:
    // PHY should not switch reception because capture window duration has elapsed when the second
    // packet arrives.

    Simulator::Schedule(Seconds(4),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower,
                        1000,
                        0);
    Simulator::Schedule(Seconds(4) + MicroSeconds(25),
                        &TestSimpleFrameCaptureModel::SendPacket,
                        this,
                        rxPower + dB_u{6.0},
                        1500,
                        0);
    Simulator::Schedule(Seconds(4.1), &TestSimpleFrameCaptureModel::Expect1500BPacketDropped, this);
    Simulator::Schedule(Seconds(4.2), &TestSimpleFrameCaptureModel::Reset, this);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test PHY state upon success or failure of L-SIG and SIG-A
 */
class TestPhyHeadersReception : public WifiPhyReceptionTest
{
  public:
    TestPhyHeadersReception();

  private:
    void DoRun() override;
};

TestPhyHeadersReception::TestPhyHeadersReception()
    : WifiPhyReceptionTest("PHY headers reception test")
{
}

void
TestPhyHeadersReception::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 0;
    m_phy->AssignStreams(streamNumber);

    // RX power > CCA-ED
    dBm_u rxPower{-50};

    // CASE 1: send one packet followed by a second one with same power between the end of the 4us
    // preamble detection window and the start of L-SIG of the first packet: reception should be
    // aborted since L-SIG cannot be decoded (SNR too low).

    Simulator::Schedule(Seconds(1), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(1) + MicroSeconds(10),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 10 us, STA PHY STATE should be CCA_BUSY.
    Simulator::Schedule(Seconds(1) + MicroSeconds(10),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us (end of PHY header), STA PHY STATE should not have moved to RX and be kept to
    // CCA_BUSY.
    Simulator::Schedule(Seconds(1) + NanoSeconds(44000),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8 + 10
    // = 162.8us.
    Simulator::Schedule(Seconds(1) + NanoSeconds(162799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(1) + NanoSeconds(162800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);

    // CASE 2: send one packet followed by a second one 3 dB weaker between the end of the 4us
    // preamble detection window and the start of L-SIG of the first packet: reception should not be
    // aborted since L-SIG can be decoded (SNR high enough).

    Simulator::Schedule(Seconds(2), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(2) + MicroSeconds(10),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 10 us, STA PHY STATE should be CCA_BUSY.
    Simulator::Schedule(Seconds(2) + MicroSeconds(10),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44us (end of PHY header), STA PHY STATE should have moved to RX since PHY header reception
    // should have succeeded.
    Simulator::Schedule(Seconds(2) + NanoSeconds(43999),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(2) + NanoSeconds(44000),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to IDLE at time 152.8us.
    // However, since there is a second packet transmitted with a power above CCA-ED (-62 dBm), PHY
    // should first be seen as CCA_BUSY for 10us.
    Simulator::Schedule(Seconds(2) + NanoSeconds(152799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(2) + NanoSeconds(152800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(2) + NanoSeconds(162799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(2) + NanoSeconds(162800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);

    // CASE 3: send one packet followed by a second one with same power between the end of L-SIG and
    // the start of HE-SIG of the first packet: PHY header reception should not succeed but PHY
    // should stay in RX state for the duration estimated from L-SIG.

    Simulator::Schedule(Seconds(3), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(3) + MicroSeconds(25),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 44us (end of PHY header), STA PHY STATE should not have moved to RX (HE-SIG failed) and be
    // kept to CCA_BUSY.
    Simulator::Schedule(Seconds(3) + MicroSeconds(44),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // STA PHY STATE should move back to IDLE once the duration estimated from L-SIG has elapsed,
    // i.e. at 152.8us. However, since there is a second packet transmitted with a power above
    // CCA-ED (-62 dBm), PHY should first be seen as CCA_BUSY for 25us.
    Simulator::Schedule(Seconds(3) + NanoSeconds(152799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + NanoSeconds(152800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + NanoSeconds(177799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(3) + NanoSeconds(177800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);

    // CASE 4: send one packet followed by a second one 3 dB weaker between the end of L-SIG and the
    // start of HE-SIG of the first packet: PHY header reception should succeed.

    Simulator::Schedule(Seconds(4), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(4) + MicroSeconds(25),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 10 us, STA PHY STATE should be CCA_BUSY.
    Simulator::Schedule(Seconds(4) + MicroSeconds(10),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44 us (end of HE-SIG), STA PHY STATE should move to RX since the PHY header reception
    // should have succeeded.
    Simulator::Schedule(Seconds(4) + NanoSeconds(43999),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(44000),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // STA PHY STATE should move back to IDLE once the duration estimated from L-SIG has elapsed,
    // i.e. at 152.8us. However, since there is a second packet transmitted with a power above
    // CCA-ED (-62 dBm), PHY should first be seen as CCA_BUSY for 25us.
    Simulator::Schedule(Seconds(4) + NanoSeconds(152799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(4) + NanoSeconds(152800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(177799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(4) + NanoSeconds(177800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::IDLE);

    // RX power < CCA-ED
    rxPower = dBm_u{-70};

    // CASE 5: send one packet followed by a second one with same power between the end of the 4us
    // preamble detection window and the start of L-SIG of the first packet: reception should be
    // aborted since L-SIG cannot be decoded (SNR too low).

    Simulator::Schedule(Seconds(5), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(5) + MicroSeconds(10),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 10 us, STA PHY STATE should be CCA_BUSY.
    Simulator::Schedule(Seconds(5) + MicroSeconds(10),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 24us (end of L-SIG), STA PHY STATE stay CCA_BUSY because L-SIG reception failed and the
    // start of a valid OFDM transmission has been detected
    Simulator::Schedule(Seconds(5) + NanoSeconds(24000),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);

    // CASE 6: send one packet followed by a second one 3 dB weaker between the end of the 4us
    // preamble detection window and the start of L-SIG of the first packet: reception should not be
    // aborted since L-SIG can be decoded (SNR high enough).

    Simulator::Schedule(Seconds(6), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(6) + MicroSeconds(10),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 10 us, STA PHY STATE should be CCA_BUSY.
    Simulator::Schedule(Seconds(6) + MicroSeconds(10),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 24us (end of L-SIG), STA PHY STATE should be unchanged because L-SIG reception should have
    // succeeded.
    Simulator::Schedule(Seconds(6) + MicroSeconds(24),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44 us (end of HE-SIG), STA PHY STATE should move to RX since the PHY header reception
    // should have succeeded.
    Simulator::Schedule(Seconds(6) + NanoSeconds(43999),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(6) + NanoSeconds(44000),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // Since it takes 152.8us to transmit the packet, PHY should be back to CCA_BUSY at time
    // 152.8us.
    Simulator::Schedule(Seconds(6) + NanoSeconds(152799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(6) + NanoSeconds(152800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);

    // CASE 7: send one packet followed by a second one with same power between the end of L-SIG and
    // the start of HE-SIG of the first packet: PHY header reception should not succeed but PHY
    // should stay in RX state for the duration estimated from L-SIG.

    Simulator::Schedule(Seconds(7), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(7) + MicroSeconds(25),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower,
                        1000,
                        7);
    // At 10 us, STA PHY STATE should be CCA_BUSY.
    Simulator::Schedule(Seconds(7) + MicroSeconds(10),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 24us (end of L-SIG), STA PHY STATE should be unchanged because L-SIG reception should have
    // succeeded.
    Simulator::Schedule(Seconds(7) + MicroSeconds(24),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44 us (end of HE-SIG), STA PHY STATE should be not have moved to RX since reception of
    // HE-SIG should have failed.
    Simulator::Schedule(Seconds(7) + MicroSeconds(44),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // STA PHY STATE should keep CCA_BUSY once the duration estimated from L-SIG has elapsed, i.e.
    // at 152.8us.
    Simulator::Schedule(Seconds(7) + NanoSeconds(152800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);

    // CASE 8: send one packet followed by a second one 3 dB weaker between the end of L-SIG and the
    // start of HE-SIG of the first packet: PHY header reception should succeed.

    Simulator::Schedule(Seconds(8), &TestPhyHeadersReception::SendPacket, this, rxPower, 1000, 7);
    Simulator::Schedule(Seconds(8) + MicroSeconds(25),
                        &TestPhyHeadersReception::SendPacket,
                        this,
                        rxPower - dB_u{3},
                        1000,
                        7);
    // At 10 us, STA PHY STATE should be CCA_BUSY.
    Simulator::Schedule(Seconds(8) + MicroSeconds(10),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 24us (end of L-SIG), STA PHY STATE should be unchanged because L-SIG reception should have
    // succeeded.
    Simulator::Schedule(Seconds(8) + MicroSeconds(24),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    // At 44 us (end of HE-SIG), STA PHY STATE should move to RX since the PHY header reception
    // should have succeeded.
    Simulator::Schedule(Seconds(8) + NanoSeconds(43999),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);
    Simulator::Schedule(Seconds(8) + NanoSeconds(44000),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    // STA PHY STATE should move back to CCA_BUSY once the duration estimated from L-SIG has
    // elapsed, i.e. at 152.8us.
    Simulator::Schedule(Seconds(8) + NanoSeconds(152799),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::RX);
    Simulator::Schedule(Seconds(8) + NanoSeconds(152800),
                        &TestPhyHeadersReception::CheckPhyState,
                        this,
                        WifiPhyState::CCA_BUSY);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief A-MPDU reception test
 */
class TestAmpduReception : public WifiPhyReceptionTest
{
  public:
    TestAmpduReception();

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * RX success function
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);
    /**
     * RX failure function
     * @param psdu the PSDU
     */
    void RxFailure(Ptr<const WifiPsdu> psdu);
    /**
     * RX dropped function
     * @param p the packet
     * @param reason the reason
     */
    void RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason);
    /**
     * Increment reception success bitmap.
     * @param size the size of the received packet
     */
    void IncrementSuccessBitmap(uint32_t size);
    /**
     * Increment reception failure bitmap.
     * @param size the size of the received packet
     */
    void IncrementFailureBitmap(uint32_t size);

    /**
     * Reset bitmaps function
     */
    void ResetBitmaps();

    /**
     * Send A-MPDU with 3 MPDUs of different size (i-th MSDU will have 100 bytes more than
     * (i-1)-th).
     * @param rxPower the transmit power
     * @param referencePacketSize the reference size of the packets in bytes (i-th MSDU will have
     * 100 bytes more than (i-1)-th)
     */
    void SendAmpduWithThreeMpdus(dBm_u rxPower, uint32_t referencePacketSize);

    /**
     * Check the RX success bitmap for A-MPDU 1
     * @param expected the expected bitmap
     */
    void CheckRxSuccessBitmapAmpdu1(uint8_t expected);
    /**
     * Check the RX success bitmap for A-MPDU 2
     * @param expected the expected bitmap
     */
    void CheckRxSuccessBitmapAmpdu2(uint8_t expected);
    /**
     * Check the RX failure bitmap for A-MPDU 1
     * @param expected the expected bitmap
     */
    void CheckRxFailureBitmapAmpdu1(uint8_t expected);
    /**
     * Check the RX failure bitmap for A-MPDU 2
     * @param expected the expected bitmap
     */
    void CheckRxFailureBitmapAmpdu2(uint8_t expected);
    /**
     * Check the RX dropped bitmap for A-MPDU 1
     * @param expected the expected bitmap
     */
    void CheckRxDroppedBitmapAmpdu1(uint8_t expected);
    /**
     * Check the RX dropped bitmap for A-MPDU 2
     * @param expected the expected bitmap
     */
    void CheckRxDroppedBitmapAmpdu2(uint8_t expected);

    /**
     * Check the PHY state
     * @param expectedState the expected PHY state
     */
    void CheckPhyState(WifiPhyState expectedState);

    uint8_t m_rxSuccessBitmapAmpdu1{0}; ///< bitmap of successfully received MPDUs in A-MPDU #1
    uint8_t m_rxSuccessBitmapAmpdu2{0}; ///< bitmap of successfully received MPDUs in A-MPDU #2

    uint8_t m_rxFailureBitmapAmpdu1{0}; ///< bitmap of unsuccessfully received MPDUs in A-MPDU #1
    uint8_t m_rxFailureBitmapAmpdu2{0}; ///< bitmap of unsuccessfully received MPDUs in A-MPDU #2

    uint8_t m_rxDroppedBitmapAmpdu1{0}; ///< bitmap of dropped MPDUs in A-MPDU #1
    uint8_t m_rxDroppedBitmapAmpdu2{0}; ///< bitmap of dropped MPDUs in A-MPDU #2
};

TestAmpduReception::TestAmpduReception()
    : WifiPhyReceptionTest("A-MPDU reception test")
{
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
TestAmpduReception::RxSuccess(Ptr<const WifiPsdu> psdu,
                              RxSignalInfo rxSignalInfo,
                              const WifiTxVector& txVector,
                              const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    if (statusPerMpdu.empty()) // wait for the whole A-MPDU
    {
        return;
    }
    NS_ABORT_MSG_IF(psdu->GetNMpdus() != statusPerMpdu.size(),
                    "Should have one receive status per MPDU");
    auto rxOkForMpdu = statusPerMpdu.begin();
    for (auto mpdu = psdu->begin(); mpdu != psdu->end(); ++mpdu)
    {
        if (*rxOkForMpdu)
        {
            IncrementSuccessBitmap((*mpdu)->GetSize());
        }
        else
        {
            IncrementFailureBitmap((*mpdu)->GetSize());
        }
        ++rxOkForMpdu;
    }
}

void
TestAmpduReception::IncrementSuccessBitmap(uint32_t size)
{
    if (size == 1030) // A-MPDU 1 - MPDU #1
    {
        m_rxSuccessBitmapAmpdu1 |= 1;
    }
    else if (size == 1130) // A-MPDU 1 - MPDU #2
    {
        m_rxSuccessBitmapAmpdu1 |= (1 << 1);
    }
    else if (size == 1230) // A-MPDU 1 - MPDU #3
    {
        m_rxSuccessBitmapAmpdu1 |= (1 << 2);
    }
    else if (size == 1330) // A-MPDU 2 - MPDU #1
    {
        m_rxSuccessBitmapAmpdu2 |= 1;
    }
    else if (size == 1430) // A-MPDU 2 - MPDU #2
    {
        m_rxSuccessBitmapAmpdu2 |= (1 << 1);
    }
    else if (size == 1530) // A-MPDU 2 - MPDU #3
    {
        m_rxSuccessBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    for (auto mpdu = psdu->begin(); mpdu != psdu->end(); ++mpdu)
    {
        IncrementFailureBitmap((*mpdu)->GetSize());
    }
}

void
TestAmpduReception::IncrementFailureBitmap(uint32_t size)
{
    if (size == 1030) // A-MPDU 1 - MPDU #1
    {
        m_rxFailureBitmapAmpdu1 |= 1;
    }
    else if (size == 1130) // A-MPDU 1 - MPDU #2
    {
        m_rxFailureBitmapAmpdu1 |= (1 << 1);
    }
    else if (size == 1230) // A-MPDU 1 - MPDU #3
    {
        m_rxFailureBitmapAmpdu1 |= (1 << 2);
    }
    else if (size == 1330) // A-MPDU 2 - MPDU #1
    {
        m_rxFailureBitmapAmpdu2 |= 1;
    }
    else if (size == 1430) // A-MPDU 2 - MPDU #2
    {
        m_rxFailureBitmapAmpdu2 |= (1 << 1);
    }
    else if (size == 1530) // A-MPDU 2 - MPDU #3
    {
        m_rxFailureBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << p << reason);
    if (p->GetSize() == 1030) // A-MPDU 1 - MPDU #1
    {
        m_rxDroppedBitmapAmpdu1 |= 1;
    }
    else if (p->GetSize() == 1130) // A-MPDU 1 - MPDU #2
    {
        m_rxDroppedBitmapAmpdu1 |= (1 << 1);
    }
    else if (p->GetSize() == 1230) // A-MPDU 1 - MPDU #3
    {
        m_rxDroppedBitmapAmpdu1 |= (1 << 2);
    }
    else if (p->GetSize() == 1330) // A-MPDU 2 - MPDU #1
    {
        m_rxDroppedBitmapAmpdu2 |= 1;
    }
    else if (p->GetSize() == 1430) // A-MPDU 2 - MPDU #2
    {
        m_rxDroppedBitmapAmpdu2 |= (1 << 1);
    }
    else if (p->GetSize() == 1530) // A-MPDU 2 - MPDU #3
    {
        m_rxDroppedBitmapAmpdu2 |= (1 << 2);
    }
}

void
TestAmpduReception::CheckRxSuccessBitmapAmpdu1(uint8_t expected)
{
    NS_TEST_ASSERT_MSG_EQ(m_rxSuccessBitmapAmpdu1,
                          expected,
                          "RX success bitmap for A-MPDU 1 is not as expected");
}

void
TestAmpduReception::CheckRxSuccessBitmapAmpdu2(uint8_t expected)
{
    NS_TEST_ASSERT_MSG_EQ(m_rxSuccessBitmapAmpdu2,
                          expected,
                          "RX success bitmap for A-MPDU 2 is not as expected");
}

void
TestAmpduReception::CheckRxFailureBitmapAmpdu1(uint8_t expected)
{
    NS_TEST_ASSERT_MSG_EQ(m_rxFailureBitmapAmpdu1,
                          expected,
                          "RX failure bitmap for A-MPDU 1 is not as expected");
}

void
TestAmpduReception::CheckRxFailureBitmapAmpdu2(uint8_t expected)
{
    NS_TEST_ASSERT_MSG_EQ(m_rxFailureBitmapAmpdu2,
                          expected,
                          "RX failure bitmap for A-MPDU 2 is not as expected");
}

void
TestAmpduReception::CheckRxDroppedBitmapAmpdu1(uint8_t expected)
{
    NS_TEST_ASSERT_MSG_EQ(m_rxDroppedBitmapAmpdu1,
                          expected,
                          "RX dropped bitmap for A-MPDU 1 is not as expected");
}

void
TestAmpduReception::CheckRxDroppedBitmapAmpdu2(uint8_t expected)
{
    NS_TEST_ASSERT_MSG_EQ(m_rxDroppedBitmapAmpdu2,
                          expected,
                          "RX dropped bitmap for A-MPDU 2 is not as expected");
}

void
TestAmpduReception::CheckPhyState(WifiPhyState expectedState)
{
    WifiPhyState currentState;
    PointerValue ptr;
    m_phy->GetAttribute("State", ptr);
    Ptr<WifiPhyStateHelper> state = DynamicCast<WifiPhyStateHelper>(ptr.Get<WifiPhyStateHelper>());
    currentState = state->GetState();
    NS_TEST_ASSERT_MSG_EQ(currentState,
                          expectedState,
                          "PHY State " << currentState << " does not match expected state "
                                       << expectedState << " at " << Simulator::Now());
}

void
TestAmpduReception::SendAmpduWithThreeMpdus(dBm_u rxPower, uint32_t referencePacketSize)
{
    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         true);

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    std::vector<Ptr<WifiMpdu>> mpduList;
    for (size_t i = 0; i < 3; ++i)
    {
        Ptr<Packet> p = Create<Packet>(referencePacketSize + i * 100);
        mpduList.push_back(Create<WifiMpdu>(p, hdr));
    }
    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(mpduList);

    Time txDuration =
        SpectrumWifiPhy::CalculateTxDuration(psdu->GetSize(), txVector, m_phy->GetPhyBand());

    Ptr<WifiPpdu> ppdu =
        Create<HePpdu>(psdu, txVector, m_phy->GetOperatingChannel(), txDuration, m_uid++);

    Ptr<SpectrumValue> txPowerSpectrum =
        WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(FREQUENCY,
                                                                    CHANNEL_WIDTH,
                                                                    DbmToW(rxPower),
                                                                    GUARD_WIDTH);

    Ptr<WifiSpectrumSignalParameters> txParams = Create<WifiSpectrumSignalParameters>();
    txParams->psd = txPowerSpectrum;
    txParams->txPhy = nullptr;
    txParams->duration = txDuration;
    txParams->ppdu = ppdu;

    m_phy->StartRx(txParams, nullptr);
}

void
TestAmpduReception::DoSetup()
{
    WifiPhyReceptionTest::DoSetup();

    m_phy->SetReceiveOkCallback(MakeCallback(&TestAmpduReception::RxSuccess, this));
    m_phy->SetReceiveErrorCallback(MakeCallback(&TestAmpduReception::RxFailure, this));
    m_phy->TraceConnectWithoutContext("PhyRxDrop",
                                      MakeCallback(&TestAmpduReception::RxDropped, this));

    Ptr<ThresholdPreambleDetectionModel> preambleDetectionModel =
        CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(2));
    m_phy->SetPreambleDetectionModel(preambleDetectionModel);

    Ptr<SimpleFrameCaptureModel> frameCaptureModel = CreateObject<SimpleFrameCaptureModel>();
    frameCaptureModel->SetAttribute("Margin", DoubleValue(5));
    frameCaptureModel->SetAttribute("CaptureWindow", TimeValue(MicroSeconds(16)));
    m_phy->SetFrameCaptureModel(frameCaptureModel);
}

void
TestAmpduReception::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 1;
    dBm_u rxPower{-30};
    m_phy->AssignStreams(streamNumber);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 1: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with
    // power under RX sensitivity. The second A-MPDU is received 2 microseconds after the first
    // A-MPDU (i.e. during preamble detection).
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(1),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(1) + MicroSeconds(2),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been ignored.
    Simulator::Schedule(Seconds(1.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(1.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(1.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been successfully received.
    Simulator::Schedule(Seconds(1.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(1.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(1.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(1.2), &TestAmpduReception::ResetBitmaps, this);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 2: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received
    // with power under RX sensitivity. The second A-MPDU is received 2 microseconds after the first
    // A-MPDU (i.e. during preamble detection).
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(2),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(2) + MicroSeconds(2),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1300);

    // All MPDUs of A-MPDU 1 should have been received.
    Simulator::Schedule(Seconds(2.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(2.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(2.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been ignored.
    Simulator::Schedule(Seconds(2.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(2.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(2.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(2.2), &TestAmpduReception::ResetBitmaps, this);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 3: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with
    // power under RX sensitivity. The second A-MPDU is received 10 microseconds after the first
    // A-MPDU (i.e. during the frame capture window).
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(3),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(3) + MicroSeconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been ignored.
    Simulator::Schedule(Seconds(3.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(3.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(3.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been successfully received.
    Simulator::Schedule(Seconds(3.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(3.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(3.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(3.2), &TestAmpduReception::ResetBitmaps, this);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 4: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received
    // with power under RX sensitivity. The second A-MPDU is received 10 microseconds after the
    // first A-MPDU (i.e. during the frame capture window).
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(4),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(4) + MicroSeconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1300);

    // All MPDUs of A-MPDU 1 should have been received.
    Simulator::Schedule(Seconds(4.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(4.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(4.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been ignored.
    Simulator::Schedule(Seconds(4.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(4.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(4.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(4.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 5: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with
    // power under RX sensitivity. The second A-MPDU is received 100 microseconds after the first
    // A-MPDU (i.e. after the frame capture window, during the payload of MPDU #1).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(5),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(5) + MicroSeconds(100),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been ignored.
    Simulator::Schedule(Seconds(5.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(5.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(5.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been successfully received.
    Simulator::Schedule(Seconds(5.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(5.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(5.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(5.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 6: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received
    // with power under RX sensitivity. The second A-MPDU is received 100 microseconds after the
    // first A-MPDU (i.e. after the frame capture window, during the payload of MPDU #1).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(6),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(6) + MicroSeconds(100),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1300);

    // All MPDUs of A-MPDU 1 should have been received.
    Simulator::Schedule(Seconds(6.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(6.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(6.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been ignored.
    Simulator::Schedule(Seconds(6.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(6.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(6.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(6.2), &TestAmpduReception::ResetBitmaps, this);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 7: receive two A-MPDUs (containing each 3 MPDUs) where the first A-MPDU is received with
    // power under RX sensitivity. The second A-MPDU is received during the payload of MPDU #2.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(7),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(7) + NanoSeconds(1100000),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been ignored.
    Simulator::Schedule(Seconds(7.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(7.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(7.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been successfully received.
    Simulator::Schedule(Seconds(7.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(7.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(7.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(7.2), &TestAmpduReception::ResetBitmaps, this);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 8: receive two A-MPDUs (containing each 3 MPDUs) where the second A-MPDU is received
    // with power under RX sensitivity. The second A-MPDU is received during the payload of MPDU #2.
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(8),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(8) + NanoSeconds(1100000),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower - dB_u{100},
                        1300);

    // All MPDUs of A-MPDU 1 should have been received.
    Simulator::Schedule(Seconds(8.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(8.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(8.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been ignored.
    Simulator::Schedule(Seconds(8.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(8.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(8.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(8.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 9: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power 3
    // dB higher. The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during
    // preamble detection).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(9),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(9) + MicroSeconds(2),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{3.0},
                        1300);

    // All MPDUs of A-MPDU 1 should have been dropped.
    Simulator::Schedule(Seconds(9.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(9.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(9.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000111);

    // All MPDUs of A-MPDU 2 should have been received with errors.
    Simulator::Schedule(Seconds(9.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(9.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(9.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(9.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 10: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
    // The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during preamble
    // detection).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(10) + MicroSeconds(2),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been dropped (preamble detection failed).
    Simulator::Schedule(Seconds(10.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(10.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(10.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000111);

    // All MPDUs of A-MPDU 2 should have been dropped as well.
    Simulator::Schedule(Seconds(10.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(10.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(10.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(10.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 11: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 3
    // dB higher. The second A-MPDU is received 2 microseconds after the first A-MPDU (i.e. during
    // preamble detection).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(11),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{3.0},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(11) + MicroSeconds(2),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors.
    Simulator::Schedule(Seconds(11.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(11.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(11.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been dropped.
    Simulator::Schedule(Seconds(11.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(11.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(11.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(11.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 12: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power
    // 3 dB higher. The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e.
    // during the frame capture window).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(12),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(12) + MicroSeconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{3.0},
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors (PHY header reception failed and
    // thus incorrect decoding of payload).
    Simulator::Schedule(Seconds(12.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(12.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(12.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000111);

    // All MPDUs of A-MPDU 2 should have been dropped (even though TX power is higher, it is not
    // high enough to get the PHY reception switched)
    Simulator::Schedule(Seconds(12.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(12.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(12.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(12.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 13: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
    // The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during the frame
    // capture window).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(13),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(13) + MicroSeconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors (PHY header reception failed and
    // thus incorrect decoding of payload).
    Simulator::Schedule(Seconds(13.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(13.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(13.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000111);

    // All MPDUs of A-MPDU 2 should have been dropped as well.
    Simulator::Schedule(Seconds(13.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(13.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(13.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(13.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 14: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 3
    // dB higher. The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during
    // the frame capture window).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(14),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{3.0},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(14) + MicroSeconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors.
    Simulator::Schedule(Seconds(14.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(14.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(14.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been dropped.
    Simulator::Schedule(Seconds(14.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(14.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(14.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(14.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 15: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power
    // 6 dB higher. The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e.
    // during the frame capture window).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(15),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(15) + MicroSeconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{6.0},
                        1300);

    // All MPDUs of A-MPDU 1 should have been dropped because PHY reception switched to A-MPDU 2.
    Simulator::Schedule(Seconds(15.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(15.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(15.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000111);

    // All MPDUs of A-MPDU 2 should have been successfully received
    Simulator::Schedule(Seconds(15.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(15.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(15.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000000);

    Simulator::Schedule(Seconds(15.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 16: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 6
    // dB higher. The second A-MPDU is received 10 microseconds after the first A-MPDU (i.e. during
    // the frame capture window).
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(16),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{6.0},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(16) + MicroSeconds(10),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been successfully received.
    Simulator::Schedule(Seconds(16.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(16.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(16.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been dropped.
    Simulator::Schedule(Seconds(16.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(16.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(16.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(16.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 17: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power
    // 6 dB higher. The second A-MPDU is received 25 microseconds after the first A-MPDU (i.e. after
    // the frame capture window, but still during PHY header).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(17),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(17) + MicroSeconds(25),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{6.0},
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors.
    Simulator::Schedule(Seconds(17.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(17.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(17.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000111);

    // All MPDUs of A-MPDU 2 should have been dropped (no reception switch, MPDUs dropped because
    // PHY is already in RX state).
    Simulator::Schedule(Seconds(17.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(17.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(17.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(17.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 18: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 6
    // dB higher. The second A-MPDU is received 25 microseconds after the first A-MPDU (i.e. after
    // the frame capture window, but still during PHY header).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(18),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{6.0},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(18) + MicroSeconds(25),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been successfully received.
    Simulator::Schedule(Seconds(18.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(18.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(18.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been dropped.
    Simulator::Schedule(Seconds(18.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(18.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(18.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(18.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 19: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
    // The second A-MPDU is received 25 microseconds after the first A-MPDU (i.e. after the frame
    // capture window, but still during PHY header).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(19),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(19) + MicroSeconds(25),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors.
    Simulator::Schedule(Seconds(19.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(19.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(19.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000111);

    // All MPDUs of A-MPDU 2 should have been dropped.
    Simulator::Schedule(Seconds(19.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(19.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(19.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(19.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 20: receive two A-MPDUs (containing each 3 MPDUs) with the second A-MPDU having a power
    // 6 dB higher. The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e.
    // during the payload of MPDU #1).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(20),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(20) + MicroSeconds(100),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{6.0},
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors.
    Simulator::Schedule(Seconds(20.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(20.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(20.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been dropped (no reception switch, MPDUs dropped because
    // PHY is already in RX state).
    Simulator::Schedule(Seconds(20.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(20.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(20.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(20.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 21: receive two A-MPDUs (containing each 3 MPDUs) with the first A-MPDU having a power 6
    // dB higher. The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. during
    // the payload of MPDU #1).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(21),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower + dB_u{6.0},
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(21) + MicroSeconds(100),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been successfully received.
    Simulator::Schedule(Seconds(21.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(21.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(21.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been dropped.
    Simulator::Schedule(Seconds(21.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(21.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(21.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(21.2), &TestAmpduReception::ResetBitmaps, this);

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CASE 22: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
    // The second A-MPDU is received 100 microseconds after the first A-MPDU (i.e. during the
    // payload of MPDU #1).
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(22),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(22) + MicroSeconds(100),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // All MPDUs of A-MPDU 1 should have been received with errors.
    Simulator::Schedule(Seconds(22.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(22.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000111);
    Simulator::Schedule(Seconds(22.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // All MPDUs of A-MPDU 2 should have been dropped.
    Simulator::Schedule(Seconds(22.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(22.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(22.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(22.2), &TestAmpduReception::ResetBitmaps, this);

    ///////////////////////////////////////////////////////////////////////////////
    // CASE 23: receive two A-MPDUs (containing each 3 MPDUs) with the same power.
    // The second A-MPDU is received during the payload of MPDU #2.
    ///////////////////////////////////////////////////////////////////////////////

    // A-MPDU 1
    Simulator::Schedule(Seconds(23),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1000);

    // A-MPDU 2
    Simulator::Schedule(Seconds(23) + NanoSeconds(1100000),
                        &TestAmpduReception::SendAmpduWithThreeMpdus,
                        this,
                        rxPower,
                        1300);

    // The first MPDU of A-MPDU 1 should have been successfully received (no interference).
    // The two other MPDUs failed due to interference and are marked as failure (and dropped).
    Simulator::Schedule(Seconds(23.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu1,
                        this,
                        0b00000001);
    Simulator::Schedule(Seconds(23.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu1,
                        this,
                        0b00000110);
    Simulator::Schedule(Seconds(23.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu1,
                        this,
                        0b00000000);

    // The two first MPDUs of A-MPDU 2 are dropped because PHY is already in RX state (receiving
    // A-MPDU 1). The last MPDU of A-MPDU 2 is interference free (A-MPDU 1 transmission is finished)
    // but is dropped because its PHY preamble and header were not received.
    Simulator::Schedule(Seconds(23.1),
                        &TestAmpduReception::CheckRxSuccessBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(23.1),
                        &TestAmpduReception::CheckRxFailureBitmapAmpdu2,
                        this,
                        0b00000000);
    Simulator::Schedule(Seconds(23.1),
                        &TestAmpduReception::CheckRxDroppedBitmapAmpdu2,
                        this,
                        0b00000111);

    Simulator::Schedule(Seconds(23.2), &TestAmpduReception::ResetBitmaps, this);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Unsupported Modulation Reception Test
 * This test creates a mixed network, in which an HE STA and a VHT
 * STA are associated to an HE AP and send uplink traffic. In the
 * simulated deployment the VHT STA's backoff will expire while the
 * HE STA is sending a packet, and the VHT STA will access the
 * channel anyway. This happens because the HE STA is using an HeMcs
 * that the VHT STA is not able to demodulate: the VHT STA will
 * correctly stop listening to the HE packet, but it will not update
 * its InterferenceHelper with the HE packet. Later on, this leads to
 * the STA wrongly assuming the medium is available when its back-off
 * expires in the middle of the HE packet. We detect that this is
 * happening by looking at the reason why the AP is failing to decode
 * the preamble from the VHT STA's transmission: if the reason is
 * that it's in RX already, the test fails. The test is based on
 * wifi-txop-test.cc.
 */
class TestUnsupportedModulationReception : public TestCase
{
  public:
    TestUnsupportedModulationReception();
    ~TestUnsupportedModulationReception() override = default;

  private:
    void DoRun() override;

    /**
     * Callback invoked when PHY drops an incoming packet
     * @param context the context
     * @param packet the packet that was dropped
     * @param reason the reason the packet was dropped
     */
    void Dropped(std::string context, Ptr<const Packet> packet, WifiPhyRxfailureReason reason);
    /**
     * Check correctness of transmitted frames
     */
    void CheckResults();

    uint16_t m_dropped{0}; ///< number of packets dropped by the AP because it was already receiving
};

TestUnsupportedModulationReception::TestUnsupportedModulationReception()
    : TestCase("Check correct behavior when a STA is receiving a transmission using an unsupported "
               "modulation")
{
}

void
TestUnsupportedModulationReception::Dropped(std::string context,
                                            Ptr<const Packet> packet,
                                            WifiPhyRxfailureReason reason)
{
    // Print if the test is executed through test-runner
    if (reason == RXING)
    {
        std::cout << "Dropped a packet because already receiving" << std::endl;
        m_dropped++;
    }
}

void
TestUnsupportedModulationReception::DoRun()
{
    uint16_t m_nStations = 2;        ///< number of stations
    NetDeviceContainer m_staDevices; ///< container for stations' NetDevices
    NetDeviceContainer m_apDevices;  ///< container for AP's NetDevice

    int64_t streamNumber = 100;

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(m_nStations);

    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    SpectrumWifiPhyHelper phy;
    phy.SetChannel(spectrumChannel);

    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue(65535));

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac",
                "QosSupported",
                BooleanValue(true),
                "Ssid",
                SsidValue(Ssid("non-existent-ssid")));

    wifi.SetStandard(WIFI_STANDARD_80211ax);
    m_staDevices.Add(wifi.Install(phy, mac, wifiStaNodes.Get(0)));
    wifi.SetStandard(WIFI_STANDARD_80211ac);
    m_staDevices.Add(wifi.Install(phy, mac, wifiStaNodes.Get(1)));

    wifi.SetStandard(WIFI_STANDARD_80211ax);
    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(true),
                "Ssid",
                SsidValue(Ssid("wifi-backoff-ssid")),
                "BeaconInterval",
                TimeValue(MicroSeconds(102400)),
                "EnableBeaconJitter",
                BooleanValue(false));

    m_apDevices = wifi.Install(phy, mac, wifiApNode);

    // schedule association requests at different times
    Time init = MilliSeconds(100);
    Ptr<WifiNetDevice> dev;

    for (uint16_t i = 0; i < m_nStations; i++)
    {
        dev = DynamicCast<WifiNetDevice>(m_staDevices.Get(i));
        Simulator::Schedule(init + i * MicroSeconds(102400),
                            &WifiMac::SetSsid,
                            dev->GetMac(),
                            Ssid("wifi-backoff-ssid"));
    }

    // Assign fixed streams to random variables in use
    WifiHelper::AssignStreams(m_apDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 1.0, 0.0));
    positionAlloc->Add(Vector(-1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    // set the TXOP limit on BE AC
    dev = DynamicCast<WifiNetDevice>(m_apDevices.Get(0));
    PointerValue ptr;
    dev->GetMac()->GetAttribute("BE_Txop", ptr);

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // UL Traffic
    for (uint16_t i = 0; i < m_nStations; i++)
    {
        PacketSocketAddress socket;
        socket.SetSingleDevice(m_staDevices.Get(0)->GetIfIndex());
        socket.SetPhysicalAddress(m_apDevices.Get(0)->GetAddress());
        socket.SetProtocol(1);
        Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
        client->SetAttribute("PacketSize", UintegerValue(1500));
        client->SetAttribute("MaxPackets", UintegerValue(200));
        client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        client->SetRemote(socket);
        wifiStaNodes.Get(i)->AddApplication(client);
        client->SetStartTime(MicroSeconds(400000));
        client->SetStopTime(Seconds(1));
        Ptr<PacketSocketClient> legacyStaClient = CreateObject<PacketSocketClient>();
        legacyStaClient->SetAttribute("PacketSize", UintegerValue(1500));
        legacyStaClient->SetAttribute("MaxPackets", UintegerValue(200));
        legacyStaClient->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        legacyStaClient->SetRemote(socket);
        wifiStaNodes.Get(i)->AddApplication(legacyStaClient);
        legacyStaClient->SetStartTime(MicroSeconds(400000));
        legacyStaClient->SetStopTime(Seconds(1));
        Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
        server->SetLocal(socket);
        wifiApNode.Get(0)->AddApplication(server);
        server->SetStartTime(Seconds(0));
        server->SetStopTime(Seconds(1));
    }

    // Trace dropped packets
    Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop",
                    MakeCallback(&TestUnsupportedModulationReception::Dropped, this));

    Simulator::Stop(Seconds(1));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
TestUnsupportedModulationReception::CheckResults()
{
    NS_TEST_EXPECT_MSG_EQ(m_dropped, 0, "Dropped some packets unexpectedly");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Unsupported Bandwidth Reception Test
 * This test checks whether a PHY receiving a PPDU sent over a channel width
 * larger than the one supported by the PHY is getting dropped at the expected
 * time. The expected time corresponds to the moment the PHY header indicating
 * the channel width used to transmit the PPDU is received. Since we are considering
 * 802.11ax for this test, this corresponds to the time HE-SIG-A is received.
 */
class TestUnsupportedBandwidthReception : public TestCase
{
  public:
    TestUnsupportedBandwidthReception();

  private:
    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     * Function to create a PPDU
     *
     * @param centerFreq the center frequency used for the transmission of the PPDU
     * @param bandwidth the bandwidth used for the transmission of the PPDU
     */
    void SendPpdu(MHz_u centerFreq, MHz_u bandwidth);

    /**
     * Function called upon a PSDU received successfully
     * @param psdu the PSDU
     * @param rxSignalInfo the info on the received signal (\see RxSignalInfo)
     * @param txVector the transmit vector
     * @param statusPerMpdu reception status per MPDU
     */
    void RxSuccess(Ptr<const WifiPsdu> psdu,
                   RxSignalInfo rxSignalInfo,
                   const WifiTxVector& txVector,
                   const std::vector<bool>& statusPerMpdu);

    /**
     * Function called upon a PSDU received unsuccessfuly
     * @param psdu the PSDU
     */
    void RxFailure(Ptr<const WifiPsdu> psdu);

    /**
     * Function called upon a PSDU dropped by the PHY
     * @param packet the packet that was dropped
     * @param reason the reason the packet was dropped
     */
    void RxDropped(Ptr<const Packet> packet, WifiPhyRxfailureReason reason);

    /**
     * Check the reception results
     * @param expectedCountRxSuccess the expected number of RX success
     * @param expectedCountRxFailure the expected number of RX failure
     * @param expectedCountRxDropped the expected number of RX drop
     * @param expectedLastRxSucceeded the expected time the last RX success occurred or std::nullopt
     * if the expected number of RX success is not strictly positive \param expectedLastRxFailed the
     * expected time the last RX failure occurred or std::nullopt if the expected number of RX
     * failure is not strictly positive \param expectedLastRxDropped the expected time the last RX
     * drop occurred or std::nullopt if the expected number of RX drop is not strictly positive
     */
    void CheckRx(uint32_t expectedCountRxSuccess,
                 uint32_t expectedCountRxFailure,
                 uint32_t expectedCountRxDropped,
                 std::optional<Time> expectedLastRxSucceeded,
                 std::optional<Time> expectedLastRxFailed,
                 std::optional<Time> expectedLastRxDropped);

    uint32_t m_countRxSuccess; ///< count RX success
    uint32_t m_countRxFailure; ///< count RX failure
    uint32_t m_countRxDropped; ///< count RX drop

    std::optional<Time> m_lastRxSucceeded; ///< time of last RX success, if any
    std::optional<Time> m_lastRxFailed;    ///< time of last RX failure, if any
    std::optional<Time> m_lastRxDropped;   ///< time of last RX drop, if any

    Ptr<SpectrumWifiPhy> m_rxPhy; ///< RX PHY
    Ptr<SpectrumWifiPhy> m_txPhy; ///< TX PHY
};

TestUnsupportedBandwidthReception::TestUnsupportedBandwidthReception()
    : TestCase("Check correct behavior when a STA is receiving a transmission using an unsupported "
               "bandwidth"),
      m_countRxSuccess(0),
      m_countRxFailure(0),
      m_countRxDropped(0),
      m_lastRxSucceeded(std::nullopt),
      m_lastRxFailed(std::nullopt),
      m_lastRxDropped(std::nullopt)
{
}

void
TestUnsupportedBandwidthReception::SendPpdu(MHz_u centerFreq, MHz_u bandwidth)
{
    auto txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                 0,
                                 WIFI_PREAMBLE_HE_SU,
                                 NanoSeconds(800),
                                 1,
                                 1,
                                 0,
                                 bandwidth,
                                 false);

    auto pkt = Create<Packet>(1000);
    WifiMacHeader hdr;

    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
    Time txDuration =
        SpectrumWifiPhy::CalculateTxDuration(psdu->GetSize(), txVector, m_rxPhy->GetPhyBand());

    auto ppdu = Create<HePpdu>(psdu, txVector, m_txPhy->GetOperatingChannel(), txDuration, 0);

    auto txPowerSpectrum =
        WifiSpectrumValueHelper::CreateHeOfdmTxPowerSpectralDensity(centerFreq,
                                                                    bandwidth,
                                                                    DbmToW(dBm_u{-50}),
                                                                    bandwidth);

    auto txParams = Create<WifiSpectrumSignalParameters>();
    txParams->psd = txPowerSpectrum;
    txParams->txPhy = nullptr;
    txParams->duration = txDuration;
    txParams->ppdu = ppdu;

    m_rxPhy->StartRx(txParams, nullptr);
}

void
TestUnsupportedBandwidthReception::RxSuccess(Ptr<const WifiPsdu> psdu,
                                             RxSignalInfo rxSignalInfo,
                                             const WifiTxVector& txVector,
                                             const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    m_countRxSuccess++;
    m_lastRxSucceeded = Simulator::Now();
}

void
TestUnsupportedBandwidthReception::RxFailure(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << *psdu);
    m_countRxFailure++;
    m_lastRxFailed = Simulator::Now();
}

void
TestUnsupportedBandwidthReception::RxDropped(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << p << reason);
    NS_ASSERT(reason == UNSUPPORTED_SETTINGS);
    m_countRxDropped++;
    m_lastRxDropped = Simulator::Now();
}

void
TestUnsupportedBandwidthReception::CheckRx(uint32_t expectedCountRxSuccess,
                                           uint32_t expectedCountRxFailure,
                                           uint32_t expectedCountRxDropped,
                                           std::optional<Time> expectedLastRxSucceeded,
                                           std::optional<Time> expectedLastRxFailed,
                                           std::optional<Time> expectedLastRxDropped)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxSuccess,
                          expectedCountRxSuccess,
                          "Didn't receive right number of successful packets");

    NS_TEST_ASSERT_MSG_EQ(m_countRxFailure,
                          expectedCountRxFailure,
                          "Didn't receive right number of unsuccessful packets");

    NS_TEST_ASSERT_MSG_EQ(m_countRxDropped,
                          expectedCountRxDropped,
                          "Didn't receive right number of dropped packets");

    if (expectedCountRxSuccess > 0)
    {
        NS_ASSERT(m_lastRxSucceeded.has_value());
        NS_ASSERT(expectedLastRxSucceeded.has_value());
        NS_TEST_ASSERT_MSG_EQ(m_lastRxSucceeded.value(),
                              expectedLastRxSucceeded.value(),
                              "Didn't receive the last successful packet at the expected time");
    }

    if (expectedCountRxFailure > 0)
    {
        NS_ASSERT(m_lastRxFailed.has_value());
        NS_ASSERT(expectedLastRxFailed.has_value());
        NS_TEST_ASSERT_MSG_EQ(m_lastRxFailed.value(),
                              expectedLastRxFailed.value(),
                              "Didn't receive the last unsuccessful packet at the expected time");
    }

    if (expectedCountRxDropped > 0)
    {
        NS_ASSERT(m_lastRxDropped.has_value());
        NS_ASSERT(expectedLastRxDropped.has_value());
        NS_TEST_ASSERT_MSG_EQ(m_lastRxDropped.value(),
                              expectedLastRxDropped.value(),
                              "Didn't drop the last filtered packet at the expected time");
    }
}

void
TestUnsupportedBandwidthReception::DoSetup()
{
    Ptr<MultiModelSpectrumChannel> spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<Node> node = CreateObject<Node>();
    Ptr<WifiNetDevice> dev = CreateObject<WifiNetDevice>();
    m_rxPhy = CreateObject<SpectrumWifiPhy>();
    auto rxInterferenceHelper = CreateObject<InterferenceHelper>();
    m_rxPhy->SetInterferenceHelper(rxInterferenceHelper);
    auto rxErrorRateModel = CreateObject<NistErrorRateModel>();
    m_rxPhy->SetErrorRateModel(rxErrorRateModel);
    m_rxPhy->SetDevice(dev);
    m_rxPhy->AddChannel(spectrumChannel);
    m_rxPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
    dev->SetPhy(m_rxPhy);
    node->AddDevice(dev);

    m_rxPhy->SetReceiveOkCallback(
        MakeCallback(&TestUnsupportedBandwidthReception::RxSuccess, this));
    m_rxPhy->SetReceiveErrorCallback(
        MakeCallback(&TestUnsupportedBandwidthReception::RxFailure, this));
    m_rxPhy->TraceConnectWithoutContext(
        "PhyRxDrop",
        MakeCallback(&TestUnsupportedBandwidthReception::RxDropped, this));

    m_txPhy = CreateObject<SpectrumWifiPhy>();
    auto txInterferenceHelper = CreateObject<InterferenceHelper>();
    m_txPhy->SetInterferenceHelper(txInterferenceHelper);
    auto txErrorRateModel = CreateObject<NistErrorRateModel>();
    m_txPhy->SetErrorRateModel(txErrorRateModel);
    m_txPhy->AddChannel(spectrumChannel);
    m_txPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
}

void
TestUnsupportedBandwidthReception::DoTeardown()
{
    m_rxPhy->Dispose();
    m_rxPhy = nullptr;

    m_txPhy->Dispose();
    m_txPhy = nullptr;
}

void
TestUnsupportedBandwidthReception::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    int64_t streamNumber = 0;
    m_rxPhy->AssignStreams(streamNumber);

    // Case 1: the PHY is operating on channel 36 (20 MHz) and receives a 40 MHz PPDU (channel 38).
    // The PPDU should be dropped once HE-SIG-A is successfully received, since it contains
    // indication about the BW used for the transmission and the PHY shall detect it is larger than
    // its operating BW.
    m_txPhy->SetOperatingChannel(WifiPhy::ChannelTuple{38, 40, WIFI_PHY_BAND_5GHZ, 0});
    m_rxPhy->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});

    Simulator::Schedule(Seconds(1),
                        &TestUnsupportedBandwidthReception::SendPpdu,
                        this,
                        MHz_u{5190},
                        MHz_u{40});

    auto heSigAExpectedRxTime = Seconds(1) + MicroSeconds(32);
    Simulator::Schedule(Seconds(1.5),
                        &TestUnsupportedBandwidthReception::CheckRx,
                        this,
                        0,
                        0,
                        1,
                        std::nullopt,
                        std::nullopt,
                        heSigAExpectedRxTime);

    // TODO: this test can be extended with other scenarios

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Primary 20 MHz Covered By Ppdu Test
 * This test checks whether the functions WifiPpdu::DoesOverlapChannel and
 * WifiPpdu::DoesCoverChannel are returning the expected results.
 */
class TestPrimary20CoveredByPpdu : public TestCase
{
  public:
    TestPrimary20CoveredByPpdu();

  private:
    void DoSetup() override;
    void DoRun() override;
    void DoTeardown() override;

    /**
     * Function to create a PPDU
     *
     * @param ppduCenterFreq the center frequency used for the transmission of the PPDU
     * @return the created PPDU
     */
    Ptr<HePpdu> CreatePpdu(MHz_u ppduCenterFreq);

    /**
     * Run one function
     *
     * @param band the PHY band to use
     * @param phyCenterFreq the operating center frequency of the PHY
     * @param p20Index the primary20 index
     * @param ppduCenterFreq the center frequency used for the transmission of the PPDU
     * @param expectedP20Overlap flag whether the primary 20 MHz channel is expected to be fully
     * covered by the bandwidth of the incoming PPDU
     * @param expectedP20Covered flag whether the
     * primary 20 MHz channel is expected to overlap with the bandwidth of the incoming PPDU
     */
    void RunOne(WifiPhyBand band,
                MHz_u phyCenterFreq,
                uint8_t p20Index,
                MHz_u ppduCenterFreq,
                bool expectedP20Overlap,
                bool expectedP20Covered);

    Ptr<SpectrumWifiPhy> m_rxPhy; ///< RX PHY
    Ptr<SpectrumWifiPhy> m_txPhy; ///< TX PHY
};

TestPrimary20CoveredByPpdu::TestPrimary20CoveredByPpdu()
    : TestCase("Check correct detection of whether P20 is fully covered (hence it can be received) "
               "or overlaps with the bandwidth of an incoming PPDU")
{
}

Ptr<HePpdu>
TestPrimary20CoveredByPpdu::CreatePpdu(MHz_u ppduCenterFreq)
{
    const auto& channelInfo = (*WifiPhyOperatingChannel::FindFirst(0,
                                                                   ppduCenterFreq,
                                                                   MHz_u{0},
                                                                   WIFI_STANDARD_80211ax,
                                                                   m_rxPhy->GetPhyBand()));
    m_txPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelInfo.number, channelInfo.width, channelInfo.band, 0});
    auto txVector = WifiTxVector(HePhy::GetHeMcs7(),
                                 0,
                                 WIFI_PREAMBLE_HE_SU,
                                 NanoSeconds(800),
                                 1,
                                 1,
                                 0,
                                 channelInfo.width,
                                 false);

    auto pkt = Create<Packet>(1000);
    WifiMacHeader hdr(WIFI_MAC_QOSDATA);

    auto psdu = Create<WifiPsdu>(pkt, hdr);
    auto txDuration =
        SpectrumWifiPhy::CalculateTxDuration(psdu->GetSize(), txVector, channelInfo.band);

    return Create<HePpdu>(psdu, txVector, m_txPhy->GetOperatingChannel(), txDuration, 0);
}

void
TestPrimary20CoveredByPpdu::DoSetup()
{
    m_rxPhy = CreateObject<SpectrumWifiPhy>();
    auto rxInterferenceHelper = CreateObject<InterferenceHelper>();
    m_rxPhy->SetInterferenceHelper(rxInterferenceHelper);
    auto rxErrorRateModel = CreateObject<NistErrorRateModel>();
    m_rxPhy->SetErrorRateModel(rxErrorRateModel);
    m_rxPhy->AddChannel(CreateObject<MultiModelSpectrumChannel>());
    m_rxPhy->ConfigureStandard(WIFI_STANDARD_80211ax);

    m_txPhy = CreateObject<SpectrumWifiPhy>();
    auto txInterferenceHelper = CreateObject<InterferenceHelper>();
    m_txPhy->SetInterferenceHelper(txInterferenceHelper);
    auto txErrorRateModel = CreateObject<NistErrorRateModel>();
    m_txPhy->SetErrorRateModel(txErrorRateModel);
    m_txPhy->AddChannel(CreateObject<MultiModelSpectrumChannel>());
    m_txPhy->ConfigureStandard(WIFI_STANDARD_80211ax);
}

void
TestPrimary20CoveredByPpdu::DoTeardown()
{
    m_rxPhy->Dispose();
    m_rxPhy = nullptr;
    m_txPhy->Dispose();
    m_txPhy = nullptr;
}

void
TestPrimary20CoveredByPpdu::RunOne(WifiPhyBand band,
                                   MHz_u phyCenterFreq,
                                   uint8_t p20Index,
                                   MHz_u ppduCenterFreq,
                                   bool expectedP20Overlap,
                                   bool expectedP20Covered)
{
    const auto& channelInfo = (*WifiPhyOperatingChannel::FindFirst(0,
                                                                   phyCenterFreq,
                                                                   MHz_u{0},
                                                                   WIFI_STANDARD_80211ax,
                                                                   band));

    m_rxPhy->SetOperatingChannel(
        WifiPhy::ChannelTuple{channelInfo.number, channelInfo.width, channelInfo.band, p20Index});
    auto p20CenterFreq = m_rxPhy->GetOperatingChannel().GetPrimaryChannelCenterFrequency(MHz_u{20});
    auto p20MinFreq = p20CenterFreq - MHz_u{10};
    auto p20MaxFreq = p20CenterFreq + MHz_u{10};

    auto ppdu = CreatePpdu(ppduCenterFreq);

    auto p20Overlap = ppdu->DoesOverlapChannel(p20MinFreq, p20MaxFreq);
    NS_TEST_ASSERT_MSG_EQ(p20Overlap,
                          expectedP20Overlap,
                          "PPDU is " << (expectedP20Overlap ? "expected" : "not expected")
                                     << " to overlap with the P20");

    auto p20Covered = m_rxPhy->GetPhyEntity(WIFI_STANDARD_80211ax)
                          ->CanStartRx(ppdu); // CanStartRx returns true is the P20 is fully covered
    NS_TEST_ASSERT_MSG_EQ(p20Covered,
                          expectedP20Covered,
                          "PPDU is " << (expectedP20Covered ? "expected" : "not expected")
                                     << " to cover the whole P20");
}

void
TestPrimary20CoveredByPpdu::DoRun()
{
    /*
     * Receiver PHY Operating Channel: 2.4 GHz Channel 4 (2417 MHz – 2437 MHz)
     * Transmitted 20 MHz PPDU: 2.4 GHz Channel 4 (2417 MHz – 2437 MHz)
     * Overlap with primary 20 MHz: yes
     * Primary 20 MHz fully covered: yes
     */
    RunOne(WIFI_PHY_BAND_2_4GHZ, MHz_u{2427}, 0, MHz_u{2427}, true, true);

    /*
     * Receiver PHY Operating Channel: 2.4 GHz Channel 4 (2417 MHz – 2437 MHz)
     * Transmitted 20 MHz PPDU: 2.4 GHz Channel 6 (2427 MHz – 2447 MHz)
     * Overlap with primary 20 MHz: yes
     * Primary 20 MHz fully covered: no
     */
    RunOne(WIFI_PHY_BAND_2_4GHZ, MHz_u{2427}, 0, MHz_u{2437}, true, false);

    /*
     * Receiver PHY Operating Channel: 5 GHz Channel 36 (5170 MHz – 5190 MHz)
     * Transmitted 40 MHz PPDU: 5 GHz Channel 38 (5170 MHz – 5210 MHz)
     * Overlap with primary 20 MHz: yes
     * Primary 20 MHz fully covered: yes
     */
    RunOne(WIFI_PHY_BAND_5GHZ, MHz_u{5180}, 0, MHz_u{5190}, true, true);

    /*
     * Receiver PHY Operating Channel: 5 GHz Channel 36 (5170 MHz–5190 MHz)
     * Transmitted 20 MHz PPDU: 5 GHz Channel 40 (5190 MHz – 5210 MHz)
     * Overlap with primary 20 MHz: no
     * Primary 20 MHz fully covered: no
     */
    RunOne(WIFI_PHY_BAND_5GHZ, MHz_u{5180}, 0, MHz_u{5200}, false, false);

    /*
     * Receiver PHY Operating Channel: 5 GHz Channel 38 (5170 MHz – 5210 MHz) with P20 index 0
     * Transmitted 20 MHz PPDU: 5 GHz Channel 36 (5170 MHz – 5190 MHz)
     * Overlap with primary 20 MHz: yes
     * Primary 20 MHz fully covered: yes
     */
    RunOne(WIFI_PHY_BAND_5GHZ, MHz_u{5190}, 0, MHz_u{5180}, true, true);

    /*
     * Receiver PHY Operating Channel: 5 GHz Channel 38 (5170 MHz – 5210 MHz) with P20 index 1
     * Transmitted 20 MHz PPDU: 5 GHz Channel 36 (5170 MHz – 5190 MHz)
     * Overlap with primary 20 MHz: no
     * Primary 20 MHz fully covered: no
     */
    RunOne(WIFI_PHY_BAND_5GHZ, MHz_u{5190}, 1, MHz_u{5180}, false, false);

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief This test verifies the correct function of the WifiBandwidthFilter. 2 SpectrumWifiPhy are
 * setup and connected on the same spectrum channel. The test will
 * send a packet over the channel and if the signal plus guardband overlaps the channel the
 * filter will not discard the signal but if there is no overlap the filter will filter it out.
 */
class TestSpectrumChannelWithBandwidthFilter : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param channel channel to be used by transmitter
     * @param expectedValue expected number of received packets
     */
    TestSpectrumChannelWithBandwidthFilter(uint8_t channel, uint16_t expectedValue);

  protected:
    void DoSetup() override;
    void DoTeardown() override;

  private:
    /**
     * Callback invoked when the PHY model starts to process a signal
     *
     * @param signal the arriving signal
     * @param senderNodeId node Id of the sender of the signal
     * @param rxPower received signal power
     * @param duration signal duration
     */
    void RxBegin(Ptr<const SpectrumSignalParameters> signal,
                 uint32_t senderNodeId,
                 double rxPower,
                 Time duration);

    /**
     * Send function (sends a single packet)
     */
    void Send() const;

    /**
     * Event scheduled at end of simulation for validation
     *
     * @param expectedValue expected number of receive events
     */
    void CheckRxPacketCount(uint16_t expectedValue);

    void DoRun() override;

    Ptr<SpectrumWifiPhy> m_tx{nullptr}; ///< transmit function
    Ptr<SpectrumWifiPhy> m_rx{nullptr}; ///< receive function
    uint32_t m_countRxBegin{0};         ///< count of receive events
    uint8_t m_channel{36};              ///< channel for packet transmission
    uint16_t m_expectedValue{0};        ///< expected count of receive events
};

TestSpectrumChannelWithBandwidthFilter::TestSpectrumChannelWithBandwidthFilter(
    uint8_t channel,
    uint16_t expectedValue)
    : TestCase("Test for early discard of signal in single-model-spectrum-channel::StartTx()"),
      m_channel(channel),
      m_expectedValue(expectedValue)
{
}

void
TestSpectrumChannelWithBandwidthFilter::Send() const
{
    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs7(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         false);

    Ptr<Packet> pkt = Create<Packet>(1000);
    WifiMacHeader hdr;

    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    Ptr<WifiPsdu> psdu = Create<WifiPsdu>(pkt, hdr);
    m_tx->Send(psdu, txVector);
}

void
TestSpectrumChannelWithBandwidthFilter::CheckRxPacketCount(uint16_t expectedValue)
{
    NS_TEST_ASSERT_MSG_EQ(m_countRxBegin,
                          expectedValue,
                          "Received a different amount of packets than expected.");
}

void
TestSpectrumChannelWithBandwidthFilter::RxBegin(Ptr<const SpectrumSignalParameters> signal
                                                [[maybe_unused]],
                                                uint32_t senderNodeId [[maybe_unused]],
                                                double rxPower [[maybe_unused]],
                                                Time duration [[maybe_unused]])
{
    NS_LOG_FUNCTION(this << signal << senderNodeId << rxPower << duration);
    m_countRxBegin++;
}

void
TestSpectrumChannelWithBandwidthFilter::DoSetup()
{
    NS_LOG_FUNCTION(this);
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();

    Ptr<WifiBandwidthFilter> wifiFilter = CreateObject<WifiBandwidthFilter>();
    channel->AddSpectrumTransmitFilter(wifiFilter);

    Ptr<Node> node = CreateObject<Node>();
    Ptr<WifiNetDevice> dev = CreateObject<WifiNetDevice>();
    m_tx = CreateObject<SpectrumWifiPhy>();
    m_tx->SetDevice(dev);
    m_tx->SetTxPowerStart(dBm_u{20});
    m_tx->SetTxPowerEnd(dBm_u{20});

    Ptr<Node> nodeRx = CreateObject<Node>();
    Ptr<WifiNetDevice> devRx = CreateObject<WifiNetDevice>();
    m_rx = CreateObject<SpectrumWifiPhy>();
    m_rx->SetDevice(devRx);

    Ptr<InterferenceHelper> interferenceTx = CreateObject<InterferenceHelper>();
    m_tx->SetInterferenceHelper(interferenceTx);
    Ptr<ErrorRateModel> errorTx = CreateObject<NistErrorRateModel>();
    m_tx->SetErrorRateModel(errorTx);

    Ptr<InterferenceHelper> interferenceRx = CreateObject<InterferenceHelper>();
    m_rx->SetInterferenceHelper(interferenceRx);
    Ptr<ErrorRateModel> errorRx = CreateObject<NistErrorRateModel>();
    m_rx->SetErrorRateModel(errorRx);

    m_tx->AddChannel(channel);
    m_rx->AddChannel(channel);

    m_tx->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_rx->ConfigureStandard(WIFI_STANDARD_80211ax);

    dev->SetPhy(m_tx);
    node->AddDevice(dev);
    devRx->SetPhy(m_rx);
    nodeRx->AddDevice(devRx);

    m_rx->TraceConnectWithoutContext(
        "SignalArrival",
        MakeCallback(&TestSpectrumChannelWithBandwidthFilter::RxBegin, this));
}

void
TestSpectrumChannelWithBandwidthFilter::DoTeardown()
{
    m_tx->Dispose();
    m_rx->Dispose();
}

void
TestSpectrumChannelWithBandwidthFilter::DoRun()
{
    NS_LOG_FUNCTION(this);
    m_tx->SetOperatingChannel(WifiPhy::ChannelTuple{m_channel, 0, WIFI_PHY_BAND_5GHZ, 0});
    m_rx->SetOperatingChannel(WifiPhy::ChannelTuple{36, 0, WIFI_PHY_BAND_5GHZ, 0});

    Simulator::Schedule(MilliSeconds(100), &TestSpectrumChannelWithBandwidthFilter::Send, this);
    Simulator::Schedule(MilliSeconds(101),
                        &TestSpectrumChannelWithBandwidthFilter::CheckRxPacketCount,
                        this,
                        m_expectedValue);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief This test verifies that the WifiPhyRxfailureReason distinguishes between two cases:  1) a
 * drop due to transmitting during the signal detection interval, and 2) a drop due to transmitting
 * after the receiver has detected a preamble but is waiting for the end of the preamble. 2
 * SpectrumWifiPhy are setup and connected on the same spectrum channel. The test will send a packet
 * over the channel and after a controlled amount of transmit delay (to check both cases) the
 * receiver of the packet will send its own packet. If delay is less than preamble detection period,
 * the signal detection should be aborted by transmission. If delay is greater than preamble
 * detection period, the signal reception should be aborted by transmission.
 */
class TestPhyDropDueToTx : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param delay delay in microseconds to send second packet
     * @param expectedReason expected failure reason
     */
    TestPhyDropDueToTx(Time delay, WifiPhyRxfailureReason expectedReason);

  protected:
    void DoSetup() override;
    void DoTeardown() override;

  private:
    /**
     * RX dropped function
     * @param p the packet
     * @param reason the reason
     */
    void PhyDropTraceSink(Ptr<const Packet> p, WifiPhyRxfailureReason reason);

    /**
     * Send function (sends a single packet)
     * @param phy the WifiPhy object to send the packet
     */
    void Send(Ptr<WifiPhy> phy) const;

    /**
     * Event scheduled at end of simulation for validation
     */
    void CheckDropReason();

    void DoRun() override;

    Ptr<SpectrumWifiPhy> m_phyA{nullptr}; ///< transmit function
    Ptr<SpectrumWifiPhy> m_phyB{nullptr}; ///< transmit/receive function

    Time m_delay; ///< delay between transmissions in MicroSeconds

    WifiPhyRxfailureReason m_expectedReason; ///< expected WifiPhyRxfailureReason
    WifiPhyRxfailureReason m_observedReason; ///< observed WifiPhyRxfailureReason
};

TestPhyDropDueToTx::TestPhyDropDueToTx(Time delay, WifiPhyRxfailureReason expectedReason)
    : TestCase("Test for correct WifiPhyRxfailureReason from PhyRxDrop trace"),
      m_delay(delay),
      m_expectedReason(expectedReason)
{
}

void
TestPhyDropDueToTx::Send(Ptr<WifiPhy> phy) const
{
    const auto txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                       0,
                                       WIFI_PREAMBLE_HE_SU,
                                       NanoSeconds(800),
                                       1,
                                       1,
                                       0,
                                       MHz_u{20},
                                       false);

    auto pkt = Create<Packet>(1000);
    WifiMacHeader hdr;

    hdr.SetType(WIFI_MAC_QOSDATA);
    hdr.SetQosTid(0);

    auto psdu = Create<WifiPsdu>(pkt, hdr);
    phy->Send(psdu, txVector);
}

void
TestPhyDropDueToTx::CheckDropReason()
{
    NS_TEST_ASSERT_MSG_EQ(m_expectedReason,
                          m_observedReason,
                          "Packet was dropped due to the wrong drop reason reported ");
}

void
TestPhyDropDueToTx::PhyDropTraceSink(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
    NS_LOG_FUNCTION(this << p << reason);
    m_observedReason = reason;
}

void
TestPhyDropDueToTx::DoSetup()
{
    NS_LOG_FUNCTION(this);
    auto channel = CreateObject<SingleModelSpectrumChannel>();

    auto node = CreateObject<Node>();
    auto devA = CreateObject<WifiNetDevice>();
    m_phyA = CreateObject<SpectrumWifiPhy>();
    m_phyA->SetDevice(devA);
    m_phyA->SetTxPowerStart(dBm_u{20});
    m_phyA->SetTxPowerEnd(dBm_u{20});

    auto nodeRx = CreateObject<Node>();
    auto devB = CreateObject<WifiNetDevice>();
    m_phyB = CreateObject<SpectrumWifiPhy>();
    m_phyB->SetDevice(devB);
    m_phyB->SetTxPowerStart(dBm_u{20});
    m_phyB->SetTxPowerEnd(dBm_u{20});

    auto interferenceTx = CreateObject<InterferenceHelper>();
    m_phyA->SetInterferenceHelper(interferenceTx);
    auto errorTx = CreateObject<NistErrorRateModel>();
    m_phyA->SetErrorRateModel(errorTx);

    auto interferenceRx = CreateObject<InterferenceHelper>();
    m_phyB->SetInterferenceHelper(interferenceRx);
    auto errorRx = CreateObject<NistErrorRateModel>();
    m_phyB->SetErrorRateModel(errorRx);

    m_phyA->AddChannel(channel);
    m_phyB->AddChannel(channel);

    m_phyA->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phyA->SetOperatingChannel(WifiPhy::ChannelTuple{36, 0, WIFI_PHY_BAND_5GHZ, 0});

    m_phyB->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_phyB->SetOperatingChannel(WifiPhy::ChannelTuple{36, 0, WIFI_PHY_BAND_5GHZ, 0});

    devA->SetPhy(m_phyA);
    node->AddDevice(devA);
    devB->SetPhy(m_phyB);
    nodeRx->AddDevice(devB);

    m_phyB->TraceConnectWithoutContext("PhyRxDrop",
                                       MakeCallback(&TestPhyDropDueToTx::PhyDropTraceSink, this));
}

void
TestPhyDropDueToTx::DoTeardown()
{
    m_phyA->Dispose();
    m_phyB->Dispose();
}

void
TestPhyDropDueToTx::DoRun()
{
    NS_LOG_FUNCTION(this);

    Simulator::ScheduleNow(&TestPhyDropDueToTx::Send, this, m_phyA);
    Simulator::Schedule(m_delay, &TestPhyDropDueToTx::Send, this, m_phyB);

    // Upon transmitting the second packet from m_phyB, the reception from
    // m_phyA will be immediately dropped.  Check the drop reason a short
    // while later (1 ns is sufficient)
    Simulator::Schedule(m_delay + NanoSeconds(1), &TestPhyDropDueToTx::CheckDropReason, this);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi PHY reception Test Suite
 */
class WifiPhyReceptionTestSuite : public TestSuite
{
  public:
    WifiPhyReceptionTestSuite();
};

WifiPhyReceptionTestSuite::WifiPhyReceptionTestSuite()
    : TestSuite("wifi-phy-reception", Type::UNIT)
{
    AddTestCase(new TestThresholdPreambleDetectionWithoutFrameCapture, TestCase::Duration::QUICK);
    AddTestCase(new TestThresholdPreambleDetectionWithFrameCapture, TestCase::Duration::QUICK);
    AddTestCase(new TestSimpleFrameCaptureModel, TestCase::Duration::QUICK);
    AddTestCase(new TestPhyHeadersReception, TestCase::Duration::QUICK);
    AddTestCase(new TestAmpduReception, TestCase::Duration::QUICK);
    AddTestCase(new TestUnsupportedModulationReception(), TestCase::Duration::QUICK);
    AddTestCase(new TestUnsupportedBandwidthReception(), TestCase::Duration::QUICK);
    AddTestCase(new TestPrimary20CoveredByPpdu(), TestCase::Duration::QUICK);
    // The below three test cases are related.  The test involves a receiver tuned to
    // channel 36 and a transmitter sending on channels 36, 40, and 44, respectively.
    // The second argument corresponds to the number of signals expected to be received.
    // Signals on channel 36 and 40 will fall within the receiver bandwidth, while
    // a signal on channel 44 will fall completely outside and will be filtered.
    AddTestCase(new TestSpectrumChannelWithBandwidthFilter(36, 1), TestCase::Duration::QUICK);
    AddTestCase(new TestSpectrumChannelWithBandwidthFilter(40, 1), TestCase::Duration::QUICK);
    AddTestCase(new TestSpectrumChannelWithBandwidthFilter(44, 0), TestCase::Duration::QUICK);
    AddTestCase(new TestSpectrumChannelWithBandwidthFilter(36, 1), TestCase::Duration::QUICK);
    AddTestCase(new TestSpectrumChannelWithBandwidthFilter(40, 1), TestCase::Duration::QUICK);
    AddTestCase(new TestSpectrumChannelWithBandwidthFilter(44, 0), TestCase::Duration::QUICK);
    // 4 Microseconds is just less than the preamble detection period since there is no
    // propagation delay model
    AddTestCase(new TestPhyDropDueToTx(MicroSeconds(4), SIGNAL_DETECTION_ABORTED_BY_TX),
                TestCase::Duration::QUICK);
    AddTestCase(new TestPhyDropDueToTx(MicroSeconds(5), RECEPTION_ABORTED_BY_TX),
                TestCase::Duration::QUICK);
}

static WifiPhyReceptionTestSuite wifiPhyReceptionTestSuite; ///< the test suite
