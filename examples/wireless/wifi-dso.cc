/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
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
#include "ns3/enum.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/internet-static-setup-helper.h"
#include "ns3/ipv4-address-helper.h"
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
#include "ns3/uhr-phy.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-acknowledgment.h"
#include "ns3/wifi-static-setup-helper.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <numeric>
#include <unistd.h>
#include <vector>

// This is an example to verify performance of 802.11bn dynamic subband operation (DSO).
//
// The simulation considers a single 802.11bn AP and a configurable number of DSO-capable
// non-AP STAs in an infrastructure network.
//
// The channel width of the AP and the STAs can be configured, assuming all DSO-capable STAs
// have the same channel width. It is also possible to select between downlink or uplink traffic
// (UDP or TCP) and to determine the payload size to use. Finally, some UL OFDMA parameters can
// be configured (enablement and access request interval).
//
// For comparison purposes, the simulation can also be run without DSO, in which case
// 802.11be is used instead (since regular multi-user scheduler does not support 802.11bn).
//
// The script outputs the aggregated throughput and the per-STA throughput, and it can
// also verify that the aggregated throughput equals to an expected throughput, within
// a configurable tolerance. In this test mode, it is also checked that the per-STA throughput
// is similar for all STAs, within another configurable tolerance.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("wifi-dso");

namespace
{

/**
 * @param udp true if UDP is used, false if TCP is used
 * @param serverApp the server application
 * @param payloadSize the size in bytes of the packets
 * @return the bytes received by the server application
 */
uint64_t
GetRxBytes(bool udp, const Ptr<Application> serverApp, uint32_t payloadSize)
{
    return udp ? payloadSize * DynamicCast<UdpServer>(serverApp)->GetReceived()
               : DynamicCast<PacketSink>(serverApp)->GetTotalRx();
}

} // namespace

