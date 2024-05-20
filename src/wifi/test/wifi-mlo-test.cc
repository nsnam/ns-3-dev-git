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
#include "ns3/eht-configuration.h"
#include "ns3/frame-exchange-manager.h"
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
#include "ns3/rr-multi-user-scheduler.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-mac-header.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <optional>
#include <sstream>
#include <tuple>
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
 * Test the WifiMac::SwapLinks() method.
 */
class MldSwapLinksTest : public TestCase
{
    /**
     * Test WifiMac subclass used to access the SwapLinks method.
     */
    class TestWifiMac : public WifiMac
    {
      public:
        ~TestWifiMac() override = default;

        using WifiMac::GetLinks;
        using WifiMac::SwapLinks;

        bool CanForwardPacketsTo(Mac48Address to) const override
        {
            return true;
        }

        void Enqueue(Ptr<Packet> packet, Mac48Address to) override
        {
        }

      private:
        void DoCompleteConfig() override
        {
        }
    };

  public:
    MldSwapLinksTest();
    ~MldSwapLinksTest() override = default;

  protected:
    void DoRun() override;

  private:
    /**
     * Run a single test case.
     *
     * \param text string identifying the test case
     * \param nLinks the number of links of the MLD
     * \param links a set of pairs (from, to) each mapping a current link ID to the
     *              link ID it has to become (i.e., link 'from' becomes link 'to')
     * \param expected maps each link ID to the id of the PHY that is expected
     *                 to operate on that link after the swap
     */
    void RunOne(std::string text,
                std::size_t nLinks,
                const std::map<uint8_t, uint8_t>& links,
                const std::map<uint8_t, uint8_t>& expected);
};

MldSwapLinksTest::MldSwapLinksTest()
    : TestCase("Test the WifiMac::SwapLinks() method")
{
}

void
MldSwapLinksTest::RunOne(std::string text,
                         std::size_t nLinks,
                         const std::map<uint8_t, uint8_t>& links,
                         const std::map<uint8_t, uint8_t>& expected)
{
    TestWifiMac mac;

    std::vector<Ptr<WifiPhy>> phys;
    for (std::size_t i = 0; i < nLinks; i++)
    {
        phys.emplace_back(CreateObject<SpectrumWifiPhy>());
    }
    mac.SetWifiPhys(phys); // create links containing the given PHYs

    mac.SwapLinks(links);

    NS_TEST_EXPECT_MSG_EQ(mac.GetNLinks(), nLinks, "Number of links changed after swapping");

    for (const auto& [linkId, phyId] : expected)
    {
        NS_TEST_ASSERT_MSG_EQ(mac.GetLinks().count(linkId),
                              1,
                              "Link ID " << +linkId << " does not exist");

        NS_TEST_ASSERT_MSG_LT(+phyId, nLinks, "Invalid PHY ID");

        // the id of the PHY operating on a link is the original ID of the link
        NS_TEST_EXPECT_MSG_EQ(mac.GetWifiPhy(linkId),
                              phys.at(phyId),
                              text << ": Link " << +phyId << " has not been moved to link "
                                   << +linkId);
    }
}

void
MldSwapLinksTest::DoRun()
{
    RunOne("No change needed", 3, {{0, 0}, {1, 1}, {2, 2}}, {{0, 0}, {1, 1}, {2, 2}});
    RunOne("Circular swapping", 3, {{0, 2}, {1, 0}, {2, 1}}, {{0, 1}, {1, 2}, {2, 0}});
    RunOne("Swapping two links, one unchanged", 3, {{0, 2}, {2, 0}}, {{0, 2}, {1, 1}, {2, 0}});
    RunOne("Non-circular swapping, autodetect how to close the loop",
           3,
           {{0, 2}, {2, 1}},
           {{0, 1}, {1, 2}, {2, 0}});
    RunOne("One move only, autodetect how to complete the swapping",
           3,
           {{2, 0}},
           {{0, 2}, {1, 1}, {2, 0}});
    RunOne("Create a new link ID (2), remove the unused one (0)",
           2,
           {{0, 1}, {1, 2}},
           {{1, 0}, {2, 1}});
    RunOne("One move only that creates a new link ID (2)", 2, {{0, 2}}, {{1, 1}, {2, 0}});
    RunOne("Move all links to a new set of IDs", 2, {{0, 2}, {1, 3}}, {{2, 0}, {3, 1}});
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
     * Configuration parameters common to all subclasses
     */
    struct BaseParams
    {
        std::vector<std::string>
            staChannels; //!< the strings specifying the operating channels for the non-AP MLD
        std::vector<std::string>
            apChannels; //!< the strings specifying the operating channels for the AP MLD
        std::vector<uint8_t>
            fixedPhyBands; //!< list of IDs of non-AP MLD PHYs that cannot switch band
    };

    /**
     * Constructor
     *
     * \param name The name of the new TestCase created
     * \param nStations the number of stations to create
     * \param baseParams common configuration parameters
     */
    MultiLinkOperationsTestBase(const std::string& name,
                                uint8_t nStations,
                                const BaseParams& baseParams);
    ~MultiLinkOperationsTestBase() override = default;

  protected:
    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * \param mac the MAC transmitting the PSDUs
     * \param phyId the ID of the PHY transmitting the PSDUs
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    virtual void Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW);

    /**
     * Check that the expected Capabilities information elements are present in the given
     * management frame based on the band in which the given link is operating.
     *
     * \param mpdu the given management frame
     * \param mac the MAC transmitting the management frame
     * \param phyId the ID of the PHY transmitting the management frame
     */
    void CheckCapabilities(Ptr<WifiMpdu> mpdu, Ptr<WifiMac> mac, uint8_t phyId);

    /**
     * Function to trace packets received by the server application
     * \param nodeId the ID of the node that received the packet
     * \param p the packet
     * \param addr the address
     */
    virtual void L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr);

    /**
     * \param sockAddr the packet socket address identifying local outgoing interface
     *                 and remote address
     * \param count the number of packets to generate
     * \param pktSize the size of the packets to generate
     * \param delay the delay with which traffic generation starts
     * \param priority user priority for generated packets
     * \return an application generating the given number packets of the given size destined
     *         to the given packet socket address
     */
    Ptr<PacketSocketClient> GetApplication(const PacketSocketAddress& sockAddr,
                                           std::size_t count,
                                           std::size_t pktSize,
                                           Time delay = Seconds(0),
                                           uint8_t priority = 0) const;

    void DoSetup() override;

    /// PHY band-indexed map of spectrum channels
    using ChannelMap = std::map<FrequencyRange, Ptr<MultiModelSpectrumChannel>>;

    /**
     * Uplink or Downlink direction
     */
    enum Direction
    {
        DL = 0,
        UL
    };

    /**
     * Check that the Address 1 and Address 2 fields of the given PSDU contain device MAC addresses.
     *
     * \param psdu the given PSDU
     * \param direction indicates direction for management frames (DL or UL)
     */
    void CheckAddresses(Ptr<const WifiPsdu> psdu,
                        std::optional<Direction> direction = std::nullopt);

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
        uint8_t linkId;           ///< link ID
        uint8_t phyId;            ///< ID of the transmitting PHY
    };

    std::vector<FrameInfo> m_txPsdus;             ///< transmitted PSDUs
    const std::vector<std::string> m_staChannels; ///< strings specifying channels for STA
    const std::vector<std::string> m_apChannels;  ///< strings specifying channels for AP
    const std::vector<uint8_t> m_fixedPhyBands;   ///< links on non-AP MLD with fixed PHY band
    Ptr<ApWifiMac> m_apMac;                       ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs;       ///< STA wifi MACs
    uint8_t m_nStations;                          ///< number of stations to create
    uint16_t m_lastAid;                           ///< AID of last associated station
    Time m_duration{Seconds(1)};                  ///< simulation duration
    std::vector<std::size_t> m_rxPkts; ///< number of packets received at application layer
                                       ///< by each node (index is node ID)

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
                     const ChannelMap& channelMap);

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
                                                         const BaseParams& baseParams)
    : TestCase(name),
      m_staChannels(baseParams.staChannels),
      m_apChannels(baseParams.apChannels),
      m_fixedPhyBands(baseParams.fixedPhyBands),
      m_staMacs(nStations),
      m_nStations(nStations),
      m_lastAid(0),
      m_rxPkts(nStations + 1)
{
}

void
MultiLinkOperationsTestBase::CheckAddresses(Ptr<const WifiPsdu> psdu,
                                            std::optional<Direction> direction)
{
    std::optional<Mac48Address> apAddr;
    std::optional<Mac48Address> staAddr;

    // direction for Data frames is derived from ToDS/FromDS flags
    if (psdu->GetHeader(0).IsQosData())
    {
        direction = (!psdu->GetHeader(0).IsToDs() && psdu->GetHeader(0).IsFromDs()) ? DL : UL;
    }
    NS_ASSERT(direction);

    if (direction == DL)
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
            for (const auto& linkId : m_staMacs[i]->GetLinkIds())
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
MultiLinkOperationsTestBase::Transmit(Ptr<WifiMac> mac,
                                      uint8_t phyId,
                                      WifiConstPsduMap psduMap,
                                      WifiTxVector txVector,
                                      double txPowerW)
{
    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);
    m_txPsdus.push_back({Simulator::Now(), psduMap, txVector, *linkId, phyId});

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << "PSDU #" << m_txPsdus.size() << " Link ID "
           << +linkId.value() << " Phy ID " << +phyId << " " << psdu->GetHeader(0).GetTypeString()
           << " #MPDUs " << psdu->GetNMpdus() << " duration/ID " << psdu->GetHeader(0).GetDuration()
           << " RA = " << psdu->GetAddr1() << " TA = " << psdu->GetAddr2()
           << " ADDR3 = " << psdu->GetHeader(0).GetAddr3()
           << " ToDS = " << psdu->GetHeader(0).IsToDs()
           << " FromDS = " << psdu->GetHeader(0).IsFromDs();
        if (psdu->GetHeader(0).IsQosData())
        {
            ss << " seqNo = {";
            for (auto& mpdu : *PeekPointer(psdu))
            {
                ss << mpdu->GetHeader().GetSequenceNumber() << ",";
            }
            ss << "} TID = " << +psdu->GetHeader(0).GetQosTid();
        }
        NS_LOG_INFO(ss.str());

        CheckCapabilities(*psdu->begin(), mac, phyId);
    }
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");
}

void
MultiLinkOperationsTestBase::CheckCapabilities(Ptr<WifiMpdu> mpdu, Ptr<WifiMac> mac, uint8_t phyId)
{
    auto band = mac->GetDevice()->GetPhy(phyId)->GetPhyBand();
    bool hasHtCapabilities;
    bool hasVhtCapabilities;
    bool hasHeCapabilities;
    bool hasHe6GhzCapabilities;
    bool hasEhtCapabilities;

    auto findCapabilities = [&](auto&& frame) {
        hasHtCapabilities = frame.template Get<HtCapabilities>().has_value();
        hasVhtCapabilities = frame.template Get<VhtCapabilities>().has_value();
        hasHeCapabilities = frame.template Get<HeCapabilities>().has_value();
        hasHe6GhzCapabilities = frame.template Get<He6GhzBandCapabilities>().has_value();
        hasEhtCapabilities = frame.template Get<EhtCapabilities>().has_value();
    };

    switch (mpdu->GetHeader().GetType())
    {
    case WIFI_MAC_MGT_BEACON: {
        MgtBeaconHeader beacon;
        mpdu->GetPacket()->PeekHeader(beacon);
        findCapabilities(beacon);
    }
    break;

    case WIFI_MAC_MGT_PROBE_REQUEST: {
        MgtProbeRequestHeader probeReq;
        mpdu->GetPacket()->PeekHeader(probeReq);
        findCapabilities(probeReq);
    }
    break;

    case WIFI_MAC_MGT_PROBE_RESPONSE: {
        MgtProbeResponseHeader probeResp;
        mpdu->GetPacket()->PeekHeader(probeResp);
        findCapabilities(probeResp);
    }
    break;

    case WIFI_MAC_MGT_ASSOCIATION_REQUEST: {
        MgtAssocRequestHeader assocReq;
        mpdu->GetPacket()->PeekHeader(assocReq);
        findCapabilities(assocReq);
    }
    break;

    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE: {
        MgtAssocResponseHeader assocResp;
        mpdu->GetPacket()->PeekHeader(assocResp);
        findCapabilities(assocResp);
    }
    break;

    default:
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(
        hasHtCapabilities,
        (band != WIFI_PHY_BAND_6GHZ),
        "HT Capabilities should not be present in a mgt frame sent in 6 GHz band");
    NS_TEST_EXPECT_MSG_EQ(
        hasVhtCapabilities,
        (band == WIFI_PHY_BAND_5GHZ),
        "VHT Capabilities should only be present in a mgt frame sent in 5 GHz band");
    NS_TEST_EXPECT_MSG_EQ(hasHeCapabilities,
                          true,
                          "HE Capabilities should always be present in a mgt frame");
    NS_TEST_EXPECT_MSG_EQ(
        hasHe6GhzCapabilities,
        (band == WIFI_PHY_BAND_6GHZ),
        "HE 6GHz Band Capabilities should only be present in a mgt frame sent in 6 GHz band");
    NS_TEST_EXPECT_MSG_EQ(hasEhtCapabilities,
                          true,
                          "EHT Capabilities should always be present in a mgt frame");
}

void
MultiLinkOperationsTestBase::L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_INFO("Packet received by NODE " << +nodeId << "\n");
    m_rxPkts[nodeId]++;
}

void
MultiLinkOperationsTestBase::SetChannels(SpectrumWifiPhyHelper& helper,
                                         const std::vector<std::string>& channels,
                                         const ChannelMap& channelMap)
{
    helper = SpectrumWifiPhyHelper(channels.size());
    helper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    uint8_t linkId = 0;
    for (const auto& str : channels)
    {
        helper.Set(linkId++, "ChannelSettings", StringValue(str));
    }

    // NOTE replace this for loop with the line below to use a single spectrum channel
    // helper.SetChannel(channelMap.begin()->second);
    for (const auto& [band, channel] : channelMap)
    {
        helper.AddChannel(channel, band);
    }
}

