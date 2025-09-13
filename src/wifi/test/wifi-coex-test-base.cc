/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-coex-test-base.h"

#include "ns3/boolean.h"
#include "ns3/coex-helper.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/test.h"
#include "ns3/wifi-static-setup-helper.h"

#include <algorithm>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiCoexTest");

WifiCoexTestBase::WifiCoexTestBase(const std::string& name)
    : TestCase(name)
{
}

void
WifiCoexTestBase::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 30;

    NodeContainer nodes(2); // AP, STA
    NetDeviceContainer devices;

    CoexHelper coex;
    coex.SetArbitrator(m_arbitratorName);
    coex.Install(nodes.Get(1));

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211bn);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();

    auto strings = SplitString(m_channels, ":");
    SpectrumWifiPhyHelper phy{static_cast<uint8_t>(strings.size())};
    for (std::size_t id = 0; id < strings.size(); ++id)
    {
        phy.Set(id, "ChannelSettings", StringValue(strings[id]));
    }
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(spectrumChannel);

    Ssid ssid("wifi-coex-test");
    WifiMacHelper mac;

    mac.SetType("ns3::ApWifiMac", "BeaconGeneration", BooleanValue(false), "Ssid", SsidValue(ssid));
    devices.Add(wifi.Install(phy, mac, nodes.Get(0)));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    wifi.SetCoexManager(m_managerName);
    devices.Add(wifi.Install(phy, mac, nodes.Get(1)));

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(devices, streamNumber);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));  // AP
    positionAlloc->Add(Vector(0.0, 10.0, 0.0)); // non-AP STA
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    m_apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(0))->GetMac());
    m_staMac = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(devices.Get(1))->GetMac());

    // set data and control mode for remote station managers
    for (const auto& mac : std::initializer_list<Ptr<WifiMac>>{m_apMac, m_staMac})
    {
        for (const auto linkId : mac->GetLinkIds())
        {
            const auto band = mac->GetWifiPhy(linkId)->GetPhyBand();
            std::string ctrlMode{"OfdmRate6Mbps"};
            if (band == WIFI_PHY_BAND_6GHZ)
            {
                ctrlMode = "HeMcs0";
            }
            else if (band == WIFI_PHY_BAND_2_4GHZ)
            {
                ctrlMode = "ErpOfdmRate6Mbps";
            }

            const auto rsm = mac->GetWifiRemoteStationManager(linkId);
            rsm->SetAttribute("DataMode", StringValue(m_dataMode));
            rsm->SetAttribute("ControlMode", StringValue(ctrlMode));
        }
    }

    /* static setup of association and BA agreements */
    WifiStaticSetupHelper::SetStaticAssociation(m_apMac->GetDevice(), m_staMac->GetDevice());
    WifiStaticSetupHelper::SetStaticBlockAck(m_apMac->GetDevice(), m_staMac->GetDevice(), 0);
    WifiStaticSetupHelper::SetStaticBlockAck(m_staMac->GetDevice(), m_apMac->GetDevice(), 0);

    auto coexArbitrator = m_staMac->GetDevice()->GetNode()->GetObject<coex::Arbitrator>();
    NS_TEST_ASSERT_MSG_NE(coexArbitrator, nullptr, "No Coex Arbitrator installed on non-AP STA");

    auto coexManager = coexArbitrator->GetDeviceCoexManager(coex::Rat::WIFI);
    NS_TEST_ASSERT_MSG_NE(coexManager, nullptr, "No Coex Manager installed on non-AP STA");

    m_coexManager = DynamicCast<WifiCoexManager>(coexManager);
    NS_TEST_ASSERT_MSG_NE(m_coexManager, nullptr, "No wifi Coex Manager installed on non-AP STA");

    m_coexArbitrator = m_coexManager->GetCoexArbitrator();
    NS_TEST_ASSERT_MSG_NE(m_coexArbitrator, nullptr, "No Coex Arbitrator set on the coex manager");

    m_coexArbitrator->TraceConnectWithoutContext(
        "CoexEvent",
        MakeCallback(&WifiCoexTestBase::CoexEventCallback, this));

    // Trace PSDUs passed to the PHY on all devices
    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        auto dev = DynamicCast<WifiNetDevice>(devices.Get(i));
        for (uint8_t phyId = 0; phyId < dev->GetNPhys(); ++phyId)
        {
            dev->GetPhy(phyId)->TraceConnectWithoutContext(
                "PhyTxPsduBegin",
                MakeCallback(&WifiCoexTestBase::Transmit, this).Bind(dev->GetMac(), phyId));
        }
    }

    // install packet socket on all nodes
    PacketSocketHelper packetSocket;
    packetSocket.Install(nodes);

    // install a packet socket server on all nodes
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>(nodes.Get(i)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        nodes.Get(i)->AddApplication(server);
        server->SetStartTime(Seconds(0)); // now
        server->SetStopTime(m_duration);
    }

    for (uint32_t nodeId = 0; nodeId < nodes.GetN(); ++nodeId)
    {
        Config::ConnectWithoutContext(
            "/NodeList/" + std::to_string(nodeId) +
                "/ApplicationList/*/$ns3::PacketSocketServer/Rx",
            MakeCallback(&WifiCoexTestBase::L7Receive, this).Bind(nodeId));
    }

    // set DL and UL packet socket addresses
    m_dlSocket.SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
    m_dlSocket.SetPhysicalAddress(m_staMac->GetDevice()->GetAddress());
    m_dlSocket.SetProtocol(1);

    m_ulSocket.SetSingleDevice(m_staMac->GetDevice()->GetIfIndex());
    m_ulSocket.SetPhysicalAddress(m_apMac->GetDevice()->GetAddress());
    m_ulSocket.SetProtocol(1);
}

