/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mirko Banchi <mk.banchi@gmail.com>
 *          Sebastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/attribute-container.h"
#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/ht-phy.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/packet-sink.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/tuple.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <vector>

// This is a simple example in order to show how to configure an IEEE 802.11n Wi-Fi network.
//
// It outputs the UDP or TCP goodput for every HT MCS value, which depends on the MCS value (0 to
// 7), the channel width (20 or 40 MHz) and the guard interval (long or short). The PHY bitrate is
// constant over all the simulation run. The user can also specify the distance between the access
// point and the station: the larger the distance the smaller the goodput.
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

NS_LOG_COMPONENT_DEFINE("ht-wifi-network");

int
main(int argc, char* argv[])
{
    bool udp{true};
    bool useRts{false};
    Time simulationTime{"10s"};
    meter_u distance{1.0};
    double frequency{5}; // whether 2.4 or 5 GHz
    std::string mcsStr;
    std::vector<uint64_t> mcsValues;
    int channelWidth{-1};  // in MHz, -1 indicates an unset value
    int guardInterval{-1}; // in nanoseconds, -1 indicates an unset value
    double minExpectedThroughput{0.0};
    double maxExpectedThroughput{0.0};

    CommandLine cmd(__FILE__);
    cmd.AddValue("frequency",
                 "Whether working in the 2.4 or 5.0 GHz band (other values gets rejected)",
                 frequency);
    cmd.AddValue("distance",
                 "Distance in meters between the station and the access point",
                 distance);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
    cmd.AddValue("useRts", "Enable/disable RTS/CTS", useRts);
    cmd.AddValue(
        "mcs",
        "list of comma separated MCS values to test; if unset, all MCS values (0-7) are tested",
        mcsStr);
    cmd.AddValue(
        "channelWidth",
        "if set, limit testing to a specific channel width expressed in MHz (20 or 40 MHz)",
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
    uint8_t maxMcs = 7;

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
    int maxChannelWidth = 40;
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

                YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
                YansWifiPhyHelper phy;
                phy.SetChannel(channel.Create());

                WifiMacHelper mac;
                WifiHelper wifi;
                std::ostringstream ossControlMode;

                if (frequency == 5.0)
                {
                    ossControlMode << "OfdmRate";
                    wifi.SetStandard(WIFI_STANDARD_80211n);
                }
                else if (frequency == 2.4)
                {
                    wifi.SetStandard(WIFI_STANDARD_80211n);
                    ossControlMode << "ErpOfdmRate";
                    Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                                       DoubleValue(40.046));
                }
                else
                {
                    NS_FATAL_ERROR("Wrong frequency value!");
                }

                auto nonHtRefRateMbps = HtPhy::GetNonHtReferenceRate(mcs) / 1e6;
                ossControlMode << nonHtRefRateMbps << "Mbps";

                std::ostringstream ossDataMode;
                ossDataMode << "HtMcs" << mcs;
                wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                             "DataMode",
                                             StringValue(ossDataMode.str()),
                                             "ControlMode",
                                             StringValue(ossControlMode.str()));
                // Set guard interval
                wifi.ConfigHtOptions("ShortGuardIntervalSupported", BooleanValue(sgi));

                Ssid ssid = Ssid("ns3-80211n");
                AttributeContainerValue<
                    TupleValue<UintegerValue, UintegerValue, EnumValue<WifiPhyBand>, UintegerValue>,
                    ';'>
                    channelValue;
                WifiPhyBand band = (frequency == 5.0 ? WIFI_PHY_BAND_5GHZ : WIFI_PHY_BAND_2_4GHZ);
                channelValue.Set(WifiPhy::ChannelSegments{{0, width, band, 0}});

                mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
                phy.Set("ChannelSettings", channelValue);

                NetDeviceContainer staDevice;
                staDevice = wifi.Install(phy, mac, wifiStaNode);

                mac.SetType("ns3::ApWifiMac",
                            "EnableBeaconJitter",
                            BooleanValue(false),
                            "Ssid",
                            SsidValue(ssid));

                NetDeviceContainer apDevice;
                apDevice = wifi.Install(phy, mac, wifiApNode);

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
                const auto maxLoad = HtPhy::GetDataRate(mcs,
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

                std::cout << mcs << "\t\t\t" << width << " MHz\t\t\t" << std::boolalpha << sgi
                          << "\t\t\t" << throughput << " Mbit/s" << std::endl;

                // test first element
                if (mcs == minMcs && width == 20 && !sgi)
                {
                    if (throughput < minExpectedThroughput)
                    {
                        NS_FATAL_ERROR("Obtained throughput " << throughput << " is not expected!");
                    }
                }
                // test last element
                if (mcs == maxMcs && width == 40 && sgi)
                {
                    if (maxExpectedThroughput > 0 && throughput > maxExpectedThroughput)
                    {
                        NS_FATAL_ERROR("Obtained throughput " << throughput << " is not expected!");
                    }
                }
                // test previous throughput is smaller (for the same mcs)
                if (throughput > previous)
                {
                    previous = throughput;
                }
                else
                {
                    NS_FATAL_ERROR("Obtained throughput " << throughput << " is not expected!");
                }
                // test previous throughput is smaller (for the same channel width and GI)
                if (throughput > prevThroughput[index])
                {
                    prevThroughput[index] = throughput;
                }
                else
                {
                    NS_FATAL_ERROR("Obtained throughput " << throughput << " is not expected!");
                }
                index++;
            }
        }
    }
    return 0;
}
