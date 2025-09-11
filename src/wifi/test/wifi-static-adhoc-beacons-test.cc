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
/// @brief WifiStaticSetupHelper ADHOC beacons exchange test suite
/// Test suite intended to test static management exchanges between
/// AdhocWifiMac devices.
/// The test constructs devices based on input test vector and performs
/// static exchange of capabilities using WifiStaticSetupHelper.
/// The test verifies the state updates at the input devices.

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WifiStaticAdhocBeaconsTestSuite");

/// @brief Constants used in this test suite
namespace WifiStaticAdhocBeaconsTestConstants
{
const auto DEFAULT_RNG_SEED = 3;                   ///< default RNG seed
const auto DEFAULT_RNG_RUN = 7;                    ///< default RNG run
const auto DEFAULT_STREAM_INDEX = 100;             ///< default stream index
const auto DEFAULT_SIM_STOP_TIME = NanoSeconds(1); ///< default simulation stop time
const auto DEFAULT_BEACON_GEN = false;             ///< default beacon generation value
const auto DEFAULT_WIFI_STANDARD = WifiStandard::WIFI_STANDARD_80211be; ///< default Wi-Fi standard
const auto DEFAULT_SSID_PREFIX = "adhoc-";                              ///< default SSID prefix
}; // namespace WifiStaticAdhocBeaconsTestConstants

namespace consts = WifiStaticAdhocBeaconsTestConstants;

/// @brief channel map typedef
using ChannelMap = std::unordered_map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>>;

/// @brief test case information
struct WifiStaticAdhocBeaconsTestVector
{
    /// @brief input type
    struct Input
    {
        std::string name;        ///< Test case name
        StringVector adhocChs{}; ///< Adhoc devices channels
    };

    /// @brief expected outcome type
    struct Expected
    {
        bool exchange{true}; ///< Adhoc devices capability exchange
    };

    Input m_input;       ///< input
    Expected m_expected; ///< expected outcome
};

/**
 * Static adhoc beacons test case.
 */
class WifiStaticAdhocBeaconsTest : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param testVec the test vector
     */
    WifiStaticAdhocBeaconsTest(const WifiStaticAdhocBeaconsTestVector& testVec);

  private:
    /// Construct WifiNetDevice
    /// @param index the device index
    /// @param channelMap created spectrum channels
    /// @return constructed WifiNetDevice
    Ptr<WifiNetDevice> GetWifiNetDevice(size_t index, const ChannelMap& channelMap);

    /// Construct PHY helper based on input operating channels
    /// @param settings vector of strings specifying the operating channels to configure
    /// @param channelMap created spectrum channels
    /// @return PHY helper
    SpectrumWifiPhyHelper GetPhyHelper(const std::vector<std::string>& settings,
                                       const ChannelMap& channelMap) const;

    /// @return the WifiHelper
    WifiHelper GetWifiHelper() const;
    /// @param index the device index
    /// @return the Adhoc MAC helper
    WifiMacHelper GetAdhocMacHelper(size_t index) const;
    void ValidateRecords(); ///< Validate Association

    void DoRun() override;
    void DoSetup() override;

    WifiStaticAdhocBeaconsTestVector m_testVec; ///< Test vector
    std::vector<Ptr<WifiNetDevice>> m_devs{};   ///< Adhoc devices
};

WifiStaticAdhocBeaconsTest::WifiStaticAdhocBeaconsTest(
    const WifiStaticAdhocBeaconsTestVector& testVec)
    : TestCase(testVec.m_input.name),
      m_testVec(testVec)
{
}

WifiHelper
WifiStaticAdhocBeaconsTest::GetWifiHelper() const
{
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(consts::DEFAULT_WIFI_STANDARD);
    wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager");
    return wifiHelper;
}

