/*
 * Copyright (c) 2022 Chandrakant Jena
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
 * Author: Chandrakant Jena <chandrakant.barcelona@gmail.com>
 */

// Example to demonstrate the working of the Ping Application
// Network topology:
// A ------------------------------ B ------------------------------ C
//             100 Mbps                           100 Mbps
//               5 ms (one way)                     5 ms (one way)
// IPv4 addresses:
// 10.1.1.1    <->         10.1.1.2 / 10.1.2.1     <->         10.1.2.2
//
// IPv6 addresses:
// 2001:1::200:ff:fe00:1
//     <->    2001:1::200:ff:fe00:2 / 2001:1:0:1:200:ff:fe00:3
//                                       <-> 2001:1:0:1:200:ff:fe00:4
//
// The topology has three nodes interconnected by two point-to-point links.
// Each link has 5 ms one-way delay, for a round-trip propagation delay
// of 20 ms.  The transmission rate on each link is 100 Mbps.  The routing
// between links is enabled by ns-3's NixVector routing.
//
// By default, this program will send 5 pings from node A to node C using IPv6.
// When using IPv6, the output will look like this:
//
// PING 2001:1:0:1:200:ff:fe00:4 - 56 bytes of data; 104 bytes including ICMP and IPv6 headers.
// 64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=0 ttl=63 time=20.033 ms
// 64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=1 ttl=63 time=20.033 ms
// 64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=2 ttl=63 time=20.033 ms
// 64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=3 ttl=63 time=20.033 ms
// 64 bytes from (2001:1:0:1:200:ff:fe00:4): icmp_seq=4 ttl=63 time=20.033 ms
//
// --- 2001:1:0:1:200:ff:fe00:4 ping statistics ---
// 5 packets transmitted, 5 received, 0% packet loss, time 4020ms
// rtt min/avg/max/mdev = 20/20/20/0 ms
//
// When using IPv4, the output will look like this:
//
//   PING 10.1.2.2 - 56 bytes of data; 84 bytes including ICMP and IPv4 headers.
//   64 bytes from (10.1.2.2): icmp_seq=0 ttl=63 time=20.027 ms
//   64 bytes from (10.1.2.2): icmp_seq=1 ttl=63 time=20.027 ms
//   64 bytes from (10.1.2.2): icmp_seq=2 ttl=63 time=20.027 ms
//   64 bytes from (10.1.2.2): icmp_seq=3 ttl=63 time=20.027 ms
//   64 bytes from (10.1.2.2): icmp_seq=4 ttl=63 time=20.027 ms
//   --- 10.1.2.2 ping statistics ---
//   5 packets transmitted, 5 received, 0% packet loss, time 4020ms
//   rtt min/avg/max/mdev = 20/20/20/0 ms
//
// The example program will also produce four pcap traces (one for each
// NetDevice in the scenario) that can be viewed using tcpdump or Wireshark.
//
// Other program options include options to change the destination and
// source addresses, number of packets (count), packet size, interval,
// and whether to enable logging (if logging is enabled in the build).
//
// The Ping application in this example starts at simulation time 1 and will
// stop either at simulation time 50 or once 'Count' pings have been responded
// to, whichever comes first.

#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/nix-vector-routing-module.h"
#include "ns3/point-to-point-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("PingExample");

