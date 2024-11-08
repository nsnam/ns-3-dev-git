/*
 * Copyright (c) 2009 The Boeing Company
 * Copyright (c) 2024 University of Washington (updated to 802.11ax standard)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

// The purpose of this example is to illustrate basic use of the
// WifiPhyRxTraceHelper on a simple example program.
//
// This script configures two 802.11ax Wi-Fi nodes on a YansWifiChannel,
// with devices in infrastructure mode, and by default, the station sends
// one packet of 1000 (application) bytes to the access point.  A simple
// free-space path loss (Friis) propagation loss model is configured.
// The lowest MCS ("HeMcs0") value is configured.
//
// The first packet is sent at time 1 sec.  If more packets are configured
// to be sent, they are separated in time by the 'interval' parameter, which
// defaults to 1 sec.  The simulation ends 1 sec. after the last packet is sent.
//
// This script can also be used to put the Wifi layer into full logging mode
// at the LOG_LEVEL_INFO level; this command will turn on all logging in the
// wifi module:
//
// ./ns3 run "wifi-phy-rx-trace-example --logging=1"
//
// Logging of the example program itself is enabled by default in this program;
// to turn off the printing of packet send and receive events, disable the verbose mode:
//
// ./ns3 run "wifi-phy-rx-trace-example --verbose=0"
//
// When you are done, you will notice two pcap trace files in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-phy-rx-trace-example-0-0.pcap -nn -tt
//
// The STA is indexed as node 0, the AP is indexed as node 1
//
// Additionally, an ASCII trace file is created (wifi-phy-rx-trace-example.tr)
//
// Finally, the example demonstrates that two independent BSS can exist on the
// same channel, with the trace helper configured to log rxs only on one
// of the BSS.  The outside BSS (OBSS) will send packets that are picked up
// by the trace helper on the primary BSS's devices.  The distance to the OBSS
// can be configured to place the OBSS within or outside of carrier sense range.
// The command-line options 'enableTwoBss' and 'distanceTwoBss' can be used
// to optionally enable and configure the second BSS.

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/neighbor-cache-helper.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/wifi-phy-rx-trace-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiPhyRxTraceExample");

/**
 * Function called when a packet is received.
 *
 * @param socket The receiving socket.
 */
void
ReceivePacket(Ptr<Socket> socket)
{
    Ptr<Packet> p;
    while ((p = socket->Recv()))
    {
        NS_LOG_INFO("Received packet with size " << p->GetSize());
    }
}

void
ReceiveObssPacket(Ptr<Socket> socket)
{
    Ptr<Packet> p;
    while ((p = socket->Recv()))
    {
        NS_LOG_INFO("Received packet on OBSS network with size " << p->GetSize());
    }
}

/**
 * Generate traffic.
 *
 * @param socket The sending socket.
 * @param pktSize The packet size.
 * @param pktCount The packet count.
 * @param pktInterval The interval between two packets.
 */
static void
GeneratePacket(Ptr<Socket> socket, uint32_t pktSize, uint32_t pktCount, Time pktInterval)
{
    if (pktCount > 0)
    {
        NS_LOG_INFO("Generating packet of size " << pktSize);
        socket->Send(Create<Packet>(pktSize));
        Simulator::Schedule(pktInterval,
                            &GeneratePacket,
                            socket,
                            pktSize,
                            pktCount - 1,
                            pktInterval);
    }
    else
    {
        socket->Close();
    }
}

void
PopulateNeighborCache()
{
    NeighborCacheHelper neighborCache;
    neighborCache.PopulateNeighborCache();
}

