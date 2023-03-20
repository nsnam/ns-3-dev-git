/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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
#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/ctrl-headers.h"
#include "ns3/eht-configuration.h"
#include "ns3/emlsr-manager.h"
#include "ns3/he-frame-exchange-manager.h"
#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/rr-multi-user-scheduler.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <optional>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrTest");

static uint32_t
ContextToNodeId(std::string context)
{
    std::string sub = context.substr(10);
    uint32_t pos = sub.find("/Device");
    uint32_t nodeId = std::stoi(sub.substr(0, pos));
    return nodeId;
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test EML Operating Mode Notification frame serialization and deserialization
 */
class EmlOperatingModeNotificationTest : public HeaderSerializationTestCase
{
  public:
    /**
     * Constructor
     */
    EmlOperatingModeNotificationTest();
    ~EmlOperatingModeNotificationTest() override = default;

  private:
    void DoRun() override;
};

EmlOperatingModeNotificationTest::EmlOperatingModeNotificationTest()
    : HeaderSerializationTestCase(
          "Check serialization and deserialization of the EML Operating Mode Notification frame")
{
}

void
EmlOperatingModeNotificationTest::DoRun()
{
    MgtEmlOperatingModeNotification frame;

    // Both EMLSR Mode and EMLMR Mode subfields set to 0 (no link bitmap);
    TestHeaderSerialization(frame);

    frame.m_emlControl.emlsrMode = 1;
    frame.SetLinkIdInBitmap(0);
    frame.SetLinkIdInBitmap(5);
    frame.SetLinkIdInBitmap(15);

    // Adding Link Bitmap
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ((frame.GetLinkBitmap() == std::list<uint8_t>{0, 5, 15}),
                          true,
                          "Unexpected link bitmap");

    auto padding = MicroSeconds(64);
    auto transition = MicroSeconds(128);

    frame.m_emlControl.emlsrParamUpdateCtrl = 1;
    frame.m_emlsrParamUpdate = MgtEmlOperatingModeNotification::EmlsrParamUpdate{};
    frame.m_emlsrParamUpdate->paddingDelay = CommonInfoBasicMle::EncodeEmlsrPaddingDelay(padding);
    frame.m_emlsrParamUpdate->transitionDelay =
        CommonInfoBasicMle::EncodeEmlsrTransitionDelay(transition);

    // Adding the EMLSR Parameter Update field
    TestHeaderSerialization(frame);

    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrPaddingDelay(frame.m_emlsrParamUpdate->paddingDelay),
        padding,
        "Unexpected EMLSR Padding Delay");
    NS_TEST_EXPECT_MSG_EQ(
        CommonInfoBasicMle::DecodeEmlsrTransitionDelay(frame.m_emlsrParamUpdate->transitionDelay),
        transition,
        "Unexpected EMLSR Transition Delay");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Base class for EMLSR Operations tests
 *
 * This base class setups and configures one AP MLD, a variable number of non-AP MLDs with
 * EMLSR activated and a variable number of non-AP MLD with EMLSR deactivated. Every MLD has
 * three links, each operating on a distinct PHY band (2.4 GHz, 5 GHz and 6 GHz). Therefore,
 * it is expected that three links are setup by the non-AP MLD(s). The values for the Padding
 * Delay, the Transition Delay and the Transition Timeout are provided as argument to the
 * constructor of this class, along with the IDs of the links on which EMLSR mode must be
 * enabled for the non-AP MLDs (this information is used to set the EmlsrLinkSet attribute
 * of the DefaultEmlsrManager installed on the non-AP MLDs).
 */
class EmlsrOperationsTestBase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param name The name of the new TestCase created
     */
    EmlsrOperationsTestBase(const std::string& name);
    ~EmlsrOperationsTestBase() override = default;

    /// Enumeration for traffic directions
    enum TrafficDirection : uint8_t
    {
        DOWNLINK = 0,
        UPLINK
    };

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

    /**
     * \param dir the traffic direction (downlink/uplink)
     * \param staId the index (starting at 0) of the non-AP MLD generating/receiving packets
     * \param count the number of packets to generate
     * \param pktSize the size of the packets to generate
     * \return an application generating the given number packets of the given size from/to the
     *         AP MLD to/from the given non-AP MLD
     */
    Ptr<PacketSocketClient> GetApplication(TrafficDirection dir,
                                           std::size_t staId,
                                           std::size_t count,
                                           std::size_t pktSize) const;

    void DoSetup() override;

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time startTx;             ///< TX start time
        WifiConstPsduMap psduMap; ///< transmitted PSDU map
        WifiTxVector txVector;    ///< TXVECTOR
        uint8_t linkId;           ///< link ID
    };

    std::set<uint8_t> m_linksToEnableEmlsrOn; /**< IDs of the links on which EMLSR mode has to
                                                   be enabled */
    std::size_t m_nEmlsrStations{1};          ///< number of stations to create that activate EMLSR
    std::size_t m_nNonEmlsrStations{0};       /**< number of stations to create that do not
                                                activate EMLSR */
    Time m_transitionTimeout{MicroSeconds(128)}; ///< Transition Timeout advertised by the AP MLD
    std::vector<Time> m_paddingDelay{
        {MicroSeconds(32)}}; ///< Padding Delay advertised by the non-AP MLD
    std::vector<Time> m_transitionDelay{
        {MicroSeconds(16)}};                ///< Transition Delay advertised by the non-AP MLD
    bool m_establishBaDl{false};            /**< whether BA needs to be established (for TID 0)
                                                 with the AP as originator */
    bool m_establishBaUl{false};            /**< whether BA needs to be established (for TID 0)
                                                 with the AP as recipient */
    std::vector<FrameInfo> m_txPsdus;       ///< transmitted PSDUs
    Ptr<ApWifiMac> m_apMac;                 ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs; ///< MACs of the non-AP MLDs
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket address for DL traffic
    std::vector<PacketSocketAddress> m_ulSockets; ///< packet socket address for UL traffic
    uint16_t m_lastAid{0};                        ///< AID of last associated station
    Time m_duration{0};                           ///< simulation duration

  private:
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

EmlsrOperationsTestBase::EmlsrOperationsTestBase(const std::string& name)
    : TestCase(name)
{
}

void
EmlsrOperationsTestBase::Transmit(uint8_t linkId,
                                  std::string context,
                                  WifiConstPsduMap psduMap,
                                  WifiTxVector txVector,
                                  double txPowerW)
{
    m_txPsdus.push_back({Simulator::Now(), psduMap, txVector, linkId});
    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << "PSDU #" << m_txPsdus.size() << " Link ID " << +linkId << " "
           << psdu->GetHeader(0).GetTypeString();
        if (psdu->GetHeader(0).IsAction())
        {
            ss << " ";
            WifiActionHeader actionHdr;
            psdu->GetPayload(0)->PeekHeader(actionHdr);
            actionHdr.Print(ss);
        }
        ss << " #MPDUs " << psdu->GetNMpdus() << " duration/ID " << psdu->GetHeader(0).GetDuration()
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
    }
    NS_LOG_INFO("TX duration = " << txDuration.As(Time::MS) << "  TXVECTOR = " << txVector << "\n");
}

void
EmlsrOperationsTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);
    int64_t streamNumber = 100;

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes(m_nEmlsrStations);

    WifiHelper wifi;
    // wifi.EnableLogComponents ();
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"),
                                 "ControlMode",
                                 StringValue("HtMcs0"));
    wifi.ConfigEhtOptions("EmlsrActivated",
                          BooleanValue(true),
                          "TransitionTimeout",
                          TimeValue(m_transitionTimeout));

    // MLDs are configured with three links
    SpectrumWifiPhyHelper phyHelper(3);
    phyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phyHelper.Set(0, "ChannelSettings", StringValue("{2, 0, BAND_2_4GHZ, 0}"));
    phyHelper.Set(1, "ChannelSettings", StringValue("{36, 0, BAND_5GHZ, 0}"));
    phyHelper.Set(2, "ChannelSettings", StringValue("{1, 0, BAND_6GHZ, 0}"));
    // Add three spectrum channels to use multi-RF interface
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_2_4_GHZ);
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_5_GHZ);
    phyHelper.AddChannel(CreateObject<MultiModelSpectrumChannel>(), WIFI_SPECTRUM_6_GHZ);

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

    NetDeviceContainer apDevice = wifi.Install(phyHelper, mac, wifiApNode);

    mac.SetType("ns3::StaWifiMac",
                "Ssid",
                SsidValue(Ssid("wrong-ssid")),
                "ActiveProbing",
                BooleanValue(false));
    mac.SetEmlsrManager("ns3::DefaultEmlsrManager",
                        "EmlsrLinkSet",
                        AttributeContainerValue<UintegerValue>(m_linksToEnableEmlsrOn));

    NetDeviceContainer staDevices = wifi.Install(phyHelper, mac, wifiStaNodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac());

    for (uint32_t i = 0; i < staDevices.GetN(); i++)
    {
        auto device = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        auto staMac = DynamicCast<StaWifiMac>(device->GetMac());
        NS_ASSERT_MSG(i < m_paddingDelay.size(), "Not enough padding delay values provided");
        staMac->GetEmlsrManager()->SetAttribute("EmlsrPaddingDelay",
                                                TimeValue(m_paddingDelay.at(i)));
        NS_ASSERT_MSG(i < m_transitionDelay.size(), "Not enough transition delay values provided");
        staMac->GetEmlsrManager()->SetAttribute("EmlsrTransitionDelay",
                                                TimeValue(m_transitionDelay.at(i)));
    }

    if (m_nNonEmlsrStations > 0)
    {
        // create the other non-AP MLDs for which EMLSR is not activated
        wifi.ConfigEhtOptions("EmlsrActivated", BooleanValue(false));
        NodeContainer otherStaNodes(m_nNonEmlsrStations);
        staDevices.Add(wifi.Install(phyHelper, mac, otherStaNodes));
        wifiStaNodes.Add(otherStaNodes);
    }

    for (uint32_t i = 0; i < staDevices.GetN(); i++)
    {
        auto device = DynamicCast<WifiNetDevice>(staDevices.Get(i));
        m_staMacs.push_back(DynamicCast<StaWifiMac>(device->GetMac()));
    }

    // Trace PSDUs passed to the PHY on AP MLD and non-AP MLDs
    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                            std::to_string(linkId) + "/PhyTxPsduBegin",
                        MakeCallback(&EmlsrOperationsTestBase::Transmit, this).Bind(linkId));
    }

    // Uncomment the lines below to write PCAP files
    // phyHelper.EnablePcap("wifi-emlsr_AP", apDevice);
    // phyHelper.EnablePcap("wifi-emlsr_STA", staDevices);

    // Assign fixed streams to random variables in use
    streamNumber += wifi.AssignStreams(apDevice, streamNumber);
    streamNumber += wifi.AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    for (std::size_t id = 0; id <= m_nEmlsrStations + m_nNonEmlsrStations; id++)
    {
        // all non-AP MLDs are co-located
        positionAlloc->Add(Vector(std::min<double>(id, 1), 0.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // install a packet socket server on all nodes
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); nodeIt++)
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

    // set DL and UL packet sockets
    for (const auto& staMac : m_staMacs)
    {
        m_dlSockets.emplace_back();
        m_dlSockets.back().SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
        m_dlSockets.back().SetPhysicalAddress(staMac->GetDevice()->GetAddress());
        m_dlSockets.back().SetProtocol(1);

        m_ulSockets.emplace_back();
        m_ulSockets.back().SetSingleDevice(staMac->GetDevice()->GetIfIndex());
        m_ulSockets.back().SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
        m_ulSockets.back().SetProtocol(1);
    }

    // schedule ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext("AssociatedSta",
                                        MakeCallback(&EmlsrOperationsTestBase::SetSsid, this));
    Simulator::Schedule(Seconds(0), [&]() { m_staMacs[0]->SetSsid(Ssid("ns-3-ssid")); });
}

