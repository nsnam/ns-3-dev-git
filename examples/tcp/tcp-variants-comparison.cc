/*
 * Copyright (c) 2013 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Justin P. Rohrer, Truc Anh N. Nguyen <annguyen@ittc.ku.edu>, Siddharth Gangadhar
 * <siddharth@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 *
 * "TCP Westwood(+) Protocol Implementation in ns-3"
 * Siddharth Gangadhar, Trúc Anh Ngọc Nguyễn , Greeshma Umapathi, and James P.G. Sterbenz,
 * ICST SIMUTools Workshop on ns-3 (WNS3), Cannes, France, March 2013
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/enum.h"
#include "ns3/error-model.h"
#include "ns3/event-id.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/tcp-header.h"
#include "ns3/traffic-control-module.h"
#include "ns3/udp-header.h"

#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpVariantsComparison");

static std::map<uint32_t, bool> firstCwnd;                      //!< First congestion window.
static std::map<uint32_t, bool> firstSshThr;                    //!< First SlowStart threshold.
static std::map<uint32_t, bool> firstRtt;                       //!< First RTT.
static std::map<uint32_t, bool> firstRto;                       //!< First RTO.
static std::map<uint32_t, Ptr<OutputStreamWrapper>> cWndStream; //!< Congstion window output stream.
static std::map<uint32_t, Ptr<OutputStreamWrapper>>
    ssThreshStream; //!< SlowStart threshold output stream.
static std::map<uint32_t, Ptr<OutputStreamWrapper>> rttStream;      //!< RTT output stream.
static std::map<uint32_t, Ptr<OutputStreamWrapper>> rtoStream;      //!< RTO output stream.
static std::map<uint32_t, Ptr<OutputStreamWrapper>> nextTxStream;   //!< Next TX output stream.
static std::map<uint32_t, Ptr<OutputStreamWrapper>> nextRxStream;   //!< Next RX output stream.
static std::map<uint32_t, Ptr<OutputStreamWrapper>> inFlightStream; //!< In flight output stream.
static std::map<uint32_t, uint32_t> cWndValue;                      //!< congestion window value.
static std::map<uint32_t, uint32_t> ssThreshValue;                  //!< SlowStart threshold value.

/**
 * Get the Node Id From Context.
 *
 * @param context The context.
 * @return the node ID.
 */
static uint32_t
GetNodeIdFromContext(std::string context)
{
    const std::size_t n1 = context.find_first_of('/', 1);
    const std::size_t n2 = context.find_first_of('/', n1 + 1);
    return std::stoul(context.substr(n1 + 1, n2 - n1 - 1));
}

/**
 * Congestion window tracer.
 *
 * @param context The context.
 * @param oldval Old value.
 * @param newval New value.
 */
static void
CwndTracer(std::string context, uint32_t oldval, uint32_t newval)
{
    uint32_t nodeId = GetNodeIdFromContext(context);

    if (firstCwnd[nodeId])
    {
        *cWndStream[nodeId]->GetStream() << "0.0 " << oldval << std::endl;
        firstCwnd[nodeId] = false;
    }
    *cWndStream[nodeId]->GetStream() << Simulator::Now().GetSeconds() << " " << newval << std::endl;
    cWndValue[nodeId] = newval;

    if (!firstSshThr[nodeId])
    {
        *ssThreshStream[nodeId]->GetStream()
            << Simulator::Now().GetSeconds() << " " << ssThreshValue[nodeId] << std::endl;
    }
}

/**
 * Slow start threshold tracer.
 *
 * @param context The context.
 * @param oldval Old value.
 * @param newval New value.
 */
static void
SsThreshTracer(std::string context, uint32_t oldval, uint32_t newval)
{
    uint32_t nodeId = GetNodeIdFromContext(context);

    if (firstSshThr[nodeId])
    {
        *ssThreshStream[nodeId]->GetStream() << "0.0 " << oldval << std::endl;
        firstSshThr[nodeId] = false;
    }
    *ssThreshStream[nodeId]->GetStream()
        << Simulator::Now().GetSeconds() << " " << newval << std::endl;
    ssThreshValue[nodeId] = newval;

    if (!firstCwnd[nodeId])
    {
        *cWndStream[nodeId]->GetStream()
            << Simulator::Now().GetSeconds() << " " << cWndValue[nodeId] << std::endl;
    }
}

