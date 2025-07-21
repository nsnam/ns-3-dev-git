/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/applications-module.h"
#include "ns3/bridge-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/wifi-module.h"

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

// This example considers two APs and a server connected to an Ethernet switch. A STA is initially
// associated with AP 1 and can be configured to move closer to and associate with AP 2 at a given
// point in time. It is possible to configure whether or not the STA notifies the disassociation to
// AP 1 when roaming. DL and/or UL UDP (or TCP) traffic can be exchanged between STA and the server.
//
// Network (logical) topology
//
//        ┌─────────────────────┐
//        │   Ethernet Switch   │
//        └┬─────────┬─────────┬┘
//         │         │         │ <-- CSMA links
//        AP 1      AP 2    Server
//
//
//  Implementation
//
//                   Ethernet switch
//          ┌───────────────────────────────────┐
//          │               Bridge              │
//          ├───────────┬───────────┬───────────┤
//          │CSMA NetDev│CSMA NetDev│CSMA NetDev│
//          └──┬────────┴──────┬────┴──────┬────┘
//             │               │           │
//   ┌──────┬──┴───┐ ┌──────┬──┴───┐    ┌──┴───┐
//   │ WiFi │ CSMA │ │ WiFi │ CSMA │    │ CSMA │
//   │NetDev│NetDev│ │NetDev│NetDev│    │NetDev│
//   ├──────┴──────┤ ├──────┴──────┤    └──────┘
//   │   Bridge    │ │   Bridge    │     Server
//   └─────────────┘ └─────────────┘
//        AP 1            AP 2
//
// Examples:
//
// ./ns3 run "wifi-roaming --simulationTime=5s --dlLoad=10Mbps --disassocTime=2.5s --udp=0
// --ap1Channels=42,80,BAND_5GHZ,0:106,80,BAND_5GHZ,0
// --ap2Channels=15,160,BAND_6GHZ,0:111,160,BAND_6GHZ,0"
//
// ./ns3 run "wifi-roaming --simulationTime=5s --dlLoad=10Mbps --disassocTime=2.5s --udp=0
// --ap1Channels=15,160,BAND_6GHZ,0:111,160,BAND_6GHZ,0
// --ap2Channels=42,80,BAND_5GHZ,0:106,80,BAND_5GHZ,0"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiRoamingExample");

namespace
{

/**
 * Wi-Fi setup parameters
 */
struct WifiSetupParams
{
    NodeContainer apNodes;                       ///< AP node container
    NodeContainer staNodes;                      ///< STA node container
    std::string ap1Channels{"36,0,BAND_5GHZ,0"}; ///< Colon separated (no spaces) list of
                                                 ///< channel settings for the links of AP 1
    std::string ap2Channels;                     ///< Colon separated (no spaces) list of
                                                 ///< channel settings for the links of AP 2
    std::string staChannels;                     ///< Colon separated (no spaces) list of
                                                 ///< channel settings for the links of STA
};

/**
 * Packet drop statistics.
 */
struct DropStats
{
    std::size_t noFwdCount{0}; ///< count of packets dropped because cannot be forwarded
    std::map<WifiMacDropReason, std::size_t> droppedMpdus; ///< per-reason count of dropped MPDUs
};

/**
 * Get a SpectrumWifiPhyHelper object configured with the channel settings listed in the given
 * input string and with the given Spectrum channel
 *
 * @param channelsStr the input string containing colon separated channel settings
 * @param channel the given Spectrum channel
 * @return a SpectrumWifiPhyHelper object configured with the given channel settings
 */
SpectrumWifiPhyHelper
GetPhyHelper(const std::string& channelsStr, Ptr<SpectrumChannel> channel)
{
    auto strings = SplitString(channelsStr, ":");
    SpectrumWifiPhyHelper phy{static_cast<uint8_t>(strings.size())};
    for (std::size_t id = 0; id < strings.size(); ++id)
    {
        NS_LOG_INFO("PHY ID " << id << ": " << strings[id]);
        phy.Set(id, "ChannelSettings", StringValue("{" + strings[id] + "}"));
    }
    phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
    phy.SetChannel(channel);

    return phy;
}

/**
 * Create Wi-Fi devices.
 *
 * @param wifiParams the parameters to setup Wi-Fi devices
 * @return a pair of containers for the AP and non-AP STA devices
 */
std::pair<NetDeviceContainer, NetDeviceContainer>
CreateWifiDevices(const WifiSetupParams& wifiParams)
{
    NS_ABORT_MSG_IF(wifiParams.apNodes.GetN() != 2,
                    "Expected 2 AP nodes instead of " << wifiParams.apNodes.GetN());
    NS_ABORT_MSG_IF(wifiParams.staNodes.GetN() != 1,
                    "Expected 1 STA node instead of " << wifiParams.staNodes.GetN());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211be);
    wifi.SetRemoteStationManager("ns3::MinstrelHtWifiManager");

