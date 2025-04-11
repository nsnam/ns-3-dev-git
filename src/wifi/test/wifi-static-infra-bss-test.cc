/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/assert.h"
#include "ns3/boolean.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-container.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/rr-multi-user-scheduler.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ns3-constants.h"
#include "ns3/wifi-static-setup-helper.h"
#include "ns3/wifi-utils.h"

#include <algorithm>
#include <optional>
#include <unordered_map>

/// @ingroup wifi-test
/// @ingroup tests
/// @brief WifiStaticSetupHelper test suite
/// Test suite intended to test static management exchanges between
/// AP device and client device for single link and multi
/// link operations.
/// The test prepares AP WifiNetDevice and client WifiNetDevice
/// based on test vector input and performs static exchanges for
/// association, Block ACK agreement, UL MU disable etc.
/// using WifiStaticSetupHelper. The test verifies if state machines
/// at ApWifiMac and StaWifiMac has been updated correctly.

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WifiStaticInfraBssTestSuite");

/// @brief Constants used in test suite
namespace WifiStaticInfraBssTestConstants
{
const auto DEFAULT_RNG_SEED = 3;                    ///< default RNG seed
const auto DEFAULT_RNG_RUN = 7;                     ///< default RNG run
const auto DEFAULT_STREAM_INDEX = 100;              ///< default stream index
const auto DEFAULT_SIM_STOP_TIME = NanoSeconds(1);  ///< default simulation stop time
const auto DEFAULT_BEACON_GEN = false;              ///< default beacon generation value
const auto DEFAULT_DATA_MODE = "HeMcs3";            ///< default data mode
const auto DEFAULT_CONTROL_MODE = "OfdmRate24Mbps"; ///< default control mode
const auto DEFAULT_WIFI_STANDARD = WifiStandard::WIFI_STANDARD_80211be; ///< default Wi-Fi standard
const auto DEFAULT_SSID = Ssid("wifi-static-setup");                    ///< default SSID
const tid_t DEFAULT_TEST_TID = 0;                                       ///< default TID
const uint16_t DEFAULT_BA_BUFFER_SIZE = 64;  ///< default MPDU buffer size
const uint8_t DEFAULT_WIFI_UL_MU_NUM_RU = 4; ///< default number of RUs in UL MU PPDUs
} // namespace WifiStaticInfraBssTestConstants

namespace consts = WifiStaticInfraBssTestConstants;

/// @brief channel map typedef
using ChannelMap = std::unordered_map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>>;

/// @brief test case information
struct WifiStaticInfraBssTestVector
{
    std::string name;                                      ///< Test case name
    StringVector apChs{};                                  ///< Channel setting for AP device
    StringVector clientChs{};                              ///< Channel settings for client device
    uint16_t apBufferSize{consts::DEFAULT_BA_BUFFER_SIZE}; ///< Originator Buffer Size
    uint16_t clientBufferSize{consts::DEFAULT_BA_BUFFER_SIZE}; ///< Recipient Buffer Size
    std::optional<Ipv4Address> apMulticastIp{std::nullopt};    ///< AP multicast IP
    bool ulMuDataDisable{DEFAULT_WIFI_UL_MU_DATA_DISABLE};     ///< UL MU Data Disable
};

/**
 * Test static setup of an infrastructure BSS.
 */
class WifiStaticInfraBssTest : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param testVec the test vector
     */
    WifiStaticInfraBssTest(const WifiStaticInfraBssTestVector& testVec);

  private:
    /// Construct WifiNetDevice
    /// @param isAp true if AP, false otherwise
    /// @param channelMap created spectrum channels
    /// @return constructed WifiNetDevice
    Ptr<WifiNetDevice> GetWifiNetDevice(bool isAp, const ChannelMap& channelMap);

    /// Construct PHY helper based on input operating channels
    /// @param settings vector of strings specifying the operating channels to configure
    /// @param channelMap created spectrum channels
    /// @return PHY helper
    SpectrumWifiPhyHelper GetPhyHelper(const StringVector& settings,
                                       const ChannelMap& channelMap) const;

    /// @return the WifiHelper
    WifiHelper GetWifiHelper() const;
    /// @return the AP MAC helper
    WifiMacHelper GetApMacHelper() const;
    /// @return the Client MAC helper
    WifiMacHelper GetClientMacHelper() const;
    void ValidateAssoc(); ///< Validate Association

    /// Validate Multi-user scheduler setup
    /// @param apMac AP MAC
    /// @param clientMac Non-AP MAC
    void ValidateMuScheduler(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac);

    /// Validate association state machine at AP and client
    /// for input link
    /// @param clientLinkId client local Link ID
    /// @param apMac AP MAC
    /// @param clientMac Client MAC
    void ValidateAssocForLink(linkId_t clientLinkId,
                              Ptr<ApWifiMac> apMac,
                              Ptr<StaWifiMac> clientMac);

    /// Validate Block ACK Agreement at AP and client
    /// @param apMac AP MAC
    /// @param clientMac Client MAC
    void ValidateBaAgr(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac);

    void DoRun() override;
    void DoSetup() override;

    WifiStaticInfraBssTestVector m_testVec;       ///< Test vector
    Ptr<WifiNetDevice> m_apDev{nullptr};          ///< AP WiFi device
    Ptr<WifiNetDevice> m_clientDev{nullptr};      ///< client WiFi device
    std::optional<Mac48Address> m_apGcrGroupAddr; ///< GCR group address
};