/**
 * RTT tracer.
 *
 * @param context The context.
 * @param oldval Old value.
 * @param newval New value.
 */
static void
RttTracer(std::string context, Time oldval, Time newval)
{
    uint32_t nodeId = GetNodeIdFromContext(context);

    if (firstRtt[nodeId])
    {
        *rttStream[nodeId]->GetStream() << "0.0 " << oldval.GetSeconds() << std::endl;
        firstRtt[nodeId] = false;
    }
    *rttStream[nodeId]->GetStream()
        << Simulator::Now().GetSeconds() << " " << newval.GetSeconds() << std::endl;
}

/**
 * RTO tracer.
 *
 * @param context The context.
 * @param oldval Old value.
 * @param newval New value.
 */
static void
RtoTracer(std::string context, Time oldval, Time newval)
{
    uint32_t nodeId = GetNodeIdFromContext(context);

    if (firstRto[nodeId])
    {
        *rtoStream[nodeId]->GetStream() << "0.0 " << oldval.GetSeconds() << std::endl;
        firstRto[nodeId] = false;
    }
    *rtoStream[nodeId]->GetStream()
        << Simulator::Now().GetSeconds() << " " << newval.GetSeconds() << std::endl;
}

/**
 * Next TX tracer.
 *
 * @param context The context.
 * @param old Old sequence number.
 * @param nextTx Next sequence number.
 */
static void
NextTxTracer(std::string context, SequenceNumber32 old [[maybe_unused]], SequenceNumber32 nextTx)
{
    uint32_t nodeId = GetNodeIdFromContext(context);

    *nextTxStream[nodeId]->GetStream()
        << Simulator::Now().GetSeconds() << " " << nextTx << std::endl;
}

/**
 * In-flight tracer.
 *
 * @param context The context.
 * @param old Old value.
 * @param inFlight In flight value.
 */
static void
InFlightTracer(std::string context, uint32_t old [[maybe_unused]], uint32_t inFlight)
{
    uint32_t nodeId = GetNodeIdFromContext(context);

    *inFlightStream[nodeId]->GetStream()
        << Simulator::Now().GetSeconds() << " " << inFlight << std::endl;
}

/**
 * Next RX tracer.
 *
 * @param context The context.
 * @param old Old sequence number.
 * @param nextRx Next sequence number.
 */
static void
NextRxTracer(std::string context, SequenceNumber32 old [[maybe_unused]], SequenceNumber32 nextRx)
{
    uint32_t nodeId = GetNodeIdFromContext(context);

    *nextRxStream[nodeId]->GetStream()
        << Simulator::Now().GetSeconds() << " " << nextRx << std::endl;
}

/**
 * Congestion window trace connection.
 *
 * @param cwnd_tr_file_name Congestion window trace file name.
 * @param nodeId Node ID.
 */
