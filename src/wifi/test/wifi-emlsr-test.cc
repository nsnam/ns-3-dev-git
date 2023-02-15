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
#include "ns3/eht-configuration.h"
#include "ns3/emlsr-manager.h"
#include "ns3/header-serialization-test.h"
#include "ns3/log.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/spectrum-wifi-phy.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ppdu.h"
#include "ns3/wifi-psdu.h"

#include <algorithm>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiEmlsrTest");

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
    std::vector<FrameInfo> m_txPsdus;       ///< transmitted PSDUs
    Ptr<ApWifiMac> m_apMac;                 ///< AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs; ///< MACs of the non-AP MLDs
    uint16_t m_lastAid{0};                  ///< AID of last associated station
    Time m_duration{0};                     ///< simulation duration

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

    for (const auto& [aid, psdu] : psduMap)
    {
        std::stringstream ss;
        ss << std::setprecision(10) << "PSDU #" << m_txPsdus.size() << " Link ID " << +linkId << " "
           << psdu->GetHeader(0).GetTypeString() << " #MPDUs " << psdu->GetNMpdus()
           << " duration/ID " << psdu->GetHeader(0).GetDuration() << " RA = " << psdu->GetAddr1()
           << " TA = " << psdu->GetAddr2() << " ADDR3 = " << psdu->GetHeader(0).GetAddr3()
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
    NS_LOG_INFO("TXVECTOR = " << txVector << "\n");
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

    // Trace PSDUs passed to the PHY on AP MLD and EMLSR client only
    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
    {
        Config::Connect("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                            std::to_string(linkId) + "/PhyTxPsduBegin",
                        MakeCallback(&EmlsrOperationsTestBase::Transmit, this).Bind(linkId));
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

    // schedule ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext("AssociatedSta",
                                        MakeCallback(&EmlsrOperationsTestBase::SetSsid, this));
    Simulator::Schedule(Seconds(0), [&]() { m_staMacs[0]->SetSsid(Ssid("ns-3-ssid")); });
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

    // make the next STA to start ML discovery & setup
    if (aid < m_nEmlsrStations + m_nNonEmlsrStations)
    {
        m_staMacs[aid]->SetSsid(Ssid("ns-3-ssid"));
        return;
    }
    // wait some time (5ms) to allow the completion of association before generating traffic
    Simulator::Schedule(MilliSeconds(5), &EmlsrOperationsTestBase::StartTraffic, this);
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
}

static WifiEmlsrTestSuite g_wifiEmlsrTestSuite; ///< the test suite
