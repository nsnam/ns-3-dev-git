/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/eht-phy.h"
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
#include "ns3/uinteger.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-static-setup-helper.h"

#include <cstdlib>
#include <map>
#include <tuple>
#include <vector>

// In this example, an AP transmits DL traffic to a STA using each of the two given rate adaptation
// algorithms. The DL traffic load is computed so as to saturate the channel capacity. The fixed
// RSSI propagation loss model is installed and multiple RSSI values can be set. This example
// computes the throughput achieved by each algorithm for every RSSI value, checks that these
// throughput values are within a given bound and prints the most frequently used TXVECTORs.

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRaaCompare");

namespace
{

/// @brief simulation parameters
struct Params
{
    std::string standard{"11be"};   ///< supported standard
    uint32_t payloadSize{1472};     ///< payload size in bytes
    Time stepTime{"10s"};           ///< time simulated for each RSSI
    std::vector<dBm_t> fixedRssVec; ///< vector of fixed RSSI values
    bool enablePcap{false};         ///< whether to enable PCAP tracing
    MHz_t width{80};                ///< channel width
    uint8_t nSStreams{1};           ///< number of spatial streams
    bool useUdp{true};              ///< true to use UDP, false to use TCP
};

/// tuple storing TX params (modulation class, MCS, NSS, width)
using TxVectorParams = std::tuple<WifiModulationClass, uint8_t /* MCS */, uint8_t /* NSS */, MHz_t>;
/// maps each TXVECTOR to the number of times it has been used
std::map<TxVectorParams, std::size_t> txVectors;
/// total received bytes
uint64_t totalRxBytes;
/// store the average throughput achieved by an RAA for each RSSI value
std::vector<DataRate> tputValues;

/// @brief Store the TXVECTORs used by the AP to transmit QoS data frames into a map
/// @param psduMap the transmitted PSDU map
/// @param txVector the TXVECTOR
void
TrackTxVectors(WifiConstPsduMap psduMap, WifiTxVector txVector, double)
{
    if (!psduMap.cbegin()->second->GetHeader(0).IsQosData() ||
        (txVector.GetModulationClass() < WIFI_MOD_CLASS_HT))
    {
        return;
    }

    auto [it, inserted] = txVectors.emplace(TxVectorParams{txVector.GetModulationClass(),
                                                           txVector.GetMode().GetMcsValue(),
                                                           txVector.GetNss(),
                                                           txVector.GetChannelWidth()},
                                            1);
    if (!inserted)
    {
        ++it->second;
    }
}

/// @brief Calculate throughput and TXVECTORs at the end of each time interval
/// @param raa the RAA being used
/// @param params the simulation parameters
/// @param step the current step (starting at 0)
/// @param sink the sink application
/// @param loss the fixed RSS propagation loss model
void
CalcTputAndTxVectors(const std::string& raa,
                     const Params& params,
                     std::size_t step,
                     Ptr<Application> sink,
                     Ptr<FixedRssLossModel> loss)
{
    std::cout << raa << ", RSSI: " << params.fixedRssVec[step] << " \n";

    // print the top 3 most used TXVECTORs
    std::size_t totalCount = 0;
    std::multimap<std::size_t, TxVectorParams, std::greater<>> mostUsed;

    for (const auto& [tuple, count] : txVectors)
    {
        mostUsed.emplace(count, tuple);
        totalCount += count;
    }

    std::cout << "Top-3 of most used TXVECTORs (total times: " << totalCount << "):\n";

    for (std::size_t n = 0; const auto& [count, tuple] : mostUsed)
    {
        std::cout << "class " << std::get<WifiModulationClass>(tuple) << " MCS "
                  << +std::get<1>(tuple) << " NSS " << +std::get<2>(tuple) << " TX width "
                  << std::get<3>(tuple) << " (" << count << " times)\n";
        if (++n == 3)
        {
            break;
        }
    }
    txVectors.clear();

    // print the throughput in the last time interval
    const auto newTotalRxBytes = DynamicCast<PacketSink>(sink)->GetTotalRx();
    const auto throughput =
        DataRate((newTotalRxBytes - totalRxBytes) * 8.0 / params.stepTime.GetSeconds());
    tputValues.emplace_back(throughput);
    totalRxBytes = newTotalRxBytes;
    std::cout << "Throughput: " << throughput << "\n\n";

    // set the next RSSI value (except for last iteration)
    if (step + 1 < params.fixedRssVec.size())
    {
        loss->SetRss(params.fixedRssVec[step + 1].in_dBm());
    }
}

/// @brief Run a simulation with the given RAA and given parameters
/// @param raa the given rate adaptation algorithm
/// @param params the given simulation parameters
/// @return a vector of throughput values (one for each RSSI value)
std::vector<DataRate>
RunOne(const std::string& raa, const Params& params)
{
    NodeContainer wifiStaNode(1);
    NodeContainer wifiApNode(1);

    WifiHelper wifi;
    wifi.SetRemoteStationManager("ns3::" + raa);

    if (params.standard == "11bn")
    {
        wifi.SetStandard(WIFI_STANDARD_80211bn);
    }
    else if (params.standard == "11be")
    {
        wifi.SetStandard(WIFI_STANDARD_80211be);
    }
    else if (params.standard == "11ax")
    {
        wifi.SetStandard(WIFI_STANDARD_80211ax);
    }
    else if (params.standard == "11ac")
    {
        wifi.SetStandard(WIFI_STANDARD_80211ac);
    }
    else if (params.standard == "11n")
    {
        wifi.SetStandard(WIFI_STANDARD_80211n);
    }
    else
    {
        NS_ABORT_MSG("Unsupported standard ("
                     << params.standard << "), valid values are 11n, 11ac, 11ax, 11be or 11bn");
    }

    NS_ABORT_MSG_IF(params.fixedRssVec.empty(), "At least one RSSI value must be provided");
    auto channel = CreateObject<MultiModelSpectrumChannel>();
    auto loss = CreateObjectWithAttributes<FixedRssLossModel>(
        "Rss",
        DoubleValue(params.fixedRssVec.front().in_dBm()));
    channel->AddPropagationLossModel(loss);

    SpectrumWifiPhyHelper phy;
    phy.SetChannel(channel);
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.Set("ChannelSettings",
            StringValue("{0, " + std::to_string(static_cast<uint16_t>(params.width.in_MHz())) +
                        (params.width == MHz_t{320} ? ", BAND_6GHZ" : ", BAND_5GHZ") + ", 0}"));

    Ssid ssid("wifi-raa-compare");

    WifiMacHelper mac;
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    auto staDevice = wifi.Install(phy, mac, wifiStaNode.Get(0));

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(false));
    auto apDevice = wifi.Install(phy, mac, wifiApNode.Get(0));

