/*
 * Copyright (c) 2025 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: N Nagabhushanam    <thechosentwins2005@gmail.com>
 *          Namburi Yaswanth   <yaswanthnamburi1010@gmail.com>
 *          Vishruth S Kumar   <vishruthskumar@gmail.com>
 *          Yashas             <yashas80dj@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 */

// This program simulates the following topology:
//
//           1000 Mbps           10Mbps          1000 Mbps
//  Sender -------------- R1 -------------- R2 -------------- Receiver
//              5ms               10ms               5ms
//
// This topology is the same as the topology present in tcp-bbr-example.
// The link between R1 and R2 is a bottleneck link with 10 Mbps. All other
// links are 1000 Mbps.
//
// This program runs by default for 100 seconds and creates a new directory
// called 'ecn-results' in the ns-3 root directory. The program creates one
// sub-directory called 'pcap' in 'ecn-results' directory (if pcap generation
// is enabled) and three .dat files.
//
// (1) 'pcap' sub-directory contains six PCAP files:
//     * ecn-0-0.pcap for the interface on Sender
//     * ecn-1-0.pcap for the interface on Receiver
//     * ecn-2-0.pcap for the first interface on R1
//     * ecn-2-1.pcap for the second interface on R1
//     * ecn-3-0.pcap for the first interface on R2
//     * ecn-3-1.pcap for the second interface on R2
// (2) cwnd.dat file contains congestion window trace for the sender node
// (3) throughput.dat file contains sender side throughput trace (throughput is in Mbit/s)
// (4) queueSize.dat file contains queue length trace from the bottleneck link
//
// ABE (Alternative Backoff with ECN) modifies the traditional TCP response to
// ECN (Explicit Congestion Notification) markings. While ECN signals
// congestion before queue overflows, standard TCP still reduces its congestion
// window aggressively (by 30% in TcpCubic) upon receiving these marks.
//
// ABE improves upon this by backing off less aggressively when ECN marks are
// received, typically reducing the window (by 15% in TcpCubic instead). This results in
// better throughput and faster recovery in high-bandwidth, ECN-enabled networks.

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include <filesystem>

using namespace ns3;
using namespace ns3::SystemPath;

std::string dir;          //!< The directory where the output files will be stored
std::ofstream throughput; //!< The stream where the throughput will be stored
std::ofstream queueSize;  //!< The stream where the queue size will be stored

uint32_t prev = 0; //!< Previous transmitted bytes
Time prevTime;     //!< Previous time for throughput calculation

/**
 * Traces the throughput of the flow monitor.
 * @param monitor The flow monitor to trace.
 */
static void
TraceThroughput(Ptr<FlowMonitor> monitor)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    if (!stats.empty())
    {
        auto itr = stats.begin();
        Time curTime = Now();

        // Convert (curTime - prevTime) to microseconds so that throughput is in bits per
        // microsecond (which is equivalent to Mbps)
        throughput << curTime.GetSeconds() << "s "
                   << 8 * (itr->second.txBytes - prev) / ((curTime - prevTime).ToDouble(Time::US))
                   << " Mbps" << std::endl;
        prevTime = curTime;
        prev = itr->second.txBytes;
    }
    Simulator::Schedule(Seconds(0.2), &TraceThroughput, monitor);
}

/**
 * Checks the queue size of the queue discipline.
 * @param qd The queue discipline to check.
 */
void
CheckQueueSize(Ptr<QueueDisc> qd)
{
    uint32_t qsize = qd->GetCurrentSize().GetValue();
    Simulator::Schedule(Seconds(0.2), &CheckQueueSize, qd);
    queueSize << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
}

/**
 * Writes the cWnd to the output stream.
 * @param stream The output stream to write to.
 * @param oldval The old congestion window value.
 * @param newval The new congestion window value.
 */
static void
CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / 1448.0 << std::endl;
}

/**
 * Traces the congestion window of the socket.
 * @param nodeId The node ID of the socket.
 * @param socketId The socket ID of the socket.
 */
void
TraceCwnd(uint32_t nodeId, uint32_t socketId)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(dir + "/cwnd.dat");
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/CongestionWindow",
                                  MakeBoundCallback(&CwndTracer, stream));
}

