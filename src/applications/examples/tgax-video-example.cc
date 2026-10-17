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
 * @file tgax-video-example.cc
 * @brief A simple buffered video streaming traffic generator example over Wi-Fi
 *
 * This example demonstrates how to set up a basic ns-3 simulation with video streaming
 * traffic over a Wi-Fi network. It includes support for:
 *
 * 1. Buffered video streaming (BV1-BV6): Different bit rates from 2Mbps to 15.6Mbps
 * 2. Multicast video streaming (MC1-MC2): 3Mbps and 6Mbps multicast
 * 3. Custom video parameters: User-defined Weibull parameters and bit rate
 *
 * The simulation consists of:
 * - A simple Wi-Fi network with one AP (Access Point) and one or more STAs (Stations)
 * - Video traffic flowing from the AP to the STA(s)
 * - Application-level tracing to observe video frames being generated and sent
 *
 * The video traffic model follows IEEE 802.11-14/0571r12 TGAX evaluation methodology:
 * - Video frame sizes are drawn from a Weibull distribution
 * - Network latency is modeled using a Gamma distribution (mean ~14.8ms)
 * - Multiple predefined profiles for different video quality levels
 *
 * To run with default settings (BV1, 2Mbps): ./ns3 run tgax-video-example
 * To run with higher bit rate: ./ns3 run "tgax-video-example --model=BV3"
 * To run with multicast: ./ns3 run "tgax-video-example --model=MC1"
 * To run with TCP: ./ns3 run "tgax-video-example --protocol=tcp"
 *
 * To disable verbose logging: ./ns3 run "tgax-video-example --verbose=false"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TgaxVideoExample");

/**
 * Callback invoked when a video packet is transmitted
 * @param packet The transmitted packet
 * @param latency The network latency applied to the packet
 */
void VideoPacketSent(Ptr<const Packet> packet, Time latency);

/**
 * Callback invoked when a video frame is generated
 * @param frameSize The size of the generated frame in bytes
 */
