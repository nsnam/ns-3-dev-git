/*
 * Copyright (c) 2010 University of Washington (trace writing)
 * Copyright (c) 2018-20 NITK Surathkal (topology setup)
 * Copyright (c) 2024 Tom Henderson (test definition)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// Test suite based on tcp-bbr-example.cc modified topology setup:
//
//           1 Gbps            50/100Mbps         1 Gbps
//  Sender -------------- R1 -------------- R2 -------------- Receiver
//            5us               baseRtt/2          5us
//
// This is a test suite written initially to test the addition of
// TCP friendliness but can also be extended for other Cubic tests.  By
// changing some compile-time constants below, it can be used to also produce
// gnuplots and pcaps that demonstrate the behavior.  See the WRITE_PCAP and
// WRITE_GNUPLOT booleans below to enable this output.
//
// There are four cases configured presently.
// 1. A Cubic TCP on a 5 ms RTT bottleneck, with no fast convergence nor
//    TCP friendliness configured
// 2. A Cubic TCP on a 5 ms RTT bottleneck, with fast convergence but no
//    TCP friendliness configured.  This exhibits a different cwnd plot from 1.
// 3. A Cubic TCP on a 20 ms RTT bottleneck, with fast convergence but no
//    TCP friendliness configured.  There is a step change increase in
//    bottleneck link capacity but the TCP is slow to exploit it.
// 4. A Cubic TCP on a 20 ms RTT bottleneck, with fast convergence and
//    TCP friendliness configured.  There is a step change increase in
//    bottleneck link capacity and the TCP responds more quickly than in 3.

#include "ns3/abort.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/error-model.h"
#include "ns3/gnuplot.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/pcap-file.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/tcp-header.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/test.h"
#include "ns3/trace-helper.h"
#include "ns3/traffic-control-helper.h"
#include "ns3/uinteger.h"

#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ns3TcpCubicTest");

static constexpr bool WRITE_PCAP = false;    //!< Set to true to write out pcap.
static constexpr bool WRITE_GNUPLOT = false; //!< Set to true to write out gnuplot.

/**
 * @ingroup system-tests-tcp
 *
 * Add sample trace values to data structures
 * @param gnuplotTimeSeries Gnuplot data structure
 * @param timeSeries time series of cwnd changes
 * @param oldval old value of cwnd
 * @param newval new value of cwnd
 */
void
CubicCwndTracer(Gnuplot2dDataset* gnuplotTimeSeries,
                std::map<Time, double>* timeSeries,
                uint32_t oldval,
                uint32_t newval)
{
    // We store data in two structures because Gnuplot2DDataset data is not exportable
    NS_LOG_DEBUG("cwnd " << newval / 1448.0);
    gnuplotTimeSeries->Add(Simulator::Now().GetSeconds(), newval / 1448.0);
    timeSeries->insert_or_assign(Simulator::Now(), newval / 1448.0);
}

/**
 * @ingroup system-tests-tcp
 * Test Cubic response
 */
class Ns3TcpCubicTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param testCase testcase name
     * @param prefix filename prefix if writing output files
     * @param fastConvergence whether to enable fast convergence
     * @param tcpFriendliness whether to enable TCP friendliness
     * @param baseRtt base RTT to use for test case
     * @param capacityIncrease whether to trigger a sudden capacity increase
     */
    Ns3TcpCubicTestCase(std::string testCase,
                        std::string prefix,
                        bool fastConvergence,
                        bool tcpFriendliness,
                        Time baseRtt,
                        bool capacityIncrease);
    ~Ns3TcpCubicTestCase() override;

  private:
    void DoRun() override;

    /**
     * Connect TCP cwnd trace after socket is instantiated
     *
     * @param nodeId node ID to connect to
     * @param socketId socket ID to connect to
     */
    void ConnectCwndTrace(uint32_t nodeId, uint32_t socketId);

    /**
     * Increases the device bandwidth to 100 Mbps
     * @param device device to modify
     */
    void IncreaseBandwidth(Ptr<PointToPointNetDevice> device);

    /**
     * Check that time series values within a time range are within a value range.
     * @param start start of time range
     * @param end end of time range
     * @param lowerBound lower bound of acceptable values
     * @param upperBound upper bound of acceptable values
     * @return true if values are within range
     */
    bool CheckValues(Time start, Time end, double lowerBound, double upperBound);

    bool m_writeResults{WRITE_PCAP};     //!< Whether to write pcaps
    bool m_writeGnuplot{WRITE_GNUPLOT};  //!< Whether to write gnuplot files
    Gnuplot2dDataset m_cwndTimeSeries;   //!< cwnd time series
    std::map<Time, double> m_timeSeries; //!< time series to check
    std::string m_prefix;                //!< filename prefix if writing files
    bool m_fastConvergence;              //!< whether to enable fast convergence
    bool m_tcpFriendliness;              //!< whether to enable TCP friendliness
    Time m_baseRtt;                      //!< the base RTT to use
    bool m_capacityIncrease;             //!< whether to trigger a capacity increase
};

