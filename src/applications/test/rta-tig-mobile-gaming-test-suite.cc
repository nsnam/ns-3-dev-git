/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/application-container.h"
#include "ns3/application-helper.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-socket-address.h"
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/rta-tig-mobile-gaming.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/traced-callback.h"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RtaTigMobileGamingTest");

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * RT mobile gaming traffic test.
 *
 * The test consider traffic values for the two models presented in IEEE 802.11 Real Time
 * Applications TIG Report (Section 4.1.4: Traffic model) and for both downlink and uplink.
 *
 * The test generates traffic between two nodes and keeps track of generated TX packets (size,
 * timestamp and current stage). The test verifies the correct transition to stages, it checks the
 * average sizes of generated packets for each stage match with the settings of the random
 * variables, and it validates the average inter-arrival between generated gaming packets against
 * the expected one based on the settings of the random variable.
 */
class RtaTigMobileGamingTestCase : public TestCase
{
  public:
    /// Information about gaming parameters
    struct GamingParams
    {
        uint32_t minInitialPacketSize{};  //!< Minimum size in bytes for initial packet payload
        uint32_t maxInitialPacketSize{};  //!< Maximum size in bytes for initial packet payload
        uint32_t minEndPacketSize{};      //!< Minimum size in bytes for end packet payload
        uint32_t maxEndPacketSize{};      //!< Maximum size in bytes for end packet payload
        uint32_t packetSizeLevLocation{}; //!< Location of largest extreme value distribution used
                                          //!< to calculate packet sizes
        double packetSizeLevScale{};      //!< Scale of largest extreme value distribution used to
                                          //!< calculate packet sizes
        Time packetArrivalLevLocation{}; //!< Location of largest extreme value distribution used to
                                         //!< calculate packet arrivals
        Time packetArrivalLevScale{};    //!< Scale of largest extreme value distribution used to
                                         //!< calculate packet arrivals
    };

    /**
     * Constructor
     * @param name the name of the test to run
     * @param params the VoIP parameters to use for the test, default parameters are used if not
     * provided
     */
    RtaTigMobileGamingTestCase(const std::string& name, const GamingParams& params);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Record a packets successfully sent
     * @param packet the transmitted packet
     * @param stage the gaming traffic model stage of the packet
     */
    void PacketTx(Ptr<const Packet> packet, RtaTigMobileGaming::TrafficModelStage stage);

    /**
     * Record a packet successfully received
     * @param context the context
     * @param p the packet
     * @param addr the sender's address
     */
    void ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr);

    /// Information about transmitted packet
    struct TxInfo
    {
        uint32_t length{0};  //!< length of the packet in bytes
        Time tstamp{Time()}; //!< timestamp at which the packet is transmitted
        RtaTigMobileGaming::TrafficModelStage stage{
            RtaTigMobileGaming::TrafficModelStage::INITIAL}; //!< traffic model stage when the
                                                             //!< packet is transmitted
    };

    std::vector<TxInfo> m_sent; //!< transmitted gaming packets
    uint64_t m_received{0};     //!< number of bytes received

    GamingParams m_params; //!< parameters of the model
};

RtaTigMobileGamingTestCase::RtaTigMobileGamingTestCase(const std::string& name,
                                                       const GamingParams& params)
    : TestCase{name},
      m_params{params}
{
}

void
RtaTigMobileGamingTestCase::PacketTx(Ptr<const Packet> packet,
                                     RtaTigMobileGaming::TrafficModelStage stage)
{
    const auto now = Simulator::Now();
    NS_LOG_FUNCTION(this << packet << packet->GetSize() << now << stage);
    m_sent.push_back({packet->GetSize(), now, stage});
}

void
RtaTigMobileGamingTestCase::ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_FUNCTION(this << p << addr << p->GetSize());
    m_received += p->GetSize();
}

