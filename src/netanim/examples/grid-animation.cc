/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Josh Pelkey <jpelkey@gatech.edu>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"

#include <iostream>

using namespace ns3;

int
main(int argc, char* argv[])
{
    Config::SetDefault("ns3::OnOffApplication::PacketSize", UintegerValue(512));
    Config::SetDefault("ns3::OnOffApplication::DataRate", StringValue("500kb/s"));

    uint32_t xSize = 5;
    uint32_t ySize = 5;
    std::string animFile = "grid-animation.xml";

    CommandLine cmd(__FILE__);
    cmd.AddValue("xSize", "Number of rows of nodes", xSize);
    cmd.AddValue("ySize", "Number of columns of nodes", ySize);
    cmd.AddValue("animFile", "File Name for Animation Output", animFile);

    cmd.Parse(argc, argv);
    if (xSize < 1 || ySize < 1 || (xSize < 2 && ySize < 2))
    {
        NS_FATAL_ERROR("Need more nodes for grid.");
    }

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    // Create Grid
    PointToPointGridHelper grid(xSize, ySize, pointToPoint);

    // Install stack on Grid
    InternetStackHelper stack;
    grid.InstallStack(stack);

    // Assign Addresses to Grid
    grid.AssignIpv4Addresses(Ipv4AddressHelper("10.1.1.0", "255.255.255.0"),
                             Ipv4AddressHelper("10.2.1.0", "255.255.255.0"));

    OnOffHelper clientHelper("ns3::UdpSocketFactory", Address());
    clientHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    clientHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer clientApps;

    // Create an on/off app sending packets
    AddressValue remoteAddress(InetSocketAddress(grid.GetIpv4Address(xSize - 1, ySize - 1), 1000));
    clientHelper.SetAttribute("Remote", remoteAddress);
    clientApps.Add(clientHelper.Install(grid.GetNode(0, 0)));

    clientApps.Start(Seconds(0));
    clientApps.Stop(Seconds(1.5));

    // Set the bounding box for animation
    grid.BoundingBox(1, 1, 100, 100);

    // Create the animation object and configure for specified output
    AnimationInterface anim(animFile);

    // Set up the actual simulation
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
