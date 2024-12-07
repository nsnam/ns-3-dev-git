/*
 * Copyright (c) 2015 SEBASTIEN DERONNE
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/multi-model-spectrum-channel.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/spectrum-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include "ns3/uinteger.h"
#include "ns3/vht-phy.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <vector>

// This is a simple example in order to show how to configure an IEEE 802.11ac Wi-Fi network.
//
// It outputs the UDP or TCP goodput for every VHT MCS value, which depends on the MCS value (0 to
// 9, where 9 is forbidden when the channel width is 20 MHz), the channel width (20, 40, 80 or 160
// MHz) and the guard interval (long or short). The PHY bitrate is constant over all the simulation
// run. The user can also specify the distance between the access point and the station: the larger
// the distance the smaller the goodput.
//
// The simulation assumes a single station in an infrastructure network:
//
//  STA     AP
//    *     *
//    |     |
//   n1     n2
//
// Packets in this simulation belong to BestEffort Access Class (AC_BE).

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("vht-wifi-network");

int
main(int argc, char* argv[])
{
    bool udp{true};
    bool useRts{false};
    bool use80Plus80{false};
    Time simulationTime{"10s"};
    meter_u distance{1.0};
    std::string mcsStr;
    std::vector<uint64_t> mcsValues;
    std::string phyModel{"Yans"};
    int channelWidth{-1};  // in MHz, -1 indicates an unset value
    int guardInterval{-1}; // in nanoseconds, -1 indicates an unset value
    double minExpectedThroughput{0.0};
    double maxExpectedThroughput{0.0};

    CommandLine cmd(__FILE__);
    cmd.AddValue("distance",
                 "Distance in meters between the station and the access point",
                 distance);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
    cmd.AddValue("useRts", "Enable/disable RTS/CTS", useRts);
    cmd.AddValue("use80Plus80", "Enable/disable use of 80+80 MHz", use80Plus80);
    cmd.AddValue(
        "mcs",
        "list of comma separated MCS values to test; if unset, all MCS values (0-9) are tested",
        mcsStr);
    cmd.AddValue("phyModel",
                 "PHY model to use (Yans or Spectrum). If 80+80 MHz is enabled, then Spectrum is "
                 "automatically selected",
                 phyModel);
    cmd.AddValue("channelWidth",
                 "if set, limit testing to a specific channel width expressed in MHz (20, 40, 80 "
                 "or 160 MHz)",
                 channelWidth);
    cmd.AddValue("guardInterval",
                 "if set, limit testing to a specific guard interval duration expressed in "
                 "nanoseconds (800 or 400 ns)",
                 guardInterval);
    cmd.AddValue("minExpectedThroughput",
                 "if set, simulation fails if the lowest throughput is below this value",
                 minExpectedThroughput);
    cmd.AddValue("maxExpectedThroughput",
                 "if set, simulation fails if the highest throughput is above this value",
                 maxExpectedThroughput);
    cmd.Parse(argc, argv);

    if (phyModel != "Yans" && phyModel != "Spectrum")
    {
        NS_ABORT_MSG("Invalid PHY model (must be Yans or Spectrum)");
    }
    if (use80Plus80)
    {
        // SpectrumWifiPhy is required for 80+80 MHz
        phyModel = "Spectrum";
    }

    if (useRts)
    {
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue("0"));
    }

    double prevThroughput[8] = {0};

    std::cout << "MCS value"
              << "\t\t"
              << "Channel width"
              << "\t\t"
              << "short GI"
              << "\t\t"
              << "Throughput" << '\n';
    uint8_t minMcs = 0;
    uint8_t maxMcs = 9;

    if (mcsStr.empty())
    {
        for (uint8_t mcs = minMcs; mcs <= maxMcs; ++mcs)
        {
            mcsValues.push_back(mcs);
        }
    }
    else
    {
        AttributeContainerValue<UintegerValue, ',', std::vector> attr;
        auto checker = DynamicCast<AttributeContainerChecker>(MakeAttributeContainerChecker(attr));
        checker->SetItemChecker(MakeUintegerChecker<uint8_t>());
        attr.DeserializeFromString(mcsStr, checker);
        mcsValues = attr.Get();
        std::sort(mcsValues.begin(), mcsValues.end());
    }

    int minChannelWidth = 20;
    int maxChannelWidth = 160;
    if (channelWidth >= minChannelWidth && channelWidth <= maxChannelWidth)
    {
        minChannelWidth = channelWidth;
        maxChannelWidth = channelWidth;
    }
    int minGi = 400;
    int maxGi = 800;
    if (guardInterval >= minGi && guardInterval <= maxGi)
    {
        minGi = guardInterval;
        maxGi = guardInterval;
    }

    for (const auto mcs : mcsValues)
    {
        uint8_t index = 0;
        double previous = 0;
        for (int width = minChannelWidth; width <= maxChannelWidth; width *= 2) // MHz
        {
            if (mcs == 9 && width == 20)
            {
                continue;
            }
            const auto is80Plus80 = (use80Plus80 && (width == 160));
            const std::string widthStr = is80Plus80 ? "80+80" : std::to_string(width);
            const auto segmentWidthStr = is80Plus80 ? "80" : widthStr;
            for (int gi = maxGi; gi >= minGi; gi /= 2) // Nanoseconds
            {
                const auto sgi = (gi == 400);
                uint32_t payloadSize; // 1500 byte IP packet
                if (udp)
                {
                    payloadSize = 1472; // bytes
                }
                else
                {
                    payloadSize = 1448; // bytes
                    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
                }

                NodeContainer wifiStaNode;
                wifiStaNode.Create(1);
                NodeContainer wifiApNode;
                wifiApNode.Create(1);

                NetDeviceContainer apDevice;
                NetDeviceContainer staDevice;
                WifiMacHelper mac;
                WifiHelper wifi;
                std::string channelStr{"{0, " + segmentWidthStr + ", BAND_5GHZ, 0}"};

                std::ostringstream ossControlMode;
                auto nonHtRefRateMbps = VhtPhy::GetNonHtReferenceRate(mcs) / 1e6;
                ossControlMode << "OfdmRate" << nonHtRefRateMbps << "Mbps";

                std::ostringstream ossDataMode;
                ossDataMode << "VhtMcs" << mcs;

                if (is80Plus80)
                {
                    channelStr += std::string(";") + channelStr;
                }

                wifi.SetStandard(WIFI_STANDARD_80211ac);
                wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                             "DataMode",
                                             StringValue(ossDataMode.str()),
                                             "ControlMode",
                                             StringValue(ossControlMode.str()));

                // Set guard interval
                wifi.ConfigHtOptions("ShortGuardIntervalSupported", BooleanValue(sgi));

                Ssid ssid = Ssid("ns3-80211ac");

                if (phyModel == "Spectrum")
                {
                    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
                    auto lossModel = CreateObject<LogDistancePropagationLossModel>();
                    spectrumChannel->AddPropagationLossModel(lossModel);

                    SpectrumWifiPhyHelper phy;
                    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
                    phy.SetChannel(spectrumChannel);

                    phy.Set("ChannelSettings",
                            StringValue("{0, " + std::to_string(width) + ", BAND_5GHZ, 0}"));

                    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
                    staDevice = wifi.Install(phy, mac, wifiStaNode);

                    mac.SetType("ns3::ApWifiMac",
                                "EnableBeaconJitter",
                                BooleanValue(false),
                                "Ssid",
                                SsidValue(ssid));
                    apDevice = wifi.Install(phy, mac, wifiApNode);
                }
                else
                {
                    auto channel = YansWifiChannelHelper::Default();
                    YansWifiPhyHelper phy;
                    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
                    phy.SetChannel(channel.Create());

                    phy.Set("ChannelSettings",
                            StringValue("{0, " + std::to_string(width) + ", BAND_5GHZ, 0}"));

                    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
                    staDevice = wifi.Install(phy, mac, wifiStaNode);

                    mac.SetType("ns3::ApWifiMac",
                                "EnableBeaconJitter",
                                BooleanValue(false),
                                "Ssid",
                                SsidValue(ssid));
                    apDevice = wifi.Install(phy, mac, wifiApNode);
                }

                int64_t streamNumber = 150;
                streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
                streamNumber += WifiHelper::AssignStreams(staDevice, streamNumber);

                // mobility.
                MobilityHelper mobility;
                Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

                positionAlloc->Add(Vector(0.0, 0.0, 0.0));
                positionAlloc->Add(Vector(distance, 0.0, 0.0));
                mobility.SetPositionAllocator(positionAlloc);

                mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

                mobility.Install(wifiApNode);
                mobility.Install(wifiStaNode);

                /* Internet stack*/
                InternetStackHelper stack;
                stack.Install(wifiApNode);
                stack.Install(wifiStaNode);
                streamNumber += stack.AssignStreams(wifiApNode, streamNumber);
                streamNumber += stack.AssignStreams(wifiStaNode, streamNumber);

                Ipv4AddressHelper address;
                address.SetBase("192.168.1.0", "255.255.255.0");
                Ipv4InterfaceContainer staNodeInterface;
                Ipv4InterfaceContainer apNodeInterface;

                staNodeInterface = address.Assign(staDevice);
                apNodeInterface = address.Assign(apDevice);

                /* Setting applications */
                const auto maxLoad = VhtPhy::GetDataRate(mcs,
                                                         MHz_u{static_cast<double>(width)},
                                                         NanoSeconds(sgi ? 400 : 800),
                                                         1);
                ApplicationContainer serverApp;
                if (udp)
                {
                    // UDP flow
                    uint16_t port = 9;
                    UdpServerHelper server(port);
                    serverApp = server.Install(wifiStaNode.Get(0));
                    streamNumber += server.AssignStreams(wifiStaNode.Get(0), streamNumber);

                    serverApp.Start(Seconds(0));
                    serverApp.Stop(simulationTime + Seconds(1));
                    const auto packetInterval = payloadSize * 8.0 / maxLoad;

                    UdpClientHelper client(staNodeInterface.GetAddress(0), port);
                    client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
                    client.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
                    client.SetAttribute("PacketSize", UintegerValue(payloadSize));
                    ApplicationContainer clientApp = client.Install(wifiApNode.Get(0));
                    streamNumber += client.AssignStreams(wifiApNode.Get(0), streamNumber);

                    clientApp.Start(Seconds(1));
                    clientApp.Stop(simulationTime + Seconds(1));
                }
                else
                {
                    // TCP flow
                    uint16_t port = 50000;
                    Address localAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
                    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", localAddress);
                    serverApp = packetSinkHelper.Install(wifiStaNode.Get(0));
                    streamNumber +=
                        packetSinkHelper.AssignStreams(wifiStaNode.Get(0), streamNumber);

                    serverApp.Start(Seconds(0));
                    serverApp.Stop(simulationTime + Seconds(1));

                    OnOffHelper onoff("ns3::TcpSocketFactory", Ipv4Address::GetAny());
                    onoff.SetAttribute("OnTime",
                                       StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                    onoff.SetAttribute("OffTime",
                                       StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                    onoff.SetAttribute("PacketSize", UintegerValue(payloadSize));
                    onoff.SetAttribute("DataRate", DataRateValue(maxLoad));
                    AddressValue remoteAddress(
                        InetSocketAddress(staNodeInterface.GetAddress(0), port));
                    onoff.SetAttribute("Remote", remoteAddress);
                    ApplicationContainer clientApp = onoff.Install(wifiApNode.Get(0));
                    streamNumber += onoff.AssignStreams(wifiApNode.Get(0), streamNumber);

                    clientApp.Start(Seconds(1));
                    clientApp.Stop(simulationTime + Seconds(1));
                }

                Ipv4GlobalRoutingHelper::PopulateRoutingTables();

                Simulator::Stop(simulationTime + Seconds(1));
                Simulator::Run();

                auto rxBytes = 0.0;
                if (udp)
                {
                    rxBytes = payloadSize * DynamicCast<UdpServer>(serverApp.Get(0))->GetReceived();
                }
                else
                {
                    rxBytes = DynamicCast<PacketSink>(serverApp.Get(0))->GetTotalRx();
                }
                auto throughput = (rxBytes * 8) / simulationTime.GetMicroSeconds(); // Mbit/s

                Simulator::Destroy();

                std::cout << +mcs << "\t\t\t" << widthStr << " MHz\t\t"
                          << (widthStr.size() > 3 ? "" : "\t") << (sgi ? "400 ns" : "800 ns")
                          << "\t\t\t" << throughput << " Mbit/s" << std::endl;

                // test first element
                if (mcs == minMcs && width == 20 && !sgi)
                {
                    if (throughput < minExpectedThroughput)
                    {
                        NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                        exit(1);
                    }
                }
                // test last element
                if (mcs == maxMcs && width == 160 && sgi)
                {
                    if (maxExpectedThroughput > 0 && throughput > maxExpectedThroughput)
                    {
                        NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                        exit(1);
                    }
                }
                // test previous throughput is smaller (for the same mcs)
                if (throughput > previous)
                {
                    previous = throughput;
                }
                else
                {
                    NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                    exit(1);
                }
                // test previous throughput is smaller (for the same channel width and GI)
                if (throughput > prevThroughput[index])
                {
                    prevThroughput[index] = throughput;
                }
                else
                {
                    NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                    exit(1);
                }
                index++;
            }
        }
    }
    return 0;
}
