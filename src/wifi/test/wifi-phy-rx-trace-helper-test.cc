/*
 * Copyright (c) 2023 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */
#include "ns3/ampdu-tag.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/double.h"
#include "ns3/eht-configuration.h"
#include "ns3/he-phy.h"
#include "ns3/he-ppdu.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mpdu-aggregator.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/nist-error-rate-model.h"
#include "ns3/ofdm-ppdu.h"
#include "ns3/packet-socket-address.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/propagation-loss-model.h"
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
#include "ns3/wifi-phy-rx-trace-helper.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-spectrum-phy-interface.h"
#include "ns3/wifi-spectrum-signal-parameters.h"
#include "ns3/wifi-spectrum-value-helper.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-phy.h"

#include <optional>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyRxTraceHelperTest");

/**
 * @ingroup wifi-test
 * @brief Implements a test case to evaluate the reception process of WiFi Physical Layer (PHY)
 * frames (PPDU) with multiple MAC Protocol Data Units (MPDUs).
 *
 * This class extends the TestCase class to simulate and analyze the reception of PPDUs containing
 * either one or two MPDUs. It specifically tests the PHY layer's capability to handle multiple
 * MPDUs addressed to specific receivers, measuring success and failure rates. The simulation setup
 * includes configuring transmit and receive power levels, and the class provides functionality to
 * check statistics related to PPDU and MPDU reception success and failure, as well as overlap
 * handling.
 */
class TestWifiPhyRxTraceHelper : public TestCase
{
  public:
    /**
     * Constructs a TestWifiPhyRxTraceHelper instance with a given test name.
     * @param test_name The name of the test case.
     */
    TestWifiPhyRxTraceHelper(std::string test_name);

    /**
     * Destructor.
     */
    ~TestWifiPhyRxTraceHelper() override = default;

  private:
    void DoSetup() override;
    void DoRun() override;
    void DoTeardown() override;

    /**
     * Sends a PPDU containing two MPDUs addressed to specific receivers.
     * @param rxPower The transmit power.
     * @param add1 The MAC address of the first receiver.
     * @param add2 The MAC address of the second receiver.
     * @param tx_phy The transmitting PHY object.
     */
    void SendPpduWithTwoMpdus(dBm_u rxPower,
                              Mac48Address add1,
                              Mac48Address add2,
                              Ptr<ns3::SpectrumWifiPhy> tx_phy);

    /**
     * Sends a PPDU containing one MPDU addressed to a specific receiver.
     * @param rxPower The transmit power.
     * @param add1 The MAC address of the receiver.
     * @param tx_phy The transmitting PHY object.
     */
    void SendPpduWithOneMpdu(dBm_u rxPower, Mac48Address add1, Ptr<ns3::SpectrumWifiPhy> tx_phy);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedPpduSuccess The expected number of successful PPDU receptions.
     * @param expectedPpduFailure The expected number of failed PPDU receptions.
     * @param expectedMpduSuccess The expected number of successful MPDU receptions within PPDUs.
     * @param expectedMpduFailure The expected number of failed MPDU receptions within PPDUs.
     * @param expectedOverlaps The expected number of overlapping reception events.
     * @param expectedNonOverlaps The expected number of non-overlapping reception events.
     */
    void CheckAllStats(uint64_t expectedPpduSuccess,
                       uint64_t expectedPpduFailure,
                       uint64_t expectedMpduSuccess,
                       uint64_t expectedMpduFailure,
                       uint64_t expectedOverlaps,
                       uint64_t expectedNonOverlaps);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedStats The expected counts for PPDU and MPDU reception.
     * @param nodeId The node ID for which to check the received stats.
     * @param deviceId The device ID for which to check the received stats.
     * @param linkId The link ID for which to check the received stats.
     */
    void CheckStats(WifiPhyTraceStatistics expectedStats,
                    uint32_t nodeId,
                    uint32_t deviceId,
                    uint32_t linkId);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedRecordCount The expected number of records.
     */
    void CheckRecords(uint64_t expectedRecordCount);

    Mac48Address wrongReceiver =
        Mac48Address("00:00:00:00:00:01"); ///< The MAC address representing an incorrect receiver,
                                           ///< used for testing.
    Mac48Address correctReceiver =
        Mac48Address("00:00:00:00:00:03"); ///< The MAC address representing the correct receiver,
                                           ///< used for testing.

    std::map<ns3::Mac48Address, uint32_t> MacToNodeId = {
        {Mac48Address("00:00:00:00:00:04"), 0},
        {Mac48Address("00:00:00:00:00:05"), 1},
        {correctReceiver, 2},
        {wrongReceiver, 3}}; ///< Maps MAC addresses to node IDs. This is done since there is no MAC
                             ///< layer configured and therefore the helper cannot obtain the MAC
                             ///< addresses automatically.

    Ptr<Node> nodeRx;                    ///< The receiving node
    Ptr<SpectrumWifiPhy> m_txA{nullptr}; ///< The transmit function for node A.
    Ptr<SpectrumWifiPhy> m_txB{nullptr}; ///< The transmit function for node B.
    Ptr<SpectrumWifiPhy> m_rx{nullptr};  ///< The receive function for testing.

    WifiPhyRxTraceHelper
        m_rxTraceHelper; ///< The helper being tested for tracking PHY reception events.
};

TestWifiPhyRxTraceHelper::TestWifiPhyRxTraceHelper(std::string test_name)
    : TestCase{test_name}
{
}

void
TestWifiPhyRxTraceHelper::DoSetup()
{
    dBm_u txPower{20};

    auto spectrumChannel = CreateObject<SingleModelSpectrumChannel>();

    auto nodeA = CreateObject<Node>();
    auto devA = CreateObject<WifiNetDevice>();
    m_txA = CreateObject<SpectrumWifiPhy>();
    m_txA->SetDevice(devA);
    m_txA->SetTxPowerStart(txPower);
    m_txA->SetTxPowerEnd(txPower);

    auto nodeB = CreateObject<Node>();
    auto devB = CreateObject<WifiNetDevice>();
    m_txB = CreateObject<SpectrumWifiPhy>();
    m_txB->SetDevice(devB);
    m_txB->SetTxPowerStart(txPower);
    m_txB->SetTxPowerEnd(txPower);

    nodeRx = CreateObject<Node>();
    auto devRx = CreateObject<WifiNetDevice>();
    m_rx = CreateObject<SpectrumWifiPhy>();
    m_rx->SetDevice(devRx);

    Ptr<InterferenceHelper> interferenceTxA = CreateObject<InterferenceHelper>();
    m_txA->SetInterferenceHelper(interferenceTxA);
    auto errorTxA = CreateObject<NistErrorRateModel>();
    m_txA->SetErrorRateModel(errorTxA);

    auto interferenceTxB = CreateObject<InterferenceHelper>();
    m_txB->SetInterferenceHelper(interferenceTxB);
    auto errorTxB = CreateObject<NistErrorRateModel>();
    m_txB->SetErrorRateModel(errorTxB);

    auto interferenceRx = CreateObject<InterferenceHelper>();
    m_rx->SetInterferenceHelper(interferenceRx);
    auto errorRx = CreateObject<NistErrorRateModel>();
    m_rx->SetErrorRateModel(errorRx);

    m_txA->AddChannel(spectrumChannel);
    m_txB->AddChannel(spectrumChannel);
    m_rx->AddChannel(spectrumChannel);

    m_txA->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_txB->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_rx->ConfigureStandard(WIFI_STANDARD_80211ax);

    m_txA->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});
    m_txB->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});
    m_rx->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});

    devA->SetPhy(m_txA);
    nodeA->AddDevice(devA);
    devB->SetPhy(m_txB);
    nodeB->AddDevice(devB);
    devRx->SetPhy(m_rx);
    nodeRx->AddDevice(devRx);

    NodeContainer nodes;
    nodes.Add(nodeA);
    nodes.Add(nodeB);
    nodes.Add(nodeRx);

    m_rxTraceHelper.Enable(nodes, MacToNodeId);

    auto preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(4));
    preambleDetectionModel->SetAttribute("MinimumRssi", DoubleValue(-82));
    m_rx->SetPreambleDetectionModel(preambleDetectionModel);
}

