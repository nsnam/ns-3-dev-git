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
#include "ns3/wifi-static-setup-helper.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-wifi-helper.h"

#include <map>
#include <memory>
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
        MHz_t staLargestSupportedChWidth; ///< largest supported channel width by the non-AP STA
        bool skipAssocIncompatibleChannelWidth{
            false}; ///< flag to skip association when STA channel width is incompatible with AP
        bool generateTraffic{true}; ///< flag to generate traffic

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
                << ", staLargestSupportedChWidth=" + staLargestSupportedChWidth.str()
                << ", skipAssocIncompatibleChannelWidth=" << skipAssocIncompatibleChannelWidth;
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
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /**
     * Check results
     */
    void CheckResults();

    Params m_params; ///< test parameters

    Ptr<ApWifiMac> m_apWifiMac;   ///< AP wifi MAC
    Ptr<StaWifiMac> m_staWifiMac; ///< STA wifi MAC

    const uint32_t m_dlPacketSize{1400}; ///< DL packet size (bytes)
    const uint32_t m_ulPacketSize{600};  ///< UL packet size (bytes)

    std::optional<MgtBeaconHeader> m_beacon; ///< beacon header received by the STA
    uint32_t m_receivedDl{0};                ///< received DL packets
    uint32_t m_receivedUl{0};                ///< received UL packets
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
WifiChannelSettingsTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    const auto& mpdu = *psduMap.cbegin()->second->begin();
    NS_LOG_FUNCTION(this << *mpdu << txVector << txPowerW);
    if (mpdu->GetHeader().IsBeacon() && !m_beacon)
    {
        MgtBeaconHeader beacon;
        mpdu->GetPacket()->PeekHeader(beacon);
        m_beacon = beacon;
        // stop generation of beacon frames in order to speedup test
        m_apWifiMac->SetAttribute("BeaconGeneration", BooleanValue(false));
    }
}

void
WifiChannelSettingsTest::DoRun()
{
    Config::SetDefault("ns3::WifiAssocManager::EnableChannelSwitch", BooleanValue(false));

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(10);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNode(1);

    std::unique_ptr<WifiPhyHelper> phy;
    if (((m_params.apStandard < WIFI_STANDARD_80211n) &&
         (m_params.staStandard < WIFI_STANDARD_80211n)) ||
        (m_params.apChannelSettings == m_params.staChannelSettings))
    {
        auto channel = YansWifiChannelHelper::Default();
        auto yansPhy = std::make_unique<YansWifiPhyHelper>();
        yansPhy->SetChannel(channel.Create());
        phy = std::move(yansPhy);
    }
    else
    {
        auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
        auto lossModel = CreateObject<FriisPropagationLossModel>();
        spectrumChannel->AddPropagationLossModel(lossModel);
        auto delayModel = CreateObject<ConstantSpeedPropagationDelayModel>();
        spectrumChannel->SetPropagationDelayModel(delayModel);
        auto spectrumPhy = std::make_unique<SpectrumWifiPhyHelper>();
        spectrumPhy->SetChannel(spectrumChannel);
        phy = std::move(spectrumPhy);
    }

    WifiHelper wifi;
    std::string rateStr;
    if ((m_params.apStandard >= WIFI_STANDARD_80211bn) &&
        (m_params.staStandard >= WIFI_STANDARD_80211bn))
    {
        rateStr = "UhrMcs0";
    }
    else if ((m_params.apStandard >= WIFI_STANDARD_80211be) &&
             (m_params.staStandard >= WIFI_STANDARD_80211be))
    {
        rateStr = "EhtMcs0";
    }
    else if ((m_params.apStandard >= WIFI_STANDARD_80211ax) &&
             (m_params.staStandard >= WIFI_STANDARD_80211ax))
    {
        rateStr = "HeMcs0";
    }
    else if ((m_params.apStandard >= WIFI_STANDARD_80211ac) &&
             (m_params.staStandard >= WIFI_STANDARD_80211ac))
    {
        rateStr = "VhtMcs0";
    }
    else if ((m_params.apStandard >= WIFI_STANDARD_80211n) &&
             (m_params.staStandard >= WIFI_STANDARD_80211n))
    {
        rateStr = "HtMcs0";
    }
    else if ((m_params.apStandard >= WIFI_STANDARD_80211g) &&
             (m_params.staStandard >= WIFI_STANDARD_80211g))
    {
        rateStr = "ErpOfdmRate24Mbps";
    }
    else if ((m_params.apStandard == WIFI_STANDARD_80211b) ||
             (m_params.staStandard == WIFI_STANDARD_80211b))
    {
        rateStr = "DsssRate1Mbps";
    }
    else
    {
        rateStr = "OfdmRate24Mbps";
    }
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager", "DataMode", StringValue(rateStr));

    wifi.SetStandard(m_params.apStandard);
    phy->Set("ChannelSettings", StringValue(m_params.apChannelSettings));

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(Ssid("ns-3-ssid")));
    auto apDevice = wifi.Install(*phy, mac, wifiApNode);

    wifi.SetStandard(m_params.staStandard);
    phy->Set("MaxRadioBw", MHzValue(m_params.staLargestSupportedChWidth));
    phy->Set("ChannelSettings", StringValue(m_params.staChannelSettings));

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
    const auto staBw = MHz_t::from_str(tmp.at(1) + std::string("MHz"));
    NS_TEST_ASSERT_MSG_EQ(staBw.has_value(), true, "STA channel settings are incorrect");
    const auto expectInvalidConfig = m_params.staLargestSupportedChWidth < staBw.value();
    auto invalidConfig{false};
    try
    {
        staDevice = wifi.Install(*phy, mac, wifiStaNode);
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

    if (!m_params.skipAssocIncompatibleChannelWidth &&
        (m_params.apStandard != WIFI_STANDARD_80211b) &&
        (m_params.staStandard != WIFI_STANDARD_80211b))
    {
        auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
        NS_ASSERT(apDev);
        WifiStaticSetupHelper::SetStaticAssoc(apDev, staDevice);
        auto staDev = DynamicCast<WifiNetDevice>(staDevice.Get(0));
        NS_ASSERT(staDev);
        if ((m_params.apStandard >= WIFI_STANDARD_80211n) &&
            (m_params.staStandard >= WIFI_STANDARD_80211n))
        {
            WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDev, 0);
            WifiStaticSetupHelper::SetStaticBlockAck(staDev, apDev, 0);
        }
    }

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    if (m_params.generateTraffic)
    {
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
    }
    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phys/0/PhyTxPsduBegin",
        MakeCallback(&WifiChannelSettingsTest::Transmit, this));

    Simulator::Stop(Seconds(0.25));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