static void
TraceCwnd(std::string cwnd_tr_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    cWndStream[nodeId] = ascii.CreateFileStream(cwnd_tr_file_name);
    Config::Connect("/NodeList/" + std::to_string(nodeId) +
                        "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                    MakeCallback(&CwndTracer));
}

/**
 * Slow start threshold trace connection.
 *
 * @param ssthresh_tr_file_name Slow start threshold trace file name.
 * @param nodeId Node ID.
 */
static void
TraceSsThresh(std::string ssthresh_tr_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    ssThreshStream[nodeId] = ascii.CreateFileStream(ssthresh_tr_file_name);
    Config::Connect("/NodeList/" + std::to_string(nodeId) +
                        "/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold",
                    MakeCallback(&SsThreshTracer));
}

/**
 * RTT trace connection.
 *
 * @param rtt_tr_file_name RTT trace file name.
 * @param nodeId Node ID.
 */
static void
TraceRtt(std::string rtt_tr_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    rttStream[nodeId] = ascii.CreateFileStream(rtt_tr_file_name);
    Config::Connect("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/RTT",
                    MakeCallback(&RttTracer));
}

/**
 * RTO trace connection.
 *
 * @param rto_tr_file_name RTO trace file name.
 * @param nodeId Node ID.
 */
static void
TraceRto(std::string rto_tr_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    rtoStream[nodeId] = ascii.CreateFileStream(rto_tr_file_name);
    Config::Connect("/NodeList/" + std::to_string(nodeId) + "/$ns3::TcpL4Protocol/SocketList/0/RTO",
                    MakeCallback(&RtoTracer));
}

/**
 * Next TX trace connection.
 *
 * @param next_tx_seq_file_name Next TX trace file name.
 * @param nodeId Node ID.
 */
static void
TraceNextTx(std::string& next_tx_seq_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    nextTxStream[nodeId] = ascii.CreateFileStream(next_tx_seq_file_name);
    Config::Connect("/NodeList/" + std::to_string(nodeId) +
                        "/$ns3::TcpL4Protocol/SocketList/0/NextTxSequence",
                    MakeCallback(&NextTxTracer));
}

/**
 * In flight trace connection.
 *
 * @param in_flight_file_name In flight trace file name.
 * @param nodeId Node ID.
 */
static void
TraceInFlight(std::string& in_flight_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    inFlightStream[nodeId] = ascii.CreateFileStream(in_flight_file_name);
    Config::Connect("/NodeList/" + std::to_string(nodeId) +
                        "/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight",
                    MakeCallback(&InFlightTracer));
}

/**
 * Next RX trace connection.
 *
 * @param next_rx_seq_file_name Next RX trace file name.
 * @param nodeId Node ID.
 */
static void
TraceNextRx(std::string& next_rx_seq_file_name, uint32_t nodeId)
{
    AsciiTraceHelper ascii;
    nextRxStream[nodeId] = ascii.CreateFileStream(next_rx_seq_file_name);
    Config::Connect("/NodeList/" + std::to_string(nodeId) +
                        "/$ns3::TcpL4Protocol/SocketList/1/RxBuffer/NextRxSequence",
                    MakeCallback(&NextRxTracer));
}

int
main(int argc, char* argv[])
{
    std::string transport_prot = "TcpWestwoodPlus";
    double error_p = 0.0;
    std::string bandwidth = "2Mbps";
    std::string delay = "0.01ms";
    std::string access_bandwidth = "10Mbps";
    std::string access_delay = "45ms";
    bool tracing = false;
    std::string prefix_file_name = "TcpVariantsComparison";
    uint64_t data_mbytes = 0;
    uint32_t mtu_bytes = 400;
    uint16_t num_flows = 1;
    double duration = 100.0;
    uint32_t run = 0;
    bool flow_monitor = false;
    bool pcap = false;
    bool sack = true;
    std::string queue_disc_type = "ns3::PfifoFastQueueDisc";
    std::string recovery = "ns3::TcpClassicRecovery";

    CommandLine cmd(__FILE__);
    cmd.AddValue("transport_prot",
                 "Transport protocol to use: TcpNewReno, TcpLinuxReno, "
                 "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                 "TcpBic, TcpYeah, TcpIllinois, TcpWestwoodPlus, TcpLedbat, "
                 "TcpLp, TcpDctcp, TcpCubic, TcpBbr",
                 transport_prot);
    cmd.AddValue("error_p", "Packet error rate", error_p);
    cmd.AddValue("bandwidth", "Bottleneck bandwidth", bandwidth);
    cmd.AddValue("delay", "Bottleneck delay", delay);
    cmd.AddValue("access_bandwidth", "Access link bandwidth", access_bandwidth);
    cmd.AddValue("access_delay", "Access link delay", access_delay);
    cmd.AddValue("tracing", "Flag to enable/disable tracing", tracing);
    cmd.AddValue("prefix_name", "Prefix of output trace file", prefix_file_name);
    cmd.AddValue("data", "Number of Megabytes of data to transmit", data_mbytes);
    cmd.AddValue("mtu", "Size of IP packets to send in bytes", mtu_bytes);
    cmd.AddValue("num_flows", "Number of flows", num_flows);
    cmd.AddValue("duration", "Time to allow flows to run in seconds", duration);
    cmd.AddValue("run", "Run index (for setting repeatable seeds)", run);
    cmd.AddValue("flow_monitor", "Enable flow monitor", flow_monitor);
    cmd.AddValue("pcap_tracing", "Enable or disable PCAP tracing", pcap);
    cmd.AddValue("queue_disc_type",
                 "Queue disc type for gateway (e.g. ns3::CoDelQueueDisc)",
                 queue_disc_type);
    cmd.AddValue("sack", "Enable or disable SACK option", sack);
    cmd.AddValue("recovery", "Recovery algorithm type to use (e.g., ns3::TcpPrrRecovery", recovery);
    cmd.Parse(argc, argv);

    transport_prot = std::string("ns3::") + transport_prot;

    SeedManager::SetSeed(1);
    SeedManager::SetRun(run);

    // User may find it convenient to enable logging
    // LogComponentEnable("TcpVariantsComparison", LOG_LEVEL_ALL);
    // LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    // LogComponentEnable("PfifoFastQueueDisc", LOG_LEVEL_ALL);

    // Calculate the ADU size
    Header* temp_header = new Ipv4Header();
    uint32_t ip_header = temp_header->GetSerializedSize();
    NS_LOG_LOGIC("IP Header size is: " << ip_header);
    delete temp_header;
    temp_header = new TcpHeader();
    uint32_t tcp_header = temp_header->GetSerializedSize();
    NS_LOG_LOGIC("TCP Header size is: " << tcp_header);
    delete temp_header;
    uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);
    NS_LOG_LOGIC("TCP ADU size is: " << tcp_adu_size);

    // Set the simulation start and stop time
    double start_time = 0.1;
    double stop_time = start_time + duration;

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(sack));

    Config::SetDefault("ns3::TcpL4Protocol::RecoveryType",
                       TypeIdValue(TypeId::LookupByName(recovery)));
    // Select TCP variant
    TypeId tcpTid;
    NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(transport_prot, &tcpTid),
                        "TypeId " << transport_prot << " not found");
    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(transport_prot)));

    // Create gateways, sources, and sinks
    NodeContainer gateways;
    gateways.Create(1);
    NodeContainer sources;
    sources.Create(num_flows);
    NodeContainer sinks;
    sinks.Create(num_flows);

    // Configure the error model
    // Here we use RateErrorModel with packet error rate
    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    uv->SetStream(50);
    RateErrorModel error_model;
    error_model.SetRandomVariable(uv);
    error_model.SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
    error_model.SetRate(error_p);

    PointToPointHelper UnReLink;
    UnReLink.SetDeviceAttribute("DataRate", StringValue(bandwidth));
    UnReLink.SetChannelAttribute("Delay", StringValue(delay));
    UnReLink.SetDeviceAttribute("ReceiveErrorModel", PointerValue(&error_model));

    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tchPfifo;
    tchPfifo.SetRootQueueDisc("ns3::PfifoFastQueueDisc");

    TrafficControlHelper tchCoDel;
    tchCoDel.SetRootQueueDisc("ns3::CoDelQueueDisc");

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");

    // Configure the sources and sinks net devices
    // and the channels between the sources/sinks and the gateways
    PointToPointHelper LocalLink;
    LocalLink.SetDeviceAttribute("DataRate", StringValue(access_bandwidth));
    LocalLink.SetChannelAttribute("Delay", StringValue(access_delay));

    Ipv4InterfaceContainer sink_interfaces;

    DataRate access_b(access_bandwidth);
    DataRate bottle_b(bandwidth);
    Time access_d(access_delay);
    Time bottle_d(delay);

    uint32_t size = static_cast<uint32_t>((std::min(access_b, bottle_b).GetBitRate() / 8) *
                                          ((access_d + bottle_d) * 2).GetSeconds());

    Config::SetDefault("ns3::PfifoFastQueueDisc::MaxSize",
                       QueueSizeValue(QueueSize(QueueSizeUnit::PACKETS, size / mtu_bytes)));
    Config::SetDefault("ns3::CoDelQueueDisc::MaxSize",
                       QueueSizeValue(QueueSize(QueueSizeUnit::BYTES, size)));

    for (uint32_t i = 0; i < num_flows; i++)
    {
        NetDeviceContainer devices;
        devices = LocalLink.Install(sources.Get(i), gateways.Get(0));
        tchPfifo.Install(devices);
        address.NewNetwork();
        Ipv4InterfaceContainer interfaces = address.Assign(devices);

        devices = UnReLink.Install(gateways.Get(0), sinks.Get(i));
        if (queue_disc_type == "ns3::PfifoFastQueueDisc")
        {
            tchPfifo.Install(devices);
        }
        else if (queue_disc_type == "ns3::CoDelQueueDisc")
        {
            tchCoDel.Install(devices);
        }
        else
        {
            NS_FATAL_ERROR("Queue not recognized. Allowed values are ns3::CoDelQueueDisc or "
                           "ns3::PfifoFastQueueDisc");
        }
        address.NewNetwork();
        interfaces = address.Assign(devices);
        sink_interfaces.Add(interfaces.Get(1));
    }

    NS_LOG_INFO("Initialize Global Routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50000;
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);

    for (uint32_t i = 0; i < sources.GetN(); i++)
    {
        AddressValue remoteAddress(InetSocketAddress(sink_interfaces.GetAddress(i, 0), port));
        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(tcp_adu_size));
        BulkSendHelper ftp("ns3::TcpSocketFactory", Address());
        ftp.SetAttribute("Remote", remoteAddress);
        ftp.SetAttribute("SendSize", UintegerValue(tcp_adu_size));
        ftp.SetAttribute("MaxBytes", UintegerValue(data_mbytes * 1000000));

        ApplicationContainer sourceApp = ftp.Install(sources.Get(i));
        sourceApp.Start(Seconds(start_time * i));
        sourceApp.Stop(Seconds(stop_time - 3));

        sinkHelper.SetAttribute("Protocol", TypeIdValue(TcpSocketFactory::GetTypeId()));
        ApplicationContainer sinkApp = sinkHelper.Install(sinks.Get(i));
        sinkApp.Start(Seconds(start_time * i));
        sinkApp.Stop(Seconds(stop_time));
    }

    // Set up tracing if enabled
    if (tracing)
    {
        std::ofstream ascii;
        Ptr<OutputStreamWrapper> ascii_wrap;
        ascii.open(prefix_file_name + "-ascii");
        ascii_wrap = new OutputStreamWrapper(prefix_file_name + "-ascii", std::ios::out);
        stack.EnableAsciiIpv4All(ascii_wrap);

        for (uint16_t index = 0; index < num_flows; index++)
        {
            std::string flowString;
            if (num_flows > 1)
            {
                flowString = "-flow" + std::to_string(index);
            }

            firstCwnd[index + 1] = true;
            firstSshThr[index + 1] = true;
            firstRtt[index + 1] = true;
            firstRto[index + 1] = true;

            Simulator::Schedule(Seconds(start_time * index + 0.00001),
                                &TraceCwnd,
                                prefix_file_name + flowString + "-cwnd.data",
                                index + 1);
            Simulator::Schedule(Seconds(start_time * index + 0.00001),
                                &TraceSsThresh,
                                prefix_file_name + flowString + "-ssth.data",
                                index + 1);
            Simulator::Schedule(Seconds(start_time * index + 0.00001),
                                &TraceRtt,
                                prefix_file_name + flowString + "-rtt.data",
                                index + 1);
            Simulator::Schedule(Seconds(start_time * index + 0.00001),
                                &TraceRto,
                                prefix_file_name + flowString + "-rto.data",
                                index + 1);
            Simulator::Schedule(Seconds(start_time * index + 0.00001),
                                &TraceNextTx,
                                prefix_file_name + flowString + "-next-tx.data",
                                index + 1);
            Simulator::Schedule(Seconds(start_time * index + 0.00001),
                                &TraceInFlight,
                                prefix_file_name + flowString + "-inflight.data",
                                index + 1);
            Simulator::Schedule(Seconds(start_time * index + 0.1),
                                &TraceNextRx,
                                prefix_file_name + flowString + "-next-rx.data",
                                num_flows + index + 1);
        }
    }

    if (pcap)
    {
        UnReLink.EnablePcapAll(prefix_file_name, true);
        LocalLink.EnablePcapAll(prefix_file_name, true);
    }

    // Flow monitor
    FlowMonitorHelper flowHelper;
    if (flow_monitor)
    {
        flowHelper.InstallAll();
    }

    Simulator::Stop(Seconds(stop_time));
    Simulator::Run();

    if (flow_monitor)
    {
        flowHelper.SerializeToXmlFile(prefix_file_name + ".flowmonitor", true, true);
    }

    Simulator::Destroy();
    return 0;
}
