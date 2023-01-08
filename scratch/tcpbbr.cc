#include "ns3/applications-module.h"
#include "ns3/core-module.h"
// #include "ns3/flow-monitor-module.h"
// #include "ns3/gtk-config-store.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
// #include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>
#include <iostream>

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
    Ptr<OutputStreamWrapper> file = asciiTraceHelper.CreateFileStream("tracecwndbbr.csv");
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file));
}

int main()
{
    LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpBbr"));

    NodeContainer nodes(2);

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("5ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("5Mbps"));

    NetDeviceContainer devices = p2phelper.Install(nodes);

    Ptr<RateErrorModel> errmodel = CreateObject<RateErrorModel>();
    errmodel->SetAttribute("ErrorRate", DoubleValue(0.00001));
    devices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errmodel));

    InternetStackHelper istack;
    istack.Install(nodes);

    Ipv4AddressHelper ipaddr("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = ipaddr.Assign(devices);

    std::cout << "Ip address of receiver : " << interfaces.GetAddress(1) << '\n';
    BulkSendHelper blkhelper("ns3::TcpSocketFactory", InetSocketAddress(interfaces.GetAddress(1), 800));

    PacketSinkHelper sinkhelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));

    ApplicationContainer apps;
    apps.Add(sinkhelper.Install(nodes.Get(1)));
    apps.Add(blkhelper.Install(nodes.Get(0)));

    apps.Get(0)->SetStartTime(Seconds(1.0));
    apps.Get(0)->SetStopTime(Seconds(10.0));
    apps.Get(1)->SetStartTime(Seconds(0.0));
    apps.Get(1)->SetStopTime(Seconds(11.0));

    Simulator::Schedule(Seconds(1.001), TraceCwnd);

    AnimationInterface anim("../animbbr.xml");

    p2phelper.EnablePcapAll("trace_bbr");

    Simulator::Run();
    Simulator::Destroy();
}