Ptr<PacketSocketClient>
EmlsrOperationsTestBase::GetApplication(TrafficDirection dir,
                                        std::size_t staId,
                                        std::size_t count,
                                        std::size_t pktSize) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
    client->SetRemote(dir == DOWNLINK ? m_dlSockets.at(staId) : m_ulSockets.at(staId));
    client->SetStartTime(Seconds(0)); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
EmlsrOperationsTestBase::SetSsid(uint16_t aid, Mac48Address /* addr */)
{
    if (m_lastAid == aid)
    {
        // another STA of this non-AP MLD has already fired this callback
        return;
    }
    m_lastAid = aid;

    // wait some time (5ms) to allow the completion of association
    auto delay = MilliSeconds(5);

    if (m_establishBaDl)
    {
        // trigger establishment of BA agreement with AP as originator
        Simulator::Schedule(delay, [=]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, aid - 1, 4, 1000));
        });

        delay += MilliSeconds(5);
    }

    if (m_establishBaUl)
    {
        // trigger establishment of BA agreement with AP as recipient
        Simulator::Schedule(delay, [=]() {
            m_staMacs[aid - 1]->GetDevice()->GetNode()->AddApplication(
                GetApplication(UPLINK, aid - 1, 4, 1000));
        });

        delay += MilliSeconds(5);
    }

    Simulator::Schedule(delay, [=]() {
        if (aid < m_nEmlsrStations + m_nNonEmlsrStations)
        {
            // make the next STA start ML discovery & setup
            m_staMacs[aid]->SetSsid(Ssid("ns-3-ssid"));
            return;
        }
        // all stations associated; start traffic if needed
        StartTraffic();
    });
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the exchange of EML Operating Mode Notification frames.
 *
 * This test considers an AP MLD and a non-AP MLD with EMLSR activated. Upon association,
 * the non-AP MLD sends an EML Operating Mode Notification frame, which is however corrupted
 * by using a post reception error model (installed on the AP MLD). We keep corrupting the
 * EML Notification frames transmitted by the non-AP MLD until the frame is dropped due to
 * exceeded max retry limit. It is checked that:
 *
 * - the Association Request contains a Multi-Link Element including an EML Capabilities field
 *   that contains the expected values for Padding Delay and Transition Delay
 * - the Association Response contains a Multi-Link Element including an EML Capabilities field
 *   that contains the expected value for Transition Timeout
 * - all EML Notification frames contain the expected values for EMLSR Mode, EMLMR Mode and
 *   Link Bitmap fields and are transmitted on the link used for association
 * - the correct EMLSR link set is stored by the EMLSR Manager, both when the transition
 *   timeout expires and when an EML Notification response is received from the AP MLD (thus,
 *   the correct EMLSR link set is stored after whichever of the two events occur first)
 */
class EmlNotificationExchangeTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * \param linksToEnableEmlsrOn IDs of links on which EMLSR mode should be enabled
     * \param transitionTimeout the Transition Timeout advertised by the AP MLD
     */
    EmlNotificationExchangeTest(const std::set<uint8_t>& linksToEnableEmlsrOn,
                                Time transitionTimeout);
    ~EmlNotificationExchangeTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(uint8_t linkId,
                  std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /**
     * Callback invoked when the non-AP MLD receives the acknowledgment for a transmitted MPDU.
     *
     * \param mpdu the acknowledged MPDU
     */
    void TxOk(Ptr<const WifiMpdu> mpdu);
    /**
     * Callback invoked when the non-AP MLD drops the given MPDU for the given reason.
     *
     * \param reason the reason why the MPDU was dropped
     * \param mpdu the dropped MPDU
     */
    void TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu);

    /**
     * Check the content of the EML Capabilities subfield of the Multi-Link Element included
     * in the Association Request frame sent by the non-AP MLD.
     *
     * \param mpdu the MPDU containing the Association Request frame
     * \param txVector the TXVECTOR used to transmit the frame
     * \param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlCapabilitiesInAssocReq(Ptr<const WifiMpdu> mpdu,
                                        const WifiTxVector& txVector,
                                        uint8_t linkId);
    /**
     * Check the content of the EML Capabilities subfield of the Multi-Link Element included
     * in the Association Response frame sent by the AP MLD to the EMLSR client.
     *
     * \param mpdu the MPDU containing the Association Response frame
     * \param txVector the TXVECTOR used to transmit the frame
     * \param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlCapabilitiesInAssocResp(Ptr<const WifiMpdu> mpdu,
                                         const WifiTxVector& txVector,
                                         uint8_t linkId);
    /**
     * Check the content of a received EML Operating Mode Notification frame.
     *
     * \param psdu the PSDU containing the EML Operating Mode Notification frame
     * \param txVector the TXVECTOR used to transmit the frame
     * \param linkId the ID of the link on which the frame was transmitted
     */
    void CheckEmlNotification(Ptr<const WifiPsdu> psdu,
                              const WifiTxVector& txVector,
                              uint8_t linkId);
    /**
     * Check that the EMLSR mode has been enabled on the expected EMLSR links.
     */
    void CheckEmlsrLinks();

  private:
    std::optional<uint8_t> m_assocLinkId;      //!< ID of the link used to establish association
    std::size_t m_checkEmlsrLinksCount;        /**< counter for the number of times CheckEmlsrLinks
                                                    is called (should be two: when the transition
                                                    timeout expires and when the EML Notification
                                                    response from the AP MLD is received */
    std::size_t m_emlNotificationDroppedCount; /**< counter for the number of times the EML
                                                    Notification frame sent by the non-AP MLD
                                                    has been dropped due to max retry limit */
    Ptr<ListErrorModel> m_errorModel;          ///< error rate model to corrupt packets at AP MLD
    std::list<uint64_t> m_uidList;             ///< list of UIDs of packets to corrupt
};

EmlNotificationExchangeTest::EmlNotificationExchangeTest(
    const std::set<uint8_t>& linksToEnableEmlsrOn,
    Time transitionTimeout)
    : EmlsrOperationsTestBase("Check EML Notification exchange"),
      m_checkEmlsrLinksCount(0),
      m_emlNotificationDroppedCount(0)
{
    m_linksToEnableEmlsrOn = linksToEnableEmlsrOn;
    m_nEmlsrStations = 1;
    m_nNonEmlsrStations = 0;
    m_transitionTimeout = transitionTimeout;
    m_duration = Seconds(0.5);
}

void
EmlNotificationExchangeTest::DoSetup()
{
    EmlsrOperationsTestBase::DoSetup();

    m_errorModel = CreateObject<ListErrorModel>();
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }

    m_staMacs[0]->TraceConnectWithoutContext(
        "AckedMpdu",
        MakeCallback(&EmlNotificationExchangeTest::TxOk, this));
    m_staMacs[0]->TraceConnectWithoutContext(
        "DroppedMpdu",
        MakeCallback(&EmlNotificationExchangeTest::TxDropped, this));
}

void
EmlNotificationExchangeTest::Transmit(uint8_t linkId,
                                      std::string context,
                                      WifiConstPsduMap psduMap,
                                      WifiTxVector txVector,
                                      double txPowerW)
{
    auto psdu = psduMap.begin()->second;

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        m_assocLinkId = linkId;
        CheckEmlCapabilitiesInAssocReq(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
        CheckEmlCapabilitiesInAssocResp(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_MGT_ACTION:
        if (auto [category, action] = WifiActionHeader::Peek(psdu->GetPayload(0));
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            CheckEmlNotification(psdu, txVector, linkId);

            if (m_emlNotificationDroppedCount == 0 &&
                m_staMacs[0]->GetLinkIdByAddress(psdu->GetAddr2()) == linkId)
            {
                // transmitted by non-AP MLD, we need to corrupt it
                m_uidList.push_front(psdu->GetPacket()->GetUid());
                m_errorModel->SetList(m_uidList);
            }
            break;
        }

    default:;
    }

    EmlsrOperationsTestBase::Transmit(linkId, context, psduMap, txVector, txPowerW);
}

void
EmlNotificationExchangeTest::CheckEmlCapabilitiesInAssocReq(Ptr<const WifiMpdu> mpdu,
                                                            const WifiTxVector& txVector,
                                                            uint8_t linkId)
{
    MgtAssocRequestHeader frame;
    mpdu->GetPacket()->PeekHeader(frame);

    const auto& mle = frame.Get<MultiLinkElement>();
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(), true, "Multi-Link Element must be present in AssocReq");

    NS_TEST_ASSERT_MSG_EQ(mle->HasEmlCapabilities(),
                          true,
                          "Multi-Link Element in AssocReq must have EML Capabilities");
    NS_TEST_ASSERT_MSG_EQ(mle->IsEmlsrSupported(),
                          true,
                          "EML Support subfield of EML Capabilities in AssocReq must be set to 1");
    NS_TEST_ASSERT_MSG_EQ(mle->GetEmlsrPaddingDelay(),
                          m_paddingDelay.at(0),
                          "Unexpected Padding Delay in EML Capabilities included in AssocReq");
    NS_TEST_ASSERT_MSG_EQ(mle->GetEmlsrTransitionDelay(),
                          m_transitionDelay.at(0),
                          "Unexpected Transition Delay in EML Capabilities included in AssocReq");
}

void
EmlNotificationExchangeTest::CheckEmlCapabilitiesInAssocResp(Ptr<const WifiMpdu> mpdu,
                                                             const WifiTxVector& txVector,
                                                             uint8_t linkId)
{
    bool sentToEmlsrClient =
        (m_staMacs[0]->GetLinkIdByAddress(mpdu->GetHeader().GetAddr1()) == linkId);

    if (!sentToEmlsrClient)
    {
        // nothing to check
        return;
    }

    MgtAssocResponseHeader frame;
    mpdu->GetPacket()->PeekHeader(frame);

    const auto& mle = frame.Get<MultiLinkElement>();
    NS_TEST_ASSERT_MSG_EQ(mle.has_value(), true, "Multi-Link Element must be present in AssocResp");

    NS_TEST_ASSERT_MSG_EQ(mle->HasEmlCapabilities(),
                          true,
                          "Multi-Link Element in AssocResp must have EML Capabilities");
    NS_TEST_ASSERT_MSG_EQ(mle->IsEmlsrSupported(),
                          true,
                          "EML Support subfield of EML Capabilities in AssocResp must be set to 1");
    NS_TEST_ASSERT_MSG_EQ(
        mle->GetTransitionTimeout(),
        m_transitionTimeout,
        "Unexpected Transition Timeout in EML Capabilities included in AssocResp");
}

