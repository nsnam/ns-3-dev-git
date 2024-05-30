/*
 * Copyright (c) 2024 University of Washington (updated to 802.11ax standard)
 * Copyright (c) 2009 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

// The purpose of this example is to illustrate basic use of the
// WifiCoTraceHelper on a simple example program.
//
// This script configures four 802.11ax Wi-Fi STAs on a YansWifiChannel,
// with devices in infrastructure mode, and each STA sends a saturating load
// of UDP datagrams to the AP for a specified simulation duration. A simple
// free-space path loss (Friis) propagation loss model is configured.
// The lowest MCS ("HeMcs0") value is configured.
//
// At the end of the simulation, a channel occupancy report is printed for
// each STA and for the AP.  There are two program options:
// -- duration:
// -- useDifferentAc:  (true or false)
//
// If 'useDifferentAc' has the value false, all STAs will have the same EDCA parameters
// for best effort) and their channel utilization (the TX time output of the
// channel access helper) will be close to equal.  If 'useDifferentAc' is true,
// then two of the four STAs will instead be configured to use the voice
// access category, and channel utilization will be different due to the
// different EDCA parameters.

#include "ns3/boolean.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/names.h"
#include "ns3/neighbor-cache-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/wifi-co-trace-helper.h"
#include "ns3/wifi-phy-rx-trace-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiCoTraceExample");

// Function for runtime manual ARP configuration
void
PopulateNeighborCache()
{
    NeighborCacheHelper neighborCache;
    neighborCache.PopulateNeighborCache();
}

int
main(int argc, char* argv[])
{
    bool useDifferentAc = false;
    Time duration{Seconds(10)};
    double distance = 1; // meters

    CommandLine cmd(__FILE__);
    cmd.AddValue("useDifferentAc",
                 "Uses VO AC on 2 STAs and BE on rest if true. Uses BE AC on all 4 STAs if false.",
                 useDifferentAc);
    cmd.AddValue("duration", "Duration of data transfer", duration);
    cmd.Parse(argc, argv);

    NodeContainer apNode(1);
    Names::Add("AP", apNode.Get(0));
    NodeContainer staNodes(4);
    Names::Add("STA0", staNodes.Get(0));
    Names::Add("STA1", staNodes.Get(1));
    Names::Add("STA2", staNodes.Get(2));
    Names::Add("STA3", staNodes.Get(3));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNode);
    positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    positionAlloc->Add(Vector(0.0, distance, 0.0));
    positionAlloc->Add(Vector(0.0, -distance, 0.0));
    positionAlloc->Add(Vector(-distance, 0.0, 0.0));
    mobility.Install(staNodes);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("HeMcs0"),
                                 "ControlMode",
                                 StringValue("HeMcs0"));

    // Setup the rest of the MAC
    Ssid ssid = Ssid("wifi-default");
    // setup AP to beacon roughly once per second (must be a multiple of 1024 us)
    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "QosSupported",
                    BooleanValue(true),
                    "BeaconInterval",
                    TimeValue(MilliSeconds(1024)));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, apNode);

    // setup STA and disable the possible loss of association due to missed beacons
    wifiMac.SetType("ns3::StaWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "QosSupported",
                    BooleanValue(true),
                    "MaxMissedBeacons",
                    UintegerValue(std::numeric_limits<uint32_t>::max()));
    NetDeviceContainer staDevices = wifi.Install(wifiPhy, wifiMac, staNodes);

    NetDeviceContainer allDevices;
    allDevices.Add(apDevice);
    allDevices.Add(staDevices);

    InternetStackHelper internet;
    internet.Install(apNode);
    internet.Install(staNodes);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(allDevices);

    uint16_t portNumber = 9;
    std::vector<uint8_t> tosValues = {0x70, 0x28, 0xb8, 0xc0}; // AC_BE, AC_BK, AC_VI, AC_VO
    auto ipv4ap = apNode.Get(0)->GetObject<Ipv4>();
    const auto address = ipv4ap->GetAddress(1, 0).GetLocal();

    ApplicationContainer sourceApplications;
    ApplicationContainer sinkApplications;
    for (uint32_t i = 0; i < 4; i++)
    {
        InetSocketAddress sinkAddress(address, portNumber + i);
        PacketSinkHelper packetSinkHelper("ns3::UdpSocketFactory", sinkAddress);
        sinkApplications.Add(packetSinkHelper.Install(apNode.Get(0)));
        OnOffHelper onOffHelper("ns3::UdpSocketFactory", sinkAddress);
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onOffHelper.SetAttribute("DataRate", DataRateValue(2000000)); // bits/sec
        onOffHelper.SetAttribute("PacketSize", UintegerValue(1472));  // bytes
        if (!useDifferentAc)
        {
            onOffHelper.SetAttribute("Tos", UintegerValue(tosValues[0])); // AC_BE
        }
        else
        {
            onOffHelper.SetAttribute("Tos",
                                     UintegerValue(tosValues[3 * (i % 2)])); // AC_BE and AC_VO
        }
        sourceApplications.Add(onOffHelper.Install(staNodes.Get(i)));
    }

    sinkApplications.Start(Seconds(0.0));
    sinkApplications.Stop(Seconds(1.0) + duration + MilliSeconds(20));
    sourceApplications.Start(Seconds(1.0));
    sourceApplications.Stop(Seconds(1.0) + duration);

    // Use the NeighborCacheHelper to avoid ARP messages (ARP replies, since they are unicast,
    // count in the statistics.  The cache operation must be scheduled after WifiNetDevices are
    // started, until issue #851 is fixed.  The indirection through a normal function is
    // necessary because NeighborCacheHelper::PopulateNeighborCache() is overloaded
    Simulator::Schedule(Seconds(0.99), &PopulateNeighborCache);

    WifiCoTraceHelper wifiCoTraceHelper(Seconds(1), Seconds(1) + duration);
    wifiCoTraceHelper.Enable(allDevices);

    Simulator::Stop(duration + Seconds(2));
    Simulator::Run();

    // The following provide some examples of how to access and print the trace helper contents.
    std::cout << "*** Print statistics for all nodes using built-in print method:" << std::endl;
    wifiCoTraceHelper.PrintStatistics(std::cout);

    std::cout << "*** Print the statistics in your own way.  Here, just sum the STAs total TX time:"
              << std::endl
              << std::endl;

    auto records = wifiCoTraceHelper.GetDeviceRecords();
    Time sumStaTxTime;
    for (const auto& it : records)
    {
        if (it.m_nodeId > 0)
        {
            const auto it2 = it.m_linkStateDurations.at(0).find(WifiPhyState::TX);
            if (it2 != it.m_linkStateDurations.at(0).end())
            {
                sumStaTxTime += it2->second;
            }
        }
    }

    std::cout << "Sum of STA time in TX state is " << sumStaTxTime.As(Time::S) << std::endl;

    Simulator::Destroy();

    return 0;
}
