/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-dso-test.h"

#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/default-dso-manager.h"
#include "ns3/eht-configuration.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/string.h"
#include "ns3/vht-configuration.h"
#include "ns3/wifi-net-device.h"

#include <algorithm>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiDsoTest");

/**
 * Extended Default DSO manager class for the purpose of the tests.
 */
class TestDsoManager : public DefaultDsoManager
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId()
    {
        static TypeId tid = TypeId("ns3::TestDsoManager")
                                .SetParent<DefaultDsoManager>()
                                .SetGroupName("Wifi")
                                .AddConstructor<TestDsoManager>();
        return tid;
    }

    using DefaultDsoManager::GetDsoSubbands;
    using DefaultDsoManager::GetPrimarySubband;
};

NS_OBJECT_ENSURE_REGISTERED(TestDsoManager);

DsoTestBase::DsoTestBase(const std::string& name)
    : TestCase(name)
{
}

void
DsoTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(10);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes;

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("UhrMcs0"),
                                 "ControlMode",
                                 StringValue("UhrMcs0"));

    SpectrumWifiPhyHelper stasPhyHelper;
    stasPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    auto channel = CreateObject<MultiModelSpectrumChannel>();
    for (const auto& freqRange : m_stasFreqRanges)
    {
        stasPhyHelper.AddChannel(channel, freqRange);
    }
    stasPhyHelper.Set("ChannelSettings", StringValue(m_stasOpChannel));

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(Ssid("ns-3-ssid")));
    mac.SetDsoManager("ns3::TestDsoManager");

    NetDeviceContainer staDevices;
    for (std::size_t i = 0; i < m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas; ++i)
    {
        wifi.ConfigUhrOptions("DsoActivated", BooleanValue(i < m_nDsoStas));
        NodeContainer staNode(1);
        wifi.SetStandard(i < (m_nDsoStas + m_nNonDsoStas) ? WIFI_STANDARD_80211bn
                                                          : WIFI_STANDARD_80211be);
        staDevices.Add(wifi.Install(stasPhyHelper, mac, staNode));
        wifiStaNodes.Add(staNode);
    }

    for (uint32_t i = 0; i < staDevices.GetN(); i++)
    {
        auto staDevice = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        m_staMacs.push_back(DynamicCast<StaWifiMac>(staDevice->GetMac()));
    }

    SpectrumWifiPhyHelper apPhyHelper;
    apPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    apPhyHelper.SetChannel(channel);

    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

    apPhyHelper.Set("ChannelSettings", StringValue(m_apOpChannel));
    wifi.SetStandard(WIFI_STANDARD_80211bn);
    auto apDevice = wifi.Install(apPhyHelper, mac, wifiApNode);
    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac());

    // Uncomment the lines below to write PCAP files
    // apPhyHelper.EnablePcap("wifi-dso_AP", apDevice);
    // stasPhyHelper.EnablePcap("wifi-dso_STA", staDevices);

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    for (std::size_t i = 0; i <= m_nDsoStas + m_nNonDsoStas + m_nNonUhrStas; ++i)
    {
        positionAlloc->Add(Vector(i, 0.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);
}

DsoSubbandsTest::DsoSubbandsTest(
    const std::string& apChannel,
    const std::string& stasChannel,
    const std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel>& expectedDsoSubbands)
    : DsoTestBase("Check computation of DSO subbands: AP operating channel is " + apChannel +
                  " and STA operating channel is " + stasChannel),
      m_expectedDsoSubbands{expectedDsoSubbands}
{
    m_nDsoStas = 1;
    m_apOpChannel = apChannel;
    m_stasOpChannel = stasChannel;
    m_duration = Seconds(0.5);
    std::set<FrequencyRange> dsoFreqRanges;
    std::transform(m_expectedDsoSubbands.cbegin(),
                   m_expectedDsoSubbands.cend(),
                   std::inserter(dsoFreqRanges, dsoFreqRanges.end()),
                   [](const auto& pair) -> FrequencyRange {
                       return {pair.second.GetFrequency() - pair.second.GetWidth() / 2,
                               pair.second.GetFrequency() + pair.second.GetWidth() / 2};
                   });
    m_stasFreqRanges = {{MHz_t{2401}, dsoFreqRanges.cbegin()->minFrequency},
                        {dsoFreqRanges.crbegin()->maxFrequency, MHz_t{7125}}};
    auto prevStopFreq = dsoFreqRanges.cbegin()->minFrequency;
    for (const auto& freqRange : dsoFreqRanges)
    {
        if (prevStopFreq < freqRange.minFrequency)
        {
            m_stasFreqRanges.emplace_back(prevStopFreq, freqRange.minFrequency);
        }
        prevStopFreq = freqRange.maxFrequency;
        m_stasFreqRanges.emplace_back(freqRange.minFrequency, freqRange.maxFrequency);
    }
}

void
DsoSubbandsTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    const auto dsoManager = DynamicCast<TestDsoManager>(m_staMacs.at(0)->GetDsoManager());
    NS_TEST_EXPECT_MSG_EQ(dsoManager->GetPrimarySubband(SINGLE_LINK_OP_ID),
                          m_staMacs.at(0)->GetWifiPhy()->GetOperatingChannel(),
                          "Unexpected primary subband");
    const auto dsoSubbands = dsoManager->GetDsoSubbands(SINGLE_LINK_OP_ID);
    NS_TEST_EXPECT_MSG_EQ(dsoSubbands.size(),
                          m_expectedDsoSubbands.size(),
                          "Unexpected number of DSO subbands");

    for (const auto& [primaryFlag, expectedSubband] : m_expectedDsoSubbands)
    {
        std::ostringstream oss;
        oss << "DSO subband '" << expectedSubband << "' not found for " << primaryFlag;
        NS_TEST_EXPECT_MSG_EQ(dsoSubbands.count(primaryFlag), 1, oss.str());
        oss.str("");
        oss << "Unexpected DSO subband '" << dsoSubbands.at(primaryFlag) << "' for " << primaryFlag;
        NS_TEST_EXPECT_MSG_EQ(dsoSubbands.at(primaryFlag), expectedSubband, oss.str());
    }

    Simulator::Destroy();
}

