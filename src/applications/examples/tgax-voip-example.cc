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
 * @file tgax-voip-example.cc
 * @brief A simple VoIP traffic generator example over Wi-Fi
 *
 * This example demonstrates how to set up a basic ns-3 simulation with VoIP traffic
 * over a Wi-Fi network. It includes three configurations:
 *
 * 1. IPv4/UDP: VoIP traffic over IPv4 using UDP sockets
 * 2. IPv6/UDP: VoIP traffic over IPv6 using UDP sockets
 * 3. PacketSocket: VoIP traffic using raw packet sockets
 *
 * The simulation consists of:
 * - A simple Wi-Fi network with one AP (Access Point) and one STA (Station)
 * - VoIP traffic flowing from the STA to the AP
 * - Application-level tracing to observe VoIP packets being sent
 * - State change tracing to see voice activity transitions
 *
 * To run with IPv4/UDP (default): ./ns3 run tgax-voip-example
 * To run with IPv6/UDP: ./ns3 run "tgax-voip-example --socketType=ipv6"
 * To run with PacketSocket: ./ns3 run "tgax-voip-example --socketType=packet"
 *
 * To disable verbose logging: ./ns3 run "tgax-voip-example --verbose=false"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TgaxVoipExample");

/**
 * Callback invoked when a VoIP packet is transmitted
 * @param packet The transmitted packet
 * @param jitter The jitter applied to the packet
 */
void VoipPacketSent(Ptr<const Packet> packet, Time jitter);
/**
 * Callback invoked when the VoIP application state changes (active/silence)
 * @param state The new voice activity state
 * @param duration The expected duration in this state
 */
