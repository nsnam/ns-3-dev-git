/*
 * Copyright (c) 2024 Rami Abdallah
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/enum.h"
#include "ns3/error-model.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tuple.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-utils.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/yans-wifi-phy.h"

#include <algorithm>
#include <filesystem>
#include <iterator>
#include <optional>
#include <variant>

using namespace ns3;

/// \ingroup wifi-test
/// \ingroup tests
/// \brief Fast Initial Link Setup (FILS) frame Test Suite
/// Test suite intended to test (de)serialization and timing
/// of frames associated with FILS procedure.
/// The test creates a BSS consisting of an AP and client and
/// and analyzes the timings and contents of frames associated
/// with FILS procedure.

NS_LOG_COMPONENT_DEFINE("WifiFilsFrameTestSuite");

static const auto DEFAULT_BANDWIDTH = 20;
static const auto INVALID_CHAN_NUM = 0;
static const auto DEFAULT_PRIMARY_INDEX = 0;
static const auto DEFAULT_SIM_STOP_TIME = MilliSeconds(610);
static const auto DEFAULT_RNG_SEED = 3;
static const auto DEFAULT_RNG_RUN = 7;
static const auto DEFAULT_STREAM_INDEX = 100;
static const auto DUMMY_AP_ADDR = Mac48Address("00:00:00:00:00:10");
static const auto DEFAULT_STANDARD = WifiStandard::WIFI_STANDARD_80211ax;
static const auto DEFAULT_BAND = WifiPhyBand::WIFI_PHY_BAND_6GHZ;
static const auto DEFAULT_SSID = "01234567890123456789012345678901"; // max length (32 bytes)
static const auto DEFAULT_BCN_INTRVL = 100 * WIFI_TU;
static const auto DEFAULT_FILS_INTRVL = 20 * WIFI_TU;
static const auto DEFAULT_TIMING_TOLERANCE = MicroSeconds(100);
static const auto DEFAULT_UNSOL_PROBE_RESP_EN = false;
static const auto DEFAULT_PCAP_PREFIX = "ap-fils";
static const auto DEFAULT_OUTDIR = ".";
static const auto DEFAULT_ENABLE_PCAP = false;
static const auto DEFAULT_AP_LOC = Vector(0.01, 0, 0);
static const auto DEFAULT_CLIENT_LOC = Vector(0, 0, 0);
static const uint8_t WIFI_6GHZ_FD_PHY_IDX = 4;

/// @brief  Wi-Fi FILS frame test parameters
struct WifiFilsFrameTestParams
{
    uint16_t bw{DEFAULT_BANDWIDTH};                     ///< Operation bandwidth
    std::string ssid{DEFAULT_SSID};                     ///< SSID name
    uint8_t nss{0};                                     ///< Number of spatial streams
    Time bcnIntrvl{DEFAULT_BCN_INTRVL};                 ///< Time between Beacons
    Time filsIntrvl{DEFAULT_FILS_INTRVL};               ///< Time between FILS frames
    bool unsolProbeRespEn{DEFAULT_UNSOL_PROBE_RESP_EN}; ///< Unsolicited Probe Response enable
    uint8_t expNssFld{0};                               ///< Expected NSS field
    uint8_t expChWidFld{0};                             ///< Expected Channel Width field
};

/// @ingroup wifi-test
/// @ingroup tests
/// Test FILS frames
class WifiFilsFrameTest : public TestCase
{
  public:
    /// @brief  constructor
    /// @param params the parameters for this test
    WifiFilsFrameTest(const WifiFilsFrameTestParams& params);

    /// Transmitted PSDUs
    struct PsduCapture
    {
        Time timeSt;              ///< timestamp
        Ptr<const WifiPsdu> psdu; ///< PSDU
    };

  private:
    /// Timing statistic for test validation
    struct TimeStats
    {
        size_t cntBcns{0};                  ///< Beacon frames count
        Time bcnTimeSt{0};                  ///< last Beacon timestamp
        size_t cntFilsOrUnsolProbeResps{0}; ///< FILS Discovery or Unsolicited Probe Response count
        Time filsOrUnsolProbeRespTimeSt{0}; ///< last FILS Discovery or Unsolicited Probe Response
                                            ///< timestamp
        Time filsOrUnsolProbeRespTimeDelta{0}; ///< time between last FILS Discovery or Unsolicited
                                               ///< Probe Response and last Beacon
    };

    /// @brief setup a WifiNetDevice
    /// @param channel the channel to attach to
    /// @param isAp whether the device is an AP
    /// @return the created WifiNetDevice
    Ptr<WifiNetDevice> SetupDevice(Ptr<YansWifiChannel>& channel, bool isAp);

    /// @brief callback connected to PSDU TX begin trace source
    /// @param psduMap the transmitted PSDU map
    /// @param txVector the TXVECTOR
    /// @param txPowerW the TX power in Watts
    void PsduTxCallback(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /// @brief validate the given FILS Discovery frame
    /// @param filsDisc the FILS Discovery frame
    void ValidateFilsDiscFrame(const FilsDiscHeader& filsDisc);

    /// @brief check the correctness of the test
    void ValidateTest();

    /// @brief Check the number of FILS Discovery frames or unsolicited Probe Response frames
    ///        transmitted since the last Beacon frame
    /// @param psduCapt information about the transmitted Beacon frame
    void ValidateCnt(const PsduCapture& psduCapt);

    /// @brief check the timing of the transmitted FILS Discovery or unsolicited Probe Response
    /// @param psduCapt information about the FILS Discovery or unsolicited Probe Response
    void ValidateTiming(const PsduCapture& psduCapt);

    /// @brief Get the FILS Discovery header, if present in the given frame
    /// @param psduCapt information about the given frame
    /// @return the FILS Discovery header, if present
    std::optional<FilsDiscHeader> GetFilsDiscFrame(const PsduCapture& psduCapt);

    void DoSetup() override;
    void DoTeardown() override;
    void DoRun() override;

    TimeStats m_timeStats;                ///< collected timing statistic
    Ptr<WifiNetDevice> m_ap{nullptr};     ///< AP device
    Ptr<WifiNetDevice> m_client{nullptr}; ///< Client device
    WifiFilsFrameTestParams m_params;     ///< Test parameters
    std::vector<PsduCapture> m_txPsdus{}; ///< TX PSDUS frame infos
};

WifiFilsFrameTest::WifiFilsFrameTest(const WifiFilsFrameTestParams& params)
    : TestCase("WifiFilsFrameTest"),
      m_params(params)
{
}

Ptr<WifiNetDevice>
WifiFilsFrameTest::SetupDevice(Ptr<YansWifiChannel>& channel, bool isAp)
{
    NodeContainer node;
    YansWifiPhyHelper phy;
    WifiMacHelper mac;
    WifiHelper wifi;
    MobilityHelper mobility;
    node.Create(1);
    phy.SetChannel(channel);
    TupleValue<UintegerValue, UintegerValue, EnumValue<WifiPhyBand>, UintegerValue> channelValue;
    channelValue.Set(
        WifiPhy::ChannelTuple{INVALID_CHAN_NUM, m_params.bw, DEFAULT_BAND, DEFAULT_PRIMARY_INDEX});
    phy.Set("ChannelSettings", channelValue);
    phy.Set("Antennas", UintegerValue(m_params.nss));
    phy.Set("MaxSupportedTxSpatialStreams", UintegerValue(m_params.nss));
    phy.Set("MaxSupportedRxSpatialStreams", UintegerValue(m_params.nss));

    wifi.SetStandard(DEFAULT_STANDARD);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");

    if (isAp)
    {
        mac.SetType("ns3::ApWifiMac",
                    "Ssid",
                    SsidValue(Ssid(m_params.ssid)),
                    "BeaconGeneration",
                    BooleanValue(true),
                    "BeaconInterval",
                    TimeValue(m_params.bcnIntrvl),
                    "FdBeaconInterval6GHz",
                    TimeValue(m_params.filsIntrvl),
                    "SendUnsolProbeResp",
                    BooleanValue(m_params.unsolProbeRespEn));
    }
    else
    {
        mac.SetType("ns3::StaWifiMac",
                    "Ssid",
                    SsidValue(Ssid(m_params.ssid)),
                    "ActiveProbing",
                    BooleanValue(false));
    }
    auto testDevs = wifi.Install(phy, mac, node);
    wifi.AssignStreams(testDevs, DEFAULT_STREAM_INDEX);
    auto dev = DynamicCast<WifiNetDevice>(testDevs.Get(0));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(node);
    node.Get(0)->GetObject<MobilityModel>()->SetPosition(isAp ? DEFAULT_AP_LOC
                                                              : DEFAULT_CLIENT_LOC);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    if (isAp && DEFAULT_ENABLE_PCAP)
    {
        auto path = std::filesystem::path(DEFAULT_OUTDIR);
        phy.EnablePcap(path.append(DEFAULT_PCAP_PREFIX).string(), testDevs);
    }
    return dev;
}

void
WifiFilsFrameTest::PsduTxCallback(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    auto psdu = psduMap[SU_STA_ID];
    m_txPsdus.push_back({Simulator::Now(), psdu});
    NS_LOG_DEBUG("MPDU " << **psdu->begin());
}

void
WifiFilsFrameTest::DoSetup()
{
    RngSeedManager::SetSeed(DEFAULT_RNG_SEED);
    RngSeedManager::SetRun(DEFAULT_RNG_RUN);
    auto channel = YansWifiChannelHelper::Default().Create();
    // setup devices and capabilities
    m_ap = SetupDevice(channel, true);
    m_client = SetupDevice(channel, false);
    // setup AP TX PSDU trace
    auto phy = m_ap->GetPhy();
    phy->TraceConnectWithoutContext("PhyTxPsduBegin",
                                    MakeCallback(&WifiFilsFrameTest::PsduTxCallback, this));
}

void
WifiFilsFrameTest::ValidateCnt(const PsduCapture& psduCapt)
{
    if (m_timeStats.cntBcns > 0)
    {
        NS_TEST_ASSERT_MSG_EQ(
            m_timeStats.cntFilsOrUnsolProbeResps,
            static_cast<std::size_t>((m_params.bcnIntrvl / m_params.filsIntrvl).GetHigh() - 1),
            "Number of FILS or Unsolicited Response Frames per Beacon Interval is not expected");
    }
    m_timeStats.bcnTimeSt = psduCapt.timeSt;
    m_timeStats.cntBcns++;
    m_timeStats.cntFilsOrUnsolProbeResps = 0;
}

void
WifiFilsFrameTest::ValidateTiming(const PsduCapture& psduCapt)
{
    m_timeStats.filsOrUnsolProbeRespTimeDelta =
        psduCapt.timeSt - (m_timeStats.cntFilsOrUnsolProbeResps > 0
                               ? m_timeStats.filsOrUnsolProbeRespTimeSt
                               : m_timeStats.bcnTimeSt);
    m_timeStats.filsOrUnsolProbeRespTimeSt = psduCapt.timeSt;
    m_timeStats.cntFilsOrUnsolProbeResps++;
    NS_TEST_ASSERT_MSG_EQ_TOL(
        m_timeStats.filsOrUnsolProbeRespTimeDelta,
        m_params.filsIntrvl,
        DEFAULT_TIMING_TOLERANCE,
        "Timing of FILS or Unsolicited Response frames is not as expected at time "
            << psduCapt.timeSt.GetTimeStep());
}

std::optional<FilsDiscHeader>
WifiFilsFrameTest::GetFilsDiscFrame(const PsduCapture& psduCapt)
{
    auto pkt = psduCapt.psdu->GetPayload(0)->Copy();
    WifiActionHeader actionHdr;
    pkt->RemoveHeader(actionHdr);
    if ((actionHdr.GetCategory() == WifiActionHeader::PUBLIC) &&
        (actionHdr.GetAction().publicAction == WifiActionHeader::FILS_DISCOVERY))
    {
        FilsDiscHeader filsDisc;
        pkt->PeekHeader(filsDisc);
        return filsDisc;
    }
    return std::nullopt;
}

void
WifiFilsFrameTest::ValidateTest()
{
    for (const auto& psduCapt : m_txPsdus)
    {
        auto hdr = psduCapt.psdu->GetHeader(0);
        if (hdr.IsBeacon())
        {
            ValidateCnt(psduCapt);
        }
        else
        {
            if (m_params.unsolProbeRespEn && hdr.IsProbeResp() && hdr.GetAddr1().IsBroadcast())
            { // Unsolicited Probe Response frame
                ValidateTiming(psduCapt);
            }
            else if (hdr.IsAction())
            { // possible FILS Discovery frame
                if (auto filsDisc = GetFilsDiscFrame(psduCapt))
                {
                    ValidateFilsDiscFrame(filsDisc.value());
                    ValidateTiming(psduCapt);
                }
            }
        }
        if (m_timeStats.cntBcns == DEFAULT_SIM_STOP_TIME / m_params.bcnIntrvl)
        {
            break;
        }
    }
}

void
WifiFilsFrameTest::ValidateFilsDiscFrame(const FilsDiscHeader& filsDisc)
{
    NS_TEST_ASSERT_MSG_EQ(filsDisc.GetSsid(), m_params.ssid, "FILS Discovery frame SSID mismatch");
    NS_TEST_ASSERT_MSG_EQ(+filsDisc.m_fdCap->m_chWidth,
                          +m_params.expChWidFld,
                          "FILS Discovery frame channel width mismatch");
    NS_TEST_ASSERT_MSG_EQ(+filsDisc.m_fdCap->m_maxNss,
                          +m_params.expNssFld,
                          "FILS Discovery frame NSS mismatch");
    NS_TEST_ASSERT_MSG_EQ(+filsDisc.m_fdCap->m_phyIdx,
                          +WIFI_6GHZ_FD_PHY_IDX,
                          "FILS Discovery frame PHY idx mismatch");
}

void
WifiFilsFrameTest::DoRun()
{
    Simulator::Stop(DEFAULT_SIM_STOP_TIME);
    Simulator::Run();
    Simulator::Destroy();
    ValidateTest();
}

void
WifiFilsFrameTest::DoTeardown()
{
    m_ap->Dispose();
    m_ap = nullptr;
    m_client->Dispose();
    m_client = nullptr;
    m_txPsdus.clear();
}

/// Testcases for FILS frame test
enum class WifiFilsFrameTestCase : uint8_t
{
    BW20MHZ_NSS1_DISC = 0,
    BW20MHZ_NSS3_DISC,
    BW40MHZ_NSS2_DISC,
    BW80MHZ_NSS2_DISC,
    BW160MHZ_NSS2_DISC,
    BW160MHZ_NSS2_PROBE,
    COUNT,
};

WifiFilsFrameTestParams
WifiFilsFrameTestBuildCase(const WifiFilsFrameTestCase& tc)
{
    WifiFilsFrameTestParams params{};
    switch (tc)
    {
    case WifiFilsFrameTestCase::BW20MHZ_NSS1_DISC:
        params.bw = 20;
        params.ssid = DEFAULT_SSID;
        params.nss = 1;
        params.expChWidFld = 0;
        params.expNssFld = 0;
        break;
    case WifiFilsFrameTestCase::BW20MHZ_NSS3_DISC:
        params.bw = 20;
        params.ssid = "BW20MHZ_NSS3";
        params.nss = 3;
        params.filsIntrvl = 15 * WIFI_TU;
        params.expChWidFld = 0;
        params.expNssFld = 2;
        break;
    case WifiFilsFrameTestCase::BW40MHZ_NSS2_DISC:
        params.bw = 40;
        params.ssid = "BW40MHZ_NSS2";
        params.nss = 2;
        params.filsIntrvl = 10 * WIFI_TU;
        params.expChWidFld = 1;
        params.expNssFld = 1;
        break;
    case WifiFilsFrameTestCase::BW80MHZ_NSS2_DISC:
        params.bw = 80;
        params.ssid = "BW80MHZ_NSS2";
        params.nss = 2;
        params.filsIntrvl = 7 * WIFI_TU;
        params.expChWidFld = 2;
        params.expNssFld = 1;
        break;
    case WifiFilsFrameTestCase::BW160MHZ_NSS2_DISC:
        params.bw = 160;
        params.ssid = "BW160MHZ_NSS2";
        params.nss = 2;
        params.filsIntrvl = 5 * WIFI_TU;
        params.expChWidFld = 3;
        params.expNssFld = 1;
        break;
    case WifiFilsFrameTestCase::BW160MHZ_NSS2_PROBE:
        params.bw = 160;
        params.ssid = "BW160MHZ_NSS2";
        params.nss = 2;
        params.unsolProbeRespEn = true;
        params.expChWidFld = 3;
        params.expNssFld = 1;
        break;
    default:
        NS_ABORT_MSG("Testcase is unsupported");
        break;
    }
    return params;
}

/// \ingroup wifi-test
/// \ingroup tests
/// \brief WiFi FILS frame Test Suite
class WifiFilsFrameTestSuite : public TestSuite
{
  public:
    WifiFilsFrameTestSuite();
};

WifiFilsFrameTestSuite::WifiFilsFrameTestSuite()
    : TestSuite("wifi-fils-frame", Type::UNIT)
{
    std::vector<WifiFilsFrameTestParams> testCases{
        {WifiFilsFrameTestBuildCase(WifiFilsFrameTestCase::BW20MHZ_NSS1_DISC)},
        {WifiFilsFrameTestBuildCase(WifiFilsFrameTestCase::BW20MHZ_NSS3_DISC)},
        {WifiFilsFrameTestBuildCase(WifiFilsFrameTestCase::BW40MHZ_NSS2_DISC)},
        {WifiFilsFrameTestBuildCase(WifiFilsFrameTestCase::BW80MHZ_NSS2_DISC)},
        {WifiFilsFrameTestBuildCase(WifiFilsFrameTestCase::BW160MHZ_NSS2_DISC)},
        {WifiFilsFrameTestBuildCase(WifiFilsFrameTestCase::BW160MHZ_NSS2_PROBE)},
    };
    for (const auto& tc : testCases)
    {
        AddTestCase(new WifiFilsFrameTest(tc), TestCase::Duration::QUICK);
    }
}

static WifiFilsFrameTestSuite g_WifiFilsFrameTestSuite; /// the test suite
