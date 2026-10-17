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
#include "ns3/pointer.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tgax-virtual-desktop.h"
#include "ns3/traced-callback.h"

#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TgaxVirtualDesktopTest");

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * TGax VDI traffic test.
 *
 * The test consider traffic values for the model presented in IEEE 802.11-14/0571r12 - 11ax
 * Evaluation Methodology (Appendix 2 – Traffic model descriptions: Virtual Desktop Infrastructure
 * Traffic Model) for both downlink and uplink.
 *
 * The test generates traffic between two nodes and keeps track of generated TX packets (size,
 * and timestamp). The test verifies average sizes and inter arrivals of generated packets.
 */
class TgaxVirtualDesktopTestCase : public TestCase
{
  public:
    /// Information about VDI parameters
    struct VdiParams
    {
        Time meanPacketArrivalTime{
            NanoSeconds(60226900)}; //!< mean of the distribution used to generate packet arrival
        std::string parametersPacketSize{
            "41.0 3.2;1478.3 11.6"}; //!< parameters of the distribution used to generate the packet
                                     //!< sizes
    };

    /**
     * Constructor
     * @param name the name of the test to run
     * @param params the VDI parameters to use for the test
     */
    TgaxVirtualDesktopTestCase(const std::string& name, const VdiParams& params);

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Record a transmitted VDI packet
     * @param packet the transmitted packet
     */
    void PacketSent(Ptr<const Packet> packet);

    /**
     * Record a VDI packet successfully received
     * @param context the context
     * @param p the packet
     * @param addr the sender's address
     */
    void ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr);

    /// Information about a transmitted VDI packet
    struct TxInfo
    {
        uint32_t size{}; //!< size of the packet in bytes
        Time tstamp{};   //!< timestamp at which the packet is transmitted
    };

    std::vector<TxInfo> m_sent; //!< transmitted VDI packets
    uint64_t m_received{0};     //!< number of bytes received

    VdiParams m_params;  //!< VDI parameters
    Time m_startAppTime; //!< Time at which the application is started
};

TgaxVirtualDesktopTestCase::TgaxVirtualDesktopTestCase(const std::string& name,
                                                       const VdiParams& params)
    : TestCase{name},
      m_params{params},
      m_startAppTime{Seconds(1.0)}
{
}

void
TgaxVirtualDesktopTestCase::PacketSent(Ptr<const Packet> packet)
{
    NS_LOG_FUNCTION(this << packet << packet->GetSize());
    m_sent.push_back({packet->GetSize(), Simulator::Now()});
}

void
TgaxVirtualDesktopTestCase::ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr)
{
    NS_LOG_FUNCTION(this << p << addr << p->GetSize());
    m_received += p->GetSize();
}

void
TgaxVirtualDesktopTestCase::DoSetup()
{
    NS_LOG_FUNCTION(this);

    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    const auto simulationTime = Seconds(600);
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

    ApplicationHelper sourceHelper(TgaxVirtualDesktop::GetTypeId());
    auto remoteAddress = InetSocketAddress(interfaces.GetAddress(1), port);
    sourceHelper.SetAttribute("Remote", AddressValue(remoteAddress));
    const auto protocol = "ns3::TcpSocketFactory";
    sourceHelper.SetAttribute("Protocol", StringValue(protocol));

    auto ipa = CreateObjectWithAttributes<ExponentialRandomVariable>(
        "Mean",
        DoubleValue(m_params.meanPacketArrivalTime.GetNanoSeconds()));
    sourceHelper.SetAttribute("CustomInterPacketArrivals", PointerValue(ipa));

    sourceHelper.SetAttribute("CustomParametersPacketSize",
                              StringValue(m_params.parametersPacketSize));
    auto sourceApp = sourceHelper.Install(sender);
    sourceApp.Start(m_startAppTime);
    sourceApp.Stop(m_startAppTime + simulationTime);

    PacketSinkHelper sinkHelper(protocol, InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApp = sinkHelper.Install(receiver);
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(2.0) + simulationTime);

    int64_t streamNumber = 100;
    sourceHelper.AssignStreams(nodes, streamNumber);

    Config::ConnectWithoutContext(
        "/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::TgaxVirtualDesktop/Tx",
        MakeCallback(&TgaxVirtualDesktopTestCase::PacketSent, this));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&TgaxVirtualDesktopTestCase::ReceiveRx, this));
}

void
TgaxVirtualDesktopTestCase::DoRun()
{
    Simulator::Run();
    Simulator::Destroy();

    const auto totalTx =
        std::accumulate(m_sent.cbegin(), m_sent.cend(), 0ULL, [](auto sum, const auto& tx) {
            return sum + tx.size;
        });
    NS_TEST_ASSERT_MSG_EQ(totalTx, m_received, "Did not receive all transmitted VDI packets");

    const auto delayConnectionEstablished = MilliSeconds(18);
    NS_TEST_ASSERT_MSG_LT_OR_EQ(m_sent.cbegin()->tstamp - m_startAppTime -
                                    delayConnectionEstablished,
                                MilliSeconds(20),
                                "Initial packet arrival larger than upper bound");

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
    const auto averagePacketArrivalNs =
        static_cast<double>(totalPacketArrivals.GetNanoSeconds()) / packetArrivals.size();
    const auto expectedAveragePacketArrivalNs = m_params.meanPacketArrivalTime.GetNanoSeconds();
    NS_TEST_EXPECT_MSG_EQ_TOL(averagePacketArrivalNs,
                              expectedAveragePacketArrivalNs,
                              0.01 * expectedAveragePacketArrivalNs,
                              "Unexpected average packet arrival");

    const auto averagePacketSize = static_cast<double>(totalTx) / m_sent.size();
    const auto modalPos = m_params.parametersPacketSize.find(';');
    double expectedAveragePacketSize{};
    auto pos = m_params.parametersPacketSize.find(' ');
    const auto mu1 = m_params.parametersPacketSize.substr(0, pos);
    const auto mean1 = std::stod(mu1);
    if (modalPos == std::string::npos)
    {
        // single mode
        expectedAveragePacketSize = mean1;
    }
    else
    {
        // bimodal
        auto mode2 = m_params.parametersPacketSize.substr(modalPos + 1,
                                                          m_params.parametersPacketSize.size() - 1);
        pos = mode2.find(' ');
        const auto mu2 = mode2.substr(0, pos);
        const auto mean2 = std::stod(mu2);
        const auto bernoulliProb = 22.4 / 76.1;
        expectedAveragePacketSize = (mean1 * (1 - bernoulliProb)) + (mean2 * bernoulliProb);
    }

    NS_TEST_EXPECT_MSG_EQ_TOL(averagePacketSize,
                              expectedAveragePacketSize,
                              0.015 * expectedAveragePacketSize,
                              "Unexpected average packet size");
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * @brief TgaxVirtualDesktop TestSuite
 */
class TgaxVirtualDesktopTestSuite : public TestSuite
{
  public:
    TgaxVirtualDesktopTestSuite();
};

TgaxVirtualDesktopTestSuite::TgaxVirtualDesktopTestSuite()
    : TestSuite("applications-tgax-virtual-desktop", Type::UNIT)
{
    AddTestCase(new TgaxVirtualDesktopTestCase("DL VDI traffic (default)", {}),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TgaxVirtualDesktopTestCase("UL VDI traffic", {MicroSeconds(48287), "50.598 5.0753"}),
        TestCase::Duration::QUICK);
}

static TgaxVirtualDesktopTestSuite
    g_TgaxVirtualDesktopTestSuite; //!< Static variable for test initialization
