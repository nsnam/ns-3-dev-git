/*
 * Copyright (c) 2024 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/arp-header.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/config.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/llc-snap-header.h"
#include "ns3/mobility-helper.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-mlo-test.h"
#include "ns3/wifi-net-device.h"

#include <array>
#include <list>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiMloUdpTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test UDP packet transmission between MLDs and SLDs.
 *
 * This test sets up an AP MLD and two non-AP MLDs having a variable number of links (possibly one).
 * The RF channels to set each link to are provided as input parameters. This test aims at veryfing
 * the successful transmission and reception of UDP packets in different traffic scenarios (from
 * the first station to the AP, from the AP to the first station, from one station to another).
 * The number of transmitted ARP Request/Reply frames is verified, as well as the source HW address
 * they carry. Specifically:
 *
 * STA to AP
 * ---------
 * The source HW address of the ARP Request sent by the STA is:
 * - the address of the link used to associate, if STA performs legacy association or AP is an SLD
 * - the non-AP MLD address (or the unique STA address), otherwise
 * The source HW address of the ARP Reply sent by the AP is the address of the link used by STA to
 * associate, if the STA performed legacy association, or the AP MLD address, otherwise.
 *
 * AP to STA
 * ---------
 * The source HW address of the ARP Request sent by the AP is:
 * - the unique AP address, if the AP is an SLD
 * - the AP MLD address, if the AP is an MLD
 * The source HW address of the ARP Reply sent by the STA is the address of the link used by STA to
 * associate, if the STA performed legacy association, or the non-AP MLD address, otherwise.
 *
 * STA 1 to STA 2
 * --------------
 * The source HW address of the ARP Request sent by STA 1 is:
 * - the address of the link used to associate, if STA performs legacy association or AP is an SLD
 * - the non-AP MLD address (or the unique STA address), otherwise
 * The source HW address of the ARP Reply sent by STA 2 is the address of the link used by STA 2 to
 * associate, if STA 2 performed legacy association, or the non-AP MLD address of STA 2, otherwise.
 */
class WifiMloUdpTest : public MultiLinkOperationsTestBase
{
  public:
    /**
     * Constructor
     *
     * @param apChannels string specifying channels for AP
     * @param firstStaChannels string specifying channels for first STA
     * @param secondStaChannels string specifying channels for second STA
     * @param trafficPattern the pattern of traffic to generate
     * @param assocType the type of association procedure for non-AP devices
     * @param amsduAggr whether A-MSDU aggregation is enabled
     */
    WifiMloUdpTest(const std::vector<std::string>& apChannels,
                   const std::vector<std::string>& firstStaChannels,
                   const std::vector<std::string>& secondStaChannels,
                   WifiTrafficPattern trafficPattern,
                   WifiAssocType assocType,
                   bool amsduAggr);

  protected:
    void DoSetup() override;
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW) override;
    void DoRun() override;

    /**
     * Check source and destination hardware addresses in ARP request frames.
     *
     * @param arp the ARP header
     * @param sender the MAC address of the sender (Address 2 field)
     * @param linkId the ID of the link on which the ARP frame is transmitted
     */
    void CheckArpRequestHwAddresses(const ArpHeader& arp, Mac48Address sender, uint8_t linkId);

    /**
     * Check source and destination hardware addresses in ARP reply frames.
     *
     * @param arp the ARP header
     * @param sender the MAC address of the sender (Address 2 field)
     * @param linkId the ID of the link on which the ARP frame is transmitted
     */
    void CheckArpReplyHwAddresses(const ArpHeader& arp, Mac48Address sender, uint8_t linkId);

  private:
    void StartTraffic() override;

    const std::vector<std::string> m_2ndStaChannels; ///< string specifying channels for second STA
    WifiTrafficPattern m_trafficPattern;             ///< the pattern of traffic to generate
    WifiAssocType m_assocType;                       //!< association type
    bool m_amsduAggr;                                ///< whether A-MSDU aggregation is enabled
    const std::size_t m_nPackets{3};                 ///< number of application packets to generate
    Ipv4InterfaceContainer m_staInterfaces;          ///< IP interfaces for non-AP MLDs
    Ipv4InterfaceContainer m_apInterface;            ///< IP interface for AP MLD
    const uint16_t m_udpPort{50000};                 ///< UDP port for application servers
    Ptr<UdpServer> m_sink;                           ///< server app on the receiving node
    std::size_t m_nArpRequest{0};        ///< counts how many ARP Requests are transmitted
    std::size_t m_nCheckedArpRequest{0}; ///< counts how many ARP Requests are checked
    std::size_t m_nArpReply{0};          ///< counts how many ARP Replies are transmitted
    std::size_t m_nCheckedArpReply{0};   ///< counts how many ARP Replies are checked
};

