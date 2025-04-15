/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-container.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-static-setup-helper.h"
#include "ns3/wifi-utils.h"

#include <optional>
#include <unordered_map>

/// @ingroup wifi-test
/// @ingroup tests
/// @brief WifiStaticSetupHelper ADHOC test suite
/// Test suite intended to test static management exchanges between
/// AdhocWifiMac device and StaWifiMac device in single link and multi
/// link operation scenarios.
/// The test constructs devices based on input test vector and performs
/// static exchange of capabilities using WifiStaticSetupHelper.
/// The test verifies the state updates at the input devices.

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WifiStaticAdhocBssTestSuite");

/// @brief Constants used in this test suite
namespace WifiStaticAdhocBssTestConstants
{
const auto DEFAULT_RNG_SEED = 3;                   ///< default RNG seed
const auto DEFAULT_RNG_RUN = 7;                    ///< default RNG run
const auto DEFAULT_STREAM_INDEX = 100;             ///< default stream index
const auto DEFAULT_SIM_STOP_TIME = NanoSeconds(1); ///< default simulation stop time
const auto DEFAULT_BEACON_GEN = false;             ///< default beacon generation value
const auto DEFAULT_WIFI_STANDARD = WifiStandard::WIFI_STANDARD_80211be; ///< default Wi-Fi standard
const auto DEFAULT_P2P_LINKS_ENABLE = true;          ///< default value for P2P links enabled
const auto DEFAULT_P2P_EMLSR_PEER = false;           ///< default value for EMLSR P2P enabled
const auto DEFAULT_SSID = Ssid("wifi-static-setup"); ///< default SSID
} // namespace WifiStaticAdhocBssTestConstants

namespace consts = WifiStaticAdhocBssTestConstants;

/// @brief channel map typedef
using ChannelMap = std::unordered_map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>>;

/// @brief test case information
struct WifiStaticAdhocBssTestVector
{
    /// @brief input type
    struct Input
    {
        std::string name;                                     ///< Test case name
        std::string adhocCh;                                  ///< Adhoc BSS channel
        StringVector clientChs{};                             ///< Client device channels
        bool clientP2pExch{consts::DEFAULT_P2P_LINKS_ENABLE}; ///< Enable client P2P links
        bool adhocEmlsrPeer{consts::DEFAULT_P2P_EMLSR_PEER};  ///< Enable Adhoc EMLSR Peer
    };

    /// @brief expected outcome type
    struct Expected
    {
        std::optional<linkId_t> p2pLinkId{std::nullopt}; ///< Client link ID for P2P
    };

    Input m_input;       ///< input
    Expected m_expected; ///< expected outcome
};

/**
 * Static adhoc BSS test case.
 */
class WifiStaticAdhocBssTest : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param testVec the test vector
     */
    WifiStaticAdhocBssTest(const WifiStaticAdhocBssTestVector& testVec);

  private:
    /// Construct WifiNetDevice
    /// @param isClient true if client, false otherwise
    /// @param channelMap created spectrum channels
    /// @return constructed WifiNetDevice
    Ptr<WifiNetDevice> GetWifiNetDevice(bool isClient, const ChannelMap& channelMap);

    /// Construct PHY helper based on input operating channels
    /// @param settings vector of strings specifying the operating channels to configure
    /// @param channelMap created spectrum channels
    /// @return PHY helper
    SpectrumWifiPhyHelper GetPhyHelper(const std::vector<std::string>& settings,
                                       const ChannelMap& channelMap) const;

    /// @return the WifiHelper
    WifiHelper GetWifiHelper() const;
    /// @return the Adhoc MAC helper
    WifiMacHelper GetAdhocMacHelper() const;
    /// @return the Client MAC helper
    WifiMacHelper GetClientMacHelper() const;
    void ValidateRecords(); ///< Validate Association

    /// validate creation of P2P link
    /// @param actualLinkId the actual ID of the P2P link
    /// @param expectedLinkId the expected ID of the P2P link
    void ValidateP2pLinkId(std::optional<linkId_t> actualLinkId,
                           std::optional<linkId_t> expectedLinkId);

    void DoRun() override;
    void DoSetup() override;

    WifiStaticAdhocBssTestVector m_testVec;  ///< Test vector
    std::optional<linkId_t> m_expP2pLinkId;  ///< expected client link ID for P2P (if any)
    Ptr<WifiNetDevice> m_adhocDev{nullptr};  ///< AdhocWifiMac device
    Ptr<WifiNetDevice> m_clientDev{nullptr}; ///< StaticWifiMac device
};

WifiStaticAdhocBssTest::WifiStaticAdhocBssTest(const WifiStaticAdhocBssTestVector& testVec)
    : TestCase(testVec.m_input.name),
      m_testVec(testVec)
{
}

