/*
 * Copyright (c) 2015
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

// This example shows how to set Wi-Fi timing parameters through WifiMac attributes.
//
// Example: set slot time to 20 microseconds, while keeping other values as defined in the
// simulation script:
//
//          ./ns3 run "wifi-timing-attributes --slot=20us"
//
// Network topology:
//
//  Wifi 192.168.1.0
//
//       AP
//  *    *
//  |    |
//  n1   n2

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("wifi-timing-attributes");

int
main(int argc, char* argv[])
{
    Time slot{"9us"};           // slot time
    Time sifs{"10us"};          // SIFS duration
    Time pifs{"19us"};          // PIFS duration
    Time simulationTime{"10s"}; // Simulation time

    CommandLine cmd(__FILE__);
    cmd.AddValue("slot", "Slot time", slot);
    cmd.AddValue("sifs", "SIFS duration", sifs);
    cmd.AddValue("pifs", "PIFS duration", pifs);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.Parse(argc, argv);

    // Since default reference loss is defined for 5 GHz, it needs to be changed when operating
    // at 2.4 GHz
    Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(40.046));

    // Create nodes
    NodeContainer wifiStaNode;
    wifiStaNode.Create(1);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    // Create wireless channel
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    // Default IEEE 802.11n (2.4 GHz)
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("HtMcs7"),
                                 "ControlMode",
                                 StringValue("HtMcs0"));
    WifiMacHelper mac;

    // Install PHY and MAC
    Ssid ssid = Ssid("ns3-wifi");
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer staDevice;
    staDevice = wifi.Install(phy, mac, wifiStaNode);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice;
    apDevice = wifi.Install(phy, mac, wifiApNode);

    // Once install is done, we overwrite the standard timing values
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/Slot", TimeValue(slot));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/Sifs", TimeValue(sifs));
    Config::Set("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/Pifs", TimeValue(pifs));

    // Mobility
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNode);

    Ipv4AddressHelper address;

    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterface;
    Ipv4InterfaceContainer apNodeInterface;

    staNodeInterface = address.Assign(staDevice);
    apNodeInterface = address.Assign(apDevice);

    // Setting applications
    uint16_t port = 9;
    UdpServerHelper server(port);
    ApplicationContainer serverApp = server.Install(wifiStaNode.Get(0));
    serverApp.Start(Seconds(0));
    serverApp.Stop(simulationTime + Seconds(1));

    UdpClientHelper client(staNodeInterface.GetAddress(0), port);
    client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
    client.SetAttribute("Interval", TimeValue(Time("0.0001"))); // packets/s
    client.SetAttribute("PacketSize", UintegerValue(1472));     // bytes

    ApplicationContainer clientApp = client.Install(wifiApNode.Get(0));
    clientApp.Start(Seconds(1));
    clientApp.Stop(simulationTime + Seconds(1));

    // Populate routing table
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Set simulation time and launch simulation
    Simulator::Stop(simulationTime + Seconds(1));
    Simulator::Run();

    // Get and print results
    double totalPacketsThrough = DynamicCast<UdpServer>(serverApp.Get(0))->GetReceived();
    auto throughput = totalPacketsThrough * 1472 * 8 / simulationTime.GetMicroSeconds(); // Mbit/s
    std::cout << "Throughput: " << throughput << " Mbit/s" << std::endl;

    Simulator::Destroy();
    return 0;
}