WifiMloUdpTest::WifiMloUdpTest(const std::vector<std::string>& apChannels,
                               const std::vector<std::string>& firstStaChannels,
                               const std::vector<std::string>& secondStaChannels,
                               WifiTrafficPattern trafficPattern,
                               WifiAssocType assocType,
                               bool amsduAggr)
    : MultiLinkOperationsTestBase(
          std::string("Check UDP packet transmission between MLDs ") +
              " (#AP_links: " + std::to_string(apChannels.size()) +
              ", #STA_1_links: " + std::to_string(firstStaChannels.size()) +
              ", #STA_2_links: " + std::to_string(secondStaChannels.size()) +
              ", Traffic pattern: " + std::to_string(static_cast<uint8_t>(trafficPattern)) +
              ", Assoc type: " + (assocType == WifiAssocType::LEGACY ? "Legacy" : "ML setup") +
              ", A-MSDU aggregation: " + std::to_string(amsduAggr) + ")",
          2,
          BaseParams{firstStaChannels, apChannels, {}}),
      m_2ndStaChannels(secondStaChannels),
      m_trafficPattern(trafficPattern),
      m_assocType(assocType),
      m_amsduAggr(amsduAggr)
{
}

void
WifiMloUdpTest::DoSetup()
{
    Config::SetDefault("ns3::WifiMac::BE_MaxAmsduSize", UintegerValue(m_amsduAggr ? 4000 : 0));

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(3);
    int64_t streamNumber = 30;

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes(2);

    WifiHelper wifi;
    // WifiHelper::EnableLogComponents();
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs0"),
                                 "ControlMode",
                                 StringValue("HtMcs0"));

    ChannelMap channelMap{{WIFI_SPECTRUM_2_4_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_5_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_6_GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    SpectrumWifiPhyHelper staPhyHelper1;
    SpectrumWifiPhyHelper staPhyHelper2;
    SpectrumWifiPhyHelper apPhyHelper;
    SetChannels(staPhyHelper1, m_staChannels, channelMap);
    SetChannels(staPhyHelper2, m_2ndStaChannels, channelMap);
    SetChannels(apPhyHelper, m_apChannels, channelMap);

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac", // default SSID
                "ActiveProbing",
                BooleanValue(false),
                "AssocType",
                EnumValue(m_assocType));

    NetDeviceContainer staDevices = wifi.Install(staPhyHelper1, mac, wifiStaNodes.Get(0));

    mac.SetType("ns3::StaWifiMac", // default SSID
                "ActiveProbing",
                BooleanValue(false),
                "AssocType",
                EnumValue(m_assocType));

    staDevices.Add(wifi.Install(staPhyHelper2, mac, wifiStaNodes.Get(1)));

    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("ns-3-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

    NetDeviceContainer apDevices = wifi.Install(apPhyHelper, mac, wifiApNode);

    // Uncomment the lines below to write PCAP files
    // apPhyHelper.EnablePcap("wifi-mlo_AP", apDevices);
    // staPhyHelper1.EnablePcap("wifi-mlo_STA", staDevices);

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(apDevices, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

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
            MakeCallback(&WifiMloUdpTest::Transmit, this).Bind(m_apMac, phyId));
    }
    for (uint8_t i = 0; i < m_nStations; i++)
    {
        for (uint8_t phyId = 0; phyId < m_staMacs[i]->GetDevice()->GetNPhys(); phyId++)
        {
            Config::ConnectWithoutContext(
                "/NodeList/" + std::to_string(i + 1) + "/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                    std::to_string(phyId) + "/PhyTxPsduBegin",
                MakeCallback(&WifiMloUdpTest::Transmit, this).Bind(m_staMacs[i], phyId));
        }
    }

    // install Internet stack on all nodes
    InternetStackHelper stack;
    auto allNodes = NodeContainer::GetGlobal();
    stack.Install(allNodes);
    streamNumber += stack.AssignStreams(allNodes, streamNumber);

    Ipv4AddressHelper address;
    address.SetBase("10.1.0.0", "255.255.255.0");
    m_apInterface = address.Assign(NetDeviceContainer(m_apMac->GetDevice()));
    for (std::size_t i = 0; i < m_nStations; i++)
    {
        m_staInterfaces.Add(address.Assign(NetDeviceContainer(m_staMacs[i]->GetDevice())));
    }

    // install a UDP server on all nodes
    UdpServerHelper serverHelper(m_udpPort);
    auto serverApps = serverHelper.Install(allNodes);
    serverApps.Start(Seconds(0.0));
    serverApps.Stop(m_duration);

    switch (m_trafficPattern)
    {
    case WifiTrafficPattern::STA_TO_AP:
        m_sink = DynamicCast<UdpServer>(serverApps.Get(0));
        break;
    case WifiTrafficPattern::AP_TO_STA:
        m_sink = DynamicCast<UdpServer>(serverApps.Get(1));
        break;
    case WifiTrafficPattern::STA_TO_STA:
        m_sink = DynamicCast<UdpServer>(serverApps.Get(2));
        break;
    default:
        m_sink = nullptr; // other cases are not supported
    }

    m_startAid = m_apMac->GetNextAssociationId();

    // schedule association/ML setup for one station at a time
    m_apMac->TraceConnectWithoutContext("AssociatedSta",
                                        MakeCallback(&WifiMloUdpTest::SetSsid, this));
    m_staMacs[0]->SetSsid(Ssid("ns-3-ssid"));
}

