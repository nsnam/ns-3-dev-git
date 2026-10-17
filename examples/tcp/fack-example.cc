/*
 * Copyright (c) 2018-20, 2025 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Aarti Nandagiri <aarti.nandagiri@gmail.com>
 *          Vivek Jain <jain.vivek.anand@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 * Modified for FACK by: Jayesh Akot <akotjayesh@gmail.com>
 */

/**
 * @file
 * @ingroup examples-tcp
 * @brief Example to verify TCP FACK performance based on Mathis & Mahdavi paper.
 *
 * The network topology used in this example is based on the Fig. 1 described in
 * Mathis, M., & Mahdavi, J. (1996, August).
 * Forward acknowledgement: Refining TCP congestion control.
 *
 * @code{.text}
 *            10 Mbps           1.5 Mbps          10 Mbps
 * Sender -------------- R1 -------------- R2 -------------- Receiver
 *              2ms               5ms               33ms
 * @endcode
 *
 * This program runs by default for 100 seconds and creates a new directory
 * called 'fack-results' in the ns-3 root directory. The program creates one
 * sub-directory called 'pcap' in 'fack-results' directory (if pcap generation
 * is enabled) and six .dat files.
 *
 * - 'pcap' sub-directory contains six PCAP files:
 *  - fack-0-0.pcap for the interface on Sender
 *  - fack-1-0.pcap for the interface on Receiver
 *  - fack-2-0.pcap for the first interface on R1
 *  - fack-2-1.pcap for the second interface on R1
 *  - fack-3-0.pcap for the first interface on R2
 *  - fack-3-1.pcap for the second interface on R2
 * - cwnd.dat file contains congestion window trace for the sender node
 * - throughput.dat file contains sender side throughput trace (throughput is in Mbit/s)
 * - queueSize.dat file contains queue length trace from the bottleneck link
 * - lostSegments.dat file contains packet lost trace at queue
 * - bytesInFlight.dat contains trace of inflight data(SND.NXT - SND.UNA).
 * - fackAwnd.dat contains trace of the FACK-specific inflight estimate.
 *
 * This example allows the user to quantitatively verify two key mechanisms
 * described in the Mathis & Mahdavi paper:
 * -# The preservation of the TCP Self-Clock during recovery (Section 4.2).
 * -# The "Overdamping" mechanism to drain queues after overshoot (Section 4.4).
 *
 * To observe these insights, you must run the simulation twice with different configurations:
 *
 * - **Step 1:** Run with FACK enabled (Default)
 * @code ./ns3 run "fack-example --fack=true" @endcode
 *
 * - **Step 2:** Run with FACK disabled (Standard Reno+SACK behavior)
 * @code ./ns3 run "fack-example --fack=false" @endcode
 *
 * - **Step 3:** Compare the output traces 'bytesInFlight.dat' and 'cwnd.dat' from both runs.
 *
 * **SPECIFIC OBSERVATIONS:**
 *
 * **A. Preservation of Self-Clock (Compare 'bytesInFlight.dat'):**
 * - WITHOUT FACK: When multiple packet losses occur (approx t=0.5s), the
 * inflight data drops significantly (often stalling near zero). This indicates
 * a loss of the ACK clock, matching the behavior of "Reno+SACK" described
 * in Section 4.2 and Figure 3 of the paper.
 * - WITH FACK: The inflight data remains elevated even during recovery.
 * This demonstrates that FACK successfully decouples congestion control
 * from data recovery, using SACK blocks to inject new data and maintain
 * the self-clock. This matches the behavior in Figure 4 of the paper.
 *
 * **B. Overdamping and Queue Draining (Compare 'cwnd.dat' vs 'bytesInFlight.dat'):**
 * - In the FACK run (between t=1.0s and t=2.0s), observe that 'cwnd' drops
 * to a low value (~10 packets) while 'bytesInFlight' remains high (~50 packets).
 * - This divergence is the "Overdamping" mechanism (Section 4.4). FACK detects
 * that the actual data in the network (inflight) exceeds the target window
 * (cwnd) due to the initial Slow-Start overshoot.
 * - Consequently, FACK inhibits transmission (visible as a throughput dip) to
 * allow the excess queue to drain.
 */
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;
using namespace ns3::SystemPath;

std::string g_dir;                      //!< Output directory
std::ofstream g_throughput;             //!< Throughput output stream
std::ofstream g_queueSize;              //!< Queue size output stream
std::ofstream g_individualLostSegments; //!< Lost segments output stream