void
TestWifiPhyRxTraceHelper::DoTeardown()
{
    m_txA->Dispose();
    m_txB->Dispose();
    m_rx->Dispose();
}

void
TestWifiPhyRxTraceHelper::SendPpduWithTwoMpdus(dBm_u rxPower,
                                               Mac48Address receiver1,
                                               Mac48Address receiver2,
                                               Ptr<ns3::SpectrumWifiPhy> tx_phy)
{
    auto txPower = rxPower;
    tx_phy->SetTxPowerStart(txPower);
    tx_phy->SetTxPowerEnd(txPower);

    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         true);

    std::vector<Ptr<WifiMpdu>> mpduList;

    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(receiver1); // Changing the expected receiver
    auto p1 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p1, hdr1));

    WifiMacHeader hdr2;
    hdr2.SetType(WIFI_MAC_QOSDATA);
    hdr2.SetQosTid(0);
    hdr2.SetAddr1(receiver2); // Changing the expected receiver
    auto p2 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p2, hdr2));

    auto psdu = Create<WifiPsdu>(mpduList);

    tx_phy->Send(psdu, txVector);
}

void
TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu(dBm_u rxPower,
                                              Mac48Address receiver1,
                                              Ptr<ns3::SpectrumWifiPhy> tx_phy)
{
    auto txPower = rxPower;
    tx_phy->SetTxPowerStart(txPower);
    tx_phy->SetTxPowerEnd(txPower);

    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         true);

    std::vector<Ptr<WifiMpdu>> mpduList;

    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(receiver1); // Changing the expected receiver
    auto p1 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p1, hdr1));

    auto psdu = Create<WifiPsdu>(mpduList);
    tx_phy->Send(psdu, txVector);
}

void
TestWifiPhyRxTraceHelper::CheckAllStats(uint64_t expectedPpduSuccess,
                                        uint64_t expectedPpduFailure,
                                        uint64_t expectedMpduSuccess,
                                        uint64_t expectedMpduFailure,
                                        uint64_t expectedOverlaps,
                                        uint64_t expectedNonOverlaps)
{
    WifiPhyTraceStatistics stats = m_rxTraceHelper.GetStatistics();

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedPpduSuccess,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedPpduFailure,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedMpduSuccess,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedMpduFailure,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedOverlaps,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedNonOverlaps,
                          "Didn't receive right number of nonoverlapping PPDUs");
    m_rxTraceHelper.Reset();
}

void
TestWifiPhyRxTraceHelper::CheckStats(WifiPhyTraceStatistics expectedStats,
                                     uint32_t nodeId,
                                     uint32_t deviceId,
                                     uint32_t linkId)
{
    WifiPhyTraceStatistics stats = m_rxTraceHelper.GetStatistics(nodeId, deviceId, linkId);

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedStats.m_receivedPpdus,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedStats.m_failedPpdus,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedStats.m_receivedMpdus,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedStats.m_failedMpdus,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedStats.m_overlappingPpdus,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedStats.m_nonOverlappingPpdus,
                          "Didn't receive right number of nonoverlapping PPDUs");
}

void
TestWifiPhyRxTraceHelper::CheckRecords(uint64_t expectedRecordCount)
{
    auto records = m_rxTraceHelper.GetPpduRecords();
    NS_TEST_ASSERT_MSG_EQ(records.size(),
                          expectedRecordCount,
                          "Didn't produce the right number of Records");
}

void
TestWifiPhyRxTraceHelper::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 1;
    dBm_u rxPower{-80};
    streamNumber += m_txA->AssignStreams(streamNumber);
    streamNumber += m_txB->AssignStreams(streamNumber);
    streamNumber += m_rx->AssignStreams(streamNumber);
    WifiPhyTraceStatistics expectedStats;

    m_rxTraceHelper.Start(Seconds(0.01));

    // CASE 1: PPDU Reception with Sufficient RSSI With SOME Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 1;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 1;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;

    // A-MPDU 1
    Simulator::Schedule(Seconds(0.1),
                        &TestWifiPhyRxTraceHelper::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.19),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        2); // Total Records

    Simulator::Schedule(Seconds(0.2),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.2),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 2: PPDU Reception with inSufficient RSSI With SOME Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 1;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 1;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;
    rxPower = dBm_u{-83};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.3),
                        &TestWifiPhyRxTraceHelper::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.39),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(0.4),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.4),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 3: PPDU Reception with Sufficient RSSI/SNR With NO Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.5),
                        &TestWifiPhyRxTraceHelper::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.59),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(0.6),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.6),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 4: PPDU Reception with Insufficient RSSI/SNR With NO Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-83};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.7),
                        &TestWifiPhyRxTraceHelper::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.79),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(0.8),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.8),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 5: PPDU Overlapping Reception with sufficient RSSI/SNR With ALL Frames Addressed to
    // Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 2;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 2;
    expectedStats.m_overlappingPpdus = 2;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);
    // A-MPDU 2
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txB);

    Simulator::Schedule(Seconds(0.99),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        4); // TotalRecords

    Simulator::Schedule(Seconds(1),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 6: PPDU Overlapping Reception with sufficient RSSI/SNR With SOME Frames Addressed to
    // Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 1;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 1;
    expectedStats.m_overlappingPpdus = 1;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);
    // A-MPDU 2
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        m_txB);

    Simulator::Schedule(Seconds(1.19),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        4); // TotalRecords

    Simulator::Schedule(Seconds(1.2),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.2),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // Stop Statistics Collection Period Since following testcases evaluate the Start and Stop
    // methods
    m_rxTraceHelper.Stop(Seconds(1.21));

    // CASE 7: Execution of "Start()" Before Signal Injection
    expectedStats.m_receivedPpdus = 1;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 1;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;
    rxPower = dBm_u{-80};
    m_rxTraceHelper.Start(Seconds(1.29));

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.3),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(1.39),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(1.4),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.4),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    m_rxTraceHelper.Stop(Seconds(1.4));

    // CASE 8: Activation of "Start()" Followed by "Stop()" Before Signal Injection
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};

    m_rxTraceHelper.Start(Seconds(1.41));

    m_rxTraceHelper.Stop(Seconds(1.42));

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.45),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(1.549),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        0); // TotalRecords

    Simulator::Schedule(Seconds(1.55),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.55),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 9: "Start()" Method Initiated During Ongoing PPDU Reception
    expectedStats.m_receivedPpdus = 1;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 1;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;
    rxPower = dBm_u{-80};

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.6),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    m_rxTraceHelper.Start(Seconds(1.6) + MicroSeconds(10));

    Simulator::Schedule(Seconds(1.69),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(1.7),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.7),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    m_rxTraceHelper.Stop(Seconds(1.7));

    // CASE 10: Execution of "Stop()" During Ongoing PPDU Reception
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};

    m_rxTraceHelper.Start(Seconds(1.79));

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.8),
                        &TestWifiPhyRxTraceHelper::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    m_rxTraceHelper.Stop(Seconds(1.8) + MicroSeconds(10));

    Simulator::Schedule(Seconds(1.89),
                        &TestWifiPhyRxTraceHelper::CheckRecords,
                        this,
                        0); // TotalRecords

    Simulator::Schedule(Seconds(1.9),
                        &TestWifiPhyRxTraceHelper::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.9),
                        &TestWifiPhyRxTraceHelper::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @brief Implements a test case to evaluate the reception process of WiFi Physical Layer (PHY)
 * frames (PPDU) containing multiple MAC Protocol Data Units (MPDUs) in Multi-Link Operation (MLO)
 * contexts.
 *
 * This class extends the TestCase class to simulate and analyze the reception of PPDUs containing
 * either one or two MPDUs in MLO setups. It specifically assesses the WifiPhyRxTraceHelper
 * ability to manage MLO by handling multiple MPDUs addressed to specific receivers across different
 * links, measuring success and failure rates. The simulation setup includes configuring transmit
 * and receive power levels and provides functionality to check statistics related to PPDU and MPDU
 * reception success and failure, as well overlaps and non-overlaps in reception
 * events.
 */