void
WifiMloUdpTest::StartTraffic()
{
    Ptr<WifiMac> srcMac;
    Ipv4Address destAddr;

    switch (m_trafficPattern)
    {
    case WifiTrafficPattern::STA_TO_AP:
        srcMac = m_staMacs[0];
        destAddr = m_apInterface.GetAddress(0);
        break;
    case WifiTrafficPattern::AP_TO_STA:
        srcMac = m_apMac;
        destAddr = m_staInterfaces.GetAddress(0);
        break;
    case WifiTrafficPattern::STA_TO_STA:
        srcMac = m_staMacs[0];
        destAddr = m_staInterfaces.GetAddress(1);
        break;
    default:
        NS_ABORT_MSG("Unsupported scenario " << +static_cast<uint8_t>(m_trafficPattern));
    }

    UdpClientHelper clientHelper(InetSocketAddress(destAddr, m_udpPort));
    clientHelper.SetAttribute("MaxPackets", UintegerValue(m_nPackets));
    clientHelper.SetAttribute("Interval", TimeValue(Time{0}));
    clientHelper.SetAttribute("PacketSize", UintegerValue(100));
    auto clientApp = clientHelper.Install(srcMac->GetDevice()->GetNode());
    clientApp.Start(Seconds(0.0));
    clientApp.Stop(m_duration);
}

