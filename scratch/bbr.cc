#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/gtk-config-store.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>
#include <iostream>

using namespace ns3;

bool firstCwnd = true;
bool firstSshThr = true;
bool firstRtt = true;
bool firstRto = true;
bool firstMinRtt = true;
Ptr<OutputStreamWrapper> cWndStream;
Ptr<OutputStreamWrapper> ssThreshStream;
Ptr<OutputStreamWrapper> rttStream;
Ptr<OutputStreamWrapper> rtoStream;
Ptr<OutputStreamWrapper> inFlightStream;
Ptr<OutputStreamWrapper> throughputStream;
Ptr<OutputStreamWrapper> lossStream;
Ptr<OutputStreamWrapper> delayStream;
uint32_t cWndValue;
uint32_t ssThreshValue;
bool m_state = false;

static void
ChangeDataRate()
{
    if (m_state)
    {
        Config::Set("/NodeList/0/DeviceList/0/DataRate", StringValue("10Mbps"));
        Config::Set("/NodeList/1/DeviceList/0/DataRate", StringValue("10Mbps"));
        m_state = false;
    }
    else
    {
        Config::Set("/NodeList/0/DeviceList/0/DataRate", StringValue("20Mbps"));
        Config::Set("/NodeList/1/DeviceList/0/DataRate", StringValue("20Mbps"));
        m_state = true;
    }
    Simulator::Schedule(Seconds(50), ChangeDataRate);
}

static void
CwndTracer(uint32_t oldval, uint32_t newval)
{
    if (firstCwnd)
    {
        *cWndStream->GetStream() << "0.0 " << oldval << std::endl;
        firstCwnd = false;
    }
    *cWndStream->GetStream() << Simulator::Now().GetSeconds() << " " << newval << std::endl;
    cWndValue = newval;

    if (!firstSshThr)
    {
        *ssThreshStream->GetStream()
            << Simulator::Now().GetSeconds() << " " << ssThreshValue << std::endl;
    }
}

static void
SsThreshTracer(uint32_t oldval, uint32_t newval)
{
    if (firstSshThr)
    {
        *ssThreshStream->GetStream() << "0.0 " << oldval << std::endl;
        firstSshThr = false;
    }
    *ssThreshStream->GetStream() << Simulator::Now().GetSeconds() << " " << newval << std::endl;
    ssThreshValue = newval;

    if (!firstCwnd)
    {
        *cWndStream->GetStream() << Simulator::Now().GetSeconds() << " " << cWndValue << std::endl;
    }
}

static void
RtoTracer(Time oldval, Time newval)
{
    if (firstRto)
    {
        *rtoStream->GetStream() << "0.0 " << oldval.GetSeconds() << std::endl;
        firstRto = false;
    }
    *rtoStream->GetStream() << Simulator::Now().GetSeconds() << " " << newval.GetSeconds()
                            << std::endl;
}

static void
InFlightTracer(uint32_t old, uint32_t inFlight)
{
    *inFlightStream->GetStream() << Simulator::Now().GetSeconds() << " " << inFlight << std::endl;
}

static void
RttTracer(Time oldval, Time newval)
{
    if (firstRtt)
    {
        *rttStream->GetStream() << "0.0 " << oldval.GetSeconds() << std::endl;
        firstRtt = false;
    }
    *rttStream->GetStream() << Simulator::Now().GetSeconds() << " " << newval.GetSeconds()
                            << std::endl;
}

