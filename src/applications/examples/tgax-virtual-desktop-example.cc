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
 * @file tgax-virtual-desktop-example.cc
 * @brief A simple Virtual Desktop Infrastructure (VDI) traffic generator example over Wi-Fi
 *
 * This example demonstrates how to set up a basic ns-3 simulation with VDI traffic
 * over a Wi-Fi network. VDI traffic models remote desktop applications where a server
 * sends desktop display data to clients.
 *
 * The simulation consists of:
 * - A simple Wi-Fi network with one AP (Access Point) and one STA (Station)
 * - VDI traffic flowing from the AP (server) to the STA (client) for downlink
 * - Optional uplink traffic from STA to AP for navigation/feedback
 * - Application-level tracing to observe VDI packets being sent
 *
 * The VDI traffic model follows IEEE 802.11-14/0571r12 TGAX evaluation methodology:
 * - Packet inter-arrival times follow an exponential distribution
 * - Packet sizes follow a normal distribution (bimodal for downlink)
 * - Initial packet arrival is uniformly distributed in [0, 20ms]
 *
 * Traffic direction parameters from the specification:
 * - Downlink (AP to STA): Mean arrival 60.2269ms, bimodal packet size (41/1478 bytes)
 * - Uplink (STA to AP): Mean arrival 48.2870ms, normal packet size (mean 50.598 bytes)
 *
 * To run downlink traffic (default): ./ns3 run tgax-virtual-desktop-example
 * To run uplink traffic: ./ns3 run "tgax-virtual-desktop-example --direction=uplink"
 * To run bidirectional: ./ns3 run "tgax-virtual-desktop-example --direction=bidirectional"
 *
 * To disable verbose logging: ./ns3 run "tgax-virtual-desktop-example --verbose=false"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TgaxVirtualDesktopExample");

/**
 * Callback invoked when a VDI packet is transmitted
 * @param context The context string identifying the source
 * @param packet The transmitted packet
 */