void
MultiLinkOperationsTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(3);
    int64_t streamNumber = 30;

    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(m_nStations);

    WifiHelper wifi;
    // wifi.EnableLogComponents ();
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"),
                                 "ControlMode",
                                 StringValue("HtMcs0"));

    ChannelMap channelMap{{WIFI_SPECTRUM_2_4_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_5_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_6_GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    SpectrumWifiPhyHelper staPhyHelper;
    SpectrumWifiPhyHelper apPhyHelper;
    SetChannels(staPhyHelper, m_staChannels, channelMap);
    SetChannels(apPhyHelper, m_apChannels, channelMap);

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
    for (uint8_t phyId = 0; phyId < m_apMac->GetDevice()->GetNPhys(); phyId++)
    {
        Config::ConnectWithoutContext(
            "/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phys/" + std::to_string(phyId) +
                "/PhyTxPsduBegin",
            MakeCallback(&MultiLinkOperationsTestBase::Transmit, this).Bind(m_apMac, phyId));
    }
    for (uint8_t i = 0; i < m_nStations; i++)
    {
        for (uint8_t phyId = 0; phyId < m_staMacs[i]->GetDevice()->GetNPhys(); phyId++)
        {
            Config::ConnectWithoutContext("/NodeList/" + std::to_string(i + 1) +
                                              "/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                                              std::to_string(phyId) + "/PhyTxPsduBegin",
                                          MakeCallback(&MultiLinkOperationsTestBase::Transmit, this)
                                              .Bind(m_staMacs[i], phyId));
        }
    }

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // install a packet socket server on all nodes
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); ++nodeIt)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>((*nodeIt)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        (*nodeIt)->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(m_duration);
    }

    for (std::size_t nodeId = 0; nodeId < NodeList::GetNNodes(); nodeId++)
    {
        Config::ConnectWithoutContext(
            "/NodeList/" + std::to_string(nodeId) +
                "/ApplicationList/*/$ns3::PacketSocketServer/Rx",
            MakeCallback(&MultiLinkOperationsTestBase::L7Receive, this).Bind(nodeId));
    }

    // schedule ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext("AssociatedSta",
                                        MakeCallback(&MultiLinkOperationsTestBase::SetSsid, this));
    m_staMacs[0]->SetSsid(Ssid("ns-3-ssid"));
}

Ptr<PacketSocketClient>
MultiLinkOperationsTestBase::GetApplication(const PacketSocketAddress& sockAddr,
                                            std::size_t count,
                                            std::size_t pktSize,
                                            Time delay,
                                            uint8_t priority) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(sockAddr);
    client->SetStartTime(delay);
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
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
 *
 * The negotiated TID-to-link mapping is tested by verifying that generated QoS data frames of
 * a given TID are transmitted on links which the TID is mapped to. Specifically, the following
 * operations are performed separately for each direction (downlink and uplink). A first TID
 * is searched such that it is not mapped on all the setup links. If no such TID is found, only
 * QoS frames of TID 0 are generated. Otherwise, we also search for a second TID that is mapped
 * to a link set that is disjoint with the link set to which the first TID is mapped. If such a
 * TID is found, QoS frames of both the first TID and the second TID are generated; otherwise,
 * only QoS frames of the first TID are generated. For each TID, a number of QoS frames equal
 * to the number of setup links is generated. For each TID, we check that the first N QoS frames,
 * where N is the size of the link set to which the TID is mapped, are transmitted concurrently,
 * while the following QoS frames are sent after the first QoS frame sent on the same link. We
 * also check that all the QoS frames are sent on a link belonging to the link set to which the
 * TID is mapped. If QoS frames of two TIDs are generated, we also check that the first N QoS
 * frames of a TID, where N is the size of the link set to which that TID is mapped, are sent
 * concurrently with the first M QoS frames of the other TID, where M is the size of the link
 * set to which the other TID is mapped.
 */
class MultiLinkSetupTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * \param baseParams common configuration parameters
     * \param scanType the scan type (active or passive)
     * \param setupLinks a list of links that are expected to be setup. In case one of the two
     *                   devices has a single link, the ID of the link on the MLD is indicated
     * \param apNegSupport TID-to-Link Mapping negotiation supported by the AP MLD (0, 1, or 3)
     * \param dlTidToLinkMapping DL TID-to-Link Mapping for EHT configuration of non-AP MLD
     * \param ulTidToLinkMapping UL TID-to-Link Mapping for EHT configuration of non-AP MLD
     */
    MultiLinkSetupTest(const BaseParams& baseParams,
                       WifiScanType scanType,
                       const std::vector<uint8_t>& setupLinks,
                       WifiTidToLinkMappingNegSupport apNegSupport,
                       const std::string& dlTidToLinkMapping,
                       const std::string& ulTidToLinkMapping);
    ~MultiLinkSetupTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    /**
     * Check correctness of Multi-Link Setup procedure.
     */
    void CheckMlSetup();

    /**
     * Check that links that are not setup on the non-AP MLD are disabled. Also, on the AP side,
     * check that the queue storing QoS data frames destined to the non-AP MLD has a mask for a
     * link if and only if the link has been setup by the non-AO MLD.
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
     * Check correctness of the given Probe Response frame.
     *
     * \param mpdu the given Probe Response frame
     * \param linkId the ID of the link on which the Probe Response frame was transmitted
     */
    void CheckProbeResponse(Ptr<WifiMpdu> mpdu, uint8_t linkId);

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

    /**
     * Check that QoS data frames are sent on links their TID is mapped to.
     *
     * \param mpdu the given QoS data frame
     * \param linkId the ID of the link on which the QoS data frame was transmitted
     * \param index index of the QoS data frame in the vector of transmitted PSDUs
     */
    void CheckQosData(Ptr<WifiMpdu> mpdu, uint8_t linkId, std::size_t index);

    const std::vector<uint8_t> m_setupLinks; //!< IDs of the expected links to setup
    WifiScanType m_scanType;                 //!< the scan type (active or passive)
    std::size_t m_nProbeResp; //!< number of Probe Responses received by the non-AP MLD
    WifiTidToLinkMappingNegSupport
        m_apNegSupport;                //!< TID-to-Link Mapping negotiation supported by the AP MLD
    std::string m_dlTidLinkMappingStr; //!< DL TID-to-Link Mapping for non-AP MLD EHT configuration
    std::string m_ulTidLinkMappingStr; //!< UL TID-to-Link Mapping for non-AP MLD EHT configuration
    WifiTidLinkMapping m_dlTidLinkMapping; //!< expected DL TID-to-Link Mapping requested by non-AP
                                           //!< MLD and accepted by AP MLD
    WifiTidLinkMapping m_ulTidLinkMapping; //!< expected UL TID-to-Link Mapping requested by non-AP
                                           //!< MLD and accepted by AP MLD
    uint8_t m_dlTid1;                      //!< the TID of the first set of DL QoS data frames
    uint8_t m_ulTid1;                      //!< the TID of the first set of UL QoS data frames
    std::optional<uint8_t> m_dlTid2;       //!< the TID of the optional set of DL QoS data frames
    std::optional<uint8_t> m_ulTid2;       //!< the TID of the optional set of UL QoS data frames
    std::vector<std::size_t>
        m_qosFrames1; //!< indices of QoS frames of the first set in the vector of TX PSDUs
    std::vector<std::size_t>
        m_qosFrames2; //!< indices of QoS frames of the optional set in the vector of TX PSDUs
};

MultiLinkSetupTest::MultiLinkSetupTest(const BaseParams& baseParams,
                                       WifiScanType scanType,
                                       const std::vector<uint8_t>& setupLinks,
                                       WifiTidToLinkMappingNegSupport apNegSupport,
                                       const std::string& dlTidToLinkMapping,
                                       const std::string& ulTidToLinkMapping)
    : MultiLinkOperationsTestBase("Check correctness of Multi-Link Setup", 1, baseParams),
      m_setupLinks(setupLinks),
      m_scanType(scanType),
      m_nProbeResp(0),
      m_apNegSupport(apNegSupport),
      m_dlTidLinkMappingStr(dlTidToLinkMapping),
      m_ulTidLinkMappingStr(ulTidToLinkMapping)
{
}

void
MultiLinkSetupTest::DoSetup()
{
    MultiLinkOperationsTestBase::DoSetup();

    m_staMacs[0]->SetAttribute("ActiveProbing", BooleanValue(m_scanType == WifiScanType::ACTIVE));
    m_apMac->GetEhtConfiguration()->SetAttribute("TidToLinkMappingNegSupport",
                                                 EnumValue(m_apNegSupport));
    // For non-AP MLD, it does not make sense to set the negotiation type to 0 (unless the AP MLD
    // also advertises 0) or 1 (the AP MLD is discarded if it advertises a support of 3)
    auto staEhtConfig = m_staMacs[0]->GetEhtConfiguration();
    staEhtConfig->SetAttribute("TidToLinkMappingNegSupport",
                               EnumValue(WifiTidToLinkMappingNegSupport::ANY_LINK_SET));
    staEhtConfig->SetAttribute("TidToLinkMappingDl", StringValue(m_dlTidLinkMappingStr));
    staEhtConfig->SetAttribute("TidToLinkMappingUl", StringValue(m_ulTidLinkMappingStr));

    // the negotiated link mapping matches the one configured in EHT configuration, unless
    // the AP MLD does not support TID-to-link mapping negotiation or the AP MLD supports
    // the negotiation type 1 and the non-AP MLD is configured with a link mapping that
    // maps distinct link sets to the TIDs, in which case the default link mapping is used
    m_dlTidLinkMapping = staEhtConfig->GetTidLinkMapping(WifiDirection::DOWNLINK);
    m_ulTidLinkMapping = staEhtConfig->GetTidLinkMapping(WifiDirection::UPLINK);

    if (m_apNegSupport == WifiTidToLinkMappingNegSupport::NOT_SUPPORTED ||
        (m_apNegSupport == WifiTidToLinkMappingNegSupport::SAME_LINK_SET &&
         !TidToLinkMappingValidForNegType1(m_dlTidLinkMapping, m_ulTidLinkMapping)))
    {
        m_dlTidLinkMapping.clear(); // default link mapping
        m_ulTidLinkMapping.clear(); // default link mapping
    }

    // find (if any) a TID that is not mapped to all setup links
    using TupleRefs = std::tuple<std::reference_wrapper<const WifiTidLinkMapping>,
                                 std::reference_wrapper<uint8_t>,
                                 std::reference_wrapper<std::optional<uint8_t>>,
                                 Ptr<WifiMac>>;
    for (auto& [mappingRef, tid1Ref, tid2Ref, mac] :
         {TupleRefs{m_dlTidLinkMapping, m_dlTid1, m_dlTid2, m_apMac},
          TupleRefs{m_ulTidLinkMapping, m_ulTid1, m_ulTid2, m_staMacs[0]}})
    {
        tid1Ref.get() = 0;
        for (uint8_t tid1 = 0; tid1 < 8; tid1++)
        {
            if (auto it1 = mappingRef.get().find(tid1);
                it1 != mappingRef.get().cend() && it1->second.size() != m_setupLinks.size())
            {
                // found. Now search for another TID with a disjoint mapped link set
                for (uint8_t tid2 = tid1 + 1; tid2 < 8; tid2++)
                {
                    if (auto it2 = mappingRef.get().find(tid2);
                        it2 != mappingRef.get().cend() && it2->second.size() != m_setupLinks.size())
                    {
                        std::list<uint8_t> intersection;
                        std::set_intersection(it1->second.cbegin(),
                                              it1->second.cend(),
                                              it2->second.cbegin(),
                                              it2->second.cend(),
                                              std::back_inserter(intersection));
                        if (intersection.empty())
                        {
                            // found a second TID
                            tid2Ref.get() = tid2;
                            break;
                        }
                    }
                }
                tid1Ref.get() = tid1;
                break;
            }
        }

        std::list<uint8_t> tids = {tid1Ref.get()};
        if (tid2Ref.get())
        {
            tids.emplace_back(*tid2Ref.get());
        }

        // prevent aggregation of MPDUs
        for (auto tid : tids)
        {
            std::string attrName;
            switch (QosUtilsMapTidToAc(tid))
            {
            case AC_VI:
                attrName = "VI_MaxAmpduSize";
                break;
            case AC_VO:
                attrName = "VO_MaxAmpduSize";
                break;
            case AC_BE:
                attrName = "BE_MaxAmpduSize";
                break;
            case AC_BK:
                attrName = "BK_MaxAmpduSize";
                break;
            default:
                NS_FATAL_ERROR("Invalid TID " << +tid);
            }

            mac->SetAttribute(attrName, UintegerValue(100));
        }
    }
}

void
MultiLinkSetupTest::StartTraffic()
{
    // DL traffic
    {
        PacketSocketAddress sockAddr;
        sockAddr.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
        sockAddr.SetPhysicalAddress(m_staMacs[0]->GetDevice()->GetAddress());
        sockAddr.SetProtocol(1);

        m_apMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(sockAddr, m_setupLinks.size(), 500, Seconds(0), m_dlTid1));
        if (m_dlTid2)
        {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(sockAddr, m_setupLinks.size(), 500, Seconds(0), *m_dlTid2));
        }
    }

    // UL Traffic
    {
        PacketSocketAddress sockAddr;
        sockAddr.SetSingleDevice(m_staMacs[0]->GetDevice()->GetIfIndex());
        sockAddr.SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
        sockAddr.SetProtocol(1);

        m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
            GetApplication(sockAddr, m_setupLinks.size(), 500, MilliSeconds(500), m_ulTid1));
        if (m_ulTid2)
        {
            m_staMacs[0]->GetDevice()->GetNode()->AddApplication(
                GetApplication(sockAddr, m_setupLinks.size(), 500, MilliSeconds(500), *m_ulTid2));
        }
    }
}