WifiHelper
WifiStaticAdhocBssTest::GetWifiHelper() const
{
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(consts::DEFAULT_WIFI_STANDARD);
    wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager");
    return wifiHelper;
}

SpectrumWifiPhyHelper
WifiStaticAdhocBssTest::GetPhyHelper(const std::vector<std::string>& settings,
                                     const ChannelMap& channelMap) const
{
    NS_ASSERT(not settings.empty());
    SpectrumWifiPhyHelper helper(settings.size());

    linkId_t linkId = 0;
    for (const auto& str : settings)
    {
        helper.Set(linkId, "ChannelSettings", StringValue(str));
        auto channelConfig = WifiChannelConfig::FromString(str);
        auto phyBand = channelConfig.front().band;
        auto freqRange = GetFrequencyRange(phyBand);
        helper.AddPhyToFreqRangeMapping(linkId, freqRange);
        helper.AddChannel(channelMap.at(phyBand), freqRange);

        ++linkId;
    }
    return helper;
}

WifiMacHelper
WifiStaticAdhocBssTest::GetAdhocMacHelper() const
{
    WifiMacHelper macHelper;

    macHelper.SetType("ns3::AdhocWifiMac",
                      "Ssid",
                      SsidValue(consts::DEFAULT_SSID),
                      "BeaconGeneration",
                      BooleanValue(consts::DEFAULT_BEACON_GEN),
                      "EmlsrPeer",
                      BooleanValue(m_testVec.m_input.adhocEmlsrPeer));
    return macHelper;
}

WifiMacHelper
WifiStaticAdhocBssTest::GetClientMacHelper() const
{
    WifiMacHelper macHelper;
    Ssid ssid = Ssid(consts::DEFAULT_SSID);
    macHelper.SetType("ns3::StaWifiMac",
                      "Ssid",
                      SsidValue(ssid),
                      "EnableP2pLinks",
                      BooleanValue(m_testVec.m_input.clientP2pExch));
    return macHelper;
}

Ptr<WifiNetDevice>
WifiStaticAdhocBssTest::GetWifiNetDevice(bool isClient, const ChannelMap& channelMap)
{
    NodeContainer node;
    node.Create(1);
    auto wifiHelper = GetWifiHelper();
    auto settings =
        isClient ? m_testVec.m_input.clientChs : StringVector{m_testVec.m_input.adhocCh};
    auto phyHelper = GetPhyHelper(settings, channelMap);
    auto macHelper = isClient ? GetClientMacHelper() : GetAdhocMacHelper();
    auto netDev = wifiHelper.Install(phyHelper, macHelper, node);
    WifiHelper::AssignStreams(netDev, consts::DEFAULT_STREAM_INDEX);
    return DynamicCast<WifiNetDevice>(netDev.Get(0));
}

