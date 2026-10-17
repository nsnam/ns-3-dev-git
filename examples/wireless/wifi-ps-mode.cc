/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

// This example aims at measuring the performance of a network in which non-AP devices may have one
// or more affiliated STAs in power save mode.
//
// The simulation considers a single AP and a (configurable) number of non-AP devices using the same
// set of links. The frequency channel used by each link can be configured. All non-AP devices have
// the same configuration for the Power Management mode of their links. Note that, by default, only
// the STAs affiliated with non-AP devices and operating on link 0 are in powersave mode; use the
// --psMode command line argument to modify this configuration. It is possible to generate downlink
// and/or uplink UDP/TCP traffic between the AP and every non-AP device. The flow data rate needs to
// be configured for both downlink and uplink, as by default it is set to zero. The rate adaptation
// manager and the size of every packet can also be configured.
//
// Per-flow statistics collected with FlowMonitor are printed at the end of the simulation. It is
// checked that the throughput of each flow is within a configurable bound of the flow data rate.
//
// Example usage and output:
//
// ./ns3 run "wifi-ps-mode --dlLoad=10Mbps --ulLoad=25Mbps --simulationTime=500ms --nStas=2
// --staticSetup=1 --psMode=0:true,1:true --channels=36,0,BAND_5GHZ,0:100,0,BAND_5GHZ,0"
//
// The IP address of the AP is 192.168.1.1
//
// Flow 1 (192.168.1.2 -> 192.168.1.1)
//   Tx Packets: 520
//   Tx Bytes:   794560
//   TxOffered:  12712960bps
//   Rx Packets: 520
//   Rx Bytes:   794560
//   Throughput: 12712960bps
// Flow 2 (192.168.1.3 -> 192.168.1.1)
//   Tx Packets: 520
//   Tx Bytes:   794560
//   TxOffered:  12712960bps
//   Rx Packets: 520
//   Rx Bytes:   794560
//   Throughput: 12712960bps
// Flow 3 (192.168.1.1 -> 192.168.1.2)
//   Tx Packets: 208
//   Tx Bytes:   317824
//   TxOffered:  5085184bps
//   Rx Packets: 182
//   Rx Bytes:   278096
//   Throughput: 4449536bps
// Flow 4 (192.168.1.1 -> 192.168.1.3)
//   Tx Packets: 208
//   Tx Bytes:   317824
//   TxOffered:  5085184bps
//   Rx Packets: 182
//   Rx Bytes:   278096
//   Throughput: 4449536bps

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/flow-monitor-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-flow-classifier.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/neighbor-cache-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-static-setup-helper.h"

#include <algorithm>
#include <unordered_map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPsMode");

