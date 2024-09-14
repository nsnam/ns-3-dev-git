/*
 * Copyright (c) 2024 Huazhong University of Science and Technology
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Muyuan Shen <muyuan@uw.edu>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/eht-configuration.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/qos-utils.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-tx-stats-helper.h"

#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiTxStatsHelperTest");

/**
 * @ingroup wifi-test
 * @brief Implements a test case to evaluate the transmission process of multiple Wi-Fi
 * MAC Layer MPDUs. The testcase has two options.
 * 1) SINGLE_LINK_NON_QOS: test the handling of regular ACKs.
 * 2) MULTI_LINK_QOS: test the handling of MPDU aggregation, Block ACKs, and Multi-Link Operation.
 *
 * To observe the operation of WifiTxStatsHelper, the test can be run from the command line as
 * follows:
 * @code
 *   NS_LOG="WifiTxStatsHelper=level_info|prefix_all" ./ns3 run 'test-runner
 * --suite=wifi-tx-stats-helper'
 * @endcode
 */
class WifiTxStatsHelperTest : public TestCase
{
  public:
    /**
     * Option for the test
     */
    enum TestOption : uint8_t
    {
        SINGLE_LINK_NON_QOS,
        MULTI_LINK_QOS
    };

    /**
     * Constructor
     * @param testName Test name
     * @param option Test option
     */
    WifiTxStatsHelperTest(const std::string& testName, TestOption option);

    /**
     * Callback invoked when PHY starts transmission of a PSDU, used to record TX start
     * time and TX duration.
     *
     * @param context the context
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPower the tx power in Watts
     */
    void Transmit(std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  Watt_u txPower);

  private:
    TestOption m_option;          //!< Test option
    NodeContainer m_wifiApNode;   //!< NodeContainer for AP
    NodeContainer m_wifiStaNodes; //!< NodeContainer for STAs
    int64_t m_streamNumber;       //!< Random variable stream number
    Time m_sifs;                  //!< SIFS time
    Time m_slot;                  //!< slot time

    Time m_difs; //!< DIFS time (for SINGLE_LINK_NON_QOS case only)
    std::map<uint8_t, std::vector<Time>> m_txStartTimes; //!< Map of independently obtain vector of
                                                         //!< PhyTxBegin trace, indexed per link
    std::map<uint8_t, std::vector<Time>>
        m_durations;                      //!< Map of vector of MPDU durations, indexed per link
    std::map<uint8_t, uint32_t> m_cwMins; //!< Map of CW Mins, indexed per link
    std::map<uint8_t, uint32_t>
        m_aifsns; //!< Map of AIFSNs, indexed per link (for MULTI_LINK_QOS case only)
    std::map<uint8_t, Time>
        m_aifss; //!< Map of AIFSs, indexed per link (for MULTI_LINK_QOS case only)

    void DoSetup() override;
    void DoRun() override;
    /**
     * Check correctness of test
     * @param wifiTxStats Reference to the helper
     */
    void CheckResults(const WifiTxStatsHelper& wifiTxStats);
};

WifiTxStatsHelperTest::WifiTxStatsHelperTest(const std::string& testName, TestOption option)
    : TestCase(testName),
      m_option(option),
      m_streamNumber(100)
{
}

void
WifiTxStatsHelperTest::Transmit(std::string context,
                                WifiConstPsduMap psduMap,
                                WifiTxVector txVector,
                                Watt_u txPower)
{
    const auto linkId = atoi(context.c_str());
    if (linkId == 0)
    {
        m_txStartTimes[0].push_back(Simulator::Now());
        m_durations[0].push_back(
            WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_5GHZ));
    }
    else if (linkId == 1)
    {
        m_txStartTimes[1].push_back(Simulator::Now());
        m_durations[1].push_back(
            WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_6GHZ));
    }
}

void
WifiTxStatsHelperTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(40);

    m_wifiApNode.Create(1);
    m_wifiStaNodes.Create(1);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_wifiApNode);
    mobility.Install(m_wifiStaNodes);

    PacketSocketHelper packetSocket;
    packetSocket.Install(m_wifiApNode);
    packetSocket.Install(m_wifiStaNodes);
}