    int64_t streamNumber = 150;
    streamNumber += WifiHelper::AssignStreams(apDevice, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevice, streamNumber);

    // Setting mobility model
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(1.0, 0.0, 0.0));

    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(wifiApNode);
    mobility.Install(wifiStaNode);

    /* static setup of association and BA agreements */
    auto apDev = DynamicCast<WifiNetDevice>(apDevice.Get(0));
    NS_ASSERT(apDev);
    WifiStaticSetupHelper::SetStaticAssoc(apDev, staDevice);
    auto staDev = DynamicCast<WifiNetDevice>(staDevice.Get(0));
    NS_ASSERT(staDev);
    WifiStaticSetupHelper::SetStaticBlockAck(apDev, staDev, 0);
    WifiStaticSetupHelper::SetStaticBlockAck(staDev, apDev, 0);

    // Internet stack
    InternetStackHelper stack;
    stack.Install(wifiApNode);
    stack.Install(wifiStaNode);

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer staInterface;
    staInterface = address.Assign(staDevice);
    Ipv4InterfaceContainer ApInterfaceA;
    ApInterfaceA = address.Assign(apDevice);

    /* static setup of ARP cache */
    InternetStaticSetupHelper::PopulateArpCache();

    const auto simulationTime = params.stepTime * params.fixedRssVec.size();

    // Setting applications
    const uint16_t port = 9;
    std::string socketType{params.useUdp ? "ns3::UdpSocketFactory" : "ns3::TcpSocketFactory"};
    PacketSinkHelper server(socketType, InetSocketAddress(Ipv4Address::GetAny(), port));
    ApplicationContainer serverApp = server.Install(wifiStaNode.Get(0));
    serverApp.Start(Seconds(0));
    serverApp.Stop(simulationTime);

    InetSocketAddress dest(staInterface.GetAddress(0), port);
    const auto maxLoad = EhtPhy::GetDataRate(13, params.width, NanoSeconds(1600), params.nSStreams);

    OnOffHelper client(socketType, dest);
    client.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    client.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    client.SetAttribute("DataRate", StringValue(std::to_string(maxLoad) + "bps"));
    client.SetAttribute("PacketSize", UintegerValue(params.payloadSize));

    ApplicationContainer clientApp = client.Install(wifiApNode.Get(0));
    clientApp.Start(NanoSeconds(1));
    clientApp.Stop(simulationTime);

    apDev->GetPhy()->TraceConnectWithoutContext("PhyTxPsduBegin", MakeCallback(&TrackTxVectors));

    txVectors.clear();
    totalRxBytes = 0;
    tputValues.clear();

    for (std::size_t i = 0; i < params.fixedRssVec.size(); ++i)
    {
        Simulator::Schedule(params.stepTime * (i + 1),
                            &CalcTputAndTxVectors,
                            raa,
                            params,
                            i,
                            serverApp.Get(0),
                            loss);
    }

    if (params.enablePcap)
    {
        phy.EnablePcap(raa + "_AP", apDevice.Get(0));
        phy.EnablePcap(raa + "_STA_", staDevice.Get(0));
    }

    Simulator::Stop(simulationTime);
    Simulator::Run();
    Simulator::Destroy();

    return tputValues;
}

} // namespace