int
main(int argc, char* argv[])
{
    std::string standard{"11be"};
    std::string channels{"36,0,BAND_5GHZ,0"};
    std::size_t nStas{1};
    std::string psMode{"0:true"};
    std::string raa{"ThompsonSamplingWifiManager"};
    uint32_t payloadSize{1500};
    bool enableRts{false};
    bool enablePcap{false};
    bool useUdp{true};
    DataRate ulLoad;
    DataRate dlLoad;
    bool staticSetup{true};
    Time simulationTime{"3s"};
    double tolerance{0.15};

    CommandLine cmd(__FILE__);
    cmd.AddValue("standard", "Supported standard (11n, 11ac, 11ax, 11be)", standard);
    cmd.AddValue("channels",
                 "Colon separated (no spaces) list of channel settings for the links of both "
                 "the AP and the STAs",
                 channels);
    cmd.AddValue("nStas", "the number of non-AP devices", nStas);
    cmd.AddValue("psMode",
                 "Comma separated (no spaces) list of pairs indicating whether PM mode must be "
                 "enabled on a given link for all non-AP devices (e.g., \"0:true,1:false\" enables "
                 "PM mode only on link 0)",
                 psMode);
    cmd.AddValue("raa", "TypeId of the rate adaptation algorithm to use", raa);
    cmd.AddValue("payloadSize", "Payload size in bytes", payloadSize);
    cmd.AddValue("enableRts", "Enable or disable RTS/CTS", enableRts);
    cmd.AddValue("enablePcap", "Enable/disable pcap file generation", enablePcap);
    cmd.AddValue("useUdp", "true to use UDP, false to use TCP", useUdp);
    cmd.AddValue("dlLoad", "Rate of the total downlink load to generate", dlLoad);
    cmd.AddValue("ulLoad", "Rate of the total uplink load to generate", ulLoad);
    cmd.AddValue("staticSetup", "whether to use the static setup helper", staticSetup);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("tolerance",
                 "simulation fails if the throughput of every flow is not within this ratio "
                 "of the offered traffic load",
                 tolerance);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                       enableRts ? StringValue("0")
                                 : StringValue(std::to_string(WIFI_MAX_RTS_THRESHOLD)));

    NodeContainer wifiStaNodes(nStas);
    NodeContainer wifiApNode(1);

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::" + raa);

    if (standard == "11be")
    {
        wifi.SetStandard(WIFI_STANDARD_80211be);
    }
    else if (standard == "11ax")
    {
        wifi.SetStandard(WIFI_STANDARD_80211ax);
    }
    else if (standard == "11ac")
    {
        wifi.SetStandard(WIFI_STANDARD_80211ac);
    }
    else if (standard == "11n")
    {
        wifi.SetStandard(WIFI_STANDARD_80211n);
    }
    else
    {
        NS_ABORT_MSG("Unsupported standard (" << standard
                                              << "), valid values are 11n, 11ac, 11ax or 11be");
    }

    std::unordered_map<WifiPhyBand, Ptr<MultiModelSpectrumChannel>> channelMap = {
        {WIFI_PHY_BAND_2_4GHZ, CreateObject<MultiModelSpectrumChannel>()},
        {WIFI_PHY_BAND_5GHZ, CreateObject<MultiModelSpectrumChannel>()},
        {WIFI_PHY_BAND_6GHZ, CreateObject<MultiModelSpectrumChannel>()}};

    auto strings = SplitString(channels, ":");
    SpectrumWifiPhyHelper phy{static_cast<uint8_t>(strings.size())};
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    linkId_t linkId = 0;
    for (auto& str : strings)
    {
        str = "{" + str + "}";
        phy.Set(linkId, "ChannelSettings", StringValue(str));

        auto channelConfig = WifiChannelConfig::FromString(str);
        auto phyBand = channelConfig.front().band;
        auto freqRange = GetFrequencyRange(phyBand);
        phy.AddPhyToFreqRangeMapping(linkId, freqRange);
        phy.AddChannel(channelMap.at(phyBand), freqRange);

        ++linkId;
    }

    Ssid ssid("wifi-ps-mode");

    WifiMacHelper mac;
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    auto apDevice = wifi.Install(phy, mac, wifiApNode);

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    // adjust psMode string to make it compatible with the format of the PowerSaveMode attribute
    std::replace(psMode.begin(), psMode.end(), ':', ' ');
    mac.SetPowerSaveManager("ns3::DefaultPowerSaveManager", "PowerSaveMode", StringValue(psMode));
    auto staDevices = wifi.Install(phy, mac, wifiStaNodes);

    int64_t streamNumber = 150;
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    // Setting mobility model
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    if (staticSetup)
    {
        /* static setup of association and BA agreements */
        auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
        NS_ASSERT(apDev);
        WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevices);
        WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevices, {0});
    }

    // Internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    auto apInterface = address.Assign(apDevice);
    auto staInterfaces = address.Assign(staDevices);

    std::cout << "The IP address of the AP is " << apInterface.GetAddress(0) << "\n\n";

    /* static setup of ARP cache */
    NeighborCacheHelper nbCache;
    nbCache.PopulateNeighborCache();

    // allow for some time to associate and establish BA agreement if static setup helper is not
    // used
    Time startTime = staticSetup ? Time{0} : MilliSeconds(500);

    // Setting applications
    const uint16_t port = 9; // Discard port (RFC 863)
    auto socketFactory = useUdp ? "ns3::UdpSocketFactory" : "ns3::TcpSocketFactory";

    // Install client and server apps for UL flow (if data rate > 0)
    if (ulLoad.GetBitRate() > 0)
    {
        OnOffHelper onoff(socketFactory,
                          Address(InetSocketAddress(apInterface.GetAddress(0), port)));
        onoff.SetConstantRate(ulLoad * (1. / nStas), payloadSize);

        for (uint32_t i = 0; i < wifiStaNodes.GetN(); ++i)
        {
            auto clientApp = onoff.Install(wifiStaNodes.Get(i));
            clientApp.Start(startTime);
            clientApp.Stop(startTime + simulationTime);
        }

        // Create a packet sink to receive these packets
        PacketSinkHelper sink(socketFactory,
                              Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
        auto serverApp = sink.Install(wifiApNode.Get(0));
        serverApp.Start(Time{0});
    }

    // Install client and server apps for DL flow (if data rate > 0)
    if (dlLoad.GetBitRate() > 0)
    {
        for (uint32_t i = 0; i < staInterfaces.GetN(); ++i)
        {
            OnOffHelper onoff(socketFactory,
                              Address(InetSocketAddress(staInterfaces.GetAddress(i), port)));
            onoff.SetConstantRate(dlLoad * (1. / nStas), payloadSize);

            auto clientApp = onoff.Install(wifiApNode.Get(0));
            clientApp.Start(startTime);
            clientApp.Stop(startTime + simulationTime);

            // Create a packet sink to receive these packets
            PacketSinkHelper sink(socketFactory,
                                  Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
            auto serverApp = sink.Install(wifiStaNodes.Get(i));
            serverApp.Start(Time{0});
        }
    }

    if (enablePcap)
    {
        phy.EnablePcap("wifi_PS_mode_AP", apDevice);
        phy.EnablePcap("wifi_PS_mode_STA", staDevices);
    }

    FlowMonitorHelper flowmon;
    auto monitor = flowmon.InstallAll();

    Simulator::Stop(startTime + simulationTime);
    Simulator::Run();

    monitor->CheckForLostPackets();
    auto classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
    auto stats = monitor->GetFlowStats();
    for (const auto& [flowId, flowStats] : stats)
    {
        auto t = classifier->FindFlow(flowId);

        const auto load = DataRate((flowStats.txBytes * 8.0) / simulationTime.GetSeconds());
        const auto tput = DataRate((flowStats.rxBytes * 8.0) / simulationTime.GetSeconds());

        std::cout << "Flow " << flowId << " (" << t.sourceAddress << " -> " << t.destinationAddress
                  << ")\n";
        std::cout << "  Tx Packets: " << flowStats.txPackets << "\n";
        std::cout << "  Tx Bytes:   " << flowStats.txBytes << "\n";
        std::cout << "  TxOffered:  " << load << "\n";
        std::cout << "  Rx Packets: " << flowStats.rxPackets << "\n";
        std::cout << "  Rx Bytes:   " << flowStats.rxBytes << "\n";
        std::cout << "  Throughput: " << tput << "\n";

        const auto minExpectedTput = load * (1.0 - tolerance);
        const auto maxExpectedTput = load * (1.0 + tolerance);
        if (tput < minExpectedTput || tput > maxExpectedTput)
        {
            NS_LOG_ERROR("Throughput " << tput << " is outside expected range [" << minExpectedTput
                                       << ", " << maxExpectedTput << "]");
            exit(1);
        }
    }

    Simulator::Destroy();

    return 0;
}
