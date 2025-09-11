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
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
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
#include "ns3/udp-client-server-helper.h"
#include "ns3/udp-server.h"
#include "ns3/uinteger.h"
#include "ns3/vht-phy.h"
#include "ns3/wifi-static-setup-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <unistd.h>
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

namespace
{

/**
 * Run a given system command and validate exit code.
 * @param command the command to run
 * @param directory the temporary directory used to run the commands
 */
void
RunCommandAndValidate(const std::string& command, const std::string& directory)
{
    // TODO: find an alternative to std::system since there is no guarantee on its output value.
    // For all supported cases, std::system returns the exit code of the invoked program.
    if (std::system(command.c_str()))
    {
        std::filesystem::remove_all(directory);
        exit(1);
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    bool udp{true};
    bool useRts{false};
    bool use80Plus80{false};
    Time simulationTime{"10s"};
    bool staticSetup{true};
    auto startTime = Seconds(1);
    meter_u distance{1.0};
    std::string mcsStr;
    std::vector<uint64_t> mcsValues;
    std::string phyModel{"Yans"};
    MHz_t channelWidth;
    Time guardInterval;
    DataRate minExpectedThroughput;
    DataRate maxExpectedThroughput;
    bool multiProcessing{false};
    unsigned int numParallelJobs{1}; // if unset, keep default of parallel scheduler
    std::string fileName{""};

    CommandLine cmd(__FILE__);
    cmd.AddValue("staticSetup",
                 "Whether devices are configured using the static setup helper",
                 staticSetup);
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
                 "if set, limit testing to a specific channel width (20, 40, 80 or 160 MHz)",
                 channelWidth);
    cmd.AddValue("guardInterval",
                 "if set, limit testing to a specific guard interval duration (400ns or 800 ns)",
                 guardInterval);
    cmd.AddValue("minExpectedThroughput",
                 "if set, simulation fails if the lowest throughput is below this value",
                 minExpectedThroughput);
    cmd.AddValue("maxExpectedThroughput",
                 "if set, simulation fails if the highest throughput is above this value",
                 maxExpectedThroughput);
    cmd.AddValue("multiProcessing", "Enable/disable multiprocessing", multiProcessing);
    cmd.AddValue("numParallelJobs",
                 "the number of jobs to execute in parallel (keep default if unset)",
                 numParallelJobs);
    cmd.AddValue("fileName", "if specified, results will be written to that file", fileName);
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

    std::string tmpDir{""};
    if (fileName.empty() || multiProcessing)
    {
        char dirTemplate[] = "/tmp/ns3-wifi-vht-network-XXXXXX";
        tmpDir = mkdtemp(dirTemplate);
        tmpDir.append("/");
    }
    const std::string inputFileName{tmpDir + "inputFile.txt"};
    const std::string validationFileName{tmpDir + "outputFile.txt"};

    std::ofstream inputFile;
    std::ofstream validationFile;
    if (multiProcessing)
    {
        inputFile.open(inputFileName, std::ofstream::out);
        if (inputFile.fail())
        {
            NS_FATAL_ERROR("File " << inputFileName << " not found");
        }
    }

    if (fileName.empty() || multiProcessing)
    {
        validationFile.open(validationFileName, std::ofstream::out);
        if (validationFile.fail())
        {
            NS_FATAL_ERROR("File " << validationFileName << " not found");
        }
    }

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

    MHz_t minChannelWidth{20};
    MHz_t maxChannelWidth{160};
    if ((channelWidth != MHz_t{0}) &&
        ((channelWidth < minChannelWidth) || (channelWidth > maxChannelWidth)))
    {
        NS_FATAL_ERROR("Invalid channel width: " << channelWidth);
    }
    if (channelWidth >= minChannelWidth && channelWidth <= maxChannelWidth)
    {
        minChannelWidth = channelWidth;
        maxChannelWidth = channelWidth;
    }
    auto minGi = NanoSeconds(400);
    auto maxGi = NanoSeconds(800);
    if (guardInterval >= minGi && guardInterval <= maxGi)
    {
        minGi = guardInterval;
        maxGi = guardInterval;
    }

    bool isFirst{true};
    for (const auto mcs : mcsValues)
    {
        for (auto width = minChannelWidth; width <= maxChannelWidth; width *= 2)
        {
            if (mcs == 9 && width == MHz_t{20})
            {
                continue;
            }
            const auto is80Plus80 = (use80Plus80 && (width == MHz_t{160}));
            const auto segmentWidthStr =
                is80Plus80 ? std::string("80")
                           : std::to_string(static_cast<uint16_t>(width.in_MHz()));
            for (auto gi = maxGi; gi >= minGi; gi = gi / 2)
            {
                std::string outputFileName{""};
                if (fileName.empty() || multiProcessing)
                {
                    auto command =
                        cmd.GetName() + " --multiProcessing=0 --mcs=" + std::to_string(mcs) +
                        " --channelWidth=" + width.str(false) +
                        " --guardInterval=" + std::to_string(gi.GetNanoSeconds()) + "ns ";
                    std::vector<std::string> programArguments(argv + 1, argv + argc);
                    for (const auto& argument : programArguments)
                    {
                        if (argument.starts_with("--multiProcessing") ||
                            argument.starts_with("--mcs") ||
                            argument.starts_with("--channelWidth") ||
                            argument.starts_with("--guardInterval") ||
                            argument.starts_with("--fileName") ||
                            argument.starts_with("--minExpectedThroughput") ||
                            argument.starts_with("--maxExpectedThroughput"))
                        {
                            continue;
                        }
                        command += argument + " ";
                    }
                    inputFile << command;

                    outputFileName = tmpDir + "mcs=" + std::to_string(mcs) +
                                     "-width=" + width.str(false) +
                                     "-gi=" + std::to_string(gi.GetNanoSeconds()) + "ns.dat";
                    inputFile << "--fileName=" << outputFileName << std::endl;
                    const auto validationFileContent =
                        std::to_string(mcs) + " " + width.str(false) + " " +
                        std::to_string(gi.GetNanoSeconds()) + "ns " + outputFileName;
                    validationFile << validationFileContent << std::endl;
                }
                if (multiProcessing)
                {
                    continue;
                }

                const auto sgi = (gi == NanoSeconds(400));
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
                    phy.Set("ChannelSettings", StringValue(channelStr));

                    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
                    staDevice = wifi.Install(phy, mac, wifiStaNode);

                    mac.SetType("ns3::ApWifiMac",
                                "EnableBeaconJitter",
                                BooleanValue(false),
                                "BeaconGeneration",
                                BooleanValue(!staticSetup),
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
                    phy.Set("ChannelSettings", StringValue(channelStr));

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

                if (staticSetup)
                {
                    /* static setup of association and BA agreements */
                    auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
                    NS_ASSERT(apDev);
                    WifiStaticSetupHelper::SetStaticAssociation(apDev, staDevice);
                    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDevice, {0});
                    startTime = MilliSeconds(1);
                }

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

                if (staticSetup)
                {
                    /* static setup of ARP cache */
                    NeighborCacheHelper nbCache;
                    nbCache.PopulateNeighborCache();
                }

                /* Setting applications */
                const auto maxLoad = VhtPhy::GetDataRate(mcs, width, gi, 1);
                ApplicationContainer serverApp;
                if (udp)
                {
                    // UDP flow
                    uint16_t port = 9;
                    UdpServerHelper server(port);
                    serverApp = server.Install(wifiStaNode.Get(0));
                    streamNumber += server.AssignStreams(wifiStaNode.Get(0), streamNumber);

                    serverApp.Start(Seconds(0));
                    serverApp.Stop(simulationTime + startTime);
                    const auto packetInterval = payloadSize * 8.0 / maxLoad;

                    UdpClientHelper client(staNodeInterface.GetAddress(0), port);
                    client.SetAttribute("MaxPackets", UintegerValue(4294967295U));
                    client.SetAttribute("Interval", TimeValue(Seconds(packetInterval)));
                    client.SetAttribute("PacketSize", UintegerValue(payloadSize));
                    ApplicationContainer clientApp = client.Install(wifiApNode.Get(0));
                    streamNumber += client.AssignStreams(wifiApNode.Get(0), streamNumber);

                    clientApp.Start(startTime);
                    clientApp.Stop(simulationTime + startTime);
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
                    serverApp.Stop(simulationTime + startTime);

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

                    clientApp.Start(startTime);
                    clientApp.Stop(simulationTime + startTime);
                }

                Ipv4GlobalRoutingHelper::PopulateRoutingTables();

                Simulator::Stop(simulationTime + startTime);
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

                std::ofstream file;
                std::string name{};
                if (multiProcessing || !fileName.empty())
                {
                    file.open(fileName, std::ofstream::out);
                    name = fileName;
                }
                else
                {
                    file.open(outputFileName, std::ofstream::out);
                    name = outputFileName;
                }
                if (file.fail())
                {
                    NS_FATAL_ERROR("File " << name << " not found");
                }
                file << throughput << std::endl;
                file.close();

                if (!multiProcessing && fileName.empty())
                {
                    const auto validationCommand =
                        "./ns3 run --no-build \"wifi-network-validator --resultsFile=" +
                        validationFileName + " --minExpectedThroughput=" +
                        std::to_string(minExpectedThroughput.GetBitRate()) + "bps" +
                        " --maxExpectedThroughput=" +
                        std::to_string(maxExpectedThroughput.GetBitRate()) + "bps" +
                        " --printLastOnly=1 --printResultBanner=" + std::to_string(isFirst) + "\"";
                    RunCommandAndValidate(validationCommand, tmpDir);
                }
                if (isFirst)
                {
                    isFirst = false;
                }
            }
        }
    }

    if (multiProcessing)
    {
        validationFile.close();
        inputFile.close();

        // ns3 executable might not be in the current path (e.g. if ran from test.py)
        std::string execPathName = "ns3";
        std::string checkExecPresentCmd = "./" + execPathName + " --dry-run > /dev/null 2>&1";
        while (std::system(checkExecPresentCmd.c_str()))
        {
            execPathName = "../" + execPathName;
            checkExecPresentCmd = "./" + execPathName + " --dry-run > /dev/null 2>&1";
        }

        const auto numParallelJobsOption =
            (numParallelJobs > 1) ? " --numParallelJobs=" + std::to_string(numParallelJobs) : "";
        const auto parallelCommand =
            "./" + execPathName +
            " run --no-build \"parallel-scheduler --inputFileName=" + inputFileName +
            numParallelJobsOption + "\"";
        RunCommandAndValidate(parallelCommand, tmpDir);
        std::filesystem::remove(inputFileName);

        const auto validationCommand =
            "./" + execPathName +
            " run --no-build \"wifi-network-validator --resultsFile=" + validationFileName +
            " --minExpectedThroughput=" + std::to_string(minExpectedThroughput.GetBitRate()) +
            "bps" +
            " --maxExpectedThroughput=" + std::to_string(maxExpectedThroughput.GetBitRate()) +
            "bps\"";
        RunCommandAndValidate(validationCommand, tmpDir);
    }

    std::filesystem::remove_all(tmpDir);
    return 0;
}