int
main(int argc, char* argv[])
{
    bool udp{true};
    bool downlink{true};
    Time simulationTime{"10s"};
    bool staticSetup{true};
    GHz_t frequency{5};
    MHz_t apBw{160};
    MHz_t staBw{80};
    std::size_t nStas{2};
    bool enableDso{true};
    bool enableUlOfdma{false};
    Time accessReqInterval{0};
    Time txopLimit{"12ms"};
    uint32_t payloadSize{1472};
    DataRate expectedThroughput;
    percent_t tolerance{10};
    percent_t perStaTolerance{5};
    bool pcap{false};

    CommandLine cmd(__FILE__);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
    cmd.AddValue("downlink",
                 "Generate downlink flows if set to 1, uplink flows otherwise",
                 downlink);
    cmd.AddValue("nStas", "Number of non-AP STAs", nStas);
    cmd.AddValue("apBw", "Bandwidth of the access point", apBw);
    cmd.AddValue("staBw", "Bandwidth of the stations", staBw);
    cmd.AddValue("frequency",
                 "Whether the link operates in the 5 or 6 GHz band (other values gets rejected)",
                 frequency);
    cmd.AddValue("enableDso", "Enable 11bn/DSO operations, otherwise 11be is used", enableDso);
    cmd.AddValue("enableUlOfdma", "Enable UL OFDMA", enableUlOfdma);
    cmd.AddValue(
        "muSchedAccessReqInterval",
        "Duration of the interval between two requests for channel access made by the MU scheduler",
        accessReqInterval);
    cmd.AddValue("payloadSize", "The application payload size in bytes", payloadSize);
    cmd.AddValue("pcap", "Enable/disable PCAP traces", pcap);
    cmd.AddValue(
        "expectedThroughput",
        "if set, simulation fails if the aggregated throughput is different than this value",
        expectedThroughput);
    cmd.AddValue("tolerance",
                 "if 'expectedThroughput' is set, simulation fails if the throughput is not within "
                 "this percentage of the expected throughput",
                 tolerance);
    cmd.AddValue("perStaTolerance",
                 "if 'expectedThroughput' is set, simulation fails if the throughput per STA is "
                 "not within this percentage of the expected throughput",
                 perStaTolerance);
    cmd.Parse(argc, argv);

    uint8_t mcs{13};
    Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                       EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
    Config::SetDefault("ns3::WifiDefaultAckManager::MaxBlockAckMcs", UintegerValue(mcs));
    Config::SetDefault("ns3::WifiMac::BE_MaxAmpduSize", UintegerValue(WIFI_PSDU_MAX_LENGTH_EHT));
    Config::SetDefault("ns3::UhrConfiguration::DsoActivated", BooleanValue(enableDso));
    if (!udp)
    {
        Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
    }

    NodeContainer wifiStaNodes;
    wifiStaNodes.Create(nStas);
    NodeContainer wifiApNode;
    wifiApNode.Create(1);

    NetDeviceContainer apDevice;
    NetDeviceContainer staDevices;
    WifiMacHelper mac;
    WifiHelper wifi;

    const std::string rate = (enableDso ? "UhrMcs" : "EhtMcs") + std::to_string(mcs);
    wifi.SetStandard(enableDso ? WIFI_STANDARD_80211bn : WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(rate),
                                 "ControlMode",
                                 StringValue(rate),
                                 "NonUnicastMode",
                                 StringValue("OfdmRate24Mbps")); // transmit ICFs at largest rate

    Ssid ssid = Ssid("ns3-80211bn-dso");

    SpectrumWifiPhyHelper phy;
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    std::string apChannelStr("{0, " + std::to_string(static_cast<uint16_t>(apBw.in_MHz())) + ", ");
    std::string staChannelStr("{0, " + std::to_string(static_cast<uint16_t>(staBw.in_MHz())) +
                              ", ");

    if (frequency == GHz_t{6})
    {
        apChannelStr += "BAND_6GHZ, 0}";
        staChannelStr += "BAND_6GHZ, 0}";
        Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss", DoubleValue(48));
    }
    else if (frequency == GHz_t{5})
    {
        apChannelStr += "BAND_5GHZ, 0}";
        staChannelStr += "BAND_5GHZ, 0}";
    }
    else
    {
        NS_FATAL_ERROR("Wrong frequency value: " << frequency);
    }

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<LogDistancePropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);
    phy.AddChannel(spectrumChannel);

    mac.SetEdca(AC_BE,
                "TxopLimits",
                StringValue(std::to_string(txopLimit.GetMicroSeconds()) + "us"));

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    phy.Set("ChannelSettings", StringValue(staChannelStr));
    staDevices = wifi.Install(phy, mac, wifiStaNodes);

    mac.SetMultiUserScheduler(enableDso ? "ns3::DsoMultiUserScheduler"
                                        : "ns3::RrMultiUserScheduler",
                              "EnableUlOfdma",
                              BooleanValue(enableUlOfdma),
                              "AccessReqInterval",
                              TimeValue(accessReqInterval));
    mac.SetType("ns3::ApWifiMac",
                "EnableBeaconJitter",
                BooleanValue(false),
                "BeaconGeneration",
                BooleanValue(!staticSetup),
                "Ssid",
                SsidValue(ssid));
    phy.Set("ChannelSettings", StringValue(apChannelStr));
    apDevice = wifi.Install(phy, mac, wifiApNode);

    int64_t streamNumber = 100;
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    if (pcap)
    {
        phy.EnablePcap("wifi-dso_AP", apDevice);
        phy.EnablePcap("wifi-dso_STA", staDevices);
    }

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    for (std::size_t i = 0; i < nStas; ++i)
    {
        positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    }
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNodes);

    Time startTime{"1s"};
    if (staticSetup)
    {
        /* static setup of association and BA agreements */
        auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
        NS_ASSERT(apDev);
        WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevices);
        WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevices, {0});
        startTime = MilliSeconds(1);
    }

    /* Internet stack*/
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNodes);
    streamNumber += stack.AssignStreams(wifiApNode, streamNumber);
    streamNumber += stack.AssignStreams(wifiStaNodes, streamNumber);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staNodeInterfaces;
    Ipv4InterfaceContainer apNodeInterface;

    staNodeInterfaces = address.Assign(staDevices);
    apNodeInterface = address.Assign(apDevice);

    if (staticSetup)
    {
        /* static setup of ARP cache */
        InternetStaticSetupHelper::PopulateArpCache();
    }

    /* Setting applications */
    ApplicationContainer serverApp;
    auto serverNodes = downlink ? std::ref(wifiStaNodes) : std::ref(wifiApNode);
    Ipv4InterfaceContainer serverInterfaces;
    NodeContainer clientNodes;
    for (std::size_t i = 0; i < nStas; ++i)
    {
        serverInterfaces.Add(downlink ? staNodeInterfaces.Get(i) : apNodeInterface.Get(0));
        clientNodes.Add(downlink ? wifiApNode.Get(0) : wifiStaNodes.Get(i));
    }

    const auto maxLoad = UhrPhy::GetDataRate(13, MHz_t{320}, NanoSeconds(800), 1) / nStas;
    if (udp)
    {
        // UDP flow
        uint16_t port = 9;
        for (std::size_t i = 0; i < nStas; ++i)
        {
            UdpServerHelper server(port + i);
            serverApp.Add(server.Install(downlink ? serverNodes.get().Get(i) : serverNodes.get()));
            streamNumber += server.AssignStreams(serverNodes.get(), streamNumber);
        }

        serverApp.Start(Seconds(0.0));
        serverApp.Stop(simulationTime + startTime);
        const auto packetInterval = payloadSize * 8.0 / maxLoad;

        for (std::size_t i = 0; i < nStas; ++i)
        {
            UdpClientHelper client(serverInterfaces.GetAddress(i), port + i);
            client.SetAttribute("MaxPackets", UintegerValue(0));
            client.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
            client.SetAttribute("PacketSize", UintegerValue(payloadSize));
            ApplicationContainer clientApp = client.Install(clientNodes.Get(i));
            streamNumber += client.AssignStreams(clientNodes.Get(i), streamNumber);

            clientApp.Start(startTime);
            clientApp.Stop(simulationTime + startTime);
        }
    }
    else
    {
        // TCP flow
        uint16_t port = 50000;
        for (std::size_t i = 0; i < nStas; ++i)
        {
            Address localAddress(InetSocketAddress(Ipv4Address::GetAny(), port + i));
            PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", localAddress);
            serverApp.Add(
                packetSinkHelper.Install(downlink ? serverNodes.get().Get(i) : serverNodes.get()));
            streamNumber += packetSinkHelper.AssignStreams(serverNodes.get(), streamNumber);
        }

        serverApp.Start(Seconds(0.0));
        serverApp.Stop(simulationTime + startTime);

        for (std::size_t i = 0; i < nStas; ++i)
        {
            OnOffHelper onoff("ns3::TcpSocketFactory", Ipv4Address::GetAny());
            onoff.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
            onoff.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
            onoff.SetAttribute("PacketSize", UintegerValue(payloadSize));
            onoff.SetAttribute("DataRate", DataRateValue(maxLoad));
            AddressValue remoteAddress(InetSocketAddress(serverInterfaces.GetAddress(i), port + i));
            onoff.SetAttribute("Remote", remoteAddress);
            ApplicationContainer clientApp = onoff.Install(clientNodes.Get(i));
            streamNumber += onoff.AssignStreams(clientNodes.Get(i), streamNumber);

            clientApp.Start(startTime);
            clientApp.Stop(simulationTime + startTime);
        }
    }

    // cumulative number of bytes received by each server application
    std::vector<uint64_t> cumulRxBytes(nStas, 0);

    Simulator::Stop(simulationTime + startTime);
    Simulator::Run();

    DataRate aggregatedThroughput;
    std::vector<DataRate> perStaThroughput(nStas, DataRate());
    for (std::size_t i = 0; i < nStas; ++i)
    {
        const auto rxBytes = GetRxBytes(udp, serverApp.Get(i), payloadSize);
        const DataRate throughput = (rxBytes * 8.0) / simulationTime.GetSeconds();
        std::cout << "Throughput STA " << i + 1 << " : " << throughput << "\n";
        perStaThroughput[i] = throughput;
        aggregatedThroughput += throughput;
    }
    std::cout << "Aggregate throughput: " << aggregatedThroughput << std::endl;

    Simulator::Destroy();

    if (expectedThroughput != DataRate())
    {
        const auto minExpectedThroughput = expectedThroughput * (1.0 - tolerance.to_ratio());
        const auto maxExpectedThroughput = expectedThroughput * (1.0 + tolerance.to_ratio());
        if ((aggregatedThroughput < minExpectedThroughput) ||
            (aggregatedThroughput > maxExpectedThroughput))
        {
            NS_LOG_ERROR("Aggregated throughput " << aggregatedThroughput
                                                  << " is outside expected range!");
            exit(1);
        }
        for (std::size_t i = 0; i < nStas; ++i)
        {
            const auto& staThroughput = perStaThroughput.at(i);
            const auto minStaExpectedThroughput =
                staThroughput * (1.0 - perStaTolerance.to_ratio());
            const auto maxStaExpectedThroughput =
                staThroughput * (1.0 + perStaTolerance.to_ratio());
            const auto equalThroughput =
                std::all_of(perStaThroughput.cbegin(),
                            perStaThroughput.cend(),
                            [&minStaExpectedThroughput,
                             &maxStaExpectedThroughput](const auto& otherStaThroughput) {
                                return (otherStaThroughput >= minStaExpectedThroughput) &&
                                       (otherStaThroughput <= maxStaExpectedThroughput);
                            });
            if (!equalThroughput)
            {
                NS_LOG_ERROR("STA " << i + 1 << " throughput " << staThroughput
                                    << " is outside expected range!");
                exit(1);
            }
        }
    }

    return 0;
}
