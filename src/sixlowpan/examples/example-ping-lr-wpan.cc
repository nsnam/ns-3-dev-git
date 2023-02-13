/*
 * Copyright (c) 2013 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/propagation-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/spectrum-module.h"

#include <fstream>

using namespace ns3;

int
main(int argc, char** argv)
{
    bool verbose = false;
    bool disablePcap = false;
    bool disableAsciiTrace = false;
    bool enableLSixlowLogLevelInfo = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.AddValue("disable-pcap", "disable PCAP generation", disablePcap);
    cmd.AddValue("disable-asciitrace", "disable ascii trace generation", disableAsciiTrace);
    cmd.AddValue("enable-sixlowpan-loginfo",
                 "enable sixlowpan LOG_LEVEL_INFO (used for tests)",
                 enableLSixlowLogLevelInfo);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("Ping", LOG_LEVEL_ALL);
        LogComponentEnable("LrWpanMac", LOG_LEVEL_ALL);
        LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);
        LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_ALL);
        LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_ALL);
    }

    NodeContainer nodes;
    nodes.Create(2);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(20),
                                  "DeltaY",
                                  DoubleValue(20),
                                  "GridWidth",
                                  UintegerValue(3),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    LrWpanHelper lrWpanHelper;
    // Add and install the LrWpanNetDevice for each node
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

    // Manual PAN association and extended and short address assignment.
    // Association using the MAC functions can also be used instead of this step.
    lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 1);

    InternetStackHelper internetv6;
    internetv6.Install(nodes);

    SixLowPanHelper sixlowpan;
    NetDeviceContainer devices = sixlowpan.Install(lrwpanDevices);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:2::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer deviceInterfaces;
    deviceInterfaces = ipv6.Assign(devices);

    if (enableLSixlowLogLevelInfo)
    {
        std::cout << "Device 0: pseudo-Mac-48 "
                  << Mac48Address::ConvertFrom(devices.Get(0)->GetAddress()) << ", IPv6 Address "
                  << deviceInterfaces.GetAddress(0, 1) << std::endl;
        std::cout << "Device 1: pseudo-Mac-48 "
                  << Mac48Address::ConvertFrom(devices.Get(1)->GetAddress()) << ", IPv6 Address "
                  << deviceInterfaces.GetAddress(1, 1) << std::endl;
    }

    uint32_t packetSize = 16;
    uint32_t maxPacketCount = 5;
    Time interPacketInterval = Seconds(1.);
    PingHelper ping(deviceInterfaces.GetAddress(1, 1));

    ping.SetAttribute("Count", UintegerValue(maxPacketCount));
    ping.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping.SetAttribute("Size", UintegerValue(packetSize));
    ApplicationContainer apps = ping.Install(nodes.Get(0));

    apps.Start(Seconds(2.0));
    apps.Stop(Seconds(20.0));

    if (!disableAsciiTrace)
    {
        AsciiTraceHelper ascii;
        lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("Ping-6LoW-lr-wpan.tr"));
    }
    if (!disablePcap)
    {
        lrWpanHelper.EnablePcapAll(std::string("Ping-6LoW-lr-wpan"), true);
    }
    if (enableLSixlowLogLevelInfo)
    {
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>(&std::cout);
        Ipv6RoutingHelper::PrintNeighborCacheAllAt(Seconds(9), routingStream);
    }

    Simulator::Stop(Seconds(20));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