Ns3TcpCubicTestCase::Ns3TcpCubicTestCase(std::string testCase,
                                         std::string prefix,
                                         bool fastConvergence,
                                         bool tcpFriendliness,
                                         Time baseRtt,
                                         bool capacityIncrease)
    : TestCase(testCase),
      m_prefix(prefix),
      m_fastConvergence(fastConvergence),
      m_tcpFriendliness(tcpFriendliness),
      m_baseRtt(baseRtt),
      m_capacityIncrease(capacityIncrease)
{
}

Ns3TcpCubicTestCase::~Ns3TcpCubicTestCase()
{
}

void
Ns3TcpCubicTestCase::IncreaseBandwidth(Ptr<PointToPointNetDevice> device)
{
    device->SetDataRate(DataRate("100Mbps"));
}

void
Ns3TcpCubicTestCase::ConnectCwndTrace(uint32_t nodeId, uint32_t socketId)
{
    Config::ConnectWithoutContext(
        "/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/" +
            std::to_string(socketId) + "/CongestionWindow",
        MakeBoundCallback(&CubicCwndTracer, &m_cwndTimeSeries, &m_timeSeries));
}

bool
Ns3TcpCubicTestCase::CheckValues(Time start, Time end, double lowerBound, double upperBound)
{
    auto itStart = m_timeSeries.lower_bound(start);
    auto itEnd = m_timeSeries.upper_bound(end);
    for (auto it = itStart; it != itEnd; it++)
    {
        if (it->second < lowerBound || it->second > upperBound)
        {
            NS_LOG_DEBUG("Value " << it->second << " at time " << it->first.GetSeconds()
                                  << " outside range (" << lowerBound << "," << upperBound << ")");
            return false;
        }
    }
    return true;
}

