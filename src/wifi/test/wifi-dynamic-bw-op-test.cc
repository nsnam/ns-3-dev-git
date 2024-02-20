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

#include "ns3/channel-access-manager.h"
#include "ns3/config.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/qos-txop.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-psdu.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiDynamicBwOpTestSuite");

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * Two BSSes, each with one AP and one non-AP STA, are configured to operate on
 * different channels. Specifically, the operating channel of BSS 1 is the secondary<X>
 * channel of BSS 0, where X is half the width of the channel used by BSS 0.
 * This test demonstrates that, if a transmission is ongoing on BSS 1, we can have
 * a transmission on BSS 0 on its primary<X> channel.
 */
class WifiUseAvailBwTest : public TestCase
{
  public:
    /**
     * Constructor
     *
     * \param channelStr channel setting strings for BSS 0 and BSS 1
     * \param bss0Width width (MHz) of the transmission in BSS 0 started when BSS 1 is transmitting
     */
    WifiUseAvailBwTest(std::initializer_list<std::string> channelStr, uint16_t bss0Width);
    ~WifiUseAvailBwTest() override;

    /**
     * Function to trace packets received by the server application in the given BSS
     * \param bss the given BSS
     * \param p the packet
     * \param addr the address
     */
    void L7Receive(uint8_t bss, Ptr<const Packet> p, const Address& addr);
    /**
     * Callback invoked when a PHY receives a PSDU to transmit
     * \param bss the BSS the PSDU belongs to
     * \param psduMap the PSDU map
     * \param txVector the TX vector
     * \param txPowerW the tx power in Watts
     */
    void Transmit(uint8_t bss, WifiConstPsduMap psduMap, WifiTxVector txVector, double txPowerW);
    /**
     * Check correctness of transmitted frames
     */
    void CheckResults();

  private:
    void DoRun() override;

    /// Information about transmitted frames
    struct FrameInfo
    {
        Time txStart;          ///< Frame start TX time
        Time txDuration;       ///< Frame TX duration
        uint8_t bss;           ///< BSS the frame belongs to
        WifiMacHeader header;  ///< Frame MAC header
        std::size_t nMpdus;    ///< number of MPDUs in the PSDU
        WifiTxVector txVector; ///< TX vector used to transmit the frame
    };

    std::vector<std::string> m_channelStr;        ///< channel setting strings
    uint16_t m_bss0Width;                         ///< width (MHz) of the transmission in BSS 0
                                                  ///< started when BSS 1 is transmitting
    NetDeviceContainer m_staDevices;              ///< container for stations' NetDevices
    NetDeviceContainer m_apDevices;               ///< container for AP's NetDevice
    std::array<PacketSocketAddress, 2> m_sockets; ///< packet sockets for the two BSSes
    std::vector<FrameInfo> m_txPsdus;             ///< transmitted PSDUs
    uint8_t m_txPkts;                             ///< TX packets per BSS (in addition to the two
                                                  ///< required to establish BA agreement)
    std::array<uint8_t, 2> m_rcvPkts;             ///< number of packets received by the stations
};

WifiUseAvailBwTest::WifiUseAvailBwTest(std::initializer_list<std::string> channelStr,
                                       uint16_t bss0Width)
    : TestCase("Check transmission on available bandwidth"),
      m_channelStr(channelStr),
      m_bss0Width(bss0Width),
      m_txPkts(bss0Width / 10), // so that they all fit in an A-MPDU
      m_rcvPkts({0, 0})
{
}

WifiUseAvailBwTest::~WifiUseAvailBwTest()
{
}

void
WifiUseAvailBwTest::L7Receive(uint8_t bss, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_INFO("Received " << p->GetSize() << " bytes in BSS " << +bss);
    m_rcvPkts[bss]++;
}