    WifiMacHelper mac;
    Ssid ssid("wifi-roaming");

    auto spectrumChannel = CreateObject<MultiModelSpectrumChannel>();
    auto lossModel = CreateObject<LogDistancePropagationLossModel>();
    spectrumChannel->AddPropagationLossModel(lossModel);

    NS_LOG_INFO("Configuring channel settings for AP 1");
    auto ap1Phy = GetPhyHelper(wifiParams.ap1Channels, spectrumChannel);
    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    auto apDevices = wifi.Install(ap1Phy, mac, wifiParams.apNodes.Get(0));

    NS_LOG_INFO("Configuring channel settings for AP 2");
    auto ap2Phy = GetPhyHelper(wifiParams.ap2Channels.empty() ? wifiParams.ap1Channels
                                                              : wifiParams.ap2Channels,
                               spectrumChannel);
    apDevices.Add(wifi.Install(ap2Phy, mac, wifiParams.apNodes.Get(1)));

    NS_LOG_INFO("Configuring channel settings for STA");
    auto staPhy = GetPhyHelper(wifiParams.staChannels.empty() ? wifiParams.ap1Channels
                                                              : wifiParams.staChannels,
                               spectrumChannel);
    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    auto staDevices = wifi.Install(staPhy, mac, wifiParams.staNodes);

    NS_LOG_INFO("AP 1 device address: " << apDevices.Get(0)->GetAddress());
    NS_LOG_INFO("AP 2 device address: " << apDevices.Get(1)->GetAddress());
    NS_LOG_INFO("STA device address: " << staDevices.Get(0)->GetAddress());

    int64_t streamNumber = 100;
    streamNumber += WifiHelper::AssignStreams(apDevices, streamNumber);
    streamNumber += WifiHelper::AssignStreams(staDevices, streamNumber);

    // mobility.
    MobilityHelper mobility;
    auto positionAlloc = CreateObject<ListPositionAllocator>();

    positionAlloc->Add(Vector(0.0, 0.0, 0.0));  // AP 1
    positionAlloc->Add(Vector(50.0, 0.0, 0.0)); // AP 2
    positionAlloc->Add(Vector(0.0, 10.0, 0.0)); // non-AP STA
    mobility.SetPositionAllocator(positionAlloc);

    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    mobility.Install(wifiParams.apNodes);
    mobility.Install(wifiParams.staNodes);

    ap1Phy.EnablePcap("wifi-AP", apDevices);
    ap1Phy.EnablePcap("wifi-STA", staDevices);

    return {apDevices, staDevices};
}

/**
 * Get a map of channels to scan including the channels on which the devices in the given container
 * are operating. The map includes a single entry having a key (PHY ID) of zero.
 *
 * @param devices the given container of devices
 * @return a PHY ID-indexed map of channels to scan
 */
WifiScanParams::Map
GetScanParamsMap(const NetDeviceContainer& devices)
{
    WifiScanParams::Map scanChannel;
    auto& phyBandChannelList = scanChannel[0];

    for (uint32_t i = 0; i < devices.GetN(); ++i)
    {
        auto dev = DynamicCast<WifiNetDevice>(devices.Get(i));
        if (!dev)
        {
            continue;
        }
        for (uint8_t phyId = 0; phyId < dev->GetNPhys(); ++phyId)
        {
            auto phy = dev->GetPhy(phyId);
            auto& channelList = phyBandChannelList[phy->GetPhyBand()];
            auto number = phy->GetPrimaryChannelNumber(MHz_t{20});
            if (std::find(channelList.cbegin(), channelList.cend(), number) == channelList.cend())
            {
                phyBandChannelList[phy->GetPhyBand()].emplace_back(number);
            }
        }
    }
    return scanChannel;
}

