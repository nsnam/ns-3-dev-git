/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/adhoc-wifi-mac.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/eht-configuration.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/mgt-headers.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiP2pTest");

namespace
{

constexpr uint16_t P2P_STA_TO_ADHOC_PROTOCOL{
    1}; ///< protocol to create socket for P2P transmission from P2P STA to ADHOC
constexpr uint16_t ADHOC_TO_P2P_STA_PROTOCOL{
    2}; ///< protocol to create socket for P2P transmission from ADHOC to P2P STA
constexpr uint16_t P2P_STA_TO_STA_PROTOCOL{
    3}; ///< protocol to create socket for P2P transmission from P2P STA to STA
constexpr uint16_t STA_TO_P2P_STA_PROTOCOL{
    4}; ///< protocol to create socket for P2P transmission from STA to P2P STA
constexpr uint16_t AP_TO_P2P_STA_PROTOCOL{
    5}; ///< protocol to create socket for downlink transmission from AP to P2P STA
constexpr uint16_t P2P_STA_TO_AP_PROTOCOL{
    6}; ///< protocol to create socket for uplink transmission from P2P STA to AP

/// Map protocol ID to payload size (in bytes)
const std::map<uint16_t, uint32_t> ProtocolToPayloadSizeMap{
    {P2P_STA_TO_ADHOC_PROTOCOL, 500},
    {ADHOC_TO_P2P_STA_PROTOCOL, 510},
    {P2P_STA_TO_STA_PROTOCOL, 750},
    {STA_TO_P2P_STA_PROTOCOL, 760},
    {AP_TO_P2P_STA_PROTOCOL, 1000},
    {P2P_STA_TO_AP_PROTOCOL, 1100},
};

} // namespace

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test the beaconing operation in IBSS.
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
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
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
        const auto& capabilities = txBeacon.beacon.m_capability;
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
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test transmissions in IBSS when peers have different capabilities.
 */
class IbssCapabilitiesTest : public TestCase
{
  public:
    /**
     * Constructor
     * @param txStandard the standard supported by the TX device
     * @param rxStandard the standard supported by the RX device
     * @param beaconing flag whether beaconing is used in IBSS
     */
    IbssCapabilitiesTest(WifiStandard txStandard, WifiStandard rxStandard, bool beaconing);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    void Transmit(WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);

    /**
     * Callback invoked when a packet is dropped by the MAC prior to enqueueing.
     *
     * @param packet the dropped packet
     */
    void TxDrop(Ptr<const Packet> packet);

    /**
     * Callback invoked when a packet is generated by the packet socket client.
     *
     * @param pkt the packet
     * @param addr the address
     */
    void TxGenerated(Ptr<const Packet> pkt, const Address& addr);

    /**
     * Callback invoked when a packet is received by the packet socket server.
     *
     * @param pkt the packet
     * @param addr the address
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
            NS_ABORT_MSG("TX and RX standards combination not covered by the test");
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
            NS_ABORT_MSG("TX standard not covered by the test");
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
    client->SetAttribute("PacketSize",
                         UintegerValue(ProtocolToPayloadSizeMap.at(AP_TO_P2P_STA_PROTOCOL)));
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
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief Test P2P support.
 *
 * In this test, there is one AP and one STA associated to the AP, and an adhoc station.
 * The STA has P2P turned on at the beginning on the test, which allows to verify communication
 * between the STA and the adhoc station via the P2P link. Then, P2P is turned off on the STA to
 * verify no communication occurs between the STA and the adhoc station. During that period, packets
 * enqueued at the STA for the adhoc station should be forwarded to the AP. At the end of the test,
 * P2P is turned on again to verify communication between the STA and the adhoc station is restored.
 * For the duration of the test, the AP and the STA exchange packets with each others
 * (bidirectional) to verify P2P does not alter usual infrastructure operations. Similarly, the STA
 * and the adhoc station also exchange packets with each others (bidirectional) for the whole
 * duration of the test. For the sake of the test, we limit the traffic to a packet per second.
 */
class P2pTest : public TestCase
{
  public:
    /**
     * The period a given mode is activated
     */
    struct ActivatedPeriod
    {
        Time start{}; //!< start time of the period
        Time stop{};  //!< stop time of the period
    };

    /**
     * Configuration parameters for P2P tests
     */
    struct P2pTestParams
    {
        std::vector<std::string>
            apChannels; //!< the strings specifying the operating channels for the AP
        std::vector<std::string>
            staChannels; //!< the strings specifying the operating channels for the non-AP STAs
        std::string p2pChannel; //!< the string specifying the operating channel for P2P
        std::set<uint8_t> infraLinkSetP2pSta{}; //!< if not empty, allow to specify a subset of the
                                                //!< links for P2P STA to use for infrastructure by
                                                //!< configuring TID-to-link mapping
        std::vector<ActivatedPeriod> p2pActivatedPeriods{
            {Seconds(0), Seconds(1)},
            {Seconds(4), Seconds(5)}}; //!< periods the P2P link is enabled
        std::vector<ActivatedPeriod> infrastructureActivatedPeriods{
            {Seconds(0), Seconds(5)}}; //!< periods the STA is associated with the AP
    };

    /**
     * Constructor
     *
     * @param name the name of the configuration
     * @param params the configuration parameters
     */
    P2pTest(const std::string& name, const P2pTestParams& params);

