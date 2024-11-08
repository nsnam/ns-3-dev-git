/*
 * Copyright (c) 2016 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/command-line.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/pointer.h"
#include "ns3/qos-txop.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

// This is an example that illustrates 802.11 QoS for different Access Categories.
// It defines 4 independent Wi-Fi networks (working on different logical channels
// on the same "ns3::YansWifiPhy" channel object).
// Each network contains one access point and one station. Each station continuously
// transmits data packets to its respective AP.
//
// Network topology (numbers in parentheses are channel numbers):
//
//  BSS A (36)        BSS B (40)       BSS C (44)        BSS D (48)
//   *      *          *      *         *      *          *      *
//   |      |          |      |         |      |          |      |
//  AP A   STA A      AP B   STA B     AP C   STA C      AP D   STA D
//
// The configuration is the following on the 4 networks:
// - STA A sends AC_BE traffic to AP A with default AC_BE TXOP value of 0 (1 MSDU);
// - STA B sends AC_BE traffic to AP B with non-default AC_BE TXOP of 4096 us;
// - STA C sends AC_VI traffic to AP C with default AC_VI TXOP of 4096 us;
// - STA D sends AC_VI traffic to AP D with non-default AC_VI TXOP value of 0 (1 MSDU);
//
// The user can select the distance between the stations and the APs, can enable/disable the RTS/CTS
// mechanism and can choose the payload size and the simulation duration. Example: ./ns3 run
// "wifi-80211e-txop --distance=10 --simulationTime=20s --payloadSize=1000"
//
// The output prints the throughput measured for the 4 cases/networks described above. When TXOP is
// enabled, results show increased throughput since the channel is granted for a longer duration.
// TXOP is enabled by default for AC_VI and AC_VO, so that they can use the channel for a longer
// duration than AC_BE and AC_BK.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("80211eTxop");

/**
 * Keeps the maximum duration among all TXOPs
 */
struct TxopDurationTracer
{
    /**
     * Callback connected to TXOP duration trace source.
     *
     * @param startTime TXOP start time
     * @param duration TXOP duration
     * @param linkId the ID of the link
     */
    void Trace(Time startTime, Time duration, uint8_t linkId);
    Time m_max; //!< maximum TXOP duration
};

void
TxopDurationTracer::Trace(Time startTime, Time duration, uint8_t linkId)
{
    if (duration > m_max)
    {
        m_max = duration;
    }
}