uint32_t g_prev = 0;           //!< Previous throughput byte count
Time g_prevTime;               //!< Previous throughput measurement time
uint32_t g_segmentSize = 1448; //!< Segment size

/**
 * @brief Calculate and trace throughput
 * @param monitor The flow monitor
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

        g_throughput << curTime.GetSeconds() << "s "
                     << 8 * (itr->second.txBytes - g_prev) /
                            ((curTime - g_prevTime).ToDouble(Time::US))
                     << " Mbps" << std::endl;
        g_prevTime = curTime;
        g_prev = itr->second.txBytes;
    }
    Simulator::Schedule(Seconds(0.2), &TraceThroughput, monitor);
}

/**
 * @brief Check and trace the queue size
 * @param qd The queue disc
 */
void
CheckQueueSize(Ptr<QueueDisc> qd)
{
    uint32_t qsize = qd->GetCurrentSize().GetValue();
    Simulator::Schedule(Seconds(0.2), &CheckQueueSize, qd);
    g_queueSize << Simulator::Now().GetSeconds() << " " << qsize << std::endl;
}

/**
 * @brief Trace packets dropped at Queue
 * @param stream The output stream
 * @param item The queue disc item dropped
 */
static void
TracePacketDrop(Ptr<OutputStreamWrapper> stream, Ptr<const QueueDiscItem> item)
{
    Ptr<const Packet> p = item->GetPacket();

    TcpHeader tcpHeader;
    p->PeekHeader(tcpHeader);
    uint32_t seqNum = tcpHeader.GetSequenceNumber().GetValue();

    // The plots in the paper use segment index, not the raw sequence number.
    // We calculate it by dividing by the segment size.
    double segmentIndex = static_cast<double>(seqNum) / g_segmentSize;

    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << segmentIndex << std::endl;
}

/**
 * @brief Trace congestion window
 * @param stream The output stream
 * @param oldval Old value
 * @param newval New value
 */
static void
CwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << newval / g_segmentSize
                         << std::endl;
}

/**
 * @brief Trace BytesInFlight in segments
 * @param stream The output stream
 * @param oldval Old value
 * @param newval New value
 */
static void
BytesInFlightTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    uint32_t segs = newval / g_segmentSize;
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << segs << std::endl;
}

/**
 * @brief Trace FACK's inflight(AWND) in segments
 * @param stream The output stream
 * @param oldval Old value
 * @param newval New value
 */
static void
FackAwndTracer(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
    uint32_t segs = newval / g_segmentSize;
    *stream->GetStream() << Simulator::Now().GetSeconds() << " " << segs << std::endl;
}

/**
 * @brief Bind the Congestion Window trace source.
 * @param nodeId The node ID.
 * @param socketId The socket ID.
 */
void
TraceCwnd(uint32_t nodeId, uint32_t socketId)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(g_dir + "/cwnd.dat");
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/CongestionWindow",
                                  MakeBoundCallback(&CwndTracer, stream));
}

/**
 * @brief Bind the BytesInFlight trace source.
 * @param nodeId The node ID.
 * @param socketId The socket ID.
 */
void
TraceBytesInFlight(uint32_t nodeId, uint32_t socketId)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(g_dir + "/bytesInFlight.dat");
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/BytesInFlight",
                                  MakeBoundCallback(&BytesInFlightTracer, stream));
}

/**
 * @brief Bind the FackAwnd trace source.
 * @param nodeId The node ID.
 * @param socketId The socket ID.
 */