int
main(int argc, char* argv[])
{
    Params params;
    std::string raa1{"ThompsonSamplingWifiManager"};
    std::string raa2{"IdealWifiManager"};
    bool enableRts{false};
    std::string rssStr{"-45"};
    uint32_t maxAmpduSize{WIFI_PSDU_MAX_LENGTH_EHT};
    percent_t tolerance{15};

    CommandLine cmd(__FILE__);
    cmd.AddValue("standard", "Supported standard (11n, 11ac, 11ax, 11be, 11bn)", params.standard);
    cmd.AddValue("raa1", "First rate adaptation algorithm to compare", raa1);
    cmd.AddValue("raa2", "Second rate adaptation algorithm to compare", raa2);
    cmd.AddValue("payloadSize", "Payload size in bytes", params.payloadSize);
    cmd.AddValue("enableRts", "Enable or disable RTS/CTS", enableRts);
    cmd.AddValue("stepTime", "Step time", params.stepTime);
    cmd.AddValue("enablePcap", "Enable/disable pcap file generation", params.enablePcap);
    cmd.AddValue("width", "The channel width", params.width);
    cmd.AddValue("nSStreams", "The number of supported spatial streams", params.nSStreams);
    cmd.AddValue("fixedRss", "list of comma separated RSSI values (in dBm) to test", rssStr);
    cmd.AddValue("useUdp", "true to use UDP, false to use TCP", params.useUdp);
    cmd.AddValue("maxAmpduSize", "Maximum A-MPDU size in bytes", maxAmpduSize);
    cmd.Parse(argc, argv);

    // parse RSSI values
    AttributeContainerValue<dBmValue, ',', std::vector> attr;
    auto checker = DynamicCast<AttributeContainerChecker>(MakeAttributeContainerChecker(attr));
    checker->SetItemChecker(MakedBmChecker());
    attr.DeserializeFromString(rssStr, checker);
    params.fixedRssVec = attr.Get();

    Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold",
                       enableRts ? StringValue("0")
                                 : StringValue(std::to_string(WIFI_MAX_RTS_THRESHOLD)));
    Config::SetDefault("ns3::WifiMac::BE_MaxAmpduSize", UintegerValue(maxAmpduSize));
    Config::SetDefault("ns3::WifiPhy::Antennas", UintegerValue(params.nSStreams));
    Config::SetDefault("ns3::WifiPhy::MaxSupportedTxSpatialStreams",
                       UintegerValue(params.nSStreams));
    Config::SetDefault("ns3::WifiPhy::MaxSupportedRxSpatialStreams",
                       UintegerValue(params.nSStreams));

    auto tputValues1 = RunOne(raa1, params);
    auto tputValues2 = RunOne(raa2, params);

    NS_ABORT_MSG_IF(tputValues1.size() != params.fixedRssVec.size(),
                    "Not all steps have been executed for " << raa1);
    NS_ABORT_MSG_IF(tputValues2.size() != params.fixedRssVec.size(),
                    "Not all steps have been executed for " << raa2);

    for (std::size_t i = 0; i < params.fixedRssVec.size(); ++i)
    {
        const auto tput1 = tputValues1[i].GetBitRate();
        const auto tput2 = tputValues2[i].GetBitRate();
        if (std::abs(static_cast<int64_t>(tput2 - tput1)) >
            tolerance.to_ratio() * std::max<uint64_t>(tput2, tput1))
        {
            NS_LOG_ERROR("Obtained throughput values for RSSI "
                         << params.fixedRssVec[i] << " are not within the given tolerance");
            exit(1);
        }
    }

    return 0;
}