void
WifiCoexTestBase::SetMockEventGen(const Time& interEventTime,
                                  const Time& startDelay,
                                  const Time& duration)
{
    if (!m_mockEventGen)
    {
        m_mockEventGen = CreateObjectWithAttributes<coex::MockEventGenerator>(
            "InterEventTime",
            StringValue("ns3::ConstantRandomVariable[Constant=" +
                        std::to_string(interEventTime.GetMicroSeconds()) + "]"));

        m_coexArbitrator->SetDeviceCoexManager(coex::Rat::UNDEFINED, m_mockEventGen);
    }

    NS_TEST_ASSERT_MSG_EQ(m_coexArbitrator->GetDeviceCoexManager(coex::Rat::UNDEFINED),
                          m_mockEventGen,
                          "Mock event generator not installed on non-AP STA");

    m_mockEventGen->SetAttribute("InterEventTime",
                                 StringValue("ns3::ConstantRandomVariable[Constant=" +
                                             std::to_string(interEventTime.GetMicroSeconds()) +
                                             "]"));
    m_mockEventGen->SetAttribute("EventStartDelay",
                                 StringValue("ns3::ConstantRandomVariable[Constant=" +
                                             std::to_string(startDelay.GetMicroSeconds()) + "]"));
    m_mockEventGen->SetAttribute("EventDuration",
                                 StringValue("ns3::ConstantRandomVariable[Constant=" +
                                             std::to_string(duration.GetMicroSeconds()) + "]"));
}

void
WifiCoexTestBase::Transmit(Ptr<WifiMac> mac,
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
WifiCoexTestBase::CoexEventCallback(const coex::Event& coexEvent)
{
    NS_LOG_INFO("Coex event notified to the coex arbitrator: " << coexEvent << "\n");
}

Ptr<PacketSocketClient>
WifiCoexTestBase::GetApplication(WifiDirection dir,
                                 std::size_t count,
                                 std::size_t pktSize,
                                 uint8_t priority) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(Time{0}));
    client->SetAttribute("Priority", UintegerValue(priority));
    client->SetRemote(dir == WifiDirection::DOWNLINK ? m_dlSocket : m_ulSocket);
    client->SetStartTime(Seconds(0)); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
WifiCoexTestBase::L7Receive(uint8_t nodeId, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_INFO("Packet received by NODE " << +nodeId << "\n");
    m_rxPkts[nodeId]++;
}