void
TraceFackAwnd(uint32_t nodeId, uint32_t socketId)
{
    AsciiTraceHelper ascii;
    Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(g_dir + "/fackAwnd.dat");
    Config::ConnectWithoutContext("/NodeList/" + std::to_string(nodeId) +
                                      "/$ns3::TcpL4Protocol/SocketList/" +
                                      std::to_string(socketId) + "/FackAwnd",
                                  MakeBoundCallback(&FackAwndTracer, stream));
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

    std::string tcpTypeId = "TcpLinuxReno";
    std::string queueDisc = "FifoQueueDisc";
    uint32_t delAckCount = 2;
    bool bql = true;
    bool enablePcap = false;
    bool sack = true;
    bool fack = true;
    Time stopTime = Seconds(10);

    queueDisc = std::string("ns3::") + queueDisc;

    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(4194304));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(6291456));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(delAckCount));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(g_segmentSize));
    Config::SetDefault("ns3::TcpSocket::InitialSlowStartThreshold",
                       UintegerValue(35 * g_segmentSize));
    Config::SetDefault("ns3::DropTailQueue<Packet>::MaxSize", QueueSizeValue(QueueSize("1p")));
    Config::SetDefault(queueDisc + "::MaxSize", QueueSizeValue(QueueSize("10p")));
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(sack));

    CommandLine cmd(__FILE__);
    cmd.AddValue("tcpTypeId", "Transport protocol to use: TcpLinuxReno", tcpTypeId);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);
    cmd.AddValue("stopTime",
                 "Stop time for applications / simulation time will be stopTime + 1",
                 stopTime);
    cmd.AddValue("fack", "Enable/Disable FACK", fack);

    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcpTypeId));
    Config::SetDefault("ns3::TcpSocketBase::Fack", BooleanValue(fack));

    // The maximum send buffer size is set to 4194304 bytes (4MB) and the
    // maximum receive buffer size is set to 6291456 bytes (6MB) in the Linux
    // kernel. The same buffer sizes are used as default in this example.

    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> streamWrapper;

    NodeContainer sender;
    NodeContainer receiver;
    NodeContainer routers;
    sender.Create(1);
    receiver.Create(1);
    routers.Create(2);

    // Create the point-to-point link helpers
    PointToPointHelper bottleneckLink;
    bottleneckLink.SetDeviceAttribute("DataRate", StringValue("1.5Mbps"));
    bottleneckLink.SetChannelAttribute("Delay", StringValue("5ms"));

    PointToPointHelper senderToR1;
    senderToR1.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    senderToR1.SetChannelAttribute("Delay", StringValue("2ms"));

    PointToPointHelper r2ToReceiver;
    r2ToReceiver.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    r2ToReceiver.SetChannelAttribute("Delay", StringValue("33ms"));

    // Create NetDevice containers
    NetDeviceContainer senderEdge = senderToR1.Install(sender.Get(0), routers.Get(0));
    NetDeviceContainer r1r2 = bottleneckLink.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer receiverEdge = r2ToReceiver.Install(routers.Get(1), receiver.Get(0));

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
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceBytesInFlight, 0, 0);
    Simulator::Schedule(Seconds(0.1) + MilliSeconds(1), &TraceFackAwnd, 0, 0);
    sourceApps.Stop(stopTime);

    // Install application on the receiver
    PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer sinkApps = sink.Install(receiver.Get(0));
    sinkApps.Start(Seconds(0));
    sinkApps.Stop(stopTime);

    // Create a new directory to store the output of the program
    g_dir = "fack-results/" + currentTime + "/";
    MakeDirectories(g_dir);

    // Open the lostSegments.dat file and write packet-loss events into it.
    g_individualLostSegments.open(g_dir + "/lostSegments.dat", std::ios::out);
    Ptr<OutputStreamWrapper> streamWrapperIndivLost =
        asciiTraceHelper.CreateFileStream(g_dir + "/lostSegments.dat");

    // Trace the queue occupancy on the second interface of R1
    tch.Uninstall(routers.Get(0)->GetDevice(1));
    QueueDiscContainer qd;
    qd = tch.Install(routers.Get(0)->GetDevice(1));

    // Get the actual QueueDisc Ptr and connect the "Drop" trace source
    Ptr<QueueDisc> q = qd.Get(0);
    q->TraceConnectWithoutContext("Drop",
                                  MakeBoundCallback(&TracePacketDrop, streamWrapperIndivLost));

    Simulator::ScheduleNow(&CheckQueueSize, qd.Get(0));
    // Generate PCAP traces if it is enabled
    if (enablePcap)
    {
        MakeDirectories(g_dir + "pcap/");
        bottleneckLink.EnablePcapAll(g_dir + "/pcap/fack", true);
    }

    // Open files for writing throughput traces and queue size
    g_throughput.open(g_dir + "/throughput.dat", std::ios::out);
    g_queueSize.open(g_dir + "/queueSize.dat", std::ios::out);

    NS_ASSERT_MSG(g_throughput.is_open(), "Throughput file was not opened correctly");
    NS_ASSERT_MSG(g_queueSize.is_open(), "Queue size file was not opened correctly");

    // Check for dropped packets using Flow Monitor
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();
    Simulator::Schedule(Seconds(0 + 0.000001), &TraceThroughput, monitor);

    Simulator::Stop(stopTime + TimeStep(1));
    Simulator::Run();
    Simulator::Destroy();

    g_throughput.close();
    g_queueSize.close();

    return 0;
}