void
Ns3TcpCubicTestCase::DoRun()
{
    NS_LOG_DEBUG("Running " << m_prefix);
    // The maximum bandwidth delay product is 20 ms (base RTT) times 100 Mbps
    // which works out to 250KB.  Setting the socket buffer sizes to four times
    // that value (1MB) should be more than enough.
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1000000));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1000000));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(2));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true));
    Config::SetDefault("ns3::TcpSocketState::PaceInitialWindow", BooleanValue(true));
    Config::SetDefault("ns3::TcpCubic::FastConvergence", BooleanValue(m_fastConvergence));
    Config::SetDefault("ns3::TcpCubic::TcpFriendliness", BooleanValue(m_tcpFriendliness));

    Time stopTime = Seconds(20);

    NodeContainer sender;
    NodeContainer receiver;
    NodeContainer routers;
    sender.Create(1);
    receiver.Create(1);
    routers.Create(2);

    // Create the point-to-point link helpers
    PointToPointHelper bottleneckLink;
    // With the DynamicQueueLimits setting below, the following statement
    // will minimize the buffering in the device and force most buffering
    // into the AQM
    bottleneckLink.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("2p"));
    if (m_capacityIncrease)
    {
        // Start at a lower capacity, and increase later
        bottleneckLink.SetDeviceAttribute("DataRate", StringValue("50Mbps"));
    }
    else
    {
        bottleneckLink.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    }
    bottleneckLink.SetChannelAttribute("Delay", TimeValue(m_baseRtt / 2));

    PointToPointHelper edgeLink;
    edgeLink.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    edgeLink.SetChannelAttribute("Delay", StringValue("5us"));

    // Create NetDevice containers
    NetDeviceContainer senderEdge = edgeLink.Install(sender.Get(0), routers.Get(0));
    NetDeviceContainer r1r2 = bottleneckLink.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer receiverEdge = edgeLink.Install(routers.Get(1), receiver.Get(0));

    // Install Stack
    InternetStackHelper internet;
    internet.Install(sender);
    internet.Install(receiver);
    internet.Install(routers);

    // Configure the root queue discipline
    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    tch.SetQueueLimits("ns3::DynamicQueueLimits");

    tch.Install(senderEdge);
    tch.Install(receiverEdge);

    // Assign IP addresses
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.0.0.0", "255.255.255.0");

    Ipv4InterfaceContainer i1i2 = ipv4.Assign(r1r2);
    ipv4.NewNetwork();
    Ipv4InterfaceContainer is1 = ipv4.Assign(senderEdge);
    ipv4.NewNetwork();
    Ipv4InterfaceContainer ir1 = ipv4.Assign(receiverEdge);

    // Populate routing tables
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Select sender side port
    uint16_t port = 50001;

    // Install application on the sender
    BulkSendHelper source("ns3::TcpSocketFactory", InetSocketAddress(ir1.GetAddress(1), port));
    source.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer sourceApps = source.Install(sender.Get(0));
    sourceApps.Start(Seconds(0.1));
    // Hook trace source after application starts
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1),
                        &Ns3TcpCubicTestCase::ConnectCwndTrace,
                        this,
                        0,
                        0);
    sourceApps.Stop(stopTime);

    // Install application on the receiver
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(receiver.Get(0));
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(stopTime);

    if (m_capacityIncrease)
    {
        // Double the capacity of the bottleneck link at 10 seconds
        Ptr<PointToPointNetDevice> device = r1r2.Get(0)->GetObject<PointToPointNetDevice>();
        Simulator::Schedule(Seconds(10), &Ns3TcpCubicTestCase::IncreaseBandwidth, this, device);
    }

    std::ostringstream oss;
    oss << m_prefix;
    if (m_writeResults)
    {
        edgeLink.EnablePcap(oss.str(), senderEdge.Get(0));
        edgeLink.EnablePcap(oss.str(), receiverEdge.Get(0));
        edgeLink.EnableAscii(oss.str(), senderEdge.Get(0));
        edgeLink.EnableAscii(oss.str(), receiverEdge.Get(0));
    }
    Simulator::Stop(Seconds(30));
    Simulator::Run();

    if (m_writeGnuplot)
    {
        std::ofstream outfile(oss.str() + ".plt");

        std::string capacityIncreaseString;
        if (m_capacityIncrease)
        {
            capacityIncreaseString = " capacity increase = 1";
        }
        Gnuplot gnuplot = Gnuplot(
            oss.str() + ".eps",
            "Cubic concave/convex response (" + std::to_string(m_baseRtt.GetMilliSeconds()) +
                " ms RTT, 1448 byte segs, 100 Mbps bottleneck)\\nFast convergence = " +
                std::to_string(m_fastConvergence) +
                " friendliness = " + std::to_string(m_tcpFriendliness) + capacityIncreaseString);
        gnuplot.SetTerminal("post eps color enhanced");
        gnuplot.SetLegend("Time (seconds)", "Cwnd (segments)");
        m_cwndTimeSeries.SetTitle("");
        gnuplot.AddDataset(m_cwndTimeSeries);
        gnuplot.GenerateOutput(outfile);
    }

    // Check that cwnd values are within tolerance of expected values
    // The expected values were determined by inspecting the plots generated
    if (m_prefix == "ns3-tcp-cubic-no-heuristic")
    {
        // Check overall min and max
        NS_TEST_ASSERT_MSG_EQ(CheckValues(Seconds(1), Seconds(19), 60, 98),
                              true,
                              "cwnd outside range");
        // Time just before a reduction does not have much variation
        NS_TEST_ASSERT_MSG_EQ(CheckValues(Seconds(9), Seconds(9.6), 84, 89),
                              true,
                              "cwnd outside range");
    }
    else if (m_prefix == "ns3-tcp-cubic-fast-conv")
    {
        // Check overall min and max
        NS_TEST_ASSERT_MSG_EQ(CheckValues(Seconds(1), Seconds(19), 60, 98),
                              true,
                              "cwnd outside range");
        // Initial convex region does not have much variation
        NS_TEST_ASSERT_MSG_EQ(CheckValues(Seconds(4), Seconds(6), 83, 88),
                              true,
                              "cwnd outside range");
    }
    else if (m_prefix == "ns3-tcp-cubic-no-friendly")
    {
        // Between time 11 and 15, cwnd should be fairly constant
        // because without TCP friendliness, Cubic does not respond quickly
        NS_TEST_ASSERT_MSG_EQ(CheckValues(Seconds(11), Seconds(15), 107, 123),
                              true,
                              "cwnd outside range");
        // After time 17.5, cwnd should have grown much higher
        NS_TEST_ASSERT_MSG_EQ(CheckValues(Seconds(17.5), Seconds(18.5), 169, 215),
                              true,
                              "cwnd outside range");
    }
    else if (m_prefix == "ns3-tcp-cubic-friendly")
    {
        // In contrast to previous case, cwnd should grow above 150 much sooner
        NS_TEST_ASSERT_MSG_EQ(CheckValues(Seconds(13), Seconds(15), 150, 210),
                              true,
                              "cwnd outside range");
    }

    Simulator::Destroy();
}