  private:
    void DoSetup() override;
    void DoRun() override;

    /// PHY band-indexed map of spectrum channels
    using ChannelMap = std::map<FrequencyRange, Ptr<MultiModelSpectrumChannel>>;

    /**
     * Reset the given PHY helper, use the given strings to set the ChannelSettings
     * attribute of the PHY objects to create, and attach them to the given spectrum
     * channels appropriately.
     *
     * @param helper the given PHY helper
     * @param channels the strings specifying the operating channels to configure
     * @param channelMap the created spectrum channels
     */
    void SetChannels(SpectrumWifiPhyHelper& helper,
                     const std::vector<std::string>& channels,
                     const ChannelMap& channelMap);

    /**
     * Set the SSID of the AP the STA should attempt to associate with.
     *
     * @param ssid the SSID
     */
    void SetStaSsid(Ssid ssid);

    /**
     * Turn on or off P2P capabilities at runtime.
     *
     * @param enable flag whether P2P should be enabled
     */
    void SetP2pEnabled(bool enable);

    /**
     * Create traffic flowing between two devices.
     * It will generate packets every second, starting at the specified time.
     *
     * @param txNode the TX node
     * @param rxNode the RX node
     * @param destinationAddress the destination address to use
     * @param protocol the protocol to identify the flow
     * @param start the starting time
     */
    void CreateTraffic(Ptr<Node> txNode,
                       Ptr<Node> rxNode,
                       const Address& destinationAddress,
                       uint16_t protocol,
                       Time start);

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param mac the MAC transmitting the PSDUs
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPowerW the tx power in Watts
     */
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW);

    /**
     * Callback invoked when a packet is received by the packet socket server.
     *
     * @param pkt the packet
     * @param addr the address
     */
    void Receive(Ptr<const Packet> pkt, const Address& addr);

    /**
     * Callback invoked when a packet to enqueue has been dropped by the MAC layer.
     *
     * @param packet the packet
     */
    void MacTxDrop(Ptr<const Packet> packet);

    /**
     * Callback invoked when a received packet has been dropped by the MAC layer.
     *
     * @param packet the packet
     */
    void MacRxDrop(Ptr<const Packet> packet);

    /**
     * Check that the simulation produced the expected results.
     */
    void CheckResults();

    /**
     * Get the MAC address to use for the packet socket client transmitting to the STA.
     * If the STA is a non-MLD, it returns its MAC address.
     * Otherwise, it returns either its MLD address if multi-link can be used
     * between the STA and the AP (ADHOC does not support multi-link), otherwise it returns the link
     * address corresponding to the link that will be established between the STA and the AP or the
     * ADHOC STA.
     *
     * @param srcMac the MAC of the source
     * @param dstMac the MAC of the destination
     * @return the address to use for the packet socket
     */
    Address GetPeerAddress(Ptr<WifiMac> srcMac, Ptr<WifiMac> dstMac) const;

    P2pTestParams m_params; ///< the configuration parameters

    Time m_testDuration{};        ///< duration of the test
    Ptr<StaWifiMac> m_p2pSta;     ///< the P2P STA MAC
    Ptr<StaWifiMac> m_staMac;     ///< the non-P2P STA MAC
    Ptr<AdhocWifiMac> m_adhocMac; ///< the adhoc STA MAC

    std::map<uint16_t /*protocol ID*/, uint64_t> m_rxPackets;      ///< count received packets
    std::map<uint32_t /*protocol ID*/, uint64_t> m_droppedPackets; ///< count dropped packets
    std::map<uint16_t /*protocol ID*/, std::vector<std::pair<Time, uint8_t>>>
        m_txLinks; ///< list of links used to transmit packets for a given traffic flow
};

P2pTest::P2pTest(const std::string& name, const P2pTestParams& params)
    : TestCase{"Check P2P operation for configuration: " + name},
      m_params{params},
      m_rxPackets{},
      m_droppedPackets{}
{
    NS_ASSERT_MSG(!m_params.p2pActivatedPeriods.empty(),
                  "P2P should be activated at least once in the test");
}

void
P2pTest::Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  double txPowerW)
{
    auto linkId = mac->GetLinkForPhy(phyId);
    NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);
    const auto& mpdu = *psduMap.begin()->second->begin();
    const auto& hdr = mpdu->GetHeader();
    if (!hdr.IsQosData())
    {
        return;
    }
    const auto payloadSize = mpdu->GetPacketSize() - 8 /* LLC */;
    NS_LOG_FUNCTION(this << phyId << txVector << payloadSize);
    const auto it =
        std::find_if(ProtocolToPayloadSizeMap.cbegin(),
                     ProtocolToPayloadSizeMap.cend(),
                     [payloadSize](const auto& item) { return item.second == payloadSize; });
    NS_ASSERT_MSG(it != ProtocolToPayloadSizeMap.cend(), "Unexpected packet size " << payloadSize);
    const auto protocol = it->first;
    if ((mac->GetTypeOfStation() == AP) && (protocol != AP_TO_P2P_STA_PROTOCOL))
    {
        return;
    }
    const auto now = Simulator::Now();
    if (m_txLinks.contains(protocol))
    {
        m_txLinks[protocol].emplace_back(now, *linkId);
    }
    else
    {
        m_txLinks[protocol] = {{now, *linkId}};
    }
}

