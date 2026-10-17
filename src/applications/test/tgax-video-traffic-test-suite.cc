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
#include "ns3/enum.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-list-routing-helper.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/string.h"
#include "ns3/test.h"
#include "ns3/tgax-video-traffic.h"
#include "ns3/traced-callback.h"

#include <numeric>
#include <optional>
#include <string>
#include <vector>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TgaxVideoTrafficTest");

namespace
{

/**
 * Get the node ID from the context string.
 *
 * @param context the context string
 * @return the corresponding node ID
 */
uint32_t
ConvertContextToNodeId(const std::string& context)
{
    auto sub = context.substr(10);
    auto pos = sub.find("/Device");
    return std::stoi(sub.substr(0, pos));
}

/**
 * Get the string from the traffic model enum.
 *
 * @param model the traffic model enum
 * @return the corresponding string
 */
std::string
ModelToString(TgaxVideoTraffic::TrafficModelClassIdentifier model)
{
    switch (model)
    {
    case TgaxVideoTraffic::TrafficModelClassIdentifier::CUSTOM:
        return "Custom";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_1:
        return "BV1";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_2:
        return "BV2";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_3:
        return "BV3";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_4:
        return "BV4";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_5:
        return "BV5";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_6:
        return "BV6";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_1:
        return "MC1";
    case TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_2:
        return "MC2";
    default:
        return "";
    }
}

/**
 * Get the expected average frame size.
 *
 * @param frameSizeBytesScale the scale of the Weibull distribution used to generate random frame
 * sizes
 * @param frameSizeBytesShape the shape of the Weibull distribution used to generate random frame
 * sizes
 * @return the expected average frame size
 */
uint32_t
GetAverageframeSize(double frameSizeBytesScale, double frameSizeBytesShape)
{
    auto weibull = CreateObject<WeibullRandomVariable>();
    return static_cast<uint32_t>(
        WeibullRandomVariable::GetMean(frameSizeBytesScale, frameSizeBytesShape));
}

/**
 * Get whether a model applies to multicast video traffic.
 *
 * @param model the traffic model
 * @return true if the model is for multicast, false otherwise
 */
bool
IsMulticast(TgaxVideoTraffic::TrafficModelClassIdentifier model)
{
    return ((model == TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_1) ||
            (model == TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_2));
}

const auto simulationTime{Seconds(20)}; //!< The simulation time

} // namespace

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * TGax Video traffic test, check for the expected inter frame interval, bit rate and packets inter
 * arrivals.
 */
class TgaxVideoTrafficTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param model the traffic model to use for the test
     * @param protocol the protocol to use for the test (Udp or Tcp)
     * @param expectedBitRate the expected bit rate in Mbps at the end of the test
     * @param parameters optional traffic model parameters (used for custom traffic model)
     * @param txBufferSizeLimit optional limit for the TX buffer size of the TCP socket
     */
    TgaxVideoTrafficTestCase(
        TgaxVideoTraffic::TrafficModelClassIdentifier model,
        const std::string& protocol,
        double expectedBitRate,
        std::optional<TgaxVideoTraffic::TrafficModelParameters> parameters = {},
        std::optional<uint32_t> txBufferSizeLimit = {});

  private:
    void DoSetup() override;
    void DoRun() override;

    /**
     * Record a packets successfully sent
     * @param packet the transmitted packet
     * @param latency the latency applied to the packet
     */
    void PacketSent(Ptr<const Packet> packet, Time latency);

    /**
     * Record video frame generated
     * @param size the size of the generated frame
     */
    void FrameGenerated(uint32_t size);

    /**
     * Record a packet successfully received
     * @param context the context
     * @param p the packet
     * @param addr the sender's address
     */
    void ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr);

    TgaxVideoTraffic::TrafficModelClassIdentifier
        m_model;            //!< Selected buffered video traffic model
    std::string m_protocol; //!< Udp or Tcp protocol
    std::optional<TgaxVideoTraffic::TrafficModelParameters>
        m_parameters; //!< Optional traffic model parameters (used for custom traffic model)
    std::optional<uint32_t>
        m_txBufferSizeLimit; //!< Optional limit for the TX buffer size of the TCP socket

    double m_expectedBitRate; //!< Expected video bit rate

    uint64_t m_sent{0};                  //!< number of bytes sent
    std::vector<uint64_t> m_received;    //!< number of bytes received per receiver
    std::vector<Time> m_generatedFrames; //!< Store time at which each frame has been generated
    std::vector<Time> m_latencies;       //!< Store latency applied to each transmitted packet
};

