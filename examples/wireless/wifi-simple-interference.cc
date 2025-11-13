/*
 * Copyright (c) 2009 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

// This script configures three nodes on an 802.11b physical layer, with
// 802.11b NICs in adhoc mode.  There is a transmitter, receiver, and
// interferer.  The transmitter sends one packet to the receiver and
// the receiver receives it with a certain configurable RSS (by default,
// -80 dBm).  The interferer does not do carrier sense and also sends
// the packet to interfere with the primary packet.  The channel model
// is clear channel.
//
// Therefore, at the receiver, the reception looks like this:
//
//     ------------------time---------------->
//     t0
//
//     |------------------------------------|
//     |                                    |
//     | primary received frame (time t0)   |
//     |                                    |
//     |------------------------------------|
//
//
//         t1
//         |-----------------------------------|
//         |                                   |
//         |  interfering frame (time t1)      |
//         |                                   |
//         |-----------------------------------|
//
// The orientation is:
//     n2  ---------> n0 <---------- n1
//  interferer      receiver       transmitter
//
// The configurable parameters are:
//   - Prss (primary rss) (-80 dBm default)
//   - Irss (interfering rss) (-95 dBm default)
//   - delta (t1-t0, may be negative, default 0ns)
//   - PpacketSize (primary packet size) (bytes, default 1000)
//   - IpacketSize (interferer packet size) (bytes, default 1000)
//
// For instance, for this configuration, the interfering frame arrives
// at -90 dBm with a time offset of 3.2 microseconds:
//
// ./ns3 run "wifi-simple-interference --Irss=-90 --delta=3.2us"
//
// Note that all ns-3 attributes (not just the ones exposed in the below
// script) can be changed at command line; see the documentation.
//
// This script can also be helpful to put the Wifi layer into verbose
// logging mode; this command will turn on all wifi logging:
//
// ./ns3 run "wifi-simple-interference --verbose=1"
//
// When you are done, you will notice a pcap trace file in your directory.
// If you have tcpdump installed, you can try this:
//
// tcpdump -r wifi-simple-interference-0-0.pcap -nn -tt
// reading from file wifi-simple-interference-0-0.pcap, link-type IEEE802_11_RADIO (802.11 plus
// radiotap header) 10.008754 13308699848833236992us tsft fragmented 0.0 Mb/s 0 MHz 16dBm signal
// 2dBm noise IP 10.0.0.2.49153 > 255.255.255.255.80: UDP, length 1000
//
// With a zero delta time offset, only the first packet will be decoded; the second packet
// transmission must be delayed past the end of the first packet to receive it..
//
// Next, try this command and look at the tcpdump-- you should see two packets
// that are no longer interfering:
// ./ns3 run "wifi-simple-interference --delta=9000us"

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/ssid.h"
#include "ns3/string.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("WifiSimpleInterference");

/**
 * Print a packer that has been received.
 *
 * @param socket The receiving socket.
 * @return a string with the packet details.
 */
static inline std::string
PrintReceivedPacket(Ptr<Socket> socket)
{
    Address addr;

    std::ostringstream oss;

    while (socket->Recv())
    {
        socket->GetSockName(addr);
        InetSocketAddress iaddr = InetSocketAddress::ConvertFrom(addr);

        oss << "Received one packet!  Socket: " << iaddr.GetIpv4() << " port: " << iaddr.GetPort();
    }

    return oss.str();
}

/**
 * Function called when a packet is received.
 *
 * @param socket The receiving socket.
 */
static void
ReceivePacket(Ptr<Socket> socket)
{
    NS_LOG_UNCOND(PrintReceivedPacket(socket));
}

/**
 * Generate traffic
 *
 * @param socket The sending socket.
 * @param pktSize The packet size.
 */
static void
GenerateTraffic(Ptr<Socket> socket, uint32_t pktSize)
{
    socket->Send(Create<Packet>(pktSize));
}

