/*
 * Copyright (c) 2019 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/core-module.h"
#include "ns3/csma-module.h"
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

    Packet::EnablePrinting();

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("Ping", LOG_LEVEL_ALL);
        LogComponentEnable("LrWpanMac", LOG_LEVEL_ALL);
        LogComponentEnable("LrWpanPhy", LOG_LEVEL_ALL);
        LogComponentEnable("LrWpanNetDevice", LOG_LEVEL_ALL);
        LogComponentEnable("SixLowPanNetDevice", LOG_LEVEL_ALL);
    }

    uint32_t nWsnNodes = 4;
    NodeContainer wsnNodes;
    wsnNodes.Create(nWsnNodes);

    NodeContainer wiredNodes;
    wiredNodes.Create(1);
    wiredNodes.Add(wsnNodes.Get(0));

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(80),
                                  "DeltaY",
                                  DoubleValue(80),
                                  "GridWidth",
                                  UintegerValue(10),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(wsnNodes);

    LrWpanHelper lrWpanHelper;
    lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");
    // Add and install the LrWpanNetDevice for each node
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(wsnNodes);

    // Manual PAN association and extended and short address assignment.
    // Association using the MAC functions can also be used instead of this step.
    lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0);

    InternetStackHelper internetv6;
    internetv6.Install(wsnNodes);
    internetv6.Install(wiredNodes.Get(0));

    SixLowPanHelper sixLowPanHelper;
    NetDeviceContainer sixLowPanDevices = sixLowPanHelper.Install(lrwpanDevices);

    CsmaHelper csmaHelper;
    NetDeviceContainer csmaDevices = csmaHelper.Install(wiredNodes);

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:cafe::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer wiredDeviceInterfaces;
    wiredDeviceInterfaces = ipv6.Assign(csmaDevices);
    wiredDeviceInterfaces.SetForwarding(1, true);
    wiredDeviceInterfaces.SetDefaultRouteInAllNodes(1);

    ipv6.SetBase(Ipv6Address("2001:f00d::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer wsnDeviceInterfaces;
    wsnDeviceInterfaces = ipv6.Assign(sixLowPanDevices);
    wsnDeviceInterfaces.SetForwarding(0, true);
    wsnDeviceInterfaces.SetDefaultRouteInAllNodes(0);

    for (uint32_t i = 0; i < sixLowPanDevices.GetN(); i++)
    {
        Ptr<NetDevice> dev = sixLowPanDevices.Get(i);
        dev->SetAttribute("UseMeshUnder", BooleanValue(true));
        dev->SetAttribute("MeshUnderRadius", UintegerValue(10));
    }

    uint32_t packetSize = 16;
    uint32_t maxPacketCount = 5;
    Time interPacketInterval = Seconds(1.);
    PingHelper ping(wiredDeviceInterfaces.GetAddress(0, 1));

    ping.SetAttribute("Count", UintegerValue(maxPacketCount));
    ping.SetAttribute("Interval", TimeValue(interPacketInterval));
    ping.SetAttribute("Size", UintegerValue(packetSize));
    ApplicationContainer apps = ping.Install(wsnNodes.Get(nWsnNodes - 1));

    apps.Start(Seconds(2));
    apps.Stop(Seconds(20));

    AsciiTraceHelper ascii;
    lrWpanHelper.EnableAsciiAll(ascii.CreateFileStream("Ping-6LoW-lr-wpan-meshunder-lr-wpan.tr"));
    lrWpanHelper.EnablePcapAll(std::string("Ping-6LoW-lr-wpan-meshunder-lr-wpan"), true);

    csmaHelper.EnableAsciiAll(ascii.CreateFileStream("Ping-6LoW-lr-wpan-meshunder-csma.tr"));
    csmaHelper.EnablePcapAll(std::string("Ping-6LoW-lr-wpan-meshunder-csma"), true);

    Simulator::Stop(Seconds(20));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