void
EmlNotificationExchangeTest::CheckEmlNotification(Ptr<const WifiPsdu> psdu,
                                                  const WifiTxVector& txVector,
                                                  uint8_t linkId)
{
    MgtEmlOperatingModeNotification frame;
    auto mpdu = *psdu->begin();
    auto pkt = mpdu->GetPacket()->Copy();
    WifiActionHeader::Remove(pkt);
    pkt->RemoveHeader(frame);
    NS_LOG_DEBUG(frame);

    bool sentbyNonApMld = m_staMacs[0]->GetLinkIdByAddress(mpdu->GetHeader().GetAddr2()) == linkId;

    NS_TEST_EXPECT_MSG_EQ(+frame.m_emlControl.emlsrMode,
                          1,
                          "EMLSR Mode subfield should be set to 1 (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    NS_TEST_EXPECT_MSG_EQ(+frame.m_emlControl.emlmrMode,
                          0,
                          "EMLMR Mode subfield should be set to 0 (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    NS_TEST_ASSERT_MSG_EQ(frame.m_emlControl.linkBitmap.has_value(),
                          true,
                          "Link Bitmap subfield should be present (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    auto setupLinks = m_staMacs[0]->GetSetupLinkIds();
    std::list<uint8_t> expectedEmlsrLinks;
    std::set_intersection(setupLinks.begin(),
                          setupLinks.end(),
                          m_linksToEnableEmlsrOn.begin(),
                          m_linksToEnableEmlsrOn.end(),
                          std::back_inserter(expectedEmlsrLinks));

    NS_TEST_EXPECT_MSG_EQ((expectedEmlsrLinks == frame.GetLinkBitmap()),
                          true,
                          "Unexpected Link Bitmap subfield (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");

    if (!sentbyNonApMld)
    {
        // the frame has been sent by the AP MLD
        NS_TEST_ASSERT_MSG_EQ(
            +frame.m_emlControl.emlsrParamUpdateCtrl,
            0,
            "EMLSR Parameter Update Control should be set to 0 in frames sent by the AP MLD");

        // as soon as the non-AP MLD receives this frame, it sets the EMLSR links
        auto delay = WifiPhy::CalculateTxDuration(psdu,
                                                  txVector,
                                                  m_staMacs[0]->GetWifiPhy(linkId)->GetPhyBand()) +
                     MicroSeconds(1); // to account for propagation delay
        Simulator::Schedule(delay, &EmlNotificationExchangeTest::CheckEmlsrLinks, this);
    }

    NS_TEST_ASSERT_MSG_EQ(m_assocLinkId.has_value(),
                          true,
                          "No link ID indicating the link on which association was performed");
    NS_TEST_EXPECT_MSG_EQ(+m_assocLinkId.value(),
                          +linkId,
                          "EML Notification received on unexpected link (frame sent by non-AP MLD: "
                              << std::boolalpha << sentbyNonApMld << ")");
}

void
EmlNotificationExchangeTest::TxOk(Ptr<const WifiMpdu> mpdu)
{
    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsMgt() && hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame that the non-AP MLD sent has been
            // acknowledged; after the transition timeout, the EMLSR links have been set
            Simulator::Schedule(m_transitionTimeout + NanoSeconds(1),
                                &EmlNotificationExchangeTest::CheckEmlsrLinks,
                                this);
        }
    }
}

void
EmlNotificationExchangeTest::TxDropped(WifiMacDropReason reason, Ptr<const WifiMpdu> mpdu)
{
    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsMgt() && hdr.IsAction())
    {
        if (auto [category, action] = WifiActionHeader::Peek(mpdu->GetPacket());
            category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            // the EML Operating Mode Notification frame has been dropped. Don't
            // corrupt it anymore
            m_emlNotificationDroppedCount++;
        }
    }
}

void
EmlNotificationExchangeTest::CheckEmlsrLinks()
{
    m_checkEmlsrLinksCount++;

    auto setupLinks = m_staMacs[0]->GetSetupLinkIds();
    std::set<uint8_t> expectedEmlsrLinks;
    std::set_intersection(setupLinks.begin(),
                          setupLinks.end(),
                          m_linksToEnableEmlsrOn.begin(),
                          m_linksToEnableEmlsrOn.end(),
                          std::inserter(expectedEmlsrLinks, expectedEmlsrLinks.end()));

    NS_TEST_EXPECT_MSG_EQ((expectedEmlsrLinks == m_staMacs[0]->GetEmlsrManager()->GetEmlsrLinks()),
                          true,
                          "Unexpected set of EMLSR links)");
}

void
EmlNotificationExchangeTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_checkEmlsrLinksCount,
                          2,
                          "Unexpected number of times CheckEmlsrLinks() is called");
    NS_TEST_EXPECT_MSG_EQ(
        m_emlNotificationDroppedCount,
        1,
        "Unexpected number of times the EML Notification frame is dropped due to max retry limit");

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the transmission of DL frames to EMLSR clients.
 *
 * This test considers an AP MLD and a configurable number of non-AP MLDs that support EMLSR
 * and a configurable number of non-AP MLDs that do not support EMLSR. All MLDs have three
 * setup links, while the set of EMLSR links for the EMLSR clients is configurable.
 * Block ack agreements (for TID 0, with the AP MLD as originator) are established with all
 * the non-AP MLDs before that EMLSR clients send the EML Operating Mode Notification frame
 * to enable the EMLSR mode on their EMLSR links.
 *
 * Before enabling EMLSR mode, it is checked that:
 *
 * - all EMLSR links (but the link used for ML setup) of the EMLSR clients are considered to
 *   be in power save mode and are blocked by the AP MLD; all the other links have transitioned
 *   to active mode and are not blocked
 * - no MU-RTS Trigger Frame is sent as Initial control frame
 * - In case of EMLSR clients having no link that is not an EMLSR link and is different than
 *   the link used for ML setup, the two A-MPDUs used to trigger BA establishment are
 *   transmitted one after another on the link used for ML setup. Otherwise, the two A-MPDUs
 *   are sent concurrently on two distinct links
 *
 * After enabling EMLSR mode, it is checked that:
 *
 * - all EMLSR links of the EMLSR clients are considered to be in active mode and are not
 *   blocked by the AP MLD
 * - If all setup links are EMLSR links, the first two frame exchanges are both protected by
 *   MU-RTS TF and occur one after another. Otherwise, one frame exchange occurs on the
 *   non-EMLSR link and is not protected by MU-RTS TF; the other frame exchange occurs on an
 *   EMLSR link and is protected by MU-RTS TF
 * - the AP MLD blocks transmission on all other EMLSR links when sending an ICF to an EMLSR client
 * - After completing a frame exchange with an EMLSR client, the AP MLD can start another frame
 *   exchange with that EMLSR client within the same TXOP (after a SIFS) without sending an ICF
 * - During the transition delay, all EMLSR links are not used for DL transmissions
 * - The padding added to Initial Control frames is the largest among all the EMLSR clients
 *   solicited by the ICF
 *
 * After disabling EMLSR mode, it is checked that:
 *
 * - all EMLSR links (but the link used to exchange EML Notification frames) of the EMLSR clients
 *   are considered to be in power save mode and are blocked by the AP MLD
 * - an MU-RTS Trigger Frame is sent by the AP MLD as ICF for sending the EML Notification
 *   response, unless the link used to exchange EML Notification frames is a non-EMLSR link
 * - no MU-RTS Trigger Frame is used as ICF for QoS data frames
 * - In case of EMLSR clients having no link that is not an EMLSR link and is different than
 *   the link used to exchange EML Notification frames, the two A-MPDUs are transmitted one
 *   after another on the link used to exchange EML Notification frames. Otherwise, the two
 *   A-MPDUs are sent concurrently on two distinct links
 */
