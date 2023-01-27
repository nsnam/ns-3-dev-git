// This script configures two nodes on an 802.11b physical layer, with
// 802.11b NICs in infrastructure mode, and by default, the station sends
// one packet of 1000 (application) bytes to the access point.  Unlike
// the default physical layer configuration in which the path loss increases
// (and the received signal strength decreases) as the distance between the
// nodes increases, this example uses an artificial path loss model that
// allows the configuration of the received signal strength (RSS) regardless
// of other transmitter parameters (such as transmit power) or distance.
// Therefore, changing position of the nodes has no effect.
//
// There are a number of command-line options available to control
// the default behavior.  The list of available command-line options
// can be listed with the following command:
// ./ns3 run "wifi-simple-infra --help"
// Additional command-line options are available via the generic attribute
// configuration system.
//
// For instance, for the default configuration, the physical layer will
// stop successfully receiving packets when rss drops to -82 dBm or below.
// To see this effect, try running:
//
// ./ns3 run "wifi-simple-infra --rss=-80
// ./ns3 run "wifi-simple-infra --rss=-81
// ./ns3 run "wifi-simple-infra --rss=-82
//
// The last command (and any RSS value lower than this) results in no
// packets received.  This is due to the preamble detection model that
// dominates the reception performance.  By default, the
// ThresholdPreambleDetectionModel is added to all WifiPhy objects, and this
// model prevents reception unless the incoming signal has a RSS above its
// 'MinimumRssi' value (default of -82 dBm) and has a SNR above the
// 'Threshold' value (default of 4).
//
// If we relax these values, we can instead observe that signal reception
// due to the 802.11b error model alone is much lower.  For instance,
// setting the MinimumRssi to -101 (around the thermal noise floor).
// and the SNR Threshold to -10 dB, shows that the DsssErrorRateModel can
// successfully decode at RSS values of -97 or -98 dBm.
//
// ./ns3 run "wifi-simple-infra --rss=-97 --numPackets=20
// --ns3::ThresholdPreambleDetectionModel::Threshold=-10
// --ns3::ThresholdPreambleDetectionModel::MinimumRssi=-101"
// ./ns3 run "wifi-simple-infra --rss=-98 --numPackets=20
// --ns3::ThresholdPreambleDetectionModel::Threshold=-10
// --ns3::ThresholdPreambleDetectionModel::MinimumRssi=-101"
// ./ns3 run "wifi-simple-infra --rss=-99 --numPackets=20
// --ns3::ThresholdPreambleDetectionModel::Threshold=-10
// --ns3::ThresholdPreambleDetectionModel::MinimumRssi=-101"

#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/bulk-send-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/animation-interface.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Wifi_simul");

static void
CwndChange(Ptr<OutputStreamWrapper> file, uint32_t oldval, uint32_t newval)
{
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << newval << '\n';
}

static void 
TraceCwnd()
{
    AsciiTraceHelper asciiTraceHelper;
    Ptr<OutputStreamWrapper> file1 = asciiTraceHelper.CreateFileStream("wifi_cwnd1.csv");
    Ptr<OutputStreamWrapper> file2 = asciiTraceHelper.CreateFileStream("wifi_cwnd2.csv");
    Ptr<OutputStreamWrapper> file3 = asciiTraceHelper.CreateFileStream("wifi_cwnd3.csv");
    Ptr<OutputStreamWrapper> file4 = asciiTraceHelper.CreateFileStream("wifi_cwnd4.csv");
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file1));
    Config::ConnectWithoutContext("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file2));
    Config::ConnectWithoutContext("/NodeList/2/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file3));
    Config::ConnectWithoutContext("/NodeList/3/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file4));
}

int
main(int argc, char* argv[])
{
    std::string phyMode("DsssRate1Mbps");
    double rss = -80;           // -dBm
    bool enable_log = false;

    if (enable_log)
    {
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("10ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));

    PointToPointHelper p2pbottleneckhelper;
    p2pbottleneckhelper.SetChannelAttribute("Delay", StringValue("100ms"));
    p2pbottleneckhelper.SetDeviceAttribute("DataRate", StringValue("10Mbps"));

    NodeContainer leftwifinodes(4);
    PointToPointDumbbellHelper dumbbellhelper(0, p2phelper, 4, p2phelper, p2pbottleneckhelper);

    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    // wifi.EnableLogComponents(); // Turn on all Wifi logging

    YansWifiPhyHelper wifiPhy;
    // This is one parameter that matters when using FixedRssLossModel
    // set it to zero; otherwise, gain will be added
    wifiPhy.Set("RxGain", DoubleValue(0));
    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    // The below FixedRssLossModel will cause the rss to be fixed regardless
    // of the distance between the two stations, and the transmit power
    wifiChannel.AddPropagationLoss("ns3::FixedRssLossModel", "Rss", DoubleValue(rss));
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));

    // Setup the rest of the MAC
    Ssid ssid = Ssid("wifi-default");
    // setup STA
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer staDevices = wifi.Install(wifiPhy, wifiMac, leftwifinodes);
    NetDeviceContainer devices = staDevices;
    // setup AP
    wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, dumbbellhelper.GetLeft());
    devices.Add(apDevice);

    // Note that with FixedRssLossModel, the positions below are not
    // used for received signal strength.
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));
    positionAlloc->Add(Vector(20.0, 0.0, 0.0));
    positionAlloc->Add(Vector(30.0, 0.0, 0.0));
    positionAlloc->Add(Vector(15.0, 10.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    NodeContainer wifinodes;
    wifinodes.Add(leftwifinodes);
    wifinodes.Add(dumbbellhelper.GetLeft());
    mobility.Install(wifinodes);

    InternetStackHelper stack;
    dumbbellhelper.InstallStack(stack);
    stack.Install(leftwifinodes);

    Ipv4AddressHelper ipv4left, ipv4bottleneck, ipv4right;
    ipv4left.SetBase("10.1.1.0", "255.255.255.0");
    ipv4bottleneck.SetBase("10.1.2.0", "255.255.255.0");
    ipv4right.SetBase("10.1.3.0", "255.255.255.0");
    
    Ipv4InterfaceContainer leftwifiinterfaces = ipv4left.Assign(staDevices);
    ipv4left.Assign(apDevice);
    dumbbellhelper.AssignIpv4Addresses(ipv4left, ipv4right, ipv4bottleneck);

    BulkSendHelper blksender0("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(0), 800));
    BulkSendHelper blksender1("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(1), 800));
    BulkSendHelper blksender2("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(2), 800));
    BulkSendHelper blksender3("ns3::TcpSocketFactory", InetSocketAddress(dumbbellhelper.GetRightIpv4Address(3), 800));

    ApplicationContainer senderApps = blksender0.Install(leftwifinodes.Get(0));
    senderApps.Add(blksender1.Install(leftwifinodes.Get(1)));
    senderApps.Add(blksender2.Install(leftwifinodes.Get(2)));
    senderApps.Add(blksender3.Install(leftwifinodes.Get(3)));
    
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

    senderApps.Stop(Seconds(100.0));
    recvApps.Stop(Seconds(150.0));

    // Tracing
    wifiPhy.EnablePcap("wifi_simul", devices);

    Simulator::Schedule(Seconds(1.001), &TraceCwnd);

    AnimationInterface anim("../animwifi.xml");

    Simulator::Stop(Seconds(200.0));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
