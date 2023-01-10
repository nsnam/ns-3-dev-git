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

       point to point
    n0 -------------- n1
          tcp cubic 
*/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TcpP2PExample");

static void
CwndChange(Ptr<OutputStreamWrapper> file, uint32_t oldval, uint32_t newval)
{
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << newval << '\n';
}

static void 
TraceCwnd()
{
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> file = asciiTraceHelper.CreateFileStream("cwndp2p.csv");
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file));
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

    NodeContainer p2pnodes(2);

    // TypeId tid = TypeId::LookupByName("ns3::" + tcp_type);
    // Config::Set("/NodeList/0/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));
    // Config::Set("/NodeList/1/$ns3::TcpL4Protocol/SocketType", TypeIdValue(tid));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcp_type));
    Config::SetDefault("ns3::TcpSocketBase::Sack", BooleanValue(false));
    // Config::SetDefault("ns3::FifoQueueDisc::MaxSize", QueueSizeValue(QueueSize("10p")));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("10ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    // p2phelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("10p"));

    NetDeviceContainer p2pdevices = p2phelper.Install(p2pnodes);

    // Ptr<RateErrorModel> errmodel = CreateObject<RateErrorModel>();
    // errmodel->SetAttribute("ErrorRate", DoubleValue(0.00001));
    // errmodel->SetUnit(RateErrorModel::ERROR_UNIT_BYTE);
    // p2pdevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errmodel));

    InternetStackHelper istack;
    istack.Install(p2pnodes);

    Ipv4AddressHelper ipaddr("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer p2pinterfaces = ipaddr.Assign(p2pdevices);

    BulkSendHelper 
        blkhelpercubic("ns3::TcpSocketFactory", InetSocketAddress(p2pinterfaces.GetAddress(1), 800));

    PacketSinkHelper sinkhelper("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));

    ApplicationContainer apps;
    apps.Add(blkhelpercubic.Install(p2pnodes.Get(0)));
    apps.Add(sinkhelper.Install(p2pnodes.Get(1)));

    apps.Get(0)->SetStartTime(Seconds(1.0));
    apps.Get(0)->SetStopTime(Seconds(30.0));

    apps.Get(1)->SetStartTime(Seconds(0.0));
    apps.Get(1)->SetStopTime(Seconds(50.0));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Schedule(Seconds(1.001), TraceCwnd);

    // AnimationInterface anim("../animbbr.xml");

    p2phelper.EnablePcapAll("p2p");

    Simulator::Run();
    Simulator::Destroy();
}