void
WifiUseAvailBwTest::Transmit(uint8_t bss,
                             WifiConstPsduMap psduMap,
                             WifiTxVector txVector,
                             double txPowerW)
{
    auto psdu = psduMap.begin()->second;
    Time now = Simulator::Now();

    // Log all frames that are not management frames (we are only interested in data
    // frames and acknowledgments) and have been transmitted after 400ms (so as to
    // skip association requests/responses)
    if (!psdu->GetHeader(0).IsMgt() && now > MilliSeconds(400))
    {
        m_txPsdus.push_back({now,
                             WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_5GHZ),
                             bss,
                             psdu->GetHeader(0),
                             psdu->GetNMpdus(),
                             txVector});
    }

    NS_LOG_INFO(now << " BSS " << +bss << " " << psdu->GetHeader(0).GetTypeString() << " seq "
                    << psdu->GetHeader(0).GetSequenceNumber() << " to " << psdu->GetAddr1()
                    << " #MPDUs " << psdu->GetNMpdus() << " size " << psdu->GetSize()
                    << " TX duration "
                    << WifiPhy::CalculateTxDuration(psduMap, txVector, WIFI_PHY_BAND_5GHZ) << "\n"
                    << "TXVECTOR " << txVector << "\n");

    // when AP of BSS 1 starts transmitting (after 1.5 s), we generate packets
    // for the AP of BSS 0 to transmit
    if (bss == 1 && psdu->GetNMpdus() == m_txPkts && now >= Seconds(1.5))
    {
        Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
        client->SetAttribute("PacketSize", UintegerValue(2000));
        client->SetAttribute("MaxPackets", UintegerValue(m_txPkts));
        client->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        client->SetRemote(m_sockets[0]);
        m_apDevices.Get(0)->GetNode()->AddApplication(client);
        client->SetStartTime(Seconds(0));  // start now
        client->SetStopTime(Seconds(1.0)); // stop in a second
        client->Initialize();

        // after 1us (to allow for propagation delay), the largest idle primary
        // channel on the AP of BSS 0 should be the expected one
        Simulator::Schedule(MicroSeconds(1), [&]() {
            auto mac = StaticCast<WifiNetDevice>(m_apDevices.Get(0))->GetMac();
            auto cam = mac->GetChannelAccessManager();
            NS_TEST_EXPECT_MSG_EQ(
                cam->GetLargestIdlePrimaryChannel(MicroSeconds(1), Simulator::Now()),
                m_bss0Width,
                "Unexpected width of the largest idle primary channel");
        });
    }
}

void
WifiUseAvailBwTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(40);
    int64_t streamNumber = 100;

    NodeContainer wifiApNodes(2);
    NodeContainer wifiStaNodes(2);

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    spectrumChannel->SetPropagationDelayModel(delayModel);

    SpectrumWifiPhyHelper phy;
    phy.SetChannel(spectrumChannel);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("HeMcs0"),
                                 "ControlMode",
                                 StringValue("OfdmRate6Mbps"));

    WifiMacHelper apMac;
    apMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(Ssid("dynamic-bw-op-ssid")));

    WifiMacHelper staMac;
    staMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(Ssid("dynamic-bw-op-ssid")));

    // BSS 0
    phy.Set("ChannelSettings", StringValue(m_channelStr.at(0)));

    m_apDevices = wifi.Install(phy, apMac, wifiApNodes.Get(0));
    m_staDevices = wifi.Install(phy, staMac, wifiStaNodes.Get(0));

    // BSS 1
    phy.Set("ChannelSettings", StringValue(m_channelStr.at(1)));

    m_apDevices.Add(wifi.Install(phy, apMac, wifiApNodes.Get(1)));
    m_staDevices.Add(wifi.Install(phy, staMac, wifiStaNodes.Get(1)));

    // schedule association requests at different times
    Ptr<WifiNetDevice> dev;

    // Assign fixed streams to random variables in use
    streamNumber += wifi.AssignStreams(m_apDevices, streamNumber);
    streamNumber += wifi.AssignStreams(m_staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(50.0, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, 50.0, 0.0));
    positionAlloc->Add(Vector(50.0, 50.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNodes);
    mobility.Install(wifiStaNodes);

    NS_LOG_INFO("Position of AP (BSS 0) = "
                << wifiApNodes.Get(0)->GetObject<MobilityModel>()->GetPosition());
    NS_LOG_INFO("Position of AP (BSS 1) = "
                << wifiApNodes.Get(1)->GetObject<MobilityModel>()->GetPosition());
    NS_LOG_INFO("Position of STA (BSS 0) = "
                << wifiStaNodes.Get(0)->GetObject<MobilityModel>()->GetPosition());
    NS_LOG_INFO("Position of STA (BSS 1) = "
                << wifiStaNodes.Get(1)->GetObject<MobilityModel>()->GetPosition());

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNodes);
    packetSocket.Install(wifiStaNodes);

    // DL frames
    for (uint8_t bss : {0, 1})
    {
        m_sockets[bss].SetSingleDevice(m_apDevices.Get(bss)->GetIfIndex());
        m_sockets[bss].SetPhysicalAddress(m_staDevices.Get(bss)->GetAddress());
        m_sockets[bss].SetProtocol(1);

        // the first client application generates two packets in order
        // to trigger the establishment of a Block Ack agreement
        Ptr<PacketSocketClient> client1 = CreateObject<PacketSocketClient>();
        client1->SetAttribute("PacketSize", UintegerValue(500));
        client1->SetAttribute("MaxPackets", UintegerValue(2));
        client1->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
        client1->SetRemote(m_sockets[bss]);
        wifiApNodes.Get(bss)->AddApplication(client1);
        client1->SetStartTime(Seconds(0.5) + bss * MilliSeconds(500));
        client1->SetStopTime(Seconds(2.0));

        // At time 1.5, start a transmission in BSS 1
        if (bss == 1)
        {
            Ptr<PacketSocketClient> client2 = CreateObject<PacketSocketClient>();
            client2->SetAttribute("PacketSize", UintegerValue(2000));
            client2->SetAttribute("MaxPackets", UintegerValue(m_txPkts));
            client2->SetAttribute("Interval", TimeValue(MicroSeconds(0)));
            client2->SetRemote(m_sockets[bss]);
            wifiApNodes.Get(bss)->AddApplication(client2);
            client2->SetStartTime(Seconds(1.5));
            client2->SetStopTime(Seconds(2.0));
        }

        Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
        server->SetLocal(m_sockets[bss]);
        wifiStaNodes.Get(bss)->AddApplication(server);
        server->SetStartTime(Seconds(0.0));
        server->SetStopTime(Seconds(2.0));

        // Trace received packets on non-AP STAs
        Config::ConnectWithoutContext("/NodeList/" + std::to_string(2 + bss) +
                                          "/ApplicationList/*/$ns3::PacketSocketServer/Rx",
                                      MakeCallback(&WifiUseAvailBwTest::L7Receive, this).Bind(bss));
        // Trace PSDUs passed to the PHY of the AP
        Config::ConnectWithoutContext("/NodeList/" + std::to_string(bss) +
                                          "/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxPsduBegin",
                                      MakeCallback(&WifiUseAvailBwTest::Transmit, this).Bind(bss));
        // Trace PSDUs passed to the PHY of the non-AP STA
        Config::ConnectWithoutContext("/NodeList/" + std::to_string(2 + bss) +
                                          "/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxPsduBegin",
                                      MakeCallback(&WifiUseAvailBwTest::Transmit, this).Bind(bss));
    }

    Simulator::Stop(Seconds(2));
    Simulator::Run();

    CheckResults();

    Simulator::Destroy();
}