/**
 * @ingroup system-tests-tcp
 * TestSuite for module tcp-cubic
 */
class Ns3TcpCubicTestSuite : public TestSuite
{
  public:
    Ns3TcpCubicTestSuite();
};

Ns3TcpCubicTestSuite::Ns3TcpCubicTestSuite()
    : TestSuite("ns3-tcp-cubic", Type::UNIT)
{
    // Test Cubic with no fast convergence or TCP friendliness enabled
    // This results in a cwnd plot that has only the concave portion of
    // returning cwnd to Wmax
    AddTestCase(new Ns3TcpCubicTestCase("no heuristic",
                                        "ns3-tcp-cubic-no-heuristic",
                                        false,
                                        false,
                                        MilliSeconds(5),
                                        false),
                TestCase::Duration::QUICK);

    // Test Cubic with fast convergence but no TCP friendliness enabled
    // This results in a cwnd plot that has concave and convex regions, as
    // well as convex-only regions; also observed on Linux testbeds
    AddTestCase(new Ns3TcpCubicTestCase("fast convergence on",
                                        "ns3-tcp-cubic-fast-conv",
                                        true,
                                        false,
                                        MilliSeconds(5),
                                        false),
                TestCase::Duration::QUICK);

    // Test Cubic with fast convergence but no TCP friendliness enabled
    // with a higher RTT (20ms) and a step change in capacity at time 10s.
    // This results in a sluggish response by TCP Cubic to use the new capacity.
    AddTestCase(new Ns3TcpCubicTestCase("fast convergence on, Reno friendliness off",
                                        "ns3-tcp-cubic-no-friendly",
                                        true,
                                        false,
                                        MilliSeconds(20),
                                        true),
                TestCase::Duration::QUICK);

    // Test Cubic with fast convergence but with TCP friendliness enabled
    // with a higher RTT (20ms) and a step change in capacity at time 10s.
    // This results in a faster response by TCP Cubic to use the new capacity.
    AddTestCase(new Ns3TcpCubicTestCase("fast convergence on, Reno friendliness on",
                                        "ns3-tcp-cubic-friendly",
                                        true,
                                        true,
                                        MilliSeconds(20),
                                        true),
                TestCase::Duration::QUICK);
}

/**
 * Static variable for test initialization
 */
static Ns3TcpCubicTestSuite ns3TcpCubicTestSuite;