class EmlsrDlTxopTest : public EmlsrOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * \param nEmlsrStations number of non-AP MLDs that support EMLSR
     * \param nNonEmlsrStations number of non-AP MLDs that do not support EMLSR
     * \param linksToEnableEmlsrOn IDs of links on which EMLSR mode should be enabled
     * \param paddingDelay vector (whose size equals <i>nEmlsrStations</i>) of the padding
     *                     delay values advertised by non-AP MLDs
     * \param transitionDelay vector (whose size equals <i>nEmlsrStations</i>) of the transition
     *                        delay values advertised by non-AP MLDs
     * \param transitionTimeout the Transition Timeout advertised by the AP MLD
     */
    EmlsrDlTxopTest(std::size_t nEmlsrStations,
                    std::size_t nNonEmlsrStations,
                    const std::set<uint8_t>& linksToEnableEmlsrOn,
                    const std::vector<Time>& paddingDelay,
                    const std::vector<Time>& transitionDelay,
                    Time transitionTimeout);
    ~EmlsrDlTxopTest() override = default;

  protected:
    void DoSetup() override;
    void DoRun() override;
    void Transmit(uint8_t linkId,
                  std::string context,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;

    /**
     * Check that the simulation produced the expected results.
     */
    void CheckResults();

    /**
     * Check that the AP MLD considers the correct Power Management mode for the links setup
     * with the given non-AP MLD. This method is intended to be called shortly after ML setup.
     *
     * \param address a link address of the given non-AP MLD
     */
    void CheckPmModeAfterAssociation(const Mac48Address& address);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting an EML Operating Mode
     * Notification response frame to an EMLSR client on the given link.
     *
     * \param mpdu the MPDU carrying the EML Operating Mode Notification frame
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckEmlNotificationFrame(Ptr<const WifiMpdu> mpdu,
                                   const WifiTxVector& txVector,
                                   uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting an initial
     * Control frame to an EMLSR client on the given link.
     *
     * \param mpdu the MPDU carrying the MU-RTS TF
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckInitialControlFrame(Ptr<const WifiMpdu> mpdu,
                                  const WifiTxVector& txVector,
                                  uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD transmitting a PPDU containing
     * QoS data frames to EMLSR clients on the given link.
     *
     * \param psduMap the PSDU(s) carrying QoS data frames
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckQosFrames(const WifiConstPsduMap& psduMap,
                        const WifiTxVector& txVector,
                        uint8_t linkId);

    /**
     * Check that appropriate actions are taken by the AP MLD receiving a PPDU containing
     * BlockAck frames from EMLSR clients on the given link.
     *
     * \param psduMap the PSDU carrying BlockAck frames
     * \param txVector the TXVECTOR used to send the PPDU
     * \param linkId the ID of the given link
     */
    void CheckBlockAck(const WifiConstPsduMap& psduMap,
                       const WifiTxVector& txVector,
                       uint8_t linkId);

  private:
    void StartTraffic() override;

    /**
     * Enable EMLSR mode on the next EMLSR client
     */
    void EnableEmlsrMode();

    std::set<uint8_t> m_emlsrLinks; /**< IDs of the links on which EMLSR mode has to be enabled */
    std::vector<std::optional<uint8_t>>
        m_assocLinkId; /**< ID of the link used to establish association (vector index is the node
                          ID) */
    Time m_emlsrEnabledTime;      //!< when EMLSR mode has been enabled on all EMLSR clients
    const Time m_fe2to3delay;     /**< time interval between 2nd and 3rd frame exchange sequences
                                       after the enablement of EMLSR mode */
    std::size_t m_countQoSframes; //!< counter for QoS frames (transition delay test)
    std::size_t m_countBlockAck;  //!< counter for BlockAck frames (transition delay test)
    Ptr<ListErrorModel> m_errorModel; ///< error rate model to corrupt BlockAck at AP MLD
};

EmlsrDlTxopTest::EmlsrDlTxopTest(std::size_t nEmlsrStations,
                                 std::size_t nNonEmlsrStations,
                                 const std::set<uint8_t>& linksToEnableEmlsrOn,
                                 const std::vector<Time>& paddingDelay,
                                 const std::vector<Time>& transitionDelay,
                                 Time transitionTimeout)
    : EmlsrOperationsTestBase("Check EML DL TXOP transmissions (" + std::to_string(nEmlsrStations) +
                              "," + std::to_string(nNonEmlsrStations) + ")"),
      m_emlsrLinks(linksToEnableEmlsrOn),
      m_assocLinkId(1 + nEmlsrStations + nNonEmlsrStations, std::nullopt),
      m_emlsrEnabledTime(0),
      m_fe2to3delay(MilliSeconds(20)),
      m_countQoSframes(0),
      m_countBlockAck(0)
{
    m_nEmlsrStations = nEmlsrStations;
    m_nNonEmlsrStations = nNonEmlsrStations;
    m_linksToEnableEmlsrOn = {}; // do not enable EMLSR right after association
    m_paddingDelay = paddingDelay;
    m_transitionDelay = transitionDelay;
    m_transitionTimeout = transitionTimeout;
    m_establishBaDl = true;
    m_duration = Seconds(1.5);

    NS_ABORT_MSG_IF(linksToEnableEmlsrOn.size() < 2,
                    "This test requires at least two links to be configured as EMLSR links");
}

void
EmlsrDlTxopTest::Transmit(uint8_t linkId,
                          std::string context,
                          WifiConstPsduMap psduMap,
                          WifiTxVector txVector,
                          double txPowerW)
{
    auto psdu = psduMap.begin()->second;
    auto nodeId = ContextToNodeId(context);

    switch (psdu->GetHeader(0).GetType())
    {
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        NS_ASSERT_MSG(nodeId > 0, "APs do not send AssocReq frames");
        m_assocLinkId.at(nodeId) = linkId;
        if (nodeId <= m_nEmlsrStations)
        {
            // this AssocReq is being sent by an EMLSR client. The other EMLSR links should be
            // in powersave mode after association; we let the non-EMLSR links transition to
            // active mode (by sending data null frames) after association
            for (uint8_t id = 0; id < m_staMacs.at(nodeId - 1)->GetNLinks(); id++)
            {
                if (id != linkId && m_emlsrLinks.count(id) == 1)
                {
                    m_staMacs[nodeId - 1]->SetPowerSaveMode({true, id});
                }
            }
        }
        break;

    case WIFI_MAC_MGT_ACTION: {
        auto [category, action] = WifiActionHeader::Peek(psdu->GetPayload(0));

        if (nodeId == 0 && category == WifiActionHeader::PROTECTED_EHT &&
            action.protectedEhtAction ==
                WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION)
        {
            CheckEmlNotificationFrame(*psdu->begin(), txVector, linkId);
        }
        else if (category == WifiActionHeader::BLOCK_ACK &&
                 action.blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST)
        {
            CheckPmModeAfterAssociation(psdu->GetAddr1());
        }
    }
    break;

    case WIFI_MAC_CTL_TRIGGER:
        CheckInitialControlFrame(*psdu->begin(), txVector, linkId);
        break;

    case WIFI_MAC_QOSDATA:
        CheckQosFrames(psduMap, txVector, linkId);
        break;

    case WIFI_MAC_CTL_BACKRESP:
        CheckBlockAck(psduMap, txVector, linkId);
        break;

    default:;
    }

    EmlsrOperationsTestBase::Transmit(linkId, context, psduMap, txVector, txPowerW);
}

void
EmlsrDlTxopTest::DoSetup()
{
    EmlsrOperationsTestBase::DoSetup();

    m_errorModel = CreateObject<ListErrorModel>();
    for (std::size_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        m_apMac->GetWifiPhy(linkId)->SetPostReceptionErrorModel(m_errorModel);
    }

    m_apMac->GetQosTxop(AC_BE)->SetTxopLimits(
        {MicroSeconds(3200), MicroSeconds(3200), MicroSeconds(3200)});

    if (m_nEmlsrStations + m_nNonEmlsrStations > 1)
    {
        auto muScheduler =
            CreateObjectWithAttributes<RrMultiUserScheduler>("EnableUlOfdma", BooleanValue(false));
        m_apMac->AggregateObject(muScheduler);
        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            m_apMac->GetFrameExchangeManager(linkId)->GetAckManager()->SetAttribute(
                "DlMuAckSequenceType",
                EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
        }
    }
}

void
EmlsrDlTxopTest::StartTraffic()
{
    if (m_emlsrEnabledTime.IsZero())
    {
        // we are done with association and Block Ack agreement; we can now enable EMLSR mode
        m_lastAid = 0;
        EnableEmlsrMode();
        return;
    }

    // we are done with sending EML Operating Mode Notification frames. We can now generate
    // packets for all non-AP MLDs
    for (std::size_t i = 0; i < m_nEmlsrStations + m_nNonEmlsrStations; i++)
    {
        // when multiple non-AP MLDs are present, MU transmission are used. Given that the
        // available bandwidth decreases as the number of non-AP MLDs increases, compute the
        // number of packets to generate so that we always have two A-MPDUs per non-AP MLD
        std::size_t count = 8 / (m_nEmlsrStations + m_nNonEmlsrStations);
        m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, i, count, 450));
    }

    // in case of 2 EMLSR clients using no non-EMLSR link, generate one additional short
    // packet to each EMLSR client to test transition delay
    if (m_nEmlsrStations == 2 && m_apMac->GetNLinks() == m_emlsrLinks.size())
    {
        Simulator::Schedule(m_fe2to3delay, [&]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 0, 1, 40));
            m_apMac->GetDevice()->GetNode()->AddApplication(GetApplication(DOWNLINK, 1, 1, 40));
        });
    }

    // schedule the transmission of EML Operating Mode Notification frames to disable EMLSR mode
    // and the generation of other packets destined to the EMLSR clients
    for (std::size_t id = 0; id < m_nEmlsrStations; id++)
    {
        Simulator::Schedule(m_fe2to3delay + MilliSeconds(5 * (id + 1)), [=]() {
            m_staMacs.at(id)->GetEmlsrManager()->SetAttribute(
                "EmlsrLinkSet",
                AttributeContainerValue<UintegerValue>({}));
        });

        Simulator::Schedule(m_fe2to3delay + MilliSeconds(5 * (m_nEmlsrStations + 1)), [=]() {
            m_apMac->GetDevice()->GetNode()->AddApplication(
                GetApplication(DOWNLINK, id, 8 / m_nEmlsrStations, 450));
        });
    }
}

void
EmlsrDlTxopTest::EnableEmlsrMode()
{
    m_staMacs.at(m_lastAid)->GetEmlsrManager()->SetAttribute(
        "EmlsrLinkSet",
        AttributeContainerValue<UintegerValue>(m_emlsrLinks));
    m_lastAid++;
    Simulator::Schedule(MilliSeconds(5), [=]() {
        if (m_lastAid < m_nEmlsrStations)
        {
            // make the next STA send EML Notification frame
            EnableEmlsrMode();
            return;
        }
        // all stations enabled EMLSR mode; start traffic
        m_emlsrEnabledTime = Simulator::Now();
        StartTraffic();
    });
}