void
MultiLinkSetupTest::DoRun()
{
    Simulator::Schedule(MilliSeconds(500), &MultiLinkSetupTest::CheckMlSetup, this);

    Simulator::Stop(m_duration);
    Simulator::Run();

    /**
     * Check content of management frames
     */
    std::size_t index = 0;

    for (const auto& frameInfo : m_txPsdus)
    {
        const auto& mpdu = *frameInfo.psduMap.begin()->second->begin();
        const auto& linkId = frameInfo.linkId;

        switch (mpdu->GetHeader().GetType())
        {
        case WIFI_MAC_MGT_BEACON:
            CheckBeacon(mpdu, linkId);
            break;

        case WIFI_MAC_MGT_PROBE_RESPONSE:
            CheckProbeResponse(mpdu, linkId);
            m_nProbeResp++;
            break;

        case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
            CheckAssocRequest(mpdu, linkId);
            break;

        case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
            CheckAssocResponse(mpdu, linkId);
            break;

        case WIFI_MAC_QOSDATA:
            CheckQosData(mpdu, linkId, index);
            break;

        default:
            break;
        }

        index++;
    }

    CheckDisabledLinks();

    std::size_t expectedProbeResp = 0;
    if (m_scanType == WifiScanType::ACTIVE)
    {
        // the number of Probe Response frames that we expect to receive in active mode equals
        // the number of channels in common between AP MLD and non-AP MLD at initialization
        for (const auto& staChannel : m_staChannels)
        {
            for (const auto& apChannel : m_apChannels)
            {
                if (staChannel == apChannel)
                {
                    expectedProbeResp++;
                    break;
                }
            }
        }
    }

    NS_TEST_EXPECT_MSG_EQ(m_nProbeResp, expectedProbeResp, "Unexpected number of Probe Responses");

    std::size_t expectedRxDlPkts = m_setupLinks.size();
    if (m_dlTid2)
    {
        expectedRxDlPkts *= 2;
    }
    NS_TEST_EXPECT_MSG_EQ(m_rxPkts[m_staMacs[0]->GetDevice()->GetNode()->GetId()],
                          expectedRxDlPkts,
                          "Unexpected number of DL packets received");

    std::size_t expectedRxUlPkts = m_setupLinks.size();
    if (m_ulTid2)
    {
        expectedRxUlPkts *= 2;
    }
    NS_TEST_EXPECT_MSG_EQ(m_rxPkts[m_apMac->GetDevice()->GetNode()->GetId()],
                          expectedRxUlPkts,
                          "Unexpected number of UL packets received");

    Simulator::Destroy();
}