void
WifiTxStatsHelperTest::DoRun()
{
    std::string dataMode;
    std::string ackMode;
    if (m_option == SINGLE_LINK_NON_QOS)
    {
        dataMode = "OfdmRate12Mbps";
        ackMode = "OfdmRate6Mbps";
    }
    else
    {
        dataMode = "EhtMcs6";
        ackMode = "OfdmRate54Mbps";
    }

    WifiHelper wifi;
    NetDeviceContainer staDevices;
    NetDeviceContainer apDevices;
    if (m_option == SINGLE_LINK_NON_QOS)
    {
        wifi.SetStandard(WIFI_STANDARD_80211a);
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                     "DataMode",
                                     StringValue(dataMode),
                                     "ControlMode",
                                     StringValue(ackMode));
        auto spectrumChannel = CreateObject<SingleModelSpectrumChannel>();
        auto lossModel = CreateObject<FriisPropagationLossModel>();
        spectrumChannel->AddPropagationLossModel(lossModel);
        auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
        spectrumChannel->SetPropagationDelayModel(delayModel);

        SpectrumWifiPhyHelper phy;
        phy.SetChannel(spectrumChannel);

        WifiMacHelper mac;
        mac.SetType("ns3::StaWifiMac",
                    "QosSupported",
                    BooleanValue(false),
                    "Ssid",
                    SsidValue(Ssid("test-ssid")));
        staDevices = wifi.Install(phy, mac, m_wifiStaNodes);

        mac.SetType("ns3::ApWifiMac",
                    "QosSupported",
                    BooleanValue(false),
                    "Ssid",
                    SsidValue(Ssid("test-ssid")),
                    "BeaconInterval",
                    TimeValue(MicroSeconds(102400)),
                    "EnableBeaconJitter",
                    BooleanValue(false));
        apDevices = wifi.Install(phy, mac, m_wifiApNode);
    }
    else
    {
        wifi.SetStandard(WIFI_STANDARD_80211be);
        // Get channel string for MLD STA
        std::array<std::string, 2> mldChannelStr;
        uint32_t frequency = 5;
        uint32_t frequency2 = 6;
        for (auto freq : {frequency, frequency2})
        {
            NS_TEST_ASSERT_MSG_EQ((freq == 5 || freq == 6), true, "Unsupported frequency for BSS");
            if (freq == 6)
            {
                mldChannelStr[1] = "{0, 20, BAND_6GHZ, 0}";
                wifi.SetRemoteStationManager(static_cast<uint8_t>(1),
                                             "ns3::ConstantRateWifiManager",
                                             "DataMode",
                                             StringValue(dataMode),
                                             "ControlMode",
                                             StringValue(ackMode));
            }
            else
            {
                mldChannelStr[0] = "{0, 20, BAND_5GHZ, 0}";
                wifi.SetRemoteStationManager(static_cast<uint8_t>(0),
                                             "ns3::ConstantRateWifiManager",
                                             "DataMode",
                                             StringValue(dataMode),
                                             "ControlMode",
                                             StringValue(ackMode));
            }
        }

        SpectrumWifiPhyHelper phy(2);

        auto lossModel = CreateObject<LogDistancePropagationLossModel>();
        auto spectrumChannel1 = CreateObject<MultiModelSpectrumChannel>();
        spectrumChannel1->AddPropagationLossModel(lossModel);
        auto spectrumChannel2 = CreateObject<MultiModelSpectrumChannel>();
        spectrumChannel2->AddPropagationLossModel(lossModel);

        phy.AddChannel(spectrumChannel1, WIFI_SPECTRUM_5_GHZ);
        phy.AddChannel(spectrumChannel2, WIFI_SPECTRUM_6_GHZ);

        for (uint8_t linkId = 0; linkId < 2; ++linkId)
        {
            phy.Set(linkId, "ChannelSettings", StringValue(mldChannelStr[linkId]));
        }

        WifiMacHelper mac;
        mac.SetType("ns3::StaWifiMac",
                    "QosSupported",
                    BooleanValue(true),
                    "Ssid",
                    SsidValue(Ssid("test-ssid")));
        staDevices = wifi.Install(phy, mac, m_wifiStaNodes);

        mac.SetType("ns3::ApWifiMac",
                    "QosSupported",
                    BooleanValue(true),
                    "Ssid",
                    SsidValue(Ssid("test-ssid")),
                    "BeaconInterval",
                    TimeValue(MicroSeconds(102400)),
                    "EnableBeaconJitter",
                    BooleanValue(false));
        apDevices = wifi.Install(phy, mac, m_wifiApNode);
    }

    m_sifs = DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetPhy()->GetSifs();
    m_slot = DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetPhy()->GetSlot();
    if (m_option == SINGLE_LINK_NON_QOS)
    {
        m_difs = m_sifs + 2 * m_slot;
        m_cwMins[SINGLE_LINK_OP_ID] =
            DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetMac()->GetTxop()->GetMinCw();
    }
    else
    {
        // Use TID-to-link Mapping to tx TID=3 pkts (BE) only on link 0,
        // TID=4 pkts (VI) only on link 1
        m_cwMins[0] = 15;
        m_cwMins[1] = 7;
        m_aifsns[0] = 3;
        m_aifsns[1] = 2;
        m_aifss[0] = m_aifsns[0] * m_slot + m_sifs;
        m_aifss[1] = m_aifsns[1] * m_slot + m_sifs;
        std::string mldMappingStr = "3 0; 4 1";
        DynamicCast<WifiNetDevice>(staDevices.Get(0))
            ->GetMac()
            ->GetEhtConfiguration()
            ->SetAttribute("TidToLinkMappingUl", StringValue(mldMappingStr));
    }

    auto streamsUsed = WifiHelper::AssignStreams(apDevices, m_streamNumber);
    NS_ASSERT_MSG(streamsUsed < 100, "Need to increment by larger quantity");
    WifiHelper::AssignStreams(staDevices, m_streamNumber + 100);

    // UL traffic (TX statistics will be installed at STA side)
    PacketSocketAddress socket;
    socket.SetSingleDevice(staDevices.Get(0)->GetIfIndex());
    socket.SetPhysicalAddress(apDevices.Get(0)->GetAddress());
    auto server = CreateObject<PacketSocketServer>();
    server->SetLocal(socket);
    m_wifiApNode.Get(0)->AddApplication(server);
    server->SetStartTime(Seconds(0.0));
    server->SetStopTime(Seconds(1.0));
    if (m_option == SINGLE_LINK_NON_QOS)
    {
        auto client = CreateObject<PacketSocketClient>();
        client->SetAttribute("PacketSize", UintegerValue(1500));
        client->SetAttribute("MaxPackets", UintegerValue(3));
        client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        client->SetRemote(socket);
        m_wifiStaNodes.Get(0)->AddApplication(client);
        client->SetStartTime(MicroSeconds(210000));
        client->SetStopTime(Seconds(1.0));
    }
    else
    {
        auto clientBe = CreateObject<PacketSocketClient>();
        clientBe->SetAttribute("Priority", UintegerValue(3));
        clientBe->SetAttribute("PacketSize", UintegerValue(1500));
        clientBe->SetAttribute("MaxPackets", UintegerValue(3));
        clientBe->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        clientBe->SetRemote(socket);
        m_wifiStaNodes.Get(0)->AddApplication(clientBe);
        clientBe->SetStartTime(MicroSeconds(200000));
        clientBe->SetStopTime(Seconds(1.0));

        auto clientVi = CreateObject<PacketSocketClient>();
        clientVi->SetAttribute("Priority", UintegerValue(4));
        clientVi->SetAttribute("PacketSize", UintegerValue(1500));
        clientVi->SetAttribute("MaxPackets", UintegerValue(3));
        clientVi->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        clientVi->SetRemote(socket);
        m_wifiStaNodes.Get(0)->AddApplication(clientVi);
        clientVi->SetStartTime(MicroSeconds(300000));
        clientVi->SetStopTime(Seconds(1.0));
    }

    // Add AP side receiver corruption
    if (m_option == SINGLE_LINK_NON_QOS)
    {
        // We corrupt AP side reception so that:
        // 1) the 2nd data frame is retransmitted and succeeds (1 failure, 1 success)
        // 2) the 3rd data frame is transmitted 7 times (=FrameRetryLimit) and finally fails (7
        // failures, 0 success)
        //
        // No. of pkt       |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8 |
        // No. recvd by AP  |       |       |   0   |       |       |   1   |       |   2   | | AP's
        // pkts        |  Bea  |  Bea  |       |  Ack  | AsRes |       |  Bea  |       | Ack1  |
        // STA's pkts       |       |       | AsReq |       |       |  Ack  |       | Data1 | |
        //
        // No. of pkt       |   9   |  10   |  11   |  12   |  13   |  ...  |  18   |  19   |  ...
        // No. recvd by AP  | 3 (x) |   4   |       | 5 (x) | 6 (x) |  ...  |11 (x) |       |  ...
        // AP's pkts        |       |       | Ack2  |       |       |  ...  |       |  Bea  |  ...
        // STA's pkts       | Data2 | Data2 |       | Data3 | Data3 |  ...  | Data3 |       |  ...
        //
        // Legend:
        // Bea = Beacon, AsReq = Association Request, AsRes = Association Response
        // AP side corruption is indicated with (x)

        auto apPem = CreateObject<ReceiveListErrorModel>();
        apPem->SetList({3, 5, 6, 7, 8, 9, 10, 11});
        DynamicCast<WifiNetDevice>(apDevices.Get(0))
            ->GetMac()
            ->GetWifiPhy()
            ->SetPostReceptionErrorModel(apPem);
    }
    else
    {
        // We corrupt AP side reception so that:
        // On Link 0 (contains uplink data with TID = 3):
        // 1) the 2nd data frame is retransmitted once and succeeds (retransmission = 1)
        // 2) the 3rd data frame is transmitted 2 times within A-MPDU and 7 times alone
        // (WifiMac::FrameRetryLimit) and finally fails (retransmission = 8)
        //
        // No. of PSDU      |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8 |
        // No. recvd by AP  |       |       |   0   |       |   1   |       |   2   |       |   3 |
        // AP's pkts        |  Bea  |  Bea  |       |  Ack  |       | AsRes |       | CfEnd | |
        // STA's pkts       |       |       | AsReq |       | CfEnd |       |  Ack  |       | ABReq
        // |
        //
        // No. of PSDU      |   9   |  10   |  11   |  12   |  12   |  12   |  13   |  14   |  14 |
        // No. recvd by AP  |       |       |   4   |   5   | 6(x)  | 7(x)  |       |   8   | 9(x) |
        // AP's pkts        |  Ack  | ABRes |       |       |       |       | BAck  |       | |
        // STA's pkts       |       |       |  Ack  | Data1 | Data2 | Data3 |       | Data2 | Data3
        // |
        //
        // No. of PSDU      |  15   |  16   |  ...  |       |  ...  |  23   |  24   |  25   |  ...
        // No. recvd by AP  |       | 10(x) |  ...  |       |  ...  | 16(x) |  17   |       |  ...
        // AP's pkts        | BAck  |       |  ...  |  Bea  |  ...  |       |       | BAck  |  ...
        // STA's pkts       |       | Data3 |  ...  |       |  ...  | Data3 |  Bar  |       |  ...
        //
        // On Link 1 (contains uplink data with TID = 4):
        // 1) the 2nd data frame is transmitted 2 times within A-MPDU and 7 times alone
        // (=WifiMac::FrameRetryLimit) and finally fails (retransmission = 8)
        // 2) the 3rd data frame is
        // retransmitted once and succeeds (retransmission = 1)
        //
        // No. of PSDU      |   0   |   1   |   2   |   3   |   4   |   5   |   6   |   7   |   8 |
        // No. recvd by AP  |       |       |   0   |       |       |   1   |       |   2   | | AP's
        // pkts        |  Bea  |  Bea  |       |  Ack  |  Bea  |       |  Ack  |       | ABRes |
        // STA's pkts       |       |       | Null  |       |       | ABReq |       | CfEnd | |
        //
        // No. of PSDU      |   9   |  10   |  11   |  11   |  11   |  12   |  13   |  13   |  14 |
        // No. recvd by AP  |   3   |       |   4   | 5(x)  | 6(x)  |       | 7(x)  |   8   | | AP's
        // pkts        |       | CfEnd |       |       |       | BAck  |       |       | BAck  |
        // STA's pkts       |  Ack  |       | Data1 | Data2 | Data3 |       | Data2 | Data3 | |
        //
        // No. of PSDU      |  15   |  ...  |  21   |  22   |  23   |  24   |  ...
        // No. recvd by AP  | 9(x)  |  ...  | 15(x) |  16   |       |  17   |  ...
        // AP's pkts        |       |  ...  |       |       | BAck  |       |  ...
        // STA's pkts       | Data2 |  ...  | Data2 |  Bar  |       | CfEnd |  ...
        //
        // Legend:
        // Bea = Beacon, AsReq = Association Request, AsRes = Association Response
        // ABReq = Add Block ACK Request, ABRes = Add Block ACK Response
        // Bar = Block ACK Request (used to notify the discarded MPDU)
        // CfEnd = CF-End, BAck = Block ACK (Response), Null = Null function
        // AP side corruption is indicated with (x)

        // Force drops on link 0 at AP
        auto apPem0 = CreateObject<ReceiveListErrorModel>();
        apPem0->SetList({6, 7, 9, 10, 11, 12, 13, 14, 15, 16});
        DynamicCast<WifiNetDevice>(apDevices.Get(0))
            ->GetMac()
            ->GetWifiPhy(0)
            ->SetPostReceptionErrorModel(apPem0);

        // Force drops on link 1 at AP
        auto apPem1 = CreateObject<ReceiveListErrorModel>();
        apPem1->SetList({5, 6, 7, 9, 10, 11, 12, 13, 14, 15});
        DynamicCast<WifiNetDevice>(apDevices.Get(0))
            ->GetMac()
            ->GetWifiPhy(1)
            ->SetPostReceptionErrorModel(apPem1);
    }

    NetDeviceContainer allNetDev;
    allNetDev.Add(apDevices);
    allNetDev.Add(staDevices);
    WifiTxStatsHelper wifiTxStats;
    wifiTxStats.Enable(allNetDev);
    wifiTxStats.Start(Seconds(0));
    wifiTxStats.Stop(Seconds(1));

    // Trace PSDU TX at both AP and STA to get start times and durations, including acks
    if (m_option == SINGLE_LINK_NON_QOS)
    {
        for (auto it = allNetDev.Begin(); it != allNetDev.End(); ++it)
        {
            auto dev = DynamicCast<WifiNetDevice>(*it);
            dev->GetPhy()->TraceConnect("PhyTxPsduBegin",
                                        std::to_string(SINGLE_LINK_OP_ID),
                                        // "0"
                                        MakeCallback(&WifiTxStatsHelperTest::Transmit, this));
        }
    }
    else
    {
        for (auto it = allNetDev.Begin(); it != allNetDev.End(); ++it)
        {
            auto dev = DynamicCast<WifiNetDevice>(*it);
            dev->GetPhy(0)->TraceConnect("PhyTxPsduBegin",
                                         "0",
                                         MakeCallback(&WifiTxStatsHelperTest::Transmit, this));
            dev->GetPhy(1)->TraceConnect("PhyTxPsduBegin",
                                         "1",
                                         MakeCallback(&WifiTxStatsHelperTest::Transmit, this));
        }
    }

    Simulator::Stop(Seconds(1));
    Simulator::Run();
    CheckResults(wifiTxStats);
    Simulator::Destroy();
}