void
P2pTest::Receive(Ptr<const Packet> pkt, const Address& addr)
{
    const auto sockAddr = PacketSocketAddress::ConvertFrom(addr);
    const auto protocol = sockAddr.GetProtocol();
    NS_LOG_FUNCTION(this << pkt << pkt->GetSize() << addr << protocol);
    NS_TEST_EXPECT_MSG_EQ(ProtocolToPayloadSizeMap.contains(protocol),
                          true,
                          "Unexpected packet received");
    NS_TEST_EXPECT_MSG_EQ(pkt->GetSize(),
                          ProtocolToPayloadSizeMap.at(protocol),
                          "Unexpected packet size for flow ID " << protocol);
    if (m_rxPackets.contains(protocol))
    {
        m_rxPackets[protocol]++;
    }
    else
    {
        m_rxPackets[protocol] = 1;
    }
}

void
P2pTest::MacTxDrop(Ptr<const Packet> packet)
{
    const auto payloadSize = packet->GetSize() - 8 /* LLC */;
    NS_LOG_FUNCTION(this << packet << payloadSize);
    const auto it =
        std::find_if(ProtocolToPayloadSizeMap.cbegin(),
                     ProtocolToPayloadSizeMap.cend(),
                     [payloadSize](const auto& item) { return item.second == payloadSize; });
    NS_ASSERT_MSG(it != ProtocolToPayloadSizeMap.cend(), "Unexpected packet size " << payloadSize);
    const auto protocol = it->first;
    if (m_droppedPackets.contains(protocol))
    {
        m_droppedPackets[protocol]++;
    }
    else
    {
        m_droppedPackets[protocol] = 1;
    }
}

void
P2pTest::MacRxDrop(Ptr<const Packet> packet)
{
    const auto payloadSize = packet->GetSize() - 8 /* LLC */;
    NS_LOG_FUNCTION(this << packet << payloadSize);
    const auto it =
        std::find_if(ProtocolToPayloadSizeMap.cbegin(),
                     ProtocolToPayloadSizeMap.cend(),
                     [payloadSize](const auto& item) { return item.second == payloadSize; });
    NS_ASSERT_MSG(it != ProtocolToPayloadSizeMap.cend(), "Unexpected packet size " << payloadSize);
    const auto protocol = it->first;
    if (m_droppedPackets.contains(protocol))
    {
        m_droppedPackets[protocol]++;
    }
    else
    {
        m_droppedPackets[protocol] = 1;
    }
}

void
P2pTest::SetStaSsid(Ssid ssid)
{
    NS_LOG_FUNCTION(this << ssid);
    m_p2pSta->SetSsid(ssid);
}

void
P2pTest::SetP2pEnabled(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_p2pSta->SetAttribute("EnableP2pLinks", BooleanValue(enable));
}