WifiStaticInfraBssTest::WifiStaticInfraBssTest(const WifiStaticInfraBssTestVector& testVec)
    : TestCase(testVec.name),
      m_testVec(testVec)
{
}

WifiHelper
WifiStaticInfraBssTest::GetWifiHelper() const
{
    WifiHelper wifiHelper;
    wifiHelper.SetStandard(consts::DEFAULT_WIFI_STANDARD);
    wifiHelper.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                       "DataMode",
                                       StringValue(consts::DEFAULT_DATA_MODE),
                                       "ControlMode",
                                       StringValue(consts::DEFAULT_CONTROL_MODE));
    return wifiHelper;
}

SpectrumWifiPhyHelper
WifiStaticInfraBssTest::GetPhyHelper(const StringVector& settings,
                                     const ChannelMap& channelMap) const
{
    NS_ASSERT(!settings.empty());
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
WifiStaticInfraBssTest::GetApMacHelper() const
{
    WifiMacHelper macHelper;
    auto ssid = Ssid(consts::DEFAULT_SSID);

    macHelper.SetType("ns3::ApWifiMac",
                      "Ssid",
                      SsidValue(ssid),
                      "BeaconGeneration",
                      BooleanValue(consts::DEFAULT_BEACON_GEN),
                      "MpduBufferSize",
                      UintegerValue(m_testVec.apBufferSize));
    macHelper.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
                                    "NStations",
                                    UintegerValue(consts::DEFAULT_WIFI_UL_MU_NUM_RU));
    return macHelper;
}

WifiMacHelper
WifiStaticInfraBssTest::GetClientMacHelper() const
{
    WifiMacHelper macHelper;
    Ssid ssid = Ssid(consts::DEFAULT_SSID);
    macHelper.SetType("ns3::StaWifiMac",
                      "Ssid",
                      SsidValue(ssid),
                      "MpduBufferSize",
                      UintegerValue(m_testVec.clientBufferSize));
    return macHelper;
}

Ptr<WifiNetDevice>
WifiStaticInfraBssTest::GetWifiNetDevice(bool isAp, const ChannelMap& channelMap)
{
    NodeContainer node(1);
    auto wifiHelper = GetWifiHelper();
    auto settings = isAp ? m_testVec.apChs : m_testVec.clientChs;
    auto phyHelper = GetPhyHelper(settings, channelMap);
    auto macHelper = isAp ? GetApMacHelper() : GetClientMacHelper();
    auto netDev = wifiHelper.Install(phyHelper, macHelper, node);
    WifiHelper::AssignStreams(netDev, consts::DEFAULT_STREAM_INDEX);
    return DynamicCast<WifiNetDevice>(netDev.Get(0));
}

