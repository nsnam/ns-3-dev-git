/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/test.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"

#include <utility>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiP2pTest");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test the beaconing operation in IBSS.
 */
class IbssBeaconingTest : public TestCase
{
  public:
    /**
     * Constructor
     */
    IbssBeaconingTest();

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    virtual void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /**
     * Check correctness of transmitted beacon frames
     */
    void CheckResults();

    /// Information about transmitted beacon frames
    struct BeaconFrameInfo
    {
        WifiMacHeader header;   ///< MAC header
        MgtBeaconHeader beacon; ///< Beacon header
    };

    std::vector<BeaconFrameInfo> m_txBeacons; ///< transmitted beacons
};

IbssBeaconingTest::IbssBeaconingTest()
    : TestCase("Check beaconing operation in IBSS")
{
}

void
IbssBeaconingTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    const auto& mpdu = *psduMap.cbegin()->second->begin();
    NS_LOG_FUNCTION(this << *mpdu << txVector << txPowerW);
    if (mpdu->GetHeader().IsBeacon())
    {
        MgtBeaconHeader beacon;
        mpdu->GetPacket()->PeekHeader(beacon);
        m_txBeacons.push_back({mpdu->GetHeader(), beacon});
    }
}

void
IbssBeaconingTest::CheckResults()
{
    NS_TEST_EXPECT_MSG_EQ(m_txBeacons.size(), 20, "Expected 20 transmitted beacons");
    std::map<Mac48Address, std::size_t> beaconsPerSta{};
    for (const auto& txBeacon : m_txBeacons)
    {
        const auto addr2 = txBeacon.header.GetAddr2();
        if (beaconsPerSta.contains(addr2))
        {
            beaconsPerSta[addr2]++;
        }
        else
        {
            beaconsPerSta[addr2] = 1;
        }
        const auto& capabilities = txBeacon.beacon.Capabilities();
        NS_TEST_EXPECT_MSG_EQ(capabilities.IsEss(), false, "ESS bit should not be set for IBSS");
    }
    NS_TEST_EXPECT_MSG_EQ(beaconsPerSta.size(), 2, "Expected transmitted beacons from two STAs");
    for (const auto& [sender, count] : beaconsPerSta)
    {
        NS_TEST_EXPECT_MSG_EQ(count, 10, "Expected 10 transmitted beacons from " << sender);
    }
}

