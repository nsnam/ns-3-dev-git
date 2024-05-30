/*
 * Copyright (c) 2024 Indraprastha Institute of Information Technology Delhi
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/eht-configuration.h"
#include "ns3/enum.h"
#include "ns3/he-phy.h"
#include "ns3/integer.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/names.h"
#include "ns3/node-list.h"
#include "ns3/object.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-co-trace-helper.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-mpdu.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-phy-state.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;
NS_LOG_COMPONENT_DEFINE("WifiCoTraceHelperTest");

/**
 * @ingroup wifi-test
 * @brief It's a base class with some utility methods for other test cases in this file.
 */
class WifiCoTraceHelperBaseTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param testName Name of a test.
     */
    WifiCoTraceHelperBaseTestCase(const std::string& testName)
        : TestCase(testName)
    {
    }

  protected:
    /**
     * It gets the channel occupancy of a link on a node measured by WifiCoTraceHelper.
     *
     * @param nodeId Id of a node.
     * @param linkId Id of link.
     * @param coHelper Helper to obtain channel occupancy
     * @return Statistics measured by WifiCoTraceHelper corresponding to nodeId,linkId.
     */
    const std::map<WifiPhyState, Time>& GetChannelOccupancy(size_t nodeId,
                                                            size_t linkId,
                                                            WifiCoTraceHelper& coHelper);

    /**
     * A helper function that sets tid-to-link mapping.
     *
     * @param mapping A string that configure tid-to-link mapping.
     */
    void ConfigureTidToLinkMapping(const std::string& mapping);

    /**
     * A helper function that installs PacketSocketServer on a node.
     *
     * @param node The node on which the server is installed.
     * @param startAfter The server starts after a delay of "startAfter" duration.
     * @param prot The PacketSocketAddress of the server uses "prot" as its protocol number.
     */
    void InstallPacketSocketServer(Ptr<Node> node, Time startAfter, size_t prot);

    /**
     * A helper function that installs PacketSocketClient application on a node.
     *
     * @param client The node on which the client is installed.
     * @param server A node on which PacketSocketServer is installed and to whom the client sends
     * requests.
     * @param prot The protocol number used by both the client and the server.
     * @param startAfter The client starts after a delay of "startAfter" duration.
     * @param numPkts The client sends these many packets to server immediately.
     * @param pktLen The length of each packet in bytes.
     * @param tid The tid of each packet.
     * @return A pointer to the installed PacketSocketClient application.
     */
    Ptr<PacketSocketClient> InstallPacketSocketClient(Ptr<Node> client,
                                                      Ptr<Node> server,
                                                      size_t prot,
                                                      Time startAfter,
                                                      size_t numPkts,
                                                      size_t pktLen,
                                                      size_t tid);

    /**
     * A helper function that schedules to send given number of packets from one node to another.
     *
     * @param from The node which sends the packets.
     * @param to The node which receives the packets.
     * @param startAfter The packets are scheduled to send after this much duration.
     * @param numPkts The number of packets.
     * @param pktLen The length of each packet in bytes.
     * @param tid The optional tid of each packet. The default value is "0".
     */
    void SchedulePackets(Ptr<Node> from,
                         Ptr<Node> to,
                         Time startAfter,
                         size_t numPkts,
                         size_t pktLen,
                         size_t tid);

    /**
     * A helper function that disables frame aggregation.
     */
    void DisableAggregation();

    Time m_simulationStop{Seconds(5.0)}; ///< Instant at which simulation should stop.
    NodeContainer m_nodes;               ///< Container of all nodes instantiated in this test case.
    NetDeviceContainer m_devices; ///< Container of all devices instantiated in this test case.
    size_t protocol =
        1; ///< A unique Protocol used in each PacketSocketClient and PacketSocketServer pair.
};