class TestWifiPhyRxTraceHelperMloStr : public TestCase
{
  public:
    /**
     * Constructs a TestWifiPhyRxTraceHelperMloStr instance for MLO reception testing.
     */
    TestWifiPhyRxTraceHelperMloStr();

  private:
    void DoSetup() override;
    void DoRun() override;

    void DoTeardown() override;

    /**
     * Sends a PPDU containing two MPDUs addressed to specific receivers, simulating an MLO
     * scenario.
     * @param rxPower The transmit power.
     * @param add1 The MAC address of the first receiver.
     * @param add2 The MAC address of the second receiver.
     * @param tx_phy The transmitting PHY object for MLO.
     */
    void SendPpduWithTwoMpdus(dBm_u rxPower,
                              Mac48Address add1,
                              Mac48Address add2,
                              Ptr<ns3::SpectrumWifiPhy> tx_phy);

    /**
     * Sends a PPDU containing one MPDU addressed to a specific receiver, within an MLO setup.
     * @param rxPower The transmit power.
     * @param add1 The MAC address of the receiver.
     * @param tx_phy The transmitting PHY object for MLO.
     */
    void SendPpduWithOneMpdu(dBm_u rxPower, Mac48Address add1, Ptr<ns3::SpectrumWifiPhy> tx_phy);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedStats The expected counts for PPDU and MPDU reception.
     * @param nodeId The node ID for which to check the received stats.
     */
    void CheckStats(WifiPhyTraceStatistics expectedStats, uint32_t nodeId);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedStats The expected counts for PPDU and MPDU reception.
     * @param nodeId The node ID for which to check the received stats.
     * @param deviceId The device ID for which to check the received stats.
     */
    void CheckStats(WifiPhyTraceStatistics expectedStats, uint32_t nodeId, uint32_t deviceId);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedStats The expected counts for PPDU and MPDU reception.
     * @param nodeId The node ID for which to check the received stats.
     * @param deviceId The device ID for which to check the received stats.
     * @param linkId The link ID for which to check the received stats.
     */
    void CheckStats(WifiPhyTraceStatistics expectedStats,
                    uint32_t nodeId,
                    uint32_t deviceId,
                    uint32_t linkId);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure in MLO scenarios.
     * @param expectedPpduSuccess The expected number of successful PPDU receptions.
     * @param expectedPpduFailure The expected number of failed PPDU receptions.
     * @param expectedMpduSuccess The expected number of successful MPDU receptions within PPDUs.
     * @param expectedMpduFailure The expected number of failed MPDU receptions within PPDUs.
     * @param expectedOverlaps The expected number of overlapping reception events.
     * @param expectedNonOverlaps The expected number of non-overlapping reception events.
     */
    void CheckAllStats(uint64_t expectedPpduSuccess,
                       uint64_t expectedPpduFailure,
                       uint64_t expectedMpduSuccess,
                       uint64_t expectedMpduFailure,
                       uint64_t expectedOverlaps,
                       uint64_t expectedNonOverlaps);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedRecordCount The expected number of records.
     */
    void CheckRecords(uint64_t expectedRecordCount);

    Mac48Address wrongReceiver =
        Mac48Address("00:00:00:00:00:01"); ///< The MAC address representing an incorrect receiver,
                                           ///< used for testing in MLO.
    Mac48Address correctReceiver =
        Mac48Address("00:00:00:00:00:03"); ///< The MAC address representing the correct receiver,
                                           ///< used for testing in MLO.

    std::map<ns3::Mac48Address, uint32_t> MacToNodeId = {
        {Mac48Address("00:00:00:00:00:04"), 0},
        {Mac48Address("00:00:00:00:00:05"), 1},
        {correctReceiver, 2},
        {wrongReceiver, 3}}; ///< Maps MAC addresses to node IDs for MLO test configuration. This is
                             ///< done since there is no MAC layer configured and therefore the
                             ///< helper cannot obtain the MAC addresses automatically.

    NodeContainer wifiNodes;              ///< All wifi nodes
    Ptr<SpectrumWifiPhy> m_tx0A{nullptr}; ///< The transmit function for node 0, link A.
    Ptr<SpectrumWifiPhy> m_tx0B{nullptr}; ///< The transmit function for node 0, link B.
    Ptr<SpectrumWifiPhy> m_tx1A{nullptr}; ///< The transmit function for node 1, link A.
    Ptr<SpectrumWifiPhy> m_tx1B{nullptr}; ///< The transmit function for node 1, link B.
    Ptr<SpectrumWifiPhy> m_rxA{nullptr};  ///< The receive function for node 2, link A.
    Ptr<SpectrumWifiPhy> m_rxB{nullptr};  ///< The receive function for node 2, link B.

    WifiPhyRxTraceHelper m_rxTraceHelper; ///< The helper being tested for tracking
                                          ///< PHY reception events in MLO scenarios.
};

TestWifiPhyRxTraceHelperMloStr::TestWifiPhyRxTraceHelperMloStr()
    : TestCase("Test for correct MLO operation")
{
}

