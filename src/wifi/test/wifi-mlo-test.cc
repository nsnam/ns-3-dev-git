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
#include "ns3/node-list.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/qos-utils.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <array>
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
 * \brief Base class for Multi-Link Operations tests
 *
 * Three spectrum channels are created, one for each band (2.4 GHz, 5 GHz and 6 GHz).
 * Each PHY object is attached to the spectrum channel corresponding to the PHY band
 * in which it is operating.
 */
class MultiLinkOperationsTestBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param name The name of the new TestCase created
     * \param nStations the number of stations to create
     * \param staChannels the strings specifying the operating channels for the STA
     * \param apChannels the strings specifying the operating channels for the AP
     * \param fixedPhyBands list of IDs of STA links that cannot switch PHY band
     */
    MultiLinkOperationsTestBase(const std::string& name,
                                uint8_t nStations,
                                std::vector<std::string> staChannels,
                                std::vector<std::string> apChannels,
                                std::vector<uint8_t> fixedPhyBands = {});
    ~MultiLinkOperationsTestBase() override = default;

  protected:
    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * \param linkId the ID of the link transmitting the PSDUs
     * \param context the context
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    virtual void Transmit(uint8_t linkId,
                          std::string context,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW);

    void DoSetup() override;

    /**
     * Check that the Address 1 and Address 2 fields of the given PSDU contain device MAC addresses.
     *
     * \param psdu the given PSDU
     * \param downlink true for downlink frames, false for uplink frames
     */
    void CheckAddresses(Ptr<const WifiPsdu> psdu, bool downlink);

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
        uint8_t linkId;           ///< link ID
    };

    std::vector<FrameInfo> m_txPsdus;             ///< transmitted PSDUs
    const std::vector<std::string> m_staChannels; ///< strings specifying channels for STA
    const std::vector<std::string> m_apChannels;  ///< strings specifying channels for AP
    const std::vector<uint8_t> m_fixedPhyBands;   ///< links on non-AP MLD with fixed PHY band
    Ptr<ApWifiMac> m_apMac;                       ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;       ///< STA wifi MACs
    uint8_t m_nStations;                          ///< number of stations to create
    uint16_t m_lastAid;                           ///< AID of last associated station

  private:
    /**
     * Reset the given PHY helper, use the given strings to set the ChannelSettings
     * attribute of the PHY objects to create, and attach them to the given spectrum
     * channels appropriately.
     *
     * \param helper the given PHY helper
     * \param channels the strings specifying the operating channels to configure
     * \param channel the created spectrum channel
     */
    void SetChannels(SpectrumWifiPhyHelper& helper,
                     const std::vector<std::string>& channels,
                     Ptr<MultiModelSpectrumChannel> channel);

    /**
     * Set the SSID on the next station that needs to start the association procedure.
     * This method is connected to the ApWifiMac's AssociatedSta trace source.
     * Start generating traffic (if needed) when all stations are associated.
     *
     * \param aid the AID assigned to the previous associated STA
     */
    void SetSsid(uint16_t aid, Mac48Address /* addr */);

    /**
     * Start the generation of traffic (needs to be overridden)
     */
    virtual void StartTraffic()
    {
    }
};

MultiLinkOperationsTestBase::MultiLinkOperationsTestBase(const std::string& name,
                                                         uint8_t nStations,
                                                         std::vector<std::string> staChannels,
                                                         std::vector<std::string> apChannels,
                                                         std::vector<uint8_t> fixedPhyBands)
    : TestCase(name),
      m_staChannels(staChannels),
      m_apChannels(apChannels),
      m_fixedPhyBands(fixedPhyBands),
      m_staMacs(nStations),
      m_nStations(nStations),
      m_lastAid(0)
{
}