int
main(int argc, char* argv[])
{
    bool logging{false};
    Time interPacketInterval{Seconds(1.0)};
    uint32_t size{56};
    uint32_t count{5};
    std::string destinationStr;
    Address destination;
    std::string sourceStr;
    Address source;
    bool useIpv6{true};

    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    CommandLine cmd(__FILE__);
    cmd.AddValue("logging", "Tell application to log if true", logging);
    cmd.AddValue("interval", "The time to wait between two packets", interPacketInterval);
    cmd.AddValue("size", "Data bytes to be sent, per-packet", size);
    cmd.AddValue("count", "Number of packets to be sent", count);
    cmd.AddValue("destination",
                 "Destination IPv4 or IPv6 address, e.g., \"10.1.2.2\"",
                 destinationStr);
    cmd.AddValue("source",
                 "Source address, needed only for multicast or broadcast destinations",
                 sourceStr);
    cmd.Parse(argc, argv);

    if (!destinationStr.empty())
    {
        Ipv4Address v4Dst(destinationStr.c_str());
        Ipv6Address v6Dst(destinationStr.c_str());
        if (v4Dst.IsInitialized())
        {
            useIpv6 = false;
            destination = v4Dst;
        }
        else if (v6Dst.IsInitialized())
        {
            useIpv6 = true;
            destination = v6Dst;
        }
    }

    if (!sourceStr.empty())
    {
        Ipv4Address v4Src(sourceStr.c_str());
        Ipv6Address v6Src(sourceStr.c_str());
        if (v4Src.IsInitialized())
        {
            source = v4Src;
        }
        else if (v6Src.IsInitialized())
        {
            source = v6Src;
        }
    }
    if (sourceStr.empty())
    {
        if (useIpv6)
        {
            Ipv6Address v6Dst = Ipv6Address(destinationStr.c_str());
            if (v6Dst.IsInitialized() && v6Dst.IsMulticast())
            {
                std::cout << "Specify a source address to use when pinging multicast addresses"
                          << std::endl;
                std::cout << "Program exiting..." << std::endl;
                return 0;
            }
        }
        else
        {
            Ipv4Address v4Dst(destinationStr.c_str());
            if (v4Dst.IsInitialized() && (v4Dst.IsBroadcast() || v4Dst.IsMulticast()))
            {
                std::cout << "Specify a source address to use when pinging broadcast or multicast "
                             "addresses"
                          << std::endl;
                std::cout << "Program exiting..." << std::endl;
                return 0;
            }
        }
    }

    if (logging)
    {
        LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_NODE));
        LogComponentEnable("Ping", LOG_LEVEL_ALL);
    }

    NodeContainer nodes;
    nodes.Create(3);
    NodeContainer link1Nodes;
    link1Nodes.Add(nodes.Get(0));
    link1Nodes.Add(nodes.Get(1));
    NodeContainer link2Nodes;
    link2Nodes.Add(nodes.Get(1));
    link2Nodes.Add(nodes.Get(2));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("5ms"));

    NetDeviceContainer link1Devices;
    link1Devices = pointToPoint.Install(link1Nodes);
    NetDeviceContainer link2Devices;
    link2Devices = pointToPoint.Install(link2Nodes);

    // The following block of code inserts an optional packet loss model
    Ptr<PointToPointNetDevice> p2pSender = DynamicCast<PointToPointNetDevice>(link1Devices.Get(0));
    Ptr<ReceiveListErrorModel> errorModel = CreateObject<ReceiveListErrorModel>();
    std::list<uint32_t> dropList;
    // Enable one or more of the below lines to force specific packet losses
    // dropList.push_back (0);
    // dropList.push_back (1);
    // dropList.push_back (2);
    // dropList.push_back (3);
    // dropList.push_back (4);
    // etc. (other lines may be added)
    errorModel->SetList(dropList);
    p2pSender->SetReceiveErrorModel(errorModel);

    if (!useIpv6)
    {
        Ipv4NixVectorHelper nixRouting;
        InternetStackHelper stack;
        stack.SetRoutingHelper(nixRouting);
        stack.SetIpv6StackInstall(false);
        stack.Install(nodes);

        Ipv4AddressHelper addressV4;
        addressV4.SetBase("10.1.1.0", "255.255.255.0");
        addressV4.Assign(link1Devices);
        addressV4.NewNetwork();
        Ipv4InterfaceContainer link2InterfacesV4 = addressV4.Assign(link2Devices);

        if (destination.IsInvalid())
        {
            destination = link2InterfacesV4.GetAddress(1, 0);
        }
    }
    else
    {
        Ipv6NixVectorHelper nixRouting;
        InternetStackHelper stack;
        stack.SetRoutingHelper(nixRouting);
        stack.SetIpv4StackInstall(false);
        stack.Install(nodes);

        Ipv6AddressHelper addressV6;
        addressV6.SetBase("2001:1::", 64);
        addressV6.Assign(link1Devices);
        addressV6.NewNetwork();
        Ipv6InterfaceContainer link2InterfacesV6 = addressV6.Assign(link2Devices);

        if (destination.IsInvalid())
        {
            destination = link2InterfacesV6.GetAddress(1, 1);
        }
    }

    // Create Ping application and installing on node A
    PingHelper pingHelper(destination, source);
    pingHelper.SetAttribute("Interval", TimeValue(interPacketInterval));
    pingHelper.SetAttribute("Size", UintegerValue(size));
    pingHelper.SetAttribute("Count", UintegerValue(count));
    ApplicationContainer apps = pingHelper.Install(nodes.Get(0));
    apps.Start(Seconds(1));
    apps.Stop(Seconds(50));

    pointToPoint.EnablePcapAll("ping-example");

    Simulator::Stop(Seconds(60.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