const std::map<WifiPhyState, Time>&
WifiCoTraceHelperBaseTestCase::GetChannelOccupancy(size_t nodeId,
                                                   size_t linkId,
                                                   WifiCoTraceHelper& coHelper)
{
    const auto& devRecords = coHelper.GetDeviceRecords();
    auto senderRecord = std::find_if(devRecords.cbegin(), devRecords.cend(), [nodeId](auto& x) {
        return x.m_nodeId == nodeId;
    });

    NS_ASSERT_MSG((senderRecord != devRecords.end()), "Expected statistics for nodeId: " << nodeId);

    auto stats = senderRecord->m_linkStateDurations.find(linkId);
    auto endItr = senderRecord->m_linkStateDurations.end();
    NS_ASSERT_MSG((stats != endItr),
                  "Expected statistics at nodeId: " << nodeId << ", linkId: " << linkId);

    return stats->second;
}

void
WifiCoTraceHelperBaseTestCase::ConfigureTidToLinkMapping(const std::string& mapping)
{
    for (size_t i = 0; i < m_devices.GetN(); ++i)
    {
        auto wifiDevice = DynamicCast<WifiNetDevice>(m_devices.Get(i));
        wifiDevice->GetMac()->GetEhtConfiguration()->SetAttribute(
            "TidToLinkMappingNegSupport",
            EnumValue(WifiTidToLinkMappingNegSupport::ANY_LINK_SET));

        wifiDevice->GetMac()->GetEhtConfiguration()->SetAttribute("TidToLinkMappingUl",
                                                                  StringValue(mapping));
    }
}

/**
 * Install Packet Socket Server on a node. A Packet Socket client generates an uplink flow and sends
 * it to the server.
 */
void
WifiCoTraceHelperBaseTestCase::InstallPacketSocketServer(Ptr<Node> node,
                                                         Time startAfter,
                                                         size_t prot)
{
    Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice>(node->GetDevice(0));
    PacketSocketAddress srvAddr;
    srvAddr.SetSingleDevice(device->GetIfIndex());
    srvAddr.SetProtocol(prot);
    auto psServer = CreateObject<PacketSocketServer>();
    psServer->SetLocal(srvAddr);
    node->AddApplication(psServer);
    psServer->SetStartTime(startAfter);
}

/**
 * Install packet socket client that generates an uplink flow on a node.
 */
Ptr<PacketSocketClient>
WifiCoTraceHelperBaseTestCase::InstallPacketSocketClient(Ptr<Node> client,
                                                         Ptr<Node> server,
                                                         size_t prot,
                                                         Time startAfter,
                                                         size_t numPkts,
                                                         size_t pktLen,
                                                         size_t tid)
{
    NS_LOG_INFO("Start Flow on node:" << client->GetId()
                                      << " at:" << Simulator::Now() + startAfter);

    Ptr<WifiNetDevice> staDevice = DynamicCast<WifiNetDevice>(client->GetDevice(0));
    PacketSocketAddress sockAddr;
    sockAddr.SetSingleDevice(staDevice->GetIfIndex());
    sockAddr.SetPhysicalAddress(server->GetDevice(0)->GetAddress());
    sockAddr.SetProtocol(prot);

    auto clientApp = CreateObject<PacketSocketClient>();
    clientApp->SetAttribute("PacketSize", UintegerValue(pktLen));
    clientApp->SetAttribute("MaxPackets", UintegerValue(numPkts));
    clientApp->SetAttribute("Interval", TimeValue(Seconds(0))); // Send packets immediately
    clientApp->SetAttribute("Priority", UintegerValue(tid));
    clientApp->SetRemote(sockAddr);
    clientApp->SetStartTime(startAfter);

    staDevice->GetNode()->AddApplication(clientApp);
    return clientApp;
}

void
WifiCoTraceHelperBaseTestCase::SchedulePackets(Ptr<Node> from,
                                               Ptr<Node> to,
                                               Time startDelay,
                                               size_t numPkts,
                                               size_t pktLen,
                                               size_t tid = 0)
{
    /* Install a PacketSocket server and client pair with a unique protocol on each invocation. */
    this->protocol++;

    InstallPacketSocketServer(to, startDelay, this->protocol);
    InstallPacketSocketClient(from, to, this->protocol, startDelay, numPkts, pktLen, tid);
}

