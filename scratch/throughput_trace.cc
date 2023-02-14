#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/queue.h"

#include <fstream>
#include <iostream>

/*
    Network Topology

       point to point
    n0 ------------- n1
            tcp
*/

using namespace ns3;

static void
TraceGoodput(Ptr<OutputStreamWrapper> file, Ptr<PacketSink> sink1, double prevBytesThrough)
{
    double goodput = ((sink1->GetTotalRx() - prevBytesThrough) * 8);
    prevBytesThrough = sink1->GetTotalRx();
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << goodput / 0.1 / 1024 / 1024
                       << std::endl;
    Simulator::Schedule(Seconds(0.1), &TraceGoodput, file, sink1, prevBytesThrough);
}

static void
TraceThroughput(Ptr<FlowMonitor> monitor, Ptr<OutputStreamWrapper> file)
{
    FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats();
    auto i = stats.begin();
    // for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin();
    //      i != stats.end();
    //      ++i)
    // {
    double throughput = (double)(i->second.rxBytes * 8.0) /
                        (double)(i->second.timeLastRxPacket.GetSeconds() -
                                 i->second.timeFirstTxPacket.GetSeconds()) /
                        1024 / 1024;
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << throughput << std::endl;
    // }
    Simulator::Schedule(Seconds(0.1), &TraceThroughput, monitor, file);
}

int
main(int argc, char* argv[])
{
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ptr<FlowMonitor> flowMonitor;
    FlowMonitorHelper flowhelper;
    flowMonitor = flowhelper.InstallAll();
    flowMonitor->CheckForLostPackets();
    // flowMonitor->SerializeToXmlFile("flow_trace.xml", true, true);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    uint16_t sinkPort = 8080;
    Address sinkAddress(InetSocketAddress(interfaces.GetAddress(1), sinkPort));
    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory",
                                      InetSocketAddress(Ipv4Address::GetAny(), sinkPort));
    ApplicationContainer sinkApps = packetSinkHelper.Install(nodes.Get(1));
    sinkApps.Start(Seconds(0.));
    sinkApps.Stop(Seconds(15.));

    BulkSendHelper blksender("ns3::TcpSocketFactory",
                             InetSocketAddress(interfaces.GetAddress(1), sinkPort));
    ApplicationContainer senderApps = blksender.Install(nodes.Get(0));
    senderApps.Get(0)->SetStartTime(Seconds(1.));
    senderApps.Get(0)->SetStopTime(Seconds(10.));

    // sinkApps.Get(0)->GetObject<PacketSink>()->GetTotalRx();

    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> throughput_file = asciiTraceHelper.CreateFileStream("throughput.csv");
    Ptr<OutputStreamWrapper> goodput_file = asciiTraceHelper.CreateFileStream("goodput.csv");
    Simulator::Schedule(Seconds(1.001),
                        MakeBoundCallback(&TraceThroughput, flowMonitor, throughput_file));
    Simulator::Schedule(
        Seconds(1.001),
        MakeBoundCallback(&TraceGoodput, goodput_file, sinkApps.Get(0)->GetObject<PacketSink>(), 0.0));

    Simulator::Stop(Seconds(20));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
