/*
 * Copyright (c) 2016 SEBASTIEN DERONNE
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
#include "ns3/he-phy.h"
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
#include "ns3/wifi-acknowledgment.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <functional>

// This is a simple example in order to show how to configure an IEEE 802.11ax Wi-Fi network.
//
// It outputs the UDP or TCP goodput for every HE MCS value, which depends on the MCS value (0 to
// 11), the channel width (20, 40, 80 or 160 MHz) and the guard interval (800ns, 1600ns or 3200ns).
// The PHY bitrate is constant over all the simulation run. The user can also specify the distance
// between the access point and the station: the larger the distance the smaller the goodput.
//
// The simulation assumes a configurable number of stations in an infrastructure network:
//
//  STA     AP
//    *     *
//    |     |
//   n1     n2
//
// Packets in this simulation belong to BestEffort Access Class (AC_BE).
// By selecting an acknowledgment sequence for DL MU PPDUs, it is possible to aggregate a
// Round Robin scheduler to the AP, so that DL MU PPDUs are sent by the AP via DL OFDMA.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("he-wifi-network");

int
main(int argc, char* argv[])
{
    bool udp{true};
    bool downlink{true};
    bool useRts{false};
    bool use80Plus80{false};
    bool useExtendedBlockAck{false};
    Time simulationTime{"10s"};
    meter_u distance{1.0};
    double frequency{5}; // whether 2.4, 5 or 6 GHz
    std::size_t nStations{1};
    std::string dlAckSeqType{"NO-OFDMA"};
    bool enableUlOfdma{false};
    bool enableBsrp{false};
    std::string mcsStr;
    std::vector<uint64_t> mcsValues;
    int channelWidth{-1};  // in MHz, -1 indicates an unset value
    int guardInterval{-1}; // in nanoseconds, -1 indicates an unset value
    uint32_t payloadSize =
        700; // must fit in the max TX duration when transmitting at MCS 0 over an RU of 26 tones
    std::string phyModel{"Yans"};
    double minExpectedThroughput{0.0};
    double maxExpectedThroughput{0.0};
    Time accessReqInterval{0};

    CommandLine cmd(__FILE__);
    cmd.AddValue("frequency",
                 "Whether working in the 2.4, 5 or 6 GHz band (other values gets rejected)",
                 frequency);
    cmd.AddValue("distance",
                 "Distance in meters between the station and the access point",
                 distance);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("udp", "UDP if set to 1, TCP otherwise", udp);
    cmd.AddValue("downlink",
                 "Generate downlink flows if set to 1, uplink flows otherwise",
                 downlink);
    cmd.AddValue("useRts", "Enable/disable RTS/CTS", useRts);
    cmd.AddValue("use80Plus80", "Enable/disable use of 80+80 MHz", use80Plus80);
    cmd.AddValue("useExtendedBlockAck", "Enable/disable use of extended BACK", useExtendedBlockAck);
    cmd.AddValue("nStations", "Number of non-AP HE stations", nStations);
    cmd.AddValue("dlAckType",
                 "Ack sequence type for DL OFDMA (NO-OFDMA, ACK-SU-FORMAT, MU-BAR, AGGR-MU-BAR)",
                 dlAckSeqType);
    cmd.AddValue("enableUlOfdma",
                 "Enable UL OFDMA (useful if DL OFDMA is enabled and TCP is used)",
                 enableUlOfdma);
    cmd.AddValue("enableBsrp",
                 "Enable BSRP (useful if DL and UL OFDMA are enabled and TCP is used)",
                 enableBsrp);
    cmd.AddValue(
        "muSchedAccessReqInterval",
        "Duration of the interval between two requests for channel access made by the MU scheduler",
        accessReqInterval);
    cmd.AddValue(
        "mcs",
        "list of comma separated MCS values to test; if unset, all MCS values (0-11) are tested",
        mcsStr);
    cmd.AddValue("channelWidth",
                 "if set, limit testing to a specific channel width expressed in MHz (20, 40, 80 "
                 "or 160 MHz)",
                 channelWidth);
    cmd.AddValue("guardInterval",
                 "if set, limit testing to a specific guard interval duration expressed in "
                 "nanoseconds (800, 1600 or 3200 ns)",
                 guardInterval);
    cmd.AddValue("payloadSize", "The application payload size in bytes", payloadSize);
    cmd.AddValue("phyModel",
                 "PHY model to use when OFDMA is disabled (Yans or Spectrum). If 80+80 MHz or "
                 "OFDMA is enabled "
                 "then Spectrum is automatically selected",
                 phyModel);
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
        Config::SetDefault("ns3::WifiDefaultProtectionManager::EnableMuRts", BooleanValue(true));
    }

    if (dlAckSeqType == "ACK-SU-FORMAT")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE));
    }
    else if (dlAckSeqType == "MU-BAR")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_TF_MU_BAR));
    }
    else if (dlAckSeqType == "AGGR-MU-BAR")
    {
        Config::SetDefault("ns3::WifiDefaultAckManager::DlMuAckSequenceType",
                           EnumValue(WifiAcknowledgment::DL_MU_AGGREGATE_TF));
    }
    else if (dlAckSeqType != "NO-OFDMA")
    {
        NS_ABORT_MSG("Invalid DL ack sequence type (must be NO-OFDMA, ACK-SU-FORMAT, MU-BAR or "
                     "AGGR-MU-BAR)");
    }

    if (phyModel != "Yans" && phyModel != "Spectrum")
    {
        NS_ABORT_MSG("Invalid PHY model (must be Yans or Spectrum)");
    }
    if (use80Plus80 || (dlAckSeqType != "NO-OFDMA"))
    {
        // SpectrumWifiPhy is required for 80+80 MHz and OFDMA
        phyModel = "Spectrum";
    }

    double prevThroughput[12] = {0};

    std::cout << "MCS value"
              << "\t\t"
              << "Channel width"
              << "\t\t"
              << "GI"
              << "\t\t\t"
              << "Throughput" << '\n';
    uint8_t minMcs = 0;
    uint8_t maxMcs = 11;

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
    int maxChannelWidth = frequency == 2.4 ? 40 : 160;
    if (channelWidth >= minChannelWidth && channelWidth <= maxChannelWidth)
    {
        minChannelWidth = channelWidth;
        maxChannelWidth = channelWidth;
    }
    int minGi = enableUlOfdma ? 1600 : 800;
    int maxGi = 3200;
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
            const auto is80Plus80 = (use80Plus80 && (width == 160));
            const std::string widthStr = is80Plus80 ? "80+80" : std::to_string(width);
            const auto segmentWidthStr = is80Plus80 ? "80" : widthStr;
            for (int gi = maxGi; gi >= minGi; gi /= 2) // Nanoseconds
            {
                if (!udp)
                {
                    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(payloadSize));
                }

                NodeContainer wifiStaNodes;
                wifiStaNodes.Create(nStations);
                NodeContainer wifiApNode;
                wifiApNode.Create(1);

                NetDeviceContainer apDevice;
                NetDeviceContainer staDevices;
                WifiMacHelper mac;
                WifiHelper wifi;
                std::string channelStr("{0, " + segmentWidthStr + ", ");
                StringValue ctrlRate;
                auto nonHtRefRateMbps = HePhy::GetNonHtReferenceRate(mcs) / 1e6;

                std::ostringstream ossDataMode;
                ossDataMode << "HeMcs" << mcs;

                if (frequency == 6)
                {
                    ctrlRate = StringValue(ossDataMode.str());
                    channelStr += "BAND_6GHZ, 0}";
                    Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                                       DoubleValue(48));
                }
                else if (frequency == 5)
                {
                    std::ostringstream ossControlMode;
                    ossControlMode << "OfdmRate" << nonHtRefRateMbps << "Mbps";
                    ctrlRate = StringValue(ossControlMode.str());
                    channelStr += "BAND_5GHZ, 0}";
                }
                else if (frequency == 2.4)
                {
                    std::ostringstream ossControlMode;
                    ossControlMode << "ErpOfdmRate" << nonHtRefRateMbps << "Mbps";
                    ctrlRate = StringValue(ossControlMode.str());
                    channelStr += "BAND_2_4GHZ, 0}";
                    Config::SetDefault("ns3::LogDistancePropagationLossModel::ReferenceLoss",
                                       DoubleValue(40));
                }
                else
                {
                    NS_FATAL_ERROR("Wrong frequency value!");
                }

                if (is80Plus80)
                {
                    channelStr += std::string(";") + channelStr;
                }

                wifi.SetStandard(WIFI_STANDARD_80211ax);
                wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                             "DataMode",
                                             StringValue(ossDataMode.str()),
                                             "ControlMode",
                                             ctrlRate);
                // Set guard interval
                wifi.ConfigHeOptions("GuardInterval", TimeValue(NanoSeconds(gi)));

                Ssid ssid = Ssid("ns3-80211ax");

                if (phyModel == "Spectrum")
                {
                    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();

                    auto lossModel = CreateObject<LogDistancePropagationLossModel>();
                    spectrumChannel->AddPropagationLossModel(lossModel);

                    SpectrumWifiPhyHelper phy;
                    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
                    phy.SetChannel(spectrumChannel);

                    mac.SetType("ns3::StaWifiMac",
                                "Ssid",
                                SsidValue(ssid),
                                "MpduBufferSize",
                                UintegerValue(useExtendedBlockAck ? 256 : 64));
                    phy.Set("ChannelSettings", StringValue(channelStr));
                    staDevices = wifi.Install(phy, mac, wifiStaNodes);

                    if (dlAckSeqType != "NO-OFDMA")
                    {
                        mac.SetMultiUserScheduler("ns3::RrMultiUserScheduler",
                                                  "EnableUlOfdma",
                                                  BooleanValue(enableUlOfdma),
                                                  "EnableBsrp",
                                                  BooleanValue(enableBsrp),
                                                  "AccessReqInterval",
                                                  TimeValue(accessReqInterval));
                    }
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

                    mac.SetType("ns3::StaWifiMac",
                                "Ssid",
                                SsidValue(ssid),
                                "MpduBufferSize",
                                UintegerValue(useExtendedBlockAck ? 256 : 64));
                    phy.Set("ChannelSettings", StringValue(channelStr));
                    staDevices = wifi.Install(phy, mac, wifiStaNodes);

                    mac.SetType("ns3::ApWifiMac",
                                "EnableBeaconJitter",
                                BooleanValue(false),
                                "Ssid",
                                SsidValue(ssid));
                    apDevice = wifi.Install(phy, mac, wifiApNode);
                }

                int64_t streamNumber = 150;
                streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
                streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

                // mobility.
                MobilityHelper mobility;
                Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();

                positionAlloc->Add(Vector(0.0, 0.0, 0.0));
                positionAlloc->Add(Vector(distance, 0.0, 0.0));
                mobility.SetPositionAllocator(positionAlloc);

                mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

                mobility.Install(wifiApNode);
                mobility.Install(wifiStaNodes);

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

                /* Setting applications */
                ApplicationContainer serverApp;
                auto serverNodes = downlink ? std::ref(wifiStaNodes) : std::ref(wifiApNode);
                Ipv4InterfaceContainer serverInterfaces;
                NodeContainer clientNodes;
                for (std::size_t i = 0; i < nStations; i++)
                {
                    serverInterfaces.Add(downlink ? staNodeInterfaces.Get(i)
                                                  : apNodeInterface.Get(0));
                    clientNodes.Add(downlink ? wifiApNode.Get(0) : wifiStaNodes.Get(i));
                }

                const auto maxLoad =
                    HePhy::GetDataRate(mcs, MHz_u{static_cast<double>(width)}, NanoSeconds(gi), 1) /
                    nStations;
                if (udp)
                {
                    // UDP flow
                    uint16_t port = 9;
                    UdpServerHelper server(port);
                    serverApp = server.Install(serverNodes.get());
                    streamNumber += server.AssignStreams(serverNodes.get(), streamNumber);

                    serverApp.Start(Seconds(0));
                    serverApp.Stop(simulationTime + Seconds(1));
                    const auto packetInterval = payloadSize * 8.0 / maxLoad;

                    for (std::size_t i = 0; i < nStations; i++)
                    {
                        UdpClientHelper client(serverInterfaces.GetAddress(i), port);
                        client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
                        client.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
                        client.SetAttribute("PacketSize", UintegerValue(payloadSize));
                        ApplicationContainer clientApp = client.Install(clientNodes.Get(i));
                        streamNumber += client.AssignStreams(clientNodes.Get(i), streamNumber);

                        clientApp.Start(Seconds(1));
                        clientApp.Stop(simulationTime + Seconds(1));
                    }
                }
                else
                {
                    // TCP flow
                    uint16_t port = 50000;
                    Address localAddress(InetSocketAddress(Ipv4Address::GetAny(), port));
                    PacketSinkHelper packetSinkHelper("ns3::TcpSocketFactory", localAddress);
                    serverApp = packetSinkHelper.Install(serverNodes.get());
                    streamNumber += packetSinkHelper.AssignStreams(serverNodes.get(), streamNumber);

                    serverApp.Start(Seconds(0));
                    serverApp.Stop(simulationTime + Seconds(1));

                    for (std::size_t i = 0; i < nStations; i++)
                    {
                        OnOffHelper onoff("ns3::TcpSocketFactory", Ipv4Address::GetAny());
                        onoff.SetAttribute("OnTime",
                                           StringValue("ns3::ConstantRandomVariable[Constant=1]"));
                        onoff.SetAttribute("OffTime",
                                           StringValue("ns3::ConstantRandomVariable[Constant=0]"));
                        onoff.SetAttribute("PacketSize", UintegerValue(payloadSize));
                        onoff.SetAttribute("DataRate", DataRateValue(maxLoad));
                        AddressValue remoteAddress(
                            InetSocketAddress(serverInterfaces.GetAddress(i), port));
                        onoff.SetAttribute("Remote", remoteAddress);
                        ApplicationContainer clientApp = onoff.Install(clientNodes.Get(i));
                        streamNumber += onoff.AssignStreams(clientNodes.Get(i), streamNumber);

                        clientApp.Start(Seconds(1));
                        clientApp.Stop(simulationTime + Seconds(1));
                    }
                }

                Simulator::Schedule(Seconds(0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);

                Simulator::Stop(simulationTime + Seconds(1));
                Simulator::Run();

                // When multiple stations are used, there are chances that association requests
                // collide and hence the throughput may be lower than expected. Therefore, we relax
                // the check that the throughput cannot decrease by introducing a scaling factor (or
                // tolerance)
                auto tolerance = 0.10;
                auto rxBytes = 0.0;
                if (udp)
                {
                    for (uint32_t i = 0; i < serverApp.GetN(); i++)
                    {
                        rxBytes +=
                            payloadSize * DynamicCast<UdpServer>(serverApp.Get(i))->GetReceived();
                    }
                }
                else
                {
                    for (uint32_t i = 0; i < serverApp.GetN(); i++)
                    {
                        rxBytes += DynamicCast<PacketSink>(serverApp.Get(i))->GetTotalRx();
                    }
                }
                auto throughput = (rxBytes * 8) / simulationTime.GetMicroSeconds(); // Mbit/s

                Simulator::Destroy();

                std::cout << +mcs << "\t\t\t" << widthStr << " MHz\t\t"
                          << (widthStr.size() > 3 ? "" : "\t") << gi << " ns\t\t\t" << throughput
                          << " Mbit/s" << std::endl;

                // test first element
                if (mcs == minMcs && width == 20 && gi == 3200)
                {
                    if (throughput * (1 + tolerance) < minExpectedThroughput)
                    {
                        NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                        exit(1);
                    }
                }
                // test last element
                if (mcs == maxMcs && width == maxChannelWidth && gi == 800)
                {
                    if (maxExpectedThroughput > 0 &&
                        throughput > maxExpectedThroughput * (1 + tolerance))
                    {
                        NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                        exit(1);
                    }
                }
                // Skip comparisons with previous cases if more than one stations are present
                // because, e.g., random collisions in the establishment of Block Ack agreements
                // have an impact on throughput
                if (nStations == 1)
                {
                    // test previous throughput is smaller (for the same mcs)
                    if (throughput * (1 + tolerance) > previous)
                    {
                        previous = throughput;
                    }
                    else if (throughput > 0)
                    {
                        NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                        exit(1);
                    }
                    // test previous throughput is smaller (for the same channel width and GI)
                    if (throughput * (1 + tolerance) > prevThroughput[index])
                    {
                        prevThroughput[index] = throughput;
                    }
                    else if (throughput > 0)
                    {
                        NS_LOG_ERROR("Obtained throughput " << throughput << " is not expected!");
                        exit(1);
                    }
                }
                index++;
            }
        }
    }
    return 0;
}