void
MultiLinkOperationsTestBase::CheckAddresses(Ptr<const WifiPsdu> psdu, bool downlink)
{
    std::optional<Mac48Address> apAddr;
    std::optional<Mac48Address> staAddr;

    if (downlink)
    {
        if (!psdu->GetAddr1().IsGroup())
        {
            staAddr = psdu->GetAddr1();
        }
        apAddr = psdu->GetAddr2();
    }
    else
    {
        if (!psdu->GetAddr1().IsGroup())
        {
            apAddr = psdu->GetAddr1();
        }
        staAddr = psdu->GetAddr2();
    }

    if (apAddr)
    {
        bool found = false;
        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            if (m_apMac->GetFrameExchangeManager(linkId)->GetAddress() == *apAddr)
            {
                found = true;
                break;
            }
        }
        NS_TEST_EXPECT_MSG_EQ(found,
                              true,
                              "Address " << *apAddr << " is not an AP device address. "
                                         << "PSDU: " << *psdu);
    }

    if (staAddr)
    {
        bool found = false;
        for (uint8_t i = 0; i < m_nStations; i++)
        {
            for (uint8_t linkId = 0; linkId < m_staMacs[i]->GetNLinks(); linkId++)
            {
                if (m_staMacs[i]->GetFrameExchangeManager(linkId)->GetAddress() == *staAddr)
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }
        NS_TEST_EXPECT_MSG_EQ(found,
                              true,
                              "Address " << *staAddr << " is not a STA device address. "
                                         << "PSDU: " << *psdu);
    }
}

void
MultiLinkOperationsTestBase::Transmit(uint8_t linkId,
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
       << " TA = " << psduMap.begin()->second->GetAddr2()
       << " ADDR3 = " << psduMap.begin()->second->GetHeader(0).GetAddr3();
    if (psduMap.begin()->second->GetHeader(0).IsQosData())
    {
        ss << " seqNo = {";
        for (auto& mpdu : *PeekPointer(psduMap.begin()->second))
        {
            ss << mpdu->GetHeader().GetSequenceNumber();
        }
        ss << "} TID = " << +psduMap.begin()->second->GetHeader(0).GetQosTid();
    }
    NS_LOG_INFO(ss.str());
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");
}

void
MultiLinkOperationsTestBase::SetChannels(SpectrumWifiPhyHelper& helper,
                                         const std::vector<std::string>& channels,
                                         Ptr<MultiModelSpectrumChannel> channel)
{
    helper = SpectrumWifiPhyHelper(channels.size());
    helper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    uint8_t linkId = 0;
    for (const auto& str : channels)
    {
        helper.Set(linkId++, "ChannelSettings", StringValue(str));
    }

    helper.SetChannel(channel);
}

void
MultiLinkOperationsTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(m_nStations);

    WifiHelper wifi;
    // wifi.EnableLogComponents ();
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"));

    auto channel = CreateObject<MultiModelSpectrumChannel>();

    SpectrumWifiPhyHelper staPhyHelper;
    SpectrumWifiPhyHelper apPhyHelper;
    SetChannels(staPhyHelper, m_staChannels, channel);
    SetChannels(apPhyHelper, m_apChannels, channel);

    for (const auto& linkId : m_fixedPhyBands)
    {
        staPhyHelper.Set(linkId, "FixedPhyBand", BooleanValue(true));
    }

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac", // default SSID
                "ActiveProbing",
                BooleanValue(false));

    NetDeviceContainer staDevices = wifi.Install(staPhyHelper, mac, wifiStaNodes);

    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

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
    mobility.Install(wifiStaNodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevices.Get(0))->GetMac());
    for (uint8_t i = 0; i < m_nStations; i++)
    {
        m_staMacs[i] =
            DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDevices.Get(i))->GetMac());
    }

    // Trace PSDUs passed to the PHY on all devices
    for (uint8_t linkId = 0; linkId < StaticCast<WifiNetDevice>(apDevices.Get(0))->GetNPhys();
         linkId++)
    {
        Config::Connect("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                            std::to_string(linkId) + "/PhyTxPsduBegin",
                        MakeCallback(&MultiLinkOperationsTestBase::Transmit, this).Bind(linkId));
    }
    for (uint8_t i = 0; i < m_nStations; i++)
    {
        for (uint8_t linkId = 0; linkId < StaticCast<WifiNetDevice>(staDevices.Get(i))->GetNPhys();
             linkId++)
        {
            Config::Connect(
                "/NodeList/" + std::to_string(i + 1) + "/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                    std::to_string(linkId) + "/PhyTxPsduBegin",
                MakeCallback(&MultiLinkOperationsTestBase::Transmit, this).Bind(linkId));
        }
    }

    // schedule ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext("AssociatedSta",
                                        MakeCallback(&MultiLinkOperationsTestBase::SetSsid, this));
    Simulator::Schedule(Seconds(0), [&]() { m_staMacs[0]->SetSsid(Ssid("ns-3-ssid")); });
}

