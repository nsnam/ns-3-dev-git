/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/common-info-probe-req-mle.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/eht-configuration.h"
#include "ns3/error-model.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-link-element.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-helper.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tuple.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mode.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-common.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/wifi-utils.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiProbeExchangeTestSuite");

// clang-format off
const auto DEFAULT_SIM_STOP_TIME         = MilliSeconds(500);
const auto DEFAULT_PROBE_REQ_TX_TIME     = MilliSeconds(10);
const auto DEFAULT_DATA_MODE             = "EhtMcs3";
const auto DEFAULT_CONTROL_MODE          = "OfdmRate24Mbps";
const auto DEFAULT_SSID                  = Ssid("probe-exch-test");
const auto DEFAULT_RNG_SEED              = 3;
const auto DEFAULT_RNG_RUN               = 7;
const auto DEFAULT_STREAM_INDEX          = 100;
const uint64_t DEFAULT_STREAM_INCREMENT  = 1e4; // some large number
const auto DEFAULT_WIFI_STANDARD         = WifiStandard::WIFI_STANDARD_80211be;
const auto DEFAULT_PROBE_REQ_ADDR1_BCAST = false;
const auto DEFAULT_PROBE_REQ_ADDR3_BCAST = false;
const auto DEFAULT_MULTI_LINK_PROBE_REQ  = false;
const uint8_t DEFAULT_PRB_EXCH_LINK_ID   = 0;
const uint8_t DEFAULT_AP_MLD_ID          = 0;
using LinkIds = std::vector<uint8_t>; ///< Link identifiers

// clang-format on

/// Parameters and expected results for a test case
struct ProbeExchTestVector
{
    std::string name{};                                ///< Test case name
    std::vector<std::string> apChs{};                  ///< AP MLD channels
    std::vector<std::string> clientChs{};              ///< Non-AP MLD channels
    bool isMultiLinkReq{DEFAULT_MULTI_LINK_PROBE_REQ}; ///< Send Multi-link Prpbe Req
    uint8_t reqTxLinkId{DEFAULT_PRB_EXCH_LINK_ID};     ///< Probe Request Tx Link ID
    LinkIds reqLinkIds{}; ///< Link IDs included in Multi-link Probe Request if any
    bool addr1Bcast{DEFAULT_PROBE_REQ_ADDR1_BCAST}; ///< Flag for Probe Request ADDR1 broadcast
    bool addr3Bcast{DEFAULT_PROBE_REQ_ADDR3_BCAST}; ///< Flag for Probe Request ADDR3 broadcast
    uint8_t respTxLinkId{DEFAULT_PRB_EXCH_LINK_ID}; ///< Probe Response Tx Link ID
    LinkIds respLinkIds{}; ///< Expected link IDs included in Multi-link Probe Response if any
};

/**
 * @ingroup wifi-test
 * @ingroup tests
 * @brief Probe Request-Probe Response exchange
 *
 * Test suite including Probe Request and multi-link Probe Request for various cases of
 * Probe Request frame contents and link of transmission.
 */
class ProbeExchTest : public TestCase
{
  public:
    /// information on transmitted PSDU
    struct TxPsdu
    {
        Ptr<const WifiPsdu> psdu; ///< WifiPsdu
        WifiTxVector txVec;       ///< TXVECTOR
        uint8_t linkId;           ///< Tx link ID
    };

    /**
     * Constructor.
     *
     * @param testVec the test vector
     * @param testCase the test case name
     */
    ProbeExchTest(ProbeExchTestVector testVec, std::string testCase);

    /// PHY band-indexed map of spectrum channels
    using ChannelMap = std::map<FrequencyRange, Ptr<MultiModelSpectrumChannel>>;

  private:
    /**
     * Setup WifiNetDevices.
     */
    void SetupDevices();

    /**
     * Setup PSDU Tx trace.
     *
     * @param dev the wifi netdevice
     * @param nodeId the node ID
     */
    void SetupTxTrace(Ptr<WifiNetDevice> dev, std::size_t nodeId);