void
WifiTxStatsHelperTest::CheckResults(const WifiTxStatsHelper& wifiTxStats)
{
    const auto tolerance = NanoSeconds(50); // due to propagation delay
    // Check both variants of GetSuccesses...()
    const auto successMap = wifiTxStats.GetSuccessesByNodeDevice();
    const auto successMapPerNodeDeviceLink = wifiTxStats.GetSuccessesByNodeDeviceLink();
    const auto failureMap = wifiTxStats.GetFailuresByNodeDevice();
    const auto retransmissionMap = wifiTxStats.GetRetransmissionsByNodeDevice();
    const auto totalSuccesses = wifiTxStats.GetSuccesses();
    const auto totalFailures = wifiTxStats.GetFailures();
    const auto totalRetransmissions = wifiTxStats.GetRetransmissions();
    const auto& successRecords = wifiTxStats.GetSuccessRecords();
    const auto& failureRecords = wifiTxStats.GetFailureRecords();

    uint32_t nodeId = 1;
    uint32_t deviceId = 0;
    auto nodeDeviceTuple = std::make_tuple(nodeId, deviceId);
    auto nodeDeviceLink0Tuple = std::make_tuple(nodeId, deviceId, 0);
    auto nodeDeviceLink1Tuple = std::make_tuple(nodeId, deviceId, 1);

    if (m_option == SINGLE_LINK_NON_QOS)
    {
        const auto totalFailuresDrop =
            wifiTxStats.GetFailures(WifiMacDropReason::WIFI_MAC_DROP_REACHED_RETRY_LIMIT);
        const auto totalFailuresDropMap = wifiTxStats.GetFailuresByNodeDevice(
            WifiMacDropReason::WIFI_MAC_DROP_REACHED_RETRY_LIMIT);

        NS_TEST_ASSERT_MSG_EQ(successMapPerNodeDeviceLink.at(nodeDeviceLink0Tuple),
                              2,
                              "Number of success packets should be 2");
        NS_TEST_ASSERT_MSG_EQ(successMap.at(nodeDeviceTuple),
                              2,
                              "Number of success packets should be 2");
        NS_TEST_ASSERT_MSG_EQ(totalSuccesses, 2, "Number of success packets should be 2");

        NS_TEST_ASSERT_MSG_EQ(retransmissionMap.at(nodeDeviceTuple),
                              1,
                              "Number of retransmitted successful packets should be 1");
        NS_TEST_ASSERT_MSG_EQ(totalRetransmissions,
                              1,
                              "Number of retransmitted successful packets should be 1");

        NS_TEST_ASSERT_MSG_EQ(failureMap.at(nodeDeviceTuple),
                              1,
                              "Number of failed packets should be 1");
        NS_TEST_ASSERT_MSG_EQ(totalFailures, 1, "Number of failed packets (aggregate) should be 1");
        NS_TEST_ASSERT_MSG_EQ(
            totalFailuresDrop,
            1,
            "Number of dropped packets (aggregate) due to retry limit reached should be 1");
        NS_TEST_ASSERT_MSG_EQ(
            totalFailuresDropMap.at(nodeDeviceTuple),
            1,
            "Number of dropped packets (aggregate) due to retry limit reached should be 1");

        auto successRecordIt = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_nodeId,
                              1,
                              "Source node ID of the 1st successful data packet should be 1");
        std::advance(successRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_nodeId,
                              1,
                              "Source node ID of the 2nd successful data packet should be 1");
        auto failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_nodeId,
                              1,
                              "Source node ID of the failed data packet should be 1");

        successRecordIt = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            successRecordIt->m_retransmissions,
            0,
            "The retransmission count of the 1st successful data packet should be 0");
        std::advance(successRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(
            successRecordIt->m_retransmissions,
            1,
            "The retransmission count of the 2nd successful data packet should be 1");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_retransmissions,
                              6,
                              "The retransmission count of the failed data packet should be 6");

        successRecordIt = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet should have been TXed");
        std::advance(successRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet should have been TXed");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The failed data packet should have been TXed");

        successRecordIt = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet should have been acked");
        std::advance(successRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet should have been acked");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_ackTime.IsStrictlyPositive(),
                              false,
                              "The failed data packet should not have been acked");

        successRecordIt = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet should have been dequeued");
        std::advance(successRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet should have been dequeued");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropTime.has_value() &&
                                  failureRecordIt->m_dropTime.value().IsStrictlyPositive(),
                              true,
                              "The failed data packet should have been dequeued");

        successRecordIt = successRecords.at(nodeDeviceLink0Tuple).begin();
        auto successRecordItNext = successRecordIt;
        std::advance(successRecordItNext, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_enqueueTime,
                              successRecordItNext->m_enqueueTime,
                              "Three packets should be enqueued at the same time");
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_enqueueTime,
                              failureRecordIt->m_enqueueTime,
                              "Three packets should be enqueued at the same time");

        successRecordIt = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordIt->m_txStartTime,
                                    successRecordIt->m_enqueueTime,
                                    "Packets should be TXed after enqueued");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordIt->m_txStartTime,
                                    successRecordIt->m_enqueueTime + tolerance +
                                        m_cwMins[SINGLE_LINK_OP_ID] * m_slot,
                                    "Packet backoff slots should not exceed cwMin");
        // Packet start time 7 corresponds to first data packet (prior to this, beacons and assoc)
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_txStartTime,
                              m_txStartTimes[SINGLE_LINK_OP_ID][7],
                              "Wrong TX start time");
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordIt->m_ackTime,
                                    m_txStartTimes[SINGLE_LINK_OP_ID][7] +
                                        m_durations[SINGLE_LINK_OP_ID][7] + m_sifs +
                                        m_durations[SINGLE_LINK_OP_ID][8],
                                    "Wrong Ack reception time");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordIt->m_ackTime,
                                    m_txStartTimes[SINGLE_LINK_OP_ID][7] +
                                        m_durations[SINGLE_LINK_OP_ID][7] + m_sifs +
                                        m_durations[SINGLE_LINK_OP_ID][8] + 2 * tolerance,
                                    "Wrong Ack reception time");
        std::advance(successRecordIt, 1);
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordIt->m_txStartTime,
                                    m_txStartTimes[SINGLE_LINK_OP_ID][8] +
                                        m_durations[SINGLE_LINK_OP_ID][8] + m_difs,
                                    "Packets should be TXed after enqueued");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordIt->m_txStartTime,
                                    m_txStartTimes[SINGLE_LINK_OP_ID][8] +
                                        m_durations[SINGLE_LINK_OP_ID][8] + m_difs + tolerance +
                                        m_cwMins[SINGLE_LINK_OP_ID] * m_slot,
                                    "Packet backoff slots should not exceed cwMin");
        NS_TEST_ASSERT_MSG_EQ(successRecordIt->m_txStartTime,
                              m_txStartTimes[SINGLE_LINK_OP_ID][9],
                              "Wrong TX start time");
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordIt->m_ackTime,
                                    m_txStartTimes[SINGLE_LINK_OP_ID][10] +
                                        m_durations[SINGLE_LINK_OP_ID][10] + m_sifs +
                                        m_durations[SINGLE_LINK_OP_ID][11],
                                    "Wrong Ack reception time");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(
            successRecordIt->m_ackTime,
            m_txStartTimes[SINGLE_LINK_OP_ID][10] + m_durations[SINGLE_LINK_OP_ID][10] + m_sifs +
                m_durations[SINGLE_LINK_OP_ID][11] +
                ((m_cwMins[SINGLE_LINK_OP_ID] + 1) * 2 - 1) * m_slot + 2 * tolerance,
            "Wrong Ack reception time");

        NS_TEST_ASSERT_MSG_GT_OR_EQ(failureRecordIt->m_txStartTime,
                                    m_txStartTimes[SINGLE_LINK_OP_ID][11] +
                                        m_durations[SINGLE_LINK_OP_ID][11] + m_difs,
                                    "Packets should be TXed after enqueued");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(failureRecordIt->m_txStartTime,
                                    m_txStartTimes[SINGLE_LINK_OP_ID][11] +
                                        m_durations[SINGLE_LINK_OP_ID][11] + m_difs + tolerance +
                                        m_cwMins[SINGLE_LINK_OP_ID] * m_slot,
                                    "Packet backoff slots should not exceed cwMin");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_txStartTime,
                              m_txStartTimes[SINGLE_LINK_OP_ID][12],
                              "Wrong TX start time");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropTime.has_value() &&
                                  failureRecordIt->m_dropReason.has_value(),
                              true,
                              "Missing drop time or reason");
        NS_TEST_ASSERT_MSG_GT_OR_EQ(failureRecordIt->m_dropTime.value(),
                                    m_txStartTimes[SINGLE_LINK_OP_ID][18] +
                                        m_durations[SINGLE_LINK_OP_ID][18],
                                    "Wrong Dequeue time for failed packet");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(failureRecordIt->m_dropTime.value(),
                                    m_txStartTimes[SINGLE_LINK_OP_ID][18] +
                                        m_durations[SINGLE_LINK_OP_ID][18] + m_sifs + m_slot +
                                        m_durations[SINGLE_LINK_OP_ID][11],
                                    "Wrong Dequeue time for failed packet");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropReason.value(),
                              WifiMacDropReason::WIFI_MAC_DROP_REACHED_RETRY_LIMIT,
                              "Wrong drop reason");
    }
    else
    {
        const auto totalFailuresQos =
            wifiTxStats.GetFailures(WifiMacDropReason::WIFI_MAC_DROP_QOS_OLD_PACKET);
        const auto totalFailuresQosMap =
            wifiTxStats.GetFailuresByNodeDevice(WifiMacDropReason::WIFI_MAC_DROP_QOS_OLD_PACKET);

        for (std::vector<Time>::size_type i = 0; i < m_txStartTimes[0].size(); ++i)
        {
            NS_LOG_INFO("link 0 pkt " << i << " start tx at " << m_txStartTimes[0][i].As(Time::US));
        }
        for (std::vector<Time>::size_type i = 0; i < m_txStartTimes[1].size(); ++i)
        {
            NS_LOG_INFO("link 1 pkt " << i << " start tx at " << m_txStartTimes[1][i].As(Time::US));
        }

        NS_TEST_ASSERT_MSG_EQ(successMapPerNodeDeviceLink.at(nodeDeviceLink0Tuple),
                              2,
                              "Number of success packets on link 0 should be 2");
        NS_TEST_ASSERT_MSG_EQ(successMapPerNodeDeviceLink.at(nodeDeviceLink1Tuple),
                              2,
                              "Number of success packets on link 1 should be 2");
        NS_TEST_ASSERT_MSG_EQ(successMap.at(nodeDeviceTuple),
                              4,
                              "Number of success packets should be 4");
        NS_TEST_ASSERT_MSG_EQ(totalSuccesses, 4, "Number of success packets should be 4");

        NS_TEST_ASSERT_MSG_EQ(retransmissionMap.at(nodeDeviceTuple),
                              2,
                              "Number of retransmitted successful packets should be 2");
        NS_TEST_ASSERT_MSG_EQ(totalRetransmissions,
                              2,
                              "Number of retransmitted successful packets (aggregate) should be 2");
        NS_TEST_ASSERT_MSG_EQ(failureMap.at(nodeDeviceTuple),
                              2,
                              "Number of failed packets should be 2");
        NS_TEST_ASSERT_MSG_EQ(totalFailures, 2, "Number of failed packets (aggregate) should be 2");
        NS_TEST_ASSERT_MSG_EQ(totalFailuresQos,
                              2,
                              "Number of dropped packets (aggregate) by QosTxop should be 2");
        NS_TEST_ASSERT_MSG_EQ(totalFailuresQosMap.at(nodeDeviceTuple),
                              2,
                              "Number of dropped packets (aggregate) by QosTxop should be 2");

        auto successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink0It->m_nodeId,
            1,
            "Source node ID of the 1st successful data packet on link 0 should be 1");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink0It->m_nodeId,
            1,
            "Source node ID of the 2nd successful data packet on link 0 should be 1");
        auto successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink1It->m_nodeId,
            1,
            "Source node ID of the 1st successful data packet on link 0 should be 1");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink1It->m_nodeId,
            1,
            "Source node ID of the 2nd successful data packet on link 0 should be 1");
        auto failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_nodeId,
                              1,
                              "Source node ID of the failed data packet on link 0 should be 1");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_nodeId,
                              1,
                              "Source node ID of the failed data packet on link 1 should be 1");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_deviceId,
                              0,
                              "Device ID of the 1st successful data packet on link 0 should be 0");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_deviceId,
                              0,
                              "Device ID of the 2nd successful data packet on link 0 should be 0");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_deviceId,
                              0,
                              "Device ID of the 1st successful data packet on link 0 should be 0");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_deviceId,
                              0,
                              "Device ID of the 2nd successful data packet on link 1 should be 0");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_deviceId,
                              0,
                              "Device ID of the failed data packet on link 1 should be 0");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_deviceId,
                              0,
                              "Device ID of the failed data packet on link 1 should be 0");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            *successRecordLink0It->m_successLinkIdSet.begin(),
            0,
            "Successful link ID of the 1st successful data packet on link 0 should be 0");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            *successRecordLink0It->m_successLinkIdSet.begin(),
            0,
            "Successful link ID of the 2nd successful data packet on link 0 should be 0");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            *successRecordLink1It->m_successLinkIdSet.begin(),
            1,
            "Successful link ID of the 1st successful data packet on link 1 should be 1");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            *successRecordLink1It->m_successLinkIdSet.begin(),
            1,
            "Successful link ID of the 2nd successful data packet on link 1 should be 1");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            failureRecordIt->m_successLinkIdSet.empty(),
            true,
            "Successful link ID set of the failed data packet on link 0 should be empty");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(
            failureRecordIt->m_successLinkIdSet.empty(),
            true,
            "Successful link ID set of the failed data packet on link 1 should be empty");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink0It->m_retransmissions,
            0,
            "The 1st successful data packet on link 0 should have no retransmissions");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink0It->m_retransmissions,
            1,
            "The 2nd successful data packet on link 0 should have 1 retransmission");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink1It->m_retransmissions,
            0,
            "The 1st successful data packet on link 1 should have no retransmissions");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink1It->m_retransmissions,
            1,
            "The 2nd successful data packet on link 1 should have 1 retransmission");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_retransmissions,
                              8,
                              "The failed data packet on link 0 should have 8 retransmissions");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_retransmissions,
                              8,
                              "The failed data packet on link 1 should have 8 retransmissions");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_tid,
                              3,
                              "The 1st successful data packet on link 0 should have a TID of 3");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_tid,
                              3,
                              "The 2nd successful data packet on link 0 should have a TID of 3");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_tid,
                              4,
                              "The 1st successful data packet on link 1 should have a TID of 4");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_tid,
                              4,
                              "The 2nd successful data packet on link 1 should have a TID of 4");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_tid,
                              3,
                              "The failed data packet on link 0 should have a TID of 3");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_tid,
                              4,
                              "The failed data packet on link 1 should have a TID of 4");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink0It->m_mpduSeqNum,
            0,
            "The 1st successful data packet on link 0 should have a Seq Num of 0");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink0It->m_mpduSeqNum,
            1,
            "The 2nd successful data packet on link 0 should have a Seq Num of 1");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink1It->m_mpduSeqNum,
            0,
            "The 1st successful data packet on link 1 should have a Seq Num of 0");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(
            successRecordLink1It->m_mpduSeqNum,
            2,
            "The 2nd successful data packet on link 1 should have a Seq Num of 2");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_mpduSeqNum,
                              2,
                              "The failed data packet on link 0 should have a Seq Num of 2");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_mpduSeqNum,
                              1,
                              "The failed data packet on link 1 should have a Seq Num of 1");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet on link 0 should have been TXed");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet on link 0 should have been TXed");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet on link 1 should have been TXed");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet on link 1 should have been TXed");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The failed data packet on link 0 should have been TXed");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_txStartTime.IsStrictlyPositive(),
                              true,
                              "The failed data packet on link 1 should have been TXed");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet on link 0 should have been acked");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet on link 0 should have been acked");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet on link 1 should have been acked");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet on link 1 should have been acked");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_ackTime.IsStrictlyPositive(),
                              false,
                              "The failed data packet on link 0 should not have been acked");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_ackTime.IsStrictlyPositive(),
                              false,
                              "The failed data packet on link 1 should not have been acked");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet on link 0 should have been dequeued");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet on link 0 should have been dequeued");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 1st successful data packet on link 1 should have been dequeued");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_ackTime.IsStrictlyPositive(),
                              true,
                              "The 2nd successful data packet on link 1 should have been dequeued");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropTime.has_value() &&
                                  failureRecordIt->m_dropReason.has_value(),
                              true,
                              "Missing drop time or reason");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropTime.value().IsStrictlyPositive(),
                              true,
                              "The failed data packet on link 0 should have been dequeued");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropReason.value(),
                              WifiMacDropReason::WIFI_MAC_DROP_QOS_OLD_PACKET,
                              "Wrong drop reason");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropTime.has_value() &&
                                  failureRecordIt->m_dropReason.has_value(),
                              true,
                              "Missing drop time or reason");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropTime.value().IsStrictlyPositive(),
                              true,
                              "The failed data packet on link 1 should have been dequeued");
        NS_TEST_ASSERT_MSG_EQ(failureRecordIt->m_dropReason.value(),
                              WifiMacDropReason::WIFI_MAC_DROP_QOS_OLD_PACKET,
                              "Wrong drop reason");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        auto successRecordLink0ItNext = successRecordLink0It;
        std::advance(successRecordLink0ItNext, 1);
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_enqueueTime,
                              successRecordLink0ItNext->m_enqueueTime,
                              "Packets on link 0 should be enqueued at the same time");
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_enqueueTime,
                              failureRecordIt->m_enqueueTime,
                              "Packets on link 0 should be enqueued at the same time");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        auto successRecordLink1ItNext = successRecordLink1It;
        std::advance(successRecordLink1ItNext, 1);
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_enqueueTime,
                              successRecordLink1ItNext->m_enqueueTime,
                              "Packets on link 1 should be enqueued at the same time");
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_enqueueTime,
                              failureRecordIt->m_enqueueTime,
                              "Packets on link 1 should be enqueued at the same time");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink0It->m_txStartTime,
                                    successRecordLink0It->m_enqueueTime,
                                    "The 1st data packet on link 0 should be TXed after enqueued");
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink0It->m_txStartTime,
                                    successRecordLink0It->m_enqueueTime,
                                    "The 2nd data packet on link 0 should be TXed after enqueued");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink1It->m_txStartTime,
                                    successRecordLink1It->m_enqueueTime,
                                    "The 1st data packet on link 1 should be TXed after enqueued");
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink1It->m_txStartTime,
                                    successRecordLink1It->m_enqueueTime,
                                    "The 2nd data packet on link 1 should be TXed after enqueued");
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(failureRecordIt->m_txStartTime,
                                    failureRecordIt->m_enqueueTime,
                                    "The 3rd data packet on link 0 should be TXed after enqueued");
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_GT_OR_EQ(failureRecordIt->m_txStartTime,
                                    failureRecordIt->m_enqueueTime,
                                    "The 3rd data packet on link 1 should be TXed after enqueued");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink0It->m_txStartTime,
                                    m_txStartTimes[0][11] + m_durations[0][11] + m_aifss[0],
                                    "link 0 pkt first tx should be after the 11th packet on link");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordLink0It->m_txStartTime,
                                    m_txStartTimes[0][11] + m_durations[0][11] + m_aifss[0] +
                                        tolerance + m_cwMins[0] * m_slot,
                                    "link 0 pkt first backoff should not exceed cwMin");
        successRecordLink0ItNext = successRecordLink0It;
        std::advance(successRecordLink0ItNext, 1);
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_txStartTime,
                              successRecordLink0ItNext->m_txStartTime,
                              "3 pkts of link 0 should tx at the same time");
        NS_TEST_ASSERT_MSG_EQ(successRecordLink0It->m_txStartTime,
                              failureRecordIt->m_txStartTime,
                              "3 pkts of link 0 should tx at the same time");

        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink1It->m_txStartTime,
                                    m_txStartTimes[1][10] + m_durations[1][10] + m_aifss[1],
                                    "link 1 pkt first tx should be after the 10th packet on link");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordLink1It->m_txStartTime,
                                    m_txStartTimes[1][10] + m_durations[1][10] + m_aifss[1] +
                                        tolerance + m_cwMins[1] * m_slot,
                                    "link 1 pkt first backoff should not exceed cwMin");
        successRecordLink1ItNext = successRecordLink1It;
        std::advance(successRecordLink1ItNext, 1);
        failureRecordIt = failureRecords.at(nodeDeviceTuple).begin();
        std::advance(failureRecordIt, 1);
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_txStartTime,
                              successRecordLink1ItNext->m_txStartTime,
                              "3 pkts of link 1 should tx at the same time");
        NS_TEST_ASSERT_MSG_EQ(successRecordLink1It->m_txStartTime,
                              failureRecordIt->m_txStartTime,
                              "3 pkts of link 1 should tx at the same time");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink0It->m_ackTime,
                                    m_txStartTimes[0][12] + m_durations[0][12] + m_sifs +
                                        m_durations[0][13],
                                    "Wrong first Block Ack reception time on link 0");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordLink0It->m_ackTime,
                                    m_txStartTimes[0][12] + m_durations[0][12] + m_sifs +
                                        m_durations[0][13] + 2 * tolerance,
                                    "Wrong first Block Ack reception time on link 0");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink1It->m_ackTime,
                                    m_txStartTimes[1][11] + m_durations[1][11] + m_sifs +
                                        m_durations[1][12],
                                    "Wrong first Block Ack reception time on link 1");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordLink1It->m_ackTime,
                                    m_txStartTimes[1][11] + m_durations[1][11] + m_sifs +
                                        m_durations[1][12] + 2 * tolerance,
                                    "Wrong first Block Ack reception time on link 1");

        successRecordLink0It = successRecords.at(nodeDeviceLink0Tuple).begin();
        std::advance(successRecordLink0It, 1);
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink0It->m_ackTime,
                                    m_txStartTimes[0][14] + m_durations[0][14] + m_sifs +
                                        m_durations[0][15],
                                    "Wrong second Block Ack reception time on link 0");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordLink0It->m_ackTime,
                                    m_txStartTimes[0][14] + m_durations[0][14] + m_sifs +
                                        m_durations[0][15] + 2 * tolerance,
                                    "Wrong second Block Ack reception time on link 0");
        successRecordLink1It = successRecords.at(nodeDeviceLink1Tuple).begin();
        std::advance(successRecordLink1It, 1);
        NS_TEST_ASSERT_MSG_GT_OR_EQ(successRecordLink1It->m_ackTime,
                                    m_txStartTimes[1][13] + m_durations[1][13] + m_sifs +
                                        m_durations[1][14],
                                    "Wrong second Block Ack reception time on link 1");
        NS_TEST_ASSERT_MSG_LT_OR_EQ(successRecordLink1It->m_ackTime,
                                    m_txStartTimes[1][13] + m_durations[1][13] + m_sifs +
                                        m_durations[1][14] + 2 * tolerance,
                                    "Wrong second Block Ack reception time on link 1");
    }
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief WifiTxStatsHelper Test Suite
 */