void
IbssBeaconingTest::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("WifiP2pTest", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 100;

    NodeContainer ibssNodes;
    ibssNodes.Create(2);

    auto channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac",
                "Ssid",
                SsidValue(Ssid("ibss-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

    auto ibssDevices = wifi.Install(phy, mac, ibssNodes);

    WifiHelper::AssignStreams(ibssDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ibssNodes);

    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/0/$ns3::WifiNetDevice/Phys/0/PhyTxPsduBegin",
        MakeCallback(&IbssBeaconingTest::Transmit, this));
}

void
IbssBeaconingTest::DoRun()
{
    Simulator::Stop(Seconds(1));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief Test transmissions in IBSS when peers have different capabilities.
 */
class IbssCapabilitiesTest : public TestCase
{
  public:
    /**
     * Constructor
     * \param txStandard the standard supported by the TX device
     * \param rxStandard the standard supported by the RX device
     * \param beaconing flag whether beaconing is used in IBSS
     */
    IbssCapabilitiesTest(WifiStandard txStandard, WifiStandard rxStandard, bool beaconing);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /**
     * Callback invoked when a packet is dropped by the MAC prior to enqueueing.
     *
     * \param packet the dropped packet
     */
    void TxDrop(Ptr<const Packet> packet);

    /**
     * Callback invoked when a packet is generated by the packet socket client.
     *
     * \param pkt the packet
     * \param addr the address
     */
    void TxGenerated(Ptr<const Packet> pkt, const Address& addr);

    /**
     * Callback invoked when a packet is received by the packet socket server.
     *
     * \param pkt the packet
     * \param addr the address
     */
    void Receive(Ptr<const Packet> pkt, const Address& addr);

    /**
     * Check that the simulation produced the expected results.
     */
    void CheckResults();

    WifiStandard m_txStandard; ///< the standard for the TX device
    WifiStandard m_rxStandard; ///< the standard for the RX device
    bool m_beaconing;          ///< flag whether beacon generation is enabled

    std::size_t m_nPackets; ///< number of packets to generate

    std::vector<WifiTxVector> m_txVectors;             ///< TXVECTOR of transmitted packets
    std::vector<Ptr<const Packet>> m_droppedPackets;   ///< dropped packets
    std::vector<Ptr<const Packet>> m_generatedPackets; ///< generated packets
    std::vector<Ptr<const Packet>> m_rxPackets;        ///< received packets
};

IbssCapabilitiesTest::IbssCapabilitiesTest(WifiStandard txStandard,
                                           WifiStandard rxStandard,
                                           bool beaconing)
    : TestCase("Check transmissions in IBSS when peers have different capabilities"),
      m_txStandard{txStandard},
      m_rxStandard{rxStandard},
      m_beaconing{beaconing},
      m_nPackets{2}
{
}

void
IbssCapabilitiesTest::Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW)
{
    const auto& mpdu = *psduMap.cbegin()->second->begin();
    NS_LOG_FUNCTION(this << *mpdu << txVector << txPowerW);
    if (mpdu->GetHeader().IsData())
    {
        m_txVectors.push_back(txVector);
    }
}

void
IbssCapabilitiesTest::TxDrop(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(this << packet);
    m_droppedPackets.push_back(packet);
}

void
IbssCapabilitiesTest::TxGenerated(Ptr<const Packet> pkt, const Address& addr)
{
    NS_LOG_FUNCTION(this << pkt << addr);
    m_generatedPackets.push_back(pkt);
}

void
IbssCapabilitiesTest::Receive(Ptr<const Packet> pkt, const Address& addr)
{
    NS_LOG_FUNCTION(this << pkt << addr);
    m_rxPackets.push_back(pkt);
}

void
IbssCapabilitiesTest::CheckResults()
{
    NS_TEST_EXPECT_MSG_EQ(m_generatedPackets.size(),
                          m_nPackets,
                          "Unexpected amount of generated packets");

    /*
     * The first packet should get dropped if capabilities are learnt from beacon,
     * since the first packet is transmitted before the first beacon is received
     */
    const std::size_t expectedDroppedPackets = m_beaconing ? 1 : 0;
    NS_TEST_EXPECT_MSG_EQ(m_droppedPackets.size(),
                          expectedDroppedPackets,
                          "Incorrect amount of dropped packets");

    /*
     * The amount of transmitted packets correspond to the number of generated packet minus
     * the amount of dropped packets. If capabilities are not learnt from beacons, TX device
     * assumes RX device supports the same capabilities. If that is not the case, packets
     * cannot be successfully received and each packet gets retransmitted 7 times (default
     * configuration of retry counter)
     */
    const std::size_t expectedTxPackets =
        m_beaconing ? (m_nPackets - 1)
                    : (m_txStandard <= m_rxStandard ? m_nPackets : (m_nPackets * 7));
    NS_TEST_EXPECT_MSG_EQ(m_txVectors.size(),
                          expectedTxPackets,
                          "Incorrect amount of transmitted packets");

    /*
     * If capabilities are learnt from beacons, the first packet is dropped and hence only
     * one packet can be received. Otherwise, the two generated packets are transmitted and
     * can be received only if the RX device supports the capabilities of the TX device.
     */
    const std::size_t expectedRxPackets =
        m_beaconing ? (m_nPackets - 1) : (m_txStandard <= m_rxStandard ? m_nPackets : 0);
    NS_TEST_EXPECT_MSG_EQ(m_rxPackets.size(),
                          expectedRxPackets,
                          "Incorrect amount of received packets");

    if (m_beaconing)
    {
        NS_TEST_EXPECT_MSG_EQ(m_droppedPackets.at(0)->GetUid(),
                              m_generatedPackets.at(0)->GetUid(),
                              "The first generated packet should have been dropped");
        NS_TEST_EXPECT_MSG_EQ(m_rxPackets.at(0)->GetUid(),
                              m_generatedPackets.at(1)->GetUid(),
                              "The second generated packet should have been received");
    }
    else
    {
        for (std::size_t i = 0; i < m_rxPackets.size(); ++i)
        {
            NS_TEST_EXPECT_MSG_EQ(m_rxPackets.at(i)->GetUid(),
                                  m_generatedPackets.at(i)->GetUid(),
                                  "Received packets are not in correct order");
        }
    }

    WifiModulationClass expectedModClass;
    if (m_beaconing)
    {
        if ((m_txStandard == WIFI_STANDARD_80211ax) && (m_rxStandard == WIFI_STANDARD_80211ax))
        {
            expectedModClass = WIFI_MOD_CLASS_HE;
        }
        else if (((m_txStandard == WIFI_STANDARD_80211ax) &&
                  (m_rxStandard == WIFI_STANDARD_80211ac)) ||
                 ((m_txStandard == WIFI_STANDARD_80211ac) &&
                  (m_rxStandard == WIFI_STANDARD_80211ax)))
        {
            expectedModClass = WIFI_MOD_CLASS_VHT;
        }
        else
        {
            NS_ASSERT_MSG(false, "TX and RX standards combination not covered by the test");
        }
    }
    else
    {
        if (m_txStandard == WIFI_STANDARD_80211ax)
        {
            expectedModClass = WIFI_MOD_CLASS_HE;
        }
        else if (m_txStandard == WIFI_STANDARD_80211ac)
        {
            expectedModClass = WIFI_MOD_CLASS_VHT;
        }
        else
        {
            NS_ASSERT_MSG(false, "TX standard not covered by the test");
        }
    }
    NS_TEST_EXPECT_MSG_EQ(m_txVectors.at(0).GetModulationClass(),
                          expectedModClass,
                          "Unexpected modulation class");
}

void
IbssCapabilitiesTest::DoSetup()
{
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("WifiP2pTest", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 100;

    NodeContainer ibssNodes;
    ibssNodes.Create(2);

    auto channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(m_txStandard);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager",
                                 "IncrementRetryCountUnderBa",
                                 BooleanValue(true));

    WifiMacHelper mac;
    mac.SetType("ns3::AdhocWifiMac",
                "Ssid",
                SsidValue(Ssid("ibss-ssid")),
                "BeaconGeneration",
                BooleanValue(m_beaconing));

    auto ibssDevices = wifi.Install(phy, mac, ibssNodes.Get(0));

    wifi.SetStandard(m_rxStandard);
    ibssDevices.Add(wifi.Install(phy, mac, ibssNodes.Get(1)));

    WifiHelper::AssignStreams(ibssDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ibssNodes);

    PacketSocketHelper packetSocket;
    packetSocket.Install(ibssNodes);

    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice(ibssDevices.Get(0)->GetIfIndex());
    socketAddr.SetPhysicalAddress(ibssDevices.Get(1)->GetAddress());

    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(1000));
    client->SetAttribute("MaxPackets", UintegerValue(m_nPackets));
    client->SetAttribute("Interval", TimeValue(Seconds(0.5)));
    client->SetRemote(socketAddr);
    ibssNodes.Get(0)->AddApplication(client);
    client->SetStartTime(Seconds(0.0));
    client->SetStopTime(Seconds(1.0));

    auto server = CreateObject<PacketSocketServer>();
    server->SetLocal(socketAddr);
    ibssNodes.Get(1)->AddApplication(server);
    server->SetStartTime(Seconds(0.0));
    server->SetStopTime(Seconds(1.0));

    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSocketClient/Tx",
                                  MakeCallback(&IbssCapabilitiesTest::TxGenerated, this));

    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxPsduBegin",
                                  MakeCallback(&IbssCapabilitiesTest::Transmit, this));

    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop",
                                  MakeCallback(&IbssCapabilitiesTest::TxDrop, this));

    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSocketServer/Rx",
                                  MakeCallback(&IbssCapabilitiesTest::Receive, this));
}

void
IbssCapabilitiesTest::DoRun()
{
    Simulator::Stop(Seconds(1));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi P2P test suite
 */
class WifiP2pTestSuite : public TestSuite
{
  public:
    WifiP2pTestSuite();
};

WifiP2pTestSuite::WifiP2pTestSuite()
    : TestSuite("wifi-p2p", Type::UNIT)
{
    AddTestCase(new IbssBeaconingTest(), TestCase::Duration::QUICK);
    for (auto& [txStandard, rxStandard] : std::vector<std::pair<WifiStandard, WifiStandard>>{
             {WIFI_STANDARD_80211ax, WIFI_STANDARD_80211ax},
             {WIFI_STANDARD_80211ax, WIFI_STANDARD_80211ac},
             {WIFI_STANDARD_80211ac, WIFI_STANDARD_80211ax}})
    {
        for (auto beaconing : {true, false})
        {
            AddTestCase(new IbssCapabilitiesTest(txStandard, rxStandard, beaconing),
                        TestCase::Duration::QUICK);
        }
    }
}

static WifiP2pTestSuite g_wifiP2pTestSuite; ///< the test suite