void
EmlsrDlTxopTest::CheckResults()
{
    auto psduIt = m_txPsdus.cbegin();

    // lambda to jump to the next QoS data frame or MU-RTS Trigger Frame transmitted
    // to an EMLSR client
    auto jumpToQosDataOrMuRts = [&]() {
        while (psduIt != m_txPsdus.cend() &&
               !psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData())
        {
            auto psdu = psduIt->psduMap.cbegin()->second;
            if (psdu->GetHeader(0).IsTrigger())
            {
                CtrlTriggerHeader trigger;
                psdu->GetPayload(0)->PeekHeader(trigger);
                if (trigger.IsMuRts())
                {
                    break;
                }
            }
            psduIt++;
        }
    };

    /**
     * Before enabling EMLSR mode, no MU-RTS TF should be sent. Four packets are generated
     * after association to trigger the establishment of a Block Ack agreement. The TXOP Limit
     * and the MCS are set such that two packets can be transmitted in a TXOP, hence we expect
     * that the AP MLD sends two A-MPDUs to each non-AP MLD.
     *
     * EMLSR client with EMLSR mode to be enabled on all links: after ML setup, all other links
     * stay in power save mode, hence BA establishment occurs on the same link.
     *
     *  [link 0]
     *  ───────────────────────────────────────────────────────────────────────────
     *                              | power save mode
     *
     *                   ┌─────┐      ┌─────┐                   ┌───┬───┐     ┌───┬───┐
     *            ┌───┐  │Assoc│      │ADDBA│             ┌───┐ │QoS│QoS│     │QoS│QoS│
     *  [link 1]  │ACK│  │Resp │      │ Req │             │ACK│ │ 0 │ 1 │     │ 2 │ 3 │
     *  ───┬─────┬┴───┴──┴─────┴┬───┬─┴─────┴┬───┬─┬─────┬┴───┴─┴───┴───┴┬──┬─┴───┴───┴┬──┬───
     *     │Assoc│              │ACK│        │ACK│ │ADDBA│               │BA│          │BA│
     *     │ Req │              └───┘        └───┘ │Resp │               └──┘          └──┘
     *     └─────┘                                 └─────┘
     *
     *  [link 2]
     *  ───────────────────────────────────────────────────────────────────────────
     *                              | power save mode
     *
     *
     * EMLSR client with EMLSR mode to be enabled on not all the links: after ML setup,
     * the other EMLSR links stay in power save mode, the non-EMLSR link (link 1) transitions
     * to active mode.
     *
     *                                             ┌─────┐                   ┌───┬───┐
     *                                      ┌───┐  │ADDBA│             ┌───┐ │QoS│QoS│
     *  [link 0 - non EMLSR]                │ACK│  │ Req │             │ACK│ │ 2 │ 3 │
     *  ──────────────────────────────┬────┬┴───┴──┴─────┴┬───┬─┬─────┬┴───┴─┴───┴───┴┬──┬─
     *                                │Data│              │ACK│ │ADDBA│               │BA│
     *                                │Null│              └───┘ │Resp │               └──┘
     *                                └────┘                    └─────┘
     *                   ┌─────┐                                       ┌───┬───┐
     *            ┌───┐  │Assoc│                                       │QoS│QoS│
     *  [link 1]  │ACK│  │Resp │                                       │ 0 │ 1 │
     *  ───┬─────┬┴───┴──┴─────┴┬───┬──────────────────────────────────┴───┴───┴┬──┬───────
     *     │Assoc│              │ACK│                                           │BA│
     *     │ Req │              └───┘                                           └──┘
     *     └─────┘
     *
     *  [link 2]
     *  ───────────────────────────────────────────────────────────────────────────
     *                              | power save mode
     *
     * Non-EMLSR client (not shown): after ML setup, all other links transition to active mode
     * by sending a Data Null frame; QoS data frame exchanges occur on two links simultaneously.
     */
    for (std::size_t i = 0; i < m_nEmlsrStations + m_nNonEmlsrStations; i++)
    {
        std::set<uint8_t> linkIds;

        jumpToQosDataOrMuRts();
        NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend() &&
                               psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData()),
                              true,
                              "Expected at least one QoS data frame before enabling EMLSR mode");
        linkIds.insert(psduIt->linkId);
        const auto firstAmpduTxEnd =
            psduIt->startTx +
            WifiPhy::CalculateTxDuration(psduIt->psduMap,
                                         psduIt->txVector,
                                         m_staMacs[i]->GetWifiPhy(psduIt->linkId)->GetPhyBand());
        psduIt++;

        jumpToQosDataOrMuRts();
        NS_TEST_ASSERT_MSG_EQ((psduIt != m_txPsdus.cend() &&
                               psduIt->psduMap.cbegin()->second->GetHeader(0).IsQosData()),
                              true,
                              "Expected at least two QoS data frames before enabling EMLSR mode");
        linkIds.insert(psduIt->linkId);
        const auto secondAmpduTxStart = psduIt->startTx;
        psduIt++;

        /**
         * If this is an EMLSR client and there is no setup link other than the one used to
         * establish association that is not an EMLSR link, then the two A-MPDUs are sent one
         * after another on the link used to establish association.
         */
        auto setupLinks = m_staMacs[i]->GetSetupLinkIds();
        if (i < m_nEmlsrStations &&
            std::none_of(setupLinks.begin(), setupLinks.end(), [&](auto&& linkId) {
                return linkId != *m_assocLinkId[i + 1] && m_emlsrLinks.count(linkId) == 0;
            }))
        {
            NS_TEST_EXPECT_MSG_EQ(linkIds.size(),
                                  1,
                                  "Expected both A-MPDUs to be sent on the same link");
            NS_TEST_EXPECT_MSG_EQ(*linkIds.begin(),
                                  *m_assocLinkId[i + 1],
                                  "A-MPDUs sent on incorrect link");
            NS_TEST_EXPECT_MSG_LT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent one after another");
        }
        /**
         * Otherwise, the two A-MPDUs can be sent concurrently on two distinct links (may be
         * the link used to establish association and a non-EMLSR link).
         */
        else
        {
            NS_TEST_EXPECT_MSG_EQ(linkIds.size(),
                                  2,
                                  "Expected A-MPDUs to be sent on distinct links");
            NS_TEST_EXPECT_MSG_GT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent concurrently");
        }
    }

    /**
     * After enabling EMLSR mode, MU-RTS TF should only be sent on EMLSR links. After the exchange
     * of EML Operating Mode Notification frames, a number of packets are generated at the AP MLD
     * to prepare two A-MPDUs for each non-AP MLD.
     *
     * EMLSR client with EMLSR mode to be enabled on all links (A is the EMLSR client, B is the
     * non-EMLSR client):
     *                                      ┌─────┬─────┐
     *                                      │QoS 4│QoS 5│
     *                                      │ to A│ to A│
     *                            ┌───┐     ├─────┼─────┤
     *                            │MU │     │QoS 4│QoS 5│
     *  [link 0]                  │RTS│     │ to B│ to B│
     *  ──────────────────────────┴───┴┬───┬┴─────┴─────┴┬──┬────────────
     *                                 │CTS│             │BA│
     *                                 ├───┤             ├──┤
     *                                 │CTS│             │BA│
     *                                 └───┘             └──┘
     *                  ┌───┐      ┌─────┬─────┐
     *           ┌───┐  │EML│      │QoS 6│QoS 7│
     *  [link 1] │ACK│  │OM │      │ to B│ to B│
     *  ────┬───┬┴───┴──┴───┴┬───┬─┴─────┴─────┴┬──┬────────────────────────────────────
     *      │EML│            │ACK│              │BA│
     *      │OM │            └───┘              └──┘
     *      └───┘
     *                                                           ┌───┐     ┌─────┬─────┐
     *                                                           │MU │     │QoS 6│QoS 7│
     *  [link 2]                                                 │RTS│     │ to A│ to A│
     *  ─────────────────────────────────────────────────────────┴───┴┬───┬┴─────┴─────┴┬──┬─
     *                                                                │CTS│             │BA│
     *                                                                └───┘             └──┘
     *
     * EMLSR client with EMLSR mode to be enabled on not all the links (A is the EMLSR client,
     * B is the non-EMLSR client):
     *                             ┌─────┬─────┐
     *                             │QoS 4│QoS 5│
     *                             │ to A│ to A│
     *                             ├─────┼─────┤
     *                             │QoS 4│QoS 5│
     *  [link 0 - non EMLSR]       │ to B│ to B│
     *  ───────────────────────────┴─────┴─────┴┬──┬───────────────────────────
     *                                          │BA│
     *                                          ├──┤
     *                                          │BA│
     *                                          └──┘
     *                                       ┌─────┬─────┐
     *                                       │QoS 6│QoS 7│
     *                                       │ to A│ to A│
     *                  ┌───┐      ┌───┐     ├─────┼─────┤
     *           ┌───┐  │EML│      │MU │     │QoS 6│QoS 7│
     *  [link 1] │ACK│  │OM │      │RTS│     │ to B│ to B│
     *  ────┬───┬┴───┴──┴───┴┬───┬─┴───┴┬───┬┴─────┴─────┴┬──┬────────────
     *      │EML│            │ACK│      │CTS│             │BA│
     *      │OM │            └───┘      ├───┤             ├──┤
     *      └───┘                       │CTS│             │BA│
     *                                  └───┘             └──┘
     *
     *  [link 2]
     *  ────────────────────────────────────────────────────────────────────────────────
     */

    /// Store a QoS data frame or an MU-RTS TF followed by a QoS data frame
    using FrameExchange = std::list<decltype(psduIt)>;

    std::vector<std::list<FrameExchange>> frameExchanges(m_nEmlsrStations);

    // compute all frame exchanges involving EMLSR clients
    while (psduIt != m_txPsdus.cend())
    {
        jumpToQosDataOrMuRts();
        if (psduIt == m_txPsdus.cend())
        {
            break;
        }

        if (IsTrigger(psduIt->psduMap))
        {
            CtrlTriggerHeader trigger;
            psduIt->psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
            // this is an MU-RTS TF starting a new frame exchange sequence; add it to all
            // the addressed EMLSR clients
            NS_TEST_ASSERT_MSG_EQ(trigger.IsMuRts(),
                                  true,
                                  "jumpToQosDataOrMuRts does not return TFs other than MU-RTS");
            for (const auto& userInfo : trigger)
            {
                for (std::size_t i = 0; i < m_nEmlsrStations; i++)
                {
                    if (m_staMacs.at(i)->GetAssociationId() == userInfo.GetAid12())
                    {
                        frameExchanges.at(i).emplace_back(FrameExchange{psduIt});
                        break;
                    }
                }
            }
            psduIt++;
            continue;
        }

        // we get here if psduIt points to a psduMap containing QoS data frame(s); find (if any)
        // the QoS data frame(s) addressed to EMLSR clients and add them to the appropriate
        // frame exchange sequence
        for (const auto& staIdPsduPair : psduIt->psduMap)
        {
            std::for_each_n(m_staMacs.cbegin(), m_nEmlsrStations, [&](auto&& staMac) {
                if (!staMac->GetLinkIdByAddress(staIdPsduPair.second->GetAddr1()))
                {
                    // not addressed to this non-AP MLD
                    return;
                }
                // a QoS data frame starts a new frame exchange sequence if there is no previous
                // MU-RTS TF that has been sent on the same link and is not already followed by
                // a QoS data frame
                std::size_t id = staMac->GetDevice()->GetNode()->GetId() - 1;
                for (auto& frameExchange : frameExchanges.at(id))
                {
                    if (IsTrigger(frameExchange.front()->psduMap) &&
                        frameExchange.front()->linkId == psduIt->linkId &&
                        frameExchange.size() == 1)
                    {
                        auto it = std::next(frameExchange.front());
                        while (it != m_txPsdus.end())
                        {
                            // stop at the first frame other than CTS sent on this link
                            if (it->linkId == psduIt->linkId &&
                                !it->psduMap.begin()->second->GetHeader(0).IsCts())
                            {
                                break;
                            }
                            ++it;
                        }
                        if (it == psduIt)
                        {
                            // the QoS data frame actually followed the MU-RTS TF
                            frameExchange.emplace_back(psduIt);
                            return;
                        }
                    }
                }
                frameExchanges.at(id).emplace_back(FrameExchange{psduIt});
            });
        }
        psduIt++;
    }

    /**
     * Let's focus on the first two frame exchanges for each EMLSR clients. If all setup links are
     * EMLSR links, both frame exchanges are protected by MU-RTS TF and occur one after another.
     * Otherwise, one frame exchange occurs on the non-EMLSR link and is not protected by
     * MU-RTS TF; the other frame exchange occurs on an EMLSR link and is protected by MU-RTS TF.
     */
    for (std::size_t i = 0; i < m_nEmlsrStations; i++)
    {
        NS_TEST_EXPECT_MSG_GT_OR_EQ(frameExchanges.at(i).size(),
                                    2,
                                    "Expected at least 2 frame exchange sequences "
                                        << "involving EMLSR client " << i);

        auto firstExchangeIt = frameExchanges.at(i).begin();
        auto secondExchangeIt = std::next(firstExchangeIt);

        const auto firstAmpduTxEnd =
            firstExchangeIt->back()->startTx +
            WifiPhy::CalculateTxDuration(
                firstExchangeIt->back()->psduMap,
                firstExchangeIt->back()->txVector,
                m_staMacs[i]->GetWifiPhy(firstExchangeIt->back()->linkId)->GetPhyBand());
        const auto secondAmpduTxStart = secondExchangeIt->front()->startTx;

        if (m_staMacs[i]->GetNLinks() == m_emlsrLinks.size())
        {
            // all links are EMLSR links
            NS_TEST_EXPECT_MSG_EQ(IsTrigger(firstExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF of first frame exchange sequence");
            NS_TEST_EXPECT_MSG_EQ(
                firstExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame in the first frame exchange sequence");

            NS_TEST_EXPECT_MSG_EQ(IsTrigger(secondExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF of second frame exchange sequence");
            NS_TEST_EXPECT_MSG_EQ(
                secondExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame in the second frame exchange sequence");

            NS_TEST_EXPECT_MSG_LT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent one after another");
        }
        else
        {
            std::vector<uint8_t> nonEmlsrIds;
            auto setupLinks = m_staMacs[i]->GetSetupLinkIds();
            std::set_difference(setupLinks.begin(),
                                setupLinks.end(),
                                m_emlsrLinks.begin(),
                                m_emlsrLinks.end(),
                                std::back_inserter(nonEmlsrIds));
            NS_TEST_ASSERT_MSG_EQ(nonEmlsrIds.size(), 1, "Unexpected number of non-EMLSR links");

            auto nonEmlsrLinkExchangeIt = firstExchangeIt->front()->linkId == nonEmlsrIds[0]
                                              ? firstExchangeIt
                                              : secondExchangeIt;
            NS_TEST_EXPECT_MSG_EQ(IsTrigger(nonEmlsrLinkExchangeIt->front()->psduMap),
                                  false,
                                  "Did not expect an MU-RTS TF as ICF on non-EMLSR link");
            NS_TEST_EXPECT_MSG_EQ(
                nonEmlsrLinkExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame on the non-EMLSR link");

            auto emlsrLinkExchangeIt =
                nonEmlsrLinkExchangeIt == firstExchangeIt ? secondExchangeIt : firstExchangeIt;
            NS_TEST_EXPECT_MSG_NE(+emlsrLinkExchangeIt->front()->linkId,
                                  +nonEmlsrIds[0],
                                  "Expected this exchange not to occur on non-EMLSR link");
            NS_TEST_EXPECT_MSG_EQ(IsTrigger(emlsrLinkExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF on the EMLSR link");
            NS_TEST_EXPECT_MSG_EQ(
                emlsrLinkExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame on the EMLSR link");

            NS_TEST_EXPECT_MSG_GT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent concurrently");
        }

        // we are done with processing the first two frame exchanges, remove them
        frameExchanges.at(i).erase(firstExchangeIt);
        frameExchanges.at(i).erase(secondExchangeIt);
    }

    /**
     * A and B are two EMLSR clients. No ICF before the second QoS data frame because B
     * has not switched to listening mode. ICF is sent before the third QoS data frame because
     * A has switched to listening mode.
     *
     *                        ┌─────┐          A switches to listening
     *                        │QoS x│          after transition delay
     *                        │ to A│          |
     *              ┌───┐     ├─────┤    ┌─────┐
     *              │MU │     │QoS x│    │QoS y│
     *  [link 0]    │RTS│     │ to B│    │ to B│
     *  ────────────┴───┴┬───┬┴─────┴┬──┬┴─────┴┬──┬────────────
     *                   │CTS│       │BA│       │BA│
     *                   ├───┤       ├──┤       └──┘
     *                   │CTS│       │BA│
     *                   └───┘       └──┘                      A switches to listening
     *                                                         after timeout + transition delay
     *                                                         │
     *                                     ┌───┐     ┌─────┐   │       ┌───┐
     *                                     │MU │     │QoS x│   │       │MU │     ┌───┐
     *  [link 1]                           │RTS│     │ to A│   │       │RTS│     │BAR│
     *  ───────────────────────────────────┴───┴┬───┬┴─────┴┬──┬───────┴───┴┬───┬┴───┴┬──┬─
     *                                          │CTS│       │BA│            │CTS│     │BA│
     *                                          └───┘       └──x            └───┘     └──┘
     */
    if (m_nEmlsrStations == 2 && m_apMac->GetNLinks() == m_emlsrLinks.size())
    {
        // the following checks are only done with 2 EMLSR clients having no non-EMLSR link
        for (std::size_t i = 0; i < m_nEmlsrStations; i++)
        {
            NS_TEST_EXPECT_MSG_GT_OR_EQ(frameExchanges.at(i).size(),
                                        2,
                                        "Expected at least 2 frame exchange sequences "
                                            << "involving EMLSR client " << i);
            // the first frame exchange must start with an ICF
            auto firstExchangeIt = frameExchanges.at(i).begin();

            NS_TEST_EXPECT_MSG_EQ(IsTrigger(firstExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF of first frame exchange sequence");
            NS_TEST_EXPECT_MSG_EQ(
                firstExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
                true,
                "Expected a QoS data frame in the first frame exchange sequence");
        }

        // the second frame exchange is the one that starts first
        auto secondExchangeIt = std::next(frameExchanges.at(0).begin())->front()->startTx <
                                        std::next(frameExchanges.at(1).begin())->front()->startTx
                                    ? std::next(frameExchanges.at(0).begin())
                                    : std::next(frameExchanges.at(1).begin());
        decltype(secondExchangeIt) thirdExchangeIt;
        std::size_t thirdExchangeStaId;

        if (secondExchangeIt == std::next(frameExchanges.at(0).begin()))
        {
            thirdExchangeIt = std::next(frameExchanges.at(1).begin());
            thirdExchangeStaId = 1;
        }
        else
        {
            thirdExchangeIt = std::next(frameExchanges.at(0).begin());
            thirdExchangeStaId = 0;
        }

        // the second frame exchange is not protected by the ICF and starts a SIFS after the end
        // of the previous one
        NS_TEST_EXPECT_MSG_EQ(IsTrigger(secondExchangeIt->front()->psduMap),
                              false,
                              "Expected no ICF for the second frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(
            secondExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the second frame exchange sequence");

        // the first two frame exchanges occur on the same link
        NS_TEST_EXPECT_MSG_EQ(+secondExchangeIt->front()->linkId,
                              +frameExchanges.at(0).begin()->front()->linkId,
                              "Expected the first two frame exchanges to occur on the same link");

        auto bAckRespIt = std::prev(secondExchangeIt->front());
        NS_TEST_EXPECT_MSG_EQ(bAckRespIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAck(),
                              true,
                              "Expected a BlockAck response before the second frame exchange");
        auto bAckRespTxEnd =
            bAckRespIt->startTx +
            WifiPhy::CalculateTxDuration(bAckRespIt->psduMap,
                                         bAckRespIt->txVector,
                                         m_apMac->GetWifiPhy(bAckRespIt->linkId)->GetPhyBand());

        // the second frame exchange starts a SIFS after the previous one
        NS_TEST_EXPECT_MSG_EQ(
            bAckRespTxEnd + m_apMac->GetWifiPhy(bAckRespIt->linkId)->GetSifs(),
            secondExchangeIt->front()->startTx,
            "Expected the second frame exchange to start a SIFS after the first one");

        // the third frame exchange is protected by MU-RTS and occurs on a different link
        NS_TEST_EXPECT_MSG_EQ(IsTrigger(thirdExchangeIt->front()->psduMap),
                              true,
                              "Expected an MU-RTS as ICF for the third frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(
            thirdExchangeIt->back()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the third frame exchange sequence");

        NS_TEST_EXPECT_MSG_NE(
            +secondExchangeIt->front()->linkId,
            +thirdExchangeIt->front()->linkId,
            "Expected the second and third frame exchanges to occur on distinct links");

        auto secondQosIt = secondExchangeIt->front();
        auto secondQosTxEnd =
            secondQosIt->startTx +
            WifiPhy::CalculateTxDuration(secondQosIt->psduMap,
                                         secondQosIt->txVector,
                                         m_apMac->GetWifiPhy(secondQosIt->linkId)->GetPhyBand());

        NS_TEST_EXPECT_MSG_GT_OR_EQ(thirdExchangeIt->front()->startTx,
                                    secondQosTxEnd + m_transitionDelay.at(thirdExchangeStaId),
                                    "Transmission started before transition delay");

        // the BlockAck of the third frame exchange is not received correctly, so there should be
        // another frame exchange
        NS_TEST_EXPECT_MSG_EQ((thirdExchangeIt != frameExchanges.at(thirdExchangeStaId).end()),
                              true,
                              "Expected a fourth frame exchange");
        auto fourthExchangeIt = std::next(thirdExchangeIt);

        // the fourth frame exchange is protected by MU-RTS
        NS_TEST_EXPECT_MSG_EQ(IsTrigger(fourthExchangeIt->front()->psduMap),
                              true,
                              "Expected an MU-RTS as ICF for the fourth frame exchange sequence");

        bAckRespIt = std::prev(fourthExchangeIt->front());
        NS_TEST_EXPECT_MSG_EQ(bAckRespIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAck(),
                              true,
                              "Expected a BlockAck response before the fourth frame exchange");
        auto phy = m_apMac->GetWifiPhy(bAckRespIt->linkId);
        bAckRespTxEnd = bAckRespIt->startTx + WifiPhy::CalculateTxDuration(bAckRespIt->psduMap,
                                                                           bAckRespIt->txVector,
                                                                           phy->GetPhyBand());
        auto timeout = phy->GetSifs() + phy->GetSlot() + MicroSeconds(20);

        // the fourth frame exchange starts a SIFS after the previous one
        NS_TEST_EXPECT_MSG_GT_OR_EQ(fourthExchangeIt->front()->startTx,
                                    bAckRespTxEnd + timeout +
                                        m_transitionDelay.at(thirdExchangeStaId),
                                    "Transmission started before transition delay");

        auto bAckReqIt = std::next(fourthExchangeIt->front(), 2);
        NS_TEST_EXPECT_MSG_EQ(bAckReqIt->psduMap.cbegin()->second->GetHeader(0).IsBlockAckReq(),
                              true,
                              "Expected a BlockAck request in the fourth frame exchange");

        // we are done with processing the frame exchanges, remove them (two frame exchanges
        // per EMLSR client, plus the last one)
        frameExchanges.at(0).pop_front();
        frameExchanges.at(0).pop_front();
        frameExchanges.at(1).pop_front();
        frameExchanges.at(1).pop_front();
        frameExchanges.at(thirdExchangeStaId).pop_front();
    }

    /**
     * After disabling EMLSR mode, no MU-RTS TF should be sent. After the exchange of
     * EML Operating Mode Notification frames, a number of packets are generated at the AP MLD
     * to prepare two A-MPDUs for each EMLSR client.
     *
     * EMLSR client with EMLSR mode to be enabled on all links (A is the EMLSR client, B is the
     * non-EMLSR client):
     *
     *  [link 0]                            | power save mode
     *  ────────────────────────────────────────────────────────
     *                                        ┌─────┬─────┐        ┌──────┬──────┐
     *                                        │QoS 8│QoS 9│        │QoS 10│QoS 11│
     *                                        │ to A│ to A│        │ to A │ to A │
     *                  ┌───┐     ┌───┐       ├─────┼─────┤        ├──────┼──────┤
     *           ┌───┐  │MU │     │EML│       │QoS 8│QoS 9│        │QoS 10│QoS 11│
     *  [link 1] │ACK│  │RTS│     │OM │       │ to B│ to B│        │ to B │ to B │
     *  ────┬───┬┴───┴──┴───┴┬───┬┴───┴┬───┬──┴─────┴─────┴┬──┬────┴──────┴──────┴┬──┬─────
     *      │EML│            │CTS│     │ACK│               │BA│                   │BA│
     *      │OM │            └───┘     └───┘               ├──┤                   ├──┤
     *      └───┘                                          │BA│                   │BA│
     *                                                     └──┘                   └──┘
     *
     *  [link 2]                            | power save mode
     *  ────────────────────────────────────────────────────────────────────────────
     *
     *
     * EMLSR client with EMLSR mode to be enabled on not all the links (A is the EMLSR client,
     * B is the non-EMLSR client):
     *                                           ┌─────┬─────┐
     *                                           │QoS 8│QoS 9│
     *                                           │ to A│ to A│
     *                                           ├─────┼─────┤
     *                                           │QoS 8│QoS 9│
     *  [link 0 - non EMLSR]                     │ to B│ to B│
     *  ─────────────────────────────────────────┴─────┴─────┴┬──┬─────────────
     *                                                        │BA│
     *                                                        ├──┤
     *                                                        │BA│
     *                                                        └──┘
     *                                        ┌──────┬──────┐
     *                                        │QoS 10│QoS 11│
     *                                        │ to A │ to A │
     *                  ┌───┐     ┌───┐       ├──────┼──────┤
     *           ┌───┐  │MU │     │EML│       │QoS 10│QoS 11│
     *  [link 1] │ACK│  │RTS│     │OM │       │ to B │ to B │
     *  ────┬───┬┴───┴──┴───┴┬───┬┴───┴┬───┬──┴──────┴──────┴┬──┬─────
     *      │EML│            │CTS│     │ACK│                 │BA│
     *      │OM │            └───┘     └───┘                 ├──┤
     *      └───┘                                            │BA│
     *                                                       └──┘
     *
     *  [link 2]                            | power save mode
     *  ────────────────────────────────────────────────────────────────────────────
     *
     */

    // for each EMLSR client, there should be a frame exchange with ICF and no data frame
    // (ICF protects the EML Notification response) and two frame exchanges with data frames
    for (std::size_t i = 0; i < m_nEmlsrStations; i++)
    {
        // if the EML Notification frame disabling EMLSR mode is sent on an EMLSR link, it
        // is protected by an ICF; otherwise, it is not protected. Given that the default
        // EMLSR Manager sends EML Notification frames on the link used to establish association,
        // ICF is sent to protect the EML Notification frame if the link used to establish
        // association is an EMLSR link
        if (m_emlsrLinks.count(*m_assocLinkId.at(i + 1)) == 1)
        {
            auto firstExchangeIt = frameExchanges.at(i).begin();

            NS_TEST_EXPECT_MSG_EQ(IsTrigger(firstExchangeIt->front()->psduMap),
                                  true,
                                  "Expected an MU-RTS TF as ICF of first frame exchange sequence");
            NS_TEST_EXPECT_MSG_EQ(firstExchangeIt->size(),
                                  1,
                                  "Expected no data frame in the first frame exchange sequence");

            frameExchanges.at(i).pop_front();
        }

        NS_TEST_EXPECT_MSG_GT_OR_EQ(frameExchanges.at(i).size(),
                                    2,
                                    "Expected at least 2 frame exchange sequences "
                                        << "involving EMLSR client " << i);

        auto firstExchangeIt = frameExchanges.at(i).begin();
        auto secondExchangeIt = std::next(firstExchangeIt);

        const auto firstAmpduTxEnd =
            firstExchangeIt->back()->startTx +
            WifiPhy::CalculateTxDuration(
                firstExchangeIt->back()->psduMap,
                firstExchangeIt->back()->txVector,
                m_staMacs[i]->GetWifiPhy(firstExchangeIt->back()->linkId)->GetPhyBand());
        const auto secondAmpduTxStart = secondExchangeIt->front()->startTx;

        NS_TEST_EXPECT_MSG_EQ(
            firstExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the first frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(firstExchangeIt->size(),
                              1,
                              "Expected one frame only in the first frame exchange sequence");

        NS_TEST_EXPECT_MSG_EQ(
            secondExchangeIt->front()->psduMap.cbegin()->second->GetHeader(0).IsQosData(),
            true,
            "Expected a QoS data frame in the second frame exchange sequence");
        NS_TEST_EXPECT_MSG_EQ(secondExchangeIt->size(),
                              1,
                              "Expected one frame only in the second frame exchange sequence");

        if (m_staMacs[i]->GetNLinks() == m_emlsrLinks.size() ||
            m_emlsrLinks.count(*m_assocLinkId.at(i + 1)) == 0)
        {
            // all links are EMLSR links or the non-EMLSR link is the link used for association:
            // the two QoS data frames are sent one after another on the link used for association
            NS_TEST_EXPECT_MSG_EQ(
                +firstExchangeIt->front()->linkId,
                +(*m_assocLinkId.at(i + 1)),
                "First frame exchange expected to occur on link used for association");

            NS_TEST_EXPECT_MSG_EQ(
                +secondExchangeIt->front()->linkId,
                +(*m_assocLinkId.at(i + 1)),
                "Second frame exchange expected to occur on link used for association");

            NS_TEST_EXPECT_MSG_LT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent one after another");
        }
        else
        {
            // the two QoS data frames are sent concurrently on distinct links
            NS_TEST_EXPECT_MSG_NE(+firstExchangeIt->front()->linkId,
                                  +secondExchangeIt->front()->linkId,
                                  "Frame exchanges expected to occur on distinct links");

            NS_TEST_EXPECT_MSG_GT(firstAmpduTxEnd,
                                  secondAmpduTxStart,
                                  "A-MPDUs are not sent concurrently");
        }
    }
}

void
EmlsrDlTxopTest::CheckPmModeAfterAssociation(const Mac48Address& address)
{
    std::optional<std::size_t> staId;
    for (std::size_t id = 0; id < m_nEmlsrStations + m_nNonEmlsrStations; id++)
    {
        if (m_staMacs.at(id)->GetLinkIdByAddress(address))
        {
            staId = id;
            break;
        }
    }
    NS_TEST_ASSERT_MSG_EQ(staId.has_value(), true, "Not an address of a non-AP MLD " << address);

    // check that all EMLSR links (but the link used for ML setup) of the EMLSR clients
    // are considered to be in power save mode by the AP MLD; all the other links have
    // transitioned to active mode instead
    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        bool psModeExpected = *staId < m_nEmlsrStations && linkId != m_assocLinkId.at(*staId + 1) &&
                              m_emlsrLinks.count(linkId) == 1;
        auto addr = m_staMacs.at(*staId)->GetAddress();
        auto psMode = m_apMac->GetWifiRemoteStationManager(linkId)->IsInPsMode(addr);
        NS_TEST_EXPECT_MSG_EQ(psMode,
                              psModeExpected,
                              "EMLSR link " << +linkId << " of EMLSR client " << *staId
                                            << " not in " << (psModeExpected ? "PS" : "active")
                                            << " mode");
        // check that AP is blocking transmission of QoS data frames on this link
        WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, addr, 0);
        auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
        NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                              true,
                              "Expected to find a mask for EMLSR link "
                                  << +linkId << " of EMLSR client " << *staId);
        auto reason = static_cast<std::size_t>(WifiQueueBlockedReason::POWER_SAVE_MODE);
        NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                              psModeExpected,
                              "Expected EMLSR link " << +linkId << " of EMLSR client " << *staId
                                                     << " to be "
                                                     << (psModeExpected ? "blocked" : "unblocked"));
    }
}

void
EmlsrDlTxopTest::CheckEmlNotificationFrame(Ptr<const WifiMpdu> mpdu,
                                           const WifiTxVector& txVector,
                                           uint8_t linkId)
{
    // the AP is replying to a received EMLSR Notification frame
    auto pkt = mpdu->GetPacket()->Copy();
    const auto& hdr = mpdu->GetHeader();
    WifiActionHeader::Remove(pkt);
    MgtEmlOperatingModeNotification frame;
    pkt->RemoveHeader(frame);

    std::optional<std::size_t> staId;
    for (std::size_t id = 0; id < m_nEmlsrStations; id++)
    {
        if (m_staMacs.at(id)->GetFrameExchangeManager(linkId)->GetAddress() == hdr.GetAddr1())
        {
            staId = id;
            break;
        }
    }
    NS_TEST_ASSERT_MSG_EQ(staId.has_value(),
                          true,
                          "Not an address of an EMLSR client " << hdr.GetAddr1());

    // The EMLSR mode change occurs a Transition Timeout after the end of the PPDU carrying the Ack
    auto phy = m_apMac->GetWifiPhy(linkId);
    auto txDuration =
        WifiPhy::CalculateTxDuration(mpdu->GetSize() + 4, // A-MPDU Subframe header size
                                     txVector,
                                     phy->GetPhyBand());
    WifiTxVector ackTxVector =
        m_staMacs.at(*staId)->GetWifiRemoteStationManager(linkId)->GetAckTxVector(hdr.GetAddr2(),
                                                                                  txVector);
    auto ackDuration = WifiPhy::CalculateTxDuration(GetAckSize() + 4, // A-MPDU Subframe header
                                                    ackTxVector,
                                                    phy->GetPhyBand());

    Simulator::Schedule(txDuration + phy->GetSifs() + ackDuration, [=]() {
        if (frame.m_emlControl.emlsrMode == 1)
        {
            // EMLSR mode enabled. Check that all EMLSR links of the EMLSR clients are considered
            // to be in active mode by the AP MLD
            for (const auto linkId : m_emlsrLinks)
            {
                auto addr = m_staMacs.at(*staId)->GetAddress();
                auto psMode = m_apMac->GetWifiRemoteStationManager(linkId)->IsInPsMode(addr);
                NS_TEST_EXPECT_MSG_EQ(psMode,
                                      false,
                                      "EMLSR link " << +linkId << " of EMLSR client " << *staId
                                                    << " not in active mode");
                // check that AP is not blocking transmission of QoS data frames on this link
                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, addr, 0);
                auto mask =
                    m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
                NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                      true,
                                      "Expected to find a mask for EMLSR link "
                                          << +linkId << " of EMLSR client " << *staId);
                auto reason = static_cast<std::size_t>(WifiQueueBlockedReason::POWER_SAVE_MODE);
                NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                      false,
                                      "Expected EMLSR link " << +linkId << " of EMLSR client "
                                                             << *staId << " to be unblocked");
            }
        }
        else
        {
            // EMLSR mode disabled. Check that all EMLSR links (but the link used to send the
            // EML Notification frame) of the EMLSR clients are considered to be in power save
            // mode by the AP MLD; the other links are in active mode
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                bool psModeExpected = id != linkId && m_emlsrLinks.count(id) == 1;
                auto addr = m_staMacs.at(*staId)->GetAddress();
                auto psMode = m_apMac->GetWifiRemoteStationManager(id)->IsInPsMode(addr);
                NS_TEST_EXPECT_MSG_EQ(psMode,
                                      psModeExpected,
                                      "EMLSR link "
                                          << +id << " of EMLSR client " << *staId << " not in "
                                          << (psModeExpected ? "PS" : "active") << " mode");
                // check that AP is blocking transmission of QoS data frames on this link
                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, addr, 0);
                auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                      true,
                                      "Expected to find a mask for EMLSR link "
                                          << +id << " of EMLSR client " << *staId);
                auto reason = static_cast<std::size_t>(WifiQueueBlockedReason::POWER_SAVE_MODE);
                NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                      psModeExpected,
                                      "Expected EMLSR link "
                                          << +id << " of EMLSR client " << *staId << " to be "
                                          << (psModeExpected ? "blocked" : "unblocked"));
            }
        }
    });
}

void
EmlsrDlTxopTest::CheckInitialControlFrame(Ptr<const WifiMpdu> mpdu,
                                          const WifiTxVector& txVector,
                                          uint8_t linkId)
{
    CtrlTriggerHeader trigger;
    mpdu->GetPacket()->PeekHeader(trigger);
    if (!trigger.IsMuRts())
    {
        return;
    }

    NS_TEST_EXPECT_MSG_EQ(m_emlsrEnabledTime.IsStrictlyPositive(),
                          true,
                          "Did not expect an ICF before enabling EMLSR mode");

    NS_TEST_EXPECT_MSG_LT(txVector.GetPreambleType(),
                          WIFI_PREAMBLE_HT_MF,
                          "Unexpected preamble type for the Initial Control frame");
    auto rate = txVector.GetMode().GetDataRate(txVector);
    NS_TEST_EXPECT_MSG_EQ((rate == 6e6 || rate == 12e6 || rate == 24e6),
                          true,
                          "Unexpected rate for the Initial Control frame: " << rate);

    bool found = false;
    Time maxPaddingDelay{};

    for (const auto& userInfo : trigger)
    {
        auto addr = m_apMac->GetMldOrLinkAddressByAid(userInfo.GetAid12());
        NS_TEST_ASSERT_MSG_EQ(addr.has_value(),
                              true,
                              "AID " << userInfo.GetAid12() << " not found");

        if (m_apMac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*addr))
        {
            found = true;

            for (std::size_t i = 0; i < m_nEmlsrStations; i++)
            {
                if (m_staMacs.at(i)->GetAddress() == *addr)
                {
                    maxPaddingDelay = Max(maxPaddingDelay, m_paddingDelay.at(i));
                    break;
                }
            }

            // check that the AP has blocked transmission on all other EMLSR links
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                if (!m_apMac->GetWifiRemoteStationManager(id)->GetEmlsrEnabled(*addr))
                {
                    continue;
                }

                if (id == linkId)
                {
                    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *addr, 0);
                    auto mask =
                        m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                    NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                          true,
                                          "Expected to find a mask for EMLSR link "
                                              << +id << " of EMLSR client " << *addr);
                    NS_TEST_EXPECT_MSG_EQ(mask->none(),
                                          true,
                                          "Expected EMLSR link " << +id << " of EMLSR client "
                                                                 << *addr << " to be unblocked");
                    continue;
                }

                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *addr, 0);
                auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                      true,
                                      "Expected to find a mask for EMLSR link "
                                          << +id << " of EMLSR client " << *addr);
                auto reason =
                    static_cast<std::size_t>(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
                NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                      true,
                                      "Expected EMLSR link " << +id << " of EMLSR client " << *addr
                                                             << " to be blocked");
                NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                      1,
                                      "Expected EMLSR link "
                                          << +id << " of EMLSR client " << *addr
                                          << " to be blocked for one reason only");
            }
        }
    }

    NS_TEST_EXPECT_MSG_EQ(found, true, "Expected ICF to be addressed to at least an EMLSR client");

    if (maxPaddingDelay.IsStrictlyPositive())
    {
        // compare the TX duration of this Trigger Frame to that of the Trigger Frame with no
        // padding added
        auto txDurationWith =
            WifiPhy::CalculateTxDuration(mpdu->GetSize(),
                                         txVector,
                                         m_apMac->GetWifiPhy(linkId)->GetPhyBand());
        trigger.SetPaddingSize(0);
        auto pkt = Create<Packet>();
        pkt->AddHeader(trigger);
        auto txDurationWithout =
            WifiPhy::CalculateTxDuration(Create<WifiPsdu>(pkt, mpdu->GetHeader()),
                                         txVector,
                                         m_apMac->GetWifiPhy(linkId)->GetPhyBand());

        NS_TEST_EXPECT_MSG_EQ(txDurationWith,
                              txDurationWithout + maxPaddingDelay,
                              "Unexpected TX duration of the MU-RTS TF with padding "
                                  << maxPaddingDelay.As(Time::US));
    }
}