void
MultiLinkSetupTest::CheckBeacon(Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
    NS_ABORT_IF(mpdu->GetHeader().GetType() != WIFI_MAC_MGT_BEACON);

    CheckAddresses(Create<WifiPsdu>(mpdu, false), MultiLinkOperationsTestBase::DL);

    NS_TEST_EXPECT_MSG_EQ(m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
                          mpdu->GetHeader().GetAddr2(),
                          "TA of Beacon frame is not the address of the link it is transmitted on");
    MgtBeaconHeader beacon;
    mpdu->GetPacket()->PeekHeader(beacon);
    const auto& rnr = beacon.Get<ReducedNeighborReport>();
    const auto& mle = beacon.Get<MultiLinkElement>();

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
MultiLinkSetupTest::CheckProbeResponse(Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
    NS_ABORT_IF(mpdu->GetHeader().GetType() != WIFI_MAC_MGT_PROBE_RESPONSE);

    CheckAddresses(Create<WifiPsdu>(mpdu, false), MultiLinkOperationsTestBase::DL);

    NS_TEST_EXPECT_MSG_EQ(
        m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
        mpdu->GetHeader().GetAddr2(),
        "TA of Probe Response is not the address of the link it is transmitted on");
    MgtProbeResponseHeader probeResp;
    mpdu->GetPacket()->PeekHeader(probeResp);
    const auto& rnr = probeResp.Get<ReducedNeighborReport>();
    const auto& mle = probeResp.Get<MultiLinkElement>();

    if (m_apMac->GetNLinks() == 1)
    {
        NS_TEST_EXPECT_MSG_EQ(rnr.has_value(),
                              false,
                              "RNR Element in Probe Response frame from single link AP");
        NS_TEST_EXPECT_MSG_EQ(mle.has_value(),
                              false,
                              "Multi-Link Element in Probe Response frame from single link AP");
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(rnr.has_value(), true, "No RNR Element in Probe Response frame");
    // All the other APs affiliated with the same AP MLD as the AP sending
    // the Probe Response frame must be reported in a separate Neighbor AP Info field
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

    NS_TEST_EXPECT_MSG_EQ(mle.has_value(), true, "No Multi-Link Element in Probe Response frame");
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

    CheckAddresses(Create<WifiPsdu>(mpdu, false), MultiLinkOperationsTestBase::UL);

    NS_TEST_EXPECT_MSG_EQ(
        m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress(),
        mpdu->GetHeader().GetAddr2(),
        "TA of Assoc Request frame is not the address of the link it is transmitted on");
    MgtAssocRequestHeader assoc;
    mpdu->GetPacket()->PeekHeader(assoc);
    const auto& mle = assoc.Get<MultiLinkElement>();

    if (m_apMac->GetNLinks() == 1 || m_staMacs[0]->GetNLinks() == 1)
    {
        NS_TEST_EXPECT_MSG_EQ(mle.has_value(),
                              false,
                              "Multi-Link Element in Assoc Request frame from single link STA");
    }
    else
    {
        NS_TEST_EXPECT_MSG_EQ(mle.has_value(),
                              true,
                              "No Multi-Link Element in Assoc Request frame");
        NS_TEST_EXPECT_MSG_EQ(mle->GetMldMacAddress(),
                              m_staMacs[0]->GetAddress(),
                              "Incorrect MLD Address advertised in Multi-Link Element");
        NS_TEST_EXPECT_MSG_EQ(
            mle->GetNPerStaProfileSubelements(),
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
            auto it = std::find(m_setupLinks.begin(), m_setupLinks.end(), staLinkId.value());
            NS_TEST_EXPECT_MSG_EQ((it != m_setupLinks.end()),
                                  true,
                                  "Not expecting to setup STA link ID " << +staLinkId.value());
            NS_TEST_EXPECT_MSG_EQ(
                +staLinkId.value(),
                +perStaProfile.GetLinkId(),
                "Not expecting to request association to AP Link ID in Per-STA Profile");
            NS_TEST_EXPECT_MSG_EQ(perStaProfile.HasAssocRequest(),
                                  true,
                                  "Missing Association Request in Per-STA Profile");
        }
    }

    const auto& tlm = assoc.Get<TidToLinkMapping>();

    // A TID-to-Link Mapping IE is included in the Association Request if and only if the AP MLD
    // and the non-AP MLD are performing ML setup (i.e., they both have multiple links) and the
    // AP MLD advertises a non-null negotiation support type
    if (m_apMac->GetNLinks() == 1 || m_staMacs[0]->GetNLinks() == 1 ||
        m_apNegSupport == WifiTidToLinkMappingNegSupport::NOT_SUPPORTED)
    {
        NS_TEST_EXPECT_MSG_EQ(tlm.empty(),
                              true,
                              "Didn't expect a TID-to-Link Mapping IE in Assoc Request frame");
    }
    else
    {
        std::size_t expectedNTlm = (m_dlTidLinkMapping == m_ulTidLinkMapping ? 1 : 2);

        NS_TEST_ASSERT_MSG_EQ(tlm.size(),
                              expectedNTlm,
                              "Unexpected number of TID-to-Link Mapping IE in Assoc Request");

        // lambda to check content of TID-to-Link Mapping IE(s)
        auto checkTlm = [&](std::size_t tlmId, WifiDirection dir) {
            NS_TEST_EXPECT_MSG_EQ(+static_cast<uint8_t>(tlm[tlmId].m_control.direction),
                                  +static_cast<uint8_t>(dir),
                                  "Unexpected direction in TID-to-Link Mapping IE " << tlmId);
            auto& expectedMapping =
                (dir == WifiDirection::UPLINK ? m_ulTidLinkMapping : m_dlTidLinkMapping);

            NS_TEST_EXPECT_MSG_EQ(tlm[tlmId].m_control.defaultMapping,
                                  expectedMapping.empty(),
                                  "Default Link Mapping bit not set correctly");
            NS_TEST_EXPECT_MSG_EQ(tlm[tlmId].m_linkMapping.size(),
                                  expectedMapping.size(),
                                  "Unexpected number of Link Mapping Of TID n fields");
            for (uint8_t tid = 0; tid < 8; tid++)
            {
                if (auto it = expectedMapping.find(tid); it != expectedMapping.cend())
                {
                    NS_TEST_EXPECT_MSG_EQ((tlm[tlmId].GetLinkMappingOfTid(tid) == it->second),
                                          true,
                                          "Unexpected link mapping for TID "
                                              << +tid << " direction " << dir);
                }
                else
                {
                    NS_TEST_EXPECT_MSG_EQ(tlm[tlmId].GetLinkMappingOfTid(tid).empty(),
                                          true,
                                          "Expecting no Link Mapping Of TID n field for TID "
                                              << +tid << " direction " << dir);
                }
            }
        };

        if (tlm.size() == 1)
        {
            checkTlm(0, WifiDirection::BOTH_DIRECTIONS);
        }
        else
        {
            std::size_t dlId = (tlm[0].m_control.direction == WifiDirection::DOWNLINK ? 0 : 1);
            std::size_t ulId = (dlId == 0 ? 1 : 0);

            checkTlm(dlId, WifiDirection::DOWNLINK);
            checkTlm(ulId, WifiDirection::UPLINK);
        }
    }
}

void
MultiLinkSetupTest::CheckAssocResponse(Ptr<WifiMpdu> mpdu, uint8_t linkId)
{
    NS_ABORT_IF(mpdu->GetHeader().GetType() != WIFI_MAC_MGT_ASSOCIATION_RESPONSE);

    CheckAddresses(Create<WifiPsdu>(mpdu, false), MultiLinkOperationsTestBase::DL);

    NS_TEST_EXPECT_MSG_EQ(
        m_apMac->GetFrameExchangeManager(linkId)->GetAddress(),
        mpdu->GetHeader().GetAddr2(),
        "TA of Assoc Response frame is not the address of the link it is transmitted on");
    MgtAssocResponseHeader assoc;
    mpdu->GetPacket()->PeekHeader(assoc);
    const auto& mle = assoc.Get<MultiLinkElement>();

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
        auto it = std::find(m_setupLinks.begin(), m_setupLinks.end(), apLinkId.value());
        NS_TEST_EXPECT_MSG_EQ((it != m_setupLinks.end()),
                              true,
                              "Not expecting to setup AP link ID " << +apLinkId.value());
        NS_TEST_EXPECT_MSG_EQ(perStaProfile.HasAssocResponse(),
                              true,
                              "Missing Association Response in Per-STA Profile");
    }

    // For the moment, the AP MLD always accepts a valid TID-to-Link Mapping request, hence
    // in every case there is no TID-to-Link Mapping IE in the Association Response
    NS_TEST_EXPECT_MSG_EQ(assoc.Get<TidToLinkMapping>().empty(),
                          true,
                          "Didn't expect to find a TID-to-Link Mapping IE in Association Response");
}

void
MultiLinkSetupTest::CheckMlSetup()
{
    /**
     * Check outcome of Multi-Link Setup
     */
    NS_TEST_EXPECT_MSG_EQ(m_staMacs[0]->IsAssociated(), true, "Expected the STA to be associated");

    for (const auto linkId : m_setupLinks)
    {
        auto staLinkId = (m_staMacs[0]->GetNLinks() > 1 ? linkId : SINGLE_LINK_OP_ID);
        auto apLinkId = (m_apMac->GetNLinks() > 1 ? linkId : SINGLE_LINK_OP_ID);

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

    // lambda to check the link mapping stored at wifi MAC
    auto checkStoredMapping =
        [this](Ptr<WifiMac> mac, Ptr<WifiMac> dest, WifiDirection dir, bool present) {
            NS_TEST_ASSERT_MSG_EQ(mac->GetTidToLinkMapping(dest->GetAddress(), dir).has_value(),
                                  present,
                                  "Link mapping stored by "
                                      << (mac->GetTypeOfStation() == AP ? "AP" : "non-AP")
                                      << " MLD for " << dir << " direction "
                                      << (present ? "expected" : "not expected"));
            if (present)
            {
                const auto& mapping =
                    (dir == WifiDirection::DOWNLINK ? m_dlTidLinkMapping : m_ulTidLinkMapping);
                NS_TEST_EXPECT_MSG_EQ(
                    (mac->GetTidToLinkMapping(dest->GetAddress(), dir)->get() == mapping),
                    true,
                    "Incorrect link mapping stored by "
                        << (mac->GetTypeOfStation() == AP ? "AP" : "non-AP") << " MLD for " << dir
                        << " direction");
            }
        };

    auto storedMapping = m_apMac->GetNLinks() > 1 && m_staMacs[0]->GetNLinks() > 1 &&
                         m_apNegSupport > WifiTidToLinkMappingNegSupport::NOT_SUPPORTED;
    checkStoredMapping(m_apMac, m_staMacs[0], WifiDirection::DOWNLINK, storedMapping);
    checkStoredMapping(m_apMac, m_staMacs[0], WifiDirection::UPLINK, storedMapping);
    checkStoredMapping(m_staMacs[0], m_apMac, WifiDirection::DOWNLINK, storedMapping);
    checkStoredMapping(m_staMacs[0], m_apMac, WifiDirection::UPLINK, storedMapping);
}

void
MultiLinkSetupTest::CheckDisabledLinks()
{
    if (m_apMac->GetNLinks() > 1)
    {
        WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE,
                                     WIFI_UNICAST,
                                     m_staMacs[0]->GetAddress(),
                                     0);

        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
        {
            auto it = std::find(m_setupLinks.cbegin(), m_setupLinks.cend(), linkId);

            // the queue on the AP should have a mask if and only if the link has been setup
            auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
            NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                  (it != m_setupLinks.cend()),
                                  "Unexpected presence/absence of mask on link " << +linkId);
        }
    }

    if (m_staMacs[0]->GetNLinks() == 1)
    {
        // no link is disabled on a single link device
        return;
    }

    for (const auto& linkId : m_staMacs[0]->GetLinkIds())
    {
        auto it = std::find(m_setupLinks.begin(), m_setupLinks.end(), linkId);
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

void
MultiLinkSetupTest::CheckQosData(Ptr<WifiMpdu> mpdu, uint8_t linkId, std::size_t index)
{
    WifiDirection dir;
    const auto& hdr = mpdu->GetHeader();

    NS_TEST_ASSERT_MSG_EQ(hdr.IsQosData(), true, "Expected a QoS data frame");

    if (!hdr.IsToDs() && hdr.IsFromDs())
    {
        dir = WifiDirection::DOWNLINK;
    }
    else if (hdr.IsToDs() && !hdr.IsFromDs())
    {
        dir = WifiDirection::UPLINK;
    }
    else
    {
        NS_ABORT_MSG("Invalid combination for QoS data frame: ToDS(" << hdr.IsToDs() << ") FromDS("
                                                                     << hdr.IsFromDs() << ")");
    }

    const auto& tid1 = (dir == WifiDirection::DOWNLINK) ? m_dlTid1 : m_ulTid1;
    const auto& tid2 = (dir == WifiDirection::DOWNLINK) ? m_dlTid2 : m_ulTid2;
    uint8_t tid = hdr.GetQosTid();

    NS_TEST_ASSERT_MSG_NE((tid == tid1),
                          (tid2.has_value() && tid == *tid2),
                          "QoS frame with unexpected TID " << +tid);

    // lambda to find the link set the given TID is mapped to
    auto findLinkSet = [this, dir](uint8_t tid) -> std::set<uint8_t> {
        std::set<uint8_t> linkSet(m_setupLinks.cbegin(), m_setupLinks.cend());
        if (auto mappingOptRef = m_apMac->GetTidToLinkMapping(m_staMacs[0]->GetAddress(), dir))
        {
            // if the TID is not present in the mapping, it is mapped to all setup links
            if (auto it = mappingOptRef->get().find(tid); it != mappingOptRef->get().cend())
            {
                linkSet = it->second;
                NS_ASSERT_MSG(!linkSet.empty(), "TID " << +tid << " mapped to no link");
            }
        }
        return linkSet;
    };

    auto linkSet = findLinkSet(tid);
    auto& qosFrames = (tid == tid1) ? m_qosFrames1 : m_qosFrames2;

    // Let N the size of the link set, the first N QoS data frames are sent simultaneously
    // on the links of the set, the others (if any) will be sent afterwards on such links

    // number of concurrent frames of the same TID transmitted so far (excluding current frame)
    std::size_t nConcurFrames = std::min(qosFrames.size(), linkSet.size());

    // iterate over the concurrent frames of the same TID transmitted so far
    for (std::size_t i = 0; i < nConcurFrames; i++)
    {
        auto prev = qosFrames[i];

        // TX duration of i-th frame
        auto band = m_apMac->GetWifiPhy(m_txPsdus[prev].linkId)->GetPhyBand();
        Time txDuration =
            WifiPhy::CalculateTxDuration(m_txPsdus[prev].psduMap, m_txPsdus[prev].txVector, band);

        // the current frame is transmitted concurrently with this previous frame if it is
        // within the first N (size of the link set) frames, otherwise it is transmitted after
        // this previous frame if they have been transmitted on the same link
        if (qosFrames.size() < linkSet.size())
        {
            // the current frame can be sent concurrently with this previous frame
            NS_TEST_EXPECT_MSG_LT(m_txPsdus[index].startTx,
                                  m_txPsdus[prev].startTx + txDuration,
                                  "The " << dir << " QoS frame number " << qosFrames.size()
                                         << " was not sent concurrently with others on link "
                                         << +linkId << " which TID " << +tid << " is mapped to");
        }
        else if (m_txPsdus[prev].linkId == linkId)
        {
            // the current  frame is sent afterwards
            NS_TEST_EXPECT_MSG_GT(m_txPsdus[index].startTx,
                                  m_txPsdus[prev].startTx + txDuration,
                                  "The " << dir << " QoS frame number " << qosFrames.size()
                                         << " was sent concurrently with others on a link "
                                         << +linkId << " which TID " << +tid << " is mapped to");
        }
    }

    if (m_apMac->GetNLinks() > 1 && m_staMacs[0]->GetNLinks() > 1)
    {
        NS_TEST_EXPECT_MSG_EQ(std::count(linkSet.cbegin(), linkSet.cend(), linkId),
                              1,
                              "QoS frame sent on Link ID "
                                  << +linkId << " that does not belong to the link set of TID "
                                  << +tid);
    }

    if (tid2)
    {
        // QoS frames of two distinct TIDs are sent.
        auto otherTid = (tid == tid1) ? *tid2 : tid1;
        const auto& otherQosFrames = (tid == tid1) ? m_qosFrames2 : m_qosFrames1;
        auto otherLinkSet = findLinkSet(otherTid);

        // number of concurrent frames of the other TID transmitted so far
        std::size_t nOtherConcurFrames = std::min(otherQosFrames.size(), otherLinkSet.size());

        // iterate over the concurrent frames of the other TID
        for (std::size_t i = 0; i < nOtherConcurFrames; i++)
        {
            auto prev = otherQosFrames[i];

            // TX duration of i-th frame
            auto band = m_apMac->GetWifiPhy(m_txPsdus[prev].linkId)->GetPhyBand();
            Time txDuration = WifiPhy::CalculateTxDuration(m_txPsdus[prev].psduMap,
                                                           m_txPsdus[prev].txVector,
                                                           band);

            // the current frame is transmitted concurrently with this previous frame of the
            // other TID if it is within the first N (size of the link set) frames of its TID
            if (qosFrames.size() < linkSet.size())
            {
                // the current frame can be sent concurrently with this previous frame
                NS_TEST_EXPECT_MSG_LT(m_txPsdus[index].startTx,
                                      m_txPsdus[prev].startTx + txDuration,
                                      "The " << dir << " QoS frame number " << qosFrames.size()
                                             << " was not sent concurrently with others with TID "
                                             << +otherTid);
            }
        }
    }

    // insert the frame
    qosFrames.emplace_back(index);

    if (qosFrames.size() == m_setupLinks.size())
    {
        qosFrames.clear();
    }
}

/**
 * Tested traffic patterns.
 */
enum class WifiTrafficPattern : uint8_t
{
    STA_TO_STA = 0,
    STA_TO_AP,
    AP_TO_STA,
    AP_TO_BCAST,
    STA_TO_BCAST
};

/**
 * Block Ack agreement enabled/disabled
 */
enum class WifiBaEnabled : uint8_t
{
    NO = 0,
    YES
};

/**
 * Whether to send a BlockAckReq after a missed BlockAck
 */
enum class WifiUseBarAfterMissedBa : uint8_t
{
    NO = 0,
    YES
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test data transmission between two MLDs.
 *
 * This test sets up an AP MLD and two non-AP MLDs having a variable number of links.
 * The RF channels to set each link to are provided as input parameters through the test
 * case constructor, along with the identifiers (starting at 0) of the links that cannot
 * switch PHY band (if any). This test aims at veryfing the successful transmission of both
 * unicast QoS data frames (from one station to another, from one station to the AP, from
 * the AP to the station) and broadcast QoS data frames (from the AP or from one station).
 * In the scenarios in which the AP forwards frames (i.e., from one station to another and
 * from one station to broadcast) the client application generates only 4 packets, in order
 * to limit the probability of collisions. In the other scenarios, 8 packets are generated.
 * When BlockAck agreements are enabled, the maximum A-MSDU size is set such that two
 * packets can be aggregated in an A-MSDU. The MPDU with sequence number equal to 1 is
 * corrupted (once, by using a post reception error model) to test its successful
 * re-transmission, unless the traffic scenario is from the AP to broadcast (broadcast frames
 * are not retransmitted) or is a scenario where the AP forwards frame (to limit the
 * probability of collisions).
 *
 * When BlockAck agreements are enabled, we also corrupt a BlockAck frame, so as to simulate
 * the case of BlockAck timeout. Both the case where a BlockAckReq is sent and the case where
 * data frame are retransmitted are tested. Finally, when BlockAck agreements are enabled, we
 * also enable the concurrent transmission of data frames over two links and check that at
 * least one MPDU is concurrently transmitted over two links.
 */
class MultiLinkTxTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * \param baseParams common configuration parameters
     * \param trafficPattern the pattern of traffic to generate
     * \param baEnabled whether BA agreement is enabled or disabled
     * \param useBarAfterMissedBa whether a BAR or Data frames are sent after missed BlockAck
     * \param nMaxInflight the max number of links on which an MPDU can be simultaneously inflight
     *                     (unused if Block Ack agreements are not established)
     */
    MultiLinkTxTest(const BaseParams& baseParams,
                    WifiTrafficPattern trafficPattern,
                    WifiBaEnabled baEnabled,
                    WifiUseBarAfterMissedBa useBarAfterMissedBa,
                    uint8_t nMaxInflight);
    ~MultiLinkTxTest() override = default;

  protected:
    /**
     * Check the content of a received BlockAck frame when the max number of links on which
     * an MPDU can be inflight is one.
     *
     * \param psdu the PSDU containing the BlockAck
     * \param txVector the TXVECTOR used to transmit the BlockAck
     * \param linkId the ID of the link on which the BlockAck was transmitted
     */
    void CheckBlockAck(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId);

    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    /// Receiver address-indexed map of list error models
    using RxErrorModelMap = std::unordered_map<Mac48Address, Ptr<ListErrorModel>, WifiAddressHash>;

    RxErrorModelMap m_errorModels;       ///< error rate models to corrupt packets
    std::list<uint64_t> m_uidList;       ///< list of UIDs of packets to corrupt
    bool m_dataCorrupted{false};         ///< whether second data frame has been already corrupted
    WifiTrafficPattern m_trafficPattern; ///< the pattern of traffic to generate
    bool m_baEnabled;                    ///< whether BA agreement is enabled or disabled
    bool m_useBarAfterMissedBa;          ///< whether to send BAR after missed BlockAck
    std::size_t m_nMaxInflight;          ///< max number of links on which an MPDU can be inflight
    std::size_t m_nPackets;              ///< number of application packets to generate
    std::size_t m_blockAckCount{0};      ///< transmitted BlockAck counter
    std::size_t m_blockAckReqCount{0};   ///< transmitted BlockAckReq counter
    std::map<uint16_t, std::size_t> m_inflightCount; ///< seqNo-indexed max number of simultaneous
                                                     ///< transmissions of a data frame
    Ptr<WifiMac> m_sourceMac; ///< MAC of the node sending application packets
};

MultiLinkTxTest::MultiLinkTxTest(const BaseParams& baseParams,
                                 WifiTrafficPattern trafficPattern,
                                 WifiBaEnabled baEnabled,
                                 WifiUseBarAfterMissedBa useBarAfterMissedBa,
                                 uint8_t nMaxInflight)
    : MultiLinkOperationsTestBase(
          std::string("Check data transmission between MLDs ") +
              (baEnabled == WifiBaEnabled::YES
                   ? (useBarAfterMissedBa == WifiUseBarAfterMissedBa::YES
                          ? "with BA agreement, send BAR after BlockAck timeout"
                          : "with BA agreement, send Data frames after BlockAck timeout")
                   : "without BA agreement") +
              " (Traffic pattern: " + std::to_string(static_cast<uint8_t>(trafficPattern)) +
              (baEnabled == WifiBaEnabled::YES ? ", nMaxInflight=" + std::to_string(nMaxInflight)
                                               : "") +
              ")",
          2,
          baseParams),
      m_trafficPattern(trafficPattern),
      m_baEnabled(baEnabled == WifiBaEnabled::YES),
      m_useBarAfterMissedBa(useBarAfterMissedBa == WifiUseBarAfterMissedBa::YES),
      m_nMaxInflight(nMaxInflight),
      m_nPackets(trafficPattern == WifiTrafficPattern::STA_TO_BCAST ||
                         trafficPattern == WifiTrafficPattern::STA_TO_STA
                     ? 4
                     : 8)
{
}

void
MultiLinkTxTest::Transmit(Ptr<WifiMac> mac,
                          uint8_t phyId,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW)
{
    MultiLinkOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    auto psdu = psduMap.begin()->second;

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ACTION:
        // a management frame is a DL frame if TA equals BSSID
        CheckAddresses(psdu,
                       psdu->GetHeader(0).GetAddr2() == psdu->GetHeader(0).GetAddr3() ? DL : UL);
        if (!m_baEnabled)
        {
            // corrupt all management action frames (ADDBA Request frames) to prevent
            // the establishment of a BA agreement
            m_uidList.push_front(psdu->GetPacket()->GetUid());
            m_errorModels.at(psdu->GetAddr1())->SetList(m_uidList);
            NS_LOG_INFO("CORRUPTED");
        }
        break;
    case WIFI_MAC_QOSDATA:
        CheckAddresses(psdu);

        for (const auto& mpdu : *psdu)
        {
            // determine the max number of simultaneous transmissions for this MPDU
            // (only if sent by the traffic source and this is not a broadcast frame)
            if (m_baEnabled && m_sourceMac->GetLinkIds().count(linkId) == 1 &&
                m_sourceMac->GetFrameExchangeManager(linkId)->GetAddress() ==
                    mpdu->GetHeader().GetAddr2() &&
                !mpdu->GetHeader().GetAddr1().IsGroup())
            {
                auto seqNo = mpdu->GetHeader().GetSequenceNumber();
                auto [it, success] =
                    m_inflightCount.insert({seqNo, mpdu->GetInFlightLinkIds().size()});
                if (!success)
                {
                    it->second = std::max(it->second, mpdu->GetInFlightLinkIds().size());
                }
            }
        }
        for (std::size_t i = 0; i < psdu->GetNMpdus(); i++)
        {
            // corrupt QoS data frame with sequence number equal to 1 (only once) if we are
            // not in the AP to broadcast traffic pattern (broadcast frames are not retransmitted)
            // nor in the STA to broadcast or STA to STA traffic patterns (retransmissions from
            // STA 1 could collide with frames forwarded by the AP)
            if (psdu->GetHeader(i).GetSequenceNumber() != 1 ||
                m_trafficPattern == WifiTrafficPattern::AP_TO_BCAST ||
                m_trafficPattern == WifiTrafficPattern::STA_TO_BCAST ||
                m_trafficPattern == WifiTrafficPattern::STA_TO_STA)
            {
                continue;
            }
            auto uid = psdu->GetPayload(i)->GetUid();
            if (!m_dataCorrupted)
            {
                m_uidList.push_front(uid);
                m_dataCorrupted = true;
                NS_LOG_INFO("CORRUPTED");
                m_errorModels.at(psdu->GetAddr1())->SetList(m_uidList);
            }
            else
            {
                // do not corrupt the QoS data frame anymore
                if (auto it = std::find(m_uidList.cbegin(), m_uidList.cend(), uid);
                    it != m_uidList.cend())
                {
                    m_uidList.erase(it);
                }
                m_errorModels.at(psdu->GetAddr1())->SetList(m_uidList);
            }
            break;
        }
        break;
    case WIFI_MAC_CTL_BACKRESP: {
        // ignore BlockAck frames not addressed to the source of the application packets
        if (!m_sourceMac->GetLinkIdByAddress(psdu->GetHeader(0).GetAddr1()))
        {
            break;
        }
        if (m_nMaxInflight > 1)
        {
            // we do not check the content of BlockAck when m_nMaxInflight is greater than 1
            break;
        }
        CheckBlockAck(psdu, txVector, linkId);
        m_blockAckCount++;
        if (m_blockAckCount == 2)
        {
            // corrupt the second BlockAck frame to simulate a missed BlockAck
            m_uidList.push_front(psdu->GetPacket()->GetUid());
            NS_LOG_INFO("CORRUPTED");
            m_errorModels.at(psdu->GetAddr1())->SetList(m_uidList);
        }
        break;
    case WIFI_MAC_CTL_BACKREQ:
        // ignore BlockAckReq frames not transmitted by the source of the application packets
        if (m_sourceMac->GetLinkIdByAddress(psdu->GetHeader(0).GetAddr2()))
        {
            m_blockAckReqCount++;
        }
        break;
    }
    default:;
    }
}

void
MultiLinkTxTest::CheckBlockAck(Ptr<const WifiPsdu> psdu,
                               const WifiTxVector& txVector,
                               uint8_t linkId)
{
    NS_TEST_ASSERT_MSG_EQ(m_baEnabled, true, "No BlockAck expected without BA agreement");
    NS_TEST_ASSERT_MSG_EQ((m_trafficPattern != WifiTrafficPattern::AP_TO_BCAST),
                          true,
                          "No BlockAck expected in AP to broadcast traffic pattern");

    /*
     *         ┌───────┬───────X        ┌───────┐
     *  link 0 │   0   │   1   │        │   1   │
     *  ───────┴───────┴───────┴┬──┬────┴───────┴┬───┬────────────────────────
     *                          │BA│             │ACK│
     *                          └──┘             └───┘
     *                      ┌───────┬───────┐       ┌───────┬───────┐
     *  link 1              │   2   │   3   │       │   2   │   3   │
     *  ────────────────────┴───────┴───────┴┬──X───┴───────┴───────┴┬──┬─────
     *                                       │BA│                    │BA│
     *                                       └──┘                    └──┘
     */
    auto mpdu = *psdu->begin();
    CtrlBAckResponseHeader blockAck;
    mpdu->GetPacket()->PeekHeader(blockAck);
    bool isMpdu1corrupted = (m_trafficPattern == WifiTrafficPattern::STA_TO_AP ||
                             m_trafficPattern == WifiTrafficPattern::AP_TO_STA);

    switch (m_blockAckCount)
    {
    case 0: // first BlockAck frame (all traffic patterns)
        NS_TEST_EXPECT_MSG_EQ(blockAck.IsPacketReceived(0),
                              true,
                              "MPDU 0 expected to be successfully received");
        NS_TEST_EXPECT_MSG_EQ(
            blockAck.IsPacketReceived(1),
            !isMpdu1corrupted,
            "MPDU 1 expected to be received only in STA_TO_STA/STA_TO_BCAST scenarios");
        // if there are at least two links setup, we expect all MPDUs to be inflight
        // (on distinct links)
        if (m_staMacs[0]->GetSetupLinkIds().size() > 1)
        {
            auto queue = m_sourceMac->GetTxopQueue(AC_BE);
            auto rcvMac = m_sourceMac == m_staMacs[0] ? StaticCast<WifiMac>(m_apMac)
                                                      : StaticCast<WifiMac>(m_staMacs[1]);
            auto item = queue->PeekByTidAndAddress(0, rcvMac->GetAddress());
            std::size_t nQueuedPkt = 0;
            auto delay = WifiPhy::CalculateTxDuration(psdu,
                                                      txVector,
                                                      rcvMac->GetWifiPhy(linkId)->GetPhyBand()) +
                         MicroSeconds(1); // to account for propagation delay

            while (item)
            {
                auto seqNo = item->GetHeader().GetSequenceNumber();
                NS_TEST_EXPECT_MSG_EQ(item->IsInFlight(),
                                      true,
                                      "MPDU with seqNo=" << seqNo << " is not in flight");
                auto linkIds = item->GetInFlightLinkIds();
                NS_TEST_EXPECT_MSG_EQ(linkIds.size(),
                                      1,
                                      "MPDU with seqNo=" << seqNo
                                                         << " is in flight on multiple links");
                // The first two MPDUs are in flight on the same link on which the BlockAck
                // is sent. The other two MPDUs (only for AP to STA/STA to AP scenarios) are
                // in flight on a different link.
                auto srcLinkId = m_sourceMac->GetLinkIdByAddress(mpdu->GetHeader().GetAddr1());
                NS_TEST_ASSERT_MSG_EQ(srcLinkId.has_value(),
                                      true,
                                      "Addr1 of BlockAck is not an originator's link address");
                NS_TEST_EXPECT_MSG_EQ((*linkIds.begin() == *srcLinkId),
                                      (seqNo <= 1),
                                      "MPDU with seqNo=" << seqNo
                                                         << " in flight on unexpected link");
                // check the Retry subfield and whether this MPDU is still queued
                // after the originator has processed this BlockAck

                // MPDUs acknowledged via this BlockAck are no longer queued
                bool isQueued = (seqNo > (isMpdu1corrupted ? 0 : 1));
                // The Retry subfield is set if the MPDU has not been acknowledged (i.e., it
                // is still queued) and has been transmitted on the same link as the BlockAck
                // (i.e., its sequence number is less than or equal to 1)
                bool isRetry = isQueued && seqNo <= 1;

                Simulator::Schedule(delay, [this, item, isQueued, isRetry]() {
                    NS_TEST_EXPECT_MSG_EQ(item->IsQueued(),
                                          isQueued,
                                          "MPDU with seqNo="
                                              << item->GetHeader().GetSequenceNumber() << " should "
                                              << (isQueued ? "" : "not") << " be queued");
                    NS_TEST_EXPECT_MSG_EQ(
                        item->GetHeader().IsRetry(),
                        isRetry,
                        "Unexpected value for the Retry subfield of the MPDU with seqNo="
                            << item->GetHeader().GetSequenceNumber());
                });

                nQueuedPkt++;
                item = queue->PeekByTidAndAddress(0, rcvMac->GetAddress(), item);
            }
            // Each MPDU contains an A-MSDU consisting of two MSDUs
            NS_TEST_EXPECT_MSG_EQ(nQueuedPkt, m_nPackets / 2, "Unexpected number of queued MPDUs");
        }
        break;
    case 1: // second BlockAck frame (STA to AP and AP to STA traffic patterns only)
    case 2: // third BlockAck frame (STA to AP and AP to STA traffic patterns only)
        NS_TEST_EXPECT_MSG_EQ((m_trafficPattern == WifiTrafficPattern::AP_TO_STA ||
                               m_trafficPattern == WifiTrafficPattern::STA_TO_AP),
                              true,
                              "Did not expect to receive a second BlockAck");
        // the second BlockAck is corrupted, but the data frames have been received successfully
        std::pair<uint16_t, uint16_t> seqNos;
        // if multiple links were setup, the transmission of the second A-MPDU started before
        // the end of the first one, so the second A-MPDU includes MPDUs with sequence numbers
        // 2 and 3. Otherwise, MPDU with sequence number 1 is retransmitted along with the MPDU
        // with sequence number 2.
        if (m_staMacs[0]->GetSetupLinkIds().size() > 1)
        {
            seqNos = {2, 3};
        }
        else
        {
            seqNos = {1, 2};
        }
        NS_TEST_EXPECT_MSG_EQ(blockAck.IsPacketReceived(seqNos.first),
                              true,
                              "MPDU " << seqNos.first << " expected to be successfully received");
        NS_TEST_EXPECT_MSG_EQ(blockAck.IsPacketReceived(seqNos.second),
                              true,
                              "MPDU " << seqNos.second << " expected to be successfully received");
        break;
    }
}

void
MultiLinkTxTest::DoSetup()
{
    MultiLinkOperationsTestBase::DoSetup();

    if (m_baEnabled)
    {
        // Enable A-MSDU aggregation. Max A-MSDU size is set such that two MSDUs can be aggregated
        for (auto mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMacs[0], m_staMacs[1]})
        {
            mac->SetAttribute("BE_MaxAmsduSize", UintegerValue(2100));
            mac->GetQosTxop(AC_BE)->SetAttribute("UseExplicitBarAfterMissedBlockAck",
                                                 BooleanValue(m_useBarAfterMissedBa));
            mac->GetQosTxop(AC_BE)->SetAttribute("NMaxInflights", UintegerValue(m_nMaxInflight));
        }
    }

    // install post reception error model on all devices
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        auto errorModel = CreateObject<ListErrorModel>();
        m_errorModels[m_apMac->GetFrameExchangeManager(linkId)->GetAddress()] = errorModel;
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(errorModel);
    }
    for (std::size_t i : {0, 1})
    {
        for (const auto linkId : m_staMacs[i]->GetLinkIds())
        {
            auto errorModel = CreateObject<ListErrorModel>();
            m_errorModels[m_staMacs[i]->GetFrameExchangeManager(linkId)->GetAddress()] = errorModel;
            m_staMacs[i]->GetWifiPhy(linkId)->SetPostReceptionErrorModel(errorModel);
        }
    }
}