    /**
     * Send Probe Request based on test vector input.
     */
    void SendProbeReq();

    /**
     *  Setup the PHY Helper based on input channel settings.
     *
     * @param helper the spectrum wifi PHY helper
     * @param channels the list of channels
     * @param channelMap the channel map to configure
     */
    void SetChannels(SpectrumWifiPhyHelper& helper,
                     const std::vector<std::string>& channels,
                     const ChannelMap& channelMap);

    /**
     * Traced callback when FEM passes PSDUs to the PHY.
     *
     * @param linkId the link ID
     * @param context the context
     * @param psduMap the transmitted PSDU map
     * @param txVector the TXVECTOR used for transmission
     * @param txPowerW the TX power in watts
     */
    void CollectTxTrace(uint8_t linkId,
                        std::string context,
                        WifiConstPsduMap psduMap,
                        WifiTxVector txVector,
                        double txPowerW);

    /**
     * Check Probe Request contents.
     *
     * @param txPsdu information about transmitted PSDU
     */
    void ValidateProbeReq(const TxPsdu& txPsdu);

    /**
     * Check Probe Response contents.
     *
     * @param txPsdu information about transmitted PSDU
     */
    void ValidateProbeResp(const TxPsdu& txPsdu);

    /**
     * Check expected outcome of test case run.
     */
    void ValidateTest();

    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    /**
     *  Get Link MAC address for input device on specified link.
     *
     * @param dev the input device
     * @param linkId the ID of the specified link
     * @return the Link MAC address for input device on specified link
     */
    Mac48Address GetLinkMacAddr(Ptr<WifiNetDevice> dev, uint8_t linkId);

    Ptr<WifiNetDevice> m_apDev{nullptr};     ///< AP MLD WifiNetDevice
    Ptr<WifiNetDevice> m_clientDev{nullptr}; ///< Non-AP MLD WifiNetDevice
    ProbeExchTestVector m_testVec;           ///< Test vector
    std::vector<TxPsdu> m_mgtPsdus{};        ///< Tx PSDUs
};

ProbeExchTest::ProbeExchTest(ProbeExchTestVector testVec, std::string testCase)
    : TestCase(testCase),
      m_testVec(testVec)
{
}

void
ProbeExchTest::SetupDevices()
{
    NodeContainer apNode(1);
    NodeContainer clientNode(1);

    WifiHelper wifi;
    wifi.SetStandard(DEFAULT_WIFI_STANDARD);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(DEFAULT_DATA_MODE),
                                 "ControlMode",
                                 StringValue(DEFAULT_CONTROL_MODE));

    ChannelMap channelMap{{WIFI_SPECTRUM_2_4_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_5_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_6_GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    SpectrumWifiPhyHelper apPhyHelper;
    SetChannels(apPhyHelper, m_testVec.apChs, channelMap);
    SpectrumWifiPhyHelper clientPhyHelper;
    SetChannels(clientPhyHelper, m_testVec.clientChs, channelMap);

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(DEFAULT_SSID),
                "BeaconGeneration",
                BooleanValue(false));
    NetDeviceContainer apDevices = wifi.Install(apPhyHelper, mac, apNode);
    m_apDev = DynamicCast<WifiNetDevice>(apDevices.Get(0));
    NS_ASSERT(m_apDev);

    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                SsidValue(DEFAULT_SSID),
                "ActiveProbing",
                BooleanValue(false));
    auto clientDevices = wifi.Install(clientPhyHelper, mac, clientNode);
    m_clientDev = DynamicCast<WifiNetDevice>(clientDevices.Get(0));
    NS_ASSERT(m_clientDev);

    // Assign fixed streams to random variables in use
    auto streamNumber = DEFAULT_STREAM_INDEX;
    auto streamsUsed = WifiHelper::AssignStreams(apDevices, streamNumber);
    NS_ASSERT_MSG(static_cast<uint64_t>(streamsUsed) < DEFAULT_STREAM_INCREMENT,
                  "Too many streams used (" << streamsUsed << "), increase the stream increment");
    streamNumber += DEFAULT_STREAM_INCREMENT;
    streamsUsed = WifiHelper::AssignStreams(clientDevices, streamNumber);
    NS_ASSERT_MSG(static_cast<uint64_t>(streamsUsed) < DEFAULT_STREAM_INCREMENT,
                  "Too many streams used (" << streamsUsed << "), increase the stream increment");

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);
    mobility.Install(clientNode);
}

