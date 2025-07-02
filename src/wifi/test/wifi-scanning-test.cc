/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/boolean.h"
#include "ns3/interference-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-net-device.h"

using namespace ns3;

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the setting of the ScanningChannels attribute of StaWifiMac.
 *
 */
class WifiScanningChannelsAttributeTest : public TestCase
{
  public:
    WifiScanningChannelsAttributeTest();

  private:
    void DoRun() override;

    /// Check that the given data structure contains an entry for the given PHY ID and such entry
    /// contains an entry for the given PHY band (if any) with the given channel list (if any)
    /// @param context a message indicating the context for this call
    /// @param scanChannels the given data structure containing channels to scan
    /// @param phyId the given PHY ID
    /// @param nPhyBandEntries the number of entries in the PHY band-indexed map of channels for
    ///                        the given PHY ID
    /// @param band the given PHY band
    /// @param list the given channel list
    void CheckChannelListPresent(const std::string& context,
                                 const WifiScanParams::Map& scanChannels,
                                 uint8_t phyId,
                                 std::size_t nPhyBandEntries,
                                 std::optional<WifiPhyBand> band = std::nullopt,
                                 const WifiScanParams::ChannelList::mapped_type& list = {});
};

WifiScanningChannelsAttributeTest::WifiScanningChannelsAttributeTest()
    : TestCase("Test the ScanningChannels attribute setting")
{
}

void
WifiScanningChannelsAttributeTest::CheckChannelListPresent(
    const std::string& context,
    const WifiScanParams::Map& scanChannels,
    uint8_t phyId,
    std::size_t nPhyBandEntries,
    std::optional<WifiPhyBand> band,
    const WifiScanParams::ChannelList::mapped_type& list)
{
    NS_TEST_ASSERT_MSG_EQ(scanChannels.contains(phyId),
                          true,
                          context << "No entry for PHY ID " << +phyId);

    const auto& phyBandChannelsMap = scanChannels.at(phyId);

    NS_TEST_EXPECT_MSG_EQ(
        phyBandChannelsMap.size(),
        nPhyBandEntries,
        context << "Unexpected size for the PHY band-indexed map of channels for PHY ID "
                << +phyId);

    if (!band.has_value())
    {
        NS_TEST_EXPECT_MSG_EQ(phyBandChannelsMap.empty(),
                              true,
                              context << "Expected empty map for PHY ID " << +phyId);
        return;
    }

    NS_TEST_ASSERT_MSG_EQ(phyBandChannelsMap.contains(*band),
                          true,
                          context << "Expected map for PHY ID " << +phyId << " to contain band "
                                  << *band);

    NS_TEST_EXPECT_MSG_EQ((phyBandChannelsMap.at(*band) == list),
                          true,
                          context << "Unexpected channel list for band " << *band << " and PHY ID "
                                  << +phyId);
}