void
MultiLinkTxTest::StartTraffic()
{
    Address destAddr;

    switch (m_trafficPattern)
    {
    case WifiTrafficPattern::STA_TO_STA:
        m_sourceMac = m_staMacs[0];
        destAddr = m_staMacs[1]->GetDevice()->GetAddress();
        break;
    case WifiTrafficPattern::STA_TO_AP:
        m_sourceMac = m_staMacs[0];
        destAddr = m_apMac->GetDevice()->GetAddress();
        break;
    case WifiTrafficPattern::AP_TO_STA:
        m_sourceMac = m_apMac;
        destAddr = m_staMacs[1]->GetDevice()->GetAddress();
        break;
    case WifiTrafficPattern::AP_TO_BCAST:
        m_sourceMac = m_apMac;
        destAddr = Mac48Address::GetBroadcast();
        break;
    case WifiTrafficPattern::STA_TO_BCAST:
        m_sourceMac = m_staMacs[0];
        destAddr = Mac48Address::GetBroadcast();
        break;
    }

    PacketSocketAddress sockAddr;
    sockAddr.SetSingleDevice(m_sourceMac->GetDevice()->GetIfIndex());
    sockAddr.SetPhysicalAddress(destAddr);
    sockAddr.SetProtocol(1);

    // install first client application generating at most 4 packets
    m_sourceMac->GetDevice()->GetNode()->AddApplication(
        GetApplication(sockAddr, std::min<std::size_t>(m_nPackets, 4), 1000));

    if (m_nPackets > 4)
    {
        // install a second client application generating the remaining packets and
        // starting during transmission of first A-MPDU, if multiple links are setup
        m_sourceMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(sockAddr, m_nPackets - 4, 1000, MilliSeconds(4)));
    }

    Simulator::Stop(m_duration);
}