void
WifiCoTraceHelperBaseTestCase::DisableAggregation()
{
    for (size_t i = 0; i < this->m_nodes.GetN(); i++)
    {
        for (uint32_t j = 0; j < this->m_nodes.Get(i)->GetNDevices(); j++)
        {
            auto device = DynamicCast<WifiNetDevice>(m_nodes.Get(i)->GetDevice(j));
            if (!device)
            {
                continue;
            }

            device->GetMac()->SetAttribute("BE_MaxAmpduSize", UintegerValue(0));
            device->GetMac()->SetAttribute("BK_MaxAmpduSize", UintegerValue(0));
            device->GetMac()->SetAttribute("VO_MaxAmpduSize", UintegerValue(0));
            device->GetMac()->SetAttribute("VI_MaxAmpduSize", UintegerValue(0));

            device->GetMac()->SetAttribute("BE_MaxAmsduSize", UintegerValue(0));
            device->GetMac()->SetAttribute("BK_MaxAmsduSize", UintegerValue(0));
            device->GetMac()->SetAttribute("VO_MaxAmsduSize", UintegerValue(0));
            device->GetMac()->SetAttribute("VI_MaxAmsduSize", UintegerValue(0));
        }
    }
}

/**
 * @ingroup wifi-test
 * @brief Send one packet from one WifiNetDevice to other.
 *
 * This test case sends a single packet from one wifi device to another, operating in Adhoc mode,
 * and compares the TX duration measured by WifiCoTraceHelper with the analytically calculated
 * value.
 */
class SendOnePacketTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    SendOnePacketTestCase();

  private:
    /** Executes test case and assertions. */
    void DoRun() override;
    /** Setup test case's scenario. */
    void DoSetup() override;
};

SendOnePacketTestCase::SendOnePacketTestCase()
    : WifiCoTraceHelperBaseTestCase(
          "SendOnePacketTestCase: Send one packet from one WifiNetDevice to other.")
{
}