WifiChannelSettingsTest::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_beacon.has_value(), true, "No beacon received by the STA");

    const auto apAddress = m_apWifiMac->GetAddress();
    const auto staRsm = m_staWifiMac->GetWifiRemoteStationManager();
    const auto apStandardFromSta = staRsm->GetStandard(apAddress);
    const auto expectedApStandard =
        (m_params.apStandard <= m_params.staStandard) ? m_params.apStandard : m_params.staStandard;
    NS_TEST_EXPECT_MSG_EQ(apStandardFromSta,
                          expectedApStandard,
                          "AP standard seen by STA (" << apStandardFromSta
                                                      << ") does not match the one used by AP ("
                                                      << expectedApStandard << ")");

    const auto apPhy = m_apWifiMac->GetDevice()->GetPhy();
    const auto apBw = apPhy->GetChannelWidth();
    const auto apBwFromSta = staRsm->GetChannelWidthSupported(apAddress);
    NS_TEST_EXPECT_MSG_EQ(apBwFromSta,
                          apBw,
                          "AP channel width seen by STA ("
                              << apBwFromSta << ") does not match the one used by AP (" << apBw
                              << ")");

    const auto staPhy = m_staWifiMac->GetDevice()->GetPhy();
    const auto staBw = staPhy->GetChannelWidth();
    const auto apChannelFromSta = GetApOperatingChannel(
        apBwFromSta,
        (staBw.IsMultipleOf(MHz_t{20}) ? std::optional(staPhy->GetPrimaryChannelNumber(MHz_t{20}))
                                       : std::nullopt),
        staPhy->GetPhyBand(),
        apStandardFromSta,
        *m_beacon);
    const auto apChannel = apPhy->GetOperatingChannel();
    NS_TEST_EXPECT_MSG_EQ(apChannelFromSta,
                          apChannel,
                          "AP channel seen by STA (" << apChannelFromSta
                                                     << ") does not match the one used by AP ("
                                                     << apChannel << ")");

    if (!m_params.generateTraffic)
    {
        return;
    }

    // if BW is not compatible, STA should not be able to receive any DL packets (since they will
    // use a larger BW than the one supported by its PHY)
    const auto compatibleBw =
        GetSupportedChannelWidthSet(staPhy->GetStandard(), staPhy->GetPhyBand()).contains(staBw) ||
        (staBw >= apBw);
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
    const std::map<std::pair<MHz_t, WifiPhyBand>, std::string> channelSettingsMap{
        {{MHz_t{20}, WIFI_PHY_BAND_2_4GHZ}, "{1, 20, BAND_2_4GHZ, 0}"},
        {{MHz_t{22}, WIFI_PHY_BAND_2_4GHZ}, "{1, 22, BAND_2_4GHZ, 0}"},
        {{MHz_t{40}, WIFI_PHY_BAND_2_4GHZ}, "{3, 40, BAND_2_4GHZ, 0}"},
        {{MHz_t{20}, WIFI_PHY_BAND_5GHZ}, "{36, 20, BAND_5GHZ, 0}"},
        {{MHz_t{40}, WIFI_PHY_BAND_5GHZ}, "{38, 40, BAND_5GHZ, 0}"},
        {{MHz_t{80}, WIFI_PHY_BAND_5GHZ}, "{42, 80, BAND_5GHZ, 0}"},
        {{MHz_t{160}, WIFI_PHY_BAND_5GHZ}, "{50, 160, BAND_5GHZ, 0}"},
        {{MHz_t{20}, WIFI_PHY_BAND_6GHZ}, "{1, 20, BAND_6GHZ, 0}"},
        {{MHz_t{40}, WIFI_PHY_BAND_6GHZ}, "{3, 40, BAND_6GHZ, 0}"},
        {{MHz_t{80}, WIFI_PHY_BAND_6GHZ}, "{7, 80, BAND_6GHZ, 0}"},
        {{MHz_t{160}, WIFI_PHY_BAND_6GHZ}, "{15, 160, BAND_6GHZ, 0}"},
        {{MHz_t{320}, WIFI_PHY_BAND_6GHZ}, "{31, 320, BAND_6GHZ, 0}"},
    };

    for (const auto apStandard : {WIFI_STANDARD_80211b,
                                  WIFI_STANDARD_80211a,
                                  WIFI_STANDARD_80211g,
                                  WIFI_STANDARD_80211n,
                                  WIFI_STANDARD_80211ac,
                                  WIFI_STANDARD_80211ax,
                                  WIFI_STANDARD_80211be,
                                  WIFI_STANDARD_80211bn})
    {
        for (const auto staStandard : {WIFI_STANDARD_80211b,
                                       WIFI_STANDARD_80211a,
                                       WIFI_STANDARD_80211g,
                                       WIFI_STANDARD_80211n,
                                       WIFI_STANDARD_80211ac,
                                       WIFI_STANDARD_80211ax,
                                       WIFI_STANDARD_80211be,
                                       WIFI_STANDARD_80211bn})
        {
            for (const auto maxSupportedBw :
                 {MHz_t{20}, MHz_t{22}, MHz_t{40}, MHz_t{80}, MHz_t{160}, MHz_t{320}})
            {
                for (const auto& [apWidthBandPair, apChannel] : channelSettingsMap)
                {
                    const auto& [apWidth, apBand] = apWidthBandPair;
                    if ((apWidth == MHz_t{22}) && (apStandard != WIFI_STANDARD_80211b))
                    {
                        continue; // 22 MHz is only supported by 802.11b
                    }
                    for (const auto& [staWidthBandPair, staChannel] : channelSettingsMap)
                    {
                        const auto& [staWidth, staBand] = staWidthBandPair;
                        if ((staWidth == MHz_t{22}) && (staStandard != WIFI_STANDARD_80211b))
                        {
                            continue; // 22 MHz is only supported by 802.11b
                        }
                        const auto minWidth = std::min(apWidth, staWidth);
                        const auto maxWidth = std::max(apWidth, staWidth);
                        if (apBand != staBand)
                        {
                            continue; // different band
                        }
                        if (const auto& allowedBands = wifiStandards.at(apStandard);
                            std::find(allowedBands.cbegin(), allowedBands.cend(), apBand) ==
                            allowedBands.cend())
                        {
                            continue; // AP standard does not operate on this band
                        }
                        if (const auto& allowedBands = wifiStandards.at(staStandard);
                            std::find(allowedBands.cbegin(), allowedBands.cend(), staBand) ==
                            allowedBands.cend())
                        {
                            continue; // STA standard does not operate on this band
                        }
                        if ((minWidth <
                             GetMinimumChannelWidth(GetModulationClassForStandard(apStandard))) ||
                            (maxWidth >
                             GetMaximumChannelWidth(GetModulationClassForStandard(staStandard))) ||
                            (minWidth <
                             GetMinimumChannelWidth(GetModulationClassForStandard(staStandard))) ||
                            (maxWidth >
                             GetMaximumChannelWidth(GetModulationClassForStandard(apStandard))))
                        {
                            continue; // channel width(s) not supported by the standard
                        }
                        if (maxSupportedBw < minWidth)
                        {
                            continue; // invalid configuration
                        }
                        for (const auto skipAssocIfIncompatible : {false, true})
                        {
                            AddTestCase(new WifiChannelSettingsTest({.apStandard = apStandard,
                                                                     .staStandard = staStandard,
                                                                     .apChannelSettings = apChannel,
                                                                     .staChannelSettings =
                                                                         staChannel,
                                                                     .staLargestSupportedChWidth =
                                                                         maxSupportedBw,
                                                                     .skipAssocIncompatibleChannelWidth =
                                                                         skipAssocIfIncompatible,
                                                                     .generateTraffic =
                                                                         (apStandard == staStandard) /* generate traffic only if same standard to speed up tests */}),
                                        TestCase::Duration::QUICK);
                        }
                    }
                }
            }
        }
    }
}

static WifiChannelSettingsTestSuite g_wifiChannelSettingsTestSuite; ///< the test suite