void
MultiLinkOperationsTestBase::SetSsid(uint16_t aid, Mac48Address /* addr */)
{
    if (m_lastAid == aid)
    {
        // another STA of this non-AP MLD has already fired this callback
        return;
    }
    m_lastAid = aid;

    // make the next STA to start ML discovery & setup
    if (aid < m_nStations)
    {
        m_staMacs[aid]->SetSsid(Ssid("ns-3-ssid"));
        return;
    }
    // wait some time (5ms) to allow the completion of association before generating traffic
    Simulator::Schedule(MilliSeconds(5), &MultiLinkOperationsTestBase::StartTraffic, this);
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Multi-Link Discovery & Setup test.
 *
 * This test sets up an AP MLD and a non-AP MLD having a variable number of links.
 * The RF channels to set each link to are provided as input parameters through the test
 * case constructor, along with the identifiers (starting at 0) of the links that cannot
 * switch PHY band (if any). The links that are expected to be setup are also provided as input
 * parameters. This test verifies that the management frames exchanged during ML discovery
 * and ML setup contain the expected values and that the two MLDs setup the expected links.
 */
class MultiLinkSetupTest : public MultiLinkOperationsTestBase
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
    MultiLinkSetupTest(std::vector<std::string> staChannels,
                       std::vector<std::string> apChannels,
                       std::vector<std::pair<uint8_t, uint8_t>> setupLinks,
                       std::vector<uint8_t> fixedPhyBands = {});
    ~MultiLinkSetupTest() override = default;

  protected:
    void DoRun() override;

  private:
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

    /// expected links to setup (STA link ID, AP link ID)
    const std::vector<std::pair<uint8_t, uint8_t>> m_setupLinks;
};

MultiLinkSetupTest::MultiLinkSetupTest(std::vector<std::string> staChannels,
                                       std::vector<std::string> apChannels,
                                       std::vector<std::pair<uint8_t, uint8_t>> setupLinks,
                                       std::vector<uint8_t> fixedPhyBands)
    : MultiLinkOperationsTestBase("Check correctness of Multi-Link Setup",
                                  1,
                                  staChannels,
                                  apChannels,
                                  fixedPhyBands),
      m_setupLinks(setupLinks)
{
}