void
EmlsrDlTxopTest::CheckQosFrames(const WifiConstPsduMap& psduMap,
                                const WifiTxVector& txVector,
                                uint8_t linkId)
{
    if (m_nEmlsrStations != 2 || m_apMac->GetNLinks() != m_emlsrLinks.size() ||
        m_emlsrEnabledTime.IsZero() || Simulator::Now() < m_emlsrEnabledTime + m_fe2to3delay)

    {
        // we are interested in frames sent to test transition delay
        return;
    }

    std::size_t firstClientId = 0;
    std::size_t secondClientId = 1;
    auto addr = m_staMacs[secondClientId]->GetAddress();
    auto txDuration =
        WifiPhy::CalculateTxDuration(psduMap, txVector, m_apMac->GetWifiPhy(linkId)->GetPhyBand());

    m_countQoSframes++;

    switch (m_countQoSframes)
    {
    case 1:
        // generate another small packet addressed to the first EMLSR client only
        m_apMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(DOWNLINK, firstClientId, 1, 40));
        break;
    case 2:
        // generate another small packet addressed to the second EMLSR client
        m_apMac->GetDevice()->GetNode()->AddApplication(
            GetApplication(DOWNLINK, secondClientId, 1, 40));

        // the EMLSR client that is not the recipient of the QoS frame being transmitted will
        // switch back to listening mode after a transition delay starting from the end of
        // the PPDU carrying this QoS data frame

        // immediately before the end of the PPDU, this link is not blocked for the EMLSR client
        Simulator::Schedule(txDuration - NanoSeconds(1), [=]() {
            WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, addr, 0);
            auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
            NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                  true,
                                  "Expected to find a mask for EMLSR link "
                                      << +linkId << " of EMLSR client " << secondClientId);
            NS_TEST_EXPECT_MSG_EQ(mask->none(),
                                  true,
                                  "Expected EMLSR link " << +linkId << " of EMLSR client "
                                                         << secondClientId << " to be unblocked");
        });
        // immediately before the end of the PPDU, the other links are blocked for the EMLSR client
        Simulator::Schedule(txDuration - NanoSeconds(1), [=]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                if (id == linkId)
                {
                    continue;
                }

                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, addr, 0);
                auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                      true,
                                      "Expected to find a mask for EMLSR link "
                                          << +id << " of EMLSR client " << secondClientId);
                auto reason =
                    static_cast<std::size_t>(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
                NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                      true,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << secondClientId << " to be blocked");
                NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                      1,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << secondClientId
                                                             << " to be blocked "
                                                                " for one reason only");
            }
        });
        // immediately after the end of the PPDU, all links are blocked for the EMLSR client
        Simulator::Schedule(txDuration + NanoSeconds(1), [=]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, addr, 0);
                auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                      true,
                                      "Expected to find a mask for EMLSR link "
                                          << +id << " of EMLSR client " << secondClientId);
                auto reason = static_cast<std::size_t>(
                    WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY);
                NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                      true,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << secondClientId << " to be blocked");
                NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                      1,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << secondClientId
                                                             << " to be blocked "
                                                                " for one reason only");
            }
        });
        // immediately before the transition delay, all links are still blocked for the EMLSR client
        Simulator::Schedule(
            txDuration + m_transitionDelay.at(secondClientId) - NanoSeconds(1),
            [=]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, addr, 0);
                    auto mask =
                        m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                    NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                          true,
                                          "Expected to find a mask for EMLSR link "
                                              << +id << " of EMLSR client " << secondClientId);
                    auto reason = static_cast<std::size_t>(
                        WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY);
                    NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                          true,
                                          "Expected EMLSR link " << +id << " of EMLSR client "
                                                                 << secondClientId
                                                                 << " to be blocked");
                    NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                          1,
                                          "Expected EMLSR link " << +id << " of EMLSR client "
                                                                 << secondClientId
                                                                 << " to be blocked "
                                                                    " for one reason only");
                }
            });

        break;
    }
}

