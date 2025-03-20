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
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <unistd.h>
#include <vector>

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
    bool multiProcessing{false};
    unsigned int numParallelJobs{1}; // if unset, keep default of parallel scheduler
    std::string fileName{""};

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
    cmd.AddValue("multiProcessing", "Enable/disable multiprocessing", multiProcessing);
    cmd.AddValue("numParallelJobs",
                 "the number of jobs to execute in parallel (keep default if unset)",
                 numParallelJobs);
    cmd.AddValue("fileName", "if specified, results will be written to that file", fileName);
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

    std::string tmpDir{""};
    if (fileName.empty() || multiProcessing)
    {
        char dirTemplate[] = "/tmp/ns3-wifi-eht-network-XXXXXX";
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

    // When multiple stations are used, there are chances that association requests
    // collide and hence the throughput may be lower than expected. Therefore, we relax
    // the check that the throughput cannot decrease by introducing a scaling factor (or
    // tolerance)
    double tolerance = 0.10;

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

    bool isFirst{true};
    for (const auto mcs : mcsValues)
    {
        for (int width = minChannelWidth; width <= maxChannelWidth; width *= 2) // MHz
        {
            const auto is80Plus80 = (use80Plus80 && (width == 160));
            const std::string widthStr = is80Plus80 ? "80+80" : std::to_string(width);
            const auto segmentWidthStr = is80Plus80 ? "80" : widthStr;
            for (int gi = maxGi; gi >= minGi; gi /= 2) // Nanoseconds
            {
                std::string outputFileName{""};
                if (fileName.empty() || multiProcessing)
                {
                    auto command = cmd.GetName() +
                                   " --multiProcessing=0 --mcs=" + std::to_string(mcs) +
                                   " --channelWidth=" + std::to_string(width) +
                                   " --guardInterval=" + std::to_string(gi) + " ";
                    std::vector<std::string> programArguments(argv + 1, argv + argc);
                    for (const auto& argument : programArguments)
                    {
                        if (argument.starts_with("--multiProcessing") ||
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
                                     "-width=" + std::to_string(width) +
                                     "-gi=" + std::to_string(gi) + ".dat";
                    inputFile << "--fileName=" << outputFileName << std::endl;
                    const auto validationFileContent = std::to_string(mcs) + " " +
                                                       std::to_string(width) + " " +
                                                       std::to_string(gi) + " " + outputFileName;
                    validationFile << validationFileContent << std::endl;
                }
                if (multiProcessing)
                {
                    continue;
                }

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
                        validationFileName + " --tolerance=" + std::to_string(tolerance) +
                        " --minExpectedThroughput=" + std::to_string(minExpectedThroughput) +
                        " --maxExpectedThroughput=" + std::to_string(maxExpectedThroughput) +
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
            " --tolerance=" + std::to_string(tolerance) +
            " --minExpectedThroughput=" + std::to_string(minExpectedThroughput) +
            " --maxExpectedThroughput=" + std::to_string(maxExpectedThroughput) + "\"";
        RunCommandAndValidate(validationCommand, tmpDir);
    }

    std::filesystem::remove_all(tmpDir);
    return 0;
}
