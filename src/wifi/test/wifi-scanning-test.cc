/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/interference-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/object-factory.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-net-device.h"

#include <array>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiScanningTest");

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
 * @brief Test offline channel scanning procedure.
 *
 * A non-AP MLD with two links performs ML setup with an AP MLD having two links. After ML setup, an
 * offline channel scanning procedure is requested to the non-AP MLD. It is checked that:
 * - the non-AP MLD sends a Data Null frame on each of the links to switch to powersave mode
 * - the powersave mode is actually turned on when those frames are ack'ed
 * - the non-AP MLD scans all the requested channels and sends a Probe Request on each of them
 * - when done, the previous channels are restored, power management mode is active again and the
 *   non-AP MLD is still associated
 */
class WifiOfflineChannelScanningTest : public TestCase
{
  public:
    /// typedef for the list of channels to scan
    using ChannelList = std::list<std::pair<WifiPhyBand, uint8_t>>;

    /**
     * Constructor.
     *
     * @param channelsToScan0 list of channels to be scanned by PHY 0
     * @param channelsToScan1 list of channels to be scanned by PHY 1
     */
    WifiOfflineChannelScanningTest(const ChannelList& channelsToScan0,
                                   const ChannelList& channelsToScan1);

  protected:
    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param mac the MAC transmitting the PSDUs
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    virtual void Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW);

    /// Setup parameters for the offline channel scanning
    void SetupOfflineScanning(Mac48Address /* bssid */);

    /**
     * Callback connected to the ScanningEnd trace. The purpose is to verify that the trace is
     * fired and the given list is empty (no AP is setup on the scanned channels).
     *
     * @param apList the list of APs discovered while scanning
     */
    void ScanEndCallback(const std::list<StaWifiMac::ApInfo>& apList);

  private:
    void DoSetup() override;
    void DoRun() override;

    /// Insert elements in the list of expected events (transmitted frames)
    void InsertEvents();

    /// Actions and checks to perform upon the transmission of each frame
    struct Events
    {
        /**
         * Constructor.
         *
         * @param type the frame MAC header type
         * @param f function to perform actions and checks
         */
        Events(WifiMacType type,
               std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>&& f = {})
            : hdrType(type),
              func(f)
        {
        }

        WifiMacType hdrType; ///< MAC header type of frame being transmitted
        std::function<void(Ptr<const WifiPsdu>, const WifiTxVector&, uint8_t)>
            func; ///< function to perform actions and checks
    };

    std::list<Events> m_events; //!< list of events for a test run

    Ptr<ApWifiMac> m_apMac;                      ///< AP wifi MAC
    Ptr<StaWifiMac> m_staMac;                    ///< STA wifi MAC
    Time m_offScanInterval{MilliSeconds(500)};   ///< offline channel scanning interval
    Time m_duration{MilliSeconds(900)};          ///< simulation duration (just one scan)
    std::array<ChannelList, 2> m_channelsToScan; ///< channels to be scanned by PHY 0 and 1
    std::size_t m_traceFiredCount{0};            ///< number of times ScanningEnd trace is fired
    std::map<uint8_t, WifiPhyOperatingChannel>
        m_channelsBeforeScan; ///< PHY ID-indexed map of the frequency channels on which STAs are
                              ///< operating before starting offline scanning
};

WifiOfflineChannelScanningTest::WifiOfflineChannelScanningTest(const ChannelList& channelsToScan0,
                                                               const ChannelList& channelsToScan1)
    : TestCase("Test the offline channel scanning provedure")
{
    m_channelsToScan[0] = channelsToScan0;
    m_channelsToScan[1] = channelsToScan1;
}