void
MultiLinkSetupTest::DoRun()
{
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

    CheckAddresses(Create<WifiPsdu>(mpdu, false), true);

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

    CheckAddresses(Create<WifiPsdu>(mpdu, false), false);

    NS_TEST_EXPECT_MSG_EQ(
        m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress(),
        mpdu->GetHeader().GetAddr2(),
        "TA of Assoc Request frame is not the address of the link it is transmitted on");
    MgtAssocRequestHeader assoc;
    mpdu->GetPacket()->PeekHeader(assoc);
    const auto& mle = assoc.GetMultiLinkElement();

    if (m_apMac->GetNLinks() == 1 || m_staMacs[0]->GetNLinks() == 1)
    {
        NS_TEST_EXPECT_MSG_EQ(mle.has_value(),
                              false,
                              "Multi-Link Element in Assoc Request frame from single link STA");
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(mle.has_value(), true, "No Multi-Link Element in Assoc Request frame");
    NS_TEST_EXPECT_MSG_EQ(mle->GetMldMacAddress(),
                          m_staMacs[0]->GetAddress(),
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
        auto staLinkId = m_staMacs[0]->GetLinkIdByAddress(perStaProfile.GetStaMacAddress());
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

    CheckAddresses(Create<WifiPsdu>(mpdu, false), true);

    NS_TEST_EXPECT_MSG_EQ(
        m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
        mpdu->GetHeader().GetAddr2(),
        "TA of Assoc Response frame is not the address of the link it is transmitted on");
    MgtAssocResponseHeader assoc;
    mpdu->GetPacket()->PeekHeader(assoc);
    const auto& mle = assoc.GetMultiLinkElement();

    if (m_apMac->GetNLinks() == 1 || m_staMacs[0]->GetNLinks() == 1)
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
    NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->IsAssociated(), true, "Expected the STA to be associated");

    for (const auto& [staLinkId, apLinkId] : m_setupLinks)
    {
        auto staAddr = m_staMacs[0]->GetFrameExchangeManager(staLinkId)->GetAddress();
        auto apAddr = m_apMac->GetFrameExchangeManager(apLinkId)->GetAddress();

        auto staRemoteMgr = m_staMacs[0]->GetWifiRemoteStationManager(staLinkId);
        auto apRemoteMgr = m_apMac->GetWifiRemoteStationManager(apLinkId);

        // STA side
        NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetFrameExchangeManager(staLinkId)->GetBssid(),
                              apAddr,
                              "Unexpected BSSID for STA link ID " << +staLinkId);
        if (m_apMac->GetNLinks() > 1 && m_staMacs[0]->GetNLinks() > 1)
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
        if (m_apMac->GetNLinks() > 1 && m_staMacs[0]->GetNLinks() > 1)
        {
            NS_TEST_EXPECT_MSG_EQ(
                (apRemoteMgr->GetMldAddress(staAddr) == m_staMacs[0]->GetAddress()),
                true,
                "Incorrect MLD address stored by AP on link ID " << +apLinkId);
            NS_TEST_EXPECT_MSG_EQ(
                (apRemoteMgr->GetAffiliatedStaAddress(m_staMacs[0]->GetAddress()) == staAddr),
                true,
                "Incorrect affiliated address stored by AP on link ID " << +apLinkId);
        }
        auto aid = m_apMac->GetAssociationId(staAddr, apLinkId);
        const auto& staList = m_apMac->GetStaList(apLinkId);
        NS_TEST_EXPECT_MSG_EQ((staList.find(aid) != staList.end()),
                              true,
                              "STA " << staAddr << " not found in list of associated STAs");

        // STA of non-AP MLD operate on the same channel as the AP
        NS_TEST_EXPECT_MSG_EQ(
            +m_staMacs[0]->GetWifiPhy(staLinkId)->GetOperatingChannel().GetNumber(),
            +m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetNumber(),
            "Incorrect operating channel number for STA on link " << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(
            m_staMacs[0]->GetWifiPhy(staLinkId)->GetOperatingChannel().GetFrequency(),
            m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetFrequency(),
            "Incorrect operating channel frequency for STA on link " << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetWifiPhy(staLinkId)->GetOperatingChannel().GetWidth(),
                              m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetWidth(),
                              "Incorrect operating channel width for STA on link " << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(
            +m_staMacs[0]->GetWifiPhy(staLinkId)->GetOperatingChannel().GetPhyBand(),
            +m_apMac->GetWifiPhy(apLinkId)->GetOperatingChannel().GetPhyBand(),
            "Incorrect operating PHY band for STA on link " << +staLinkId);
        NS_TEST_EXPECT_MSG_EQ(
            +m_staMacs[0]->GetWifiPhy(staLinkId)->GetOperatingChannel().GetPrimaryChannelIndex(20),
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
            NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetWifiPhy(linkId)->GetState()->IsStateOff(),
                                  true,
                                  "Link " << +linkId << " has not been setup but is not disabled");
            continue;
        }

        // the link has been setup and must be active
        NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->GetWifiPhy(linkId)->GetState()->IsStateOff(),
                              false,
                              "Expecting link " << +linkId << " to be active");
    }
}

/**
 * Tested traffic patterns.
 */
enum class TrafficPattern : uint8_t
{
    STA_TO_STA = 0,
    STA_TO_AP,
    AP_TO_STA,
    AP_TO_BCAST,
    STA_TO_BCAST
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test data transmission between two MLDs when Block Ack agreement is not established.
 *
 * This test sets up an AP MLD and two non-AP MLDs having a variable number of links.
 * The RF channels to set each link to are provided as input parameters through the test
 * case constructor, along with the identifiers (starting at 0) of the links that cannot
 * switch PHY band (if any). This test aims at veryfing the successful transmission of both
 * unicast QoS data frames (from one station to another, from one station to the AP, from
 * the AP to the station) and broadcast QoS data frames (from the AP or from one station).
 * Two QoS data frames are generated for this purpose. The second one is also corrupted once
 * (by using a post reception error model) to test its successful re-transmission.
 * Establishment of a BA agreement is prevented by corrupting all ADDBA_REQUEST frames
 * with a post reception error model.
 */
class MultiLinkTxNoBaTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * \param trafficPattern the pattern of traffic to generate
     * \param staChannels the strings specifying the operating channels for the STA
     * \param apChannels the strings specifying the operating channels for the AP
     * \param fixedPhyBands list of IDs of STA links that cannot switch PHY band
     */
    MultiLinkTxNoBaTest(TrafficPattern trafficPattern,
                        std::vector<std::string> staChannels,
                        std::vector<std::string> apChannels,
                        std::vector<uint8_t> fixedPhyBands = {});
    ~MultiLinkTxNoBaTest() override = default;