TgaxVideoTrafficTestCase::TgaxVideoTrafficTestCase(
    TgaxVideoTraffic::TrafficModelClassIdentifier model,
    const std::string& protocol,
    double expectedBitRate,
    std::optional<TgaxVideoTraffic::TrafficModelParameters> parameters,
    std::optional<uint32_t> txBufferSizeLimit)
    : TestCase("Check video traffic with model " + ModelToString(model) + " and protocol " +
               protocol),
      m_model(model),
      m_protocol(protocol),
      m_parameters(parameters),
      m_txBufferSizeLimit(txBufferSizeLimit),
      m_expectedBitRate(expectedBitRate),
      m_received(IsMulticast(m_model) ? 2 : 1, 0)
{
}

void
TgaxVideoTrafficTestCase::PacketSent(Ptr<const Packet> packet, Time latency)
{
    NS_LOG_FUNCTION(this << packet << packet->GetSize() << latency);
    m_sent += packet->GetSize();
    m_latencies.push_back(latency);
}

void
TgaxVideoTrafficTestCase::FrameGenerated(uint32_t size)
{
    NS_LOG_FUNCTION(this << size);
    m_generatedFrames.push_back(Simulator::Now());
}

void
TgaxVideoTrafficTestCase::ReceiveRx(std::string context, Ptr<const Packet> p, const Address& addr)
{
    const auto receiver = ConvertContextToNodeId(context) - 1;
    NS_LOG_FUNCTION(this << receiver << p << addr << p->GetSize());
    m_received.at(receiver) += p->GetSize();
}