void
WifiOfflineChannelScanningTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 30;

    NodeContainer nodes(2); // AP, STA
    NetDeviceContainer devices;

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    spectrumChannel->AddPropagationLossModel(CreateObject<FriisPropagationLossModel>());

    SpectrumWifiPhyHelper phy(2);
    phy.Set(0, "ChannelSettings", StringValue("{0,80,BAND_5GHZ,0}"));
    phy.Set(1, "ChannelSettings", StringValue("{0,80,BAND_6GHZ,0}"));
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(spectrumChannel);

    Ssid ssid("wifi-scan-test");
    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    devices.Add(wifi.Install(phy, mac, nodes.Get(0)));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

    devices.Add(wifi.Install(phy, mac, nodes.Get(1)));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(devices, streamNumber);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));   // AP 1
    positionAlloc->Add(Vector(20.0, 0.0, 0.0));  // AP 2
    positionAlloc->Add(Vector(10.0, 10.0, 0.0)); // non-AP STA
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(0))->GetMac());
    m_staMac = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(1))->GetMac());

    // Trace PSDUs passed to the PHY on all devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        auto dev = DynamicCast<WifiNetDevice>(devices.Get(i));
        for (uint8_t phyId = 0; phyId < dev->GetNPhys(); ++phyId)
        {
            dev->GetPhy(phyId)->TraceConnectWithoutContext(
                "PhyTxPsduBegin",
                MakeCallback(&WifiOfflineChannelScanningTest::Transmit, this)
                    .Bind(dev->GetMac(), phyId));
        }
    }

    // setup offline channel scan interval
    m_staMac->GetAssocManager()->SetAttribute("OffChannelScanInterval",
                                              TimeValue(m_offScanInterval));
    // once associated, setup parameters for offline channel scanning
    m_staMac->TraceConnectWithoutContext(
        "Assoc",
        MakeCallback(&WifiOfflineChannelScanningTest::SetupOfflineScanning, this));

    InsertEvents();
}

void
WifiOfflineChannelScanningTest::ScanEndCallback(const std::list<StaWifiMac::ApInfo>& apList)
{
    ++m_traceFiredCount;
    NS_TEST_EXPECT_MSG_EQ(apList.empty(), true, "Expected no AP discovered during scanning");
    for (const auto& ap : apList)
    {
        std::cout << ap << std::endl;
    }
}

void
WifiOfflineChannelScanningTest::Transmit(Ptr<WifiMac> mac,
                                         uint8_t phyId,
                                         WifiConstPsduMap psduMap,
                                         WifiTxVector txVector,
                                         double txPowerW)
{
    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << " Link ID " << +linkId.value() << " Phy ID " << +phyId
           << " #MPDUs " << psdu->GetNMpdus();
        for (auto it = psdu->begin(); it != psdu->end(); ++it)
        {
            ss << "\n" << **it;
        }
        NS_LOG_INFO(ss.str());
    }
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");

    const auto psdu = psduMap.cbegin()->second;
    const auto& hdr = psdu->GetHeader(0);

    // skip Beacon frames and all frames sent before the start of the offline channel scanning
    if (hdr.IsBeacon() || Simulator::Now() < m_offScanInterval)
    {
        return;
    }

    if (!m_events.empty())
    {
        // check that the expected frame is being transmitted
        NS_TEST_EXPECT_MSG_EQ(WifiMacHeader(m_events.front().hdrType).GetTypeString(),
                              hdr.GetTypeString(),
                              "Unexpected MAC header type for frame transmitted at time "
                                  << Simulator::Now().As(Time::US));
        // perform actions/checks, if any
        if (m_events.front().func)
        {
            m_events.front().func(psdu, txVector, linkId.value());
        }

        m_events.pop_front();
    }
}

