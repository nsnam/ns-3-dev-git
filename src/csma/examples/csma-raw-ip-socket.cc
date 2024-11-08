/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

//
// Network topology
//    (sender)         (receiver)
//       n0    n1   n2   n3
//       |     |    |    |
//     =====================
//
// Node n0 sends data to node n3 over a raw IP socket.  The protocol
// number used is 2.
// The default traffic rate is to send 1250 bytes/second for 10 kb/s
// The data rate can be toggled by command line argument

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("CsmaRawIpSocketExample");

/**
 * Receive sink function
 *
 * @param p the packet.
 * @param ad the sender's address.
 */
static void
SinkRx(Ptr<const Packet> p, const Address& ad)
{
    // Enable the below line to see the packet contents printed out at the
    // receive sink
    // std::cout << Simulator::Now().As (Time::S) << " " << *p << std::endl;
}

int
main(int argc, char* argv[])
{
#if 0
  LogComponentEnable ("CsmaPacketSocketExample", LOG_LEVEL_INFO);
#endif
    uint32_t dataRate = 10;
    CommandLine cmd(__FILE__);
    cmd.AddValue("dataRate", "application dataRate (Kb/s)", dataRate);
    cmd.Parse(argc, argv);

    // Here, we will explicitly create four nodes.
    NS_LOG_INFO("Create nodes.");
    NodeContainer c;
    c.Create(4);

    // connect all our nodes to a shared channel.
    NS_LOG_INFO("Build Topology.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", DataRateValue(DataRate(5000000)));
    csma.SetChannelAttribute("Delay", TimeValue(MilliSeconds(2)));
    csma.SetDeviceAttribute("EncapsulationMode", StringValue("Llc"));
    NetDeviceContainer devs = csma.Install(c);

    // add an ip stack to all nodes.
    NS_LOG_INFO("Add ip stack.");
    InternetStackHelper ipStack;
    ipStack.Install(c);

    // assign ip addresses
    NS_LOG_INFO("Assign ip addresses.");
    Ipv4AddressHelper ip;
    ip.SetBase("192.168.1.0", "255.255.255.0");
    Ipv4InterfaceContainer addresses = ip.Assign(devs);

    NS_LOG_INFO("Create Source");
    // IP protocol configuration
    Config::SetDefault("ns3::Ipv4RawSocketImpl::Protocol", StringValue("2"));
    InetSocketAddress dst(addresses.GetAddress(3));
    OnOffHelper onoff = OnOffHelper("ns3::Ipv4RawSocketFactory", dst);
    onoff.SetConstantRate(DataRate(dataRate * 1000));
    onoff.SetAttribute("PacketSize", UintegerValue(1250));

    ApplicationContainer apps = onoff.Install(c.Get(0));
    apps.Start(Seconds(0.5));
    apps.Stop(Seconds(11));

    NS_LOG_INFO("Create Sink.");
    PacketSinkHelper sink = PacketSinkHelper("ns3::Ipv4RawSocketFactory", dst);
    apps = sink.Install(c.Get(3));
    apps.Start(Seconds(0));
    apps.Stop(Seconds(12));

    NS_LOG_INFO("Configure Tracing.");
    // first, pcap tracing in non-promiscuous mode
    csma.EnablePcapAll("csma-raw-ip-socket", false);
    // then, print what the packet sink receives.
    Config::ConnectWithoutContext("/NodeList/3/ApplicationList/0/$ns3::PacketSink/Rx",
                                  MakeCallback(&SinkRx));

    Packet::EnablePrinting();

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