void
ProbeExchTest::SetChannels(SpectrumWifiPhyHelper& helper,
                           const std::vector<std::string>& channels,
                           const ChannelMap& channelMap)
{
    helper = SpectrumWifiPhyHelper(channels.size());
    helper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    for (std::size_t idx = 0; idx != channels.size(); ++idx)
    {
        helper.Set(idx, "ChannelSettings", StringValue(channels[idx]));
    }

    for (const auto& [band, channel] : channelMap)
    {
        helper.AddChannel(channel, band);
    }
}

Mac48Address
ProbeExchTest::GetLinkMacAddr(Ptr<WifiNetDevice> dev, uint8_t linkId)
{
    auto mac = dev->GetMac();
    NS_ASSERT(linkId < mac->GetNLinks());
    return mac->GetFrameExchangeManager(linkId)->GetAddress();
}

void
ProbeExchTest::SetupTxTrace(Ptr<WifiNetDevice> dev, std::size_t nodeId)
{
    for (uint8_t idx = 0; idx < dev->GetNPhys(); ++idx)
    {
        Config::Connect("/NodeList/" + std::to_string(nodeId) +
                            "/DeviceList/*/$ns3::WifiNetDevice/Phys/" + std::to_string(idx) +
                            "/PhyTxPsduBegin",
                        MakeCallback(&ProbeExchTest::CollectTxTrace, this).Bind(idx));
    }
}

void
ProbeExchTest::CollectTxTrace(uint8_t linkId,
                              std::string context,
                              WifiConstPsduMap psduMap,
                              WifiTxVector txVector,
                              double txPowerW)
{
    auto psdu = psduMap.begin()->second;
    if (psdu->GetHeader(0).IsMgt())
    {
        m_mgtPsdus.push_back({psdu, txVector, linkId});
    }
}

void
ProbeExchTest::SendProbeReq()
{
    auto clientMac = DynamicCast<StaWifiMac>(m_clientDev->GetMac());
    NS_ASSERT(clientMac);

    const auto& reqTxLinkId = m_testVec.reqTxLinkId;
    MgtProbeRequestHeader probeReq;
    if (m_testVec.isMultiLinkReq)
    {
        probeReq = clientMac->GetMultiLinkProbeRequest(reqTxLinkId,
                                                       m_testVec.reqLinkIds,
                                                       DEFAULT_AP_MLD_ID);
    }
    else
    {
        probeReq = clientMac->GetProbeRequest(reqTxLinkId);
    }

    auto apLinkAddr = GetLinkMacAddr(m_apDev, m_testVec.respTxLinkId);
    auto bcastAddr = Mac48Address::GetBroadcast();
    auto addr1 = m_testVec.addr1Bcast ? bcastAddr : apLinkAddr;
    auto addr3 = m_testVec.addr3Bcast ? bcastAddr : apLinkAddr;
    clientMac->EnqueueProbeRequest(probeReq, reqTxLinkId, addr1, addr3);
}

void
ProbeExchTest::DoSetup()
{
    RngSeedManager::SetSeed(DEFAULT_RNG_SEED);
    RngSeedManager::SetRun(DEFAULT_RNG_RUN);
    SetupDevices();
    SetupTxTrace(m_apDev, 0);
    SetupTxTrace(m_clientDev, 1);
}