/**
 * Callback connected to the Assoc trace source.
 *
 * @param mac the MAC of the associating STA
 * @param address the AP address
 */
void
AssocCallback(Ptr<WifiMac> mac, Mac48Address address)
{
    std::cout << "At time " << Simulator::Now().As(Time::S)
              << ", STA completed association with BSSID " << address << std::endl;
    for (const auto linkId : mac->GetLinkIds())
    {
        if (auto phy = mac->GetWifiPhy(linkId))
        {
            NS_LOG_DEBUG("STA on link " << +linkId << " is operating on "
                                        << phy->GetOperatingChannel());
        }
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    const Time startTime{"1s"};
    Time simulationTime{"10s"};
    DataRate ulLoad;
    DataRate dlLoad;
    bool udp{true};
    Time disassocTime{"5s"};
    bool notifyAp{false};
    WifiSetupParams wifiParams;
    bool sendUlNullPkt{true};
    uint32_t pktSize{2000};
    std::map<Ptr<WifiMac>, DropStats> stats;
    uint64_t dlTxBytes{0};
    uint64_t ulTxBytes{0};
    uint64_t dlRxBytes{0};
    uint64_t ulRxBytes{0};

    CommandLine cmd(__FILE__);
    cmd.AddValue("simulationTime", "Simulation time", simulationTime);
    cmd.AddValue("dlLoad", "Rate of the downlink load to generate", dlLoad);
    cmd.AddValue("ulLoad", "Rate of the uplink load to generate", ulLoad);
    cmd.AddValue("udp", "True to use UDP, false to use TCP", udp);
    cmd.AddValue("disassocTime",
                 "If non-null, the time at which the STA sends a Disassociation frame and moves "
                 "closer to AP 2",
                 disassocTime);
    cmd.AddValue("notifyAp",
                 "Whether to send a Disassociation frame to notify disassociation to AP 1",
                 notifyAp);
    cmd.AddValue("ap1Channels",
                 "Colon separated (no spaces) list of channel settings for the links of AP 1",
                 wifiParams.ap1Channels);
    cmd.AddValue("ap2Channels",
                 "Colon separated (no spaces) list of channel settings for the links of AP 2; "
                 "leave empty to use the same channel set as AP 1",
                 wifiParams.ap2Channels);
    cmd.AddValue("staChannels",
                 "Colon separated (no spaces) list of channel settings for the links of the STA; "
                 "leave empty to use the same channel set as AP 1",
                 wifiParams.staChannels);
    cmd.AddValue("sendUlNullPkt",
                 "Whether APs send a null packet to the Ethernet switch to make the latter learn "
                 "the address of a newly associated STA",
                 sendUlNullPkt);
    cmd.AddValue("pktSize", "The application packet size in bytes", pktSize);
    cmd.Parse(argc, argv);

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NodeContainer ethSwitch(1);
    NodeContainer serverNodes(1);
    wifiParams.apNodes.Create(2);
    wifiParams.staNodes.Create(1);

    BridgeHelper bridge;
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate("10Gbps")));
    csma.SetChannelAttribute("Delay", TimeValue(Time{0}));

    NetDeviceContainer ethSwitchPorts;
    NetDeviceContainer serverCsmaDevices;
    NetDeviceContainer apCsmaDevices;

    // Create a CSMA link between the server and the switch
    for (std::size_t i = 0; i < serverNodes.GetN(); ++i)
    {
        auto link = csma.Install(NodeContainer(serverNodes.Get(i), ethSwitch));
        serverCsmaDevices.Add(link.Get(0));
        ethSwitchPorts.Add(link.Get(1));
        NS_LOG_INFO("Server-Switch CSMA link addresses: " << link.Get(0)->GetAddress() << " - "
                                                          << link.Get(1)->GetAddress());
    }

    // Create the wifi devices
    Config::SetDefault("ns3::ApWifiMac::FwdProtNumber", UintegerValue(sendUlNullPkt ? 2000 : 0));
    Config::SetDefault("ns3::WifiDefaultAssocManager::SkipScanningIfApInfoAvail",
                       BooleanValue(false));

    auto [apWifiDevices, staWifiDevices] = CreateWifiDevices(wifiParams);

    for (std::size_t i = 0; i < wifiParams.apNodes.GetN(); ++i)
    {
        // create a CSMA link between AP i and the ethernet switch (which adds a CSMA netdevice to
        // each node)
        auto csmaDevices = csma.Install(NodeContainer(wifiParams.apNodes.Get(i), ethSwitch));
        // bridge the AP CSMA netdevice with the AP Wi-Fi netdevice
        bridge.Install(wifiParams.apNodes.Get(i),
                       NetDeviceContainer(csmaDevices.Get(0), apWifiDevices.Get(i)));
        // add the other CSMA netdevice as another switch port
        ethSwitchPorts.Add(csmaDevices.Get(1));
        NS_LOG_INFO("AP " << i
                          << "-Switch CSMA link addresses: " << csmaDevices.Get(0)->GetAddress()
                          << " - " << csmaDevices.Get(1)->GetAddress());
    }

    bridge.Install(ethSwitch.Get(0), ethSwitchPorts);

    // Add internet stack to client and server
    InternetStackHelper internet;
    internet.Install(serverNodes);
    internet.Install(wifiParams.staNodes);

    // We've got the "hardware" in place.  Now we need to add IP addresses.
    //
    NS_LOG_INFO("Assign IP Addresses.");
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    auto serverInterfaces = ipv4.Assign(serverCsmaDevices);
    auto staWifiInterfaces = ipv4.Assign(staWifiDevices);

    NS_LOG_INFO("Create Applications.");
    uint16_t port = 9; // Discard port (RFC 863)
    auto socketFactory = udp ? "ns3::UdpSocketFactory" : "ns3::TcpSocketFactory";

    // Install client and server apps for UL flow (if data rate > 0)
    if (ulLoad.GetBitRate() > 0)
    {
        OnOffHelper onoff(socketFactory,
                          Address(InetSocketAddress(serverInterfaces.GetAddress(0), port)));
        onoff.SetConstantRate(ulLoad, pktSize);

        auto clientApp = onoff.Install(wifiParams.staNodes.Get(0));
        // Start the application
        clientApp.Start(startTime);
        clientApp.Stop(startTime + simulationTime);

        // Create a packet sink to receive these packets
        PacketSinkHelper sink(socketFactory,
                              Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
        auto serverApp = sink.Install(serverNodes.Get(0));
        serverApp.Start(Time{0});
    }

    // Install client and server apps for DL flow (if data rate > 0)
    if (dlLoad.GetBitRate() > 0)
    {
        OnOffHelper onoff(socketFactory,
                          Address(InetSocketAddress(staWifiInterfaces.GetAddress(0), port)));
        onoff.SetConstantRate(dlLoad, pktSize);

        auto clientApp = onoff.Install(serverNodes.Get(0));
        // Start the application
        clientApp.Start(startTime);
        clientApp.Stop(startTime + simulationTime);

        // Create a packet sink to receive these packets
        PacketSinkHelper sink(socketFactory,
                              Address(InetSocketAddress(Ipv4Address::GetAny(), port)));
        auto serverApp = sink.Install(wifiParams.staNodes.Get(0));
        serverApp.Start(Time{0});
    }

    auto staDev = DynamicCast<WifiNetDevice>(staWifiDevices.Get(0));
    NS_ASSERT(staDev);
    auto staMac = DynamicCast<StaWifiMac>(staDev->GetMac());
    NS_ASSERT(staMac);

    const auto scanChannelsStr = StaWifiMac::ScanningChannelsToStr(GetScanParamsMap(apWifiDevices));
    staMac->SetAttribute("ScanningChannels", StringValue(scanChannelsStr));

    // print messages when (dis)association is completed
    staMac->TraceConnectWithoutContext("Assoc", MakeCallback(&AssocCallback).Bind(staMac));

    staMac->TraceConnectWithoutContext("DeAssoc",
                                       Callback<void, Mac48Address>([](Mac48Address apAddr) {
                                           std::cout << "At time " << Simulator::Now().As(Time::S)
                                                     << ", STA disassociated from BSSID " << apAddr
                                                     << std::endl;
                                       }));

    auto ap1Dev = DynamicCast<WifiNetDevice>(apWifiDevices.Get(0));
    NS_ASSERT(ap1Dev);
    auto ap1Mac = DynamicCast<ApWifiMac>(ap1Dev->GetMac());
    NS_ASSERT(ap1Mac);

    auto ap2Dev = DynamicCast<WifiNetDevice>(apWifiDevices.Get(1));
    NS_ASSERT(ap2Dev);
    auto ap2Mac = DynamicCast<ApWifiMac>(ap2Dev->GetMac());
    NS_ASSERT(ap2Mac);

    // print messages when BA agreements (TID 0) are established
    staMac->GetQosTxop(AC_BE)->TraceConnectWithoutContext(
        "BaEstablished",
        Callback<void, Mac48Address, uint8_t, std::optional<Mac48Address>>(
            [](Mac48Address recipient, uint8_t tid, std::optional<Mac48Address> /* gcrGroup */) {
                std::cout << "At time " << Simulator::Now().As(Time::S)
                          << ", STA established BA agreement with " << recipient << " for TID "
                          << +tid << std::endl;
            }));

    ap1Mac->GetQosTxop(AC_BE)->TraceConnectWithoutContext(
        "BaEstablished",
        Callback<void, Mac48Address, uint8_t, std::optional<Mac48Address>>(
            [](Mac48Address recipient, uint8_t tid, std::optional<Mac48Address> /* gcrGroup */) {
                std::cout << "At time " << Simulator::Now().As(Time::S)
                          << ", AP 1 established BA agreement with " << recipient << " for TID "
                          << +tid << std::endl;
            }));

    ap2Mac->GetQosTxop(AC_BE)->TraceConnectWithoutContext(
        "BaEstablished",
        Callback<void, Mac48Address, uint8_t, std::optional<Mac48Address>>(
            [](Mac48Address recipient, uint8_t tid, std::optional<Mac48Address> /* gcrGroup */) {
                std::cout << "At time " << Simulator::Now().As(Time::S)
                          << ", AP 2 established BA agreement with " << recipient << " for TID "
                          << +tid << std::endl;
            }));

    // connect callbacks to Tx trace source to track transmitted UL and DL bytes
    for (uint32_t i = 0; i < wifiParams.staNodes.Get(0)->GetNApplications(); ++i)
    {
        if (auto onoff =
                DynamicCast<OnOffApplication>(wifiParams.staNodes.Get(0)->GetApplication(i)))
        {
            onoff->TraceConnectWithoutContext(
                "Tx",
                Callback<void, uint64_t&, Ptr<const Packet>>([](uint64_t& txBytes,
                                                                Ptr<const Packet> packet) {
                    txBytes += packet->GetSize();
                }).Bind(std::ref(ulTxBytes)));
        }
    }

    for (uint32_t i = 0; i < serverNodes.Get(0)->GetNApplications(); ++i)
    {
        if (auto onoff = DynamicCast<OnOffApplication>(serverNodes.Get(0)->GetApplication(i)))
        {
            onoff->TraceConnectWithoutContext(
                "Tx",
                Callback<void, uint64_t&, Ptr<const Packet>>([](uint64_t& txBytes,
                                                                Ptr<const Packet> packet) {
                    txBytes += packet->GetSize();
                }).Bind(std::ref(dlTxBytes)));
        }
    }

    stats.try_emplace(staMac);
    stats.try_emplace(ap1Mac);
    stats.try_emplace(ap2Mac);

    for (auto& [mac, drops] : stats)
    {
        mac->TraceConnectWithoutContext(
            "MacTxDrop",
            Callback<void, Ptr<const Packet>>([&](Ptr<const Packet> p) { ++drops.noFwdCount; }));
        // clang-format off
        mac->TraceConnectWithoutContext("DroppedMpdu",
                                        Callback<void, WifiMacDropReason, Ptr<const WifiMpdu>>(
                                            [&](WifiMacDropReason reason, Ptr<const WifiMpdu>) {
                                                auto [it, inserted] =
                                                    drops.droppedMpdus.insert({reason, 1});
                                                if (!inserted)
                                                {
                                                    ++it->second;
                                                }
                                            }));
        // clang-format on
    }

    // lambda to print DL and UL average throughput during the given time interval
    auto printTput = [&, serverNodes](const Time& start, const Time& end) {
        // compute UL throughput
        for (uint32_t i = 0; i < serverNodes.Get(0)->GetNApplications(); ++i)
        {
            if (auto sink = DynamicCast<PacketSink>(serverNodes.Get(0)->GetApplication(i)))
            {
                std::cout << "---------------- UL ----------------"
                          << "From " << start.As(Time::S) << " to " << end.As(Time::S) << ":\n"
                          << "Wi-Fi STA transmitted " << ulTxBytes << " bytes\n";
                auto rxBytes = sink->GetTotalRx() - ulRxBytes;
                if (rxBytes == 0)
                {
                    NS_LOG_ERROR("UL throughput is zero");
                    exit(1);
                }
                std::cout << "SERVER received " << rxBytes << " bytes, UL throughput = "
                          << (rxBytes * 8.0) / (end - start).GetMilliSeconds() << " kbps"
                          << std::endl
                          << "------------------------------------" << std::endl;
                ulRxBytes = sink->GetTotalRx();
            }
        }

        // compute DL throughput
        for (uint32_t i = 0; i < wifiParams.staNodes.Get(0)->GetNApplications(); ++i)
        {
            if (auto sink = DynamicCast<PacketSink>(wifiParams.staNodes.Get(0)->GetApplication(i)))
            {
                std::cout << "---------------- DL ----------------\n"
                          << "From " << start.As(Time::S) << " to " << end.As(Time::S) << ":\n"
                          << "SERVER transmitted " << dlTxBytes << " bytes\n";
                auto rxBytes = sink->GetTotalRx() - dlRxBytes;
                if (rxBytes == 0)
                {
                    NS_LOG_ERROR("DL throughput is zero");
                    exit(1);
                }
                std::cout << "Wi-Fi STA received " << rxBytes << " bytes, DL throughput = "
                          << (rxBytes * 8.0) / (end - start).GetMilliSeconds() << " kbps"
                          << std::endl
                          << "------------------------------------" << std::endl;
                dlRxBytes = sink->GetTotalRx();
            }
        }
    };

    // schedule disassociation, if requested
    if (disassocTime.IsStrictlyPositive())
    {
        Simulator::Schedule(disassocTime, [=]() {
            if (!staMac->IsAssociated())
            {
                std::cout << "STA not associated at time disassociation is requested ("
                          << Simulator::Now().As(Time::S) << "), do nothing" << std::endl;
                return;
            }
            staMac->ForceDisassociation(notifyAp);
            auto mobility = wifiParams.staNodes.Get(0)->GetObject<MobilityModel>();
            NS_ASSERT(mobility);
            mobility->SetPosition(Vector(50.0, 10.0, 0.0));
            printTput(startTime, disassocTime);
            std::cout << "Packets left in the STA BE queue: "
                      << staMac->GetTxopQueue(AC_BE)->GetNPackets() << std::endl;
            std::cout << "Packets left in the AP 1 BE queue: "
                      << ap1Mac->GetTxopQueue(AC_BE)->GetNPackets() << std::endl;
        });
    }

    csma.EnablePcapAll("csma-bridge", false);

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(startTime + simulationTime);
    Simulator::Run();

    printTput(disassocTime, startTime + simulationTime);

    // print statistics
    for (auto& [mac, drops] : stats)
    {
        const auto str = (mac->GetTypeOfStation() == STA
                              ? "STA"
                              : "AP " + std::to_string(mac->GetDevice()->GetNode()->GetId() - 1));

        std::cout << "\nStatistics for " << str << "\n--------------\n"
                  << "Dropped because cannot be forwarded: " << drops.noFwdCount << std::endl;
        for (const auto& [reason, count] : drops.droppedMpdus)
        {
            std::cout << reason << ": " << count << std::endl;
        }
    }

    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
