/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/config.h"
#include "ns3/he-configuration.h"
#include "ns3/he-frame-exchange-manager.h"
#include "ns3/he-phy.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-link-element.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/qos-utils.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"

#include <iomanip>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMloTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the implementation of WifiAssocManager::GetNextAffiliatedAp(), which
 * searches a given RNR element for APs affiliated to the same AP MLD as the
 * reporting AP that sent the frame containing the element.
 */
class GetRnrLinkInfoTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    GetRnrLinkInfoTest();
    ~GetRnrLinkInfoTest() override = default;

  private:
    void DoRun() override;
};

GetRnrLinkInfoTest::GetRnrLinkInfoTest()
    : TestCase("Check the implementation of WifiAssocManager::GetNextAffiliatedAp()")
{
}

void
GetRnrLinkInfoTest::DoRun()
{
    ReducedNeighborReport rnr;
    std::size_t nbrId;
    std::size_t tbttId;

    // Add a first Neighbor AP Information field without MLD Parameters
    rnr.AddNbrApInfoField();
    nbrId = rnr.GetNNbrApInfoFields() - 1;

    rnr.AddTbttInformationField(nbrId);
    rnr.AddTbttInformationField(nbrId);

    // Add a second Neighbor AP Information field with MLD Parameters; the first
    // TBTT Information field is related to an AP affiliated to the same AP MLD
    // as the reported AP; the second TBTT Information field is not (it does not
    // make sense that two APs affiliated to the same AP MLD are using the same
    // channel).
    rnr.AddNbrApInfoField();
    nbrId = rnr.GetNNbrApInfoFields() - 1;

    rnr.AddTbttInformationField(nbrId);
    tbttId = rnr.GetNTbttInformationFields(nbrId) - 1;
    rnr.SetMldParameters(nbrId, tbttId, 0, 0, 0);

    rnr.AddTbttInformationField(nbrId);
    tbttId = rnr.GetNTbttInformationFields(nbrId) - 1;
    rnr.SetMldParameters(nbrId, tbttId, 5, 0, 0);

    // Add a third Neighbor AP Information field with MLD Parameters; none of the
    // TBTT Information fields is related to an AP affiliated to the same AP MLD
    // as the reported AP.
    rnr.AddNbrApInfoField();
    nbrId = rnr.GetNNbrApInfoFields() - 1;

    rnr.AddTbttInformationField(nbrId);
    tbttId = rnr.GetNTbttInformationFields(nbrId) - 1;
    rnr.SetMldParameters(nbrId, tbttId, 3, 0, 0);

    rnr.AddTbttInformationField(nbrId);
    tbttId = rnr.GetNTbttInformationFields(nbrId) - 1;
    rnr.SetMldParameters(nbrId, tbttId, 4, 0, 0);

    // Add a fourth Neighbor AP Information field with MLD Parameters; the first
    // TBTT Information field is not related to an AP affiliated to the same AP MLD
    // as the reported AP; the second TBTT Information field is.
    rnr.AddNbrApInfoField();
    nbrId = rnr.GetNNbrApInfoFields() - 1;

    rnr.AddTbttInformationField(nbrId);
    tbttId = rnr.GetNTbttInformationFields(nbrId) - 1;
    rnr.SetMldParameters(nbrId, tbttId, 6, 0, 0);

    rnr.AddTbttInformationField(nbrId);
    tbttId = rnr.GetNTbttInformationFields(nbrId) - 1;
    rnr.SetMldParameters(nbrId, tbttId, 0, 0, 0);

    // check implementation of WifiAssocManager::GetNextAffiliatedAp()
    auto ret = WifiAssocManager::GetNextAffiliatedAp(rnr, 0);

    NS_TEST_EXPECT_MSG_EQ(ret.has_value(), true, "Expected to find a suitable reported AP");
    NS_TEST_EXPECT_MSG_EQ(ret->m_nbrApInfoId, 1, "Unexpected neighbor ID of the first reported AP");
    NS_TEST_EXPECT_MSG_EQ(ret->m_tbttInfoFieldId, 0, "Unexpected tbtt ID of the first reported AP");

    ret = WifiAssocManager::GetNextAffiliatedAp(rnr, ret->m_nbrApInfoId + 1);

    NS_TEST_EXPECT_MSG_EQ(ret.has_value(), true, "Expected to find a second suitable reported AP");
    NS_TEST_EXPECT_MSG_EQ(ret->m_nbrApInfoId,
                          3,
                          "Unexpected neighbor ID of the second reported AP");
    NS_TEST_EXPECT_MSG_EQ(ret->m_tbttInfoFieldId,
                          1,
                          "Unexpected tbtt ID of the second reported AP");

    ret = WifiAssocManager::GetNextAffiliatedAp(rnr, ret->m_nbrApInfoId + 1);

    NS_TEST_EXPECT_MSG_EQ(ret.has_value(),
                          false,
                          "Did not expect to find a third suitable reported AP");

    // check implementation of WifiAssocManager::GetAllAffiliatedAps()
    auto allAps = WifiAssocManager::GetAllAffiliatedAps(rnr);

    NS_TEST_EXPECT_MSG_EQ(allAps.size(), 2, "Expected to find two suitable reported APs");

    auto apIt = allAps.begin();
    NS_TEST_EXPECT_MSG_EQ(apIt->m_nbrApInfoId,
                          1,
                          "Unexpected neighbor ID of the first reported AP");
    NS_TEST_EXPECT_MSG_EQ(apIt->m_tbttInfoFieldId,
                          0,
                          "Unexpected tbtt ID of the first reported AP");

    apIt++;
    NS_TEST_EXPECT_MSG_EQ(apIt->m_nbrApInfoId,
                          3,
                          "Unexpected neighbor ID of the second reported AP");
    NS_TEST_EXPECT_MSG_EQ(apIt->m_tbttInfoFieldId,
                          1,
                          "Unexpected tbtt ID of the second reported AP");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test Multi-Link Discovery & Setup
 *
 * Three spectrum channels are created, one for each band (2.4 GHz, 5 GHz and 6 GHz).
 * Each PHY object is attached to the spectrum channel corresponding to the PHY band
 * in which it is operating.
 */
class MultiLinkSetupTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param staChannels the strings specifying the operating channels for the STA
     * \param apChannels the strings specifying the operating channels for the AP
     * \param setupLinks a list of links (STA link ID, AP link ID) that are expected to be setup
     * \param fixedPhyBands list of IDs of STA links that cannot switch PHY band
     */
    MultiLinkSetupTest(std::initializer_list<std::string> staChannels,
                       std::initializer_list<std::string> apChannels,
                       std::initializer_list<std::pair<uint8_t, uint8_t>> setupLinks,
                       std::initializer_list<uint8_t> fixedPhyBands = {});
    ~MultiLinkSetupTest() override;

  private:
    /**
     * Reset the given PHY helper, use the given strings to set the ChannelSettings
     * attribute of the PHY objects to create, and attach them to the given spectrum
     * channels appropriately.
     *
     * \param helper the given PHY helper
     * \param channels the strings specifying the operating channels to configure
     * \param channelMap the created spectrum channels
     */
    void SetChannels(SpectrumWifiPhyHelper& helper,
                     const std::vector<std::string>& channels,
                     const std::map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>>& channelMap);

    /**
     * \param str the given channel string
     * \return the PHY band specified in the given channel string
     */
    WifiPhyBand GetPhyBandFromChannelStr(const std::string& str);

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * \param linkId the ID of the link transmitting the PSDUs
     * \param context the context
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    void Transmit(uint8_t linkId,
                  std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW);
    /**
     * Check correctness of Multi-Link Setup procedure.
     */
    void CheckMlSetup();

    /**
     * Check that links that are not setup on the non-AP MLD are disabled.
     */
    void CheckDisabledLinks();

    /**
     * Check correctness of the given Beacon frame.
     *
     * \param mpdu the given Beacon frame
     * \param linkId the ID of the link on which the Beacon frame was transmitted
     */
    void CheckBeacon(Ptr<WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Check correctness of the given Association Request frame.
     *
     * \param mpdu the given Association Request frame
     * \param linkId the ID of the link on which the Association Request frame was transmitted
     */
    void CheckAssocRequest(Ptr<WifiMpdu> mpdu, uint8_t linkId);

    /**
     * Check correctness of the given Association Response frame.
     *
     * \param mpdu the given Association Response frame
     * \param linkId the ID of the link on which the Association Response frame was transmitted
     */
    void CheckAssocResponse(Ptr<WifiMpdu> mpdu, uint8_t linkId);

    void DoRun() override;

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
        uint8_t linkId;           ///< link ID
    };

    std::vector<FrameInfo> m_txPsdus;       ///< transmitted PSDUs
    std::vector<std::string> m_staChannels; ///< strings specifying channels for STA
    std::vector<std::string> m_apChannels;  ///< strings specifying channels for AP
    std::vector<std::pair<uint8_t, uint8_t>>
        m_setupLinks;                     ///< expected links to setup (STA link ID, AP link ID)
    std::vector<uint8_t> m_fixedPhyBands; ///< links on non-AP MLD with fixed PHY band
    Ptr<ApWifiMac> m_apMac;               ///< AP wifi MAC
    Ptr<StaWifiMac> m_staMac;             ///< STA wifi MAC
};

