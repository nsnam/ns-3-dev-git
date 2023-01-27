#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
// #include "ns3/animation-interface.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("p2p_simul");

static void
CwndChange(Ptr<OutputStreamWrapper> file, uint32_t oldval, uint32_t newval)
{
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << newval << '\n';
}

static void 
TraceCwnd()
{
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> file1 = asciiTraceHelper.CreateFileStream("p2p_cwnd1.csv");
    Ptr<OutputStreamWrapper> file2 = asciiTraceHelper.CreateFileStream("p2p_cwnd2.csv");
    Ptr<OutputStreamWrapper> file3 = asciiTraceHelper.CreateFileStream("p2p_cwnd3.csv");
    Ptr<OutputStreamWrapper> file4 = asciiTraceHelper.CreateFileStream("p2p_cwnd4.csv");
    Config::ConnectWithoutContext("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file1));
    Config::ConnectWithoutContext("/NodeList/3/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file2));
    Config::ConnectWithoutContext("/NodeList/4/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file3));
    Config::ConnectWithoutContext("/NodeList/5/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file4));
}

int
main(int argc, char* argv[])
{
    bool enable_log = false;

    if (enable_log)
    {
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("10ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));

    PointToPointHelper p2pbottleneckhelper;
    p2pbottleneckhelper.SetChannelAttribute("Delay", StringValue("100ms"));
    p2pbottleneckhelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));

    PointToPointDumbbellHelper dumbbellhelper(4, p2phelper, 4, p2phelper, p2pbottleneckhelper);

    InternetStackHelper stack;
    dumbbellhelper.InstallStack(stack);

    Ipv4AddressHelper ipv4left, ipv4bottleneck, ipv4right;
    ipv4left.SetBase("10.1.1.0", "255.255.255.0");
    ipv4bottleneck.SetBase("10.2.1.0", "255.255.255.0");
    ipv4right.SetBase("10.3.1.0", "255.255.255.0");
    
    dumbbellhelper.AssignIpv4Addresses(ipv4left, ipv4right, ipv4bottleneck);

    BulkSendHelper blksender0("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(0), 800));
    BulkSendHelper blksender1("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(1), 800));
    BulkSendHelper blksender2("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(2), 800));
    BulkSendHelper blksender3("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(3), 800));

    ApplicationContainer senderApps;
    senderApps.Add(blksender0.Install(dumbbellhelper.GetLeft(0)));
    senderApps.Add(blksender1.Install(dumbbellhelper.GetLeft(1)));
    senderApps.Add(blksender2.Install(dumbbellhelper.GetLeft(2)));
    senderApps.Add(blksender3.Install(dumbbellhelper.GetLeft(3)));
    
    PacketSinkHelper pktsink0("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));
    PacketSinkHelper pktsink1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));
    PacketSinkHelper pktsink2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));
    PacketSinkHelper pktsink3("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 800));

    ApplicationContainer recvApps;
    recvApps.Add(pktsink0.Install(dumbbellhelper.GetRight(0)));
    recvApps.Add(pktsink1.Install(dumbbellhelper.GetRight(1)));
    recvApps.Add(pktsink2.Install(dumbbellhelper.GetRight(2)));
    recvApps.Add(pktsink3.Install(dumbbellhelper.GetRight(3)));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    senderApps.Start(Seconds(1.0));
    recvApps.Start(Seconds(0.0));

    senderApps.Stop(Seconds(50.0));
    recvApps.Stop(Seconds(100.0));

    // Tracing
    p2pbottleneckhelper.EnablePcap("p2p_simul_router1", dumbbellhelper.GetLeft()->GetDevice(0), false);

    Simulator::Schedule(Seconds(1.001), &TraceCwnd);

    // AnimationInterface anim("../animwifi.xml");

    Simulator::Stop(Seconds(200.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
