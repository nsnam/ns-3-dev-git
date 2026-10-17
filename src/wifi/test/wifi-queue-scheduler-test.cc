/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/node-list.h"
#include "ns3/nstime.h"
#include "ns3/packet-socket-client.h"
#include "ns3/packet-socket-helper.h"
#include "ns3/packet-socket-server.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-static-setup-helper.h"

#include <algorithm>
#include <iomanip>
#include <list>
#include <sstream>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiQueueSchedulerTest");

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * A configurable number of non-AP STAs associate with an AP. All devices have the same number of
 * links (1 or 2). The MAC queue scheduler to be installed on the devices is configurable, too. The
 * test input is represented by a list of DL packets addressed to the non-AP STAs, along with the
 * interval between the generation of consecutive packets. A-MPDU aggregation is disabled, so that
 * one packet at a time is transmitted by the AP. The sequence of packets transmitted by the AP is
 * compared to the expected sequence of packets based on the configured MAC queue scheduler.
 */
class WifiQueueSchedulerTest : public TestCase
{
  public:
    /// @brief Input parameters
    struct InputParams
    {
        std::string schedulerTid;  ///< scheduler TypeId
        std::size_t nLinks;        ///< number of links for each device
        std::size_t nMaxInflights; ///< max number of links on which an MPDU can be
                                   ///< simultaneously inflight
        std::vector<std::pair<std::size_t, Time>> packets; ///< destination STA ID (starting at 0)
                                                           ///< and interval since the previous
                                                           ///< packet for all packets to enqueue
        /// @return a string with the parameters' values
        std::string ToStr() const;
    };

    /// @brief Expected results
    struct ExpectedResults
    {
        std::vector<std::size_t> servedStas; ///< IDs of STAs in the order in which they are served
    };

    /**
     * Constructor.
     *
     * @param params the input parameters
     * @param results the expected parameters
     */
    WifiQueueSchedulerTest(const InputParams& params, const ExpectedResults& results);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Callback invoked when a FEM passes PSDUs to the PHY.
     *
     * @param mac the MAC transmitting the PSDUs
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPower the tx power
     */
    void Transmit(Ptr<WifiMac> mac,
                  uint8_t phyId,
                  WifiConstPsduMap psduMap,
                  WifiTxVector txVector,
                  Watt_u txPower);

    /**
     * Get an application generating packets according to given parameters.
     *
     * @param staId the ID of the STA the packets are addressed to
     * @param count the number of packets to generate
     * @param pktSize the size of the packets to generate
     * @return an application generating the given number packets from the AP to the given STA
     */
    Ptr<PacketSocketClient> GetApplication(std::size_t staId,
                                           std::size_t count,
                                           std::size_t pktSize) const;

    /**
     * Enqueue packets according to input parameters.
     */
    void EnqueuePackets();

    InputParams m_params;                   ///< input parameters
    ExpectedResults m_expectedResults;      ///< expected results
    std::list<std::size_t> m_servedStas;    ///< IDs of STAs in the order they are actually served
    Ptr<ApWifiMac> m_apMac;                 ///< the AP wifi MAC
    std::vector<Ptr<StaWifiMac>> m_staMacs; ///< the STA wifi MACs
    std::vector<PacketSocketAddress> m_dlSockets; ///< packet socket addresses for DL traffic
    const Time m_duration{MilliSeconds(100)};     ///< simulation duration
};

std::string
WifiQueueSchedulerTest::InputParams::ToStr() const
{
    std::stringstream ss;
    ss << schedulerTid << ", nLinks=" << nLinks << ", nMaxInflights=" << nMaxInflights
       << ", packets=";
    for (const auto& [staId, interval] : packets)
    {
        ss << "(" << staId << ", " << interval << ")";
    }
    return ss.str();
}