MultiLinkSetupTest::MultiLinkSetupTest(
    std::initializer_list<std::string> staChannels,
    std::initializer_list<std::string> apChannels,
    std::initializer_list<std::pair<uint8_t, uint8_t>> setupLinks,
    std::initializer_list<uint8_t> fixedPhyBands)
    : TestCase("Check correctness of Multi-Link Setup"),
      m_staChannels(staChannels),
      m_apChannels(apChannels),
      m_setupLinks(setupLinks),
      m_fixedPhyBands(fixedPhyBands)
{
}

MultiLinkSetupTest::~MultiLinkSetupTest()
{
}

void
MultiLinkSetupTest::Transmit(uint8_t linkId,
                             std::string context,
                             WifiConstPsduMap psduMap,
                             WifiTxVector txVector,
                             double txPowerW)
{
    m_txPsdus.push_back({Simulator::Now(), psduMap, txVector, linkId});

    std::stringstream ss;
    ss << std::setprecision(10) << "PSDU #" << m_txPsdus.size() << " Link ID " << +linkId << " "
       << psduMap.begin()->second->GetHeader(0).GetTypeString() << " #MPDUs "
       << psduMap.begin()->second->GetNMpdus() << " duration/ID "
       << psduMap.begin()->second->GetHeader(0).GetDuration()
       << " RA = " << psduMap.begin()->second->GetAddr1()
       << " TA = " << psduMap.begin()->second->GetAddr2();
    if (psduMap.begin()->second->GetHeader(0).IsQosData())
    {
        ss << " TID = " << +psduMap.begin()->second->GetHeader(0).GetQosTid();
    }
    NS_LOG_INFO(ss.str());
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");
}

