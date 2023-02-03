#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
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

NS_LOG_COMPONENT_DEFINE("QueueExample");

static void
CwndChange(ns3::Ptr<ns3::Packet const> p)
{
    std::cout << "Drop occured" << '\n';
    // *file->GetStream() << Simulator::Now().GetSeconds() << '\n';
}

static void 
TraceCwnd()
{
    std::cout << "Connecting trace" << '\n';
    Config::ConnectWithoutContext("/NodeList/0/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback(&CwndChange));
    Config::ConnectWithoutContext("/NodeList/1/DeviceList/0/$ns3::PointToPointNetDevice/RxQueue/Drop", MakeCallback(&CwndChange));
    Config::ConnectWithoutContext("/NodeList/1/DeviceList/1/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback(&CwndChange));
    Config::ConnectWithoutContext("/NodeList/2/DeviceList/0/$ns3::PointToPointNetDevice/TxQueue/Drop", MakeCallback(&CwndChange));
}

int main()
{
    bool enable_log = false;
    std::string tcp_type = "TcpCubic";

    if (enable_log)
    {
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    NodeContainer p2pnodes(3);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcp_type));
    // Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
    // Config::SetDefault("ns3::FifoQueueDisc::MaxSize", QueueSizeValue(QueueSize("10p")));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("10ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    // p2phelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));

    NetDeviceContainer p2pleftdevices = p2phelper.Install(p2pnodes.Get(0), p2pnodes.Get(1));
    NetDeviceContainer p2prightdevices = p2phelper.Install(p2pnodes.Get(1), p2pnodes.Get(2));

    InternetStackHelper istack;
    istack.Install(p2pnodes);

    Ipv4AddressHelper ipaddr("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pinterfacesleft = ipaddr.Assign(p2pleftdevices);
    ipaddr.NewNetwork();
    Ipv4InterfaceContainer p2pinterfacesright = ipaddr.Assign(p2prightdevices);

    BulkSendHelper 
        blkhelper("ns3::TcpSocketFactory", InetSocketAddress(p2pinterfacesright.GetAddress(1), 800));

    PacketSinkHelper sinkhelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));

    ApplicationContainer apps;
    apps.Add(blkhelper.Install(p2pnodes.Get(0)));
    apps.Add(sinkhelper.Install(p2pnodes.Get(2)));

    apps.Get(0)->SetStartTime(Seconds(1.0));
    apps.Get(0)->SetStopTime(Seconds(10.0));

    apps.Get(1)->SetStartTime(Seconds(0.0));
    apps.Get(1)->SetStopTime(Seconds(50.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Schedule(Seconds(1.001), &TraceCwnd);

    // AnimationInterface anim("../anim.xml");

    p2phelper.EnablePcapAll("p2p");

    Simulator::Run();
    Simulator::Destroy();
}