  protected:
    /**
     * Function to trace packets received by the server application
     * \param nodeId the ID of the node that received the packet
     * \param p the packet
     * \param addr the address
     */
    void L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr);

    void Transmit(uint8_t linkId,
                  std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    Ptr<ListErrorModel> m_errorModel;      ///< error rate model to corrupt packets
    std::list<uint64_t> m_uidList;         ///< list of UIDs of packets to corrupt
    bool m_dataCorrupted{false};           ///< whether second data frame has been already corrupted
    TrafficPattern m_trafficPattern;       ///< the pattern of traffic to generate
    std::array<std::size_t, 3> m_rxPkts{}; ///< number of packets received at application layer
                                           ///< by each node (AP, STA 0, STA 1)
};

MultiLinkTxNoBaTest::MultiLinkTxNoBaTest(TrafficPattern trafficPattern,
                                         std::vector<std::string> staChannels,
                                         std::vector<std::string> apChannels,
                                         std::vector<uint8_t> fixedPhyBands)
    : MultiLinkOperationsTestBase("Check data transmission between MLDs without BA agreement",
                                  2,
                                  staChannels,
                                  apChannels,
                                  fixedPhyBands),
      m_errorModel(CreateObject<ListErrorModel>()),
      m_trafficPattern(trafficPattern)
{
}

void
MultiLinkTxNoBaTest::Transmit(uint8_t linkId,
                              std::string context,
                              WifiConstPsduMap psduMap,
                              WifiTxVector txVector,
                              double txPowerW)
{
    auto psdu = psduMap.begin()->second;
    auto uid = psdu->GetPacket()->GetUid();

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ACTION:
        // a management frame is a DL frame if TA equals BSSID
        CheckAddresses(psdu, psdu->GetHeader(0).GetAddr2() == psdu->GetHeader(0).GetAddr3());
        // corrupt all management action frames (ADDBA Request frames) to prevent
        // the establishment of a BA agreement
        m_uidList.push_front(uid);
        m_errorModel->SetList(m_uidList);
        NS_LOG_INFO("CORRUPTED");
        break;
    case WIFI_MAC_QOSDATA: {
        Address dlAddr3; // Source Address (SA)
        switch (m_trafficPattern)
        {
        case TrafficPattern::STA_TO_STA:
        case TrafficPattern::STA_TO_BCAST:
            dlAddr3 = m_staMacs[0]->GetDevice()->GetAddress();
            break;
        case TrafficPattern::STA_TO_AP:
            dlAddr3 = Mac48Address("00:00:00:00:00:00"); // invalid address, no DL frames
            break;
        case TrafficPattern::AP_TO_STA:
        case TrafficPattern::AP_TO_BCAST:
            dlAddr3 = m_apMac->GetAddress();
            break;
        }
        // a QoS data frame is a DL frame if Addr3 is the SA
        CheckAddresses(psdu, psdu->GetHeader(0).GetAddr3() == dlAddr3);
    }
        // corrupt the second QoS data frame (only once)
        if (psdu->GetHeader(0).GetSequenceNumber() != 1 ||
            m_trafficPattern == TrafficPattern::AP_TO_BCAST ||
            m_trafficPattern == TrafficPattern::STA_TO_BCAST)
        {
            break;
        }
        if (!m_dataCorrupted)
        {
            m_uidList.push_front(uid);
            m_dataCorrupted = true;
            NS_LOG_INFO("CORRUPTED");
            m_errorModel->SetList(m_uidList);
            break;
        }
        // do not corrupt the second data frame anymore
        if (auto it = std::find(m_uidList.cbegin(), m_uidList.cend(), uid); it != m_uidList.cend())
        {
            m_uidList.erase(it);
        }
        m_errorModel->SetList(m_uidList);
        break;
    default:;
    }

    MultiLinkOperationsTestBase::Transmit(linkId, context, psduMap, txVector, txPowerW);
}

void
MultiLinkTxNoBaTest::L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_INFO("Packet received by NODE " << +nodeId << "\n");
    m_rxPkts[nodeId]++;
}

