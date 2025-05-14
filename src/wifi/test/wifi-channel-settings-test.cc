/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-standards.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiChannelSettingsTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test initial channel settings for AP and non-AP STAs when those are not necessarily
 * identical
 *
 * This test verifies the channel width used by the non-AP STA is properly advertised to the AP STA,
 * and that invalid combinations get rejected. It generates DL and UL traffic (limited to 1 packet
 * for each direction) and checks that the number of received packets matches with the expectation.
 */
class WifiChannelSettingsTest : public TestCase
{
  public:
    /**
     * Parameters for the test
     */
    struct Params
    {
        WifiStandard apStandard;          ///< wifi standard for AP STA
        WifiStandard staStandard;         ///< wifi standard for non-AP STA
        std::string apChannelSettings;    ///< channel setting string for AP STA
        std::string staChannelSettings;   ///< channel setting string for non-AP STA
        MHz_u staLargestSupportedChWidth; ///< largest supported channel width by the non-AP STA
        bool skipAssocIncompatibleChannelWidth{
            false}; ///< flag to skip association when STA channel width is incompatible with AP

        /**
         * Print the parameters
         *
         * @return a string with the parameters
         */
        std::string Print() const
        {
            std::ostringstream oss;
            oss << "AP standard=" << apStandard << ", non-AP STA standard=" << staStandard
                << ", AP settings="
                << apChannelSettings + ", non-AP STA settings=" + staChannelSettings
                << ", staLargestSupportedChWidth=" << staLargestSupportedChWidth
                << " MHz, skipAssocIncompatibleChannelWidth=" << skipAssocIncompatibleChannelWidth;
            return oss.str();
        }
    };

    /**
     * Constructor
     *
     * @param params parameters for the test
     */
    WifiChannelSettingsTest(const Params& params);
    ~WifiChannelSettingsTest() override = default;

  private:
    void DoRun() override;

    /**
     * Callback invoked when a packet is received by the server application
     * @param p the packet
     * @param adr the address
     */
    void AppRx(Ptr<const Packet> p, const Address& adr);

    /**
     * Check results
     */
    void CheckResults();

    Params m_params; ///< test parameters

    Ptr<ApWifiMac> m_apWifiMac;   ///< AP wifi MAC
    Ptr<StaWifiMac> m_staWifiMac; ///< STA wifi MAC

    const uint32_t m_dlPacketSize{1400}; ///< DL packet size (bytes)
    const uint32_t m_ulPacketSize{600};  ///< UL packet size (bytes)

    uint32_t m_receivedDl{0}; ///< received DL packets
    uint32_t m_receivedUl{0}; ///< received UL packets
};

WifiChannelSettingsTest::WifiChannelSettingsTest(const Params& params)
    : TestCase("Check correct behaviour for scenario: " + params.Print()),
      m_params{params}
{
}

void
WifiChannelSettingsTest::AppRx(Ptr<const Packet> p, const Address& adr)
{
    NS_LOG_INFO("Received " << p->GetSize() << " bytes");
    if (p->GetSize() == m_dlPacketSize)
    {
        m_receivedDl++;
    }
    else if (p->GetSize() == m_ulPacketSize)
    {
        m_receivedUl++;
    }
}

