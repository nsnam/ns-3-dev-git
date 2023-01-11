#include "ns3/applications-module.h"
#include "ns3/core-module.h"
// #include "ns3/flow-monitor-module.h"
// #include "ns3/gtk-config-store.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
// #include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>
#include <iostream>

/*
                   Network Topology
        tcp        LogComponentEnable                        tcp
n0 -------------\      10.1.2.0     /---------------n4
  (LAN 10.1.3.0) n2----------------n3 (LAN 10.1.3.0)
n1 -------------/   point-to-point  \---------------n5
        tcp                                tcp
*/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpIntraProtocolExample");

static void
CwndChange(Ptr<OutputStreamWrapper> file, uint32_t oldval, uint32_t newval)
{
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << newval << '\n';
}

static void 
TraceCwnd()
{
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> file1 = asciiTraceHelper.CreateFileStream("tracecwnd1.csv");
    Ptr<OutputStreamWrapper> file2 = asciiTraceHelper.CreateFileStream("tracecwnd2.csv");
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file1));
    Config::ConnectWithoutContext("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file2));
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

    NodeContainer leftcsmanodes(2), p2pnodes(2), rightcsmanodes(2);
    NodeContainer nodes;
    nodes.Add(leftcsmanodes);
    nodes.Add(p2pnodes);
    nodes.Add(rightcsmanodes);
    
    leftcsmanodes.Add(p2pnodes.Get(0));
    rightcsmanodes.Add(p2pnodes.Get(1));

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcp_type));
    // TypeId 
    //     tid = TypeId::LookupByName("ns3::" + tcp_type);
    // Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));
    // Config::Set("/NodeList/1/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));
    // Config::Set("/NodeList/4/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));
    // Config::Set("/NodeList/5/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("10ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));

    NetDeviceContainer p2pdevices = p2phelper.Install(p2pnodes);

    CsmaHelper csmahelper;
    csmahelper.SetChannelAttribute("DataRate", StringValue("10Mbps"));
    csmahelper.SetChannelAttribute("Delay", StringValue("10ms"));

    NetDeviceContainer 
        leftcsmadevices = csmahelper.Install(leftcsmanodes),
        rightcsmadevices = csmahelper.Install(rightcsmanodes);

    // Ptr<RateErrorModel> errmodel = CreateObject<RateErrorModel>();
    // errmodel->SetAttribute("ErrorRate", DoubleValue(0.00001));
    // rightcsmadevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errmodel));

    InternetStackHelper istack;
    istack.Install(nodes);

    Ipv4AddressHelper ipaddr("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer leftinterfaces = ipaddr.Assign(leftcsmadevices);
    ipaddr.NewNetwork();
    Ipv4InterfaceContainer p2pinterfaces = ipaddr.Assign(p2pdevices);
    ipaddr.NewNetwork();
    Ipv4InterfaceContainer rightinterfaces = ipaddr.Assign(rightcsmadevices);

    std::cout << "Ip address of receiver : " << rightinterfaces.GetAddress(1) << '\n';
    BulkSendHelper 
        blkhelper1("ns3::TcpSocketFactory", InetSocketAddress(rightinterfaces.GetAddress(0), 800)),
        blkhelper2("ns3::TcpSocketFactory", InetSocketAddress(rightinterfaces.GetAddress(1), 800));

    PacketSinkHelper sinkhelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));

    ApplicationContainer apps;
    apps.Add(blkhelper1.Install(leftcsmanodes.Get(0)));
    apps.Add(blkhelper2.Install(leftcsmanodes.Get(1)));
    apps.Add(sinkhelper.Install(rightcsmanodes.Get(0)));
    apps.Add(sinkhelper.Install(rightcsmanodes.Get(1)));

    apps.Get(0)->SetStartTime(Seconds(1.0));
    apps.Get(0)->SetStopTime(Seconds(30.0));
    apps.Get(1)->SetStartTime(Seconds(1.0));
    apps.Get(1)->SetStopTime(Seconds(30.0));

    apps.Get(2)->SetStartTime(Seconds(0.0));
    apps.Get(2)->SetStopTime(Seconds(100.0));
    apps.Get(3)->SetStartTime(Seconds(0.0));
    apps.Get(3)->SetStopTime(Seconds(100.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Schedule(Seconds(1.001), TraceCwnd);

    // AnimationInterface anim("../anim.xml");

    p2phelper.EnablePcapAll("p2p");
    csmahelper.EnablePcapAll("csma");

    Simulator::Run();
    Simulator::Destroy();
}