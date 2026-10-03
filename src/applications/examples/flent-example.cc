/*
 * Copyright (c) 2015 Universita' degli Studi di Napoli Federico II
 * Copyright (c) 2020 Harsha Sharma : Adapt for Flent
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Harsha Sharma <harshasha256@gmail.com>
 *          Tom Henderson <tomh@tomh.org>
 */

/**
 * @file flent-example.cc
 * @brief A simple example demonstrating the FlentApplication
 *
 * This example demonstrates how to set up a basic ns-3 simulation using the
 * FlentApplication to generate and measure network traffic. It configures
 * a network topology, installs the Flent applications using the FlentHelper,
 * and runs a specified network test.
 *
 * The simulation consists of the following topology:
 * client <-> router1 <-> router2 <-> server
 *
 * The middle link (router1 <-> router2) acts as the bottleneck with
 * configurable bandwidth and RTT, while the edge links have higher capacity.
 *
 * Supported Flent tests:
 * - rrul: Realtime Response Under Load (default)
 * - tcp_upload: TCP upload throughput test
 * - tcp_download: TCP download throughput test
 * - ping: Simple ICMP ping test
 *
 * To run this example with default parameters (rrul test):
 * ./ns3 run flent-example
 *
 * To run a specific test, e.g., tcp_upload:
 * ./ns3 run "flent-example --test=tcp_upload"
 *
 * To modify the bottleneck bandwidth and rtt:
 * ./ns3 run "flent-example --bw=20Mbps --rtt=20ms"
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("FlentExample");

int
main(int argc, char* argv[])
{
    std::string testName = "rrul";
    Time rtt = MilliSeconds(10);
    DataRate bw("50Mbps");
    Time length = Seconds(60);
    Time delay = Seconds(0);
    bool verbose = false;
    std::string tcpType = "TcpLinuxReno";

    // 2 MB of TCP buffer
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 21));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1448));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(true));

    CommandLine cmd(__FILE__);
    cmd.AddValue("test", "Type of ns-3 Flent test", testName);
    cmd.AddValue("rtt", "Delay value", rtt);
    cmd.AddValue("bw", "Data Rate", bw);
    cmd.AddValue("length", "Base test duration (--length in Flent)", length);
    cmd.AddValue("delay", "Time to delay test (--delay in Flent)", delay);
    cmd.AddValue("verbose", "Verbose output", verbose);
    cmd.AddValue("tcpType", "TCP congestion control type (e.g. TcpCubic, TcpLinuxReno)", tcpType);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::" + tcpType));

    // Check arguments
    if (testName != "rrul" && testName != "tcp_upload" && testName != "tcp_download" &&
        testName != "ping")
    {
        NS_FATAL_ERROR("Test name must be one of 'rrul', 'tcp_upload', 'tcp_download', or 'ping'");
    }

    std::optional<ShowProgress> progress;
    if (verbose)
    {
        progress.emplace(Seconds(10), std::cerr);
    }

    NodeContainer n;
    n.Create(4); // client <-> router1 <-> router2 <-> server
    // Create node containers for configuring individual links
    NodeContainer n0; // Group the client and router1 together
    n0.Add(n.Get(0));
    n0.Add(n.Get(1));
    NodeContainer n1; // Group the routers together
    n1.Add(n.Get(1));
    n1.Add(n.Get(2));
    NodeContainer n2; // Group the router2 and server together
    n2.Add(n.Get(2));
    n2.Add(n.Get(3));

    PointToPointHelper deviceHelper;
    DataRate edgeRate(100 * bw.GetBitRate());
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(edgeRate));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(MicroSeconds(1)));
    deviceHelper.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    NetDeviceContainer devices0;
    devices0 = deviceHelper.Install(n0);
    NetDeviceContainer devices2;
    devices2 = deviceHelper.Install(n2);
    // The middle link has the bandwidth and delay constraints
    NetDeviceContainer devices1;
    deviceHelper.SetDeviceAttribute("DataRate", DataRateValue(bw));
    deviceHelper.SetChannelAttribute("Delay", TimeValue(rtt / 2));
    devices1 = deviceHelper.Install(n1);

    // Configure the IP and traffic control layers
    InternetStackHelper stack;
    stack.InstallAll();

    TrafficControlHelper tch;
    tch.SetRootQueueDisc("ns3::FqCoDelQueueDisc");
    Config::SetDefault("ns3::FqCoDelQueueDisc::MaxSize", QueueSizeValue(QueueSize("200p")));
    tch.SetQueueLimits("ns3::DynamicQueueLimits"); // enable BQL
    QueueDiscContainer qdiscs;
    qdiscs = tch.Install(devices0);
    qdiscs = tch.Install(devices1);
    qdiscs = tch.Install(devices2);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces0 = address.Assign(devices0);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces1 = address.Assign(devices1);
    address.NewNetwork();
    Ipv4InterfaceContainer interfaces2 = address.Assign(devices2);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // Configure with the help of FlentHelper
    FlentHelper flentHelper(testName, interfaces2.GetAddress(1));
    flentHelper.SetTestStartTime(delay);
    flentHelper.SetTestLength(length);
    flentHelper.SetAttribute("StepSize", TimeValue(Seconds(0.2)));

    flentHelper.Install(n.Get(0));

    // Stop the simulation one second after Flent ends
    Simulator::Stop(flentHelper.GetStopTime() + Seconds(1));

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