void
WifiChannelSettingsTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(10);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNode(1);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<FriisPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    SpectrumWifiPhyHelper phy;
    phy.SetChannel(spectrumChannel);

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");

    wifi.SetStandard(m_params.apStandard);
    phy.Set("ChannelSettings", StringValue(m_params.apChannelSettings));

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(Ssid("ns-3-ssid")));
    auto apDevice = wifi.Install(phy, mac, wifiApNode);

    wifi.SetStandard(m_params.staStandard);
    phy.Set("MaxRadioBw", DoubleValue(m_params.staLargestSupportedChWidth));
    phy.Set("ChannelSettings", StringValue(m_params.staChannelSettings));

    NetDeviceContainer staDevice;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(Ssid("ns-3-ssid")));

    mac.SetAssocManager(
        "ns3::WifiDefaultAssocManager",
        "AllowAssocAllChannelWidths",
        BooleanValue(true), // avoid assert in test, it checks for received DL packets instead
        "SkipAssocIncompatibleChannelWidth",
        BooleanValue(m_params.skipAssocIncompatibleChannelWidth));

    std::istringstream split(m_params.staChannelSettings);
    std::vector<std::string> tmp;
    for (std::string val; std::getline(split, val, ',');)
    {
        tmp.emplace_back(val);
    }
    const auto staBw = std::stold(tmp.at(1));
    const auto expectInvalidConfig = m_params.staLargestSupportedChWidth < staBw;
    auto invalidConfig{false};
    try
    {
        staDevice = wifi.Install(phy, mac, wifiStaNode);
    }
    catch (const std::runtime_error&)
    {
        invalidConfig = true;
    }

    NS_TEST_ASSERT_MSG_EQ(invalidConfig,
                          expectInvalidConfig,
                          "Configuration should " << (expectInvalidConfig ? "not " : "")
                                                  << "have been allowed");
    if (invalidConfig)
    {
        Simulator::Destroy();
        return;
    }

    m_apWifiMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac());
    m_staWifiMac = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDevice.Get(0))->GetMac());

    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevice, streamNumber);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiStaNode);
    packetSocket.Install(wifiApNode);

    // generate a single packet in DL direction
    PacketSocketAddress dlSocket;
    dlSocket.SetSingleDevice(apDevice.Get(0)->GetIfIndex());
    dlSocket.SetPhysicalAddress(staDevice.Get(0)->GetAddress());
    dlSocket.SetProtocol(0);

    auto dlClient = CreateObject<PacketSocketClient>();
    dlClient->SetAttribute("PacketSize", UintegerValue(m_dlPacketSize));
    dlClient->SetAttribute("MaxPackets", UintegerValue(1));
    dlClient->SetRemote(dlSocket);
    wifiApNode.Get(0)->AddApplication(dlClient);
    dlClient->SetStartTime(Seconds(0.15));
    dlClient->SetStopTime(Seconds(0.25));

    auto dlServer = CreateObject<PacketSocketServer>();
    dlServer->SetLocal(dlSocket);
    wifiStaNode.Get(0)->AddApplication(dlServer);
    dlServer->SetStartTime(Seconds(0.0));
    dlServer->SetStopTime(Seconds(0.25));

    // generate a single packet in UL direction
    PacketSocketAddress ulSocket;
    ulSocket.SetSingleDevice(staDevice.Get(0)->GetIfIndex());
    ulSocket.SetPhysicalAddress(apDevice.Get(0)->GetAddress());
    ulSocket.SetProtocol(1);

    auto ulClient = CreateObject<PacketSocketClient>();
    ulClient->SetAttribute("PacketSize", UintegerValue(m_ulPacketSize));
    ulClient->SetAttribute("MaxPackets", UintegerValue(1));
    ulClient->SetRemote(ulSocket);
    wifiStaNode.Get(0)->AddApplication(ulClient);
    ulClient->SetStartTime(Seconds(0.2));
    ulClient->SetStopTime(Seconds(0.25));

    auto ulServer = CreateObject<PacketSocketServer>();
    ulServer->SetLocal(ulSocket);
    wifiApNode.Get(0)->AddApplication(ulServer);
    ulServer->SetStartTime(Seconds(0.0));
    ulServer->SetStopTime(Seconds(0.25));

    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSocketServer/Rx",
                                  MakeCallback(&WifiChannelSettingsTest::AppRx, this));

    Simulator::Stop(Seconds(0.25));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