WifiDsoTestSuite::WifiDsoTestSuite()
    : TestSuite("wifi-dso", Type::UNIT)
{
    WifiPhyOperatingChannel expectedChannel;

    /**
     * AP operating on 160 MHz in 5 GHz band with P80 being the lower 80 MHz, DSO STA supports up to
     * 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 106(P80)│CH 122(S80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │  PRIMARY  │    DSO    │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{122, MHz_t{5610}, MHz_t{80}, WIFI_PHY_BAND_5GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{114, 0, BAND_5GHZ, 0}",
                                    "{106, 0, BAND_5GHZ, 0}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 160 MHz in 5 GHz band with P80 being the higher 80 MHz, DSO STA supports up
     * to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 155(S80)│CH 171(P80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │   DSO     │  PRIMARY  │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{155, MHz_t{5775}, MHz_t{80}, WIFI_PHY_BAND_5GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{163, 0, BAND_5GHZ, 7}",
                                    "{171, 0, BAND_5GHZ, 3}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 160 MHz in 6 GHz band with P80 being the lower 80 MHz, DSO STA supports up to
     * 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 103(P80)│CH 119(S80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │  PRIMARY  │    DSO    │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{119, MHz_t{6545}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{111, 0, BAND_6GHZ, 2}",
                                    "{103, 0, BAND_6GHZ, 2}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 160 MHz in 6 GHz band with P80 being the higher 80 MHz, DSO STA supports up
     * to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┐
     *  AP            │CH 103(S80)│CH 119(P80)│
     *                └───────────┴───────────┘
     *
     *                ┌───────────┬───────────┐
     *  STA           │   DSO     │  PRIMARY  │
     *                └───────────┴───────────┘
     */
    expectedChannel.Set({{103, MHz_t{6465}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{111, 0, BAND_6GHZ, 7}",
                                    "{119, 0, BAND_6GHZ, 3}",
                                    {{EhtRu::SECONDARY_80_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 320 MHz with P160 being the lower 160 MHz, DSO STA supports up to 160 MHz:
     *
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  AP            │       CH 15(P160)     │      CH 47(S160)      │
     *                └───────────────────────┴───────────────────────┘
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  STA           │       PRIMARY         │          DSO          │
     *                └───────────────────────┴───────────────────────┘
     */
    expectedChannel.Set({{47, MHz_t{6185}, MHz_t{160}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{31, 0, BAND_6GHZ, 6}",
                                    "{15, 0, BAND_6GHZ, 6}",
                                    {{EhtRu::SECONDARY_160_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 320 MHz with P160 being the higher 160 MHz, DSO STA supports up to 160 MHz:
     *
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  AP            │       CH 15(S160)     │      CH 47(P160)      │
     *                └───────────────────────┴───────────────────────┘
     *
     *                ┌───────────────────────┬───────────────────────┐
     *  STA           │          DSO          │       PRIMARY         │
     *                └───────────────────────┴───────────────────────┘
     */
    expectedChannel.Set({{15, MHz_t{6025}, MHz_t{160}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    AddTestCase(new DsoSubbandsTest("{31, 0, BAND_6GHZ, 9}",
                                    "{47, 0, BAND_6GHZ, 1}",
                                    {{EhtRu::SECONDARY_160_FLAGS, expectedChannel}}),
                TestCase::Duration::QUICK);

    /**
     * AP operating on 320 MHz with P160 being the lower 160 MHz and P80 being the lower 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(P80)│CH 183(S80)│CH 199(L80)│CH 215(H80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │  PRIMARY  │    DSO    │    DSO    │    DSO    │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{183, MHz_t{6865}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    std::map<EhtRu::PrimaryFlags, WifiPhyOperatingChannel> expectedDsoSubbands;
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{199, MHz_t{6945}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{215, MHz_t{7025}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 3}",
                                    "{167, 0, BAND_6GHZ, 3}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();

    /**
     * AP operating on 320 MHz with P160 being the lower 160 MHz and P80 being the higher 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(S80)│CH 183(P80)│CH 199(L80)│CH 215(H80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │    DSO    │  PRIMARY  │   DSO     │   DSO     │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{167, MHz_t{6785}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{199, MHz_t{6945}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{215, MHz_t{7025}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 5}",
                                    "{183, 0, BAND_6GHZ, 1}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();

    /**
     * AP operating on 320 MHz with P160 being the higher 160 MHz and P80 being the lower 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(L80)│CH 183(H80)│CH 199(P80)│CH 215(S80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │    DSO    │    DSO    │  PRIMARY  │    DSO    │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{215, MHz_t{7025}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{167, MHz_t{6785}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{183, MHz_t{6865}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 10}",
                                    "{199, 0, BAND_6GHZ, 2}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();

    /**
     * AP operating on 320 MHz with P160 being the higher 160 MHz and P80 being the higher 80 MHz
     * within the P160, DSO STA supports up to 80 MHz:
     *
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  AP            │CH 167(L80)│CH 183(H80)│CH 199(S80)│CH 215(P80)│
     *                └───────────┴───────────┴───────────┴───────────┘
     *
     *                ┌───────────┬───────────┬───────────┬───────────┐
     *  STA           │    DSO    │    DSO    │    DSO    │  PRIMARY  │
     *                └───────────┴───────────┴───────────┴───────────┘
     */
    expectedChannel.Set({{199, MHz_t{6945}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_80_FLAGS] = expectedChannel;
    expectedChannel.Set({{167, MHz_t{6785}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_LOW_FLAGS] = expectedChannel;
    expectedChannel.Set({{183, MHz_t{6865}, MHz_t{80}, WIFI_PHY_BAND_6GHZ}}, WIFI_STANDARD_80211bn);
    expectedDsoSubbands[EhtRu::SECONDARY_160_HIGH_FLAGS] = expectedChannel;
    AddTestCase(new DsoSubbandsTest("{191, 0, BAND_6GHZ, 12}",
                                    "{215, 0, BAND_6GHZ, 0}",
                                    expectedDsoSubbands),
                TestCase::Duration::QUICK);
    expectedDsoSubbands.clear();
}

static WifiDsoTestSuite g_wifiDsoTestSuite; ///< the test suite