void
TestWifiPhyRxTraceHelperMloStr::DoSetup()
{
    dBm_u txPower{20};
    auto ehtConfiguration = CreateObject<EhtConfiguration>();

    auto nodeA = CreateObject<Node>();
    auto devA = CreateObject<WifiNetDevice>();
    devA->SetStandard(WIFI_STANDARD_80211be);
    devA->SetEhtConfiguration(ehtConfiguration);

    m_tx0A = CreateObject<SpectrumWifiPhy>();
    m_tx0A->SetDevice(devA);
    m_tx0A->SetTxPowerStart(txPower);
    m_tx0A->SetTxPowerEnd(txPower);

    m_tx0B = CreateObject<SpectrumWifiPhy>();
    m_tx0B->SetDevice(devA);
    m_tx0B->SetTxPowerStart(txPower);
    m_tx0B->SetTxPowerEnd(txPower);

    auto nodeB = CreateObject<Node>();
    auto devB = CreateObject<WifiNetDevice>();
    devB->SetStandard(WIFI_STANDARD_80211be);
    devB->SetEhtConfiguration(ehtConfiguration);

    m_tx1A = CreateObject<SpectrumWifiPhy>();
    m_tx1A->SetDevice(devB);
    m_tx1A->SetTxPowerStart(txPower);
    m_tx1A->SetTxPowerEnd(txPower);

    m_tx1B = CreateObject<SpectrumWifiPhy>();
    m_tx1B->SetDevice(devB);
    m_tx1B->SetTxPowerStart(txPower);
    m_tx1B->SetTxPowerEnd(txPower);

    auto nodeRx = CreateObject<Node>();
    auto devRx = CreateObject<WifiNetDevice>();
    devRx->SetStandard(WIFI_STANDARD_80211be);
    devRx->SetEhtConfiguration(ehtConfiguration);
    m_rxA = CreateObject<SpectrumWifiPhy>();
    m_rxA->SetDevice(devRx);
    m_rxB = CreateObject<SpectrumWifiPhy>();
    m_rxB->SetDevice(devRx);

    auto interferenceTx0A = CreateObject<InterferenceHelper>();
    m_tx0A->SetInterferenceHelper(interferenceTx0A);
    auto errorTx0A = CreateObject<NistErrorRateModel>();
    m_tx0A->SetErrorRateModel(errorTx0A);

    auto interferenceTx0B = CreateObject<InterferenceHelper>();
    m_tx0B->SetInterferenceHelper(interferenceTx0B);
    auto errorTx0B = CreateObject<NistErrorRateModel>();
    m_tx0B->SetErrorRateModel(errorTx0B);

    auto interferenceTx1A = CreateObject<InterferenceHelper>();
    m_tx1A->SetInterferenceHelper(interferenceTx1A);
    auto errorTx1A = CreateObject<NistErrorRateModel>();
    m_tx1A->SetErrorRateModel(errorTx1A);

    auto interferenceTx1B = CreateObject<InterferenceHelper>();
    m_tx1B->SetInterferenceHelper(interferenceTx1B);
    auto errorTx1B = CreateObject<NistErrorRateModel>();
    m_tx1B->SetErrorRateModel(errorTx1B);

    auto interferenceRxA = CreateObject<InterferenceHelper>();
    m_rxA->SetInterferenceHelper(interferenceRxA);
    auto errorRxA = CreateObject<NistErrorRateModel>();
    m_rxA->SetErrorRateModel(errorRxA);

    auto interferenceRxB = CreateObject<InterferenceHelper>();
    m_rxB->SetInterferenceHelper(interferenceRxB);
    auto errorRxB = CreateObject<NistErrorRateModel>();
    m_rxB->SetErrorRateModel(errorRxB);

    auto spectrumChannelA = CreateObject<MultiModelSpectrumChannel>();
    auto spectrumChannelB = CreateObject<MultiModelSpectrumChannel>();

    m_tx0A->SetOperatingChannel(WifiPhy::ChannelTuple{2, 0, WIFI_PHY_BAND_2_4GHZ, 0});
    m_tx0B->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});

    m_tx1A->SetOperatingChannel(WifiPhy::ChannelTuple{2, 0, WIFI_PHY_BAND_2_4GHZ, 0});
    m_tx1B->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});

    m_rxA->SetOperatingChannel(WifiPhy::ChannelTuple{2, 0, WIFI_PHY_BAND_2_4GHZ, 0});
    m_rxB->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});

    m_tx0A->AddChannel(spectrumChannelA, WIFI_SPECTRUM_2_4_GHZ);
    m_tx0B->AddChannel(spectrumChannelB, WIFI_SPECTRUM_5_GHZ);

    m_tx1A->AddChannel(spectrumChannelA, WIFI_SPECTRUM_2_4_GHZ);
    m_tx1B->AddChannel(spectrumChannelB, WIFI_SPECTRUM_5_GHZ);

    m_rxA->AddChannel(spectrumChannelA, WIFI_SPECTRUM_2_4_GHZ);
    m_rxB->AddChannel(spectrumChannelB, WIFI_SPECTRUM_5_GHZ);

    m_tx0A->ConfigureStandard(WIFI_STANDARD_80211be);
    m_tx0B->ConfigureStandard(WIFI_STANDARD_80211be);

    m_tx1A->ConfigureStandard(WIFI_STANDARD_80211be);
    m_tx1B->ConfigureStandard(WIFI_STANDARD_80211be);

    m_rxA->ConfigureStandard(WIFI_STANDARD_80211be);
    m_rxB->ConfigureStandard(WIFI_STANDARD_80211be);

    std::vector<Ptr<WifiPhy>> phys0;
    phys0.emplace_back(m_tx0A);
    phys0.emplace_back(m_tx0B);

    std::vector<Ptr<WifiPhy>> phys1;
    phys1.emplace_back(m_tx1A);
    phys1.emplace_back(m_tx1B);

    std::vector<Ptr<WifiPhy>> physRx;
    physRx.emplace_back(m_rxA);
    physRx.emplace_back(m_rxB);

    devA->SetPhys(phys0);
    nodeA->AddDevice(devA);

    devB->SetPhys(phys1);
    nodeB->AddDevice(devB);

    devRx->SetPhys(physRx);
    nodeRx->AddDevice(devRx);

    wifiNodes.Add(nodeA);
    wifiNodes.Add(nodeB);
    wifiNodes.Add(nodeRx);

    m_rxTraceHelper.Enable(wifiNodes, MacToNodeId);

    auto preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(4));
    preambleDetectionModel->SetAttribute("MinimumRssi", DoubleValue(-82));
    m_rxA->SetPreambleDetectionModel(preambleDetectionModel);
    m_rxB->SetPreambleDetectionModel(preambleDetectionModel);
}

void
TestWifiPhyRxTraceHelperMloStr::DoTeardown()
{
    m_tx0A->Dispose();
    m_tx0B->Dispose();
    m_tx1A->Dispose();
    m_tx1B->Dispose();
    m_rxA->Dispose();
    m_rxB->Dispose();
}

void
TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus(dBm_u rxPower,
                                                     Mac48Address receiver1,
                                                     Mac48Address receiver2,
                                                     Ptr<ns3::SpectrumWifiPhy> tx_phy)
{
    auto txPower = rxPower;
    tx_phy->SetTxPowerStart(txPower);
    tx_phy->SetTxPowerEnd(txPower);

    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         true);

    std::vector<Ptr<WifiMpdu>> mpduList;

    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(receiver1); // Changing the expected receiver
    auto p1 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p1, hdr1));

    WifiMacHeader hdr2;
    hdr2.SetType(WIFI_MAC_QOSDATA);
    hdr2.SetQosTid(0);
    hdr2.SetAddr1(receiver2); // Changing the expected receiver
    auto p2 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p2, hdr2));

    auto psdu = Create<WifiPsdu>(mpduList);

    tx_phy->Send(psdu, txVector);
}

void
TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu(dBm_u rxPower,
                                                    Mac48Address receiver1,
                                                    Ptr<ns3::SpectrumWifiPhy> tx_phy)
{
    auto txPower = rxPower;
    tx_phy->SetTxPowerStart(txPower);
    tx_phy->SetTxPowerEnd(txPower);

    WifiTxVector txVector =
        WifiTxVector(HePhy::GetHeMcs0(),
                     0,
                     WIFI_PREAMBLE_HE_SU,
                     NanoSeconds(800),
                     1,
                     1,
                     0,
                     MHz_u{20},
                     true); // for some reason needs to be set to true even though only one MPDU

    std::vector<Ptr<WifiMpdu>> mpduList;

    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(receiver1); // Changing the expected receiver
    auto p1 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p1, hdr1));

    auto psdu = Create<WifiPsdu>(mpduList);
    tx_phy->Send(psdu, txVector);
}