void
TgaxVideoTrafficTestCase::DoSetup()
{
    NS_LOG_FUNCTION(this);

    // LogComponentEnable("TgaxVideoTrafficTest", LOG_LEVEL_ALL);
    // LogComponentEnable("TgaxVideoTraffic", LOG_LEVEL_ALL);
    // LogComponentEnable("PacketSink", LOG_LEVEL_ALL);

    RngSeedManager::SetSeed(6);
    RngSeedManager::SetRun(8);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1500));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(1));
    if (m_txBufferSizeLimit)
    {
        Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(*m_txBufferSizeLimit));
    }

    uint16_t port = 90;
    const std::string multicastAddress = "239.192.100.1";

    auto sender = CreateObject<Node>();
    NodeContainer receivers(m_received.size());

    NodeContainer nodes;
    nodes.Add(sender);
    nodes.Add(receivers);

    SimpleNetDeviceHelper simpleHelper;
    auto devices = simpleHelper.Install(nodes);

    Ipv4StaticRoutingHelper staticRouting;
    InternetStackHelper internet;
    if (IsMulticast(m_model))
    {
        Ipv4ListRoutingHelper listRouting;
        listRouting.Add(staticRouting, 0);
        internet.SetRoutingHelper(listRouting);
    }
    internet.Install(nodes);

    Ipv4AddressHelper ipv4Helper;
    ipv4Helper.SetBase("10.11.12.0", "255.255.255.0");
    auto interfaces = ipv4Helper.Assign(devices);

    if (IsMulticast(m_model))
    {
        auto ipv4 = sender->GetObject<Ipv4>();
        auto routing = staticRouting.GetStaticRouting(ipv4);
        routing->AddHostRouteTo(multicastAddress.c_str(),
                                ipv4->GetInterfaceForDevice(sender->GetDevice(0)),
                                0);
    }

    ApplicationHelper sourceHelper(TgaxVideoTraffic::GetTypeId());
    auto remoteAddress = InetSocketAddress(IsMulticast(m_model) ? multicastAddress.c_str()
                                                                : interfaces.GetAddress(1),
                                           port);
    sourceHelper.SetAttribute("Remote", AddressValue(remoteAddress));
    const std::string protocol = "ns3::" + m_protocol + "SocketFactory";
    sourceHelper.SetAttribute("Protocol", StringValue(protocol));
    sourceHelper.SetAttribute("TrafficModelClassIdentifier", EnumValue(m_model));
    sourceHelper.SetAttribute("LatencyScale", DoubleValue(60.227));
    if (m_parameters)
    {
        sourceHelper.SetAttribute("CustomVideoBitRate", DataRateValue(m_parameters->bitRate));
        sourceHelper.SetAttribute("CustomFrameSizeScale",
                                  DoubleValue(m_parameters->frameSizeBytesScale));
        sourceHelper.SetAttribute("CustomFrameSizeShape",
                                  DoubleValue(m_parameters->frameSizeBytesShape));
    }
    auto sourceApp = sourceHelper.Install(sender);
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(1.0) + simulationTime);

    PacketSinkHelper sinkHelper(protocol, InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApps = sinkHelper.Install(receivers);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(2.0) + simulationTime);

    int64_t streamNumber = 55;
    sourceHelper.AssignStreams(nodes, streamNumber);

    Config::ConnectWithoutContext(
        "/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::TgaxVideoTraffic/TxWithLatency",
        MakeCallback(&TgaxVideoTrafficTestCase::PacketSent, this));

    Config::ConnectWithoutContext(
        "/NodeList/*/$ns3::Node/ApplicationList/*/$ns3::TgaxVideoTraffic/VideoFrameGenerated",
        MakeCallback(&TgaxVideoTrafficTestCase::FrameGenerated, this));

    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&TgaxVideoTrafficTestCase::ReceiveRx, this));
}

void
TgaxVideoTrafficTestCase::DoRun()
{
    NS_LOG_FUNCTION(this);

    Simulator::Run();
    Simulator::Destroy();

    for (std::size_t i = 0; i < m_received.size(); ++i)
    {
        NS_TEST_ASSERT_MSG_EQ(m_sent,
                              m_received.at(i),
                              "Did not receive all transmitted video frames for receiver " << i);
        const auto measuredBitrate =
            static_cast<double>(m_received.at(i) * 8) / simulationTime.GetMicroSeconds();
        NS_TEST_EXPECT_MSG_EQ_TOL(measuredBitrate,
                                  m_expectedBitRate,
                                  0.05 * m_expectedBitRate,
                                  "Unexpected video bit rate " << measuredBitrate
                                                               << " for receiver " << i);
    }

    const auto averageFrameSize =
        (m_parameters) ? GetAverageframeSize(m_parameters->frameSizeBytesScale,
                                             m_parameters->frameSizeBytesShape)
                       : GetAverageframeSize(
                             TgaxVideoTraffic::m_trafficModels.at(m_model).frameSizeBytesScale,
                             TgaxVideoTraffic::m_trafficModels.at(m_model).frameSizeBytesShape);
    const auto expectedInterFrameDuration =
        static_cast<double>(averageFrameSize * 8) / m_expectedBitRate;
    std::vector<Time> interFrameDurations;
    std::transform(m_generatedFrames.cbegin(),
                   m_generatedFrames.cend() - 1,
                   m_generatedFrames.cbegin() + 1,
                   std::back_inserter(interFrameDurations),
                   [](const auto& lhs, const auto& rhs) { return (rhs - lhs); });
    const auto totalInterFrameDurations =
        std::accumulate(interFrameDurations.cbegin(),
                        interFrameDurations.cend(),
                        Time(),
                        [](auto sum, const auto t) { return sum + t; });
    const auto interFrameDuration = totalInterFrameDurations / interFrameDurations.size();
    NS_TEST_EXPECT_MSG_EQ_TOL(interFrameDuration.GetMicroSeconds(),
                              expectedInterFrameDuration,
                              1,
                              "Unexpected frame interval");

    const auto totalLatency = std::accumulate(m_latencies.cbegin(),
                                              m_latencies.cend(),
                                              Time(),
                                              [](auto sum, auto latency) { return sum + latency; });
    const auto avgLatency = totalLatency / m_latencies.size();
    NS_TEST_EXPECT_MSG_EQ_TOL(avgLatency.GetMicroSeconds(),
                              14834,
                              200,
                              "Unexpected average networking latency");
}