void
EmlsrDlTxopTest::CheckBlockAck(const WifiConstPsduMap& psduMap,
                               const WifiTxVector& txVector,
                               uint8_t linkId)
{
    if (m_nEmlsrStations != 2 || m_apMac->GetNLinks() != m_emlsrLinks.size() ||
        m_emlsrEnabledTime.IsZero() || Simulator::Now() < m_emlsrEnabledTime + m_fe2to3delay)
    {
        // we are interested in frames sent to test transition delay
        return;
    }

    auto taddr = psduMap.cbegin()->second->GetAddr2();
    std::size_t clientId;
    if (m_staMacs[0]->GetLinkIdByAddress(taddr))
    {
        clientId = 0;
    }
    else
    {
        NS_TEST_ASSERT_MSG_EQ(m_staMacs[1]->GetLinkIdByAddress(taddr).has_value(),
                              true,
                              "Unexpected TA for BlockAck: " << taddr);
        clientId = 1;
    }
    // we need the MLD address to check the status of the container queues
    auto addr = m_apMac->GetWifiRemoteStationManager(linkId)->GetMldAddress(taddr);
    NS_TEST_ASSERT_MSG_EQ(addr.has_value(), true, "MLD address not found for " << taddr);

    auto apPhy = m_apMac->GetWifiPhy(linkId);
    auto txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, apPhy->GetPhyBand());
    auto timeout = apPhy->GetSifs() + apPhy->GetSlot() + MicroSeconds(20);

    m_countBlockAck++;

    switch (m_countBlockAck)
    {
    case 4:
        // at the end of the PPDU carrying this BlockAck, the EMLSR client sending this
        // frame will start a timeout interval, after which it will start the transition to
        // the listening mode (such transition lasting the transition delay)

        // immediately before the end of the PPDU plus timeout, this link is not blocked
        // for the EMLSR client
        Simulator::Schedule(txDuration + timeout - NanoSeconds(1), [=]() {
            WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *addr, 0);
            auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
            NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                  true,
                                  "Expected to find a mask for EMLSR link "
                                      << +linkId << " of EMLSR client " << clientId);
            NS_TEST_EXPECT_MSG_EQ(mask->none(),
                                  true,
                                  "Expected EMLSR link " << +linkId << " of EMLSR client "
                                                         << clientId << " to be unblocked");
        });
        // immediately before the end of the PPDU plus timeout, the other links are blocked
        // for the EMLSR client
        Simulator::Schedule(txDuration + timeout - NanoSeconds(1), [=]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                if (id == linkId)
                {
                    continue;
                }

                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *addr, 0);
                auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                      true,
                                      "Expected to find a mask for EMLSR link "
                                          << +id << " of EMLSR client " << clientId);
                auto reason =
                    static_cast<std::size_t>(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
                NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                      true,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << clientId << " to be blocked");
                NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                      1,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << clientId
                                                             << " to be blocked "
                                                                " for one reason only");
            }
        });
        // immediately after the end of the PPDU plus timeout, all links are blocked for the EMLSR
        // client
        Simulator::Schedule(txDuration + timeout + MicroSeconds(1), [=]() {
            for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
            {
                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *addr, 0);
                auto mask = m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                      true,
                                      "Expected to find a mask for EMLSR link "
                                          << +id << " of EMLSR client " << clientId);
                auto reason = static_cast<std::size_t>(
                    WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY);
                NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                      true,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << clientId << " to be blocked");
                NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                      1,
                                      "Expected EMLSR link " << +id << " of EMLSR client "
                                                             << clientId
                                                             << " to be blocked "
                                                                " for one reason only");
            }
        });
        // immediately before the transition delay, all links are still blocked for the EMLSR client
        Simulator::Schedule(
            txDuration + timeout + m_transitionDelay.at(clientId) - NanoSeconds(1),
            [=]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *addr, 0);
                    auto mask =
                        m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                    NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                          true,
                                          "Expected to find a mask for EMLSR link "
                                              << +id << " of EMLSR client " << clientId);
                    auto reason = static_cast<std::size_t>(
                        WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY);
                    NS_TEST_EXPECT_MSG_EQ(mask->test(reason),
                                          true,
                                          "Expected EMLSR link " << +id << " of EMLSR client "
                                                                 << clientId << " to be blocked");
                    NS_TEST_EXPECT_MSG_EQ(mask->count(),
                                          1,
                                          "Expected EMLSR link " << +id << " of EMLSR client "
                                                                 << clientId
                                                                 << " to be blocked "
                                                                    " for one reason only");
                }
            });
        // immediately after the transition delay, all links are unblocked for the EMLSR client
        Simulator::Schedule(
            txDuration + timeout + m_transitionDelay.at(clientId) + MicroSeconds(1),
            [=]() {
                for (uint8_t id = 0; id < m_apMac->GetNLinks(); id++)
                {
                    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *addr, 0);
                    auto mask =
                        m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, id);
                    NS_TEST_EXPECT_MSG_EQ(mask.has_value(),
                                          true,
                                          "Expected to find a mask for EMLSR link "
                                              << +id << " of EMLSR client " << clientId);
                    NS_TEST_EXPECT_MSG_EQ(mask->none(),
                                          true,
                                          "Expected EMLSR link " << +id << " of EMLSR client "
                                                                 << clientId << " to be unblocked");
                }
            });

        // corrupt this BlockAck so that the AP MLD sends a BlockAckReq later on
        auto uid = psduMap.cbegin()->second->GetPacket()->GetUid();
        m_errorModel->SetList({uid});
        break;
    }
}