void
WifiScanningChannelsAttributeTest::DoRun()
{
    ObjectFactory macFactory;

    // Create a StaWifiMac object with ScanningChannels set to empty string
    macFactory.SetTypeId("ns3::StaWifiMac");
    macFactory.Set("QosSupported", BooleanValue(true));
    for (const auto aci : edcaAcIndices)
    {
        std::stringstream ss;
        ss << aci << "_Txop";
        auto s = ss.str().substr(3); // discard "AC "
        auto edca = CreateObjectWithAttributes<QosTxop>("AcIndex", EnumValue<AcIndex>(aci));
        macFactory.Set(s, PointerValue(edca));
    }
    macFactory.Set("ScanningChannels", StringValue(""));

    auto staMac = macFactory.Create<StaWifiMac>();
    const auto& scanChannels = staMac->m_scanChannels;

    /**
     * Empty string, before installing PHYs.
     *
     * ExpandScanningChannelsMap() does nothing and m_scanChannels stays empty.
     */
    NS_TEST_EXPECT_MSG_EQ(scanChannels.empty(),
                          true,
                          "Expected empty data structure for channels to scan");

    /**
     * Non-empty string, before installing PHYs.
     *
     * Lists of scanning channels are not expanded yet.
     */
    std::string context{"Non-empty string, channel lists not expanded: "};
    std::string channelsStr{"0: INVALID 0 | 1: | 2: 5GHz 104,108; 6GHz 0"};
    staMac->SetAttribute("ScanningChannels", StringValue(channelsStr));

    NS_TEST_ASSERT_MSG_EQ(scanChannels.size(), 3, context << "Expected scanning info for 3 PHYs");

    CheckChannelListPresent(context, scanChannels, 0, 1, WIFI_PHY_BAND_UNSPECIFIED, {0});
    CheckChannelListPresent(context, scanChannels, 1, 0);
    CheckChannelListPresent(context, scanChannels, 2, 2, WIFI_PHY_BAND_5GHZ, {104, 108});
    CheckChannelListPresent(context, scanChannels, 2, 2, WIFI_PHY_BAND_6GHZ, {0});

    // Test string serialization
    auto str = StaWifiMac::ScanningChannelsToStr(scanChannels);
    NS_TEST_EXPECT_MSG_EQ(channelsStr, str, "Serialization failed");

    // Install four PHY objects
    std::vector<Ptr<WifiPhy>> phys(4);
    const std::vector<WifiPhy::ChannelTuple> opChannels{{4, 20, WIFI_PHY_BAND_2_4GHZ, 0},
                                                        {38, 40, WIFI_PHY_BAND_5GHZ, 1},
                                                        {23, 80, WIFI_PHY_BAND_6GHZ, 2},
                                                        {153, 20, WIFI_PHY_BAND_6GHZ, 0}};

    for (std::size_t id = 0; id < 4; ++id)
    {
        auto phy = CreateObject<SpectrumWifiPhy>();
        phy->SetInterferenceHelper(CreateObject<InterferenceHelper>());
        phy->AddChannel(CreateObject<MultiModelSpectrumChannel>());
        phy->SetOperatingChannel(opChannels[id]);
        phy->ConfigureStandard(WIFI_STANDARD_80211be);
        phys[id] = phy;
    }

    staMac->SetWifiPhys(phys);

    auto device = CreateObject<WifiNetDevice>();
    staMac->SetDevice(device);
    device->SetEhtConfiguration(CreateObject<EhtConfiguration>());
    device->SetMac(staMac);
    device->SetPhys(phys);

    const WifiScanParams::ChannelList::mapped_type all2_4GHzChannels =
        {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    const WifiScanParams::ChannelList::mapped_type all5GHzChannels = {
        36,  40,  44,  48,  52,  56,  60,  64,  100, 104, 108, 112, 116, 120, 124,
        128, 132, 136, 140, 144, 149, 153, 157, 161, 165, 169, 173, 177, 181};
    const WifiScanParams::ChannelList::mapped_type all6GHzChannels = {
        1,   5,   9,   13,  17,  21,  25,  29,  33,  37,  41,  45,  49,  53,  57,
        61,  65,  69,  73,  77,  81,  85,  89,  93,  97,  101, 105, 109, 113, 117,
        121, 125, 129, 133, 137, 141, 145, 149, 153, 157, 161, 165, 169, 173, 177,
        181, 185, 189, 193, 197, 201, 205, 209, 213, 217, 221, 225, 229, 233};

    /**
     * Non-empty string, lists of scanning channels are expanded.
     */
    context = "Non-empty string, channel lists expanded: ";
    staMac->ExpandScanningChannelsMap();
    auto scanChannelsE = staMac->SetCurrentChannelsForScanning();

    NS_TEST_ASSERT_MSG_EQ(scanChannels.size(), 3, context << "Expected scanning info for 3 PHYs");

    // The unspecified PHY band for PHY ID 0 is replaced by all 20 MHz channels in all bands
    CheckChannelListPresent(context, scanChannelsE, 0, 3, WIFI_PHY_BAND_2_4GHZ, all2_4GHzChannels);
    CheckChannelListPresent(context, scanChannelsE, 0, 3, WIFI_PHY_BAND_5GHZ, all5GHzChannels);
    CheckChannelListPresent(context, scanChannelsE, 0, 3, WIFI_PHY_BAND_6GHZ, all6GHzChannels);

    // the empty map for PHY ID 1 is replaced by the current primary20 channel
    CheckChannelListPresent(context, scanChannelsE, 1, 1, WIFI_PHY_BAND_5GHZ, {40});

    // PHY ID 2 uses requested channels in the 5 GHz band and all channels in the 6 GHz band
    CheckChannelListPresent(context, scanChannelsE, 2, 2, WIFI_PHY_BAND_5GHZ, {104, 108});
    CheckChannelListPresent(context, scanChannelsE, 2, 2, WIFI_PHY_BAND_6GHZ, all6GHzChannels);

    /**
     * Empty string, scanning channels are expanded.
     */
    context = "Empty string, channel lists expanded: ";
    staMac->SetAttribute("ScanningChannels", StringValue(""));
    staMac->ExpandScanningChannelsMap();
    scanChannelsE = staMac->SetCurrentChannelsForScanning();

    // all 4 PHYs use the current primary20 channel
    CheckChannelListPresent(context, scanChannelsE, 0, 1, WIFI_PHY_BAND_2_4GHZ, {4});
    CheckChannelListPresent(context, scanChannelsE, 1, 1, WIFI_PHY_BAND_5GHZ, {40});
    CheckChannelListPresent(context, scanChannelsE, 2, 1, WIFI_PHY_BAND_6GHZ, {25});
    CheckChannelListPresent(context, scanChannelsE, 3, 1, WIFI_PHY_BAND_6GHZ, {153});

    device->Dispose();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi scanning Test Suite
 */
class WifiScanningTestSuite : public TestSuite
{
  public:
    WifiScanningTestSuite();
};

WifiScanningTestSuite::WifiScanningTestSuite()
    : TestSuite("wifi-scanning-test", Type::UNIT)
{
    AddTestCase(new WifiScanningChannelsAttributeTest, TestCase::Duration::QUICK);
}

static WifiScanningTestSuite g_wifiScanningTestSuite; ///< the test suite