class WifiTxStatsHelperTestSuite : public TestSuite
{
  public:
    WifiTxStatsHelperTestSuite();
};

WifiTxStatsHelperTestSuite::WifiTxStatsHelperTestSuite()
    : TestSuite("wifi-tx-stats-helper", Type::UNIT)
{
    // A test case to evaluate the transmission process of multiple Wi-Fi MAC Layer MPDUs in
    // a single link device. This testcase uses .11a to test the handling of regular ACKs.
    //
    // This class tests the WifiTxStatsHelper output by creating three transmission cases:
    // 1) packet is sent successfully on the first try
    // 2) packet is lost on the first try but successfully transmitted on the second try
    // 3) packet is lost on all seven tries and a failure is logged
    // The MPDU losses are forced by the use of WifiPhy post-reception error model.
    //
    // This test also connects to the PHY trace PhyTxPsduBegin and records the sequence of
    // transmission times and packet durations observed at the PHY layer, to cross-check against
    // the times recorded in the WifiTxStatsHelper record (traced at the MAC layer).
    // The testcase also checks the various fields in this helper's output records for correctness.
    AddTestCase(new WifiTxStatsHelperTest("Check single link non-QoS configuration",
                                          WifiTxStatsHelperTest::SINGLE_LINK_NON_QOS),
                TestCase::Duration::QUICK);

    // A test case to evaluate the transmission process of multiple Wi-Fi MAC Layer MPDUs in
    // a multi link device. This testcase, unlike the previous, uses .11be to test the
    // handling of MPDU aggregation, Block ACKs, and Multi-Link Operation.
    //
    // This class tests the WifiTxStatsHelper output by creating three transmission cases:
    // 1) packet is sent successfully on the first try
    // 2) packet is lost on the first try (in an A-MPDU) but successfully transmitted on the
    // second try (also in an A-MPDU)
    // 3) packet is lost on all 9 tries (first 2 in A-MPDU, other 7 alone) and a failure is logged
    // The MPDU losses are forced by the use of WifiPhy post-reception error model.
    //
    // This test also connects to the PHY trace PhyTxPsduBegin and records the sequence of
    // transmission times and packet durations observed at the PHY layer, to cross-check against
    // the times recorded in the WifiTxStatsHelper record (traced at the MAC layer).
    // The testcase also checks the various fields in this helper's output records for correctness.
    AddTestCase(new WifiTxStatsHelperTest("Check multi-link QoS configuration",
                                          WifiTxStatsHelperTest::MULTI_LINK_QOS),
                TestCase::Duration::QUICK);
}

static WifiTxStatsHelperTestSuite g_wifiTxStatsHelperTestSuite; //!< the test suite