void
SendOnePacketTestCase::DoRun()
{
    size_t senderNodeId = 1;
    size_t recNodeId = 0;
    Ptr<Node> sender = m_nodes.Get(senderNodeId);
    Ptr<Node> receiver = m_nodes.Get(recNodeId);

    // Send a packet to initiate one-time control frame exchanges in Adhoc mode.
    // To avoid one-time control frames, we will not measure channel occupancy during transmission
    // of this packet.
    SchedulePackets(sender, receiver, Seconds(0.5), 1 /*num of packets*/, 1000 /*packet length*/);

    // Send a packet that requires only one symbol to transmit and measure its channel occupancy.
    /**
     * Size of frame in Bytes = 204 (payload) + 34 (Mac & Phy Headers) = 238
     * Frame Size in bits + Phy service and tail bits = 238*8 + 22 = 1926
     * Phy rate at MCS 11, 40 MHz, 800 ns GI, 1 spatial stream = 286.6 Mbps
     * Symbol duration = 12.8 + 0.8 = 13.6 us
     * Number of symbols = ceil(1926 / (286.6 * 13.6)) = 1
     */
    Time scheduleAt{Seconds(1.0)};
    size_t packetLen = 200; /*In Bytes*/
    SchedulePackets(sender, receiver, scheduleAt, 1 /*num of packets*/, packetLen);
    WifiCoTraceHelper helper1{scheduleAt, scheduleAt + Seconds(0.1)};
    helper1.Enable(m_nodes);

    // Send a packet that requires two symbols to transmit and measure its channel occupancy.
    /**
     * If we replace payload size from 204 to 504 in the above calculation in comments, then number
     * of symbols equals 2.
     * Number of symbols = ceil(4326 / (286.6 * 13.6)) = 2
     */
    scheduleAt = Seconds(1.5);
    packetLen = 500; /*In Bytes*/
    SchedulePackets(sender, receiver, scheduleAt, 1 /*num of packets*/, packetLen);
    WifiCoTraceHelper helper2{scheduleAt, scheduleAt + Seconds(0.1)};
    helper2.Enable(m_nodes);

    // Send a packet that requires three symbols to transmit and measure its channel occupancy.
    /**
     * If we replace payload size from 204 to 1004 in the above calculation in comments, then number
     * of symbols equals 3.
     * Number of symbols = ceil(8326 / (286.6 * 13.6)) = 2
     */
    scheduleAt = Seconds(2.0);
    packetLen = 1000; /*In Bytes*/
    SchedulePackets(sender, receiver, scheduleAt, 1 /*num of packets*/, packetLen);
    WifiCoTraceHelper helper3{scheduleAt, scheduleAt + Seconds(0.1)};
    helper3.Enable(m_nodes);

    Simulator::Stop(Seconds(2.5));

    Simulator::Run();
    Simulator::Destroy();

    /**
     * Data Packet
     * ===========
     * Preamble Duration = 48 us
     * Symbol Duration = 12.8 + 0.8 (Guard Interval) = 13.6 us
     * Tx Duration of a packet requiring 1 symbol  = (1 * 13.6) + 48 = 61.6 us
     * Tx Duration of a packet requiring 2 symbols = (2 * 13.6) + 48 = 75.2 us
     * Tx Duration of a packet requiring 3 symbols = (3 * 13.6) + 48 = 88.8 us
     */

    auto map1 = GetChannelOccupancy(senderNodeId, 0 /*linkId*/, helper1);
    NS_TEST_ASSERT_MSG_EQ(map1.at(WifiPhyState::TX),
                          NanoSeconds(61600),
                          "TX duration does not match");

    auto map2 = GetChannelOccupancy(senderNodeId, 0 /*linkId*/, helper2);
    NS_TEST_ASSERT_MSG_EQ(map2.at(WifiPhyState::TX),
                          NanoSeconds(75200),
                          "TX duration does not match");

    auto map3 = GetChannelOccupancy(senderNodeId, 0 /*linkId*/, helper3);
    NS_TEST_ASSERT_MSG_EQ(map3.at(WifiPhyState::TX),
                          NanoSeconds(88800),
                          "TX duration does not match");

    /**
     * Acknowledgement Packet
     * ======================
     * Preamble Duration = 20 us
     * Symbol Duration = 4 us
     * Number of symbols = 2
     * Tx Duration of Ack packet = 2*4 + 20 = 28 us
     */
    auto ackMap = GetChannelOccupancy(recNodeId, 0 /*linkId*/, helper1);
    NS_TEST_ASSERT_MSG_EQ(ackMap.at(WifiPhyState::TX),
                          NanoSeconds(28000),
                          "TX duration does not match");
}

void
SendOnePacketTestCase::DoSetup()
{
    std::string mcs{"11"};

    uint32_t nWifi = 2;
    m_nodes.Create(nWifi);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    // Create multiple spectrum channels
    Ptr<MultiModelSpectrumChannel> spectrumChannel5Ghz = CreateObject<MultiModelSpectrumChannel>();

    SpectrumWifiPhyHelper phy(1);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);

    // configure operating channel for each link
    phy.Set(0, "ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));

    uint8_t linkId = 0;
    wifi.SetRemoteStationManager(linkId,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs" + mcs),
                                 "ControlMode",
                                 StringValue("OfdmRate24Mbps"));

    mac.SetType("ns3::AdhocWifiMac");
    m_devices = wifi.Install(phy, mac, m_nodes);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    auto distance = 0.1;
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    /* Disable aggregation and set guard interval */
    DisableAggregation();
    int gi = 800; // Guard Interval in nano seconds
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval",
                TimeValue(NanoSeconds(gi)));

    PacketSocketHelper helper;
    helper.Install(m_nodes);
}