void VdiPacketSent(std::string context, Ptr<const Packet> packet);

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
    Time duration{Seconds(10)};        // Simulation time in seconds
    std::string direction{"downlink"}; // Traffic direction: downlink, uplink, or bidirectional
    bool verbose = true;               // Enable/disable verbose logging

    CommandLine cmd(__FILE__);
    cmd.Usage("Virtual Desktop Infrastructure (VDI) traffic example");
    cmd.AddValue("duration", "Duration of traffic flow, in seconds", duration);
    cmd.AddValue("direction",
                 "Traffic direction (downlink, uplink, or bidirectional). Default: downlink",
                 direction);
    cmd.AddValue("verbose",
                 "Enable verbose logging of TgaxVirtualDesktop, PacketSink, and this program",
                 verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable(
            "TgaxVirtualDesktopExample",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_ALL));
        LogComponentEnable(
            "TgaxVirtualDesktop",
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
    Ssid ssid = Ssid("vdi-network");
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

    // VDI uses TCP as specified in the standard
    std::string protocolFactory = "ns3::TcpSocketFactory";
    uint16_t dlPort = 5000; // Downlink port
    uint16_t ulPort = 5001; // Uplink port

    ApplicationContainer sourceApps;
    ApplicationContainer sinkApps;

    // Configure downlink traffic (AP -> STA)
    if (direction == "downlink" || direction == "bidirectional")
    {
        ApplicationHelper dlSourceHelper(TgaxVirtualDesktop::GetTypeId());
        dlSourceHelper.SetAttribute("Protocol", TypeIdValue(TypeId::LookupByName(protocolFactory)));

        // Downlink parameters from IEEE 802.11-14/0571r12:
        // - Mean inter-arrival: 60.2269 ms
        // - Packet size: Bimodal Normal (mu1=41.0, sigma1=3.2; mu2=1478.3, sigma2=11.6)
        // Default attributes already match downlink specification

        Address dlRemoteAddr = InetSocketAddress(ipv4Interfaces.GetAddress(1), dlPort);
        dlSourceHelper.SetAttribute("Remote", AddressValue(dlRemoteAddr));

        auto dlSourceApp = dlSourceHelper.Install(apNode);
        dlSourceApp.Start(Seconds(1.0));
        dlSourceApp.Stop(Seconds(1.0) + duration);
        sourceApps.Add(dlSourceApp);

        PacketSinkHelper dlSinkHelper(protocolFactory,
                                      InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        auto dlSinkApp = dlSinkHelper.Install(staNode);
        dlSinkApp.Start(Seconds(0.0));
        dlSinkApp.Stop(Seconds(2.0) + duration);
        sinkApps.Add(dlSinkApp);

        NS_LOG_INFO("Downlink VDI traffic configured (AP -> STA)");
        NS_LOG_INFO("  Mean inter-arrival: 60.2269 ms");
        NS_LOG_INFO("  Packet size: Bimodal Normal (41.0/1478.3 bytes)");
    }

    // Configure uplink traffic (STA -> AP)
    if (direction == "uplink" || direction == "bidirectional")
    {
        ApplicationHelper ulSourceHelper(TgaxVirtualDesktop::GetTypeId());
        ulSourceHelper.SetAttribute("Protocol", TypeIdValue(TypeId::LookupByName(protocolFactory)));

        // Uplink parameters from IEEE 802.11-14/0571r12:
        // - Mean inter-arrival: 48.2870 ms
        // - Packet size: Normal (mu=50.598, sigma=5.0753)
        auto ulInterArrival = CreateObjectWithAttributes<ExponentialRandomVariable>(
            "Mean",
            DoubleValue(48287000.0)); // 48.287 ms in nanoseconds
        ulSourceHelper.SetAttribute("InterPacketArrivals", PointerValue(ulInterArrival));
        ulSourceHelper.SetAttribute("ParametersPacketSize", StringValue("50.598 5.0753"));

        Address ulRemoteAddr = InetSocketAddress(ipv4Interfaces.GetAddress(0), ulPort);
        ulSourceHelper.SetAttribute("Remote", AddressValue(ulRemoteAddr));

        auto ulSourceApp = ulSourceHelper.Install(staNode);
        ulSourceApp.Start(Seconds(1.0));
        ulSourceApp.Stop(Seconds(1.0) + duration);
        sourceApps.Add(ulSourceApp);

        PacketSinkHelper ulSinkHelper(protocolFactory,
                                      InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        auto ulSinkApp = ulSinkHelper.Install(apNode);
        ulSinkApp.Start(Seconds(0.0));
        ulSinkApp.Stop(Seconds(2.0) + duration);
        sinkApps.Add(ulSinkApp);

        NS_LOG_INFO("Uplink VDI traffic configured (STA -> AP)");
        NS_LOG_INFO("  Mean inter-arrival: 48.2870 ms");
        NS_LOG_INFO("  Packet size: Normal (mean 50.598 bytes)");
    }

    if (direction != "downlink" && direction != "uplink" && direction != "bidirectional")
    {
        NS_FATAL_ERROR("Invalid direction: " << direction
                                             << ". Use 'downlink', 'uplink', or 'bidirectional'.");
    }

    // Connect to VDI TX trace
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::TgaxVirtualDesktop/Tx",
                    MakeCallback(&VdiPacketSent));

    // Connect to RX trace
    Config::Connect("/NodeList/*/ApplicationList/*/$ns3::PacketSink/Rx",
                    MakeCallback(&PacketReceived));

    NS_LOG_INFO("Starting simulation for traffic duration: " << duration.As(Time::S));
    NS_LOG_INFO("Traffic direction: " << direction);

    Simulator::Stop(Seconds(2) + duration);
    Simulator::Run();

    // Get statistics from packet sink applications
    for (uint32_t i = 0; i < sinkApps.GetN(); ++i)
    {
        Ptr<PacketSink> sink = DynamicCast<PacketSink>(sinkApps.Get(i));
        if (sink)
        {
            uint32_t bytesRx = sink->GetTotalRx();
            NS_LOG_INFO("Sink " << i << " received: " << bytesRx << " bytes");
        }
    }

    Simulator::Destroy();

    return 0;
}

void
VdiPacketSent(std::string context, Ptr<const Packet> packet)
{
    NS_LOG_INFO("VDI TX [" << context << "]: Packet size (bytes): " << packet->GetSize());
}

void
PacketReceived(std::string context, Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Packet RX [" << context << "]: Size(bytes): " << packet->GetSize());
}
