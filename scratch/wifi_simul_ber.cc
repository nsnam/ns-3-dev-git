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

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"


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
    Ptr<OutputStreamWrapper> file1 = asciiTraceHelper.CreateFileStream("wifi_cubic_1.csv");
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file1));
    Ptr<OutputStreamWrapper> file2 = asciiTraceHelper.CreateFileStream("wifi_cubic_2.csv");
    Config::ConnectWithoutContext("/NodeList/1/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow", MakeBoundCallback(&CwndChange, file2));
}

class PhyTraceCounter
{
public:
  PhyTraceCounter () : total_Packets (0), dropped_Packets (0) {}

  void PhyTxTrace (Ptr<const Packet> packet, double snr)
  {
    total_Packets++;
  }
    void PhyRxDropTrace (Ptr<const Packet> packet, ns3::WifiPhyRxfailureReason reason)
  {
    dropped_Packets++;
    std::cout<<reason<<std::endl;
  }

  uint32_t GetTotalPackets () const { return total_Packets; }
  uint32_t GetDroppedPackets () const { return dropped_Packets; }

private:
  uint32_t total_Packets,dropped_Packets;
};

int
main(int argc, char* argv[])
{
    std::string phyMode("DsssRate11Mbps");
    double rss = -80;           // -dBm
    bool enable_log = false;

    int n_nodes=2;

    if (enable_log)
    {
        LogComponentEnable("BulkSendApplication", LOG_LEVEL_INFO);
        LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
    }

    // Fix non-unicast data rate to be the same as that of unicast
    //Unicast is Multi-cast or broadcast, and WifiMode(PhyMode) is setting rate for all control frames 
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));

    PointToPointHelper p2phelper;
    p2phelper.SetChannelAttribute("Delay", StringValue("10ms"));
    p2phelper.SetDeviceAttribute("DataRate", StringValue("100Mbps"));

    PointToPointHelper p2pbottleneckhelper;
    p2pbottleneckhelper.SetChannelAttribute("Delay", StringValue("100ms"));
    p2pbottleneckhelper.SetDeviceAttribute("DataRate", StringValue("1Mbps"));

    NodeContainer leftwifinodes;
    leftwifinodes.Create(n_nodes);
    PointToPointDumbbellHelper dumbbellhelper(0, p2phelper, n_nodes, p2phelper, p2pbottleneckhelper);

    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211b);
    // wifi.EnableLogComponents(); // Turn on all Wifi logging
    YansWifiPhyHelper wifiPhy;
    // This is one parameter that matters when using FixedRssLossModel
    // set it to zero; otherwise, gain will be added
    wifiPhy.Set("RxGain", DoubleValue(0));
    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    //SetPcapDataLinkType - Set the data link type of PCAP traces to be used.
    //This function has to be called before EnablePcap(), so that the header of the pcap file can be written correctly.
    //DLT_IEEE802_11_RADIO - Include Radiotap link layer information.
    //For more supported type check - SupportedPcapDataLinkTypes
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
    //SSID stands for service set identifier.
    //SSID is simply the technical term for a Wi-Fi network name.
    //When you set up a wireless home network, you give it a name to distinguish it from other networks in your neighbourhood.
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
    // positionAlloc->Add(Vector(20.0, 0.0, 0.0));
    // positionAlloc->Add(Vector(30.0, 0.0, 0.0));
    positionAlloc->Add(Vector(5.0, 10.0, 0.0));
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

    senderApps.Start(Seconds(0.1));
    recvApps.Start(Seconds(0.0));

    senderApps.Stop(Seconds(50.0));
    recvApps.Stop(Seconds(100.0));

    // Tracing
    wifiPhy.EnablePcap("wifi_simul", devices);
    PhyTraceCounter phyTraceCounter;
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/Phy/PhyTxBegin", MakeCallback (&PhyTraceCounter::PhyTxTrace, &phyTraceCounter));  
    
    Config::ConnectWithoutContext ("/NodeList/*/DeviceList/*/Phy/PhyRxDrop", MakeCallback (&PhyTraceCounter::PhyRxDropTrace, &phyTraceCounter));
    
    Simulator::Schedule(Seconds(1.001), &TraceCwnd);

    AnimationInterface anim("../animwifi.xml");

    Simulator::Stop(Seconds(200.0));
    Simulator::Run();
    std::cout << "Total packets sent through PHY layer: " << phyTraceCounter.GetTotalPackets () << std::endl;
    std::cout << "Total packets dropped in PHY layer: " << phyTraceCounter.GetDroppedPackets () << std::endl;

    NodeContainer nodes = dumbbellhelper.GetLeft();
    for (uint32_t i = 0; i < nodes.GetN(); i++)
    {
      Ptr<Node> node = nodes.Get(i);
      std::cout << "Node ID: " << node->GetId() << std::endl;
    }

    Simulator::Destroy();

    return 0;
}