static void
TraceCwnd(std::string cwnd_tr_file_name)
{
    AsciiTraceHelper ascii;
    cWndStream = ascii.CreateFileStream(cwnd_tr_file_name.c_str());
    Config::ConnectWithoutContext("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                  MakeCallback(&CwndTracer));
}

static void
TraceSsThresh(std::string ssthresh_tr_file_name)
{
    AsciiTraceHelper ascii;
    ssThreshStream = ascii.CreateFileStream(ssthresh_tr_file_name.c_str());
    Config::ConnectWithoutContext("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/SlowStartThreshold",
                                  MakeCallback(&SsThreshTracer));
}

static void
TraceRtt(std::string rtt_tr_file_name)
{
    AsciiTraceHelper ascii;
    rttStream = ascii.CreateFileStream(rtt_tr_file_name.c_str());
    Config::ConnectWithoutContext("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/RTT",
                                  MakeCallback(&RttTracer));
}

static void
TraceRto(std::string rto_tr_file_name)
{
    AsciiTraceHelper ascii;
    rtoStream = ascii.CreateFileStream(rto_tr_file_name.c_str());
    Config::ConnectWithoutContext("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/RTO",
                                  MakeCallback(&RtoTracer));
}

static void
TraceInFlight(std::string& in_flight_file_name)
{
    AsciiTraceHelper ascii;
    inFlightStream = ascii.CreateFileStream(in_flight_file_name.c_str());
    Config::ConnectWithoutContext("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/BytesInFlight",
                                  MakeCallback(&InFlightTracer));
}

static void
TraceThroughput(Ptr<FlowMonitor> monitor)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        double throughput = (double)(i->second.rxBytes * 8.0) /
                            (double)(i->second.timeLastRxPacket.GetSeconds() -
                                     i->second.timeFirstTxPacket.GetSeconds()) /
                            1024 / 1024;
        *throughputStream->GetStream()
            << Simulator::Now().GetSeconds() << " " << i->first << " " << throughput << std::endl;
    }
    Simulator::Schedule(Seconds(0.1), &TraceThroughput, monitor);
}

static void
TraceLoss(Ptr<FlowMonitor> monitor)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        *lossStream->GetStream() << Simulator::Now().GetSeconds() << " " << i->first << " "
                                 << i->second.lostPackets << std::endl;
    }
    Simulator::Schedule(Seconds(0.1), &TraceLoss, monitor);
}

static void
TraceDelay(Ptr<FlowMonitor> monitor)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        *delayStream->GetStream() << Simulator::Now().GetSeconds() << " " << i->first << " "
                                  << i->second.lastDelay << std::endl;
    }
    Simulator::Schedule(Seconds(0.1), &TraceDelay, monitor);
}

// static std::string
// WhichVariant (TcpBbr::BbrVar variant)
// {
//   switch (variant)
//     {
//     case 0:
//       return "BBR";
//     case 1:
//       return "BBR_PRIME";
//     case 2:
//       return "BBR_PLUS";
//     case 3:
//       return "BBR_HSR";
//     case 4:
//       return "BBR_V2";
//     case 5:
//       return "BBR_DELAY";
//     }
//   NS_ASSERT (false);
// }

