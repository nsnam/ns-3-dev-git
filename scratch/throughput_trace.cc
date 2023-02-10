#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
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
    n0 ------- r1 ------- n1
               tcp
*/

using namespace ns3;

double prevBytesThrough = 0.0;

static void
TraceThroughput(Ptr<OutputStreamWrapper> file, Ptr<Application> sink1Apps)
{
    Ptr<PacketSink> sink1 = DynamicCast<PacketSink>(sink1Apps);
    double throughput = ((sink1->GetTotalRx() - prevBytesThrough) * 8);
    prevBytesThrough = sink1->GetTotalRx();
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << throughput << std::endl;
    Simulator::Schedule(MilliSeconds(1.0), &TraceThroughput, file, sink1Apps);
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
    Ptr<OutputStreamWrapper> file = asciiTraceHelper.CreateFileStream("goodput.csv");
    Simulator::Schedule(
        Seconds(1.001),
        MakeBoundCallback(&TraceThroughput, file, sinkApps.Get(0)->GetObject<PacketSink>()));

    Simulator::Stop(Seconds(20));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