void
EmlsrDlTxopTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi EMLSR Test Suite
 */
class WifiEmlsrTestSuite : public TestSuite
{
  public:
    WifiEmlsrTestSuite();
};

WifiEmlsrTestSuite::WifiEmlsrTestSuite()
    : TestSuite("wifi-emlsr", UNIT)
{
    AddTestCase(new EmlOperatingModeNotificationTest(), TestCase::QUICK);
    AddTestCase(new EmlNotificationExchangeTest({1, 2}, MicroSeconds(0)), TestCase::QUICK);
    AddTestCase(new EmlNotificationExchangeTest({1, 2}, MicroSeconds(2048)), TestCase::QUICK);
    AddTestCase(new EmlNotificationExchangeTest({0, 1, 2, 3}, MicroSeconds(0)), TestCase::QUICK);
    AddTestCase(new EmlNotificationExchangeTest({0, 1, 2, 3}, MicroSeconds(2048)), TestCase::QUICK);
    for (const auto& emlsrLinks : {std::set<uint8_t>{0, 1, 2},
                                   std::set<uint8_t>{1, 2},
                                   std::set<uint8_t>{0, 2},
                                   std::set<uint8_t>{0, 1}})
    {
        AddTestCase(new EmlsrDlTxopTest(1,
                                        0,
                                        emlsrLinks,
                                        {MicroSeconds(32)},
                                        {MicroSeconds(32)},
                                        MicroSeconds(512)),
                    TestCase::QUICK);
        AddTestCase(new EmlsrDlTxopTest(1,
                                        1,
                                        emlsrLinks,
                                        {MicroSeconds(64)},
                                        {MicroSeconds(64)},
                                        MicroSeconds(512)),
                    TestCase::QUICK);
        AddTestCase(new EmlsrDlTxopTest(2,
                                        2,
                                        emlsrLinks,
                                        {MicroSeconds(128), MicroSeconds(256)},
                                        {MicroSeconds(128), MicroSeconds(256)},
                                        MicroSeconds(512)),
                    TestCase::QUICK);
    }
}

static WifiEmlsrTestSuite g_wifiEmlsrTestSuite; ///< the test suite