void
TestWifiPhyRxTraceHelperMloStr::CheckStats(WifiPhyTraceStatistics expectedStats, uint32_t nodeId)
{
    WifiPhyTraceStatistics stats;
    for (uint32_t i = 0; i < wifiNodes.Get(nodeId)->GetNDevices(); i++)
    {
        for (uint32_t j = 0;
             j < DynamicCast<WifiNetDevice>(wifiNodes.Get(nodeId)->GetDevice(i))->GetNPhys();
             j++)
        {
            stats = stats + m_rxTraceHelper.GetStatistics(nodeId, i, j);
        }
    }

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedStats.m_receivedPpdus,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedStats.m_failedPpdus,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedStats.m_receivedMpdus,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedStats.m_failedMpdus,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedStats.m_overlappingPpdus,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedStats.m_nonOverlappingPpdus,
                          "Didn't receive right number of nonoverlapping PPDUs");
}

void
TestWifiPhyRxTraceHelperMloStr::CheckStats(WifiPhyTraceStatistics expectedStats,
                                           uint32_t nodeId,
                                           uint32_t deviceId)
{
    WifiPhyTraceStatistics stats;

    for (uint32_t i = 0;
         i < DynamicCast<WifiNetDevice>(wifiNodes.Get(nodeId)->GetDevice(deviceId))->GetNPhys();
         i++)
    {
        stats = stats + m_rxTraceHelper.GetStatistics(nodeId, deviceId, i);
    }

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedStats.m_receivedPpdus,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedStats.m_failedPpdus,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedStats.m_receivedMpdus,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedStats.m_failedMpdus,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedStats.m_overlappingPpdus,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedStats.m_nonOverlappingPpdus,
                          "Didn't receive right number of nonoverlapping PPDUs");
}

void
TestWifiPhyRxTraceHelperMloStr::CheckStats(WifiPhyTraceStatistics expectedStats,
                                           uint32_t nodeId,
                                           uint32_t deviceId,
                                           uint32_t linkId)
{
    WifiPhyTraceStatistics stats = m_rxTraceHelper.GetStatistics(nodeId, deviceId, linkId);

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedStats.m_receivedPpdus,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedStats.m_failedPpdus,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedStats.m_receivedMpdus,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedStats.m_failedMpdus,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedStats.m_overlappingPpdus,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedStats.m_nonOverlappingPpdus,
                          "Didn't receive right number of nonoverlapping PPDUs");
}

void
TestWifiPhyRxTraceHelperMloStr::CheckAllStats(uint64_t expectedPpduSuccess,
                                              uint64_t expectedPpduFailure,
                                              uint64_t expectedMpduSuccess,
                                              uint64_t expectedMpduFailure,
                                              uint64_t expectedOverlaps,
                                              uint64_t expectedNonOverlaps)
{
    WifiPhyTraceStatistics stats = m_rxTraceHelper.GetStatistics();

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedPpduSuccess,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedPpduFailure,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedMpduSuccess,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedMpduFailure,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedOverlaps,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedNonOverlaps,
                          "Didn't receive right number of nonoverlapping PPDUs");

    m_rxTraceHelper.Reset();
}

void
TestWifiPhyRxTraceHelperMloStr::CheckRecords(uint64_t expectedRecordCount)
{
    auto records = m_rxTraceHelper.GetPpduRecords();
    NS_TEST_ASSERT_MSG_EQ(records.size(),
                          expectedRecordCount,
                          "Didn't produce the right number of Records");
}