void
MultiLinkSetupTest::SetChannels(
    SpectrumWifiPhyHelper& helper,
    const std::vector<std::string>& channels,
    const std::map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>>& channelMap)
{
    helper = SpectrumWifiPhyHelper(channels.size());
    helper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    uint8_t linkId = 0;
    for (const auto& str : channels)
    {
        helper.Set(linkId, "ChannelSettings", StringValue(str));
        helper.SetChannel(linkId, channelMap.at(GetPhyBandFromChannelStr(str)));

        linkId++;
    }
}

WifiPhyBand
MultiLinkSetupTest::GetPhyBandFromChannelStr(const std::string& str)
{
    if (str.find("2_4GHZ") != std::string::npos)
    {
        return WIFI_PHY_BAND_2_4GHZ;
    }
    if (str.find("5GHZ") != std::string::npos)
    {
        return WIFI_PHY_BAND_5GHZ;
    }
    if (str.find("6GHZ") != std::string::npos)
    {
        return WIFI_PHY_BAND_6GHZ;
    }
    NS_ABORT_MSG("Band in channel settings must be specified");
    return WIFI_PHY_BAND_UNSPECIFIED;
}

void
MultiLinkSetupTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);

    WifiHelper wifi;
    // wifi.EnableLogComponents ();
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"));

    std::map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>> channelMap = {
        {WIFI_PHY_BAND_2_4GHZ, CreateObject<MultiModelSpectrumChannel>()},
        {WIFI_PHY_BAND_5GHZ, CreateObject<MultiModelSpectrumChannel>()},
        {WIFI_PHY_BAND_6GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    SpectrumWifiPhyHelper staPhyHelper;
    SpectrumWifiPhyHelper apPhyHelper;
    SetChannels(staPhyHelper, m_staChannels, channelMap);
    SetChannels(apPhyHelper, m_apChannels, channelMap);

    for (const auto& linkId : m_fixedPhyBands)
    {
        staPhyHelper.Set(linkId, "FixedPhyBand", BooleanValue(true));
    }

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices = wifi.Install(staPhyHelper, mac, wifiStaNode);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));

    NetDeviceContainer apDevices = wifi.Install(apPhyHelper, mac, wifiApNode);

    // Uncomment the lines below to write PCAP files
    // apPhyHelper.EnablePcap("wifi-mlo_AP", apDevices);
    // staPhyHelper.EnablePcap("wifi-mlo_STA", staDevices);

    // Assign fixed streams to random variables in use
    streamNumber += wifi.AssignStreams(apDevices, streamNumber);
    streamNumber += wifi.AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetMac());
    m_staMac = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDevices.Get(0))->GetMac());

    // Trace PSDUs passed to the PHY on all devices
    for (uint8_t linkId = 0; linkId < StaticCast<WifiNetDevice>(apDevices.Get(0))->GetNPhys();
         linkId++)
    {
        Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                            std::to_string(linkId) + "/PhyTxPsduBegin",
                        MakeCallback(&MultiLinkSetupTest::Transmit, this).Bind(linkId));
    }
    for (uint8_t linkId = 0; linkId < StaticCast<WifiNetDevice>(staDevices.Get(0))->GetNPhys();
         linkId++)
    {
        Config::Connect("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                            std::to_string(linkId) + "/PhyTxPsduBegin",
                        MakeCallback(&MultiLinkSetupTest::Transmit, this).Bind(linkId));
    }

    Simulator::Schedule(MilliSeconds(500), &MultiLinkSetupTest::CheckMlSetup, this);

    Simulator::Stop(Seconds(1.5));
    Simulator::Run();

    /**
     * Check content of management frames
     */
    for (const auto& frameInfo : m_txPsdus)
    {
        const auto& mpdu = *frameInfo.psduMap.begin()->second->begin();
        const auto& linkId = frameInfo.linkId;

        switch (mpdu->GetHeader().GetType())
        {
        case WIFI_MAC_MGT_BEACON:
            CheckBeacon(mpdu, linkId);
            break;

        case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
            CheckAssocRequest(mpdu, linkId);
            break;

        case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
            CheckAssocResponse(mpdu, linkId);
            break;

        default:
            break;
        }
    }

    CheckDisabledLinks();

    Simulator::Destroy();
}