void
MultiLinkTxTest::DoRun()
{
    Simulator::Run();

    // Expected number of packets received by each node (AP, STA 0, STA 1) at application layer
    std::array<std::size_t, 3> expectedRxPkts{};

    switch (m_trafficPattern)
    {
    case WifiTrafficPattern::STA_TO_STA:
    case WifiTrafficPattern::AP_TO_STA:
        // only STA 1 receives the m_nPackets packets that have been transmitted
        expectedRxPkts[2] = m_nPackets;
        break;
    case WifiTrafficPattern::STA_TO_AP:
        // only the AP receives the m_nPackets packets that have been transmitted
        expectedRxPkts[0] = m_nPackets;
        break;
    case WifiTrafficPattern::AP_TO_BCAST:
        // the AP replicates the broadcast frames on all the links, hence each station
        // receives the m_nPackets packets N times, where N is the number of setup link
        expectedRxPkts[1] = m_nPackets * m_staMacs[0]->GetSetupLinkIds().size();
        expectedRxPkts[2] = m_nPackets * m_staMacs[1]->GetSetupLinkIds().size();
        break;
    case WifiTrafficPattern::STA_TO_BCAST:
        // the AP receives the m_nPackets packets and then replicates them on all the links,
        // hence STA 1 receives m_nPackets packets N times, where N is the number of setup link
        expectedRxPkts[0] = m_nPackets;
        expectedRxPkts[2] = m_nPackets * m_staMacs[1]->GetSetupLinkIds().size();
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

    // check that the expected number of BlockAck frames are transmitted
    if (m_baEnabled && m_nMaxInflight == 1)
    {
        std::size_t expectedBaCount = 0;
        std::size_t expectedBarCount = 0;

        switch (m_trafficPattern)
        {
        case WifiTrafficPattern::STA_TO_AP:
        case WifiTrafficPattern::AP_TO_STA:
            // two A-MPDUs are transmitted and one BlockAck is corrupted
            expectedBaCount = 3;
            // one BlockAckReq is sent if m_useBarAfterMissedBa is true
            expectedBarCount = m_useBarAfterMissedBa ? 1 : 0;
            break;
        case WifiTrafficPattern::STA_TO_STA:
        case WifiTrafficPattern::STA_TO_BCAST:
            // only one A-MPDU is transmitted and the BlockAck is not corrupted
            expectedBaCount = 1;
            break;
        default:;
        }
        NS_TEST_EXPECT_MSG_EQ(m_blockAckCount,
                              expectedBaCount,
                              "Unexpected number of BlockAck frames");
        NS_TEST_EXPECT_MSG_EQ(m_blockAckReqCount,
                              expectedBarCount,
                              "Unexpected number of BlockAckReq frames");
    }

    // check that setting the QosTxop::NMaxInflights attribute has the expected effect.
    // We do not support sending an MPDU multiple times concurrently without Block Ack
    // agreement. Also, broadcast frames are already duplicated and sent on all links.
    if (m_baEnabled && m_trafficPattern != WifiTrafficPattern::AP_TO_BCAST)
    {
        NS_TEST_EXPECT_MSG_EQ(
            m_inflightCount.size(),
            m_nPackets / 2,
            "Did not collect number of simultaneous transmissions for all data frames");

        auto nMaxInflight = std::min(m_nMaxInflight, m_staMacs[0]->GetSetupLinkIds().size());
        std::size_t maxCount = 0;
        for (const auto& [seqNo, count] : m_inflightCount)
        {
            NS_TEST_EXPECT_MSG_LT_OR_EQ(
                count,
                nMaxInflight,
                "MPDU with seqNo=" << seqNo
                                   << " transmitted simultaneously more times than allowed");
            maxCount = std::max(maxCount, count);
        }

        NS_TEST_EXPECT_MSG_EQ(
            maxCount,
            nMaxInflight,
            "Expected that at least one data frame was transmitted simultaneously a number of "
            "times equal to the NMaxInflights attribute");
    }

    Simulator::Destroy();
}

/**
 * Tested MU traffic patterns.
 */
enum class WifiMuTrafficPattern : uint8_t
{
    DL_MU_BAR_BA_SEQUENCE = 0,
    DL_MU_MU_BAR,
    DL_MU_AGGR_MU_BAR,
    UL_MU
};

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test data transmission between MLDs using OFDMA MU transmissions
 *
 * This test sets up an AP MLD and two non-AP MLDs having a variable number of links.
 * The RF channels to set each link to are provided as input parameters through the test
 * case constructor, along with the identifiers (starting at 0) of the links that cannot
 * switch PHY band (if any). This test aims at veryfing the successful transmission of both
 * DL MU and UL MU frames. In the DL MU scenarios, the client applications installed on the
 * AP generate 8 packets addressed to each of the stations (plus 3 packets to trigger the
 * establishment of BlockAck agreements). In the UL MU scenario, client applications
 * installed on the stations generate 4 packets each (plus 3 packets to trigger the
 * establishment of BlockAck agreements).
 *
 * The maximum A-MSDU size is set such that two packets can be aggregated in an A-MSDU.
 * The MPDU with sequence number equal to 3 is corrupted (by using a post reception error
 * model) once and for a single station, to test its successful re-transmission.
 *
 * Also, we enable the concurrent transmission of data frames over two links and check that at
 * least one MPDU is concurrently transmitted over two links.
 */
class MultiLinkMuTxTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * \param baseParams common configuration parameters
     * \param muTrafficPattern the pattern of traffic to generate
     * \param useBarAfterMissedBa whether a BAR or Data frames are sent after missed BlockAck
     * \param nMaxInflight the max number of links on which an MPDU can be simultaneously inflight
     *                     (unused if Block Ack agreements are not established)
     */
    MultiLinkMuTxTest(const BaseParams& baseParams,
                      WifiMuTrafficPattern muTrafficPattern,
                      WifiUseBarAfterMissedBa useBarAfterMissedBa,
                      uint8_t nMaxInflight);
    ~MultiLinkMuTxTest() override = default;

  protected:
    /**
     * Check the content of a received BlockAck frame when the max number of links on which
     * an MPDU can be inflight is one.
     *
     * \param psdu the PSDU containing the BlockAck
     * \param txVector the TXVECTOR used to transmit the BlockAck
     * \param linkId the ID of the link on which the BlockAck was transmitted
     */
    void CheckBlockAck(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector, uint8_t linkId);

    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void DoSetup() override;
    void DoRun() override;

  private:
    void StartTraffic() override;

    /// Receiver address-indexed map of list error models
    using RxErrorModelMap = std::unordered_map<Mac48Address, Ptr<ListErrorModel>, WifiAddressHash>;

    /// A pair of a MAC address (the address of the receiver for DL frames and the address of
    /// the sender for UL frames) and a sequence number identifying a transmitted QoS data frame
    using AddrSeqNoPair = std::pair<Mac48Address, uint16_t>;

    RxErrorModelMap m_errorModels;                  ///< error rate models to corrupt packets
    std::list<uint64_t> m_uidList;                  ///< list of UIDs of packets to corrupt
    std::optional<Mac48Address> m_dataCorruptedSta; ///< MAC address of the station that received
                                                    ///< MPDU with SeqNo=2 corrupted
    bool m_waitFirstTf{true}; ///< whether we are waiting for the first Basic Trigger Frame
    WifiMuTrafficPattern m_muTrafficPattern; ///< the pattern of traffic to generate
    bool m_useBarAfterMissedBa;              ///< whether to send BAR after missed BlockAck
    std::size_t m_nMaxInflight; ///< max number of links on which an MPDU can be inflight
    std::vector<PacketSocketAddress> m_sockets; ///< packet socket addresses for STAs
    std::size_t m_nPackets;                     ///< number of application packets to generate
    std::size_t m_blockAckCount{0};             ///< transmitted BlockAck counter
    std::size_t m_tfCount{0};                   ///< transmitted Trigger Frame counter
    // std::size_t m_blockAckReqCount{0};     ///< transmitted BlockAckReq counter
    std::map<AddrSeqNoPair, std::size_t> m_inflightCount; ///< max number of simultaneous
                                                          ///< transmissions of each data frame
    Ptr<WifiMac> m_sourceMac; ///< MAC of the node sending application packets
};

MultiLinkMuTxTest::MultiLinkMuTxTest(const BaseParams& baseParams,
                                     WifiMuTrafficPattern muTrafficPattern,
                                     WifiUseBarAfterMissedBa useBarAfterMissedBa,
                                     uint8_t nMaxInflight)
    : MultiLinkOperationsTestBase(
          std::string("Check MU data transmission between MLDs ") +
              (useBarAfterMissedBa == WifiUseBarAfterMissedBa::YES
                   ? "(send BAR after BlockAck timeout,"
                   : "(send Data frames after BlockAck timeout,") +
              " MU Traffic pattern: " + std::to_string(static_cast<uint8_t>(muTrafficPattern)) +
              ", nMaxInflight=" + std::to_string(nMaxInflight) + ")",
          2,
          baseParams),
      m_muTrafficPattern(muTrafficPattern),
      m_useBarAfterMissedBa(useBarAfterMissedBa == WifiUseBarAfterMissedBa::YES),
      m_nMaxInflight(nMaxInflight),
      m_sockets(m_nStations),
      m_nPackets(muTrafficPattern == WifiMuTrafficPattern::UL_MU ? 4 : 8)
{
}

void
MultiLinkMuTxTest::Transmit(Ptr<WifiMac> mac,
                            uint8_t phyId,
                            WifiConstPsduMap psduMap,
                            WifiTxVector txVector,
                            double txPowerW)
{
    MultiLinkOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);
    auto linkId = m_txPsdus.back().linkId;

    CtrlTriggerHeader trigger;

    for (const auto& [staId, psdu] : psduMap)
    {
        switch (psdu->GetHeader(0).GetType())
        {
        case WIFI_MAC_QOSDATA:
            CheckAddresses(psdu);
            if (psdu->GetHeader(0).HasData())
            {
                bool isDl = psdu->GetHeader(0).IsFromDs();
                auto linkAddress =
                    isDl ? psdu->GetHeader(0).GetAddr1() : psdu->GetHeader(0).GetAddr2();
                auto address = m_apMac->GetMldAddress(linkAddress).value_or(linkAddress);

                for (const auto& mpdu : *psdu)
                {
                    // determine the max number of simultaneous transmissions for this MPDU
                    auto seqNo = mpdu->GetHeader().GetSequenceNumber();
                    auto [it, success] = m_inflightCount.insert(
                        {{address, seqNo}, mpdu->GetInFlightLinkIds().size()});
                    if (!success)
                    {
                        it->second = std::max(it->second, mpdu->GetInFlightLinkIds().size());
                    }
                }
                for (std::size_t i = 0; i < psdu->GetNMpdus(); i++)
                {
                    // MPDUs with seqNo=2 are always transmitted in an MU PPDU
                    if (psdu->GetHeader(i).GetSequenceNumber() == 2)
                    {
                        if (m_muTrafficPattern == WifiMuTrafficPattern::UL_MU)
                        {
                            NS_TEST_EXPECT_MSG_EQ(txVector.IsUlMu(),
                                                  true,
                                                  "MPDU " << **std::next(psdu->begin(), i)
                                                          << " not transmitted in a TB PPDU");
                        }
                        else
                        {
                            NS_TEST_EXPECT_MSG_EQ(txVector.GetHeMuUserInfoMap().size(),
                                                  2,
                                                  "MPDU " << **std::next(psdu->begin(), i)
                                                          << " not transmitted in a DL MU PPDU");
                        }
                    }
                    // corrupt QoS data frame with sequence number equal to 3 (only once)
                    if (psdu->GetHeader(i).GetSequenceNumber() != 3)
                    {
                        continue;
                    }
                    auto uid = psdu->GetPayload(i)->GetUid();
                    if (!m_dataCorruptedSta)
                    {
                        m_uidList.push_front(uid);
                        m_dataCorruptedSta = isDl ? psdu->GetAddr1() : psdu->GetAddr2();
                        NS_LOG_INFO("CORRUPTED");
                        m_errorModels.at(psdu->GetAddr1())->SetList(m_uidList);
                    }
                    else if ((isDl && m_dataCorruptedSta == psdu->GetAddr1()) ||
                             (!isDl && m_dataCorruptedSta == psdu->GetAddr2()))
                    {
                        // do not corrupt the QoS data frame anymore
                        if (auto it = std::find(m_uidList.cbegin(), m_uidList.cend(), uid);
                            it != m_uidList.cend())
                        {
                            m_uidList.erase(it);
                        }
                        m_errorModels.at(psdu->GetAddr1())->SetList(m_uidList);
                    }
                    break;
                }
            }
            break;
        case WIFI_MAC_CTL_BACKRESP:
            if (m_nMaxInflight > 1)
            {
                // we do not check the content of BlockAck when m_nMaxInflight is greater than 1
                break;
            }
            CheckBlockAck(psdu, txVector, linkId);
            m_blockAckCount++;
            // to simulate a missed BlockAck, corrupt the fifth BlockAck frame (the first
            // two BlockAck frames are sent to acknowledge the QoS data frames that triggered
            // the establishment of Block Ack agreements)
            if (m_blockAckCount == 5)
            {
                // corrupt the third BlockAck frame to simulate a missed BlockAck
                m_uidList.push_front(psdu->GetPacket()->GetUid());
                NS_LOG_INFO("CORRUPTED");
                m_errorModels.at(psdu->GetAddr1())->SetList(m_uidList);
            }
            break;
        case WIFI_MAC_CTL_TRIGGER:
            psdu->GetPayload(0)->PeekHeader(trigger);
            // the MU scheduler requests channel access on all links but we have to perform the
            // following actions only once (hence why we only consider TF transmitted on link 0)
            if (trigger.IsBasic() && m_waitFirstTf)
            {
                m_waitFirstTf = false;
                // the AP is starting the transmission of the Basic Trigger frame, so generate
                // the configured number of packets at STAs, which are sent in TB PPDUs, when
                // transmission of the Trigger Frame ends
                auto band = mac->GetWifiPhy(linkId)->GetPhyBand();
                Time txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, band);
                for (uint8_t i = 0; i < m_nStations; i++)
                {
                    m_staMacs[i]->GetDevice()->GetNode()->AddApplication(
                        GetApplication(m_sockets[i], m_nPackets, 450, txDuration, i * 4));
                }
            }
            if (++m_tfCount == m_staMacs[0]->GetSetupLinkIds().size())
            {
                // a TF has been sent on all the setup links, we can now disable UL OFDMA
                auto muScheduler = m_apMac->GetObject<MultiUserScheduler>();
                NS_TEST_ASSERT_MSG_NE(muScheduler, nullptr, "Expected an aggregated MU scheduler");
                muScheduler->SetAttribute("EnableUlOfdma", BooleanValue(false));
            }
            break;
        default:;
        }
    }
}