int
main(int argc, char* argv[])
{
    std::string phyMode{"DsssRate1Mbps"};
    dBm_u Prss{-80};
    dBm_u Irss{-95};
    Time delta{"0ns"};
    uint32_t PpacketSize{1000}; // bytes
    uint32_t IpacketSize{1000}; // bytes
    bool verbose{false};

    // these are not command line arguments for this version
    Time startTime{"10s"};
    meter_u distanceToRx{100.0}; // If you change this, also change TxGain below

    CommandLine cmd(__FILE__);
    cmd.AddValue("phyMode", "Wifi Phy mode", phyMode);
    cmd.AddValue("Prss", "Intended primary received signal strength (dBm)", Prss);
    cmd.AddValue("Irss", "Intended interfering received signal strength (dBm)", Irss);
    cmd.AddValue("delta", "time offset for interfering signal", delta);
    cmd.AddValue("PpacketSize", "size of application packet sent", PpacketSize);
    cmd.AddValue("IpacketSize", "size of interfering packet sent", IpacketSize);
    cmd.AddValue("verbose", "turn on all WifiNetDevice log components", verbose);
    cmd.Parse(argc, argv);

    // Fix non-unicast data rate to be the same as that of unicast
    Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue(phyMode));

    NodeContainer c;
    c.Create(3);

    // The below set of helpers will help us to put together the wifi NICs we want
    WifiHelper wifi;
    if (verbose)
    {
        WifiHelper::EnableLogComponents(); // Turn on all Wifi logging
    }
    wifi.SetStandard(WIFI_STANDARD_80211b);

    YansWifiPhyHelper wifiPhy;

    // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
    wifiPhy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);

    // Disable preamble detection model to receive signals below -82 dBm
    wifiPhy.DisablePreambleDetectionModel();

    YansWifiChannelHelper wifiChannel;
    wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
    wifiChannel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");
    wifiPhy.SetChannel(wifiChannel.Create());

    // Add a mac and disable rate control
    WifiMacHelper wifiMac;
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue(phyMode),
                                 "ControlMode",
                                 StringValue(phyMode));
    // Set it to adhoc mode
    wifiMac.SetType("ns3::AdhocWifiMac");
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, c.Get(0));
    // Setting RxSensitivity to 0 dBm will disable the two sending devices from detecting
    // received signals, so that they do not back off
    wifiPhy.Set("RxSensitivity", DoubleValue(0));
    // We use the TxGain parameter on each sender to control the received signal power.
    // The transmit power is roughly 16 dBm.  The signal attenuation from both senders to
    // the receiving device is 106.7 dB (100 meters at this frequency, based on the
    // LogDistancePropagationLossModel).  We want the receive signal strength to be
    // Prss dBm.  We therefore want to solve for the gain as follows:
    // 16 dBm + TxGain - propagationLoss = Prss (dBm)
    // Working backwards, TxGain = Prss (dBm) - 16 dB + 106.7 dB = Prss (dBm) + 90.7 dB
    dB_u powerOffset{90.7};
    wifiPhy.Set("TxGain", DoubleValue(Prss + powerOffset));
    devices.Add(wifi.Install(wifiPhy, wifiMac, c.Get(1)));
    // Repeat for the interferer
    wifiPhy.Set("TxGain", DoubleValue(Irss + powerOffset));
    devices.Add(wifi.Install(wifiPhy, wifiMac, c.Get(2)));

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0));
    positionAlloc->Add(Vector(distanceToRx, 0.0, 0.0));
    positionAlloc->Add(Vector(-1 * distanceToRx, 0.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c);

    InternetStackHelper internet;
    internet.Install(c);

    Ipv4AddressHelper address;
    address.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer ipInterfaces;
    ipInterfaces = address.Assign(devices);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> recvSink = Socket::CreateSocket(c.Get(0), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address("10.1.1.1"), 80);
    recvSink->Bind(local);
    recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

    Ptr<Socket> source = Socket::CreateSocket(c.Get(1), tid);
    InetSocketAddress remote = InetSocketAddress(Ipv4Address("255.255.255.255"), 80);
    source->SetAllowBroadcast(true);
    source->Connect(remote);

    // Interferer will send to a different port; we will not see a
    // "Received packet" message
    Ptr<Socket> interferer = Socket::CreateSocket(c.Get(2), tid);
    InetSocketAddress interferingAddr = InetSocketAddress(Ipv4Address("255.255.255.255"), 49000);
    interferer->SetAllowBroadcast(true);
    interferer->Connect(interferingAddr);

    // Tracing
    wifiPhy.EnablePcap("wifi-simple-interference", devices.Get(0));

    // Output what we are doing
    NS_LOG_UNCOND("Primary packet RSS=" << Prss << " dBm and interferer RSS=" << Irss
                                        << " dBm at time offset=" << delta.As(Time::US));

    Simulator::ScheduleWithContext(source->GetNode()->GetId(),
                                   startTime,
                                   &GenerateTraffic,
                                   source,
                                   PpacketSize);

    Simulator::ScheduleWithContext(interferer->GetNode()->GetId(),
                                   startTime + delta,
                                   &GenerateTraffic,
                                   interferer,
                                   IpacketSize);

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