void
MultiLinkSetupTest::CheckBeacon(Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
    NS_ABORT_IF(mpdu->GetHeader().GetType() != WIFI_MAC_MGT_BEACON);

    NS_TEST_EXPECT_MSG_EQ(m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                          mpdu->GetHeader().GetAddr2(),
                          "TA of Beacon frame is not the address of the link it is transmitted on");
    MgtBeaconHeader beacon;
    mpdu->GetPacket()->PeekHeader(beacon);
    const auto& rnr = beacon.GetReducedNeighborReport();
    const auto& mle = beacon.GetMultiLinkElement();

    if (m_apMac->GetNLinks() == 1)
    {
        NS_TEST_EXPECT_MSG_EQ(rnr.has_value(),
                              false,
                              "RNR Element in Beacon frame from single link AP");
        NS_TEST_EXPECT_MSG_EQ(mle.has_value(),
                              false,
                              "Multi-Link Element in Beacon frame from single link AP");
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(rnr.has_value(), true, "No RNR Element in Beacon frame");
    // All the other APs affiliated with the same AP MLD as the AP sending
    // the Beacon frame must be reported in a separate Neighbor AP Info field
    NS_TEST_EXPECT_MSG_EQ(rnr->GetNNbrApInfoFields(),
                          static_cast<std::size_t>(m_apMac->GetNLinks() - 1),
                          "Unexpected number of Neighbor AP Info fields in RNR");
    for (std::size_t nbrApInfoId = 0; nbrApInfoId < rnr->GetNNbrApInfoFields(); nbrApInfoId++)
    {
        NS_TEST_EXPECT_MSG_EQ(rnr->HasMldParameters(nbrApInfoId),
                              true,
                              "MLD Parameters not present");
        NS_TEST_EXPECT_MSG_EQ(rnr->GetNTbttInformationFields(nbrApInfoId),
                              1,
                              "Expected only one TBTT Info subfield per Neighbor AP Info");
        uint8_t nbrLinkId = rnr->GetLinkId(nbrApInfoId, 0);
        NS_TEST_EXPECT_MSG_EQ(rnr->GetBssid(nbrApInfoId, 0),
                              m_apMac->GetFrameExchangeManager(nbrLinkId)->GetAddress(),
                              "BSSID advertised in Neighbor AP Info field "
                                  << nbrApInfoId
                                  << " does not match the address configured on the link "
                                     "advertised in the same field");
    }

    NS_TEST_EXPECT_MSG_EQ(mle.has_value(), true, "No Multi-Link Element in Beacon frame");
    NS_TEST_EXPECT_MSG_EQ(mle->GetMldMacAddress(),
                          m_apMac->GetAddress(),
                          "Incorrect MLD address advertised in Multi-Link Element");
    NS_TEST_EXPECT_MSG_EQ(mle->GetLinkIdInfo(),
                          +linkId,
                          "Incorrect Link ID advertised in Multi-Link Element");
}

void
MultiLinkSetupTest::CheckAssocRequest(Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
    NS_ABORT_IF(mpdu->GetHeader().GetType() != WIFI_MAC_MGT_ASSOCIATION_REQUEST);

    NS_TEST_EXPECT_MSG_EQ(
        m_staMac->GetFrameExchangeManager(linkId)->GetAddress(),
        mpdu->GetHeader().GetAddr2(),
        "TA of Assoc Request frame is not the address of the link it is transmitted on");
    MgtAssocRequestHeader assoc;
    mpdu->GetPacket()->PeekHeader(assoc);
    const auto& mle = assoc.GetMultiLinkElement();

    if (m_apMac->GetNLinks() == 1 || m_staMac->GetNLinks() == 1)
    {
        NS_TEST_EXPECT_MSG_EQ(mle.has_value(),
                              false,
                              "Multi-Link Element in Assoc Request frame from single link STA");
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(mle.has_value(), true, "No Multi-Link Element in Assoc Request frame");
    NS_TEST_EXPECT_MSG_EQ(mle->GetMldMacAddress(),
                          m_staMac->GetAddress(),
                          "Incorrect MLD Address advertised in Multi-Link Element");
    NS_TEST_EXPECT_MSG_EQ(mle->GetNPerStaProfileSubelements(),
                          m_setupLinks.size() - 1,
                          "Incorrect number of Per-STA Profile subelements in Multi-Link Element");
    for (std::size_t i = 0; i < mle->GetNPerStaProfileSubelements(); i++)
    {
        auto& perStaProfile = mle->GetPerStaProfile(i);
        NS_TEST_EXPECT_MSG_EQ(perStaProfile.HasStaMacAddress(),
                              true,
                              "Per-STA Profile must contain STA MAC address");
        // find ID of the local link corresponding to this subelement
        auto staLinkId = m_staMac->GetLinkIdByAddress(perStaProfile.GetStaMacAddress());
        NS_TEST_EXPECT_MSG_EQ(
            staLinkId.has_value(),
            true,
            "No link found with the STA MAC address advertised in Per-STA Profile");
        NS_TEST_EXPECT_MSG_NE(
            +staLinkId.value(),
            +linkId,
            "The STA that sent the Assoc Request should not be included in a Per-STA Profile");
        auto it = std::find_if(m_setupLinks.begin(), m_setupLinks.end(), [&staLinkId](auto&& pair) {
            return pair.first == staLinkId.value();
        });
        NS_TEST_EXPECT_MSG_EQ((it != m_setupLinks.end()),
                              true,
                              "Not expecting to setup STA link ID " << +staLinkId.value());
        NS_TEST_EXPECT_MSG_EQ(
            +it->second,
            +perStaProfile.GetLinkId(),
            "Not expecting to request association to AP Link ID in Per-STA Profile");
        NS_TEST_EXPECT_MSG_EQ(perStaProfile.HasAssocRequest(),
                              true,
                              "Missing Association Request in Per-STA Profile");
    }
}

void
MultiLinkSetupTest::CheckAssocResponse(Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
    NS_ABORT_IF(mpdu->GetHeader().GetType() != WIFI_MAC_MGT_ASSOCIATION_RESPONSE);

    NS_TEST_EXPECT_MSG_EQ(
        m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
        mpdu->GetHeader().GetAddr2(),
        "TA of Assoc Response frame is not the address of the link it is transmitted on");
    MgtAssocResponseHeader assoc;
    mpdu->GetPacket()->PeekHeader(assoc);
    const auto& mle = assoc.GetMultiLinkElement();

    if (m_apMac->GetNLinks() == 1 || m_staMac->GetNLinks() == 1)
    {
        NS_TEST_EXPECT_MSG_EQ(
            mle.has_value(),
            false,
            "Multi-Link Element in Assoc Response frame with single link AP or single link STA");
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(mle.has_value(), true, "No Multi-Link Element in Assoc Request frame");
    NS_TEST_EXPECT_MSG_EQ(mle->GetMldMacAddress(),
                          m_apMac->GetAddress(),
                          "Incorrect MLD Address advertised in Multi-Link Element");
    NS_TEST_EXPECT_MSG_EQ(mle->GetNPerStaProfileSubelements(),
                          m_setupLinks.size() - 1,
                          "Incorrect number of Per-STA Profile subelements in Multi-Link Element");
    for (std::size_t i = 0; i < mle->GetNPerStaProfileSubelements(); i++)
    {
        auto& perStaProfile = mle->GetPerStaProfile(i);
        NS_TEST_EXPECT_MSG_EQ(perStaProfile.HasStaMacAddress(),
                              true,
                              "Per-STA Profile must contain STA MAC address");
        // find ID of the local link corresponding to this subelement
        auto apLinkId = m_apMac->GetLinkIdByAddress(perStaProfile.GetStaMacAddress());
        NS_TEST_EXPECT_MSG_EQ(
            apLinkId.has_value(),
            true,
            "No link found with the STA MAC address advertised in Per-STA Profile");
        NS_TEST_EXPECT_MSG_EQ(+apLinkId.value(),
                              +perStaProfile.GetLinkId(),
                              "Link ID and MAC address advertised in Per-STA Profile do not match");
        NS_TEST_EXPECT_MSG_NE(
            +apLinkId.value(),
            +linkId,
            "The AP that sent the Assoc Response should not be included in a Per-STA Profile");
        auto it = std::find_if(m_setupLinks.begin(), m_setupLinks.end(), [&apLinkId](auto&& pair) {
            return pair.second == apLinkId.value();
        });
        NS_TEST_EXPECT_MSG_EQ((it != m_setupLinks.end()),
                              true,
                              "Not expecting to setup AP link ID " << +apLinkId.value());
        NS_TEST_EXPECT_MSG_EQ(perStaProfile.HasAssocResponse(),
                              true,
                              "Missing Association Response in Per-STA Profile");
    }
}

void
MultiLinkSetupTest::CheckMlSetup()
{
    /**
     * Check outcome of Multi-Link Setup
     */
    NS_TEST_EXPECT_MSG_EQ(m_staMac->IsAssociated(), true, "Expected the STA to be associated");

    for (const auto& [staLinkId, apLinkId] : m_setupLinks)
    {
        auto staAddr = m_staMac->GetFrameExchangeManager(staLinkId)->GetAddress();
        auto apAddr = m_apMac->GetFrameExchangeManager(apLinkId)->GetAddress();

        auto staRemoteMgr = m_staMac->GetWifiRemoteStationManager(staLinkId);
        auto apRemoteMgr = m_apMac->GetWifiRemoteStationManager(apLinkId);

        // STA side
        NS_TEST_EXPECT_MSG_EQ(m_staMac->GetFrameExchangeManager(staLinkId)->GetBssid(),
                              apAddr,
                              "Unexpected BSSID for STA link ID " << +staLinkId);
        if (m_apMac->GetNLinks() > 1 && m_staMac->GetNLinks() > 1)
        {
            NS_TEST_EXPECT_MSG_EQ((staRemoteMgr->GetMldAddress(apAddr) == m_apMac->GetAddress()),
                                  true,
                                  "Incorrect MLD address stored by STA on link ID " << +staLinkId);
            NS_TEST_EXPECT_MSG_EQ(
                (staRemoteMgr->GetAffiliatedStaAddress(m_apMac->GetAddress()) == apAddr),
                true,
                "Incorrect affiliated address stored by STA on link ID " << +staLinkId);
        }

        // AP side
        NS_TEST_EXPECT_MSG_EQ(apRemoteMgr->IsAssociated(staAddr),
                              true,
                              "Expecting STA " << staAddr << " to be associated on link "
                                               << +apLinkId);
        if (m_apMac->GetNLinks() > 1 && m_staMac->GetNLinks() > 1)
        {
            NS_TEST_EXPECT_MSG_EQ((apRemoteMgr->GetMldAddress(staAddr) == m_staMac->GetAddress()),
                                  true,
                                  "Incorrect MLD address stored by AP on link ID " << +apLinkId);
            NS_TEST_EXPECT_MSG_EQ(
                (apRemoteMgr->GetAffiliatedStaAddress(m_staMac->GetAddress()) == staAddr),
                true,
                "Incorrect affiliated address stored by AP on link ID " << +apLinkId);
        }
        auto aid = m_apMac->GetAssociationId(staAddr, apLinkId);
        const auto& staList = m_apMac->GetStaList(apLinkId);
        NS_TEST_EXPECT_MSG_EQ((staList.find(aid) != staList.end()),
                              true,
                              "STA " << staAddr << " not found in list of associated STAs");

        // STA of non-AP MLD operate on the same channel as the AP
        NS_TEST_EXPECT_MSG_EQ(+m_staMac->GetWifiPhy(staLinkId)->GetOperatingChannel().GetNumber(),
                              +m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetNumber(),
                              "Incorrect operating channel number for STA on link " << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(m_staMac->GetWifiPhy(staLinkId)->GetOperatingChannel().GetFrequency(),
                              m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetFrequency(),
                              "Incorrect operating channel frequency for STA on link "
                                  << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(m_staMac->GetWifiPhy(staLinkId)->GetOperatingChannel().GetWidth(),
                              m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetWidth(),
                              "Incorrect operating channel width for STA on link " << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(+m_staMac->GetWifiPhy(staLinkId)->GetOperatingChannel().GetPhyBand(),
                              +m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetPhyBand(),
                              "Incorrect operating PHY band for STA on link " << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(
            +m_staMac->GetWifiPhy(staLinkId)->GetOperatingChannel().GetPrimaryChannelIndex(20),
            +m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetPrimaryChannelIndex(20),
            "Incorrect operating primary channel index for STA on link " << +staLinkId);
    }
}

void
MultiLinkSetupTest::CheckDisabledLinks()
{
    for (std::size_t linkId = 0; linkId < m_staChannels.size(); linkId++)
    {
        auto it = std::find_if(m_setupLinks.begin(), m_setupLinks.end(), [&linkId](auto&& link) {
            return link.first == linkId;
        });
        if (it == m_setupLinks.end())
        {
            // the link has not been setup
            NS_TEST_EXPECT_MSG_EQ(m_staMac->GetWifiPhy(linkId)->GetState()->IsStateOff(),
                                  true,
                                  "Link " << +linkId << " has not been setup but is not disabled");
            continue;
        }

        if (GetPhyBandFromChannelStr(m_staChannels.at(it->first)) !=
            GetPhyBandFromChannelStr(m_apChannels.at(it->second)))
        {
            // STA had to switch PHY band to match AP's operating band. Given that we
            // are using three distinct spectrum channels (one per band), a STA will
            // not receive Beacon frames after switching PHY band. After a number of
            // missed Beacon frames, the link is disabled
            NS_TEST_EXPECT_MSG_EQ(m_staMac->GetWifiPhy(linkId)->GetState()->IsStateOff(),
                                  true,
                                  "Expecting link " << +linkId
                                                    << " to be disabled due to switching PHY band");
            continue;
        }

        // the link has been setup and must be active
        NS_TEST_EXPECT_MSG_EQ(m_staMac->GetWifiPhy(linkId)->GetState()->IsStateOff(),
                              false,
                              "Expecting link " << +linkId << " to be active");
    }
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi 11be MLD Test Suite
 */
class WifiMultiLinkOperationsTestSuite : public TestSuite
{
  public:
    WifiMultiLinkOperationsTestSuite();
};

WifiMultiLinkOperationsTestSuite::WifiMultiLinkOperationsTestSuite()
    : TestSuite("wifi-mlo", UNIT)
{
    AddTestCase(new GetRnrLinkInfoTest(), TestCase::QUICK);
    // matching channels: setup all links
    AddTestCase(new MultiLinkSetupTest(
                    {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                    {{0, 0}, {1, 1}, {2, 2}}),
                TestCase::QUICK);
    // non-matching channels, matching PHY bands: setup all links
    AddTestCase(new MultiLinkSetupTest(
                    {"{108, 0, BAND_5GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{120, 0, BAND_5GHZ, 0}", "{5, 0, BAND_6GHZ, 0}"},
                    {{1, 0}, {0, 1}, {2, 2}}),
                TestCase::QUICK);
    // non-AP MLD switches band on some links to setup 3 links
    AddTestCase(new MultiLinkSetupTest(
                    {"{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{9, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                    {{2, 0}, {0, 1}, {1, 2}}),
                TestCase::QUICK);
    // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
    // that band, hence only 2 links are setup
    AddTestCase(new MultiLinkSetupTest(
                    {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{8, 20, BAND_2_4GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                    {{1, 0}, {2, 1}},
                    {0}),
                TestCase::QUICK);
    // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
    // that band; the second link of the non-AP MLD cannot change PHY band and there is
    // an AP operating on the same channel; hence 2 links are setup
    AddTestCase(new MultiLinkSetupTest(
                    {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{8, 20, BAND_2_4GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                    {{1, 0}, {2, 1}},
                    {0, 1}),
                TestCase::QUICK);
    // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
    // that band; the second link of the non-AP MLD cannot change PHY band and there is
    // an AP operating on the same channel; the third link of the non-AP MLD cannot
    // change PHY band and there is an AP operating on the same band (different channel);
    // hence 2 links are setup by switching channel (not band) on the third link
    AddTestCase(new MultiLinkSetupTest(
                    {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{60, 0, BAND_5GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                    {{1, 0}, {2, 2}},
                    {0, 1, 2}),
                TestCase::QUICK);
    // non-AP MLD has only two STAs and setups two links
    AddTestCase(new MultiLinkSetupTest(
                    {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                    {{0, 1}, {1, 0}}),
                TestCase::QUICK);
    // single link non-AP STA associates with an AP affiliated with an AP MLD
    AddTestCase(new MultiLinkSetupTest(
                    {"{120, 0, BAND_5GHZ, 0}"},
                    {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                    {{0, 2}}),
                TestCase::QUICK);
    // a STA affiliated with a non-AP MLD associates with a single link AP
    AddTestCase(new MultiLinkSetupTest(
                    {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                    {"{120, 0, BAND_5GHZ, 0}"},
                    {{2, 0}}),
                TestCase::QUICK);
}

static WifiMultiLinkOperationsTestSuite g_wifiMultiLinkOperationsTestSuite; ///< the test suite
