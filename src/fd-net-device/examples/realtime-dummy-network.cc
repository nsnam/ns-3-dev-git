/*
 * Copyright (c) 2012 University of Washington, 2012 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

// Network topology
//
#include "ns3/core-module.h"
#include "ns3/fd-net-device-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

#include <errno.h>
#include <sys/socket.h>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("RealtimeDummyNetworkExample");

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    GlobalValue::Bind("SimulatorImplementationType", StringValue("ns3::RealtimeSimulatorImpl"));
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    NodeContainer nodes;
    nodes.Create(2);

    InternetStackHelper stack;
    stack.Install(nodes);

    FdNetDeviceHelper fd;
    NetDeviceContainer devices = fd.Install(nodes);

    int sv[2];
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) < 0)
    {
        NS_FATAL_ERROR("Error creating pipe=" << strerror(errno));
    }

    Ptr<NetDevice> d1 = devices.Get(0);
    Ptr<FdNetDevice> device1 = d1->GetObject<FdNetDevice>();
    device1->SetFileDescriptor(sv[0]);

    Ptr<NetDevice> d2 = devices.Get(1);
    Ptr<FdNetDevice> device2 = d2->GetObject<FdNetDevice>();
    device2->SetFileDescriptor(sv[1]);

    Ipv4AddressHelper addresses;
    addresses.SetBase("10.0.0.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = addresses.Assign(devices);

    Ptr<Ping> app = CreateObject<Ping>();
    app->SetAttribute("Destination", AddressValue(interfaces.GetAddress(0)));
    app->SetAttribute("VerboseMode", EnumValue(Ping::VerboseMode::VERBOSE));
    nodes.Get(1)->AddApplication(app);
    app->SetStartTime(Seconds(0));
    app->SetStopTime(Seconds(4));

    fd.EnablePcapAll("realtime-dummy-network", false);

    Simulator::Stop(Seconds(5.));
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