void
WifiOfflineChannelScanningTest::SetupOfflineScanning(Mac48Address /* bssid */)
{
    // save current channels
    for (uint8_t phyId = 0; phyId < m_staMac->GetNLinks(); ++phyId)
    {
        m_channelsBeforeScan[phyId] = m_staMac->GetDevice()->GetPhy(phyId)->GetOperatingChannel();
    }

    // configure active scanning with short probe delay and probe request timeout values
    m_staMac->SetAttribute("ActiveProbing", BooleanValue(true));
    m_staMac->SetAttribute("ProbeDelay", StringValue("ns3::ConstantRandomVariable[Constant=50.0]"));
    m_staMac->SetAttribute("ProbeRequestTimeout", TimeValue(MilliSeconds(5)));

    // configure the given channels to scan
    WifiScanParams::Map channelsToScan;

    for (const uint8_t phyId : {0, 1})
    {
        for (const auto& [band, channelNo] : m_channelsToScan[phyId])
        {
            channelsToScan[phyId][band].emplace_back(channelNo);
        }
    }

    const auto channelStr = StaWifiMac::ScanningChannelsToStr(channelsToScan);
    m_staMac->SetAttribute("ScanningChannels", StringValue(channelStr));

    m_staMac->SetAttribute("OfflineScanProtection",
                           EnumValue(WifiOfflineScanProtection::POWERSAVE));

    // connect callback to ScanningEnd trace
    m_staMac->GetAssocManager()->TraceConnectWithoutContext(
        "ScanningEnd",
        MakeCallback(&WifiOfflineChannelScanningTest::ScanEndCallback, this));
}