/**
 * @ingroup wifi-test
 * @brief Trace channel occupany on each link of MLDs
 *
 * This test case sends a packet on each link of a MLD and asserts that WifiCoTraceHelper measures
 * TX duration correctly on every link. It uses tid-to-link mapping to schedule a packet to a
 * specific link.
 *
 */
class MLOTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    MLOTestCase();

  private:
    void DoRun() override;
    void DoSetup() override;
};

MLOTestCase::MLOTestCase()
    : WifiCoTraceHelperBaseTestCase(
          "MLOTestCase: Track channel occupancy on multiple links of a multi-link device (MLD).")
{
}

void
MLOTestCase::DoRun()
{
    /* Configure tid-to-link mapping such that packets with different tids are sent on different
     * links. */
    ConfigureTidToLinkMapping("0 0; 1,2,3,4,5,6,7 1");

    size_t senderNodeId = 1;
    size_t recNodeId = 0;
    Ptr<Node> sender = m_nodes.Get(senderNodeId);
    Ptr<Node> receiver = m_nodes.Get(recNodeId);

    SchedulePackets(sender, receiver, Seconds(0.5), 1, 1000, 0 /*tid*/);
    SchedulePackets(sender, receiver, Seconds(0.5), 1, 1000, 3 /*tid*/);

    // Send a packet with tid '0' and measure channel occupancy.
    Time scheduleAt = Seconds(1.05);
    size_t tid = 0;
    SchedulePackets(sender, receiver, scheduleAt, 1 /*num of packets*/, 1000 /*Bytes*/, tid);
    WifiCoTraceHelper helper0{scheduleAt, scheduleAt + Seconds(0.01)};
    helper0.Enable(m_nodes);

    // Send a packet with tid '3' and measure channel occupancy.
    scheduleAt = Seconds(2.0);
    tid = 3;
    SchedulePackets(sender, receiver, scheduleAt, 1 /*num of packets*/, 1000 /*Bytes*/, tid);
    WifiCoTraceHelper helper1{scheduleAt, scheduleAt + Seconds(0.1)};
    helper1.Enable(m_nodes);

    Simulator::Stop(Seconds(3.5));

    Simulator::Run();

    std::cout << "## MLOTestCase: Tid 0 Packet ##\n";
    helper0.PrintStatistics(std::cout);
    std::cout << "## MLOTestCase: Tid 3 Packet ##\n";
    helper1.PrintStatistics(std::cout);

    /* Assert TX time for each packet matches analytically compuated value of 88.8 us. Refer
     * SendOnePacketTestCase above for the analytical calculation of TX time of 88.8 us.*/
    // Assert on link 0
    auto map0 = GetChannelOccupancy(senderNodeId, 0 /*linkId*/, helper0);
    NS_TEST_ASSERT_MSG_EQ(map0.at(WifiPhyState::TX),
                          NanoSeconds(88800),
                          "TX duration does not match");

    // Assert on link 1
    auto map1 = GetChannelOccupancy(senderNodeId, 1 /*linkId*/, helper1);
    NS_TEST_ASSERT_MSG_EQ(map1.at(WifiPhyState::TX),
                          NanoSeconds(88800),
                          "TX duration does not match");

    Simulator::Destroy();
}