void
TestWifiPhyRxTraceHelperMloStr::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 1;
    dBm_u rxPower{-80};
    streamNumber += m_tx0A->AssignStreams(streamNumber);
    streamNumber += m_tx0B->AssignStreams(streamNumber);
    streamNumber += m_tx1A->AssignStreams(streamNumber);
    streamNumber += m_tx1B->AssignStreams(streamNumber);
    streamNumber += m_rxA->AssignStreams(streamNumber);
    streamNumber += m_rxB->AssignStreams(streamNumber);
    WifiPhyTraceStatistics expectedStats;

    m_rxTraceHelper.Start(Seconds(0.01));

    // CASE 1: PPDU Reception with Sufficient RSSI With SOME Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 2;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 2;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 2;

    // A-MPDU 1
    Simulator::Schedule(Seconds(0.1),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_tx0A);

    Simulator::Schedule(Seconds(0.1),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_tx0B);

    Simulator::Schedule(Seconds(0.19),
                        &TestWifiPhyRxTraceHelperMloStr::CheckRecords,
                        this,
                        4); // Total Records

    Simulator::Schedule(
        Seconds(0.2),
        static_cast<void (TestWifiPhyRxTraceHelperMloStr::*)(WifiPhyTraceStatistics, uint32_t)>(
            &TestWifiPhyRxTraceHelperMloStr::CheckStats),
        this,
        expectedStats,
        wifiNodes.Get(2)->GetId());

    Simulator::Schedule(Seconds(0.2),
                        &TestWifiPhyRxTraceHelperMloStr::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 2: PPDU Reception with Insufficient RSSI With SOME Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 2;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 2;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 2;
    rxPower = dBm_u{-83};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.3),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_tx0A);

    Simulator::Schedule(Seconds(0.3),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_tx0B);

    Simulator::Schedule(Seconds(0.39),
                        &TestWifiPhyRxTraceHelperMloStr::CheckRecords,
                        this,
                        4); // TotalRecords

    Simulator::Schedule(
        Seconds(0.4),
        static_cast<void (TestWifiPhyRxTraceHelperMloStr::*)(WifiPhyTraceStatistics, uint32_t)>(
            &TestWifiPhyRxTraceHelperMloStr::CheckStats),
        this,
        expectedStats,
        wifiNodes.Get(2)->GetId());

    Simulator::Schedule(Seconds(0.4),
                        &TestWifiPhyRxTraceHelperMloStr::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 3: PPDU Reception with Sufficient RSSI/SNR With NO Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.5),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_tx0A);

    // A-MPDU 1
    Simulator::Schedule(Seconds(0.5),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_tx0B);

    Simulator::Schedule(Seconds(0.59),
                        &TestWifiPhyRxTraceHelperMloStr::CheckRecords,
                        this,
                        4); // TotalRecords

    Simulator::Schedule(
        Seconds(0.6),
        static_cast<void (TestWifiPhyRxTraceHelperMloStr::*)(WifiPhyTraceStatistics, uint32_t)>(
            &TestWifiPhyRxTraceHelperMloStr::CheckStats),
        this,
        expectedStats,
        wifiNodes.Get(2)->GetId());

    Simulator::Schedule(Seconds(0.6),
                        &TestWifiPhyRxTraceHelperMloStr::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 4: PPDU Reception with Insufficient RSSI/SNR With NO Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-83};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.7),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_tx0A);

    // A-MPDU 1
    Simulator::Schedule(Seconds(0.7),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_tx0B);

    Simulator::Schedule(Seconds(0.79),
                        &TestWifiPhyRxTraceHelperMloStr::CheckRecords,
                        this,
                        4); // TotalRecords

    Simulator::Schedule(
        Seconds(0.8),
        static_cast<void (TestWifiPhyRxTraceHelperMloStr::*)(WifiPhyTraceStatistics, uint32_t)>(
            &TestWifiPhyRxTraceHelperMloStr::CheckStats),
        this,
        expectedStats,
        wifiNodes.Get(2)->GetId());

    Simulator::Schedule(Seconds(0.8),
                        &TestWifiPhyRxTraceHelperMloStr::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 5: PPDU Overlapping Reception with sufficient RSSI/SNR With ALL Frames Addressed to
    // Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 4;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 4;
    expectedStats.m_overlappingPpdus = 4;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_tx0A);
    // A-MPDU 2
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_tx1A);

    // A-MPDU 1
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_tx0B);
    // A-MPDU 2
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_tx1B);

    Simulator::Schedule(Seconds(0.99),
                        &TestWifiPhyRxTraceHelperMloStr::CheckRecords,
                        this,
                        8); // TotalRecords

    Simulator::Schedule(
        Seconds(1),
        static_cast<void (TestWifiPhyRxTraceHelperMloStr::*)(WifiPhyTraceStatistics, uint32_t)>(
            &TestWifiPhyRxTraceHelperMloStr::CheckStats),
        this,
        expectedStats,
        wifiNodes.Get(2)->GetId());

    Simulator::Schedule(Seconds(1),
                        &TestWifiPhyRxTraceHelperMloStr::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 6: PPDU Overlapping Reception with sufficient RSSI/SNR With SOME Frames Addressed to
    // Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 2;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 2;
    expectedStats.m_overlappingPpdus = 2;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_tx0A);
    // A-MPDU 2
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        m_tx1A);

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_tx0B);
    // A-MPDU 2
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelperMloStr::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        m_tx1B);

    Simulator::Schedule(Seconds(1.19),
                        &TestWifiPhyRxTraceHelperMloStr::CheckRecords,
                        this,
                        8); // TotalRecords

    Simulator::Schedule(
        Seconds(1.2),
        static_cast<void (TestWifiPhyRxTraceHelperMloStr::*)(WifiPhyTraceStatistics, uint32_t)>(
            &TestWifiPhyRxTraceHelperMloStr::CheckStats),
        this,
        expectedStats,
        wifiNodes.Get(2)->GetId());

    Simulator::Schedule(Seconds(1.2),
                        &TestWifiPhyRxTraceHelperMloStr::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    m_rxTraceHelper.Stop(Seconds(1.21));

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @brief Implements a test case to evaluate the reception process of WiFi Physical Layer (PHY)
 * frames (PPDU) with multiple MAC Protocol Data Units (MPDUs) when using YansWifiPhy.
 *
 * This class extends the TestCase class to simulate and analyze the reception of PPDUs containing
 * either one or two MPDUs. It specifically tests the PHY layer's capability to handle multiple
 * MPDUs addressed to specific receivers, measuring success and failure rates. The simulation setup
 * includes configuring transmit and receive power levels, and the class provides functionality to
 * check statistics related to PPDU and MPDU reception success and failure, as well as overlap
 * handling.
 */
class TestWifiPhyRxTraceHelperYans : public TestCase
{
  public:
    /**
     * Constructs a TestWifiPhyRxTraceHelperYans instance for testing the trace helper with
     * Yans.
     */
    TestWifiPhyRxTraceHelperYans();

  private:
    void DoSetup() override;
    void DoRun() override;

    void DoTeardown() override;

    /**
     * Sends a PPDU containing two MPDUs addressed to specific receivers.
     * @param rxPower The transmit power.
     * @param add1 The MAC address of the first receiver.
     * @param add2 The MAC address of the second receiver.
     * @param tx_phy The transmitting PHY object.
     */
    void SendPpduWithTwoMpdus(dBm_u rxPower,
                              Mac48Address add1,
                              Mac48Address add2,
                              Ptr<ns3::YansWifiPhy> tx_phy);

    /**
     * Sends a PPDU containing one MPDU addressed to a specific receiver.
     * @param rxPower The transmit power.
     * @param add1 The MAC address of the receiver.
     * @param tx_phy The transmitting PHY object.
     */
    void SendPpduWithOneMpdu(dBm_u rxPower, Mac48Address add1, Ptr<ns3::YansWifiPhy> tx_phy);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedPpduSuccess The expected number of successful PPDU receptions.
     * @param expectedPpduFailure The expected number of failed PPDU receptions.
     * @param expectedMpduSuccess The expected number of successful MPDU receptions within PPDUs.
     * @param expectedMpduFailure The expected number of failed MPDU receptions within PPDUs.
     * @param expectedOverlaps The expected number of overlapping reception events.
     * @param expectedNonOverlaps The expected number of non-overlapping reception events.
     */
    void CheckAllStats(uint64_t expectedPpduSuccess,
                       uint64_t expectedPpduFailure,
                       uint64_t expectedMpduSuccess,
                       uint64_t expectedMpduFailure,
                       uint64_t expectedOverlaps,
                       uint64_t expectedNonOverlaps);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedStats The expected counts for PPDU and MPDU reception.
     * @param nodeId The node ID for which to check the received stats.
     * @param deviceId The device ID for which to check the received stats.
     * @param linkId The link ID for which to check the received stats.
     */
    void CheckStats(WifiPhyTraceStatistics expectedStats,
                    uint32_t nodeId,
                    uint32_t deviceId,
                    uint32_t linkId);

    /**
     * Checks the statistics of PPDU and MPDU reception success and failure.
     * @param expectedRecordCount The expected number of records.
     */
    void CheckRecords(uint64_t expectedRecordCount);

    Mac48Address wrongReceiver =
        Mac48Address("00:00:00:00:00:01"); ///< The MAC address representing an incorrect receiver,
                                           ///< used for testing.
    Mac48Address correctReceiver =
        Mac48Address("00:00:00:00:00:03"); ///< The MAC address representing the correct receiver,
                                           ///< used for testing.

    std::map<ns3::Mac48Address, uint32_t> MacToNodeId = {
        {Mac48Address("00:00:00:00:00:04"), 0},
        {Mac48Address("00:00:00:00:00:05"), 1},
        {correctReceiver, 2},
        {wrongReceiver, 3}}; ///< Maps MAC addresses to node IDs. This is done since there is no MAC
                             ///< layer configured and therefore the helper cannot obtain the MAC
                             ///< addresses automatically.

    Ptr<Node> nodeRx;                ///< The receiving node
    Ptr<YansWifiPhy> m_txA{nullptr}; ///< The transmit function for node A.
    Ptr<YansWifiPhy> m_txB{nullptr}; ///< The transmit function for node B.
    Ptr<YansWifiPhy> m_rx{nullptr};  ///< The receive function for testing.

    Ptr<FixedRssLossModel> propLoss; ///< The propagation loss model used to configure RSSI

    WifiPhyRxTraceHelper
        m_rxTraceHelper; ///< The helper being tested for tracking PHY reception events.

    uint64_t m_uid{0}; //!< The unique identifier used for the PPDU in the test.
};

TestWifiPhyRxTraceHelperYans::TestWifiPhyRxTraceHelperYans()
    : TestCase("Test for correct operation when using Yans")
{
}

void
TestWifiPhyRxTraceHelperYans::DoSetup()
{
    dBm_u txPower{20};

    auto yansChannel = CreateObject<YansWifiChannel>();
    auto propDelay = CreateObject<ConstantSpeedPropagationDelayModel>();
    propLoss = CreateObject<FixedRssLossModel>();
    yansChannel->SetPropagationDelayModel(propDelay);
    yansChannel->SetPropagationLossModel(propLoss);

    auto nodeA = CreateObject<Node>();
    auto devA = CreateObject<WifiNetDevice>();
    m_txA = CreateObject<YansWifiPhy>();
    m_txA->SetDevice(devA);
    m_txA->SetTxPowerStart(txPower);
    m_txA->SetTxPowerEnd(txPower);

    auto nodeB = CreateObject<Node>();
    auto devB = CreateObject<WifiNetDevice>();
    m_txB = CreateObject<YansWifiPhy>();
    m_txB->SetDevice(devB);
    m_txB->SetTxPowerStart(txPower);
    m_txB->SetTxPowerEnd(txPower);

    nodeRx = CreateObject<Node>();
    auto devRx = CreateObject<WifiNetDevice>();
    m_rx = CreateObject<YansWifiPhy>();
    m_rx->SetDevice(devRx);

    auto interferenceTxA = CreateObject<InterferenceHelper>();
    m_txA->SetInterferenceHelper(interferenceTxA);
    auto errorTxA = CreateObject<NistErrorRateModel>();
    m_txA->SetErrorRateModel(errorTxA);

    auto interferenceTxB = CreateObject<InterferenceHelper>();
    m_txB->SetInterferenceHelper(interferenceTxB);
    auto errorTxB = CreateObject<NistErrorRateModel>();
    m_txB->SetErrorRateModel(errorTxB);

    auto interferenceRx = CreateObject<InterferenceHelper>();
    m_rx->SetInterferenceHelper(interferenceRx);
    auto errorRx = CreateObject<NistErrorRateModel>();
    m_rx->SetErrorRateModel(errorRx);

    m_txA->SetChannel(yansChannel);
    m_txB->SetChannel(yansChannel);
    m_rx->SetChannel(yansChannel);

    m_txA->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_txB->ConfigureStandard(WIFI_STANDARD_80211ax);
    m_rx->ConfigureStandard(WIFI_STANDARD_80211ax);

    m_txA->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});
    m_txB->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});
    m_rx->SetOperatingChannel(WifiPhy::ChannelTuple{36, 20, WIFI_PHY_BAND_5GHZ, 0});

    devA->SetPhy(m_txA);
    nodeA->AddDevice(devA);
    devB->SetPhy(m_txB);
    nodeB->AddDevice(devB);
    devRx->SetPhy(m_rx);
    nodeRx->AddDevice(devRx);

    auto mobilityA = CreateObject<ConstantPositionMobilityModel>();
    mobilityA->SetPosition(Vector(0.0, 0.0, 0.0));
    nodeA->AggregateObject(mobilityA);

    auto mobilityB = CreateObject<ConstantPositionMobilityModel>();
    mobilityB->SetPosition(Vector(0.0, 0.0, 0.0));
    nodeB->AggregateObject(mobilityB);

    auto mobilityRx = CreateObject<ConstantPositionMobilityModel>();
    mobilityRx->SetPosition(Vector(0.0, 0.0, 0.0));
    nodeRx->AggregateObject(mobilityRx);

    NodeContainer nodes;
    nodes.Add(nodeA);
    nodes.Add(nodeB);
    nodes.Add(nodeRx);

    m_rxTraceHelper.Enable(nodes, MacToNodeId);

    auto preambleDetectionModel = CreateObject<ThresholdPreambleDetectionModel>();
    preambleDetectionModel->SetAttribute("Threshold", DoubleValue(4));
    preambleDetectionModel->SetAttribute("MinimumRssi", DoubleValue(-82));
    m_rx->SetPreambleDetectionModel(preambleDetectionModel);
}

