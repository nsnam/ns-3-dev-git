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

#include "ns3/animation-interface.h"
#include "ns3/applications-module.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/point-to-point-dumbbell.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/command-line.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Wifi_simul");

int n_nodes = 1;
int total_Packets = 0, dropped_Packets = 0;

static void
PhyRxBeginTrace(Ptr<const Packet> packet, RxPowerWattPerChannelBand rxPowersW)
{
    total_Packets++;
}

static void
PhyRxDropTrace(Ptr<const Packet> packet, ns3::WifiPhyRxfailureReason reason)
{
    dropped_Packets++;
    std::cout << reason << std::endl;
}

static void
GoodputChange(Ptr<OutputStreamWrapper> file, Ptr<PacketSink> sink1, double prevBytesThrough)
{
    double recvBytes = sink1->GetTotalRx();
    double throughput = ((recvBytes - prevBytesThrough) * 8);
    *file->GetStream() << Simulator::Now().GetSeconds() << "," << throughput << std::endl;
    Simulator::Schedule(MilliSeconds(1.0), &GoodputChange, file, sink1, recvBytes);
}

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
            asciiTraceHelper.CreateFileStream("wifi_cwnd" + std::to_string(i) + ".csv");
        Config::ConnectWithoutContext("/NodeList/" + std::to_string(i) +
                                          "/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                      MakeBoundCallback(&CwndChange, file));
    }
}

static void
TraceGoodput(ApplicationContainer* apps)
{
    AsciiTraceHelper asciiTraceHelper;
    for (int i = 0; i < n_nodes; i++)
    {
        Ptr<OutputStreamWrapper> file =
            asciiTraceHelper.CreateFileStream("wifi_goodput" + std::to_string(i) + ".csv");
        Simulator::Schedule(
            MilliSeconds(1.0),
            MakeBoundCallback(&GoodputChange, file, apps->Get(i)->GetObject<PacketSink>(), 0.0));
    }
}

void
TraceDropRatio()
{
    Config::ConnectWithoutContext(
        "/NodeList/" + std::to_string(n_nodes) + "/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxBegin",
        MakeBoundCallback(&PhyRxBeginTrace));
    Config::ConnectWithoutContext(
        "/NodeList/" + std::to_string(n_nodes) + "/DeviceList/1/$ns3::WifiNetDevice/Phy/$ns3::WifiPhy/PhyRxDrop",
        MakeBoundCallback(&PhyRxDropTrace));
}

int
main(int argc, char* argv[])
{
    std::string phyMode("DsssRate11Mbps");
    std::string tcp_mode = "TcpCubic";
    int runtime = 50; // Seconds
    double rss = -80; // -dBm
    bool enable_log = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("numnodes", "number of senders in simulation", n_nodes);
    cmd.AddValue("logging", "turn on all Application log components", enable_log);
    cmd.AddValue("tcpmode", "specify the type of tcp socket", tcp_mode);
    cmd.AddValue("runtime", "Time (in Seconds) sender applications run", runtime);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.AddValue("rss", "received signal strength", rss);
    cmd.Parse(argc, argv);

    if (enable_log)
    {
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcp_mode));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("50ns"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("11Mbps"));

    PointToPointHelper p2pbottleneckhelper;
    p2pbottleneckhelper.SetChannelAttribute("Delay", StringValue("1us"));
    p2pbottleneckhelper.SetDeviceAttribute("DataRate", StringValue("1Mbps"));

    NodeContainer leftwifinodes(n_nodes);
    PointToPointDumbbellHelper dumbbellhelper(0,
                                              p2phelper,
                                              n_nodes,
                                              p2phelper,
                                              p2pbottleneckhelper);

    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
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
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(2.0),
                                  "DeltaY",
                                  DoubleValue(2.0),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    // mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
    //                           "Bounds",
    //                           RectangleValue(Rectangle(-50, 50, -50, 50)));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(leftwifinodes);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(dumbbellhelper.GetLeft());

    Ptr<MobilityModel> mobModel = dumbbellhelper.GetLeft()->GetObject<MobilityModel>();
    mobModel->SetPosition(Vector(10, 10, 0));

    std::cout << "Delay of wifi nodes = " << std::sqrt(200) * 1000000000.0 / 299792458 << "ns"
              << '\n';

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

    ApplicationContainer senderApps;
    for (int i = 0; i < n_nodes; i++)
    {
        BulkSendHelper blksender("ns3::TcpSocketFactory",
                                 InetSocketAddress(dumbbellhelper.GetRightIpv4Address(i), 800));
        senderApps.Add(blksender.Install(leftwifinodes.Get(i)));
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
    wifiPhy.EnablePcap("wifi_simul", apDevice);

    Simulator::Schedule(Seconds(1.001), &TraceCwnd);
    Simulator::Schedule(Seconds(1.001), MakeBoundCallback(&TraceGoodput, &recvApps));
    Simulator::Schedule(Seconds(1.001), &TraceDropRatio);

    // AnimationInterface anim("../animwifi.xml");
    Simulator::Stop(Seconds(runtime + 30));
    Simulator::Run();

    std::cout << "Ratio of dropped packets on AP: " << dropped_Packets * 1.0 / total_Packets
              << '\n';

    Simulator::Destroy();

    return 0;
}
