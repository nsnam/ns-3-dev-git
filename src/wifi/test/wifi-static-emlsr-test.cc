/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/emlsr-manager.h"
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

#include <algorithm>
#include <iterator>
#include <set>
#include <unordered_map>

/// @ingroup wifi-test
/// @ingroup tests
/// @brief WifiStaticSetupHelper EMLSR setup test suite
/// Test suite intended to test static EMLSR setup between AP MLD and client MLD.
/// The test prepares AP WifiNetDevice and client WifiNetDevice based on test vector input and
/// performs static EMLSR setup using WifiStaticSetupHelper. The test verifies if EMLSR state
/// machine at ApWifiMac and StaWifiMac has been updated correctly.

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WifiStaticEmlsrTestSuite");

namespace WifiStaticEmlsrTestConstants
{
const auto DEFAULT_RNG_SEED = 3;
const auto DEFAULT_RNG_RUN = 7;
const auto DEFAULT_STREAM_INDEX = 100;
const auto DEFAULT_SIM_STOP_TIME = NanoSeconds(1);
const auto DEFAULT_BEACON_GEN = false;
const auto DEFAULT_DATA_MODE = "HeMcs3";
const auto DEFAULT_CONTROL_MODE = "OfdmRate24Mbps";
const auto DEFAULT_SWITCH_DELAY = MicroSeconds(64);
const auto DEFAULT_AUX_PHY_CH_WIDTH = MHz_u{20};
const auto DEFAULT_SWITCH_AUX_PHY = false;
const auto DEFAULT_WIFI_STANDARD = WifiStandard::WIFI_STANDARD_80211be;
const auto DEFAULT_SSID = Ssid("static-assoc-test");
const std::string CHANNEL_0 = "{42, 80, BAND_5GHZ, 0}";
const std::string CHANNEL_1 = "{23, 80, BAND_6GHZ, 0}";
const std::string CHANNEL_2 = "{2, 0, BAND_2_4GHZ, 0}";
const std::vector<std::string> DEFAULT_AP_CHS = {CHANNEL_0, CHANNEL_1, CHANNEL_2};
using ChannelMap = std::unordered_map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>>;
} // namespace WifiStaticEmlsrTestConstants

/// @brief test case information
struct WifiStaticEmlsrTestVector
{
    std::string name;                     ///< Test case name
    std::vector<std::string> clientChs{}; ///< Channel settings for client device
    std::set<uint8_t> emlsrLinks{};       ///< EMLSR mode links
    Time switchDelay{WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY}; ///< Radio Switch Delay
    MHz_u auxPhyWidth{
        WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH}; ///< Aux PHY channel width
    bool switchAuxPhy{WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY}; ///< Switch Aux PHY
    std::vector<WifiPowerManagementMode> pmModes; ///< if non-empty, PM mode for each STA affiliated
                                                  ///< with the client in increasing order of client
                                                  ///< link ID (as assigned before association); if
                                                  ///< empty, no PowerSave manager is installed
};

/**
 * Test static setup of the EMLSR mode.
 *
 * It is checked that:
 * - EMLSR mode is enabled on the expected set of links, both at client side and AP MLD side
 * - the channel switch delay is configured on the client links as expected
 */
class WifiStaticEmlsrTest : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param testVec the test vector
     */
    WifiStaticEmlsrTest(const WifiStaticEmlsrTestVector& testVec);

  private:
    /// Construct WifiNetDevice
    /// @param isAp true if AP, false otherwise
    /// @param channelMap created spectrum channels
    /// @return constructed WifiNetDevice
    Ptr<WifiNetDevice> GetWifiNetDevice(bool isAp,
                                        const WifiStaticEmlsrTestConstants::ChannelMap& channelMap);

    /// Construct PHY helper based on input operating channels
    /// @param settings vector of strings specifying the operating channels to configure
    /// @param channelMap created spectrum channels
    /// @return PHY helper
    SpectrumWifiPhyHelper GetPhyHelper(
        const std::vector<std::string>& settings,
        const WifiStaticEmlsrTestConstants::ChannelMap& channelMap) const;

    /// @return the WifiHelper
    WifiHelper GetWifiHelper() const;
    /// @return the AP MAC helper
    WifiMacHelper GetApMacHelper() const;
    /// @return the Client MAC helper
    WifiMacHelper GetClientMacHelper() const;
    void ValidateEmlsr(); ///< Validate EMLSR setup
    void DoRun() override;
    void DoSetup() override;

    /// Validate the configured PM mode for the STA(s) affiliated with the client device
    /// @param apMac AP MAC
    /// @param clientMac Client MAC
    void ValidatePmMode(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac);

    WifiStaticEmlsrTestVector m_testVec;      ///< Test vector
    Ptr<WifiNetDevice> m_apDev{nullptr};      ///< AP WiFi device
    Ptr<WifiNetDevice> m_clientDev{nullptr};  ///< client WiFi device
    std::map<linkId_t, linkId_t> m_linkIdMap; ///< non-AP MLD link ID to AP MLD link ID mapping
};