void
WifiMloUdpTest::Transmit(Ptr<WifiMac> mac,
                         uint8_t phyId,
                         WifiConstPsduMap psduMap,
                         WifiTxVector txVector,
                         double txPowerW)
{
    MultiLinkOperationsTestBase::Transmit(mac, phyId, psduMap, txVector, txPowerW);

    auto psdu = psduMap.begin()->second;
    auto linkId = m_txPsdus.back().linkId;

    if (!psdu->GetHeader(0).IsQosData() || !psdu->GetHeader(0).HasData())
    {
        // we are interested in ARP Request/Reply frames, which are carried by QoS data frames
        return;
    }

    for (const auto& mpdu : *PeekPointer(psdu))
    {
        std::list<Ptr<const Packet>> packets;

        if (mpdu->GetHeader().IsQosAmsdu())
        {
            for (const auto& msdu : *PeekPointer(mpdu))
            {
                packets.push_back(msdu.first);
            }
        }
        else
        {
            packets.push_back(mpdu->GetPacket());
        }

        for (auto pkt : packets)
        {
            LlcSnapHeader llc;
            auto packet = pkt->Copy();
            packet->RemoveHeader(llc);

            if (llc.GetType() != ArpL3Protocol::PROT_NUMBER)
            {
                continue;
            }

            ArpHeader arp;
            packet->RemoveHeader(arp);

            if (arp.IsRequest())
            {
                CheckArpRequestHwAddresses(arp, psdu->GetAddr2(), linkId);
            }
            if (arp.IsReply())
            {
                CheckArpReplyHwAddresses(arp, psdu->GetAddr2(), linkId);
            }
        }
    }
}

void
WifiMloUdpTest::CheckArpRequestHwAddresses(const ArpHeader& arp,
                                           Mac48Address sender,
                                           uint8_t linkId)
{
    ++m_nArpRequest;

    // source and destination HW addresses cannot be checked for forwarded frames because
    // they can be forwarded on different links
    if (auto srcMac =
            (m_trafficPattern == WifiTrafficPattern::AP_TO_STA ? StaticCast<WifiMac>(m_apMac)
                                                               : StaticCast<WifiMac>(m_staMacs[0]));
        !srcMac->GetLinkIdByAddress(sender) && sender != srcMac->GetAddress())
    {
        // the sender address is not the MLD address nor a link address of the source device
        return;
    }

    Mac48Address expectedSrc;

    switch (m_trafficPattern)
    {
    case WifiTrafficPattern::STA_TO_AP:
    case WifiTrafficPattern::STA_TO_STA:
        // if the ARP Request is sent by a STA, the source HW address is:
        // - the address of the link used to associate, if the STA performs legacy association or
        //   the AP is an SLD
        // - the non-AP MLD address (or the unique STA address), otherwise
        expectedSrc = (m_assocType == WifiAssocType::LEGACY || m_apMac->GetNLinks() == 1)
                          ? m_staMacs[0]->GetFrameExchangeManager(linkId)->GetAddress()
                          : m_staMacs[0]->GetAddress();
        break;
    case WifiTrafficPattern::AP_TO_STA:
        // if the ARP Request is sent by an AP, the source HW address is:
        // - the unique AP address, if the AP is an SLD
        // - the AP MLD address, if the AP is an MLD
        expectedSrc = m_apMac->GetAddress();
        break;
    default:
        NS_ABORT_MSG("Unsupported scenario " << +static_cast<uint8_t>(m_trafficPattern));
    }

    ++m_nCheckedArpRequest;

    NS_TEST_EXPECT_MSG_EQ(Mac48Address::ConvertFrom(arp.GetSourceHardwareAddress()),
                          expectedSrc,
                          "Unexpected source HW address");
    NS_TEST_EXPECT_MSG_EQ(Mac48Address::ConvertFrom(arp.GetDestinationHardwareAddress()),
                          Mac48Address::GetBroadcast(),
                          "Unexpected destination HW address");
}