void
MultiLinkTxNoBaTest::DoSetup()
{
    MultiLinkOperationsTestBase::DoSetup();

    // install post reception error model on receivers
    switch (m_trafficPattern)
    {
    case TrafficPattern::STA_TO_STA:
        for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
        }
        for (std::size_t linkId = 0; linkId < m_staMacs[1]->GetNLinks(); linkId++)
        {
            m_staMacs[1]->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
        }
        break;
    case TrafficPattern::STA_TO_AP:
        for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
        }
        break;
    case TrafficPattern::AP_TO_STA:
        for (std::size_t linkId = 0; linkId < m_staMacs[1]->GetNLinks(); linkId++)
        {
            m_staMacs[1]->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
        }
        break;
    case TrafficPattern::AP_TO_BCAST:
        // No frame to corrupt (broadcast frames do not trigger establishment of BA agreement)
        break;
    case TrafficPattern::STA_TO_BCAST:
        // uplink frames are not broadcast
        for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
        }
        break;
    }
}

void
MultiLinkTxNoBaTest::StartTraffic()
{
    const Time duration = Seconds(1);
    Ptr<WifiMac> sender;
    Address destAddr;

    switch (m_trafficPattern)
    {
    case TrafficPattern::STA_TO_STA:
        sender = m_staMacs[0];
        destAddr = m_staMacs[1]->GetDevice()->GetAddress();
        break;
    case TrafficPattern::STA_TO_AP:
        sender = m_staMacs[0];
        destAddr = m_apMac->GetDevice()->GetAddress();
        break;
    case TrafficPattern::AP_TO_STA:
        sender = m_apMac;
        destAddr = m_staMacs[1]->GetDevice()->GetAddress();
        break;
    case TrafficPattern::AP_TO_BCAST:
        sender = m_apMac;
        destAddr = Mac48Address::GetBroadcast();
        break;
    case TrafficPattern::STA_TO_BCAST:
        sender = m_staMacs[0];
        destAddr = Mac48Address::GetBroadcast();
        break;
    }

    PacketSocketHelper packetSocket;
    packetSocket.Install(m_apMac->GetDevice()->GetNode());
    packetSocket.Install(m_staMacs[0]->GetDevice()->GetNode());
    packetSocket.Install(m_staMacs[1]->GetDevice()->GetNode());

    PacketSocketAddress socket;
    socket.SetSingleDevice(sender->GetDevice()->GetIfIndex());
    socket.SetPhysicalAddress(destAddr);
    socket.SetProtocol(1);

    // install client application
    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(1400));
    client->SetAttribute("MaxPackets", UintegerValue(2));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetRemote(socket);
    sender->GetDevice()->GetNode()->AddApplication(client);
    client->SetStartTime(Seconds(0)); // now
    client->SetStopTime(duration);

    // install a server on all nodes
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); nodeIt++)
    {
        Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
        server->SetLocal(socket);
        (*nodeIt)->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(duration);
    }

    for (std::size_t nodeId = 0; nodeId < NodeList::GetNNodes(); nodeId++)
    {
        Config::ConnectWithoutContext(
            "/NodeList/" + std::to_string(nodeId) +
                "/ApplicationList/*/$ns3::PacketSocketServer/Rx",
            MakeCallback(&MultiLinkTxNoBaTest::L7Receive, this).Bind(nodeId));
    }

    Simulator::Stop(duration);
}