void
WifiUseAvailBwTest::CheckResults()
{
    NS_TEST_ASSERT_MSG_EQ(m_txPsdus.size(), 12, "Expected 12 transmitted frames");

    // first logged frames are Acks after ADDBA Request/Response frames
    auto psduIt = m_txPsdus.begin();
    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsAck(), true, "Expected Ack after ADDBA Request");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 0, "Expected a transmission in BSS 0");
    psduIt++;

    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsAck(), true, "Expected Ack after ADDBA Response");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 0, "Expected a transmission in BSS 0");
    psduIt++;

    // the first data frame is an A-MPDU sent by the AP of BSS 0 right after the
    // establishment of the Block Ack agreement
    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsQosData(), true, "Expected a QoS data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 0, "Expected a transmission in BSS 0");
    NS_TEST_EXPECT_MSG_EQ(psduIt->nMpdus, 2, "Expected an A-MPDU of 2 MPDUs after Block Ack");
    NS_TEST_EXPECT_MSG_EQ(
        psduIt->txVector.GetChannelWidth(),
        StaticCast<WifiNetDevice>(m_apDevices.Get(0))->GetPhy()->GetChannelWidth(),
        "Expected a transmission on the whole channel width");
    psduIt++;

    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsBlockAck(), true, "Expected Block Ack after data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 0, "Expected a transmission in BSS 0");
    psduIt++;

    // same sequence for BSS 1
    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsAck(), true, "Expected Ack after ADDBA Request");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 1, "Expected a transmission in BSS 1");
    psduIt++;

    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsAck(), true, "Expected Ack after ADDBA Response");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 1, "Expected a transmission in BSS 1");
    psduIt++;

    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsQosData(), true, "Expected a QoS data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 1, "Expected a transmission in BSS 1");
    NS_TEST_EXPECT_MSG_EQ(psduIt->nMpdus, 2, "Expected an A-MPDU of 2 MPDUs after Block Ack");
    NS_TEST_EXPECT_MSG_EQ(
        psduIt->txVector.GetChannelWidth(),
        StaticCast<WifiNetDevice>(m_apDevices.Get(1))->GetPhy()->GetChannelWidth(),
        "Expected a transmission on the whole channel width");
    psduIt++;

    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsBlockAck(), true, "Expected Block Ack after data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 1, "Expected a transmission in BSS 1");
    psduIt++;

    // after some time, we have another A-MPDU transmitted in BSS 1
    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsQosData(), true, "Expected a QoS data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 1, "Expected a transmission in BSS 1");
    NS_TEST_EXPECT_MSG_EQ(psduIt->nMpdus,
                          +m_txPkts,
                          "Expected an A-MPDU of " << +m_txPkts << " MPDUs");
    NS_TEST_EXPECT_MSG_EQ(
        psduIt->txVector.GetChannelWidth(),
        StaticCast<WifiNetDevice>(m_apDevices.Get(1))->GetPhy()->GetChannelWidth(),
        "Expected a transmission on the whole channel width");
    psduIt++;

    // we expect that the AP of BSS 0 starts transmitting while the AP of BSS 1 is transmitting
    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsQosData(), true, "Expected a QoS data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 0, "Expected a transmission in BSS 0");
    NS_TEST_EXPECT_MSG_EQ(psduIt->nMpdus,
                          +m_txPkts,
                          "Expected an A-MPDU of " << +m_txPkts << " MPDUs");
    NS_TEST_EXPECT_MSG_EQ(psduIt->txVector.GetChannelWidth(),
                          m_bss0Width,
                          "Unexpected transmission width");
    NS_TEST_EXPECT_MSG_LT(psduIt->txStart,
                          std::prev(psduIt)->txStart + std::prev(psduIt)->txDuration,
                          "AP 0 is expected to transmit before the end of transmission of AP 1");
    psduIt++;

    // receive a Block Ack in BSS 1 and then a Block Ack in BSS 0
    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsBlockAck(), true, "Expected Block Ack after data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 1, "Expected a transmission in BSS 1");
    psduIt++;

    NS_TEST_EXPECT_MSG_EQ(psduIt->header.IsBlockAck(), true, "Expected Block Ack after data frame");
    NS_TEST_EXPECT_MSG_EQ(+psduIt->bss, 0, "Expected a transmission in BSS 0");
    psduIt++;

    // each application server (on STAs) received 2 packets right after Block Ack
    // agreement establishment and m_txPkts packets afterwards
    NS_TEST_EXPECT_MSG_EQ(+m_rcvPkts[0],
                          2 + m_txPkts,
                          "Unexpected number of packets received by STA 0");
    NS_TEST_EXPECT_MSG_EQ(+m_rcvPkts[1],
                          2 + m_txPkts,
                          "Unexpected number of packets received by STA 1");
}