void
WifiStaticInfraBssTest::DoSetup()
{
    RngSeedManager::SetSeed(consts::DEFAULT_RNG_SEED);
    RngSeedManager::SetRun(consts::DEFAULT_RNG_RUN);

    ChannelMap channelMap = {{WIFI_PHY_BAND_2_4GHZ, CreateObject<MultiModelSpectrumChannel>()},
                             {WIFI_PHY_BAND_5GHZ, CreateObject<MultiModelSpectrumChannel>()},
                             {WIFI_PHY_BAND_6GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    m_apDev = GetWifiNetDevice(true, channelMap); // AP
    NS_ASSERT(m_apDev);
    m_clientDev = GetWifiNetDevice(false, channelMap); // Client
    NS_ASSERT(m_clientDev);

    WifiStaticSetupHelper::SetStaticAssociation(m_apDev, m_clientDev);
    if (auto multicastIp = m_testVec.apMulticastIp)
    {
        NS_ASSERT_MSG(multicastIp->IsMulticast(),
                      "Assigned IP " << multicastIp.value() << " is not multicast");
        m_apGcrGroupAddr = Mac48Address::ConvertFrom(m_apDev->GetMulticast(multicastIp.value()));
    }
    WifiStaticSetupHelper::SetStaticBlockAck(m_apDev,
                                             m_clientDev,
                                             consts::DEFAULT_TEST_TID,
                                             m_apGcrGroupAddr);
    WifiStaticSetupHelper::SetStaticBlockAck(m_clientDev, m_apDev, consts::DEFAULT_TEST_TID);
}

void
WifiStaticInfraBssTest::ValidateAssocForLink(linkId_t clientLinkId,
                                             Ptr<ApWifiMac> apMac,
                                             Ptr<StaWifiMac> clientMac)
{
    const auto isMldAssoc = (apMac->GetNLinks() > 1) && (clientMac->GetNLinks() > 1);
    const auto apLinkId = clientLinkId;
    const auto clientFem = clientMac->GetFrameExchangeManager(clientLinkId);
    const auto apFem = apMac->GetFrameExchangeManager(apLinkId);
    const auto staAddr = clientFem->GetAddress();
    const auto apAddr = apFem->GetAddress();
    const auto staRemoteMgr = clientMac->GetWifiRemoteStationManager(clientLinkId);
    const auto apRemoteMgr = apMac->GetWifiRemoteStationManager(apLinkId);

    NS_TEST_ASSERT_MSG_EQ(clientFem->GetBssid(),
                          apAddr,
                          "Unexpected BSSID for STA link ID " << +clientLinkId);
    NS_TEST_ASSERT_MSG_EQ(apRemoteMgr->IsAssociated(staAddr),
                          true,
                          "Expecting STA " << staAddr << " to be associated on AP link "
                                           << +apLinkId);

    const auto aid = apMac->GetAssociationId(staAddr, apLinkId);
    NS_TEST_ASSERT_MSG_EQ(apMac->GetStaList(apLinkId).contains(aid),
                          true,
                          "STA " << staAddr << " not found in list of associated STAs");

    if (!isMldAssoc)
    {
        return;
    }

    NS_TEST_ASSERT_MSG_EQ((staRemoteMgr->GetMldAddress(apAddr) == apMac->GetAddress()),
                          true,
                          "Incorrect MLD address stored by STA on link ID " << +clientLinkId);
    NS_TEST_ASSERT_MSG_EQ((staRemoteMgr->GetAffiliatedStaAddress(apMac->GetAddress()) == apAddr),
                          true,
                          "Incorrect affiliated address stored by STA on link ID "
                              << +clientLinkId);

    NS_TEST_ASSERT_MSG_EQ((apRemoteMgr->GetMldAddress(staAddr) == clientMac->GetAddress()),
                          true,
                          "Incorrect MLD address stored by AP on link ID " << +apLinkId);
    NS_TEST_ASSERT_MSG_EQ(
        (apRemoteMgr->GetAffiliatedStaAddress(clientMac->GetAddress()) == staAddr),
        true,
        "Incorrect affiliated address stored by AP on link ID " << +apLinkId);
}

void
WifiStaticInfraBssTest::ValidateMuScheduler(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac)
{
    auto muScheduler = apMac->GetObject<RrMultiUserScheduler>();
    NS_ASSERT(muScheduler);
    auto clientList = muScheduler->GetUlMuStas();
    std::size_t expectedSize = m_testVec.ulMuDataDisable ? 0 : 1;
    if (expectedSize == 0)
    {
        return;
    }
    auto clientAddr = clientList.front().address;
    auto expectedAddr = clientMac->GetAddress();
    NS_TEST_ASSERT_MSG_EQ(clientAddr, expectedAddr, "Client MAC address mismatch");
}

void
WifiStaticInfraBssTest::ValidateBaAgr(Ptr<ApWifiMac> apMac, Ptr<StaWifiMac> clientMac)
{
    auto isMldAssoc = (apMac->GetNLinks() > 1) && (clientMac->GetNLinks() > 1);
    auto setupLinks = clientMac->GetSetupLinkIds();
    auto linkId = *(setupLinks.begin());
    auto apAddr =
        isMldAssoc ? apMac->GetAddress() : clientMac->GetFrameExchangeManager(linkId)->GetBssid();
    auto clientAddr = isMldAssoc ? clientMac->GetAddress()
                                 : clientMac->GetFrameExchangeManager(linkId)->GetAddress();

    auto expectedBufferSize = std::min(m_testVec.apBufferSize, m_testVec.clientBufferSize);

    // AP Block ACK Manager
    auto baApOrig = apMac->GetBaAgreementEstablishedAsOriginator(clientAddr,
                                                                 consts::DEFAULT_TEST_TID,
                                                                 m_apGcrGroupAddr);
    NS_TEST_ASSERT_MSG_EQ(baApOrig.has_value(),
                          true,
                          "BA Agreement not established at AP as originator");
    NS_TEST_ASSERT_MSG_EQ(baApOrig->get().GetBufferSize(),
                          expectedBufferSize,
                          "BA Agreement buffer size mismatch");
    auto baApRecip =
        apMac->GetBaAgreementEstablishedAsRecipient(clientAddr, consts::DEFAULT_TEST_TID);
    NS_TEST_ASSERT_MSG_EQ(baApRecip.has_value(),
                          true,
                          "BA Agreement not established at AP as recipient");
    NS_TEST_ASSERT_MSG_EQ(baApRecip->get().GetBufferSize(),
                          expectedBufferSize,
                          "BA Agreement buffer size mismatch");

    // Non-AP Block ACK Manager
    auto baClientOrig =
        clientMac->GetBaAgreementEstablishedAsOriginator(apAddr, consts::DEFAULT_TEST_TID);
    NS_TEST_ASSERT_MSG_EQ(baClientOrig.has_value(),
                          true,
                          "BA Agreement not established at client as originator");
    NS_TEST_ASSERT_MSG_EQ(baClientOrig->get().GetBufferSize(),
                          expectedBufferSize,
                          "BA Agreement buffer size mismatch");
    auto baClientRecip = clientMac->GetBaAgreementEstablishedAsRecipient(apAddr,
                                                                         consts::DEFAULT_TEST_TID,
                                                                         m_apGcrGroupAddr);
    NS_TEST_ASSERT_MSG_EQ(baClientRecip.has_value(),
                          true,
                          "BA Agreement not established at client as recipient");
    NS_TEST_ASSERT_MSG_EQ(baClientOrig->get().GetBufferSize(),
                          expectedBufferSize,
                          "BA Agreement buffer size mismatch");
}

void
WifiStaticInfraBssTest::ValidateAssoc()
{
    auto apMac = DynamicCast<ApWifiMac>(m_apDev->GetMac());
    NS_ASSERT(apMac);
    auto clientMac = DynamicCast<StaWifiMac>(m_clientDev->GetMac());
    NS_ASSERT(clientMac);

    NS_TEST_ASSERT_MSG_EQ(clientMac->IsAssociated(), true, "Expected the STA to be associated");
    const auto nClientLinks = m_testVec.clientChs.size();
    auto clientLinkIds = clientMac->GetLinkIds();
    NS_TEST_EXPECT_MSG_EQ(clientLinkIds.size(), nClientLinks, "Client number of links mismatch");
    for (auto linkId : clientLinkIds)
    {
        ValidateAssocForLink(linkId, apMac, clientMac);
    }

    ValidateBaAgr(apMac, clientMac);
    ValidateMuScheduler(apMac, clientMac);
}

void
WifiStaticInfraBssTest::DoRun()
{
    Simulator::Stop(consts::DEFAULT_SIM_STOP_TIME);
    Simulator::Run();
    ValidateAssoc();
    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief WifiStaticSetupHelper test suite
 */
class WifiStaticInfraBssTestSuite : public TestSuite
{
  public:
    WifiStaticInfraBssTestSuite();
};

WifiStaticInfraBssTestSuite::WifiStaticInfraBssTestSuite()
    : TestSuite("wifi-static-infra-bss-test", Type::UNIT)
{
    for (const std::vector<WifiStaticInfraBssTestVector> inputs{
             {"AP-1-link-Client-1-link",
              {"{36, 0, BAND_5GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"AP-1-link-Client-1-link-multicast",
              {"{36, 0, BAND_5GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              "239.192.1.1",
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"AP-2-link-Client-1-link",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"AP-2-link-Client-1-link-Diff-Order",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}"},
              {"{2, 0, BAND_2_4GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"AP-3-link-Client-2-link",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"AP-3-link-Client-2-link-Diff-Order",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"AP-3-link-Client-3-link",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"AP-80MHz-Client-20MHz",
              {"{42, 80, BAND_5GHZ, 0}"},
              {"{36, 20, BAND_5GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"Single-linkBuffer-Size-Test",
              {"{36, 0, BAND_5GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}"},
              64,
              256,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"Single-linkBuffer-Size-Test-Alt",
              {"{36, 0, BAND_5GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}"},
              1024,
              256,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"Multi-link-Buffer-Size-Test",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              256,
              64,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"Multi-link-Buffer-Size-Test-Alt",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              1024,
              1024,
              std::nullopt,
              DEFAULT_WIFI_UL_MU_DATA_DISABLE},
             {"Single-link-UL-MU-Disable",
              {"{36, 0, BAND_5GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              true},
             {"2-link-UL-MU-Disable",
              {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
              consts::DEFAULT_BA_BUFFER_SIZE,
              consts::DEFAULT_BA_BUFFER_SIZE,
              std::nullopt,
              true}};

         const auto& input : inputs)
    {
        AddTestCase(new WifiStaticInfraBssTest(input), TestCase::Duration::QUICK);
    }
}

static WifiStaticInfraBssTestSuite g_wifiStaticInfraBssTestSuite;