void
WifiOfflineChannelScanningTest::InsertEvents()
{
    // when offline channel scanning is started, non-AP MLD sends a Data Null frame on each link to
    // switch to power save mode
    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  true,
                                  "Expected PM bit set in Data Null frame sent on link "
                                      << +linkId);
        });

    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  true,
                                  "Expected PM bit set in Data Null frame sent on link "
                                      << +linkId);
        });

    // when Ack is received, PowerSave mode is turned on
    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            Simulator::Schedule(txDuration + TimeStep(1), [=, this] {
                NS_TEST_EXPECT_MSG_EQ(m_staMac->GetPmMode(linkId),
                                      WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                      "Expected PowerSave mode to be configured on link "
                                          << +linkId);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            Simulator::Schedule(txDuration + TimeStep(1), [=, this] {
                NS_TEST_EXPECT_MSG_EQ(m_staMac->GetPmMode(linkId),
                                      WifiPowerManagementMode::WIFI_PM_POWERSAVE,
                                      "Expected PowerSave mode to be configured on link "
                                          << +linkId);
            });
        });

    // a Probe Request frame is sent for every channel to scan
    const std::size_t nProbeReqs = m_channelsToScan[0].size() + m_channelsToScan[1].size();

    for (std::size_t i = 0; i < nProbeReqs; ++i)
    {
        m_events.emplace_back(
            WIFI_MAC_MGT_PROBE_REQUEST,
            [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
                NS_TEST_EXPECT_MSG_EQ(psdu->GetAddr2(),
                                      m_staMac->GetFrameExchangeManager(linkId)->GetAddress(),
                                      "Unexpected Transmitted Address for Probe Request #" << i);

                const auto phy = m_staMac->GetWifiPhy(linkId);
                NS_TEST_ASSERT_MSG_NE(phy, nullptr, "No PHY operating on link " << +linkId);
                const auto phyId = phy->GetPhyId();
                const auto channel = phy->GetOperatingChannel();

                auto channelIt =
                    std::find_if(m_channelsToScan[phyId].cbegin(),
                                 m_channelsToScan[phyId].cend(),
                                 [channel](auto&& bandChannelNoPair) {
                                     return (bandChannelNoPair.first == channel.GetPhyBand()) &&
                                            (bandChannelNoPair.second == channel.GetNumber());
                                 });
                NS_TEST_EXPECT_MSG_EQ((channelIt != m_channelsToScan[phyId].cend()),
                                      true,
                                      channel << " not present in the list of channels for PHY "
                                              << +phy->GetPhyId());
            });
    }

    // when offline channel scanning is completed, non-AP MLD restores previous channels and sends a
    // Data Null frame on each link to switch back to active mode
    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto phy = m_staMac->GetWifiPhy(linkId);
            NS_TEST_ASSERT_MSG_NE(phy, nullptr, "No PHY operating on link " << +linkId);
            const auto phyId = phy->GetPhyId();

            NS_TEST_EXPECT_MSG_EQ((phy->GetOperatingChannel() == m_channelsBeforeScan[phyId]),
                                  true,
                                  "Failed to restore channel for PHY " << +phyId);

            NS_TEST_EXPECT_MSG_EQ(m_staMac->IsAssociated(),
                                  true,
                                  "Expected non-AP MLD to be associated");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  false,
                                  "Expected PM bit unset in Data Null frame sent on link "
                                      << +linkId);
        });

    m_events.emplace_back(
        WIFI_MAC_DATA_NULL,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto phy = m_staMac->GetWifiPhy(linkId);
            NS_TEST_ASSERT_MSG_NE(phy, nullptr, "No PHY operating on link " << +linkId);
            const auto phyId = phy->GetPhyId();

            NS_TEST_EXPECT_MSG_EQ((phy->GetOperatingChannel() == m_channelsBeforeScan[phyId]),
                                  true,
                                  "Failed to restore channel for PHY " << +phyId);

            NS_TEST_EXPECT_MSG_EQ(m_staMac->IsAssociated(),
                                  true,
                                  "Expected non-AP MLD to be associated");

            NS_TEST_EXPECT_MSG_EQ(psdu->GetHeader(0).IsPowerManagement(),
                                  false,
                                  "Expected PM bit unset in Data Null frame sent on link "
                                      << +linkId);
        });

    // when Ack is received, PowerSave mode is turned on
    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            Simulator::Schedule(txDuration + TimeStep(1), [=, this] {
                NS_TEST_EXPECT_MSG_EQ(m_staMac->GetPmMode(linkId),
                                      WifiPowerManagementMode::WIFI_PM_ACTIVE,
                                      "Expected active mode to be configured on link " << +linkId);
            });
        });

    m_events.emplace_back(
        WIFI_MAC_CTL_ACK,
        [=, this](Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId) {
            const auto txDuration =
                WifiPhy::CalculateTxDuration(psdu,
                                             txVector,
                                             m_apMac->GetWifiPhy(linkId)->GetPhyBand());
            Simulator::Schedule(txDuration + TimeStep(1), [=, this] {
                NS_TEST_EXPECT_MSG_EQ(m_staMac->GetPmMode(linkId),
                                      WifiPowerManagementMode::WIFI_PM_ACTIVE,
                                      "Expected active mode to be configured on link " << +linkId);
            });
        });
}

void
WifiOfflineChannelScanningTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_events.empty(), true, "Not all events took place");
    NS_TEST_EXPECT_MSG_EQ(m_traceFiredCount,
                          1,
                          "ScanningEnd trace fired incorrect number of times");

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

    const auto channelsToScan0 =
        WifiOfflineChannelScanningTest::ChannelList{{WIFI_PHY_BAND_2_4GHZ, 1},
                                                    {WIFI_PHY_BAND_2_4GHZ, 5},
                                                    {WIFI_PHY_BAND_2_4GHZ, 9},
                                                    {WIFI_PHY_BAND_2_4GHZ, 13}};
    const auto channelsToScan1 =
        WifiOfflineChannelScanningTest::ChannelList{{WIFI_PHY_BAND_5GHZ, 100},
                                                    {WIFI_PHY_BAND_5GHZ, 108},
                                                    {WIFI_PHY_BAND_6GHZ, 101},
                                                    {WIFI_PHY_BAND_6GHZ, 109}};

    AddTestCase(new WifiOfflineChannelScanningTest(channelsToScan0, channelsToScan1),
                TestCase::Duration::QUICK);
}

static WifiScanningTestSuite g_wifiScanningTestSuite; ///< the test suite