int
main(int argc, char* argv[])
{
    double minRto = 0.2;
    uint32_t initialCwnd = 10;
    double error_p = 0.00;
    double size = 3.0 / 2.0;
    uint32_t nLeaf = 5; // If non-zero, number of both left and right
    double start_time = 0.01;
    double stop_time = 200.0;
    double data_mbytes = 0;
    uint32_t mtu_bytes = 536;
    std::string bandwidth = "10Mbps";
    std::string delay = "10ms";
    std::string access_bandwidth = "40Mbps";
    std::string access_delay = "1ms";
    std::string transport_prot = "TcpBbr";
    // TcpBbr::BbrVar variant = TcpBbr::BBR_V2;
    // std::string varstr = WhichVariant (variant);
    std::string varstr = "BBRv2";
    std::string scenario = "1";
    bool ecn = true;
    bool exp = false;
    double lambda = 1.0 / 2.0;
    bool pcap = false;
    bool cubic = false;
    bool vegas = false;

    // if (variant == TcpBbr::BBR_HSR)
    //   {
    //     varstr += std::to_string(lambda);
    //   }

    // if (variant == TcpBbr::BBR_V2)
    //   {
    //     if (ecn)
    //       {
    //         varstr += "-ECN";
    //       }
    //     if (exp)
    //       {
    //         varstr += "-EXP";
    //       }
    //   }
    if (ecn)
    {
        varstr += "-ECN";
    }
    if (exp)
    {
        varstr += "-EXP";
    }

    varstr += bandwidth + "-" + delay + "-" + std::to_string(size);

    if (cubic)
    {
        varstr += "vsCubic";
    }
    else if (vegas)
    {
        varstr += "vsVegas";
    }

    time_t rawtime;
    struct tm* timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof(buffer), "%d-%m-%Y-%I-%M-%S", timeinfo);
    std::string currentTime(buffer);

    CommandLine cmd;
    cmd.AddValue("nLeaf", "Number of left and right side leaf nodes", nLeaf);
    cmd.AddValue("bandwidth", "Bottleneck bandwidth", bandwidth);
    cmd.AddValue("delay", "Bottleneck delay", delay);
    cmd.AddValue("access_bandwidth", "Access link bandwidth", access_bandwidth);
    cmd.AddValue("access_delay", "Access link delay", access_delay);
    cmd.AddValue("mtu", "Size of IP packets to send in bytes", mtu_bytes);
    cmd.AddValue("data", "Number of Megabytes of data to transmit", data_mbytes);
    cmd.AddValue("error_p", "Packet error rate", error_p);
    cmd.AddValue("qSize", "Queue Size", size);
    cmd.AddValue("start_time", "Start Time", start_time);
    cmd.AddValue("stop_time", "Stop Time", stop_time);
    cmd.AddValue("scenario", "Scenario", scenario);
    cmd.AddValue("initialCwnd", "Initial Cwnd", initialCwnd);
    cmd.AddValue("minRto", "Minimum RTO", minRto);
    cmd.AddValue("transport_prot",
                 "Transport protocol to use: TcpNewReno, "
                 "TcpHybla, TcpHighSpeed, TcpHtcp, TcpVegas, TcpScalable, TcpVeno, "
                 "TcpBic, TcpYeah, TcpIllinois, TcpWestwood, TcpWestwoodPlus, TcpLedbat, "
                 "TcpLp, TcpBbr",
                 transport_prot);
    cmd.Parse(argc, argv);

    // Calculate the ADU size
    Header* temp_header = new Ipv4Header();
    uint32_t ip_header = temp_header->GetSerializedSize();
    delete temp_header;

    temp_header = new TcpHeader();
    uint32_t tcp_header = temp_header->GetSerializedSize();
    delete temp_header;
    uint32_t tcp_adu_size = mtu_bytes - 20 - (ip_header + tcp_header);

    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(tcp_adu_size));

    if (exp)
    {
        Config::SetDefault("ns3::TcpBbr::EnableExp", BooleanValue(true));
    }
    if (ecn)
    {
        Config::SetDefault("ns3::TcpSocketBase::EcnMode", StringValue("ClassicEcn"));
        Config::SetDefault("ns3::RedQueueDisc::UseEcn", BooleanValue(true));
        Config::SetDefault("ns3::TcpBbr::EnableEcn", BooleanValue(true));
    }

    DataRate access_b(access_bandwidth);
    DataRate bottle_b(bandwidth);
    Time access_d(access_delay);
    Time bottle_d(delay);

    if (size != 0)
    {
        size *= (std::min(access_b, bottle_b).GetBitRate() / 8) *
                ((access_d + bottle_d) * 2).GetSeconds();
        Config::SetDefault("ns3::PfifoFastQueueDisc::Limit", UintegerValue(size / mtu_bytes));
    }

    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(initialCwnd));
    Config::SetDefault("ns3::TcpSocketBase::MinRto", TimeValue(Seconds(minRto)));

    transport_prot = std::string("ns3::") + transport_prot;
    // Select TCP variant
    if (transport_prot.compare("ns3::TcpWestwoodPlus") == 0)
    {
        // TcpWestwoodPlus is not an actual TypeId name; we need TcpWestwood here
        Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpWestwoodPlus::GetTypeId()));
        // the default protocol type in ns3::TcpWestwood is WESTWOOD
        // Config::SetDefault("ns3::TcpWestwood::ProtocolType", EnumValue(TcpWestwoodPlus::WESTWOODPLUS));
    }
    else
    {
        TypeId tcpTid;
        NS_ABORT_MSG_UNLESS(TypeId::LookupByNameFailSafe(transport_prot, &tcpTid),
                            "TypeId " << transport_prot << " not found");
        Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                           TypeIdValue(TypeId::LookupByName(transport_prot)));
        // Config::SetDefault("ns3::TcpBbr::BBRVariant", EnumValue(variant));
        Config::SetDefault("ns3::TcpBbr::RTPropLambda", DoubleValue(lambda));
    }

    Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
    uv->SetStream(50);
    RateErrorModel error_model;
    error_model.SetRandomVariable(uv);
    error_model.SetUnit(RateErrorModel::ERROR_UNIT_PACKET);
    error_model.SetRate(error_p);

    // Create the point-to-point link helpers
    PointToPointHelper pointToPointRouter;
    pointToPointRouter.SetDeviceAttribute("DataRate", StringValue(bandwidth));
    pointToPointRouter.SetChannelAttribute("Delay", StringValue(delay));
    pointToPointRouter.SetDeviceAttribute("ReceiveErrorModel", PointerValue(&error_model));

    PointToPointHelper pointToPointLeaf;
    pointToPointLeaf.SetDeviceAttribute("DataRate", StringValue(access_bandwidth));
    pointToPointLeaf.SetChannelAttribute("Delay", StringValue(access_delay));

    PointToPointDumbbellHelper d(nLeaf + 1,
                                 pointToPointLeaf,
                                 nLeaf + 1,
                                 pointToPointLeaf,
                                 pointToPointRouter);
    // Install Stack
    InternetStackHelper stack;
    d.InstallStack(stack);

    // Assign IP Addresses
    d.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.2.1.0", "255.255.255.0"),
                          Ipv4AddressHelper("10.3.1.0", "255.255.255.0"));

    // Install app on all right side nodes
    uint16_t port = 50000;
    Address sinkLocalAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
    PacketSinkHelper sinkHelper("ns3::TcpSocketFactory", sinkLocalAddress);
    ApplicationContainer sinkApp;

    if (cubic)
    {
        TypeId tid = TypeId::LookupByName("ns3::TcpCubic");
        std::stringstream nodeId;
        nodeId << d.GetLeft(2)->GetId();
        std::string node = "/NodeList/" + nodeId.str() + "/$ns3::TcpL4Protocol/SocketType";
        Config::Set(node, TypeIdValue(tid));
    }
    else if (vegas)
    {
        TypeId tid = TypeId::LookupByName("ns3::TcpVegas");
        std::stringstream nodeId;
        nodeId << d.GetLeft(2)->GetId();
        std::string node = "/NodeList/" + nodeId.str() + "/$ns3::TcpL4Protocol/SocketType";
        Config::Set(node, TypeIdValue(tid));
    }

    for (uint32_t i = 0; i < nLeaf; ++i)
    {
        sinkApp.Add(sinkHelper.Install(d.GetRight(i)));
        sinkApp.Start(Seconds(start_time));
        sinkApp.Stop(Seconds(stop_time));

        BulkSendHelper ftp("ns3::TcpSocketFactory", Address());
        ftp.SetAttribute("MaxBytes", UintegerValue(int(data_mbytes * 1000000)));
        ftp.SetAttribute("SendSize", UintegerValue(tcp_adu_size));

        ApplicationContainer sourceApp;

        AddressValue remoteAddress(InetSocketAddress(d.GetRightIpv4Address(i), port));
        ftp.SetAttribute("Remote", remoteAddress);
        sourceApp = ftp.Install(d.GetLeft(i));
        sourceApp.Start(Seconds(start_time + i * 20));
        if (i == 3)
        {
            sourceApp.Stop(Seconds(stop_time / 2));
        }
        else if (i == 4)
        {
            sourceApp.Stop(Seconds(stop_time - 50));
        }
        else
        {
            sourceApp.Stop(Seconds(stop_time - 1));
        }
    }

    // Set up the acutal simulation
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    std::string dir = "results/" + transport_prot.substr(5, transport_prot.length()) + "/" +
                      currentTime + "-" + varstr + "/";
    std::string dirToSave = "mkdir -p " + dir;
    system(dirToSave.c_str());
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceCwnd, dir + "cwnd.data");
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceSsThresh, dir + "ssth.data");
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceRtt, dir + "rtt.data");
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceRto, dir + "rto.data");
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceInFlight, dir + "inflight.data");

    if (scenario == "2")
    {
        Simulator::Schedule(Seconds(50), &ChangeDataRate);
    }

    if (pcap)
    {
        pointToPointRouter.EnablePcapAll(dir + "p", d.GetLeft());
    }

    // Install FlowMonitor on all nodes
    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    std::string throughputFileName = dir + "throughput.data";
    std::string lossFileName = dir + "loss.data";
    std::string delayFileName = dir + "delay.data";
    AsciiTraceHelper ascii;
    throughputStream = ascii.CreateFileStream(throughputFileName.c_str());
    lossStream = ascii.CreateFileStream(lossFileName.c_str());
    delayStream = ascii.CreateFileStream(delayFileName.c_str());
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceThroughput, monitor);
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceLoss, monitor);
    Simulator::Schedule(Seconds(start_time + 0.000001), &TraceDelay, monitor);

    std::ofstream myfile;
    myfile.open(dir + "config.txt", std::fstream::in | std::fstream::out | std::fstream::app);
    myfile << "nLeaf " << nLeaf << "\n";
    myfile << "bandwidth " << bandwidth << "\n";
    myfile << "delay  " << delay << "\n";
    myfile << "access_bandwidth " << access_bandwidth << "\n";
    myfile << "access_delay " << access_delay << "\n";
    myfile << "mtu " << std::to_string(mtu_bytes) << "\n";
    myfile << "data  " << std::to_string(data_mbytes) << "\n";
    myfile << "error_p " << error_p << "\n";
    myfile << "qSize " << size << "\n";
    myfile << "scenario " << scenario << "\n";
    myfile << "initialCwnd  " << initialCwnd << "\n";
    myfile << "minRto " << minRto << "\n";
    myfile << "transport_prot " << transport_prot << "\n";
    myfile << "variant" << varstr << "\n";
    myfile.close();

    Simulator::Stop(Seconds(stop_time + 1));
    Simulator::Run();

    uint32_t totalRxBytesCounter = 0;
    for (uint32_t i = 0; i < sinkApp.GetN(); i++)
    {
        Ptr<Application> app = sinkApp.Get(i);
        Ptr<PacketSink> pktSink = DynamicCast<PacketSink>(app);
        totalRxBytesCounter += pktSink->GetTotalRx();
    }
    std::cout << "\nGoodput Bytes/sec: " << totalRxBytesCounter / Simulator::Now().GetSeconds()
              << "\n";

    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    std::ofstream file;
    file.open(dir + "flows.data", std::fstream::in | std::fstream::out | std::fstream::app);
    for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
         i != stats.end();
         ++i)
    {
        Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(i->first);
        std::cout << "Flow " << i->first << " (" << t.sourceAddress << " -> "
                  << t.destinationAddress << ")\n";
        file << "Flow " << i->first << " (" << t.sourceAddress << " -> " << t.destinationAddress
             << ")\n";
        file << "Tx Packets: " << i->second.txPackets << "\n";
        file << "Tx Bytes: " << i->second.txBytes << "\n";
        file << "Rx Packets: " << i->second.rxPackets << "\n";
        file << "Rx Bytes: " << i->second.rxBytes << "\n";
        file << "Delay Sum: " << i->second.delaySum << "\n";
        file << "Jitter Sum: " << i->second.jitterSum << "\n";
        file << "Last Delay: " << i->second.lastDelay << "\n";
        file << "Lost Packets: " << i->second.lostPackets << "\n";
        file << "Times Forwarded: " << i->second.timesForwarded << "\n";
        file << "Throughput: "
             << ((double)i->second.rxBytes * 8.0) /
                    (double)(i->second.timeLastRxPacket.GetSeconds() -
                             i->second.timeFirstTxPacket.GetSeconds()) /
                    1024 / 1024
             << " Mbps"
             << "\n";
        file << "Packet loss Ratio: "
             << ((double)i->second.txPackets - (double)i->second.rxPackets) /
                    (double)i->second.txPackets
             << "\n";
        file << "Delay mean: " << i->second.delaySum.GetSeconds() / i->second.rxPackets << "\n";
        file << "Jitter mean: " << i->second.jitterSum.GetSeconds() / i->second.rxPackets << "\n";
    }

    file << "Goodput Bytes/Sec: " << totalRxBytesCounter / Simulator::Now().GetSeconds();
    file.close();
    Simulator::Destroy();
    return 0;
}