void
MultiLinkMuTxTest::CheckBlockAck(Ptr<const WifiPsdu> psdu,
                                 const WifiTxVector& txVector,
                                 uint8_t linkId)
{
    /*
     * Example sequence with DL_MU_BAR_BA_SEQUENCE
     *                  ┌───────┬───────X
     *           (To:1) │   2   │   3   │
     *                  ├───────┼───────┤   ┌───┐             ┌───────┐
     *  [link 0] (To:0) │   2   │   3   │   │BAR│      (To:1) │   3   │
     *  ────────────────┴───────┴───────┴┬──┼───┼──┬──────────┴───────┴┬───┬────────
     *                                   │BA│   │BA│                   │ACK│
     *                                   └──┘   └──┘                   └───┘
     *                              ┌───────┬───────┐
     *                       (To:1) │   4   │   5   │
     *                              ├───────┼───────┤        ┌───┐   ┌───┐
     *  [link 1]             (To:0) │   4   │   5   │        │BAR│   │BAR│
     *  ────────────────────────────┴───────┴───────┴┬──X────┴───┴┬──┼───┼──┬───────
     *                                               │BA│         │BA│   │BA│
     *                                               └──┘         └──┘   └──┘
     *
     * Example sequence with UL_MU
     *
     *           ┌──┐                     ┌────┐                      ┌───┐
     *  [link 0] │TF│                     │M-BA│                      │ACK│
     *  ─────────┴──┴──┬───────┬───────┬──┴────┴────────────┬───────┬─┴───┴─────────
     *        (From:0) │   2   │   3   │           (From:1) │   3   │
     *                 ├───────┼───────┤                    └───────┘
     *        (From:1) │   2   │   3   │
     *                 └───────┴───────X
     *           ┌──┐
     *  [link 1] │TF│
     *  ─────────┴──┴──┬───────────────┬────────────────────────────────────────────
     *        (From:0) │   QoS Null    │
     *                 ├───────────────┤
     *        (From:1) │   QoS Null    │
     *                 └───────────────┘
     */
    auto mpdu = *psdu->begin();
    CtrlBAckResponseHeader blockAck;
    mpdu->GetPacket()->PeekHeader(blockAck);
    bool isMpdu3corrupted;

    switch (m_blockAckCount)
    {
    case 0:
    case 1: // Ignore the first two BlockAck frames that acknowledged frames sent to establish BA
        break;
    case 2:
        if (m_muTrafficPattern == WifiMuTrafficPattern::UL_MU)
        {
            NS_TEST_EXPECT_MSG_EQ(blockAck.IsMultiSta(), true, "Expected a Multi-STA BlockAck");
            for (uint8_t i = 0; i < m_nStations; i++)
            {
                auto indices = blockAck.FindPerAidTidInfoWithAid(m_staMacs[i]->GetAssociationId());
                NS_TEST_ASSERT_MSG_EQ(indices.size(), 1, "Expected one Per AID TID Info per STA");
                auto index = indices.front();
                NS_TEST_ASSERT_MSG_EQ(m_dataCorruptedSta.has_value(),
                                      true,
                                      "Expected that a QoS data frame was corrupted");
                isMpdu3corrupted =
                    m_staMacs[i]->GetLinkIdByAddress(*m_dataCorruptedSta).has_value();
                NS_TEST_EXPECT_MSG_EQ(blockAck.IsPacketReceived(2, index),
                                      true,
                                      "MPDU 2 expected to be successfully received");
                NS_TEST_EXPECT_MSG_EQ(blockAck.IsPacketReceived(3, index),
                                      !isMpdu3corrupted,
                                      "Unexpected reception status for MPDU 3");
            }

            break;
        }
    case 3:
        // BlockAck frames in response to the first DL MU PPDU
        isMpdu3corrupted = (mpdu->GetHeader().GetAddr2() == m_dataCorruptedSta);
        NS_TEST_EXPECT_MSG_EQ(blockAck.IsPacketReceived(2),
                              true,
                              "MPDU 2 expected to be successfully received");
        NS_TEST_EXPECT_MSG_EQ(blockAck.IsPacketReceived(3),
                              !isMpdu3corrupted,
                              "Unexpected reception status for MPDU 3");
        // in case of DL MU, if there are at least two links setup, we expect all MPDUs to
        // be inflight (on distinct links)
        if (m_muTrafficPattern != WifiMuTrafficPattern::UL_MU &&
            m_staMacs[0]->GetSetupLinkIds().size() > 1)
        {
            auto queue = m_apMac->GetTxopQueue(AC_BE);
            Ptr<StaWifiMac> rcvMac;
            if (m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress() ==
                mpdu->GetHeader().GetAddr2())
            {
                rcvMac = m_staMacs[0];
            }
            else if (m_staMacs[1]->GetFrameExchangeManager(linkId)->GetAddress() ==
                     mpdu->GetHeader().GetAddr2())
            {
                rcvMac = m_staMacs[1];
            }
            else
            {
                NS_ABORT_MSG("BlockAck frame not sent by a station in DL scenario");
            }
            auto item = queue->PeekByTidAndAddress(0, rcvMac->GetAddress());
            std::size_t nQueuedPkt = 0;
            auto delay = WifiPhy::CalculateTxDuration(psdu,
                                                      txVector,
                                                      rcvMac->GetWifiPhy(linkId)->GetPhyBand()) +
                         MicroSeconds(1); // to account for propagation delay

            while (item)
            {
                auto seqNo = item->GetHeader().GetSequenceNumber();
                NS_TEST_EXPECT_MSG_EQ(item->IsInFlight(),
                                      true,
                                      "MPDU with seqNo=" << seqNo << " is not in flight");
                auto linkIds = item->GetInFlightLinkIds();
                NS_TEST_EXPECT_MSG_EQ(linkIds.size(),
                                      1,
                                      "MPDU with seqNo=" << seqNo
                                                         << " is in flight on multiple links");
                // The first two MPDUs are in flight on the same link on which the BlockAck
                // is sent. The other two MPDUs (only for AP to STA/STA to AP scenarios) are
                // in flight on a different link.
                auto srcLinkId = m_apMac->GetLinkIdByAddress(mpdu->GetHeader().GetAddr1());
                NS_TEST_ASSERT_MSG_EQ(srcLinkId.has_value(),
                                      true,
                                      "Addr1 of BlockAck is not an originator's link address");
                NS_TEST_EXPECT_MSG_EQ((*linkIds.begin() == *srcLinkId),
                                      (seqNo <= 3),
                                      "MPDU with seqNo=" << seqNo
                                                         << " in flight on unexpected link");
                // check the Retry subfield and whether this MPDU is still queued
                // after the originator has processed this BlockAck

                // MPDUs acknowledged via this BlockAck are no longer queued
                bool isQueued = (seqNo > (isMpdu3corrupted ? 2 : 3));
                // The Retry subfield is set if the MPDU has not been acknowledged (i.e., it
                // is still queued) and has been transmitted on the same link as the BlockAck
                // (i.e., its sequence number is less than or equal to 2)
                bool isRetry = isQueued && seqNo <= 3;

                Simulator::Schedule(delay, [this, item, isQueued, isRetry]() {
                    NS_TEST_EXPECT_MSG_EQ(item->IsQueued(),
                                          isQueued,
                                          "MPDU with seqNo="
                                              << item->GetHeader().GetSequenceNumber() << " should "
                                              << (isQueued ? "" : "not") << " be queued");
                    NS_TEST_EXPECT_MSG_EQ(
                        item->GetHeader().IsRetry(),
                        isRetry,
                        "Unexpected value for the Retry subfield of the MPDU with seqNo="
                            << item->GetHeader().GetSequenceNumber());
                });

                nQueuedPkt++;
                item = queue->PeekByTidAndAddress(0, rcvMac->GetAddress(), item);
            }
            // Each MPDU contains an A-MSDU consisting of two MSDUs
            NS_TEST_EXPECT_MSG_EQ(nQueuedPkt, m_nPackets / 2, "Unexpected number of queued MPDUs");
        }
        break;
    }
}

void
MultiLinkMuTxTest::DoSetup()
{
    switch (m_muTrafficPattern)
    {
    case WifiMuTrafficPattern::DL_MU_BAR_BA_SEQUENCE:
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
        break;
    case WifiMuTrafficPattern::DL_MU_MU_BAR:
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_TF_MU_BAR));
        break;
    case WifiMuTrafficPattern::DL_MU_AGGR_MU_BAR:
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
        break;
    default:;
    }

    MultiLinkOperationsTestBase::DoSetup();

    // Enable A-MSDU aggregation. Max A-MSDU size is set such that two MSDUs can be aggregated
    for (auto mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMacs[0], m_staMacs[1]})
    {
        mac->SetAttribute("BE_MaxAmsduSize", UintegerValue(1050));
        mac->GetQosTxop(AC_BE)->SetAttribute("UseExplicitBarAfterMissedBlockAck",
                                             BooleanValue(m_useBarAfterMissedBa));
        mac->GetQosTxop(AC_BE)->SetAttribute("NMaxInflights", UintegerValue(m_nMaxInflight));

        mac->SetAttribute("VI_MaxAmsduSize", UintegerValue(1050));
        mac->GetQosTxop(AC_VI)->SetAttribute("UseExplicitBarAfterMissedBlockAck",
                                             BooleanValue(m_useBarAfterMissedBa));
        mac->GetQosTxop(AC_VI)->SetAttribute("NMaxInflights", UintegerValue(m_nMaxInflight));
    }

    // aggregate MU scheduler
    auto muScheduler = CreateObjectWithAttributes<RrMultiUserScheduler>("EnableUlOfdma",
                                                                        BooleanValue(false),
                                                                        "EnableBsrp",
                                                                        BooleanValue(false),
                                                                        "UlPsduSize",
                                                                        UintegerValue(2000));
    m_apMac->AggregateObject(muScheduler);

    // install post reception error model on all devices
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        auto errorModel = CreateObject<ListErrorModel>();
        m_errorModels[m_apMac->GetFrameExchangeManager(linkId)->GetAddress()] = errorModel;
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(errorModel);
    }
    for (std::size_t i : {0, 1})
    {
        for (const auto linkId : m_staMacs[i]->GetLinkIds())
        {
            auto errorModel = CreateObject<ListErrorModel>();
            m_errorModels[m_staMacs[i]->GetFrameExchangeManager(linkId)->GetAddress()] = errorModel;
            m_staMacs[i]->GetWifiPhy(linkId)->SetPostReceptionErrorModel(errorModel);
        }
    }
}

void
MultiLinkMuTxTest::StartTraffic()
{
    if (m_muTrafficPattern < WifiMuTrafficPattern::UL_MU)
    {
        // DL Traffic
        for (uint8_t i = 0; i < m_nStations; i++)
        {
            PacketSocketAddress sockAddr;
            sockAddr.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
            sockAddr.SetPhysicalAddress(m_staMacs[i]->GetDevice()->GetAddress());
            sockAddr.SetProtocol(1);

            // the first client application generates three packets in order
            // to trigger the establishment of a Block Ack agreement
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(sockAddr, 3, 450, i * MilliSeconds(50)));

            // the second client application generates the first half of the selected number
            // of packets, which are sent in DL MU PPDUs, and starts after all BA agreements
            // are established
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(sockAddr, m_nPackets / 2, 450, m_nStations * MilliSeconds(50)));

            // the third client application generates the second half of the selected number
            // of packets, which are sent in DL MU PPDUs, and starts during transmission of
            // first A-MPDU, if multiple links are setup
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(sockAddr,
                               m_nPackets / 2,
                               450,
                               m_nStations * MilliSeconds(50) + MilliSeconds(3)));
        }
    }
    else
    {
        // UL Traffic
        for (uint8_t i = 0; i < m_nStations; i++)
        {
            m_sockets[i].SetSingleDevice(m_staMacs[i]->GetDevice()->GetIfIndex());
            m_sockets[i].SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
            m_sockets[i].SetProtocol(1);

            // the first client application generates three packets in order
            // to trigger the establishment of a Block Ack agreement
            m_staMacs[i]->GetDevice()->GetNode()->AddApplication(
                GetApplication(m_sockets[i], 3, 450, i * MilliSeconds(50), i * 4));

            // packets to be included in TB PPDUs are generated (by Transmit()) when
            // the first Basic Trigger Frame is sent by the AP
        }

        // MU scheduler starts requesting channel access when we are done with BA agreements
        Simulator::Schedule(m_nStations * MilliSeconds(50), [this]() {
            auto muScheduler = m_apMac->GetObject<MultiUserScheduler>();
            NS_TEST_ASSERT_MSG_NE(muScheduler, nullptr, "Expected an aggregated MU scheduler");
            muScheduler->SetAttribute("EnableUlOfdma", BooleanValue(true));
            muScheduler->SetAccessReqInterval(MilliSeconds(3));
            // channel access is requested only once
            muScheduler->SetAccessReqInterval(Seconds(0));
        });
    }

    Simulator::Stop(m_duration);
}