void
ProbeExchTest::ValidateProbeReq(const TxPsdu& txPsdu)
{
    auto psdu = txPsdu.psdu;
    auto macHdr = psdu->GetHeader(0);
    NS_TEST_ASSERT_MSG_EQ(macHdr.IsProbeReq(), true, "Probe Request expected, actual =" << macHdr);
    NS_TEST_ASSERT_MSG_EQ(+txPsdu.linkId,
                          +m_testVec.reqTxLinkId,
                          "Probe Request transmission link mismatch");

    auto packet = psdu->GetPayload(0);
    MgtProbeRequestHeader probeReq;
    packet->PeekHeader(probeReq);
    auto mle = probeReq.Get<MultiLinkElement>();
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(),
                          m_testVec.isMultiLinkReq,
                          "Multi-link Element expectation mismatch");

    if (!mle.has_value())
    { // Further checks on Multi-link Element contents
        return;
    }

    auto nProfiles = mle->GetNPerStaProfileSubelements();
    auto expectedNProfiles = m_testVec.reqLinkIds.size();
    NS_TEST_ASSERT_MSG_EQ(nProfiles, expectedNProfiles, "Number of Per-STA Profiles mismatch");

    for (std::size_t i = 0; i < nProfiles; ++i)
    {
        auto actualLinkId = mle->GetPerStaProfile(i).GetLinkId();
        NS_TEST_ASSERT_MSG_EQ(+actualLinkId,
                              +m_testVec.reqLinkIds[i],
                              "Per-STA Profile Link ID mismatch");
    }
}

void
ProbeExchTest::ValidateProbeResp(const TxPsdu& txPsdu)
{
    auto psdu = txPsdu.psdu;
    auto macHdr = psdu->GetHeader(0);
    NS_TEST_ASSERT_MSG_EQ(macHdr.IsProbeResp(),
                          true,
                          "Probe Response expected, actual =" << macHdr);
    NS_TEST_ASSERT_MSG_EQ(+txPsdu.linkId,
                          +m_testVec.respTxLinkId,
                          "Probe Response transmission link mismatch");

    auto packet = psdu->GetPayload(0);
    MgtProbeResponseHeader probeResp;
    packet->PeekHeader(probeResp);
    auto mle = probeResp.Get<MultiLinkElement>();
    auto isMleExpected = m_testVec.apChs.size() > 1;
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(),
                          isMleExpected,
                          "Multi-link Element expectation mismatch");

    if (!mle.has_value())
    { // Further checks on Multi-link Element contents
        return;
    }

    auto nProfiles = mle->GetNPerStaProfileSubelements();
    auto expectedNProfiles = m_testVec.respLinkIds.size();
    NS_TEST_ASSERT_MSG_EQ(nProfiles, expectedNProfiles, "Number of Per-STA Profiles mismatch");

    LinkIds respLinkIds{};
    for (std::size_t i = 0; i < nProfiles; ++i)
    {
        auto actualLinkId = mle->GetPerStaProfile(i).GetLinkId();
        NS_TEST_ASSERT_MSG_EQ(+actualLinkId,
                              +m_testVec.respLinkIds[i],
                              "Per-STA Profile Link ID mismatch");
    }
}

void
ProbeExchTest::ValidateTest()
{
    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_mgtPsdus.size(), 2, "Expected Probe Request and Response");

    // Test first Management PSDU is Probe Request
    ValidateProbeReq(m_mgtPsdus[0]);

    // Test second Management PSDU is Probe Response
    ValidateProbeResp(m_mgtPsdus[1]);
}

void
ProbeExchTest::DoRun()
{
    Simulator::Schedule(DEFAULT_PROBE_REQ_TX_TIME, &ProbeExchTest::SendProbeReq, this);
    Simulator::Stop(DEFAULT_SIM_STOP_TIME);
    Simulator::Run();
    Simulator::Destroy();
    ValidateTest();
}