WifiQueueSchedulerTest::WifiQueueSchedulerTest(const InputParams& params,
                                               const ExpectedResults& results)
    : TestCase("Test case for wifi queue scheduler " + params.ToStr()),
      m_params(params),
      m_expectedResults(results)
{
    NS_TEST_ASSERT_MSG_EQ(m_params.packets.empty(), false, "No packets to generate");
    NS_TEST_ASSERT_MSG_EQ((m_params.nLinks == 1) || (m_params.nLinks == 2),
                          true,
                          "Unsupported number of links (" << +m_params.nLinks
                                                          << "), must be 1 or 2");
}

void
WifiQueueSchedulerTest::DoSetup()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    int64_t streamNumber = 10;

    const auto maxIt =
        std::max_element(m_params.packets.cbegin(),
                         m_params.packets.cend(),
                         [](auto&& lhs, auto&& rhs) { return lhs.first < rhs.first; });
    const std::size_t nStas = maxIt->first + 1;

    NodeContainer wifiStaNodes(nStas);
    NodeContainer wifiApNode(1);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager");

    SpectrumWifiPhyHelper phy(m_params.nLinks);
    phy.SetChannel(CreateObject<MultiModelSpectrumChannel>());
    // use default 40 MHz channel in 5 GHz band for link 0
    phy.Set(0, "ChannelSettings", StringValue("{0, 40, BAND_5GHZ, 0}"));
    if (m_params.nLinks == 2)
    {
        // use default 40 MHz channel in 6 GHz band for link 1
        phy.Set(1, "ChannelSettings", StringValue("{0, 40, BAND_6GHZ, 0}"));
    }

    WifiMacHelper mac;
    mac.SetMacQueueScheduler(m_params.schedulerTid);
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(Ssid("scheduler-ssid")));

    auto staDevices = wifi.Install(phy, mac, wifiStaNodes);
    for (uint32_t i = 0; i < staDevices.GetN(); ++i)
    {
        m_staMacs.emplace_back(
            StaticCast<StaWifiMac>(StaticCast<WifiNetDevice>(staDevices.Get(i))->GetMac()));
    }

    mac.SetType("ns3::ApWifiMac",
                "BeaconGeneration",
                BooleanValue(false),
                "BE_MaxAmpduSize",
                UintegerValue(0)); // disable A-MPDU aggregation
    mac.SetEdca(AC_BE, "NMaxInflights", UintegerValue(m_params.nMaxInflights));

    auto apDevice = wifi.Install(phy, mac, wifiApNode);
    m_apMac = StaticCast<ApWifiMac>(StaticCast<WifiNetDevice>(apDevice.Get(0))->GetMac());

    /* static setup of association and BA agreements */
    auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
    NS_ASSERT(apDev);
    WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevices);
    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevices, {0});

    // Assign fixed streams to random variables in use
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    PacketSocketHelper packetSocket;
    packetSocket.Install(wifiApNode);
    packetSocket.Install(wifiStaNodes);

    // install a packet socket server on the STAs
    for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i)
    {
        PacketSocketAddress srvAddr;
        auto device = DynamicCast<WifiNetDevice>(wifiStaNodes.Get(i)->GetDevice(0));
        NS_TEST_ASSERT_MSG_NE(device, nullptr, "Expected a WifiNetDevice");
        srvAddr.SetSingleDevice(device->GetIfIndex());
        srvAddr.SetProtocol(1);

        auto server = CreateObject<PacketSocketServer>();
        server->SetLocal(srvAddr);
        wifiStaNodes.Get(i)->AddApplication(server);
        server->SetStartTime(Time{0}); // now
        server->SetStopTime(m_duration);
    }

    // set DL packet socket
    for (const auto& staMac : m_staMacs)
    {
        m_dlSockets.emplace_back();
        m_dlSockets.back().SetSingleDevice(m_apMac->GetDevice()->GetIfIndex());
        m_dlSockets.back().SetPhysicalAddress(staMac->GetDevice()->GetAddress());
        m_dlSockets.back().SetProtocol(1);
    }

    // Trace PSDUs passed to the PHY on all devices
    for (auto nodeIt = NodeList::Begin(); nodeIt != NodeList::End(); ++nodeIt)
    {
        auto dev = DynamicCast<WifiNetDevice>((*nodeIt)->GetDevice(0));
        for (uint8_t phyId = 0; phyId < dev->GetNPhys(); ++phyId)
        {
            dev->GetPhy(phyId)->TraceConnectWithoutContext(
                "PhyTxPsduBegin",
                MakeCallback(&WifiQueueSchedulerTest::Transmit, this).Bind(dev->GetMac(), phyId));
        }
    }
}