void VideoFrameGenerated(uint32_t frameSize);

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
    Time duration{Seconds(10)};  // Simulation time in seconds
    std::string model{"BV1"};    // Traffic model: BV1-BV6, MC1-MC2, or Custom
    std::string protocol{"udp"}; // Protocol: udp or tcp
    bool verbose = true;         // Enable/disable verbose logging

    CommandLine cmd(__FILE__);
    cmd.Usage("Buffered video streaming example");
    cmd.AddValue("duration", "Duration of traffic flow, in seconds", duration);
    cmd.AddValue("model",
                 "Traffic model to use (BV1, BV2, BV3, BV4, BV5, BV6, MC1, MC2, or Custom). "
                 "Default: BV1",
                 model);
    cmd.AddValue("protocol", "Protocol to use (udp or tcp). Default: udp", protocol);
    cmd.AddValue("verbose",
                 "Enable verbose logging of TgaxVideoTraffic, PacketSink, and this program",
                 verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable(
            "TgaxVideoExample",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_ALL));
        LogComponentEnable(
            "TgaxVideoTraffic",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_INFO));
        LogComponentEnable(
            "PacketSink",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_INFO));
    }

    // Determine if this is a multicast scenario
    bool isMulticast = (model == "MC1" || model == "MC2");
    uint32_t numStations = isMulticast ? 2 : 1;

    NodeContainer wifiNodes;
    wifiNodes.Create(1 + numStations); // AP + STAs

    Ptr<Node> apNode = wifiNodes.Get(0);
    NodeContainer staNodes;
    for (uint32_t i = 0; i < numStations; ++i)
    {
        staNodes.Add(wifiNodes.Get(1 + i));
    }

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    NetDeviceContainer apDevices;
    NetDeviceContainer staDevices;

    // Configure AP
    Ssid ssid = Ssid("video-network");
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    apDevices = wifi.Install(phy, mac, apNode);

    // Configure STAs
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    staDevices = wifi.Install(phy, mac, staNodes);

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
                                  UintegerValue(3),
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
    for (uint32_t i = 0; i < numStations; ++i)
    {
        NS_LOG_INFO("STA " << i << " address: " << ipv4Interfaces.GetAddress(1 + i));
    }

    std::string protocolFactory;
    if (protocol == "udp")
    {
        protocolFactory = "ns3::UdpSocketFactory";
    }
    else if (protocol == "tcp")
    {
        protocolFactory = "ns3::TcpSocketFactory";
    }
    else
    {
        NS_FATAL_ERROR("Invalid protocol: " << protocol << ". Use 'udp' or 'tcp'.");
    }

    uint16_t remotePort = 5000;
    std::string remoteAddress;
    if (isMulticast)
    {
        // For multicast, use a multicast address
        remoteAddress = "239.192.100.1";

        // Add multicast route on AP
        Ipv4StaticRoutingHelper staticRouting;
        Ptr<Ipv4> ipv4Proto = apNode->GetObject<Ipv4>();
        Ptr<Ipv4StaticRouting> routing = staticRouting.GetStaticRouting(ipv4Proto);
        routing->AddHostRouteTo(Ipv4Address(remoteAddress.c_str()),
                                ipv4Proto->GetInterfaceForDevice(apNode->GetDevice(0)),
                                0);
    }
    else
    {
        // For unicast, use the first STA's address
        remoteAddress = "10.1.1.2";
    }

    ApplicationHelper sourceHelper(TgaxVideoTraffic::GetTypeId());

    // Configure video parameters based on model
    // Traffic models from IEEE 802.11-14/0571r12:
    // BV1: 2Mbps, BV2: 4Mbps, BV3: 6Mbps, BV4: 8Mbps, BV5: 10Mbps, BV6: 15.6Mbps
    // MC1: 3Mbps multicast, MC2: 6Mbps multicast
    sourceHelper.SetAttribute("Protocol", TypeIdValue(TypeId::LookupByName(protocolFactory)));
    sourceHelper.SetAttribute("TrafficModelClassIdentifier", StringValue(model));

    Address remoteAddr = InetSocketAddress(Ipv4Address(remoteAddress.c_str()), remotePort);
    sourceHelper.SetAttribute("Remote", AddressValue(remoteAddr));

    // Install video source on AP node (downlink traffic)
    auto sourceApps = sourceHelper.Install(apNode);
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(1.0) + duration);

    // Install packet sinks on STAs
    PacketSinkHelper sinkHelper(protocolFactory,
                                InetSocketAddress(Ipv4Address::GetAny(), remotePort));
    auto sinkApps = sinkHelper.Install(staNodes);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(2.0) + duration);

    NS_LOG_INFO("PacketSink application installed on STA node(s)");

    // Connect to video TX with latency trace
    Config::ConnectWithoutContext(
        "/NodeList/*/ApplicationList/*/$ns3::TgaxVideoTraffic/TxWithLatency",
        MakeCallback(&VideoPacketSent));

    // Connect to video frame generation trace
    Config::ConnectWithoutContext(
        "/NodeList/*/ApplicationList/*/$ns3::TgaxVideoTraffic/VideoFrameGenerated",
        MakeCallback(&VideoFrameGenerated));

    // Connect to RX trace
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&PacketReceived));

    // Get expected bit rate for the model
    std::map<std::string, double> modelBitRates = {{"BV1", 2.0},
                                                   {"BV2", 4.0},
                                                   {"BV3", 6.0},
                                                   {"BV4", 8.0},
                                                   {"BV5", 10.0},
                                                   {"BV6", 15.6},
                                                   {"MC1", 3.0},
                                                   {"MC2", 6.0},
                                                   {"Custom", 2.0}};

    double expectedBitRate = modelBitRates.count(model) ? modelBitRates[model] : 2.0;

    NS_LOG_INFO("Starting simulation for traffic duration: " << duration.As(Time::S));
    NS_LOG_INFO("Traffic model: " << model);
    NS_LOG_INFO("Protocol: " << protocol);
    NS_LOG_INFO("Expected bit rate: " << expectedBitRate << " Mbps");
    if (isMulticast)
    {
        NS_LOG_INFO("Multicast mode: delivering to " << numStations << " receivers");
    }

    Simulator::Stop(Seconds(2) + duration);
    Simulator::Run();

    // Get statistics from packet sink applications
    uint64_t totalBytesReceived = 0;
    for (uint32_t i = 0; i < sinkApps.GetN(); ++i)
    {
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(i));
        if (sink)
        {
            uint32_t bytesRx = sink->GetTotalRx();
            NS_LOG_INFO("STA " << i << " received: " << bytesRx << " bytes");
            totalBytesReceived += bytesRx;
        }
    }

    // Calculate approximate expected bytes
    // expectedBitRate is in Mbps, duration is in seconds
    double expectedBytes = (expectedBitRate * 1e6 / 8) * duration.GetSeconds();
    NS_LOG_INFO("Total bytes received: " << totalBytesReceived);
    NS_LOG_INFO("Approximate expected bytes per receiver: " << expectedBytes);

    // Calculate measured bit rate
    double measuredBitRate =
        static_cast<double>(totalBytesReceived * 8) / (duration.GetSeconds() * 1e6);
    if (isMulticast)
    {
        measuredBitRate /= numStations; // Per-receiver rate
    }
    NS_LOG_INFO("Measured bit rate: " << measuredBitRate << " Mbps");

    Simulator::Destroy();

    return 0;
}

void
VideoPacketSent(Ptr<const Packet> packet, Time latency)
{
    NS_LOG_INFO("Video TX: Packet size (bytes): " << packet->GetSize()
                                                  << " Latency: " << latency.As(Time::US));
}

void
VideoFrameGenerated(uint32_t frameSize)
{
    NS_LOG_INFO("Video Frame Generated: " << frameSize << " bytes");
}

void
PacketReceived(std::string context, Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Packet RX: Size(bytes): " << packet->GetSize());
}
