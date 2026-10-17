/*
 * Copyright (c) 2005,2006,2007 INRIA
 * Copyright (c) 2016 Magister Solutions
 * Copyright (c) 2025 Tom Henderson
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Borrows from examples/wireless/wifi-ap.cc for overall Wi-Fi network setup pattern
 * Borrows from src/applications/examples/three-gpp-http-example.cc for overall simulation flow
 */

/**
 * @file rta-tig-mobile-gaming-example.cc
 * @brief A simple real-time mobile gaming traffic generator example over Wi-Fi
 *
 * This example demonstrates how to set up a basic ns-3 simulation with real-time
 * mobile gaming traffic over a Wi-Fi network. Real-time mobile gaming is characterized
 * by small packets (30-500 bytes) sent frequently (every 30-60ms) with low latency
 * requirements.
 *
 * The simulation consists of:
 * - A simple Wi-Fi network with one AP (Access Point) and one STA (Station)
 * - Gaming traffic with three stages: Initial, Gaming, and Ending
 * - Application-level tracing to observe gaming packets and stage transitions
 *
 * The traffic model follows IEEE 802.11-18/2009r6 RTA TIG Report (Section 4.1.4):
 * - Two synchronization mechanisms: Status Sync and Frame Lockstep Sync
 * - Packet sizes and arrivals follow Largest Extreme Value distribution
 * - Initial and end packets follow Uniform distribution
 *
 * Traffic model presets:
 * - Status Sync DL: a=13ms, b=3.7ms arrivals; a=50, b=11 bytes sizes
 * - Status Sync UL: a=15ms, b=5.7ms arrivals; a=38, b=3.7 bytes sizes
 * - Lockstep DL: a=28ms, b=4.2ms arrivals; a=210, b=35 bytes sizes
 * - Lockstep UL: a=22ms, b=3.4ms arrivals; a=92, b=38 bytes sizes
 *
 * To run with default (status-sync DL): ./ns3 run rta-tig-mobile-gaming-example
 * To run status-sync UL: ./ns3 run "rta-tig-mobile-gaming-example --model=status-sync-ul"
 * To run lockstep DL: ./ns3 run "rta-tig-mobile-gaming-example --model=lockstep-dl"
 * To run lockstep UL: ./ns3 run "rta-tig-mobile-gaming-example --model=lockstep-ul"
 *
 * To disable verbose logging: ./ns3 run "rta-tig-mobile-gaming-example --verbose=false"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RtaTigMobileGamingExample");

/**
 * Callback invoked when a gaming packet is transmitted
 * @param context The context string
 * @param packet The transmitted packet
 * @param stage The traffic model stage
 */
void GamingPacketSent(std::string context,
                      Ptr<const Packet> packet,
                      RtaTigMobileGaming::TrafficModelStage stage);

/**
 * Callback invoked when the PacketSink receives a packet
 * @param context The context string
 * @param packet The received packet
 * @param address The sender's address
 */
void PacketReceived(std::string context, Ptr<const Packet> packet, const Address& address);