void
WifiQueueSchedulerTest::Transmit(Ptr<WifiMac> mac,
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
    NS_LOG_INFO("TXVECTOR = " << txVector << " " << mac << " " << m_apMac << "\n");

    const auto psdu = psduMap.cbegin()->second;
    const auto addr1 = psdu->GetAddr1();

    if ((mac != m_apMac) || addr1.IsBroadcast())
    {
        return;
    }

    NS_TEST_ASSERT_MSG_LT(m_servedStas.size(),
                          m_expectedResults.servedStas.size(),
                          "Served more STAs than expected");

    auto expectedServedSta = m_expectedResults.servedStas.at(m_servedStas.size());

    NS_TEST_ASSERT_MSG_LT(expectedServedSta,
                          m_staMacs.size(),
                          "Expected STA ID exceeds the number of STAs");

    std::size_t actualServedSta{0};

    for (std::size_t id = 0; id < m_staMacs.size(); ++id)
    {
        if (m_staMacs[id]->GetLinkIdByAddress(addr1))
        {
            actualServedSta = id;
            break;
        }
    }

    NS_TEST_ASSERT_MSG_NE(actualServedSta,
                          m_staMacs.size(),
                          "Receiver address (" << addr1 << ") does not belong to any STA");

    NS_TEST_EXPECT_MSG_EQ(actualServedSta,
                          expectedServedSta,
                          "Incorrect STA served at time " << Simulator::Now());

    m_servedStas.emplace_back(actualServedSta);
}

Ptr<PacketSocketClient>
WifiQueueSchedulerTest::GetApplication(std::size_t staId,
                                       std::size_t count,
                                       std::size_t pktSize) const
{
    auto client = CreateObject<PacketSocketClient>();
    client->SetAttribute("PacketSize", UintegerValue(pktSize));
    client->SetAttribute("MaxPackets", UintegerValue(count));
    client->SetAttribute("Interval", TimeValue(Time{0}));
    client->SetRemote(m_dlSockets[staId]);
    client->SetStartTime(Time{0}); // now
    client->SetStopTime(m_duration - Simulator::Now());

    return client;
}

void
WifiQueueSchedulerTest::EnqueuePackets()
{
    for (Time delayTot; const auto& [staId, delay] : m_params.packets)
    {
        delayTot += delay;
        Simulator::Schedule(delayTot,
                            &Node::AddApplication,
                            m_apMac->GetDevice()->GetNode(),
                            GetApplication(staId, 1, 1000));
    }
}

void
WifiQueueSchedulerTest::DoRun()
{
    EnqueuePackets();

    Simulator::Stop(m_duration);
    Simulator::Run();

    NS_TEST_EXPECT_MSG_EQ(m_servedStas.size(),
                          m_expectedResults.servedStas.size(),
                          "Not all the expected STAs were served");

    Simulator::Destroy();
}

/**
 * @ingroup wifi-test
 * @ingroup tests
 *
 * @brief wifi queue scheduler Test Suite
 */
class WifiQueueSchedulerTestSuite : public TestSuite
{
  public:
    WifiQueueSchedulerTestSuite();
};