void
MLOTestCase::DoSetup()
{
    // LogComponentEnable("WifiCoTraceHelper", LOG_LEVEL_INFO);
    RngSeedManager::SetSeed(1);

    NodeContainer ap;
    ap.Create(1);

    uint32_t nWifi = 1;
    NodeContainer sta;
    sta.Create(nWifi);

    m_nodes.Add(ap);
    m_nodes.Add(sta);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    // Create multiple spectrum channels
    Ptr<MultiModelSpectrumChannel> spectrumChannel5Ghz = CreateObject<MultiModelSpectrumChannel>();
    Ptr<MultiModelSpectrumChannel> spectrumChannel6Ghz = CreateObject<MultiModelSpectrumChannel>();

    // SpectrumWifiPhyHelper (3 links)
    SpectrumWifiPhyHelper phy(2);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
    phy.AddChannel(spectrumChannel6Ghz, WIFI_SPECTRUM_6_GHZ);

    // configure operating channel for each link
    phy.Set(0, "ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));
    phy.Set(1, "ChannelSettings", StringValue("{0, 40, BAND_6GHZ, 0}"));

    // configure rate manager for each link
    uint8_t linkId = 0;
    wifi.SetRemoteStationManager(linkId,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs11"),
                                 "ControlMode",
                                 StringValue("OfdmRate24Mbps"));
    wifi.SetRemoteStationManager(1,
                                 "ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("EhtMcs11"),
                                 "ControlMode",
                                 StringValue("OfdmRate24Mbps"));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    m_devices.Add(wifi.Install(phy, mac, ap));
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false));
    m_devices.Add(wifi.Install(phy, mac, sta));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    auto distance = 1;
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    /* Disable aggregation and set guard interval */
    DisableAggregation();
    int gi = 800; // Guard Interval in nano seconds
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/HeConfiguration/GuardInterval",
                TimeValue(NanoSeconds(gi)));

    PacketSocketHelper helper;
    helper.Install(m_nodes);
}

/**
 * @ingroup wifi-test
 * @brief LinkId of non-AP MLD changes after MLO setup.
 *
 * This test case configures one AP MLD with three links and one non-AP MLD with two links. Ngon-AP
 * MLD swaps (i.e., renames) its link after MLO setup. It asserts that WifiCoTraceHelper should
 * capture statistics of the renamed link.
 */
class LinkRenameTestCase : public WifiCoTraceHelperBaseTestCase
{
  public:
    /** Constructor. */
    LinkRenameTestCase();

  private:
    void DoRun() override;
    void DoSetup() override;
};

LinkRenameTestCase::LinkRenameTestCase()
    : WifiCoTraceHelperBaseTestCase(
          "LinkRenameTestCase: WifiCoTraceHelper should record statistics under new LinkId.")
{
}

void
LinkRenameTestCase::DoRun()
{
    // Adding nodes to wificohelper
    WifiCoTraceHelper coHelper;
    coHelper.Stop(m_simulationStop);
    coHelper.Enable(m_nodes);
    auto staNodeId = 1;

    SchedulePackets(m_nodes.Get(1), m_nodes.Get(0), Seconds(2.0), 1000, 1000);
    Simulator::Stop(m_simulationStop);

    Simulator::Run();
    Simulator::Destroy();

    std::cout << "## LinkRenameTestCase ##\n";
    coHelper.PrintStatistics(std::cout);

    auto staStatistics = coHelper.GetDeviceRecords().at(staNodeId).m_linkStateDurations;

    // Note that sta has only two phys. So, a linkId of '2' is created by renaming one of the
    // existing links.
    auto renamedLinkId = 2;
    auto it = staStatistics.find(renamedLinkId);
    NS_TEST_ASSERT_MSG_EQ((it != staStatistics.end()),
                          true,
                          "Link: " << renamedLinkId << " isn't present at non-AP MLD");
}