WifiStaticEmlsrTest::WifiStaticEmlsrTest(const WifiStaticEmlsrTestVector& testVec)
    : TestCase(testVec.name),
      m_testVec(testVec)
{
}

WifiHelper
WifiStaticEmlsrTest::GetWifiHelper() const
{
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(WifiStaticEmlsrTestConstants::DEFAULT_WIFI_STANDARD);
    wifiHelper.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode",
        StringValue(WifiStaticEmlsrTestConstants::DEFAULT_DATA_MODE),
        "ControlMode",
        StringValue(WifiStaticEmlsrTestConstants::DEFAULT_CONTROL_MODE));
    wifiHelper.ConfigEhtOptions("EmlsrActivated", BooleanValue(true));
    return wifiHelper;
}

SpectrumWifiPhyHelper
WifiStaticEmlsrTest::GetPhyHelper(const std::vector<std::string>& settings,
                                  const WifiStaticEmlsrTestConstants::ChannelMap& channelMap) const
{
    NS_ASSERT(not settings.empty());
    SpectrumWifiPhyHelper helper(settings.size());

    uint8_t linkId = 0;
    for (const auto& str : settings)
    {
        helper.Set(linkId, "ChannelSettings", StringValue(str));
        helper.Set(linkId, "ChannelSwitchDelay", TimeValue(m_testVec.switchDelay));
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
WifiStaticEmlsrTest::GetApMacHelper() const
{
    WifiMacHelper macHelper;
    auto ssid = Ssid(WifiStaticEmlsrTestConstants::DEFAULT_SSID);

    macHelper.SetType("ns3::ApWifiMac",
                      "Ssid",
                      SsidValue(ssid),
                      "BeaconGeneration",
                      BooleanValue(WifiStaticEmlsrTestConstants::DEFAULT_BEACON_GEN));
    return macHelper;
}

WifiMacHelper
WifiStaticEmlsrTest::GetClientMacHelper() const
{
    WifiMacHelper macHelper;
    Ssid ssid = Ssid(WifiStaticEmlsrTestConstants::DEFAULT_SSID);
    macHelper.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    macHelper.SetEmlsrManager("ns3::DefaultEmlsrManager",
                              "EmlsrLinkSet",
                              AttributeContainerValue<UintegerValue>(m_testVec.emlsrLinks),
                              "AuxPhyChannelWidth",
                              UintegerValue(m_testVec.auxPhyWidth),
                              "SwitchAuxPhy",
                              BooleanValue(m_testVec.switchAuxPhy));
    // install and configure a PowerSave manager if PM modes are provided
    if (!m_testVec.pmModes.empty())
    {
        std::string s;
        for (std::size_t id = 0; const auto pmMode : m_testVec.pmModes)
        {
            s += std::to_string(id++) + (pmMode == WIFI_PM_POWERSAVE ? " true, " : " false, ");
        }
        s.pop_back();
        s.pop_back();
        macHelper.SetPowerSaveManager("ns3::DefaultPowerSaveManager",
                                      "PowerSaveMode",
                                      StringValue(s));
    }
    return macHelper;
}

Ptr<WifiNetDevice>
WifiStaticEmlsrTest::GetWifiNetDevice(bool isAp,
                                      const WifiStaticEmlsrTestConstants::ChannelMap& channelMap)
{
    NodeContainer node(1);
    auto wifiHelper = GetWifiHelper();
    auto settings = isAp ? WifiStaticEmlsrTestConstants::DEFAULT_AP_CHS : m_testVec.clientChs;
    auto phyHelper = GetPhyHelper(settings, channelMap);
    auto macHelper = isAp ? GetApMacHelper() : GetClientMacHelper();
    auto netDev = wifiHelper.Install(phyHelper, macHelper, node);
    WifiHelper::AssignStreams(netDev, WifiStaticEmlsrTestConstants::DEFAULT_STREAM_INDEX);
    return DynamicCast<WifiNetDevice>(netDev.Get(0));
}

void
WifiStaticEmlsrTest::DoSetup()
{
    RngSeedManager::SetSeed(WifiStaticEmlsrTestConstants::DEFAULT_RNG_SEED);
    RngSeedManager::SetRun(WifiStaticEmlsrTestConstants::DEFAULT_RNG_RUN);

    WifiStaticEmlsrTestConstants::ChannelMap channelMap = {
        {WIFI_PHY_BAND_2_4GHZ, CreateObject<MultiModelSpectrumChannel>()},
        {WIFI_PHY_BAND_5GHZ, CreateObject<MultiModelSpectrumChannel>()},
        {WIFI_PHY_BAND_6GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    NS_TEST_ASSERT_MSG_EQ(
        (m_testVec.pmModes.empty() || (m_testVec.pmModes.size() == m_testVec.clientChs.size())),
        true,
        "PM modes (" << m_testVec.pmModes.size() << ") and link (" << m_testVec.clientChs.size()
                     << ") mismatch");

    m_apDev = GetWifiNetDevice(true, channelMap);
    NS_ASSERT(m_apDev);
    m_clientDev = GetWifiNetDevice(false, channelMap);
    NS_ASSERT(m_clientDev);

    m_linkIdMap = WifiStaticSetupHelper::GetLinkIdMap(m_apDev, m_clientDev);

    WifiStaticSetupHelper::SetStaticAssociation(m_apDev, m_clientDev);
    WifiStaticSetupHelper::SetStaticEmlsr(m_apDev, m_clientDev);
}

void
WifiStaticEmlsrTest::ValidateEmlsr()
{
    auto clientMac = DynamicCast<StaWifiMac>(m_clientDev->GetMac());
    NS_ASSERT_MSG(clientMac, "Expected StaWifiMac");
    NS_TEST_ASSERT_MSG_EQ(clientMac->IsAssociated(), true, "Expected non-AP MLD to be associated");
    auto setupLinks = clientMac->GetSetupLinkIds();
    auto isMldAssoc = (setupLinks.size() > 1);
    NS_TEST_ASSERT_MSG_EQ(isMldAssoc, true, "EMLSR mode requires association on multiple links");
    auto emlsrManager = clientMac->GetEmlsrManager();
    NS_ASSERT_MSG(emlsrManager, "EMLSR Manager not set");
    auto clientEmlsrLinks = emlsrManager->GetEmlsrLinks();
    auto match = (clientEmlsrLinks == m_testVec.emlsrLinks);
    NS_TEST_ASSERT_MSG_EQ(match, true, "Unexpected set of EMLSR links enabled");
    auto apMac = DynamicCast<ApWifiMac>(m_apDev->GetMac());
    NS_TEST_ASSERT_MSG_NE(apMac, nullptr, "Expected ApWifiMac");
    for (auto linkId : setupLinks)
    {
        auto expectedState = clientEmlsrLinks.contains(linkId);
        auto clientLinkAddr = clientMac->GetFrameExchangeManager(linkId)->GetAddress();
        auto apManager = m_apDev->GetRemoteStationManager(linkId);
        auto actualState = apManager->GetEmlsrEnabled(clientLinkAddr);
        NS_TEST_ASSERT_MSG_EQ(actualState,
                              expectedState,
                              "EMLSR state mismatch on client link ID " << +linkId);

        // Validate Channel switch delay
        auto actualDelay = clientMac->GetWifiPhy(linkId)->GetChannelSwitchDelay();
        NS_TEST_ASSERT_MSG_EQ(actualDelay,
                              m_testVec.switchDelay,
                              "Channel switch delay mismatch on client link ID " << +linkId);
    }

    ValidatePmMode(apMac, clientMac);
}

void
WifiStaticEmlsrTest::ValidatePmMode(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac)
{
    const auto& clientLinkIds = clientMac->GetLinkIds();
    // if PM modes are not provided (hence, no PowerSave manager is installed), we expect all
    // STAs affiliated with the client to be in active mode
    auto pmModes = !m_testVec.pmModes.empty()
                       ? m_testVec.pmModes
                       : std::vector<WifiPowerManagementMode>(clientLinkIds.size(), WIFI_PM_ACTIVE);

    NS_TEST_ASSERT_MSG_EQ(pmModes.size(), clientLinkIds.size(), "Number of PM modes mismatch");
    for (std::size_t id = 0; const auto pmMode : pmModes)
    {
        NS_TEST_ASSERT_MSG_EQ(m_linkIdMap.contains(id),
                              true,
                              "Non-AP MLD did not have link " << id << " before association");
        const auto linkId = m_linkIdMap.at(id);
        NS_TEST_EXPECT_MSG_EQ(
            clientMac->GetPmMode(linkId),
            pmMode,
            "Unexpected PM mode for STA affiliated with the non-AP MLD and operating on link "
                << +linkId << "(non-AP MLD side)");

        NS_TEST_EXPECT_MSG_EQ(
            apMac->GetWifiRemoteStationManager(linkId)->IsInPsMode(clientMac->GetAddress()),
            (pmMode == WIFI_PM_POWERSAVE),
            "Unexpected PM mode for STA affiliated with the non-AP MLD and operating on link "
                << +linkId << "(AP MLD side)");

        NS_TEST_EXPECT_MSG_EQ(apMac->GetNStationsInPsMode(linkId),
                              (pmMode == WIFI_PM_POWERSAVE ? 1 : 0),
                              "AP is tracking an unexpected number of STAs in PS mode on link "
                                  << +linkId);
        ++id;
    }
}

void
WifiStaticEmlsrTest::DoRun()
{
    Simulator::Stop(WifiStaticEmlsrTestConstants::DEFAULT_SIM_STOP_TIME);
    Simulator::Run();
    ValidateEmlsr();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief EMLSR static setup test suite
 */
class WifiStaticEmlsrTestSuite : public TestSuite
{
  public:
    WifiStaticEmlsrTestSuite();
};

WifiStaticEmlsrTestSuite::WifiStaticEmlsrTestSuite()
    : TestSuite("wifi-static-emlsr-test", Type::UNIT)
{
    auto CHANNELS_2_LINKS = {WifiStaticEmlsrTestConstants::CHANNEL_0,
                             WifiStaticEmlsrTestConstants::CHANNEL_1};
    auto CHANNELS_3_LINKS = {WifiStaticEmlsrTestConstants::CHANNEL_0,
                             WifiStaticEmlsrTestConstants::CHANNEL_1,
                             WifiStaticEmlsrTestConstants::CHANNEL_2};
    auto CHANNELS_2_LINKS_ALT = {WifiStaticEmlsrTestConstants::CHANNEL_0,
                                 WifiStaticEmlsrTestConstants::CHANNEL_2};

    for (const std::vector<WifiStaticEmlsrTestVector> inputs{
             {"Setup-2-link-EMLSR-2-link",
              CHANNELS_2_LINKS,
              {0, 1},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {}},
             {"Setup-3-link-EMLSR-2-link",
              CHANNELS_3_LINKS,
              {0, 1},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {}},
             {"Setup-3-link-EMLSR-2-link-Diff",
              CHANNELS_3_LINKS,
              {1, 2},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {WIFI_PM_ACTIVE, WIFI_PM_ACTIVE, WIFI_PM_ACTIVE}},
             {"Setup-3-link-EMLSR-2-link-Diff-2",
              CHANNELS_3_LINKS,
              {0, 2},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {WIFI_PM_ACTIVE, WIFI_PM_POWERSAVE, WIFI_PM_ACTIVE}},
             {"Setup-3-link-EMLSR-3-link",
              CHANNELS_3_LINKS,
              {0, 1, 2},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {WIFI_PM_ACTIVE, WIFI_PM_POWERSAVE, WIFI_PM_ACTIVE}},
             {"Setup-2-link-EMLSR-2-link-Diff-Set",
              CHANNELS_2_LINKS_ALT,
              {0, 2},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {WIFI_PM_POWERSAVE, WIFI_PM_POWERSAVE}},
             {"EMLSR-2-link-16us-delay",
              CHANNELS_2_LINKS,
              {0, 1},
              MicroSeconds(16),
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {WIFI_PM_POWERSAVE, WIFI_PM_ACTIVE}},
             {"EMLSR-2-link-32us-delay",
              CHANNELS_2_LINKS,
              {0, 1},
              MicroSeconds(32),
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {WIFI_PM_ACTIVE, WIFI_PM_POWERSAVE}},
             {"EMLSR-2-link-80MHz-AuxPhy",
              CHANNELS_2_LINKS,
              {0, 1},
              MicroSeconds(32),
              MHz_u{80},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_AUX_PHY,
              {WIFI_PM_ACTIVE, WIFI_PM_ACTIVE}},
             {"EMLSR-2-link-Switch-Aux-PHY",
              CHANNELS_2_LINKS,
              {0, 1},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              WifiStaticEmlsrTestConstants::DEFAULT_AUX_PHY_CH_WIDTH,
              true,
              {WIFI_PM_POWERSAVE, WIFI_PM_POWERSAVE}},
             {"EMLSR-2-link-80MHz-AuxPhy-Switch",
              CHANNELS_2_LINKS,
              {0, 1},
              WifiStaticEmlsrTestConstants::DEFAULT_SWITCH_DELAY,
              MHz_u{80},
              true,
              {}}};
         const auto& input : inputs)
    {
        AddTestCase(new WifiStaticEmlsrTest(input), TestCase::Duration::QUICK);
    }
}

static WifiStaticEmlsrTestSuite g_wifiStaticEmlsrTestSuite;