void
RtaTigMobileGamingTestCase::DoSetup()
{
    NS_LOG_FUNCTION(this);
    // LogComponentEnable("RtaTigMobileGamingTest", LOG_LEVEL_ALL);
    // LogComponentEnable("RtaTigMobileGaming", LOG_LEVEL_ALL);
    // LogComponentEnable("PacketSink", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(6);

    const auto simulationTime = Seconds(60);
    uint16_t port = 90;

    auto sender = CreateObject<Node>();
    auto receiver = CreateObject<Node>();

    NodeContainer nodes;
    nodes.Add(sender);
    nodes.Add(receiver);

    SimpleNetDeviceHelper simpleHelper;
    auto devices = simpleHelper.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper ipv4Helper;
    ipv4Helper.SetBase("10.11.12.0", "255.255.255.0");
    auto interfaces = ipv4Helper.Assign(devices);

    ApplicationHelper sourceHelper(RtaTigMobileGaming::GetTypeId());
    auto remoteAddress = InetSocketAddress(interfaces.GetAddress(1), port);
    sourceHelper.SetAttribute("Remote", AddressValue(remoteAddress));

    auto ips = CreateObjectWithAttributes<UniformRandomVariable>(
        "Min",
        DoubleValue(m_params.minInitialPacketSize),
        "Max",
        DoubleValue(m_params.maxInitialPacketSize));
    sourceHelper.SetAttribute("CustomInitialPacketSize", PointerValue(ips));

    auto eps =
        CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                          DoubleValue(m_params.minEndPacketSize),
                                                          "Max",
                                                          DoubleValue(m_params.maxEndPacketSize));
    sourceHelper.SetAttribute("CustomEndPacketSize", PointerValue(eps));

    auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
        "Location",
        DoubleValue(m_params.packetSizeLevLocation),
        "Scale",
        DoubleValue(m_params.packetSizeLevScale));
    sourceHelper.SetAttribute("CustomPacketSizeLev", PointerValue(psl));

    auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
        "Location",
        DoubleValue(m_params.packetArrivalLevLocation.GetMicroSeconds()),
        "Scale",
        DoubleValue(m_params.packetArrivalLevScale.GetMicroSeconds()));
    sourceHelper.SetAttribute("CustomPacketArrivalLev", PointerValue(pal));

    auto sourceApp = sourceHelper.Install(sender);
    const auto startAppTime = Seconds(1.0);
    sourceApp.Start(startAppTime);
    sourceApp.Stop(startAppTime + simulationTime);

    PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApp = sinkHelper.Install(receiver);
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(2.0) + simulationTime);

    int64_t streamNumber = 100;
    sourceHelper.AssignStreams(nodes, streamNumber);

    Config::ConnectWithoutContext(
        "/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::RtaTigMobileGaming/TxWithStage",
        MakeCallback(&RtaTigMobileGamingTestCase::PacketTx, this));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&RtaTigMobileGamingTestCase::ReceiveRx, this));
}