int
main(int argc, char* argv[])
{
    Time duration{Seconds(10)};          // Simulation time in seconds
    std::string model{"status-sync-dl"}; // Traffic model preset
    bool verbose = true;                 // Enable/disable verbose logging

    CommandLine cmd(__FILE__);
    cmd.Usage("Real-time mobile gaming traffic example");
    cmd.AddValue("duration", "Duration of traffic flow, in seconds", duration);
    cmd.AddValue("model",
                 "Traffic model preset (status-sync-dl, status-sync-ul, lockstep-dl, lockstep-ul). "
                 "Default: status-sync-dl",
                 model);
    cmd.AddValue("verbose",
                 "Enable verbose logging of RtaTigMobileGaming, PacketSink, and this program",
                 verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable(
            "RtaTigMobileGamingExample",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_ALL));
        LogComponentEnable(
            "RtaTigMobileGaming",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_INFO));
        LogComponentEnable(
            "PacketSink",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_INFO));
    }

    NodeContainer wifiNodes;
    wifiNodes.Create(2); // Create 2 nodes: one will be AP, one will be STA

    Ptr<Node> apNode = wifiNodes.Get(0);
    Ptr<Node> staNode = wifiNodes.Get(1);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    NetDeviceContainer apDevices;
    NetDeviceContainer staDevices;

    // Configure AP
    Ssid ssid = Ssid("gaming-network");
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, apNode);

    // Configure STA
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    staDevices = wifi.Install(phy, mac, staNode);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(5.0),
                                  "DeltaY",
                                  DoubleValue(5.0),
                                  "GridWidth",
                                  UintegerValue(2),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wifiNodes);

    InternetStackHelper internet;
    internet.Install(wifiNodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");

    NetDeviceContainer allDevices;
    allDevices.Add(apDevices);
    allDevices.Add(staDevices);
    Ipv4InterfaceContainer ipv4Interfaces = ipv4.Assign(allDevices);

    NS_LOG_INFO("AP address: " << ipv4Interfaces.GetAddress(0));
    NS_LOG_INFO("STA address: " << ipv4Interfaces.GetAddress(1));

    // Gaming uses UDP as specified
    std::string protocolFactory = "ns3::UdpSocketFactory";
    uint16_t port = 5000;

    ApplicationHelper sourceHelper(RtaTigMobileGaming::GetTypeId());
    sourceHelper.SetAttribute("Protocol", TypeIdValue(TypeId::LookupByName(protocolFactory)));

    // Configure traffic model parameters based on preset
    // Parameters from IEEE 802.11-18/2009r6 Tables 4-3 and 4-4
    Ptr<Node> sourceNode;
    Ptr<Node> sinkNode;
    std::string remoteAddress;

    if (model == "status-sync-dl")
    {
        // Downlink: AP -> STA
        sourceNode = apNode;
        sinkNode = staNode;
        remoteAddress = "10.1.1.2";

        // Status-sync DL parameters (defaults)
        // Initial: [0, 20], End: [500, 600]
        // Arrivals: LEV(13ms, 3.7ms), Sizes: LEV(50, 11)
        NS_LOG_INFO("Using Status-Sync Downlink model (default parameters)");
    }
    else if (model == "status-sync-ul")
    {
        // Uplink: STA -> AP
        sourceNode = staNode;
        sinkNode = apNode;
        remoteAddress = "10.1.1.1";

        // Status-sync UL parameters
        auto ips = CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                     DoubleValue(0),
                                                                     "Max",
                                                                     DoubleValue(20));
        sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

        auto eps = CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                     DoubleValue(400),
                                                                     "Max",
                                                                     DoubleValue(550));
        sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

        auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
            "Location",
            DoubleValue(15000), // 15ms in us
            "Scale",
            DoubleValue(5700)); // 5.7ms in us
        sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));

        auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>("Location",
                                                                                 DoubleValue(38),
                                                                                 "Scale",
                                                                                 DoubleValue(3.7));
        sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

        NS_LOG_INFO("Using Status-Sync Uplink model");
    }
    else if (model == "lockstep-dl")
    {
        // Downlink: AP -> STA
        sourceNode = apNode;
        sinkNode = staNode;
        remoteAddress = "10.1.1.2";

        // Lockstep DL parameters
        auto ips = CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                     DoubleValue(0),
                                                                     "Max",
                                                                     DoubleValue(80));
        sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

        auto eps = CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                     DoubleValue(1400),
                                                                     "Max",
                                                                     DoubleValue(1500));
        sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

        auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
            "Location",
            DoubleValue(28000), // 28ms in us
            "Scale",
            DoubleValue(4200)); // 4.2ms in us
        sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));

        auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>("Location",
                                                                                 DoubleValue(210),
                                                                                 "Scale",
                                                                                 DoubleValue(35));
        sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

        NS_LOG_INFO("Using Frame Lockstep Downlink model");
    }
    else if (model == "lockstep-ul")
    {
        // Uplink: STA -> AP
        sourceNode = staNode;
        sinkNode = apNode;
        remoteAddress = "10.1.1.1";

        // Lockstep UL parameters
        auto ips = CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                     DoubleValue(0),
                                                                     "Max",
                                                                     DoubleValue(80));
        sourceHelper.SetAttribute("InitialPacketSize", PointerValue(ips));

        auto eps = CreateObjectWithAttributes<UniformRandomVariable>("Min",
                                                                     DoubleValue(500),
                                                                     "Max",
                                                                     DoubleValue(600));
        sourceHelper.SetAttribute("EndPacketSize", PointerValue(eps));

        auto pal = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>(
            "Location",
            DoubleValue(22000), // 22ms in us
            "Scale",
            DoubleValue(3400)); // 3.4ms in us
        sourceHelper.SetAttribute("PacketArrivalLev", PointerValue(pal));

        auto psl = CreateObjectWithAttributes<LargestExtremeValueRandomVariable>("Location",
                                                                                 DoubleValue(92),
                                                                                 "Scale",
                                                                                 DoubleValue(38));
        sourceHelper.SetAttribute("PacketSizeLev", PointerValue(psl));

        NS_LOG_INFO("Using Frame Lockstep Uplink model");
    }
    else
    {
        NS_FATAL_ERROR(
            "Invalid model: "
            << model
            << ". Use 'status-sync-dl', 'status-sync-ul', 'lockstep-dl', or 'lockstep-ul'.");
    }

    Address remoteAddr = InetSocketAddress(Ipv4Address(remoteAddress.c_str()), port);
    sourceHelper.SetAttribute("Remote", AddressValue(remoteAddr));

    auto sourceApps = sourceHelper.Install(sourceNode);
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(1.0) + duration);

    PacketSinkHelper sinkHelper(protocolFactory, InetSocketAddress(Ipv4Address::GetAny(), port));
    auto sinkApps = sinkHelper.Install(sinkNode);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(2.0) + duration);

    // Connect to gaming TX with stage trace
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::RtaTigMobileGaming/TxWithStage",
                    MakeCallback(&GamingPacketSent));

    // Connect to RX trace
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&PacketReceived));

    NS_LOG_INFO("Starting simulation for traffic duration: " << duration.As(Time::S));
    NS_LOG_INFO("Traffic model: " << model);

    Simulator::Stop(Seconds(2) + duration);
    Simulator::Run();

    // Get statistics from packet sink
    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(0));
    if (sink)
    {
        uint32_t bytesRx = sink->GetTotalRx();
        NS_LOG_INFO("Total bytes received: " << bytesRx);
    }

    Simulator::Destroy();

    return 0;
}

void
GamingPacketSent(std::string context,
                 Ptr<const Packet> packet,
                 RtaTigMobileGaming::TrafficModelStage stage)
{
    NS_LOG_INFO("Gaming TX [" << context << "]: Packet size (bytes): " << packet->GetSize()
                              << " Stage: " << stage);
}

void
PacketReceived(std::string context, Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Packet RX [" << context << "]: Size(bytes): " << packet->GetSize());
}