WifiChannelSettingsTest::CheckResults()
{
    const auto staPhy = m_staWifiMac->GetDevice()->GetPhy();
    const auto staBw = staPhy->GetChannelWidth();
    const auto compatibleBw =
        GetSupportedChannelWidthSet(staPhy->GetStandard(), staPhy->GetPhyBand()).contains(staBw) ||
        (staBw >= m_apWifiMac->GetDevice()->GetPhy()->GetChannelWidth());

    NS_TEST_EXPECT_MSG_EQ(m_staWifiMac->GetWifiRemoteStationManager()->GetChannelWidthSupported(
                              m_apWifiMac->GetAddress()),
                          m_apWifiMac->GetDevice()->GetPhy()->GetChannelWidth(),
                          "Incorrect AP channel width information received by STA");

    // if BW is not compatible, STA should not be able to receive any DL packets (since they will
    // use a larger BW than the one supported by its PHY)
    const uint32_t expectedRxDl = compatibleBw ? 1 : 0;
    NS_TEST_EXPECT_MSG_EQ(m_receivedDl,
                          expectedRxDl,
                          "Unexpected number of received packets in downlink direction");

    // if BW is not compatible and skipAssocIncompatibleChannelWidth is true, STA should not be
    // associated to AP and hence no traffic should be received
    const uint32_t expectedRxUl =
        (compatibleBw || !m_params.skipAssocIncompatibleChannelWidth) ? 1 : 0;
    NS_TEST_EXPECT_MSG_EQ(m_receivedUl,
                          expectedRxUl,
                          "Unexpected number of received packets in uplink direction");
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi channel settings test suite
 */
class WifiChannelSettingsTestSuite : public TestSuite
{
  public:
    WifiChannelSettingsTestSuite();
};

WifiChannelSettingsTestSuite::WifiChannelSettingsTestSuite()
    : TestSuite("wifi-channel-settings", Type::UNIT)
{
    const std::map<std::pair<MHz_u, WifiPhyBand>, std::string> channelSettingsMap{
        {{MHz_u{20}, WIFI_PHY_BAND_2_4GHZ}, "{1, 20, BAND_2_4GHZ, 0}"},
        {{MHz_u{40}, WIFI_PHY_BAND_2_4GHZ}, "{3, 40, BAND_2_4GHZ, 0}"},
        {{MHz_u{20}, WIFI_PHY_BAND_5GHZ}, "{36, 20, BAND_5GHZ, 0}"},
        {{MHz_u{40}, WIFI_PHY_BAND_5GHZ}, "{38, 40, BAND_5GHZ, 0}"},
        {{MHz_u{80}, WIFI_PHY_BAND_5GHZ}, "{42, 80, BAND_5GHZ, 0}"},
        {{MHz_u{160}, WIFI_PHY_BAND_5GHZ}, "{50, 160, BAND_5GHZ, 0}"},
        {{MHz_u{20}, WIFI_PHY_BAND_6GHZ}, "{1, 20, BAND_6GHZ, 0}"},
        {{MHz_u{40}, WIFI_PHY_BAND_6GHZ}, "{3, 40, BAND_6GHZ, 0}"},
        {{MHz_u{80}, WIFI_PHY_BAND_6GHZ}, "{7, 80, BAND_6GHZ, 0}"},
        {{MHz_u{160}, WIFI_PHY_BAND_6GHZ}, "{15, 160, BAND_6GHZ, 0}"},
        {{MHz_u{320}, WIFI_PHY_BAND_6GHZ}, "{31, 320, BAND_6GHZ, 0}"},
    };

    for (const auto standard : {WIFI_STANDARD_80211n,
                                WIFI_STANDARD_80211ac,
                                WIFI_STANDARD_80211ax,
                                WIFI_STANDARD_80211be})
    {
        for (const auto maxSupportedBw : {MHz_u{20}, MHz_u{40}, MHz_u{80}, MHz_u{160}, MHz_u{320}})
        {
            for (const auto& [apWidthBandPair, apChannel] : channelSettingsMap)
            {
                for (const auto& [staWidthBandPair, staChannel] : channelSettingsMap)
                {
                    if (apWidthBandPair.second != staWidthBandPair.second)
                    {
                        continue; // different band
                    }
                    if (const auto& allowedBands = wifiStandards.at(standard);
                        std::find(allowedBands.cbegin(),
                                  allowedBands.cend(),
                                  apWidthBandPair.second) == allowedBands.cend())
                    {
                        continue; // standard does not operate on this band
                    }
                    if (const auto maxWidth =
                            std::max(apWidthBandPair.first, staWidthBandPair.first);
                        maxWidth > GetMaximumChannelWidth(GetModulationClassForStandard(standard)))
                    {
                        continue; // channel width(s) not supported by the standard
                    }
                    for (const auto skipAssocIfIncompatible : {false, true})
                    {
                        AddTestCase(
                            new WifiChannelSettingsTest(
                                {.apStandard = standard,
                                 .staStandard = standard,
                                 .apChannelSettings = apChannel,
                                 .staChannelSettings = staChannel,
                                 .staLargestSupportedChWidth = maxSupportedBw,
                                 .skipAssocIncompatibleChannelWidth = skipAssocIfIncompatible}),
                            TestCase::Duration::QUICK);
                    }
                }
            }
        }
    }
}

static WifiChannelSettingsTestSuite g_wifiChannelSettingsTestSuite; ///< the test suite