int
main(int argc, char* argv[])
{
    uint32_t payloadSize{1472}; // bytes
    Time simulationTime{"10s"};
    meter_u distance{5};
    bool enablePcap{false};
    bool verifyResults{false}; // used for regression
    Time txopLimit{"4096us"};

    CommandLine cmd(__FILE__);
    cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("distance",
                 "Distance in meters between the station and the access point",
                 distance);
    cmd.AddValue("enablePcap", "Enable/disable pcap file generation", enablePcap);
    cmd.AddValue("verifyResults",
                 "Enable/disable results verification at the end of the simulation",
                 verifyResults);
    cmd.Parse(argc, argv);

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(4);
    NodeContainer wifiApNodes;
    wifiApNodes.Create(4);

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::IdealWifiManager");
    WifiMacHelper mac;

    NetDeviceContainer staDeviceA;
    NetDeviceContainer staDeviceB;
    NetDeviceContainer staDeviceC;
    NetDeviceContainer staDeviceD;
    NetDeviceContainer apDeviceA;
    NetDeviceContainer apDeviceB;
    NetDeviceContainer apDeviceC;
    NetDeviceContainer apDeviceD;
    Ssid ssid;

    // Network A
    ssid = Ssid("network-A");
    phy.Set("ChannelSettings", StringValue("{36, 20, BAND_5GHZ, 0}"));
    mac.SetType("ns3::StaWifiMac", "QosSupported", BooleanValue(true), "Ssid", SsidValue(ssid));
    staDeviceA = wifi.Install(phy, mac, wifiStaNodes.Get(0));

    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(true),
                "Ssid",
                SsidValue(ssid),
                "EnableBeaconJitter",
                BooleanValue(false));
    apDeviceA = wifi.Install(phy, mac, wifiApNodes.Get(0));

    // Network B
    ssid = Ssid("network-B");
    phy.Set("ChannelSettings", StringValue("{40, 20, BAND_5GHZ, 0}"));
    mac.SetType("ns3::StaWifiMac", "QosSupported", BooleanValue(true), "Ssid", SsidValue(ssid));

    staDeviceB = wifi.Install(phy, mac, wifiStaNodes.Get(1));

    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(true),
                "Ssid",
                SsidValue(ssid),
                "EnableBeaconJitter",
                BooleanValue(false));
    apDeviceB = wifi.Install(phy, mac, wifiApNodes.Get(1));

    // Modify EDCA configuration (TXOP limit) for AC_BE
    Ptr<NetDevice> dev = wifiApNodes.Get(1)->GetDevice(0);
    Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
    Ptr<WifiMac> wifi_mac = wifi_dev->GetMac();
    PointerValue ptr;
    Ptr<QosTxop> edca;
    wifi_mac->GetAttribute("BE_Txop", ptr);
    edca = ptr.Get<QosTxop>();
    edca->SetTxopLimit(txopLimit);

    // Trace TXOP duration for BE on STA1
    dev = wifiStaNodes.Get(1)->GetDevice(0);
    wifi_dev = DynamicCast<WifiNetDevice>(dev);
    wifi_mac = wifi_dev->GetMac();
    wifi_mac->GetAttribute("BE_Txop", ptr);
    edca = ptr.Get<QosTxop>();
    TxopDurationTracer beTxopTracer;
    edca->TraceConnectWithoutContext("TxopTrace",
                                     MakeCallback(&TxopDurationTracer::Trace, &beTxopTracer));

    // Network C
    ssid = Ssid("network-C");
    phy.Set("ChannelSettings", StringValue("{44, 20, BAND_5GHZ, 0}"));
    mac.SetType("ns3::StaWifiMac", "QosSupported", BooleanValue(true), "Ssid", SsidValue(ssid));

    staDeviceC = wifi.Install(phy, mac, wifiStaNodes.Get(2));

    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(true),
                "Ssid",
                SsidValue(ssid),
                "EnableBeaconJitter",
                BooleanValue(false));
    apDeviceC = wifi.Install(phy, mac, wifiApNodes.Get(2));

    // Trace TXOP duration for VI on STA2
    dev = wifiStaNodes.Get(2)->GetDevice(0);
    wifi_dev = DynamicCast<WifiNetDevice>(dev);
    wifi_mac = wifi_dev->GetMac();
    wifi_mac->GetAttribute("VI_Txop", ptr);
    edca = ptr.Get<QosTxop>();
    TxopDurationTracer viTxopTracer;
    edca->TraceConnectWithoutContext("TxopTrace",
                                     MakeCallback(&TxopDurationTracer::Trace, &viTxopTracer));

    // Network D
    ssid = Ssid("network-D");
    phy.Set("ChannelSettings", StringValue("{48, 20, BAND_5GHZ, 0}"));
    mac.SetType("ns3::StaWifiMac", "QosSupported", BooleanValue(true), "Ssid", SsidValue(ssid));

    staDeviceD = wifi.Install(phy, mac, wifiStaNodes.Get(3));

    mac.SetType("ns3::ApWifiMac",
                "QosSupported",
                BooleanValue(true),
                "Ssid",
                SsidValue(ssid),
                "EnableBeaconJitter",
                BooleanValue(false));
    apDeviceD = wifi.Install(phy, mac, wifiApNodes.Get(3));

    // Modify EDCA configuration (TXOP limit) for AC_VO
    dev = wifiApNodes.Get(3)->GetDevice(0);
    wifi_dev = DynamicCast<WifiNetDevice>(dev);
    wifi_mac = wifi_dev->GetMac();
    wifi_mac->GetAttribute("VI_Txop", ptr);
    edca = ptr.Get<QosTxop>();
    edca->SetTxopLimit(MicroSeconds(0));

    /* Setting mobility model */
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Set position for APs
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(10.0, 0.0, 0.0));
    positionAlloc->Add(Vector(20.0, 0.0, 0.0));
    positionAlloc->Add(Vector(30.0, 0.0, 0.0));
    // Set position for STAs
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    positionAlloc->Add(Vector(10 + distance, 0.0, 0.0));
    positionAlloc->Add(Vector(20 + distance, 0.0, 0.0));
    positionAlloc->Add(Vector(30 + distance, 0.0, 0.0));
    // Remark: while we set these positions 10 meters apart, the networks do not interact
    // and the only variable that affects transmission performance is the distance.

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNodes);
    mobility.Install(wifiStaNodes);

    /* Internet stack */
    InternetStackHelper stack;
    stack.Install(wifiApNodes);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer StaInterfaceA;
    StaInterfaceA = address.Assign(staDeviceA);
    Ipv4InterfaceContainer ApInterfaceA;
    ApInterfaceA = address.Assign(apDeviceA);

    address.SetBase("192.168.2.0", "255.255.255.0");
    Ipv4InterfaceContainer StaInterfaceB;
    StaInterfaceB = address.Assign(staDeviceB);
    Ipv4InterfaceContainer ApInterfaceB;
    ApInterfaceB = address.Assign(apDeviceB);

    address.SetBase("192.168.3.0", "255.255.255.0");
    Ipv4InterfaceContainer StaInterfaceC;
    StaInterfaceC = address.Assign(staDeviceC);
    Ipv4InterfaceContainer ApInterfaceC;
    ApInterfaceC = address.Assign(apDeviceC);

    address.SetBase("192.168.4.0", "255.255.255.0");
    Ipv4InterfaceContainer StaInterfaceD;
    StaInterfaceD = address.Assign(staDeviceD);
    Ipv4InterfaceContainer ApInterfaceD;
    ApInterfaceD = address.Assign(apDeviceD);

    /* Setting applications */
    uint16_t port = 5001;
    UdpServerHelper serverA(port);
    ApplicationContainer serverAppA = serverA.Install(wifiApNodes.Get(0));
    serverAppA.Start(Seconds(0));
    serverAppA.Stop(simulationTime + Seconds(1));

    InetSocketAddress destA(ApInterfaceA.GetAddress(0), port);

    OnOffHelper clientA("ns3::UdpSocketFactory", destA);
    clientA.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientA.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientA.SetAttribute("DataRate", StringValue("100000kb/s"));
    clientA.SetAttribute("PacketSize", UintegerValue(payloadSize));
    clientA.SetAttribute("Tos", UintegerValue(0x70)); // AC_BE

    ApplicationContainer clientAppA = clientA.Install(wifiStaNodes.Get(0));
    clientAppA.Start(Seconds(1));
    clientAppA.Stop(simulationTime + Seconds(1));

    UdpServerHelper serverB(port);
    ApplicationContainer serverAppB = serverB.Install(wifiApNodes.Get(1));
    serverAppB.Start(Seconds(0));
    serverAppB.Stop(simulationTime + Seconds(1));

    InetSocketAddress destB(ApInterfaceB.GetAddress(0), port);

    OnOffHelper clientB("ns3::UdpSocketFactory", destB);
    clientB.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientB.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientB.SetAttribute("DataRate", StringValue("100000kb/s"));
    clientB.SetAttribute("PacketSize", UintegerValue(payloadSize));
    clientB.SetAttribute("Tos", UintegerValue(0x70)); // AC_BE

    ApplicationContainer clientAppB = clientB.Install(wifiStaNodes.Get(1));
    clientAppB.Start(Seconds(1));
    clientAppB.Stop(simulationTime + Seconds(1));

    UdpServerHelper serverC(port);
    ApplicationContainer serverAppC = serverC.Install(wifiApNodes.Get(2));
    serverAppC.Start(Seconds(0));
    serverAppC.Stop(simulationTime + Seconds(1));

    InetSocketAddress destC(ApInterfaceC.GetAddress(0), port);

    OnOffHelper clientC("ns3::UdpSocketFactory", destC);
    clientC.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientC.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientC.SetAttribute("DataRate", StringValue("100000kb/s"));
    clientC.SetAttribute("PacketSize", UintegerValue(payloadSize));
    clientC.SetAttribute("Tos", UintegerValue(0xb8)); // AC_VI

    ApplicationContainer clientAppC = clientC.Install(wifiStaNodes.Get(2));
    clientAppC.Start(Seconds(1));
    clientAppC.Stop(simulationTime + Seconds(1));

    UdpServerHelper serverD(port);
    ApplicationContainer serverAppD = serverD.Install(wifiApNodes.Get(3));
    serverAppD.Start(Seconds(0));
    serverAppD.Stop(simulationTime + Seconds(1));

    InetSocketAddress destD(ApInterfaceD.GetAddress(0), port);

    OnOffHelper clientD("ns3::UdpSocketFactory", destD);
    clientD.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientD.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    clientD.SetAttribute("DataRate", StringValue("100000kb/s"));
    clientD.SetAttribute("PacketSize", UintegerValue(payloadSize));
    clientD.SetAttribute("Tos", UintegerValue(0xb8)); // AC_VI

    ApplicationContainer clientAppD = clientD.Install(wifiStaNodes.Get(3));
    clientAppD.Start(Seconds(1));
    clientAppD.Stop(simulationTime + Seconds(1));

    if (enablePcap)
    {
        phy.EnablePcap("AP_A", apDeviceA.Get(0));
        phy.EnablePcap("STA_A", staDeviceA.Get(0));
        phy.EnablePcap("AP_B", apDeviceB.Get(0));
        phy.EnablePcap("STA_B", staDeviceB.Get(0));
        phy.EnablePcap("AP_C", apDeviceC.Get(0));
        phy.EnablePcap("STA_C", staDeviceC.Get(0));
        phy.EnablePcap("AP_D", apDeviceD.Get(0));
        phy.EnablePcap("STA_D", staDeviceD.Get(0));
    }

    Simulator::Stop(simulationTime + Seconds(1));
    Simulator::Run();

    /* Show results */
    double totalPacketsThroughA = DynamicCast<UdpServer>(serverAppA.Get(0))->GetReceived();
    double totalPacketsThroughB = DynamicCast<UdpServer>(serverAppB.Get(0))->GetReceived();
    double totalPacketsThroughC = DynamicCast<UdpServer>(serverAppC.Get(0))->GetReceived();
    double totalPacketsThroughD = DynamicCast<UdpServer>(serverAppD.Get(0))->GetReceived();

    Simulator::Destroy();

    auto throughput = totalPacketsThroughA * payloadSize * 8 / simulationTime.GetMicroSeconds();
    std::cout << "AC_BE with default TXOP limit (0ms): " << '\n'
              << "  Throughput = " << throughput << " Mbit/s" << '\n';
    if (verifyResults && (throughput < 28 || throughput > 29))
    {
        NS_LOG_ERROR("Obtained throughput " << throughput << " is not in the expected boundaries!");
        exit(1);
    }

    throughput = totalPacketsThroughB * payloadSize * 8 / simulationTime.GetMicroSeconds();
    std::cout << "AC_BE with non-default TXOP limit (4.096ms): " << '\n'
              << "  Throughput = " << throughput << " Mbit/s" << '\n';
    if (verifyResults && (throughput < 36.5 || throughput > 37))
    {
        NS_LOG_ERROR("Obtained throughput " << throughput << " is not in the expected boundaries!");
        exit(1);
    }
    std::cout << "  Maximum TXOP duration = " << beTxopTracer.m_max.GetMicroSeconds() << " us"
              << '\n';
    if (verifyResults &&
        (beTxopTracer.m_max < MicroSeconds(3008) || beTxopTracer.m_max > txopLimit))
    {
        NS_LOG_ERROR("Maximum TXOP duration " << beTxopTracer.m_max
                                              << " is not in the expected boundaries!");
        exit(1);
    }

    throughput = totalPacketsThroughC * payloadSize * 8 / simulationTime.GetMicroSeconds();
    std::cout << "AC_VI with default TXOP limit (4.096ms): " << '\n'
              << "  Throughput = " << throughput << " Mbit/s" << '\n';
    if (verifyResults && (throughput < 36.5 || throughput > 37.5))
    {
        NS_LOG_ERROR("Obtained throughput " << throughput << " is not in the expected boundaries!");
        exit(1);
    }
    std::cout << "  Maximum TXOP duration = " << viTxopTracer.m_max.GetMicroSeconds() << " us"
              << '\n';
    if (verifyResults &&
        (viTxopTracer.m_max < MicroSeconds(3008) || viTxopTracer.m_max > txopLimit))
    {
        NS_LOG_ERROR("Maximum TXOP duration " << viTxopTracer.m_max
                                              << " is not in the expected boundaries!");
        exit(1);
    }

    throughput = totalPacketsThroughD * payloadSize * 8 / simulationTime.GetMicroSeconds();
    std::cout << "AC_VI with non-default TXOP limit (0ms): " << '\n'
              << "  Throughput = " << throughput << " Mbit/s" << '\n';
    if (verifyResults && (throughput < 31.5 || throughput > 32.5))
    {
        NS_LOG_ERROR("Obtained throughput " << throughput << " is not in the expected boundaries!");
        exit(1);
    }

    return 0;
}