void
WifiStaticAdhocBssTest::DoSetup()
{
    RngSeedManager::SetSeed(consts::DEFAULT_RNG_SEED);
    RngSeedManager::SetRun(consts::DEFAULT_RNG_RUN);

    ChannelMap channelMap = {{WIFI_PHY_BAND_2_4GHZ, CreateObject<MultiModelSpectrumChannel>()},
                             {WIFI_PHY_BAND_5GHZ, CreateObject<MultiModelSpectrumChannel>()},
                             {WIFI_PHY_BAND_6GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    m_adhocDev = GetWifiNetDevice(false, channelMap);
    NS_ASSERT(m_adhocDev);
    m_clientDev = GetWifiNetDevice(true, channelMap);
    NS_ASSERT(m_clientDev);

    WifiStaticSetupHelper::SetStaticAssoc(m_adhocDev, m_clientDev);
}

void
WifiStaticAdhocBssTest::DoRun()
{
    Simulator::Stop(consts::DEFAULT_SIM_STOP_TIME);
    Simulator::Run();
    ValidateRecords();
    Simulator::Destroy();
}

void
WifiStaticAdhocBssTest::ValidateP2pLinkId(std::optional<linkId_t> actualLinkId,
                                          std::optional<linkId_t> expectedLinkId)
{
    NS_TEST_ASSERT_MSG_EQ(actualLinkId.has_value(),
                          expectedLinkId.has_value(),
                          "Unexpected P2P link ID option");

    if (actualLinkId.has_value())
    {
        NS_TEST_ASSERT_MSG_EQ(*actualLinkId, *expectedLinkId, "Unexpected P2P link ID value");
    }
}

void
WifiStaticAdhocBssTest::ValidateRecords()
{
    // Validate WifiStaticSetupHelper logic
    if (m_testVec.m_input.clientP2pExch)
    {
        auto p2pLinkId = WifiStaticSetupHelper::GetP2pLinkId(m_adhocDev, m_clientDev);
        ValidateP2pLinkId(p2pLinkId, m_testVec.m_expected.p2pLinkId);
    }

    // Validate StaWifiMac record
    auto clientMac = DynamicCast<StaWifiMac>(m_clientDev->GetMac());
    auto adhocAddr = m_adhocDev->GetMac()->GetAddress();
    auto p2pLinkId = clientMac->GetLinkIdForPeer(adhocAddr);
    ValidateP2pLinkId(p2pLinkId, m_testVec.m_expected.p2pLinkId);

    if (!p2pLinkId)
    {
        return;
    }

    auto clientStaMgr = m_clientDev->GetRemoteStationManager(*p2pLinkId);
    NS_TEST_ASSERT_MSG_EQ(clientStaMgr->IsAdhocPeer(adhocAddr),
                          true,
                          "Unexpected Adhoc Peer record at client for adhoc device");
    NS_TEST_ASSERT_MSG_EQ(clientStaMgr->GetEhtSupported(adhocAddr),
                          true,
                          "Unexpected EHT Supported record at client for adhoc device");

    // Validate AdhocWifiMac record
    auto clientAddr = clientMac->GetFrameExchangeManager(*p2pLinkId)->GetAddress();
    auto adhocStaMgr = m_adhocDev->GetRemoteStationManager(SINGLE_LINK_OP_ID);
    NS_TEST_ASSERT_MSG_EQ(adhocStaMgr->IsAdhocPeer(clientAddr),
                          true,
                          "Unexpected Adhoc Peer record at adhoc device for client");
    NS_TEST_ASSERT_MSG_EQ(adhocStaMgr->GetEhtSupported(clientAddr),
                          true,
                          "Unexpected EHT Supported record for client");
    NS_TEST_ASSERT_MSG_EQ(adhocStaMgr->GetEmlsrEnabled(clientAddr),
                          m_testVec.m_input.adhocEmlsrPeer,
                          "Unexpected EMLSR record at adhoc device for client");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Adhoc BSS static setup test suite
 */
class WifiStaticAdhocBssTestSuite : public TestSuite
{
  public:
    WifiStaticAdhocBssTestSuite();
};

WifiStaticAdhocBssTestSuite::WifiStaticAdhocBssTestSuite()
    : TestSuite("wifi-static-adhoc-bss-test", Type::UNIT)
{
    const std::vector<WifiStaticAdhocBssTestVector> testVectors = {
        {.m_input = {"1-link", "{36, 0, BAND_5GHZ, 0}", {"{36, 0, BAND_5GHZ, 0}"}, true, false},
         .m_expected = {0}},
        {.m_input =
             {"1-link-No-P2p", "{36, 0, BAND_5GHZ, 0}", {"{36, 0, BAND_5GHZ, 0}"}, false, false},
         .m_expected = {std::nullopt}},
        {
            .m_input = {"1-link-Different-BW",
                        "{36, 0, BAND_5GHZ, 0}",
                        {"{42, 0, BAND_5GHZ, 0}"},
                        true,
                        false},
            .m_expected = {0},
        },
        {.m_input =
             {"1-link-No-Match", "{36, 0, BAND_5GHZ, 0}", {"{1, 0, BAND_6GHZ, 0}"}, true, false},
         .m_expected = {std::nullopt}},
        {.m_input = {"2-link",
                     "{36, 0, BAND_5GHZ, 0}",
                     {"{1, 0, BAND_6GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                     true,
                     false},
         .m_expected = {1}},
        {.m_input = {"2-link-Reverse",
                     "{36, 0, BAND_5GHZ, 0}",
                     {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                     true,
                     false},
         .m_expected = {0}},
        {.m_input = {"2-link-No-P2p",
                     "{36, 0, BAND_5GHZ, 0}",
                     {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                     false,
                     false},
         .m_expected = {std::nullopt}},
        {.m_input = {"2-link-No-Match",
                     "{36, 0, BAND_5GHZ, 0}",
                     {"{52, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                     true,
                     false},
         .m_expected = {std::nullopt}},
        {.m_input = {"2-link-Different-BW",
                     "{36, 0, BAND_5GHZ, 0}",
                     {"{1, 0, BAND_6GHZ, 0}", "{42, 0, BAND_5GHZ, 0}"},
                     true,
                     false},
         .m_expected = {1}},
        {.m_input = {"2-link-EMLSR",
                     "{36, 0, BAND_5GHZ, 0}",
                     {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                     true,
                     true},
         .m_expected = {0}}

    };

    for (const auto& testVec : testVectors)
    {
        AddTestCase(new WifiStaticAdhocBssTest(testVec), TestCase::Duration::QUICK);
    }
}

static WifiStaticAdhocBssTestSuite g_wifiStaticAdhocBssTestSuite;