void
MultiLinkTxNoBaTest::DoRun()
{
    Simulator::Run();

    // Expected number of packets received by each node (AP, STA 0, STA 1) at application layer
    std::array<std::size_t, 3> expectedRxPkts{};

    switch (m_trafficPattern)
    {
    case TrafficPattern::STA_TO_STA:
    case TrafficPattern::AP_TO_STA:
        // only STA 1 receives the 2 packets that have been transmitted
        expectedRxPkts[2] = 2;
        break;
    case TrafficPattern::STA_TO_AP:
        // only the AP receives the 2 packets that have been transmitted
        expectedRxPkts[0] = 2;
        break;
    case TrafficPattern::AP_TO_BCAST:
        // the AP replicates the broadcast frames on all the links, hence each station
        // receives the 2 packets N times, where N is the number of setup link
        expectedRxPkts[1] = 2 * m_staMacs[0]->GetSetupLinkIds().size();
        expectedRxPkts[2] = 2 * m_staMacs[1]->GetSetupLinkIds().size();
        break;
    case TrafficPattern::STA_TO_BCAST:
        // the AP receives the two packets and then replicates them on all the links,
        // hence STA 1 receives 2 packets N times, where N is the number of setup link
        expectedRxPkts[0] = 2;
        expectedRxPkts[2] = 2 * m_staMacs[1]->GetSetupLinkIds().size();
        break;
    }

    NS_TEST_EXPECT_MSG_EQ(+m_rxPkts[0],
                          +expectedRxPkts[0],
                          "Unexpected number of packets received by the AP");
    NS_TEST_EXPECT_MSG_EQ(+m_rxPkts[1],
                          +expectedRxPkts[1],
                          "Unexpected number of packets received by STA 0");
    NS_TEST_EXPECT_MSG_EQ(+m_rxPkts[2],
                          +expectedRxPkts[2],
                          "Unexpected number of packets received by STA 1");

    Simulator::Destroy();
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
    using ParamsTuple = std::tuple<std::vector<std::string>,
                                   std::vector<std::string>,
                                   std::vector<std::pair<uint8_t, uint8_t>>,
                                   std::vector<uint8_t>>;

    AddTestCase(new GetRnrLinkInfoTest(), TestCase::QUICK);

    for (const auto& [staChannels, apChannels, setupLinks, fixedPhyBands] :
         {// matching channels: setup all links
          ParamsTuple({"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                      {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                      {{0, 0}, {1, 1}, {2, 2}},
                      {}),
          // non-matching channels, matching PHY bands: setup all links
          ParamsTuple({"{108, 0, BAND_5GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                      {"{36, 0, BAND_5GHZ, 0}", "{120, 0, BAND_5GHZ, 0}", "{5, 0, BAND_6GHZ, 0}"},
                      {{1, 0}, {0, 1}, {2, 2}},
                      {}),
          // non-AP MLD switches band on some links to setup 3 links
          ParamsTuple({"{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                      {"{36, 0, BAND_5GHZ, 0}", "{9, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                      {{2, 0}, {0, 1}, {1, 2}},
                      {}),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band, hence only 2 links are setup
          ParamsTuple(
              {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{8, 20, BAND_2_4GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
              {{1, 0}, {2, 1}},
              {0}),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band; the second link of the non-AP MLD cannot change PHY band and there is
          // an AP operating on the same channel; hence 2 links are setup
          ParamsTuple(
              {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{8, 20, BAND_2_4GHZ, 0}"},
              {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
              {{1, 0}, {2, 1}},
              {0, 1}),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band; the second link of the non-AP MLD cannot change PHY band and there is
          // an AP operating on the same channel; the third link of the non-AP MLD cannot
          // change PHY band and there is an AP operating on the same band (different channel);
          // hence 2 links are setup by switching channel (not band) on the third link
          ParamsTuple({"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{60, 0, BAND_5GHZ, 0}"},
                      {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                      {{1, 0}, {2, 2}},
                      {0, 1, 2}),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band; the second link of the non-AP MLD cannot change PHY band and there is
          // an AP operating on the same channel; hence one link only is setup
          ParamsTuple({"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                      {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                      {{1, 0}},
                      {0, 1}),
          // non-AP MLD has only two STAs and setups two links
          ParamsTuple({"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                      {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                      {{0, 1}, {1, 0}},
                      {}),
          // single link non-AP STA associates with an AP affiliated with an AP MLD
          ParamsTuple({"{120, 0, BAND_5GHZ, 0}"},
                      {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                      {{0, 2}},
                      {}),
          // a STA affiliated with a non-AP MLD associates with a single link AP
          ParamsTuple({"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                      {"{120, 0, BAND_5GHZ, 0}"},
                      {{2, 0}},
                      {})})
    {
        AddTestCase(new MultiLinkSetupTest(staChannels, apChannels, setupLinks, fixedPhyBands),
                    TestCase::QUICK);

        for (const auto& trafficPattern : {TrafficPattern::STA_TO_STA,
                                           TrafficPattern::STA_TO_AP,
                                           TrafficPattern::AP_TO_STA,
                                           TrafficPattern::AP_TO_BCAST,
                                           TrafficPattern::STA_TO_BCAST})
        {
            AddTestCase(
                new MultiLinkTxNoBaTest(trafficPattern, staChannels, apChannels, fixedPhyBands),
                TestCase::QUICK);
        }
    }
}

static WifiMultiLinkOperationsTestSuite g_wifiMultiLinkOperationsTestSuite; ///< the test suite