void
P2pTest::CheckResults()
{
    NS_LOG_FUNCTION(this);

    const std::size_t packetsPerSeconds = 1;
    const auto maxPacketsPerDirection = packetsPerSeconds * m_testDuration.GetSeconds();

    const auto infrastructureDuration =
        m_params.infrastructureActivatedPeriods.empty()
            ? Time()
            : std::accumulate(
                  m_params.infrastructureActivatedPeriods.cbegin(),
                  m_params.infrastructureActivatedPeriods.cend(),
                  Time(),
                  [](auto sum, const auto& period) { return sum + (period.stop - period.start); });
    const auto expectedPacketsInfrastructure =
        packetsPerSeconds * infrastructureDuration.GetSeconds();
    NS_TEST_EXPECT_MSG_EQ(
        (m_rxPackets.contains(AP_TO_P2P_STA_PROTOCOL) ? m_rxPackets.at(AP_TO_P2P_STA_PROTOCOL) : 0),
        expectedPacketsInfrastructure,
        "Unexpected amount of packets sent from AP to P2P STA");
    NS_TEST_EXPECT_MSG_EQ(
        (m_rxPackets.contains(P2P_STA_TO_AP_PROTOCOL) ? m_rxPackets.at(P2P_STA_TO_AP_PROTOCOL) : 0),
        expectedPacketsInfrastructure,
        "Unexpected amount of packets sent from P2P STA to AP");
    NS_TEST_EXPECT_MSG_EQ((m_rxPackets.contains(STA_TO_P2P_STA_PROTOCOL)
                               ? m_rxPackets.at(STA_TO_P2P_STA_PROTOCOL)
                               : 0),
                          expectedPacketsInfrastructure,
                          "Unexpected amount of packets sent from STA to P2P STA");
    NS_TEST_EXPECT_MSG_EQ((m_rxPackets.contains(P2P_STA_TO_STA_PROTOCOL)
                               ? m_rxPackets.at(P2P_STA_TO_STA_PROTOCOL)
                               : 0),
                          expectedPacketsInfrastructure,
                          "Unexpected amount of packets sent from P2P STA to STA");

    const auto p2pDuration = std::accumulate(
        m_params.p2pActivatedPeriods.cbegin(),
        m_params.p2pActivatedPeriods.cend(),
        Time(),
        [](auto sum, const auto& period) { return sum + (period.stop - period.start); });
    const auto expectedPacketsP2p = packetsPerSeconds * p2pDuration.GetSeconds();
    NS_TEST_EXPECT_MSG_EQ(expectedPacketsP2p,
                          expectedPacketsP2p,
                          "Unexpected amount of packets sent from P2P STA to ADHOC");
    NS_TEST_EXPECT_MSG_EQ(expectedPacketsP2p,
                          expectedPacketsP2p,
                          "Unexpected amount of packets sent from ADHOC to P2P STA");

    const auto expectedDroppedPacketsInfrastructure =
        maxPacketsPerDirection - expectedPacketsInfrastructure;
    NS_TEST_EXPECT_MSG_EQ((m_droppedPackets.contains(AP_TO_P2P_STA_PROTOCOL)
                               ? m_droppedPackets.at(AP_TO_P2P_STA_PROTOCOL)
                               : 0),
                          expectedDroppedPacketsInfrastructure,
                          "Unexpected amount of dropped packet sent from AP to P2P STA");
    NS_TEST_EXPECT_MSG_EQ((m_droppedPackets.contains(P2P_STA_TO_AP_PROTOCOL)
                               ? m_droppedPackets.at(P2P_STA_TO_AP_PROTOCOL)
                               : 0),
                          expectedDroppedPacketsInfrastructure,
                          "Unexpected amount of dropped packet sent from P2P STA to AP");

    // when P2P is disabled, the adhoc stations also transmits packets to the STA, but these
    // should be dropped by the STA since its P2P capabilities are turned off
    const auto expectedDroppedPacketsP2p = maxPacketsPerDirection - expectedPacketsP2p;
    NS_TEST_EXPECT_MSG_EQ(
        (m_droppedPackets.contains(ADHOC_TO_P2P_STA_PROTOCOL)
             ? m_droppedPackets.at(ADHOC_TO_P2P_STA_PROTOCOL)
             : 0),
        expectedDroppedPacketsP2p,
        "Unexpected amount of dropped packet sent from ADHOC to P2P STA (dropped by P2P STA)");

    // check link used for P2P communication from STA to adhoc
    std::vector<std::pair<Time, uint8_t>> p2pStaTxLinks{};
    std::copy_if(m_txLinks.at(P2P_STA_TO_ADHOC_PROTOCOL).cbegin(),
                 m_txLinks.at(P2P_STA_TO_ADHOC_PROTOCOL).cend(),
                 std::back_inserter(p2pStaTxLinks),
                 [&p2pPeriods = m_params.p2pActivatedPeriods](const auto& txLinkInfo) {
                     return std::any_of(p2pPeriods.cbegin(),
                                        p2pPeriods.cend(),
                                        [tstamp = txLinkInfo.first](const auto& period) {
                                            return (tstamp >= period.start &&
                                                    tstamp <= period.stop);
                                        });
                 });
    uint8_t expectedP2pLinkId{0};
    for (uint8_t phyId = 0; phyId < m_p2pSta->GetDevice()->GetNPhys(); ++phyId)
    {
        if (m_adhocMac->GetDevice()->GetPhy(SINGLE_LINK_OP_ID)->GetPhyBand() ==
            m_p2pSta->GetDevice()->GetPhy(phyId)->GetPhyBand())
        {
            auto linkId = m_p2pSta->GetLinkForPhy(phyId);
            NS_TEST_ASSERT_MSG_EQ(linkId.has_value(), true, "No link found for PHY ID " << +phyId);
            expectedP2pLinkId = *linkId;
            break;
        }
    }
    NS_TEST_EXPECT_MSG_EQ(
        (!p2pStaTxLinks.empty() && std::all_of(p2pStaTxLinks.cbegin(),
                                               p2pStaTxLinks.cend(),
                                               [expectedP2pLinkId](const auto& txLinkInfo) {
                                                   return txLinkInfo.second == expectedP2pLinkId;
                                               })),
        true,
        "Unexpected link ID used by P2P STA during P2P period");

    // check link used for P2P communication from adhoc to STA
    NS_TEST_EXPECT_MSG_EQ((m_txLinks.contains(ADHOC_TO_P2P_STA_PROTOCOL) &&
                           std::all_of(m_txLinks.at(ADHOC_TO_P2P_STA_PROTOCOL).cbegin(),
                                       m_txLinks.at(ADHOC_TO_P2P_STA_PROTOCOL).cend(),
                                       [](const auto& txLinkInfo) {
                                           return txLinkInfo.second == SINGLE_LINK_OP_ID;
                                       })),
                          true,
                          "Unexpected link ID used by adhoc STA");

    // check links used for infrastructure mode
    if (expectedPacketsInfrastructure > 0)
    {
        std::set<uint8_t> infraApLinks{};
        std::vector<std::string> infraChannels{};
        for (const auto& apChannel : m_params.apChannels)
        {
            if (std::find_if(m_params.staChannels.cbegin(),
                             m_params.staChannels.cend(),
                             [&apChannel](const auto& staChannel) {
                                 return apChannel == staChannel;
                             }) != m_params.staChannels.cend())
            {
                infraChannels.push_back(apChannel);
            }
        }
        NS_ASSERT_MSG(!infraChannels.empty(),
                      "There should be at least one common channel between AP and STAs");
        for (const auto& channel : infraChannels)
        {
            const auto apIt =
                std::find_if(m_params.apChannels.cbegin(),
                             m_params.apChannels.cend(),
                             [&channel](const auto& apChannel) { return channel == apChannel; });
            const auto apLinkId = std::distance(m_params.apChannels.cbegin(), apIt);
            infraApLinks.insert(apLinkId);
        }
        const auto infraStaLinks = m_staMac->GetSetupLinkIds();
        const auto infraP2pStaLinks =
            m_params.infraLinkSetP2pSta.empty() ? infraStaLinks : m_params.infraLinkSetP2pSta;
        NS_ASSERT(!infraApLinks.empty());
        NS_ASSERT(!infraStaLinks.empty());
        NS_ASSERT(!infraP2pStaLinks.empty());

        NS_TEST_EXPECT_MSG_EQ((m_txLinks.contains(AP_TO_P2P_STA_PROTOCOL) &&
                               std::all_of(m_txLinks.at(AP_TO_P2P_STA_PROTOCOL).cbegin(),
                                           m_txLinks.at(AP_TO_P2P_STA_PROTOCOL).cend(),
                                           [&infraApLinks](const auto& txLinkInfo) {
                                               return infraApLinks.contains(txLinkInfo.second);
                                           })),
                              true,
                              "Unexpected link ID used for AP to P2P STA");
        NS_TEST_EXPECT_MSG_EQ((m_txLinks.contains(P2P_STA_TO_AP_PROTOCOL) &&
                               std::all_of(m_txLinks.at(P2P_STA_TO_AP_PROTOCOL).cbegin(),
                                           m_txLinks.at(P2P_STA_TO_AP_PROTOCOL).cend(),
                                           [&infraP2pStaLinks](const auto& txLinkInfo) {
                                               return infraP2pStaLinks.contains(txLinkInfo.second);
                                           })),
                              true,
                              "Unexpected link ID used for P2P STA to AP");
        NS_TEST_EXPECT_MSG_EQ((m_txLinks.contains(P2P_STA_TO_STA_PROTOCOL) &&
                               std::all_of(m_txLinks.at(P2P_STA_TO_STA_PROTOCOL).cbegin(),
                                           m_txLinks.at(P2P_STA_TO_STA_PROTOCOL).cend(),
                                           [&infraP2pStaLinks](const auto& txLinkInfo) {
                                               return infraP2pStaLinks.contains(txLinkInfo.second);
                                           })),
                              true,
                              "Unexpected link ID used for P2P STA to STA");
        NS_TEST_EXPECT_MSG_EQ((m_txLinks.contains(STA_TO_P2P_STA_PROTOCOL) &&
                               std::all_of(m_txLinks.at(STA_TO_P2P_STA_PROTOCOL).cbegin(),
                                           m_txLinks.at(STA_TO_P2P_STA_PROTOCOL).cend(),
                                           [&infraStaLinks](const auto& txLinkInfo) {
                                               return infraStaLinks.contains(txLinkInfo.second);
                                           })),
                              true,
                              "Unexpected link ID used for STA to P2P STA");
        std::vector<std::pair<Time, uint8_t>> nonP2pStaTxLinks{};
        std::copy_if(m_txLinks.at(P2P_STA_TO_ADHOC_PROTOCOL).cbegin(),
                     m_txLinks.at(P2P_STA_TO_ADHOC_PROTOCOL).cend(),
                     std::back_inserter(nonP2pStaTxLinks),
                     [&p2pPeriods = m_params.p2pActivatedPeriods](const auto& txLinkInfo) {
                         return std::none_of(p2pPeriods.cbegin(),
                                             p2pPeriods.cend(),
                                             [tstamp = txLinkInfo.first](const auto& period) {
                                                 return (tstamp >= period.start &&
                                                         tstamp <= period.stop);
                                             });
                     });
        NS_TEST_EXPECT_MSG_EQ((!nonP2pStaTxLinks.empty() &&
                               std::all_of(nonP2pStaTxLinks.cbegin(),
                                           nonP2pStaTxLinks.cend(),
                                           [&infraP2pStaLinks](const auto& txLinkInfo) {
                                               return infraP2pStaLinks.contains(txLinkInfo.second);
                                           })),
                              true,
                              "Unexpected link ID used by P2P STA during non-P2P period");
    }
}