SpectrumWifiPhyHelper
WifiStaticAdhocBeaconsTest::GetPhyHelper(const std::vector<std::string>& settings,
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
WifiStaticAdhocBeaconsTest::GetAdhocMacHelper(size_t index) const
{
    WifiMacHelper macHelper;
    std::string ssidStr = consts::DEFAULT_SSID_PREFIX + std::to_string(index);

    macHelper.SetType("ns3::AdhocWifiMac",
                      "Ssid",
                      SsidValue(Ssid(ssidStr)),
                      "BeaconGeneration",
                      BooleanValue(consts::DEFAULT_BEACON_GEN));
    return macHelper;
}

Ptr<WifiNetDevice>
WifiStaticAdhocBeaconsTest::GetWifiNetDevice(size_t index, const ChannelMap& channelMap)
{
    NodeContainer node(1);
    auto wifiHelper = GetWifiHelper();
    const auto& input = m_testVec.m_input;
    auto settings = StringVector{input.adhocChs[index]};
    auto phyHelper = GetPhyHelper(settings, channelMap);
    auto macHelper = GetAdhocMacHelper(index);
    auto netDevs = wifiHelper.Install(phyHelper, macHelper, node);
    WifiHelper::AssignStreams(netDevs, consts::DEFAULT_STREAM_INDEX);
    return DynamicCast<WifiNetDevice>(netDevs.Get(0));
}

void
WifiStaticAdhocBeaconsTest::DoSetup()
{
    RngSeedManager::SetSeed(consts::DEFAULT_RNG_SEED);
    RngSeedManager::SetRun(consts::DEFAULT_RNG_RUN);

    ChannelMap channelMap = {{WIFI_PHY_BAND_2_4GHZ, CreateObject<MultiModelSpectrumChannel>()},
                             {WIFI_PHY_BAND_5GHZ, CreateObject<MultiModelSpectrumChannel>()},
                             {WIFI_PHY_BAND_6GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    for (size_t i = 0; i < m_testVec.m_input.adhocChs.size(); ++i)
    {
        m_devs.push_back(GetWifiNetDevice(i, channelMap));
    }

    WifiStaticSetupHelper::SetStaticAssociation(m_devs[0], m_devs[1]);
}

void
WifiStaticAdhocBeaconsTest::DoRun()
{
    Simulator::Stop(consts::DEFAULT_SIM_STOP_TIME);
    Simulator::Run();
    ValidateRecords();
    Simulator::Destroy();
}

void
WifiStaticAdhocBeaconsTest::ValidateRecords()
{
    auto adhocStaMgr0 = m_devs[0]->GetRemoteStationManager(SINGLE_LINK_OP_ID);
    auto adhocAddr0 = m_devs[0]->GetMac()->GetAddress();
    auto adhocStaMgr1 = m_devs[1]->GetRemoteStationManager(SINGLE_LINK_OP_ID);
    auto adhocAddr1 = m_devs[1]->GetMac()->GetAddress();

    // Validate Adhoc device 1 record at Adhoc device 0
    NS_TEST_ASSERT_MSG_EQ(adhocStaMgr0->IsAdhocPeer(adhocAddr1),
                          m_testVec.m_expected.exchange,
                          "Unexpected Adhoc Peer record at device 0 for device 1");
    NS_TEST_ASSERT_MSG_EQ(adhocStaMgr0->GetEhtSupported(adhocAddr1),
                          m_testVec.m_expected.exchange,
                          "Unexpected EHT Supported record at device 0 for device 1");

    // Validate Adhoc device 0 record at Adhoc device 1
    NS_TEST_ASSERT_MSG_EQ(adhocStaMgr1->IsAdhocPeer(adhocAddr0),
                          m_testVec.m_expected.exchange,
                          "Unexpected Adhoc Peer record at device 1 for device 0");
    NS_TEST_ASSERT_MSG_EQ(adhocStaMgr1->GetEhtSupported(adhocAddr0),
                          m_testVec.m_expected.exchange,
                          "Unexpected EHT Supported record at device 1 for device 0");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Adhoc beacons static setup test suite
 */
class WifiStaticAdhocBeaconsTestSuite : public TestSuite
{
  public:
    WifiStaticAdhocBeaconsTestSuite();
};

WifiStaticAdhocBeaconsTestSuite::WifiStaticAdhocBeaconsTestSuite()
    : TestSuite("wifi-static-adhoc-beacons-test", Type::UNIT)
{
    const std::vector<WifiStaticAdhocBeaconsTestVector> testVectors = {
        {.m_input = {"Match", {"{36, 0, BAND_5GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"}},
         .m_expected = {true}},
        {.m_input = {"Match-Different-BW", {"{36, 0, BAND_5GHZ, 0}", "{42, 0, BAND_5GHZ, 0}"}},
         .m_expected = {true}},
        {.m_input = {"No-Match", {"{36, 0, BAND_5GHZ, 0}", "{56, 0, BAND_5GHZ, 0}"}},
         .m_expected = {false}}};

    for (const auto& testVec : testVectors)
    {
        AddTestCase(new WifiStaticAdhocBeaconsTest(testVec), TestCase::Duration::QUICK);
    }
}

static WifiStaticAdhocBeaconsTestSuite g_WifiStaticAdhocBeaconsTestSuite;