WifiQueueSchedulerTestSuite::WifiQueueSchedulerTestSuite()
    : TestSuite("wifi-queue-scheduler", Type::UNIT)
{
    using ParamsResultsList = std::initializer_list<
        std::pair<WifiQueueSchedulerTest::InputParams, WifiQueueSchedulerTest::ExpectedResults>>;

    for (std::size_t nLinks = 1; nLinks <= 2; ++nLinks)
    {
        for (const auto& [params, results] : ParamsResultsList{
                 // with FCFS scheduler, packets are transmitted in the same order they are enqueued
                 {{.schedulerTid = "ns3::FcfsWifiQueueScheduler",
                   .nLinks = nLinks,
                   .nMaxInflights = 1,
                   .packets = {{0, Time{0}},
                               {0, Time{0}},
                               {1, Time{0}},
                               {1, Time{0}},
                               {2, Time{0}},
                               {2, Time{0}}}},
                  {.servedStas = {0, 0, 1, 1, 2, 2}}},
                 // packets are enqueued at intervals of 10us, same result
                 {{.schedulerTid = "ns3::FcfsWifiQueueScheduler",
                   .nLinks = nLinks,
                   .nMaxInflights = 1,
                   .packets = {{0, Time{0}},
                               {0, MicroSeconds(10)},
                               {1, MicroSeconds(10)},
                               {1, MicroSeconds(10)},
                               {2, MicroSeconds(10)},
                               {2, MicroSeconds(10)}}},
                  {.servedStas = {0, 0, 1, 1, 2, 2}}},
                 // with nLinks and nMaxInflights equal to 2, the FCFS scheduler transmits every
                 // packet twice because the STA sending a packet on the first link has the highest
                 // priority when transmitting on the second link
                 {{.schedulerTid = "ns3::FcfsWifiQueueScheduler",
                   .nLinks = nLinks,
                   .nMaxInflights = 2,
                   .packets = {{0, Time{0}}, {1, Time{0}}, {2, Time{0}}, {3, Time{0}}}},
                  {.servedStas = {0, 0, 1, 1, 2, 2, 3, 3}}},
                 // with RR scheduler, STAs are served in a round robin fashion
                 {{.schedulerTid = "ns3::RrWifiQueueScheduler",
                   .nLinks = nLinks,
                   .nMaxInflights = 1,
                   .packets = {{0, Time{0}},
                               {0, Time{0}},
                               {1, Time{0}},
                               {1, Time{0}},
                               {2, Time{0}},
                               {2, Time{0}}}},
                  {.servedStas = {0, 1, 2, 0, 1, 2}}},
                 // packets are enqueued at intervals of 10us, same result
                 {{.schedulerTid = "ns3::RrWifiQueueScheduler",
                   .nLinks = nLinks,
                   .nMaxInflights = 1,
                   .packets = {{0, Time{0}},
                               {0, MicroSeconds(10)},
                               {1, MicroSeconds(10)},
                               {1, MicroSeconds(10)},
                               {2, MicroSeconds(10)},
                               {2, MicroSeconds(10)}}},
                  {.servedStas = {0, 1, 2, 0, 1, 2}}},
                 // with nLinks and nMaxInflights equal to 2, the RR scheduler transmits every
                 // packet just once because the STA sending a packet on the first link has the
                 // lowest priority when transmitting on the second link
                 {{.schedulerTid = "ns3::RrWifiQueueScheduler",
                   .nLinks = nLinks,
                   .nMaxInflights = 2,
                   .packets = {{0, Time{0}}, {1, Time{0}}, {2, Time{0}}, {3, Time{0}}}},
                  {.servedStas = {0, 1, 2, 3}}},
             })
        {
            if (params.nLinks < 2 && params.nMaxInflights > 1)
            {
                continue;
            }

            AddTestCase(new WifiQueueSchedulerTest(params, results), TestCase::Duration::QUICK);
        }
    }
}

static WifiQueueSchedulerTestSuite g_wifiQueueSchedulerTestSuite; ///< the test suite