int
main(int argc, char* argv[])
{
    // Naming the output directory using local system time
    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, sizeof(buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
    std::string currentTime(buffer);

    std::string tcpTypeId = "TcpCubic";
    uint32_t delAckCount = 2;
    bool bql = true;
    bool enablePcap = false;
    bool useEcn = true;
    bool useAbe = false;
    Time stopTime = Seconds(100);

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcpTypeId",
                 "Transport protocol to use: TcpNewReno,TcpCubic, TcpLinuxReno",
                 tcpTypeId);
    cmd.AddValue("delAckCount", "Delayed ACK count", delAckCount);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);
    cmd.AddValue("useEcn", "Enable/Disable ECN", useEcn);
    cmd.AddValue("useAbe", "Enable/Disable ABE", useAbe);
    cmd.AddValue("stopTime",
                 "Stop time for applications / simulation time will be stopTime + 1",
                 stopTime);
    cmd.Parse(argc, argv);

    if (tcpTypeId.find("ns3::") == std::string::npos)
    {
        tcpTypeId = "ns3::" + tcpTypeId;
    }

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(tcpTypeId));

    // The maximum send buffer size is set to 4194304 bytes (4MB) and the
    // maximum receive buffer size is set to 6291456 bytes (6MB) in the Linux
    // kernel. The same buffer sizes are used as default in this example.
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));

    // AQM
    std::string queueDisc = "ns3::CoDelQueueDisc";
    std::string bottleNeckLinkBw = "10Mbps";
    std::string bottleNeckLinkDelay = "10ms";

    // CoDel parameters
    Config::SetDefault("ns3::CoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("1000p")));
    Config::SetDefault("ns3::CoDelQueueDisc::Interval", TimeValue(MilliSeconds(100)));
    Config::SetDefault("ns3::CoDelQueueDisc::Target", TimeValue(MilliSeconds(5)));
    Config::SetDefault("ns3::CoDelQueueDisc::UseEcn", BooleanValue(true));

    if (useAbe)
    {
        Config::SetDefault("ns3::TcpSocketBase::UseAbe", BooleanValue(true));
    }
    if (useEcn)
    {
        Config::SetDefault("ns3::TcpSocketBase::UseEcn", EnumValue(TcpSocketState::On));
    }

    NodeContainer sender;
    NodeContainer receiver;
    NodeContainer routers;
    sender.Create(1);
    receiver.Create(1);
    routers.Create(2);

    // Create the point-to-point link helpers
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("10ms"));

    PointToPointHelper edgeLink;
    edgeLink.SetDeviceAttribute("DataRate", StringValue("1000Mbps"));
    edgeLink.SetChannelAttribute("Delay", StringValue("5ms"));

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
    tch.SetRootQueueDisc(queueDisc);

    if (bql)
    {
        tch.SetQueueLimits("ns3::DynamicQueueLimits", "HoldTime", StringValue("1000ms"));
    }
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
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceCwnd, 0, 0);
    sourceApps.Stop(stopTime);

    // Install application on the receiver
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(receiver.Get(0));
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(stopTime);
    // Create a new directory to store the output of the program
    dir = "ecn-results/" + currentTime + "/";
    MakeDirectories(dir);

    // The plotting scripts are provided in the following repository, if needed:
    // https://github.com/mohittahiliani/BBR-Validation/
    //
    // Download 'PlotScripts' directory (which is inside ns-3 scripts directory)
    // from the link given above and place it in the ns-3 root directory.
    // Uncomment the following three lines to copy plot scripts for
    // Congestion Window, sender side throughput and queue occupancy on the
    // bottleneck link into the output directory.
    //
    // std::filesystem::copy("PlotScripts/gnuplotScriptCwnd", dir);
    // std::filesystem::copy("PlotScripts/gnuplotScriptThroughput", dir);
    // std::filesystem::copy("PlotScripts/gnuplotScriptQueueSize", dir);

    // Trace the queue occupancy on the second interface of R1
    tch.Uninstall(routers.Get(0)->GetDevice(1));
    QueueDiscContainer qd;
    qd = tch.Install(routers.Get(0)->GetDevice(1));
    Simulator::ScheduleNow(&CheckQueueSize, qd.Get(0));

    // Generate PCAP traces if it is enabled
    if (enablePcap)
    {
        MakeDirectories(dir + "pcap/");
        bottleneckLink.EnablePcapAll(dir + "/pcap/ecn", true);
    }

    // Open files for writing throughput traces and queue size
    throughput.open(dir + "/throughput.dat", std::ios::out);
    queueSize.open(dir + "/queueSize.dat", std::ios::out);

    NS_ASSERT_MSG(throughput.is_open(), "Throughput file was not opened correctly");
    NS_ASSERT_MSG(queueSize.is_open(), "Queue size file was not opened correctly");

    // Check for dropped packets using Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Simulator::ScheduleNow(&TraceThroughput, monitor);

    Simulator::Stop(stopTime + TimeStep(1));
    Simulator::Run();
    Simulator::Destroy();

    throughput.close();
    queueSize.close();

    return 0;
}
