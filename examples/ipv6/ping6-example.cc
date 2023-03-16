/*
 * Copyright (c) 2008-2009 Strasbourg University
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

// Network topology
//
//       n0    n1
//       |     |
//       =================
//              LAN
//
// - ICMPv6 echo request flows from n0 to n1 and back with ICMPv6 echo reply
// - DropTail queues
// - Tracing of queues and packet receptions to file "ping6.tr"

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ping6Example");

int
main(int argc, char** argv)
{
    bool verbose = false;
    bool allNodes = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("allNodes", "Ping all the nodes (true) or just one neighbor (false)", allNodes);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("Ping6Example", LOG_LEVEL_INFO);
        LogComponentEnable("Ipv6EndPointDemux", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6L3Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6StaticRouting", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6ListRouting", LOG_LEVEL_ALL);
        LogComponentEnable("Ipv6Interface", LOG_LEVEL_ALL);
        LogComponentEnable("Icmpv6L4Protocol", LOG_LEVEL_ALL);
        LogComponentEnable("Ping", LOG_LEVEL_ALL);
        LogComponentEnable("NdiscCache", LOG_LEVEL_ALL);
    }

    NS_LOG_INFO("Create nodes.");
    NodeContainer n;
    n.Create(4);

    /* Install IPv4/IPv6 stack */
    InternetStackHelper internetv6;
    internetv6.SetIpv4StackInstall(false);
    internetv6.Install(n);

    NS_LOG_INFO("Create channels.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(5000000));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    NetDeviceContainer d = csma.Install(n);

    Ipv6AddressHelper ipv6;
    NS_LOG_INFO("Assign IPv6 Addresses.");
    Ipv6InterfaceContainer i = ipv6.Assign(d);

    NS_LOG_INFO("Create Applications.");

    // Create a Ping application to send ICMPv6 echo request from node zero
    uint32_t packetSize = 1024;
    uint32_t maxPacketCount = 5;

    Ipv6Address destination = allNodes ? Ipv6Address::GetAllNodesMulticast() : i.GetAddress(1, 0);
    PingHelper ping(destination);

    ping.SetAttribute("Count", UintegerValue(maxPacketCount));
    ping.SetAttribute("Size", UintegerValue(packetSize));
    ping.SetAttribute("InterfaceAddress", AddressValue(i.GetAddress(0, 0)));

    ApplicationContainer apps = ping.Install(n.Get(0));
    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(10.0));

    AsciiTraceHelper ascii;
    csma.EnableAsciiAll(ascii.CreateFileStream("ping6.tr"));
    csma.EnablePcapAll(std::string("ping6"), true);

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