void
WifiMloUdpTest::CheckArpReplyHwAddresses(const ArpHeader& arp, Mac48Address sender, uint8_t linkId)
{
    ++m_nArpReply;

    // source and destination HW addresses cannot be checked for forwarded frames because
    // they can be forwarded on different links
    auto srcMac =
        (m_trafficPattern == WifiTrafficPattern::STA_TO_AP   ? StaticCast<WifiMac>(m_apMac)
         : m_trafficPattern == WifiTrafficPattern::AP_TO_STA ? StaticCast<WifiMac>(m_staMacs[0])
                                                             : StaticCast<WifiMac>(m_staMacs[1]));

    if (!srcMac->GetLinkIdByAddress(sender) && sender != srcMac->GetAddress())
    {
        // the sender address is not the MLD address nor a link address of the source device
        return;
    }

    // the source HW address of the ARP Reply is the address of the link on which the ARP Reply is
    // sent, if the sender performed legacy association, or the MLD address, otherwise
    Mac48Address expectedSrc = (m_assocType == WifiAssocType::LEGACY || m_apMac->GetNLinks() == 1)
                                   ? srcMac->GetFrameExchangeManager(linkId)->GetAddress()
                                   : srcMac->GetAddress();

    ++m_nCheckedArpReply;

    NS_TEST_EXPECT_MSG_EQ(Mac48Address::ConvertFrom(arp.GetSourceHardwareAddress()),
                          expectedSrc,
                          "Unexpected source HW address");
}

