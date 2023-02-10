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

int n_nodes = 4;

static void
CwndChange(Ptr<OutputStreamWrapper> file, uint32_t oldval, uint32_t newval)
{
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << newval << '\n';
}

static void
TraceCwnd()
{
    AsciiTraceHelper asciiTraceHelper;
    for (int i = 0; i < n_nodes; i++)
    {
        Ptr<OutputStreamWrapper> file =
            asciiTraceHelper.CreateFileStream("p2p_cwnd" + std::to_string(i) + ".csv");
        Config::ConnectWithoutContext(
            "/NodeList/" + std::to_string(i+2) + "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
            MakeBoundCallback(&CwndChange, file));
    }
}

int
main(int argc, char* argv[])
{
    std::string tcp_mode = "TcpCubic";
    int runtime = 50; //Seconds
    bool enable_log = false;

    if (enable_log)
    {
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcp_mode));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("50ns"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("11Mbps"));

    PointToPointHelper p2pbottleneckhelper;
    p2pbottleneckhelper.SetChannelAttribute("Delay", StringValue("1us"));
    p2pbottleneckhelper.SetDeviceAttribute("DataRate", StringValue("1Mbps"));

    
    PointToPointDumbbellHelper dumbbellhelper(n_nodes, p2phelper, n_nodes, p2phelper, p2pbottleneckhelper);

    InternetStackHelper stack;
    dumbbellhelper.InstallStack(stack);

    Ipv4AddressHelper ipv4left, ipv4bottleneck, ipv4right;
    ipv4left.SetBase("10.1.1.0", "255.255.255.0");
    ipv4bottleneck.SetBase("10.2.1.0", "255.255.255.0");
    ipv4right.SetBase("10.3.1.0", "255.255.255.0");
    
    dumbbellhelper.AssignIpv4Addresses(ipv4left, ipv4right, ipv4bottleneck);

    ApplicationContainer senderApps;
    for (int i = 0; i < n_nodes; i++)
    {
        BulkSendHelper blksender("ns3::TcpSocketFactory",
                                 InetSocketAddress(dumbbellhelper.GetRightIpv4Address(i), 800));
        senderApps.Add(blksender.Install(dumbbellhelper.GetLeft(i)));
    }

    ApplicationContainer recvApps;
    for (int i = 0; i < n_nodes; i++)
    {
        PacketSinkHelper pktsink("ns3::TcpSocketFactory",
                                 InetSocketAddress(Ipv4Address::GetAny(), 800));
        recvApps.Add(pktsink.Install(dumbbellhelper.GetRight(i)));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    senderApps.Start(Seconds(1.0));
    recvApps.Start(Seconds(0.0));

    senderApps.Stop(Seconds(runtime));
    recvApps.Stop(Seconds(runtime + 20));

    // Tracing
    p2pbottleneckhelper.EnablePcap("p2p_simul", dumbbellhelper.GetLeft()->GetDevice(0), false);

    Simulator::Schedule(Seconds(1.001), &TraceCwnd);

    // AnimationInterface anim("../animwifi.xml");

    Simulator::Stop(Seconds(runtime + 30));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