void
MultiLinkMuTxTest::DoRun()
{
    Simulator::Run();

    // Expected number of packets received by each node (AP, STA 0, STA 1) at application layer
    std::array<std::size_t, 3> expectedRxPkts{};

    switch (m_muTrafficPattern)
    {
    case WifiMuTrafficPattern::DL_MU_BAR_BA_SEQUENCE:
    case WifiMuTrafficPattern::DL_MU_MU_BAR:
    case WifiMuTrafficPattern::DL_MU_AGGR_MU_BAR:
        // both STA 0 and STA 1 receive m_nPackets + 3 (sent to trigger BA establishment) packets
        expectedRxPkts[1] = m_nPackets + 3;
        expectedRxPkts[2] = m_nPackets + 3;
        break;
    case WifiMuTrafficPattern::UL_MU:
        // AP receives m_nPackets + 3 (sent to trigger BA establishment) packets from each station
        expectedRxPkts[0] = 2 * (m_nPackets + 3);
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

    // check that setting the QosTxop::NMaxInflights attribute has the expected effect.
    // For DL, for each station we send 2 MPDUs to trigger BA agreement and m_nPackets / 2 MPDUs
    // For UL, each station sends 2 MPDUs to trigger BA agreement and m_nPackets / 2 MPDUs
    NS_TEST_EXPECT_MSG_EQ(
        m_inflightCount.size(),
        2 * (2 + m_nPackets / 2),
        "Did not collect number of simultaneous transmissions for all data frames");

    auto nMaxInflight = std::min(m_nMaxInflight, m_staMacs[0]->GetSetupLinkIds().size());
    std::size_t maxCount = 0;
    for (const auto& [txSeqNoPair, count] : m_inflightCount)
    {
        NS_TEST_EXPECT_MSG_LT_OR_EQ(count,
                                    nMaxInflight,
                                    "MPDU with seqNo="
                                        << txSeqNoPair.second
                                        << " transmitted simultaneously more times than allowed");
        maxCount = std::max(maxCount, count);
    }

    NS_TEST_EXPECT_MSG_EQ(
        maxCount,
        nMaxInflight,
        "Expected that at least one data frame was transmitted simultaneously a number of "
        "times equal to the NMaxInflights attribute");

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test release of sequence numbers upon CTS timeout in multi-link operations
 *
 * In this test, an AP MLD and a non-AP MLD setup 3 links. Usage of RTS/CTS protection is
 * enabled for frames whose length is at least 1000 bytes. The AP MLD receives a first set
 * of 4 packets from the upper layer and sends an RTS frame, which is corrupted at the
 * receiver, on a first link. When the RTS frame is transmitted, the AP MLD receives another
 * set of 4 packets, which are transmitted after a successful RTS/CTS exchange on a second
 * link. In the meantime, a new RTS/CTS exchange is successfully carried out (on the first
 * link or on the third link) to transmit the first set of 4 packets. When the transmission
 * of the first set of 4 packets starts, the AP MLD receives the third set of 4 packets from
 * the upper layer, which are transmitted after a successful RTS/CTS exchange.
 *
 * This test checks that sequence numbers are correctly assigned to all the MPDUs carrying data.
 */
class ReleaseSeqNoAfterCtsTimeoutTest : public MultiLinkOperationsTestBase
{
  public:
    ReleaseSeqNoAfterCtsTimeoutTest();
    ~ReleaseSeqNoAfterCtsTimeoutTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

  private:
    void StartTraffic() override;

    PacketSocketAddress m_sockAddr;   //!< packet socket address
    std::size_t m_nQosDataFrames;     //!< counter for transmitted QoS data frames
    Ptr<ListErrorModel> m_errorModel; //!< error rate model to corrupt first RTS frame
    bool m_rtsCorrupted;              //!< whether the first RTS frame has been corrupted
};

ReleaseSeqNoAfterCtsTimeoutTest::ReleaseSeqNoAfterCtsTimeoutTest()
    : MultiLinkOperationsTestBase(
          "Check sequence numbers after CTS timeout",
          1,
          BaseParams{{"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                     {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                     {}}),
      m_nQosDataFrames(0),
      m_errorModel(CreateObject<ListErrorModel>()),
      m_rtsCorrupted(false)
{
}

void
ReleaseSeqNoAfterCtsTimeoutTest::DoSetup()
{
    // Enable RTS/CTS
    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("1000"));

    MultiLinkOperationsTestBase::DoSetup();

    // install post reception error model on all STAs affiliated with non-AP MLD
    for (const auto linkId : m_staMacs[0]->GetLinkIds())
    {
        m_staMacs[0]->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }
}

void
ReleaseSeqNoAfterCtsTimeoutTest::StartTraffic()
{
    m_sockAddr.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
    m_sockAddr.SetPhysicalAddress(m_staMacs[0]->GetAddress());
    m_sockAddr.SetProtocol(1);

    // install client application generating 4 packets
    m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(m_sockAddr, 4, 1000));
}

void
ReleaseSeqNoAfterCtsTimeoutTest::Transmit(Ptr<WifiMac> mac,
                                          uint8_t phyId,
                                          WifiConstPsduMap psduMap,
                                          WifiTxVector txVector,
                                          double txPowerW)
{
    MultiLinkOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    auto psdu = psduMap.begin()->second;

    if (psdu->GetHeader(0).IsRts() && !m_rtsCorrupted)
    {
        m_errorModel->SetList({psdu->GetPacket()->GetUid()});
        m_rtsCorrupted = true;
        // generate other packets when the first RTS is transmitted
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(m_sockAddr, 4, 1000));
    }
    else if (psdu->GetHeader(0).IsQosData())
    {
        m_nQosDataFrames++;

        if (m_nQosDataFrames == 2)
        {
            // generate other packets when the second QoS data frame is transmitted
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(m_sockAddr, 4, 1000));
        }
    }
}

void
ReleaseSeqNoAfterCtsTimeoutTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_nQosDataFrames, 3, "Unexpected number of transmitted QoS data frames");

    std::size_t count{};

    for (const auto& txPsdu : m_txPsdus)
    {
        auto psdu = txPsdu.psduMap.begin()->second;

        if (!psdu->GetHeader(0).IsQosData())
        {
            continue;
        }

        NS_TEST_EXPECT_MSG_EQ(psdu->GetNMpdus(), 4, "Unexpected number of MPDUs in A-MPDU");

        count++;
        uint16_t expectedSeqNo{};

        switch (count)
        {
        case 1:
            expectedSeqNo = 4;
            break;
        case 2:
            expectedSeqNo = 0;
            break;
        case 3:
            expectedSeqNo = 8;
            break;
        }

        for (const auto& mpdu : *PeekPointer(psdu))
        {
            NS_TEST_EXPECT_MSG_EQ(mpdu->GetHeader().GetSequenceNumber(),
                                  expectedSeqNo++,
                                  "Unexpected sequence number");
        }
    }

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
    : TestSuite("wifi-mlo", Type::UNIT)
{
    using ParamsTuple = std::tuple<MultiLinkOperationsTestBase::BaseParams, // base config params
                                   std::vector<uint8_t>,           // link ID of setup links
                                   WifiTidToLinkMappingNegSupport, // AP negotiation support
                                   std::string,                    // DL TID-to-Link Mapping
                                   std::string>;                   // UL TID-to-Link Mapping

    AddTestCase(new GetRnrLinkInfoTest(), TestCase::Duration::QUICK);
    AddTestCase(new MldSwapLinksTest(), TestCase::Duration::QUICK);

    for (const auto& [baseParams,
                      setupLinks,
                      apNegSupport,
                      dlTidLinkMapping,
                      ulTidLinkMapping] :
         {// matching channels: setup all links
          ParamsTuple(
              {{"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
               {"{36, 0, BAND_5GHZ, 0}", "{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
               {}},
              {0, 1, 2},
              WifiTidToLinkMappingNegSupport::NOT_SUPPORTED, // AP MLD does not support TID-to-Link
                                                             // Mapping negotiation
              "0,1,2,3  0,1,2;  4,5  0,1",                   // default mapping used instead
              "0,1,2,3  1,2;    6,7  0,1"                    // default mapping used instead
              ),
          // non-matching channels, matching PHY bands: setup all links
          ParamsTuple({{"{108, 0, BAND_5GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                       {"{36, 0, BAND_5GHZ, 0}", "{120, 0, BAND_5GHZ, 0}", "{5, 0, BAND_6GHZ, 0}"},
                       {}},
                      {0, 1, 2},
                      WifiTidToLinkMappingNegSupport::SAME_LINK_SET, // AP MLD does not support
                                                                     // distinct link sets for TIDs
                      "0,1,2,3  0,1,2;  4,5  0,1",                   // default mapping used instead
                      ""),
          // non-AP MLD switches band on some links to setup 3 links
          ParamsTuple({{"{2, 0, BAND_2_4GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                       {"{36, 0, BAND_5GHZ, 0}", "{9, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                       {}},
                      {0, 1, 2},
                      WifiTidToLinkMappingNegSupport::ANY_LINK_SET,
                      "0,1,2,3  0;  4,5,6,7  1,2", // frames of two TIDs are generated
                      "0,2,3  1,2;  1,4,5,6,7  0"  // frames of two TIDs are generated
                      ),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band, hence only 2 links are setup
          ParamsTuple(
              {{"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{8, 20, BAND_2_4GHZ, 0}"},
               {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
               {0}},
              {0, 1},
              WifiTidToLinkMappingNegSupport::SAME_LINK_SET, // AP MLD does not support distinct
                                                             // link sets for TIDs
              "0,1,2,3,4,5,6,7  0",
              "0,1,2,3,4,5,6,7  0"),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band; the second link of the non-AP MLD cannot change PHY band and there is
          // an AP operating on the same channel; hence 2 links are setup
          ParamsTuple(
              {{"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{8, 20, BAND_2_4GHZ, 0}"},
               {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
               {0, 1}},
              {0, 1},
              WifiTidToLinkMappingNegSupport::ANY_LINK_SET,
              "0,1,2,3  1",
              "0,1,2,3  1"),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band; the second link of the non-AP MLD cannot change PHY band and there is
          // an AP operating on the same channel; the third link of the non-AP MLD cannot
          // change PHY band and there is an AP operating on the same band (different channel);
          // hence 2 links are setup by switching channel (not band) on the third link
          ParamsTuple({{"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}", "{60, 0, BAND_5GHZ, 0}"},
                       {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                       {0, 1, 2}},
                      {0, 2},
                      WifiTidToLinkMappingNegSupport::ANY_LINK_SET,
                      "",
                      ""),
          // the first link of the non-AP MLD cannot change PHY band and no AP is operating on
          // that band; the second link of the non-AP MLD cannot change PHY band and there is
          // an AP operating on the same channel; hence one link only is setup
          ParamsTuple({{"{2, 0, BAND_2_4GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                       {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                       {0, 1}},
                      {2},
                      WifiTidToLinkMappingNegSupport::ANY_LINK_SET,
                      "",
                      ""),
          // non-AP MLD has only two STAs and setups two links
          ParamsTuple({{"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                       {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                       {}},
                      {1, 0},
                      WifiTidToLinkMappingNegSupport::ANY_LINK_SET,
                      "0,1,2,3  1",
                      ""),
          // single link non-AP STA associates with an AP affiliated with an AP MLD
          ParamsTuple({{"{120, 0, BAND_5GHZ, 0}"},
                       {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                       {}},
                      {2}, // link ID of AP MLD only (non-AP STA is single link)
                      WifiTidToLinkMappingNegSupport::ANY_LINK_SET,
                      "",
                      ""),
          // a STA affiliated with a non-AP MLD associates with a single link AP
          ParamsTuple({{"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}", "{120, 0, BAND_5GHZ, 0}"},
                       {"{120, 0, BAND_5GHZ, 0}"},
                       {}},
                      {2}, // link ID of non-AP MLD only (AP is single link)
                      WifiTidToLinkMappingNegSupport::NOT_SUPPORTED,
                      "0,1,2,3  0,1;  4,5,6,7  0,1", // ignored by single link AP
                      "")})
    {
        AddTestCase(new MultiLinkSetupTest(baseParams,
                                           WifiScanType::PASSIVE,
                                           setupLinks,
                                           apNegSupport,
                                           dlTidLinkMapping,
                                           ulTidLinkMapping),
                    TestCase::Duration::QUICK);
        AddTestCase(new MultiLinkSetupTest(baseParams,
                                           WifiScanType::ACTIVE,
                                           setupLinks,
                                           apNegSupport,
                                           dlTidLinkMapping,
                                           ulTidLinkMapping),
                    TestCase::Duration::QUICK);

        for (const auto& trafficPattern : {WifiTrafficPattern::STA_TO_STA,
                                           WifiTrafficPattern::STA_TO_AP,
                                           WifiTrafficPattern::AP_TO_STA,
                                           WifiTrafficPattern::AP_TO_BCAST,
                                           WifiTrafficPattern::STA_TO_BCAST})
        {
            // No Block Ack agreement
            AddTestCase(new MultiLinkTxTest(baseParams,
                                            trafficPattern,
                                            WifiBaEnabled::NO,
                                            WifiUseBarAfterMissedBa::NO,
                                            1),
                        TestCase::Duration::QUICK);
            for (const auto& useBarAfterMissedBa :
                 {WifiUseBarAfterMissedBa::YES, WifiUseBarAfterMissedBa::NO})
            {
                // Block Ack agreement with nMaxInflight=1
                AddTestCase(new MultiLinkTxTest(baseParams,
                                                trafficPattern,
                                                WifiBaEnabled::YES,
                                                useBarAfterMissedBa,
                                                1),
                            TestCase::Duration::QUICK);
                // Block Ack agreement with nMaxInflight=2
                AddTestCase(new MultiLinkTxTest(baseParams,
                                                trafficPattern,
                                                WifiBaEnabled::YES,
                                                useBarAfterMissedBa,
                                                2),
                            TestCase::Duration::QUICK);
            }
        }

        for (const auto& muTrafficPattern : {WifiMuTrafficPattern::DL_MU_BAR_BA_SEQUENCE,
                                             WifiMuTrafficPattern::DL_MU_MU_BAR,
                                             WifiMuTrafficPattern::DL_MU_AGGR_MU_BAR,
                                             WifiMuTrafficPattern::UL_MU})
        {
            for (const auto& useBarAfterMissedBa :
                 {WifiUseBarAfterMissedBa::YES, WifiUseBarAfterMissedBa::NO})
            {
                // Block Ack agreement with nMaxInflight=1
                AddTestCase(
                    new MultiLinkMuTxTest(baseParams, muTrafficPattern, useBarAfterMissedBa, 1),
                    TestCase::Duration::QUICK);
                // Block Ack agreement with nMaxInflight=2
                AddTestCase(
                    new MultiLinkMuTxTest(baseParams, muTrafficPattern, useBarAfterMissedBa, 2),
                    TestCase::Duration::QUICK);
            }
        }
    }

    AddTestCase(new ReleaseSeqNoAfterCtsTimeoutTest(), TestCase::Duration::QUICK);
}

static WifiMultiLinkOperationsTestSuite g_wifiMultiLinkOperationsTestSuite; ///< the test suite