void
TestWifiPhyRxTraceHelperYans::DoTeardown()
{
    m_txA->Dispose();
    m_txB->Dispose();
    m_rx->Dispose();
}

void
TestWifiPhyRxTraceHelperYans::SendPpduWithTwoMpdus(dBm_u rxPower,
                                                   Mac48Address receiver1,
                                                   Mac48Address receiver2,
                                                   Ptr<ns3::YansWifiPhy> tx_phy)
{
    propLoss->SetRss(rxPower);

    WifiTxVector txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                         0,
                                         WIFI_PREAMBLE_HE_SU,
                                         NanoSeconds(800),
                                         1,
                                         1,
                                         0,
                                         MHz_u{20},
                                         true);

    std::vector<Ptr<WifiMpdu>> mpduList;

    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(receiver1); // Changing the expected receiver
    auto p1 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p1, hdr1));

    WifiMacHeader hdr2;
    hdr2.SetType(WIFI_MAC_QOSDATA);
    hdr2.SetQosTid(0);
    hdr2.SetAddr1(receiver2); // Changing the expected receiver
    auto p2 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p2, hdr2));

    auto psdu = Create<WifiPsdu>(mpduList);
    auto ppdu = Create<HePpdu>(
        psdu,
        txVector,
        tx_phy->GetOperatingChannel(),
        YansWifiPhy::CalculateTxDuration(psdu->GetSize(), txVector, tx_phy->GetPhyBand()),
        m_uid);

    m_uid++;

    tx_phy->StartTx(ppdu);
}

void
TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu(dBm_u rxPower,
                                                  Mac48Address receiver1,
                                                  Ptr<ns3::YansWifiPhy> tx_phy)
{
    propLoss->SetRss(rxPower);

    auto txVector = WifiTxVector(HePhy::GetHeMcs0(),
                                 0,
                                 WIFI_PREAMBLE_HE_SU,
                                 NanoSeconds(800),
                                 1,
                                 1,
                                 0,
                                 MHz_u{20},
                                 true);

    std::vector<Ptr<WifiMpdu>> mpduList;

    WifiMacHeader hdr1;
    hdr1.SetType(WIFI_MAC_QOSDATA);
    hdr1.SetQosTid(0);
    hdr1.SetAddr1(receiver1); // Changing the expected receiver
    auto p1 = Create<Packet>(750);
    mpduList.emplace_back(Create<WifiMpdu>(p1, hdr1));

    auto psdu = Create<WifiPsdu>(mpduList);
    auto ppdu = Create<HePpdu>(
        psdu,
        txVector,
        tx_phy->GetOperatingChannel(),
        YansWifiPhy::CalculateTxDuration(psdu->GetSize(), txVector, tx_phy->GetPhyBand()),
        m_uid);

    m_uid++;

    tx_phy->StartTx(ppdu);
}

void
TestWifiPhyRxTraceHelperYans::CheckAllStats(uint64_t expectedPpduSuccess,
                                            uint64_t expectedPpduFailure,
                                            uint64_t expectedMpduSuccess,
                                            uint64_t expectedMpduFailure,
                                            uint64_t expectedOverlaps,
                                            uint64_t expectedNonOverlaps)
{
    auto stats = m_rxTraceHelper.GetStatistics();

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedPpduSuccess,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedPpduFailure,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedMpduSuccess,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedMpduFailure,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedOverlaps,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedNonOverlaps,
                          "Didn't receive right number of nonoverlapping PPDUs");

    m_rxTraceHelper.Reset();
}