/**
 * \ingroup wifi-test
 * \ingroup tests
 *
 * \brief wifi dynamic bandwidth operation Test Suite
 */
class WifiDynamicBwOpTestSuite : public TestSuite
{
  public:
    WifiDynamicBwOpTestSuite();
};

WifiDynamicBwOpTestSuite::WifiDynamicBwOpTestSuite()
    : TestSuite("wifi-dynamic-bw-op", Type::UNIT)
{
    /**
     *                    primary20
     *          ┌────────┬────────┐
     *  BSS 0   │   52   │   56   │
     *          └────────┴────────┘
     *
     *          ┌────────┐
     *  BSS 1   │   52   │
     *          └────────┘
     */
    AddTestCase(new WifiUseAvailBwTest({"{54, 40, BAND_5GHZ, 1}", "{52, 20, BAND_5GHZ, 0}"}, 20),
                TestCase::Duration::QUICK);
    /**
     *           ─── primary 40 ───
     *           primary20
     *          ┌────────┬────────┬────────┬────────┐
     *  BSS 0   │   52   │   56   │   60   │   64   │
     *          └────────┴────────┴────────┴────────┘
     *
     *                            ┌────────┬────────┐
     *  BSS 1                     │   60   │   64   │
     *                            └────────┴────────┘
     *                                      primary20
     */
    AddTestCase(new WifiUseAvailBwTest({"{58, 80, BAND_5GHZ, 0}", "{62, 40, BAND_5GHZ, 1}"}, 40),
                TestCase::Duration::QUICK);
    /**
     *                                               ─────────── primary 80 ───────────
     *                                                       primary20
     *          ┌────────┬────────┬────────┬────────┬───────┬────────┬────────┬────────┐
     *  BSS 0   │   36   │   40   │   44   │   48   │  52   │   56   │   60   │   64   │
     *          └────────┴────────┴────────┴────────┴───────┴────────┴────────┴────────┘
     *
     *          ┌────────┬────────┬────────┬────────┐
     *  BSS 1   │   36   │   40   │   44   │   48   │
     *          └────────┴────────┴────────┴────────┘
     *                             primary20
     */
    AddTestCase(new WifiUseAvailBwTest({"{50, 160, BAND_5GHZ, 5}", "{42, 80, BAND_5GHZ, 2}"}, 80),
                TestCase::Duration::QUICK);
}

static WifiDynamicBwOpTestSuite g_wifiDynamicBwOpTestSuite; ///< the test suite