void
P2pTest::SetChannels(SpectrumWifiPhyHelper& helper,
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

    for (const auto& [band, channel] : channelMap)
    {
        helper.AddChannel(channel, band);
    }
}

Address
P2pTest::GetPeerAddress(Ptr<WifiMac> srcMac, Ptr<WifiMac> dstMac) const
{
    if (const auto nLinks = dstMac->GetNLinks(); (srcMac->GetNLinks() == 1) && (nLinks > 1))
    {
        for (std::size_t id = 0; id < nLinks; ++id)
        {
            if (dstMac->GetWifiPhy(id)->GetPhyBand() == srcMac->GetWifiPhy()->GetPhyBand())
            {
                return dstMac->GetFrameExchangeManager(id)->GetAddress();
            }
        }
    }
    return dstMac->GetAddress();
}

void
P2pTest::CreateTraffic(Ptr<Node> txNode,
                       Ptr<Node> rxNode,
                       const Address& destinationAddress,
                       uint16_t protocol,
                       Time start)
{
    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice(txNode->GetDevice(0)->GetIfIndex());
    socketAddr.SetPhysicalAddress(destinationAddress);
    socketAddr.SetProtocol(protocol);

    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(ProtocolToPayloadSizeMap.at(protocol)));
    client->SetAttribute("MaxPackets", UintegerValue(5));
    client->SetAttribute("Interval", TimeValue(Seconds(1.0)));
    client->SetRemote(socketAddr);
    txNode->AddApplication(client);
    client->SetStartTime(start);
    client->SetStopTime(m_testDuration);

    auto server = CreateObject<PacketSocketServer>();
    server->SetLocal(socketAddr);
    rxNode->AddApplication(server);
    server->SetStartTime(Seconds(0.0));
    server->SetStopTime(m_testDuration);
}

