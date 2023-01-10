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
     tcp cubic                           tcp cubic
n0 -------------\      10.1.2.0     /---------------n4
  (LAN 10.1.3.0) n2----------------n3 (LAN 10.1.3.0)
n1 -------------/   point-to-point  \---------------n5
      tcp bbr                             tcp bbr
*/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpBbrExample");

static void
CwndChange(Ptr<OutputStreamWrapper> file, uint32_t oldval, uint32_t newval)
{
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << newval << '\n';
}

static void 
TraceCwnd()
{
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> filebbr = asciiTraceHelper.CreateFileStream("tracecwndbbr.csv");
    Ptr<OutputStreamWrapper> filecubic = asciiTraceHelper.CreateFileStream("tracecwndcubic.csv");
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, filecubic));
    Config::ConnectWithoutContext("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, filebbr));
}

int main()
{
    bool enable_log = false;

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

    // Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpBbr"));
    TypeId 
        tid_cubic = TypeId::LookupByName("ns3::TcpCubic"),
        tid_bbr = TypeId::LookupByName("ns3::TcpBbr");
    Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid_cubic));
    Config::Set("/NodeList/1/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid_bbr));
    Config::Set("/NodeList/4/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid_cubic));
    Config::Set("/NodeList/5/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid_bbr));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("10ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));

    NetDeviceContainer p2pdevices = p2phelper.Install(p2pnodes);

    CsmaHelper csmahelper;
    csmahelper.SetChannelAttribute("DataRate", StringValue("100Mbps"));
    csmahelper.SetChannelAttribute("Delay", StringValue("1ms"));

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
        blkhelpercubic("ns3::TcpSocketFactory", InetSocketAddress(rightinterfaces.GetAddress(0), 800)),
        blkhelperbbr("ns3::TcpSocketFactory", InetSocketAddress(rightinterfaces.GetAddress(1), 800));

    PacketSinkHelper sinkhelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));

    ApplicationContainer apps;
    apps.Add(blkhelpercubic.Install(leftcsmanodes.Get(0)));
    apps.Add(blkhelperbbr.Install(leftcsmanodes.Get(1)));
    apps.Add(sinkhelper.Install(rightcsmanodes.Get(0)));
    apps.Add(sinkhelper.Install(rightcsmanodes.Get(1)));

    apps.Get(0)->SetStartTime(Seconds(1.0));
    apps.Get(0)->SetStopTime(Seconds(100.0));
    apps.Get(1)->SetStartTime(Seconds(1.0));
    apps.Get(1)->SetStopTime(Seconds(100.0));

    apps.Get(2)->SetStartTime(Seconds(0.0));
    apps.Get(2)->SetStopTime(Seconds(150.0));
    apps.Get(3)->SetStartTime(Seconds(0.0));
    apps.Get(3)->SetStopTime(Seconds(150.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Schedule(Seconds(1.001), TraceCwnd);

    // AnimationInterface anim("../animbbr.xml");

    p2phelper.EnablePcapAll("p2p");
    csmahelper.EnablePcapAll("csma");

    Simulator::Run();
    Simulator::Destroy();
}