void
ProbeExchTest::DoTeardown()
{
    m_apDev->Dispose();
    m_apDev = nullptr;
    m_clientDev->Dispose();
    m_clientDev = nullptr;
    m_mgtPsdus.clear();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi probe exchange Test Suite
 */
class ProbeExchTestSuite : public TestSuite
{
  public:
    ProbeExchTestSuite();
};

ProbeExchTestSuite::ProbeExchTestSuite()
    : TestSuite("wifi-probe-exchange", Type::UNIT)
{
    using ChCfgVec = std::vector<std::string>;
    ChCfgVec ap1Link{"{1, 0, BAND_6GHZ, 0}"};
    ChCfgVec ap1LinkAlt{"{36, 0, BAND_5GHZ, 0}"};
    ChCfgVec ap1LinkAlt2{"{2, 0, BAND_2_4GHZ, 0}"};
    ChCfgVec ap2Links{"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"};
    ChCfgVec ap2LinksAlt{"{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"};
    ChCfgVec ap3Links{"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"};
    ChCfgVec clientChCfg{"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"};
    std::vector<ProbeExchTestVector> vecs{
        {.name = "Single link AP, non-AP MLD sends Probe Request on link 2",
         .apChs = ap1Link,
         .clientChs = clientChCfg,
         .isMultiLinkReq = false,
         .reqTxLinkId = 2,
         .reqLinkIds = {},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {}},
        {.name = "Single link AP, non-AP MLD sends Probe Request on link 1",
         .apChs = ap1LinkAlt,
         .clientChs = clientChCfg,
         .isMultiLinkReq = false,
         .reqTxLinkId = 1,
         .reqLinkIds = {},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {}},
        {.name = "Single link AP, non-AP MLD sends Probe Request on link 0",
         .apChs = ap1LinkAlt2,
         .clientChs = clientChCfg,
         .isMultiLinkReq = false,
         .reqTxLinkId = 0,
         .reqLinkIds = {},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request on link 0 requesting a different link",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 0,
         .reqLinkIds = {2},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {2}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request with broadcast Addr1 and Addr3",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 1,
         .reqLinkIds = {0, 1, 2},
         .addr1Bcast = true,
         .addr3Bcast = true,
         .respTxLinkId = 1,
         .respLinkIds = {}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request on link 2 requesting the same link",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 2,
         .reqLinkIds = {2},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 2,
         .respLinkIds = {}},
        {.name = "Non-AP MLD sends Probe Request to AP MLD",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = false,
         .reqTxLinkId = 1,
         .reqLinkIds = {},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 1,
         .respLinkIds = {}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request to AP MLD with 3 links requesting all "
                 "links",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 0,
         .reqLinkIds = {0, 1, 2},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {1, 2}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request on link 1 to AP MLD with 2 links "
                 "requesting all links",
         .apChs = ap2Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 1,
         .reqLinkIds = {0, 1},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {1}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request on link 0 to AP MLD with 2 links "
                 "requesting all links",
         .apChs = ap2LinksAlt,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 0,
         .reqLinkIds = {0, 1},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {1}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request with no Per-STA-Profile",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 0,
         .reqLinkIds = {},
         .addr1Bcast = false,
         .addr3Bcast = true,
         .respTxLinkId = 0,
         .respLinkIds = {1, 2}},
        {.name = "Non-AP MLD sends Multi-Link Probe Request with broadcast Addr1",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 1,
         .reqLinkIds = {1, 2},
         .addr1Bcast = true,
         .addr3Bcast = false,
         .respTxLinkId = 1,
         .respLinkIds = {2}},
        {.name = "Duplicate requested Link IDs",
         .apChs = ap3Links,
         .clientChs = clientChCfg,
         .isMultiLinkReq = true,
         .reqTxLinkId = 0,
         .reqLinkIds = {0, 1, 1, 2, 2},
         .addr1Bcast = false,
         .addr3Bcast = false,
         .respTxLinkId = 0,
         .respLinkIds = {1, 2}},
    };

    for (const auto& testVec : vecs)
    {
        AddTestCase(new ProbeExchTest(testVec, testVec.name), TestCase::Duration::QUICK);
    }
}

static ProbeExchTestSuite g_probeExchTestSuite;