void
P2pTest::DoSetup()
{
    // LogComponentEnableAll(LOG_PREFIX_TIME);
    // LogComponentEnableAll(LOG_PREFIX_NODE);
    // WifiHelper::EnableLogComponents();
    // LogComponentEnable("WifiP2pTest", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 100;

    Config::SetDefault("ns3::WifiPhy::FixedPhyBand", BooleanValue(true));

    ChannelMap channelMap{{WIFI_SPECTRUM_2_4_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_5_GHZ, CreateObject<MultiModelSpectrumChannel>()},
                          {WIFI_SPECTRUM_6_GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    SpectrumWifiPhyHelper apPhyHelper;
    SpectrumWifiPhyHelper staPhyHelper;
    SpectrumWifiPhyHelper p2pAdhocPhyHelper;

    SetChannels(apPhyHelper, m_params.apChannels, channelMap);
    SetChannels(staPhyHelper, m_params.staChannels, channelMap);
    SetChannels(p2pAdhocPhyHelper, {m_params.p2pChannel}, channelMap);

    NodeContainer wifiApNode(1);
    NodeContainer wifiStaNodes(2);
    NodeContainer wifiAdhocNode(1);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);

    WifiMacHelper mac;

    mac.SetType("ns3::ApWifiMac",
                "Ssid",
                SsidValue(Ssid("bss-ssid")),
                "BeaconGeneration",
                BooleanValue(true),
                "EnableBeaconJitter",
                BooleanValue(true));
    auto apDevice = wifi.Install(apPhyHelper, mac, wifiApNode.Get(0));
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    auto apMac = DynamicCast<ApWifiMac>(DynamicCast<WifiNetDevice>(apDevice.Get(0))->GetMac());

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(Ssid("bss-ssid")));
    auto staDevices = wifi.Install(staPhyHelper, mac, wifiStaNodes.Get(0));
    m_staMac = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDevices.Get(0))->GetMac());

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(Ssid("wrong-ssid")));
    staDevices.Add(wifi.Install(staPhyHelper, mac, wifiStaNodes.Get(1)));

    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);
    m_p2pSta = DynamicCast<StaWifiMac>(DynamicCast<WifiNetDevice>(staDevices.Get(1))->GetMac());
    if (!m_params.infraLinkSetP2pSta.empty())
    {
        std::stringstream ss;
        ss << "0 ";
        std::copy(m_params.infraLinkSetP2pSta.cbegin(),
                  m_params.infraLinkSetP2pSta.cend(),
                  std::ostream_iterator<uint16_t>(ss, ","));
        auto staEhtConfig = m_p2pSta->GetEhtConfiguration();
        staEhtConfig->m_tidLinkMappingSupport = WifiTidToLinkMappingNegSupport::ANY_LINK_SET;
        staEhtConfig->SetAttribute("TidToLinkMappingDl", StringValue(ss.str()));
        staEhtConfig->SetAttribute("TidToLinkMappingUl", StringValue(ss.str()));
    }
    mac.SetType("ns3::AdhocWifiMac",
                "Ssid",
                SsidValue(Ssid("ibss-ssid")),
                "BeaconGeneration",
                BooleanValue(true));

    auto adhocDevice = wifi.Install(p2pAdhocPhyHelper, mac, wifiAdhocNode.Get(0));
    streamNumber += WifiHelper::AssignStreams(adhocDevice, streamNumber);
    m_adhocMac =
        DynamicCast<AdhocWifiMac>(DynamicCast<WifiNetDevice>(adhocDevice.Get(0))->GetMac());

    for (uint8_t phyId = 0; phyId < apMac->GetDevice()->GetNPhys(); ++phyId)
    {
        Config::ConnectWithoutContext("/NodeList/0/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                                          std::to_string(phyId) + "/PhyTxPsduBegin",
                                      MakeCallback(&P2pTest::Transmit, this).Bind(apMac, phyId));
    }

    for (uint8_t phyId = 0; phyId < m_staMac->GetDevice()->GetNPhys(); ++phyId)
    {
        Config::ConnectWithoutContext("/NodeList/1/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                                          std::to_string(phyId) + "/PhyTxPsduBegin",
                                      MakeCallback(&P2pTest::Transmit, this).Bind(m_staMac, phyId));
    }

    for (uint8_t phyId = 0; phyId < m_p2pSta->GetDevice()->GetNPhys(); ++phyId)
    {
        Config::ConnectWithoutContext("/NodeList/2/DeviceList/*/$ns3::WifiNetDevice/Phys/" +
                                          std::to_string(phyId) + "/PhyTxPsduBegin",
                                      MakeCallback(&P2pTest::Transmit, this).Bind(m_p2pSta, phyId));
    }

    for (uint8_t phyId = 0; phyId < m_adhocMac->GetDevice()->GetNPhys(); ++phyId)
    {
        Config::ConnectWithoutContext(
            "/NodeList/3/DeviceList/*/$ns3::WifiNetDevice/Phys/" + std::to_string(phyId) +
                "/PhyTxPsduBegin",
            MakeCallback(&P2pTest::Transmit, this).Bind(m_adhocMac, phyId));
    }

    // Uncomment the lines below to write PCAP files
    // apPhyHelper.EnablePcap("wifi-p2p_AP", apDevice);
    // staPhyHelper.EnablePcap("wifi-p2p_STA", staDevices.Get(0));
    // staPhyHelper.EnablePcap("wifi-p2p_P2P_STA", staDevices.Get(1));
    // p2pAdhocPhyHelper.EnablePcap("wifi-p2p_ADHOC", adhocDevice);

    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    positionAlloc->Add(Vector(2.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);
    mobility.Install(wifiAdhocNode);

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);
    packetSocket.Install(wifiAdhocNode);

    m_testDuration = m_params.p2pActivatedPeriods.back().stop;
    if (!m_params.infrastructureActivatedPeriods.empty())
    {
        m_testDuration =
            std::max(m_testDuration, m_params.infrastructureActivatedPeriods.back().stop);
    }

    // traffic from P2P STA to ADHOC
    CreateTraffic(wifiStaNodes.Get(1),
                  wifiAdhocNode.Get(0),
                  adhocDevice.Get(0)->GetAddress(),
                  P2P_STA_TO_ADHOC_PROTOCOL,
                  Seconds(0.2));

    // traffic from ADHOC to P2P STA
    CreateTraffic(wifiAdhocNode.Get(0),
                  wifiStaNodes.Get(1),
                  GetPeerAddress(m_adhocMac, m_p2pSta),
                  ADHOC_TO_P2P_STA_PROTOCOL,
                  Seconds(0.3));

    // traffic from P2P STA to STA
    CreateTraffic(wifiStaNodes.Get(1),
                  wifiStaNodes.Get(0),
                  GetPeerAddress(apMac, m_staMac),
                  P2P_STA_TO_STA_PROTOCOL,
                  Seconds(0.4));

    // traffic from STA to P2P STA
    CreateTraffic(wifiStaNodes.Get(0),
                  wifiStaNodes.Get(1),
                  GetPeerAddress(apMac, m_p2pSta),
                  STA_TO_P2P_STA_PROTOCOL,
                  Seconds(0.5));

    // traffic from AP to P2P STA
    CreateTraffic(wifiApNode.Get(0),
                  wifiStaNodes.Get(1),
                  GetPeerAddress(apMac, m_p2pSta),
                  AP_TO_P2P_STA_PROTOCOL,
                  Seconds(0.7));

    // traffic from P2P STA to AP
    CreateTraffic(wifiStaNodes.Get(1),
                  wifiApNode.Get(0),
                  apDevice.Get(0)->GetAddress(),
                  P2P_STA_TO_AP_PROTOCOL,
                  Seconds(0.8));

    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::PacketSocketServer/Rx",
                                  MakeCallback(&P2pTest::Receive, this));

    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop",
                                  MakeCallback(&P2pTest::MacTxDrop, this));

    Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRxDrop",
                                  MakeCallback(&P2pTest::MacRxDrop, this));
}