void
WifiMloUdpTest::DoRun()
{
    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_ASSERT_MSG_NE(m_sink, nullptr, "Sink is not set");
    NS_TEST_EXPECT_MSG_EQ(m_sink->GetReceived(),
                          m_nPackets,
                          "Unexpected number of received packets");

    std::size_t expectedNOrigArpRequest{0};
    std::size_t expectedNFwdArpRequest{0};

    switch (m_trafficPattern)
    {
    case WifiTrafficPattern::STA_TO_AP:
    case WifiTrafficPattern::STA_TO_STA:
        // STA transmits ARP Request on one link, AP retransmits it on all of its links
        expectedNOrigArpRequest = 1;
        expectedNFwdArpRequest = m_apMac->GetNLinks();
        break;
    case WifiTrafficPattern::AP_TO_STA:
        // AP transmits ARP Request on all of its links
        expectedNOrigArpRequest = m_apMac->GetNLinks();
        expectedNFwdArpRequest = 0;
        break;
    default:
        NS_ABORT_MSG("Unsupported scenario " << +static_cast<uint8_t>(m_trafficPattern));
    }

    NS_TEST_EXPECT_MSG_EQ(m_nArpRequest,
                          expectedNOrigArpRequest + expectedNFwdArpRequest,
                          "Unexpected number of transmitted ARP Request frames");
    NS_TEST_EXPECT_MSG_EQ(m_nCheckedArpRequest,
                          expectedNOrigArpRequest,
                          "Unexpected number of checked ARP Request frames");

    std::size_t expectedNOrigArpReply{0};
    std::size_t expectedNFwdArpReply{0};

    switch (m_trafficPattern)
    {
    case WifiTrafficPattern::STA_TO_AP:
        // STA transmits only one ARP Request, AP replies with one (unicast) ARP Reply
        expectedNOrigArpReply = 1;
        expectedNFwdArpReply = 0;
        break;
    case WifiTrafficPattern::AP_TO_STA:
        // ARP Request is broadcast, so it is sent by the AP on all of its links; the STA sends
        // an ARP Reply for each setup link
        expectedNOrigArpReply =
            std::min<std::size_t>(m_apMac->GetNLinks(), m_staMacs[0]->GetSetupLinkIds().size());
        expectedNFwdArpReply = 0;
        break;
    case WifiTrafficPattern::STA_TO_STA:
        // AP forwards ARP Request on all of its links; STA 2 sends as many ARP Replies as the
        // number of setup links; each such ARP Reply is forwarded by the AP to STA 1
        expectedNOrigArpReply = expectedNFwdArpReply = m_staMacs[1]->GetSetupLinkIds().size();
        break;
    default:
        NS_ABORT_MSG("Unsupported scenario " << +static_cast<uint8_t>(m_trafficPattern));
    }

    NS_TEST_EXPECT_MSG_EQ(m_nArpReply,
                          expectedNOrigArpReply + expectedNFwdArpReply,
                          "Unexpected number of transmitted ARP Reply frames");
    NS_TEST_EXPECT_MSG_EQ(m_nCheckedArpReply,
                          expectedNOrigArpReply,
                          "Unexpected number of checked ARP Reply frames");

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Multi-Link Operations with UDP traffic Test Suite
 */
class WifiMloUdpTestSuite : public TestSuite
{
  public:
    WifiMloUdpTestSuite();
};

WifiMloUdpTestSuite::WifiMloUdpTestSuite()
    : TestSuite("wifi-mlo-udp", Type::SYSTEM)
{
    using ParamsTuple = std::array<std::vector<std::string>, 3>; // AP, STA 1, STA 2 channels

    for (const auto& channels :
         {// single link AP, non-AP MLD with 3 links, single link non-AP STA
          ParamsTuple{
              {{"{7, 80, BAND_6GHZ, 0}"},
               {"{42, 80, BAND_5GHZ, 2}", "{5, 40, BAND_2_4GHZ, 0}", "{7, 80, BAND_6GHZ, 0}"},
               {"{7, 80, BAND_6GHZ, 0}"}}},
          // single link AP, non-AP MLD with 3 links, non-AP MLD with 2 links
          ParamsTuple{
              {{"{7, 80, BAND_6GHZ, 0}"},
               {"{42, 80, BAND_5GHZ, 2}", "{5, 40, BAND_2_4GHZ, 0}", "{7, 80, BAND_6GHZ, 0}"},
               {"{42, 80, BAND_5GHZ, 2}", "{7, 80, BAND_6GHZ, 0}"}}},
          // AP MLD with 3 links, single link non-AP STA, non-AP MLD with 2 links
          ParamsTuple{
              {{"{42, 80, BAND_5GHZ, 2}", "{5, 40, BAND_2_4GHZ, 0}", "{7, 80, BAND_6GHZ, 0}"},
               {"{7, 80, BAND_6GHZ, 0}"},
               {"{42, 80, BAND_5GHZ, 2}", "{5, 40, BAND_2_4GHZ, 0}"}}},
          // AP MLD with 3 links, non-AP MLD with 3 links, non-AP MLD with 2 links
          ParamsTuple{
              {{"{42, 80, BAND_5GHZ, 2}", "{7, 80, BAND_6GHZ, 0}", "{5, 40, BAND_2_4GHZ, 0}"},
               {"{42, 80, BAND_5GHZ, 2}", "{7, 80, BAND_6GHZ, 0}", "{5, 40, BAND_2_4GHZ, 0}"},
               {"{7, 80, BAND_6GHZ, 0}", "{5, 40, BAND_2_4GHZ, 0}"}}}})
    {
        for (const auto& trafficPattern : {WifiTrafficPattern::STA_TO_AP,
                                           WifiTrafficPattern::AP_TO_STA,
                                           WifiTrafficPattern::STA_TO_STA})
        {
            for (const auto amsduAggr : {false, true})
            {
                for (const auto assocType : {WifiAssocType::LEGACY, WifiAssocType::ML_SETUP})
                {
                    AddTestCase(new WifiMloUdpTest(channels[0],
                                                   channels[1],
                                                   channels[2],
                                                   trafficPattern,
                                                   assocType,
                                                   amsduAggr),
                                TestCase::Duration::QUICK);
                }
            }
        }
    }
}

static WifiMloUdpTestSuite g_wifiMloUdpTestSuite; ///< the test suite