/**
 * @ingroup applications-test
 * @ingroup tests
 *
 * @brief TgaxVideoTraffic TestSuite
 */
class TgaxVideoTrafficTestSuite : public TestSuite
{
  public:
    TgaxVideoTrafficTestSuite();
};

TgaxVideoTrafficTestSuite::TgaxVideoTrafficTestSuite()
    : TestSuite("applications-tgax-video-traffic", Type::UNIT)
{
    for (const auto& protocol : {"Tcp", "Udp"})
    {
        AddTestCase(new TgaxVideoTrafficTestCase(
                        TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_1,
                        protocol,
                        2.0),
                    TestCase::Duration::QUICK);
        AddTestCase(new TgaxVideoTrafficTestCase(
                        TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_2,
                        protocol,
                        4.0),
                    TestCase::Duration::QUICK);
        AddTestCase(new TgaxVideoTrafficTestCase(
                        TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_3,
                        protocol,
                        6.0),
                    TestCase::Duration::QUICK);
        AddTestCase(new TgaxVideoTrafficTestCase(
                        TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_4,
                        protocol,
                        8.0),
                    TestCase::Duration::QUICK);
        AddTestCase(new TgaxVideoTrafficTestCase(
                        TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_5,
                        protocol,
                        10.0),
                    TestCase::Duration::QUICK);
        AddTestCase(new TgaxVideoTrafficTestCase(
                        TgaxVideoTraffic::TrafficModelClassIdentifier::BUFFERED_VIDEO_6,
                        protocol,
                        15.6),
                    TestCase::Duration::QUICK);
    }
    AddTestCase(new TgaxVideoTrafficTestCase(
                    TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_1,
                    "Udp",
                    3.0),
                TestCase::Duration::QUICK);
    AddTestCase(new TgaxVideoTrafficTestCase(
                    TgaxVideoTraffic::TrafficModelClassIdentifier::MULTICAST_VIDEO_2,
                    "Udp",
                    6.0),
                TestCase::Duration::QUICK);
    AddTestCase(new TgaxVideoTrafficTestCase(TgaxVideoTraffic::TrafficModelClassIdentifier::CUSTOM,
                                             "Udp",
                                             5.0,
                                             {{DataRate("5Mbps"), 5000, 0.5}}),
                TestCase::Duration::QUICK);
    AddTestCase(new TgaxVideoTrafficTestCase(TgaxVideoTraffic::TrafficModelClassIdentifier::CUSTOM,
                                             "Tcp",
                                             20.0,
                                             {{DataRate("20Mbps"), 15000, 1.0}}),
                TestCase::Duration::QUICK);
    AddTestCase(new TgaxVideoTrafficTestCase(TgaxVideoTraffic::TrafficModelClassIdentifier::CUSTOM,
                                             "Tcp",
                                             20.0,
                                             {{DataRate("20Mbps"), 15000, 1.0}},
                                             1500),
                TestCase::Duration::QUICK);
}

static TgaxVideoTrafficTestSuite
    g_TgaxVideoTrafficTestSuite; //!< Static variable for test initialization