void VoipStateChanged(TgaxVoipTraffic::VoiceActivityState state, Time duration);
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
    Time duration{Seconds(10)};     // Simulation time in seconds
    std::string socketType{"ipv4"}; // Socket type: ipv4, ipv6, or packet
    bool verbose = true;            // Enable/disable verbose logging

    CommandLine cmd(__FILE__);
    cmd.Usage("Basic VoIP example");
    cmd.AddValue("duration", "Duration of traffic flow, in seconds", duration);
    cmd.AddValue("socketType",
                 "Socket type to use (ipv4, ipv6, or packet). Default: ipv4",
                 socketType);
    cmd.AddValue("verbose",
                 "Enable verbose logging of TgaxVoipTraffic, PacketSink, and this program",
                 verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable(
            "TgaxVoipExample",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_ALL));
        LogComponentEnable(
            "TgaxVoipTraffic",
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

    NetDeviceContainer wifiDevices;

    // Configure AP
    Ssid ssid = Ssid("voip-network");
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    auto apDevices = wifi.Install(phy, mac, apNode);
    wifiDevices.Add(apDevices);

    // Configure STA
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    auto staDevices = wifi.Install(phy, mac, staNode);
    wifiDevices.Add(staDevices);

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
    Ipv4InterfaceContainer ipv4Interfaces;
    Ipv6AddressHelper ipv6;
    Ipv6InterfaceContainer ipv6Interfaces;

    if (socketType == "ipv4")
    {
        // Assign IPv4 addresses (needed even for IPv6-only test for basic network function)
        ipv4.SetBase("10.1.1.0", "255.255.255.0");
        ipv4Interfaces = ipv4.Assign(wifiDevices);
        NS_LOG_INFO("AP address:  10.1.1.1");
        NS_LOG_INFO("STA address: 10.1.1.2");
    }
    else if (socketType == "ipv6")
    {
        // Assign IPv6 addresses
        ipv6.SetBase("2001:db8::", Ipv6Prefix(64));
        ipv6Interfaces = ipv6.Assign(wifiDevices);
        NS_LOG_INFO("AP address:  2001:db8::1");
        NS_LOG_INFO("STA address: 2001:db8::2");
    }

    std::string protocol;
    std::string remoteAddress;
    uint16_t remotePort = 5000;

    if (socketType == "ipv4")
    {
        protocol = "ns3::UdpSocketFactory";
        remoteAddress = "10.1.1.1"; // AP's IPv4 address
    }
    else if (socketType == "ipv6")
    {
        protocol = "ns3::UdpSocketFactory";
        remoteAddress = "2001:db8::1"; // AP's IPv6 address
    }
    else if (socketType == "packet")
    {
        protocol = "ns3::PacketSocketFactory";
    }
    else
    {
        NS_FATAL_ERROR("Invalid socket type: " << socketType);
    }

    ApplicationHelper sourceHelper(TgaxVoipTraffic::GetTypeId());

    // Configure VoIP parameters
    // These are the IEEE 802.11ax TGAX defaults:
    // - Active state: 33-byte payload at 20ms intervals
    // - Silence state: 7-byte payload at 160ms intervals
    // - State transitions: 2-state Markov model with 50% voice activity
    // - Jitter: Laplacian distribution (downlink only)

    // For IPv4/UDP, add 3 bytes for compressed protocol header to payload sizes
    // For IPv6/UDP, add 5 bytes for compressed protocol header to payload sizes
    // For PacketSocket, use default sizes
    uint32_t activePayloadSize = 33;
    uint32_t silencePayloadSize = 7;

    if (socketType == "ipv4")
    {
        // IPv4 header compression adds 3 bytes
        activePayloadSize += 3;
        silencePayloadSize += 3;
    }
    else if (socketType == "ipv6")
    {
        // IPv6 header compression adds 5 bytes
        activePayloadSize += 5;
        silencePayloadSize += 5;
    }

    sourceHelper.SetAttribute("Protocol", StringValue(protocol));
    sourceHelper.SetAttribute("ActivePacketPayloadSize", UintegerValue(activePayloadSize));
    sourceHelper.SetAttribute("SilencePacketPayloadSize", UintegerValue(silencePayloadSize));

    // Optional: Configure VoIP state machine parameters
    // Uncomment to customize:
    // sourceHelper.SetAttribute("MeanActiveStateDuration", TimeValue(MilliSeconds(1250)));
    // sourceHelper.SetAttribute("MeanInactiveStateDuration", TimeValue(MilliSeconds(1250)));
    // sourceHelper.SetAttribute("VoiceToSilenceProbability", DoubleValue(0.016));
    // sourceHelper.SetAttribute("SilenceToVoiceProbability", DoubleValue(0.016));

    // Set remote address and port based on socket type
    Address remoteAddr;
    if (socketType == "ipv4")
    {
        remoteAddr = InetSocketAddress(Ipv4Address(remoteAddress.c_str()), remotePort);
    }
    else if (socketType == "ipv6")
    {
        remoteAddr = Inet6SocketAddress(Ipv6Address(remoteAddress.c_str()), remotePort);
    }
    else // PacketSocket
    {
        PacketSocketAddress socketAddr;
        socketAddr.SetSingleDevice(staDevices.Get(0)->GetIfIndex());
        socketAddr.SetPhysicalAddress(apDevices.Get(0)->GetAddress());
        socketAddr.SetProtocol(1);
        remoteAddr = socketAddr;
    }

    sourceHelper.SetAttribute("Remote", AddressValue(remoteAddr));

    // Install VoIP source on STA node
    auto sourceApps = sourceHelper.Install(staNode);
    sourceApps.Start(Seconds(1.0));
    sourceApps.Stop(Seconds(1.0) + duration);

    Address sinkAddr;
    if (socketType == "ipv4")
    {
        sinkAddr = InetSocketAddress(Ipv4Address::GetAny(), remotePort);
    }
    else if (socketType == "ipv6")
    {
        sinkAddr = Inet6SocketAddress(Ipv6Address::GetAny(), remotePort);
    }
    else // PacketSocket
    {
        PacketSocketAddress socketAddr;
        socketAddr.SetSingleDevice(apDevices.Get(0)->GetIfIndex());
        socketAddr.SetPhysicalAddress(apDevices.Get(0)->GetAddress());
        socketAddr.SetProtocol(1);
        sinkAddr = socketAddr;
    }

    PacketSinkHelper sinkHelper(protocol, sinkAddr);
    auto sinkApps = sinkHelper.Install(apNode);
    sinkApps.Start(Seconds(0.0));
    sinkApps.Stop(Seconds(2.0) + duration);

    NS_LOG_INFO("PacketSink application installed on AP (Node 0)");

    // Connect to VoIP TX with jitter trace
    Config::ConnectWithoutContext(
        "/NodeList/*/ApplicationList/*/$ns3::TgaxVoipTraffic/TxWithJitter",
        MakeCallback(&VoipPacketSent));

    // Connect to VoIP state change trace
    Config::ConnectWithoutContext("/NodeList/*/ApplicationList/*/$ns3::TgaxVoipTraffic/StateUpdate",
                                  MakeCallback(&VoipStateChanged));

    // Connect to RX trace
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&PacketReceived));

    NS_LOG_INFO("Starting simulation for traffic duration: " << duration.As(Time::S));
    NS_LOG_INFO("Socket type: " << socketType);
    NS_LOG_INFO("Active payload size: " << activePayloadSize << " bytes");
    NS_LOG_INFO("Silence payload size: " << silencePayloadSize << " bytes");

    Simulator::Stop(Seconds(2) + duration);
    Simulator::Run();

    // Get the packet sink application to retrieve statistics
    Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(0));
    if (sink)
    {
        uint32_t totalBytes = sink->GetTotalRx();
        NS_LOG_INFO("Total bytes received: " << totalBytes);

        // Calculate approximate statistics for default configuration
        // Active packets: ~36 bytes every 20ms = 1800 bytes per second (1 second of talking)
        // Silence packets: ~10 bytes every 160ms = 62.5 bytes per second (1 second of silence)
        // With 50% voice activity: average = (1800 + 62.5) / 2 = ~931 bytes per second
        // For default 10 seconds of simulation, this is 9310 bytes.  Users can observe
        // variation of the actual bytes sent by varying the RngRun parameter to use
        // different random variable run numbers.  Because the duration of this test is short
        // by default (10 seconds), the difference between actual and expected will vary but
        // will gradually converge as traffic duration time is increased.
        // Note that if you change the default configuration of the model, the below estimate
        // will not update accordingly.
        NS_LOG_INFO("Approximate expected bytes (50% activity): " << 931 * duration.GetSeconds());
    }

    Simulator::Destroy();

    return 0;
}

void
VoipPacketSent(Ptr<const Packet> packet, Time jitter)
{
    NS_LOG_INFO("VoIP TX: " << " Packet size (bytes): " << packet->GetSize()
                            << " Jitter: " << jitter.As(Time::US));
}

void
VoipStateChanged(TgaxVoipTraffic::VoiceActivityState state, Time duration)
{
    std::string stateStr = (state == TgaxVoipTraffic::VoiceActivityState::ACTIVE_TALKING)
                               ? "ACTIVE_TALKING"
                               : "INACTIVE_SILENCE";
    NS_LOG_INFO("VoIP State: " << stateStr << " Duration: " << duration.As(Time::MS));
}

void
PacketReceived(std::string context, Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Packet RX: Size(bytes): " << packet->GetSize());
}