void
RtaTigMobileGamingTestCase::DoRun()
{
    Simulator::Run();
    Simulator::Destroy();

    const auto totalTx =
        std::accumulate(m_sent.cbegin(), m_sent.cend(), 0ULL, [](auto sum, const auto& tx) {
            return sum + tx.length;
        });
    NS_TEST_ASSERT_MSG_EQ(totalTx, m_received, "Did not receive all transmitted gaming packets");

    NS_TEST_ASSERT_MSG_EQ(m_sent.cbegin()->stage,
                          RtaTigMobileGaming::TrafficModelStage::INITIAL,
                          "First received packet is not an initial packet");
    NS_TEST_ASSERT_MSG_EQ(m_sent.crbegin()->stage,
                          RtaTigMobileGaming::TrafficModelStage::ENDING,
                          "Last received packet is not an ending packet");
    const auto allGamingPackets =
        std::all_of(m_sent.cbegin() + 1, m_sent.cend() - 1, [](const auto& tx) {
            return tx.stage == RtaTigMobileGaming::TrafficModelStage::GAMING;
        });
    NS_TEST_ASSERT_MSG_EQ(allGamingPackets, true, "Incorrectly reported stage during gaming stage");

    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_sent.cbegin()->length,
                                m_params.minInitialPacketSize,
                                "Size of initial packet is lower than expected");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(m_sent.cbegin()->length,
                                m_params.maxInitialPacketSize,
                                "Size of initial packet is higher than expected");

    NS_TEST_ASSERT_MSG_GT_OR_EQ(m_sent.crbegin()->length,
                                m_params.minEndPacketSize,
                                "Size of end packet is lower than expected");
    NS_TEST_ASSERT_MSG_LT_OR_EQ(m_sent.crbegin()->length,
                                m_params.maxEndPacketSize,
                                "Size of end packet is higher than expected");

    const auto totalGamingTx =
        std::accumulate(m_sent.cbegin() + 1, m_sent.cend() - 1, 0.0, [](auto sum, const auto& tx) {
            return sum + tx.length;
        });
    const auto averageGamingPacketSize = totalGamingTx / (m_sent.size() - 2);
    const auto expectedAverageGamingPacketSize =
        LargestExtremeValueRandomVariable::GetMean(m_params.packetSizeLevLocation,
                                                   m_params.packetSizeLevScale);
    NS_TEST_EXPECT_MSG_EQ_TOL(averageGamingPacketSize,
                              expectedAverageGamingPacketSize,
                              0.5,
                              "Unexpected average gaming packet size");

    std::vector<Time> packetArrivals;
    std::transform(m_sent.cbegin(),
                   m_sent.cend() - 1,
                   m_sent.cbegin() + 1,
                   std::back_inserter(packetArrivals),
                   [](const auto& lhs, const auto& rhs) { return (rhs.tstamp - lhs.tstamp); });
    const auto totalPacketArrivals =
        std::accumulate(packetArrivals.cbegin(),
                        packetArrivals.cend(),
                        Time(),
                        [](auto sum, const auto t) { return sum + t; });
    const auto averagePacketArrivalUs =
        static_cast<double>(totalPacketArrivals.GetMicroSeconds()) / packetArrivals.size();
    const auto expectedAveragePacketArrivalUs = LargestExtremeValueRandomVariable::GetMean(
        m_params.packetArrivalLevLocation.GetMicroSeconds(),
        m_params.packetArrivalLevScale.GetMicroSeconds());
    NS_TEST_EXPECT_MSG_EQ_TOL(averagePacketArrivalUs,
                              expectedAveragePacketArrivalUs,
                              0.01 * expectedAveragePacketArrivalUs,
                              "Unexpected average packet arrival");
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * @brief RtaTigMobileGaming TestSuite
 */
class RtaTigMobileGamingTestSuite : public TestSuite
{
  public:
    RtaTigMobileGamingTestSuite();
};

RtaTigMobileGamingTestSuite::RtaTigMobileGamingTestSuite()
    : TestSuite("applications-rta-tig-mobile-gaming", Type::UNIT)
{
    AddTestCase(new RtaTigMobileGamingTestCase(
                    "Check real-time mobile gaming traffic for DL status-sync",
                    {0, 20, 500, 600, 50, 11.0, MilliSeconds(13), MicroSeconds(3700)}),
                TestCase::Duration::QUICK);
    AddTestCase(new RtaTigMobileGamingTestCase(
                    "Check real-time mobile gaming traffic for UL status-sync",
                    {0, 20, 400, 550, 38, 3.7, MilliSeconds(15), MicroSeconds(5700)}),
                TestCase::Duration::QUICK);
    AddTestCase(new RtaTigMobileGamingTestCase(
                    "Check real-time mobile gaming traffic for DL lockstep",
                    {0, 80, 1400, 1500, 210, 35.0, MilliSeconds(28), MicroSeconds(4200)}),
                TestCase::Duration::QUICK);
    AddTestCase(new RtaTigMobileGamingTestCase(
                    "Check real-time mobile gaming traffic for UL lockstep",
                    {0, 80, 500, 600, 92, 38.0, MilliSeconds(22), MicroSeconds(3400)}),
                TestCase::Duration::QUICK);
}

static RtaTigMobileGamingTestSuite
    g_RtaTigMobileGamingTestSuite; //!< Static variable for test initialization