int
main(int argc, char* argv[])
{
    uint32_t packetSize = 1000; // bytes
    uint32_t numPackets = 1;
    double distance = 1;        // meters
    bool enableTwoBss = false;  // whether to enable a second (non-traced) BSS
    double distanceTwoBss = 10; // meters (distance between APs if enableTwoBss is true)
    Time interval = Seconds(1);
    bool verbose = true;
    bool logging = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("packetSize", "size of application packet sent", packetSize);
    cmd.AddValue("numPackets", "number of packets generated", numPackets);
    cmd.AddValue("interval", "interval between packets", interval);
    cmd.AddValue("distance", "distance between AP and STA", distance);
    cmd.AddValue("enableTwoBss", "enable a second BSS (not traced)", enableTwoBss);
    cmd.AddValue("distanceTwoBss", "distance between BSS (meters)", distanceTwoBss);
    cmd.AddValue("logging", "enable all wifi module log components", logging);
    cmd.AddValue("verbose", "enable this program's log components", verbose);
    cmd.Parse(argc, argv);

    if (numPackets == 0)
    {
        std::cout << "No packets configured to be sent; exiting" << std::endl;
        return 0;
    }

    NodeContainer c;
    c.Create(2);

    NodeContainer c2;
    if (enableTwoBss)
    {
        c2.Create(2);
    }

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(distance, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c);

    if (enableTwoBss)
    {
        MobilityHelper obssMobility;
        Ptr<ListPositionAllocator> obssPositionAlloc = CreateObject<ListPositionAllocator>();
        obssPositionAlloc->Add(Vector(0.0, distanceTwoBss, 0.0));
        obssPositionAlloc->Add(Vector(distance, distanceTwoBss, 0.0));
        obssMobility.SetPositionAllocator(obssPositionAlloc);
        obssMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        obssMobility.Install(c2);
    }

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211ax);

    YansWifiPhyHelper wifiPhy;
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

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
    // setup STA
    wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer staDevice = wifi.Install(wifiPhy, wifiMac, c.Get(0));
    NetDeviceContainer devices = staDevice;
    // setup AP to beacon roughly once per second (must be a multiple of 1024 us)
    wifiMac.SetType("ns3::ApWifiMac",
                    "Ssid",
                    SsidValue(ssid),
                    "BeaconInterval",
                    TimeValue(MilliSeconds(1024)));
    NetDeviceContainer apDevice = wifi.Install(wifiPhy, wifiMac, c.Get(1));
    devices.Add(apDevice);

    NetDeviceContainer obssDevices;
    if (enableTwoBss)
    {
        ssid = Ssid("obss");
        wifiMac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid));
        NetDeviceContainer obssStaDevice = wifi.Install(wifiPhy, wifiMac, c2.Get(0));
        obssDevices = obssStaDevice;
        wifiMac.SetType("ns3::ApWifiMac",
                        "Ssid",
                        SsidValue(ssid),
                        "BeaconInterval",
                        TimeValue(MilliSeconds(1024)));
        NetDeviceContainer obssApDevice = wifi.Install(wifiPhy, wifiMac, c2.Get(1));
        obssDevices.Add(obssApDevice);
    }

    InternetStackHelper internet;
    internet.Install(c);

    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i = ipv4.Assign(devices);

    if (enableTwoBss)
    {
        internet.Install(c2);
        ipv4.SetBase("10.1.2.0", "255.255.255.0");
        ipv4.Assign(obssDevices);
    }

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(c.Get(1), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    Ptr<Socket> source = Socket::CreateSocket(c.Get(0), tid);
    InetSocketAddress remote = InetSocketAddress(Ipv4Address("10.1.1.2"), 80);
    source->Connect(remote);

    Ptr<Socket> obssRecvSink;
    Ptr<Socket> obssSource;
    if (enableTwoBss)
    {
        obssRecvSink = Socket::CreateSocket(c2.Get(1), tid);
        InetSocketAddress obssLocal = InetSocketAddress(Ipv4Address::GetAny(), 80);
        obssRecvSink->Bind(obssLocal);
        obssRecvSink->SetRecvCallback(MakeCallback(&ReceiveObssPacket));

        obssSource = Socket::CreateSocket(c2.Get(0), tid);
        InetSocketAddress obssRemote = InetSocketAddress(Ipv4Address("10.1.2.2"), 80);
        obssSource->Connect(obssRemote);
    }

    // Use the NeighborCacheHelper to avoid ARP messages (ARP replies, since they are unicast,
    // count in the statistics.  The cache operation must be scheduled after WifiNetDevices are
    // started, until issue #851 is fixed.  The indirection through a normal function is
    // necessary because NeighborCacheHelper::PopulateNeighborCache() is overloaded
    Simulator::Schedule(Seconds(0.99), &PopulateNeighborCache);

    // Tracing
    wifiPhy.EnablePcap("wifi-phy-rx-trace-example", devices);
    AsciiTraceHelper asciiTrace;
    wifiPhy.EnableAsciiAll(asciiTrace.CreateFileStream("wifi-phy-rx-trace-example.tr"));

    // Logging configuration
    if (logging)
    {
        WifiHelper::EnableLogComponents(LOG_LEVEL_INFO); // Turn on all Wifi logging
    }
    if (verbose)
    {
        LogComponentEnable(
            "WifiPhyRxTraceExample",
            (LogLevel)(LOG_PREFIX_FUNC | LOG_PREFIX_NODE | LOG_PREFIX_TIME | LOG_LEVEL_ALL));
    }
    WifiPhyRxTraceHelper rxTraceHelper;
    // Enable trace helper only on one BSS
    rxTraceHelper.Enable(c);
    rxTraceHelper.Start(MilliSeconds(999)); // 1 ms before applications
    // The last packet will be sent at time 1 sec. + (numPackets - 1) * interval
    // Configure the stop time to be 1 sec. later than this.
    Time stopTime = Seconds(1) + (numPackets - 1) * interval + Seconds(1);
    rxTraceHelper.Stop(stopTime);

    Simulator::ScheduleWithContext(source->GetNode()->GetId(),
                                   Seconds(1),
                                   &GeneratePacket,
                                   source,
                                   packetSize,
                                   numPackets,
                                   interval);

    if (enableTwoBss)
    {
        Simulator::ScheduleWithContext(obssSource->GetNode()->GetId(),
                                       Seconds(1.5),
                                       &GeneratePacket,
                                       obssSource,
                                       packetSize,
                                       numPackets,
                                       interval);
    }

    Simulator::Stop(stopTime);
    Simulator::Run();

    // The following provide some examples of how to access and print the trace helper contents.

    std::cout << "*** Print statistics for all nodes using built-in print method:" << std::endl;
    rxTraceHelper.PrintStatistics();
    std::cout << std::endl;

    std::cout << "*** Print statistics for the STA only using built-in print method:" << std::endl;
    rxTraceHelper.PrintStatistics(c.Get(0)->GetId());
    std::cout << std::endl;

    std::cout << "*** Print statistics for the AP only using built-in print method:" << std::endl;
    rxTraceHelper.PrintStatistics(c.Get(1));
    std::cout << std::endl;

    std::cout << "*** Get statistics object and print the fields one-by-one:" << std::endl;
    auto stats = rxTraceHelper.GetStatistics();
    std::cout << "  overlapppingPpdu: " << stats.m_overlappingPpdus << std::endl;
    std::cout << "  nonOverlapppingPpdu: " << stats.m_nonOverlappingPpdus << std::endl;
    std::cout << "  receivedPpdus: " << stats.m_receivedPpdus << std::endl;
    std::cout << "  failedPpdus: " << stats.m_failedPpdus << std::endl;
    std::cout << "  receivedMpdus: " << stats.m_receivedMpdus << std::endl;
    std::cout << "  failedMpdus: " << stats.m_failedMpdus << std::endl;
    std::cout << std::endl;

    std::cout << "*** Get vector of reception records and print out some fields:" << std::endl;
    auto optionalRecords = rxTraceHelper.GetPpduRecords(1);
    if (optionalRecords.has_value())
    {
        auto records = optionalRecords->get(); // const std::vector<WifiPpduRxRecord>&
        std::cout << "*** Records vector has size of " << records.size() << std::endl;
        if (!records.empty())
        {
            // First record
            std::cout << "  First record:" << std::endl;
            std::cout << "    first PPDU's RSSI (dBm): " << records[0].m_rssi << std::endl;
            std::cout << "    first PPDU's receiver ID: " << records[0].m_receiverId << std::endl;
            std::cout << "    first PPDU's sender ID: " << records[0].m_senderId << std::endl;
            std::cout << "    first PPDU's start time: " << records[0].m_startTime.GetSeconds()
                      << std::endl;
            std::cout << "    first PPDU's end time: " << records[0].m_endTime.GetSeconds()
                      << std::endl;
            std::cout << "    first PPDU's number of MPDUs: " << records[0].m_statusPerMpdu.size()
                      << std::endl;
            std::cout << "    first PPDU's sender device ID: " << records[0].m_senderDeviceId
                      << std::endl;
        }
        if (records.size() > 1)
        {
            // Second record
            std::cout << "  Second record:" << std::endl;
            std::cout << "    second PPDU's RSSI (dBm): " << records[1].m_rssi << std::endl;
            std::cout << "    second PPDU's receiver ID: " << records[1].m_receiverId << std::endl;
            std::cout << "    second PPDU's sender ID: " << records[1].m_senderId << std::endl;
            std::cout << "    second PPDU's start time: " << records[1].m_startTime.GetSeconds()
                      << std::endl;
            std::cout << "    second PPDU's end time: " << records[1].m_endTime.GetSeconds()
                      << std::endl;
            std::cout << "    second PPDU's number of MPDUs: " << records[1].m_statusPerMpdu.size()
                      << std::endl;
            std::cout << "    second PPDU's sender device ID: " << records[1].m_senderDeviceId
                      << std::endl;
        }
        if (records.size() > 2)
        {
            // Third record
            std::cout << "  Third record:" << std::endl;
            std::cout << "    third PPDU's RSSI (dBm): " << records[2].m_rssi << std::endl;
            std::cout << "    third PPDU's receiver ID: " << records[2].m_receiverId << std::endl;
            std::cout << "    third PPDU's sender ID: " << records[2].m_senderId << std::endl;
            std::cout << "    third PPDU's start time: " << records[2].m_startTime.GetSeconds()
                      << std::endl;
            std::cout << "    third PPDU's end time: " << records[2].m_endTime.GetSeconds()
                      << std::endl;
            std::cout << "    third PPDU's number of MPDUs: " << records[2].m_statusPerMpdu.size()
                      << std::endl;
            std::cout << "    third PPDU's sender device ID: " << records[2].m_senderDeviceId
                      << std::endl;
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "*** Records vector is empty" << std::endl;
        std::cout << std::endl;
    }

    Simulator::Destroy();

    return 0;
}