void
TestWifiPhyRxTraceHelperYans::CheckStats(WifiPhyTraceStatistics expectedStats,
                                         uint32_t nodeId,
                                         uint32_t deviceId,
                                         uint32_t linkId)
{
    WifiPhyTraceStatistics stats = m_rxTraceHelper.GetStatistics(nodeId, deviceId, linkId);

    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedPpdus,
                          expectedStats.m_receivedPpdus,
                          "Didn't receive right number of successful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedPpdus,
                          expectedStats.m_failedPpdus,
                          "Didn't receive right number of unsuccessful PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_receivedMpdus,
                          expectedStats.m_receivedMpdus,
                          "Didn't receive right number of successful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_failedMpdus,
                          expectedStats.m_failedMpdus,
                          "Didn't receive right number of unsuccessful MPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_overlappingPpdus,
                          expectedStats.m_overlappingPpdus,
                          "Didn't receive right number of overlapping PPDUs");
    NS_TEST_ASSERT_MSG_EQ(stats.m_nonOverlappingPpdus,
                          expectedStats.m_nonOverlappingPpdus,
                          "Didn't receive right number of nonoverlapping PPDUs");
}

void
TestWifiPhyRxTraceHelperYans::CheckRecords(uint64_t expectedRecordCount)
{
    auto records = m_rxTraceHelper.GetPpduRecords();
    NS_TEST_ASSERT_MSG_EQ(records.size(),
                          expectedRecordCount,
                          "Didn't produce the right number of Records");
}

void
TestWifiPhyRxTraceHelperYans::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 1;
    dBm_u rxPower{-80};
    streamNumber += m_txA->AssignStreams(streamNumber);
    streamNumber += m_txB->AssignStreams(streamNumber);
    streamNumber += m_rx->AssignStreams(streamNumber);
    WifiPhyTraceStatistics expectedStats;

    m_rxTraceHelper.Start(Seconds(0.01));

    // CASE 1: PPDU Reception with Sufficient RSSI With SOME Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 1;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 1;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;

    // A-MPDU 1
    Simulator::Schedule(Seconds(0.1),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.19),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        2); // Total Records

    Simulator::Schedule(Seconds(0.2),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.2),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);
    // CASE 2: PPDU Reception with inSufficient RSSI With SOME Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 1;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 1;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;
    rxPower = dBm_u{-83};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.3),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        wrongReceiver,   // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.39),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(0.4),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.4),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 3: PPDU Reception with Sufficient RSSI/SNR With NO Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.5),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.59),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(0.6),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.6),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 4: PPDU Reception with Insufficient RSSI/SNR With NO Frames Addressed to Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-83};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.7),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithTwoMpdus,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        wrongReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(0.79),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(0.8),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(0.8),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 5: PPDU Overlapping Reception with sufficient RSSI/SNR With ALL Frames Addressed to
    // Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 2;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 2;
    expectedStats.m_overlappingPpdus = 2;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);
    // A-MPDU 2
    Simulator::Schedule(Seconds(0.9),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txB);

    Simulator::Schedule(Seconds(0.99),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        4); // TotalRecords

    Simulator::Schedule(Seconds(1),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 6: PPDU Overlapping Reception with sufficient RSSI/SNR With SOME Frames Addressed to
    // Receiver
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 1;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 1;
    expectedStats.m_overlappingPpdus = 1;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};
    // A-MPDU 1
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);
    // A-MPDU 2
    Simulator::Schedule(Seconds(1.1),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        wrongReceiver, // One MPDU addressed to
                        m_txB);

    Simulator::Schedule(Seconds(1.19),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        4); // TotalRecords

    Simulator::Schedule(Seconds(1.2),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.2),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // Stop Statistics Collection Period Since following testcases evaluate the Start and Stop
    // methods
    m_rxTraceHelper.Stop(Seconds(1.21));

    // CASE 7: Execution of "Start()" Before Signal Injection
    expectedStats.m_receivedPpdus = 1;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 1;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;
    rxPower = dBm_u{-80};
    m_rxTraceHelper.Start(Seconds(1.29));

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.3),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(1.39),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(1.4),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.4),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    m_rxTraceHelper.Stop(Seconds(1.4));

    // CASE 8: Activation of "Start()" Followed by "Stop()" Before Signal Injection
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};

    m_rxTraceHelper.Start(Seconds(1.41));

    m_rxTraceHelper.Stop(Seconds(1.42));

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.45),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    Simulator::Schedule(Seconds(1.549),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        0); // TotalRecords

    Simulator::Schedule(Seconds(1.55),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.55),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    // CASE 9: "Start()" Method Initiated During Ongoing PPDU Reception
    expectedStats.m_receivedPpdus = 1;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 1;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 1;
    rxPower = dBm_u{-80};

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.6),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    m_rxTraceHelper.Start(Seconds(1.6) + MicroSeconds(10));

    Simulator::Schedule(Seconds(1.69),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        2); // TotalRecords

    Simulator::Schedule(Seconds(1.7),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.7),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    m_rxTraceHelper.Stop(Seconds(1.7));

    // CASE 10: Execution of "Stop()" During Ongoing PPDU Reception
    expectedStats.m_receivedPpdus = 0;
    expectedStats.m_failedPpdus = 0;
    expectedStats.m_receivedMpdus = 0;
    expectedStats.m_failedMpdus = 0;
    expectedStats.m_overlappingPpdus = 0;
    expectedStats.m_nonOverlappingPpdus = 0;
    rxPower = dBm_u{-80};

    m_rxTraceHelper.Start(Seconds(1.79));

    // A-MPDU 1
    Simulator::Schedule(Seconds(1.8),
                        &TestWifiPhyRxTraceHelperYans::SendPpduWithOneMpdu,
                        this,
                        rxPower,
                        correctReceiver, // One MPDU addressed to
                        m_txA);

    m_rxTraceHelper.Stop(Seconds(1.8) + MicroSeconds(10));

    Simulator::Schedule(Seconds(1.89),
                        &TestWifiPhyRxTraceHelperYans::CheckRecords,
                        this,
                        0); // TotalRecords

    Simulator::Schedule(Seconds(1.9),
                        &TestWifiPhyRxTraceHelperYans::CheckStats,
                        this,
                        expectedStats,
                        nodeRx->GetId(),
                        0,
                        0);

    Simulator::Schedule(Seconds(1.9),
                        &TestWifiPhyRxTraceHelperYans::CheckAllStats,
                        this,
                        expectedStats.m_receivedPpdus,
                        expectedStats.m_failedPpdus,
                        expectedStats.m_receivedMpdus,
                        expectedStats.m_failedMpdus,
                        expectedStats.m_overlappingPpdus,
                        expectedStats.m_nonOverlappingPpdus);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi PHY reception Test Suite
 */
class WifiPhyRxTraceHelperTestSuite : public TestSuite
{
  public:
    WifiPhyRxTraceHelperTestSuite();
};

WifiPhyRxTraceHelperTestSuite::WifiPhyRxTraceHelperTestSuite()
    : TestSuite("wifi-phy-rx-trace-helper", Type::UNIT)
{
    AddTestCase(new TestWifiPhyRxTraceHelper("test-statistics"), TestCase::Duration::QUICK);
    AddTestCase(new TestWifiPhyRxTraceHelperMloStr, TestCase::Duration::QUICK);
    AddTestCase(new TestWifiPhyRxTraceHelperYans, TestCase::Duration::QUICK);
}

static WifiPhyRxTraceHelperTestSuite wifiPhyRxTestSuite; ///< the test suite