void
LinkRenameTestCase::DoSetup()
{
    // LogComponentEnable("WifiCoTraceHelper", LOG_LEVEL_DEBUG);

    m_simulationStop = Seconds(3);

    NodeContainer ap;
    ap.Create(1);

    uint32_t nWifi = 1;
    NodeContainer sta;
    sta.Create(nWifi);

    m_nodes.Add(ap);
    m_nodes.Add(sta);

    WifiMacHelper mac;
    Ssid ssid = Ssid("ns-3-ssid");

    // Create multiple spectrum channels
    Ptr<MultiModelSpectrumChannel> spectrumChannel2_4Ghz =
        CreateObject<MultiModelSpectrumChannel>();
    Ptr<MultiModelSpectrumChannel> spectrumChannel5Ghz = CreateObject<MultiModelSpectrumChannel>();

    // SpectrumWifiPhyHelper (2 links)
    SpectrumWifiPhyHelper nonApPhyHelper(2);
    nonApPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    nonApPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
    nonApPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);

    // configure operating channel for each link
    nonApPhyHelper.Set(0, "ChannelSettings", StringValue("{42, 80, BAND_5GHZ, 0}"));
    nonApPhyHelper.Set(1, "ChannelSettings", StringValue("{0, 80, BAND_5GHZ, 0}"));

    nonApPhyHelper.Set("FixedPhyBand", BooleanValue(true));

    WifiHelper nonApWifiHelper;
    nonApWifiHelper.SetStandard(WIFI_STANDARD_80211be);

    // configure rate manager for each link
    uint8_t firstLinkId = 0;
    nonApWifiHelper.SetRemoteStationManager(firstLinkId,
                                            "ns3::ConstantRateWifiManager",
                                            "DataMode",
                                            StringValue("EhtMcs9"),
                                            "ControlMode",
                                            StringValue("EhtMcs9"));
    nonApWifiHelper.SetRemoteStationManager(1,
                                            "ns3::ConstantRateWifiManager",
                                            "DataMode",
                                            StringValue("EhtMcs9"),
                                            "ControlMode",
                                            StringValue("EhtMcs9"));

    SpectrumWifiPhyHelper apPhyHelper(3);
    apPhyHelper.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    apPhyHelper.AddChannel(spectrumChannel2_4Ghz, WIFI_SPECTRUM_2_4_GHZ);
    apPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);
    apPhyHelper.AddChannel(spectrumChannel5Ghz, WIFI_SPECTRUM_5_GHZ);

    // configure operating channel for each link
    apPhyHelper.Set(0, "ChannelSettings", StringValue("{6, 40, BAND_2_4GHZ, 0}"));
    apPhyHelper.Set(1, "ChannelSettings", StringValue("{42, 80, BAND_5GHZ, 0}"));
    apPhyHelper.Set(2, "ChannelSettings", StringValue("{0, 0, BAND_5GHZ, 0}"));

    apPhyHelper.Set("FixedPhyBand", BooleanValue(true));

    WifiHelper apWifiHelper;
    apWifiHelper.SetStandard(WIFI_STANDARD_80211be);

    apWifiHelper.SetRemoteStationManager(firstLinkId,
                                         "ns3::ConstantRateWifiManager",
                                         "DataMode",
                                         StringValue("EhtMcs9"),
                                         "ControlMode",
                                         StringValue("EhtMcs9"));
    apWifiHelper.SetRemoteStationManager(1,
                                         "ns3::ConstantRateWifiManager",
                                         "DataMode",
                                         StringValue("EhtMcs9"),
                                         "ControlMode",
                                         StringValue("EhtMcs9"));
    apWifiHelper.SetRemoteStationManager(2,
                                         "ns3::ConstantRateWifiManager",
                                         "DataMode",
                                         StringValue("EhtMcs9"),
                                         "ControlMode",
                                         StringValue("EhtMcs9"));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));
    m_devices.Add(apWifiHelper.Install(apPhyHelper, mac, ap));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(true));
    m_devices.Add(nonApWifiHelper.Install(nonApPhyHelper, mac, sta));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    auto distance = 0.1;
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(m_nodes);

    PacketSocketHelper helper;
    helper.Install(m_nodes);
}

/**
 * @ingroup wifi-test
 * @brief Wifi Channel Occupancy Helper Test Suite
 */
class WifiCoHelperTestSuite : public TestSuite
{
  public:
    /** Constructor. */
    WifiCoHelperTestSuite();
};

WifiCoHelperTestSuite::WifiCoHelperTestSuite()
    : TestSuite("wifi-co-trace-helper", Type::UNIT)
{
    AddTestCase(new SendOnePacketTestCase, TestCase::Duration::QUICK);
    AddTestCase(new MLOTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LinkRenameTestCase, TestCase::Duration::QUICK);
}

/**
 * @ingroup wifi-test
 * WifiCoHelperTestSuite instance variable.
 */
static WifiCoHelperTestSuite g_WifiCoHelperTestSuite;
