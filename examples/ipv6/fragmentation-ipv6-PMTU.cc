/*
 * Copyright (c) 2008-2009 Strasbourg University
 * Copyright (c) 2013 Universita' di Firenze
 * Copyright (c) 2022 Jadavpur University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: David Gross <gdavid.devel@gmail.com>
 *         Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 * Modified by Akash Mondal <a98mondal@gmail.com>
 */

// Network topology
// //
// //      Src  n0        r1     n1      r2        n2
// //           |         _      |       _         |
// //           =========|_|============|_|=========
// //      MTU   5000           2000          1500
// //
// // - Tracing of queues and packet receptions to file "fragmentation-ipv6-PMTU.tr"

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv6-routing-table-entry.h"
#include "ns3/ipv6-static-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FragmentationIpv6PmtuExample");

int
main(int argc, char** argv)
{
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("Ipv6L3Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv6L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6Interface", LOG_LEVEL_ALL);
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_ALL);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_ALL);
    }
    LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);

    NS_LOG_INFO("Create nodes.");
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> r1 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<Node> r2 = CreateObject<Node>();
    Ptr<Node> n2 = CreateObject<Node>();

    NodeContainer net1(n0, r1);
    NodeContainer net2(r1, n1, r2);
    NodeContainer net3(r2, n2);
    NodeContainer all(n0, r1, n1, r2, n2);

    NS_LOG_INFO("Create IPv6 Internet Stack");
    InternetStackHelper internetv6;
    internetv6.Install(all);

    NS_LOG_INFO("Create channels.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    csma.SetDeviceAttribute("Mtu", UintegerValue(2000));
    NetDeviceContainer d2 = csma.Install(net2); // CSMA Network with MTU 2000

    csma.SetDeviceAttribute("Mtu", UintegerValue(5000));
    NetDeviceContainer d1 = csma.Install(net1); // CSMA Network with MTU 5000

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", DataRateValue(5000000));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));
    pointToPoint.SetDeviceAttribute("Mtu", UintegerValue(1500));
    NetDeviceContainer d3 = pointToPoint.Install(net3); // P2P Network with MTU 1500

    NS_LOG_INFO("Create networks and assign IPv6 Addresses.");
    Ipv6AddressHelper ipv6;

    ipv6.SetBase(Ipv6Address("2001:1::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i1 = ipv6.Assign(d1);
    i1.SetForwarding(1, true);
    i1.SetDefaultRouteInAllNodes(1);

    ipv6.SetBase(Ipv6Address("2001:2::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i2 = ipv6.Assign(d2);
    i2.SetForwarding(0, true);
    i2.SetDefaultRouteInAllNodes(0);
    i2.SetForwarding(1, true);
    i2.SetDefaultRouteInAllNodes(0);
    i2.SetForwarding(2, true);
    i2.SetDefaultRouteInAllNodes(2);

    ipv6.SetBase(Ipv6Address("2001:3::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer i3 = ipv6.Assign(d3);
    i3.SetForwarding(0, true);
    i3.SetDefaultRouteInAllNodes(0);

    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(&std::cout);
    Ipv6RoutingHelper::PrintRoutingTableAt(Seconds(0), r1, routingStream);

    // Create an UDP Echo server on n2
    UdpEchoServerHelper echoServer(42);
    ApplicationContainer serverApps = echoServer.Install(n2);
    serverApps.Start(Seconds(0));
    serverApps.Stop(Seconds(30));

    uint32_t maxPacketCount = 5;

    // Create an UDP Echo client on n1 to send UDP packets to n2 via r1
    uint32_t packetSizeN1 = 1600; // Packet should fragment as intermediate link MTU is 1500
    UdpEchoClientHelper echoClient(i3.GetAddress(1, 1), 42);
    echoClient.SetAttribute("PacketSize", UintegerValue(packetSizeN1));
    echoClient.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    ApplicationContainer clientAppsN1 = echoClient.Install(n1);
    clientAppsN1.Start(Seconds(2));
    clientAppsN1.Stop(Seconds(10));

    // Create an UDP Echo client on n0 to send UDP packets to n2 via r0 and r1
    uint32_t packetSizeN2 = 4000; // Packet should fragment as intermediate link MTU is 1500
    echoClient.SetAttribute("PacketSize", UintegerValue(packetSizeN2));
    echoClient.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    ApplicationContainer clientAppsN2 = echoClient.Install(n1);
    clientAppsN2.Start(Seconds(11));
    clientAppsN2.Stop(Seconds(20));

    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("fragmentation-ipv6-PMTU.tr"));
    csma.EnablePcapAll(std::string("fragmentation-ipv6-PMTU"), true);
    pointToPoint.EnablePcapAll(std::string("fragmentation-ipv6-PMTU"), true);

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