void
P2pTest::DoRun()
{
    for (const auto& infrastructureActivatedPeriod : m_params.infrastructureActivatedPeriods)
    {
        Simulator::Schedule(infrastructureActivatedPeriod.start,
                            &P2pTest::SetStaSsid,
                            this,
                            Ssid("bss-ssid"));
        Simulator::Schedule(infrastructureActivatedPeriod.stop,
                            &P2pTest::SetStaSsid,
                            this,
                            Ssid("wrong-ssid"));
    }

    for (const auto& p2pActivatedPeriod : m_params.p2pActivatedPeriods)
    {
        Simulator::Schedule(p2pActivatedPeriod.start, &P2pTest::SetP2pEnabled, this, true);
        Simulator::Schedule(p2pActivatedPeriod.stop, &P2pTest::SetP2pEnabled, this, false);
    }

    Simulator::Stop(m_testDuration);
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi P2P test suite
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

    for (const auto& [txStandard, rxStandard] : std::vector<std::pair<WifiStandard, WifiStandard>>{
             {WIFI_STANDARD_80211ax, WIFI_STANDARD_80211ax},
             {WIFI_STANDARD_80211ax, WIFI_STANDARD_80211ac},
             {WIFI_STANDARD_80211ac, WIFI_STANDARD_80211ax}})
    {
        for (const auto beaconing : {true, false})
        {
            AddTestCase(new IbssCapabilitiesTest(txStandard, rxStandard, beaconing),
                        TestCase::Duration::QUICK);
        }
    }
    // clang-format off
    for (const auto& [name, params] : std::vector<std::pair<std::string, P2pTest::P2pTestParams>>{
             {"single link on 5 GHz",
              {
                  .apChannels = {"{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{36, 0, BAND_5GHZ, 0}"},
                 .p2pChannel = "{36, 0, BAND_5GHZ, 0}",
              }},
             {"single link on 2.4 GHz",
              {
                  .apChannels = {"{2, 0, BAND_2_4GHZ, 0}"},
                 .staChannels = {"{2, 0, BAND_2_4GHZ, 0}"},
                 .p2pChannel = "{2, 0, BAND_2_4GHZ, 0}",
              }},
             {"single link on 6 GHz",
              {
                  .apChannels = {"{1, 0, BAND_6GHZ, 0}"},
                 .staChannels = {"{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{1, 0, BAND_6GHZ, 0}",
              }},
             {"P2P only on 5 GHz",
              {
                  .apChannels = {"{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{36, 0, BAND_5GHZ, 0}"},
                 .p2pChannel = "{36, 0, BAND_5GHZ, 0}",
                 .p2pActivatedPeriods = {{Seconds(0), Seconds(5)}},
                 .infrastructureActivatedPeriods = {},
              }},
             {"P2P only on 2.4 GHz",
              {
                  .apChannels = {"{2, 0, BAND_2_4GHZ, 0}"},
                 .staChannels = {"{2, 0, BAND_2_4GHZ, 0}"},
                 .p2pChannel = "{2, 0, BAND_2_4GHZ, 0}",
                 .p2pActivatedPeriods = {{Seconds(0), Seconds(5)}},
                 .infrastructureActivatedPeriods = {},
              }},
             {"P2P only on 6 GHz",
              {
                  .apChannels = {"{1, 0, BAND_6GHZ, 0}"},
                 .staChannels = {"{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{1, 0, BAND_6GHZ, 0}",
                 .p2pActivatedPeriods = {{Seconds(0), Seconds(5)}},
                 .infrastructureActivatedPeriods = {},
              }},
             {"AP MLD with 2 links, non-APs MLD with 2 links, P2P on first link",
              {
                  .apChannels = {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                 .p2pChannel = "{2, 0, BAND_2_4GHZ, 0}",
              }},
             {"AP MLD with 2 links, non-APs MLD with 2 links, P2P on second link",
              {
                  .apChannels = {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                 .p2pChannel = "{36, 0, BAND_5GHZ, 0}",
              }},
             {"AP MLD with 2 links, non-APs SLD, P2P on single link",
              {
                  .apChannels = {"{1, 0, BAND_6GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{36, 0, BAND_5GHZ, 0}"},
                 .p2pChannel = "{36, 0, BAND_5GHZ, 0}",
              }},
             {"AP SLD, non-APs MLD with 2 links, AP and P2P on first link",
              {
                  .apChannels = {"{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{36, 0, BAND_5GHZ, 0}",
              }},
             {"AP SLD, non-APs MLD with 2 links, AP and P2P on second link",
              {
                  .apChannels = {"{1, 0, BAND_6GHZ, 0}"},
                 .staChannels = {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{1, 0, BAND_6GHZ, 0}",
              }},
             {"AP SLD, non-APs MLD with 2 links, AP on first link, P2P on second link",
              {
                  .apChannels = {"{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{1, 0, BAND_6GHZ, 0}",
              }},
             {"AP SLD, non-APs MLD with 2 links, AP on second link, P2P on first link",
              {
                  .apChannels = {"{1, 0, BAND_6GHZ, 0}"},
                 .staChannels = {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{36, 0, BAND_5GHZ, 0}",
              }},
             {"AP MLD with 2 links, non-APs MLD with 3 links, P2P on dedicated link (last link)",
              {
                  .apChannels = {"{2, 0, BAND_2_4GHZ, 0}", "{36, 0, BAND_5GHZ, 0}"},
                 .staChannels = {"{2, 0, BAND_2_4GHZ, 0}",
                                 "{36, 0, BAND_5GHZ, 0}",
                                 "{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{1, 0, BAND_6GHZ, 0}",
              }},
             {"AP MLD with 2 links, non-APs MLD with 3 links, P2P on dedicated link (first link)",
              {
                  .apChannels = {"{36, 0, BAND_5GHZ, 0}", "{1, 0, BAND_6GHZ, 0}"},
                 .staChannels = {"{2, 0, BAND_2_4GHZ, 0}",
                                 "{36, 0, BAND_5GHZ, 0}",
                                 "{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{2, 0, BAND_2_4GHZ, 0}",
              }},
             {"AP MLD with 3 links, non-APs MLD with 3 links, TID-to-link mapping to use the first "
              "2 links, P2P on last link",
              {
                  .apChannels = {"{2, 0, BAND_2_4GHZ, 0}",
                                 "{36, 0, BAND_5GHZ, 0}",
                                 "{1, 0, BAND_6GHZ, 0}"},
                 .staChannels = {"{2, 0, BAND_2_4GHZ, 0}",
                                 "{36, 0, BAND_5GHZ, 0}",
                                 "{1, 0, BAND_6GHZ, 0}"},
                 .p2pChannel = "{1, 0, BAND_6GHZ, 0}",
                 .infraLinkSetP2pSta = {0, 1},
              }},
         })
    // clang-format on
    {
        AddTestCase(new P2pTest(name, params), TestCase::Duration::QUICK);
    }
}

static WifiP2pTestSuite g_wifiP2pTestSuite; ///< the test suite
