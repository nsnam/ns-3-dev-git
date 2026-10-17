/*
 * Copyright (c) 2025
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Boh Jie Qi <jieqiboh5836@gmail.com>
 */

// Network Topology:
//
//            +---------+
//            |  6LBR   |  (Node 0)
//            +----+----+
//                 |
//        +--------+-------- ... --------+
//        |        |                     |
//     +--+--+  +--+--+               +--+--+
//     |6LN1 |  |6LN2 |               |6LNn |
//     +-----+  +-----+               +-----+
//     (Node 1) (Node 2)              (Node n)

#include "ns3/core-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/spectrum-module.h"

#include <fstream>

using namespace ns3;

int
main(int argc, char* argv[])
{
    // Command‑line flags:
    bool disablePcap = false;
    uint32_t numLn = 4; // default number of 6LNs

    CommandLine cmd(__FILE__);
    cmd.AddValue("disable-pcap", "Disable PCAP generation on all LR‑WPAN devices", disablePcap);
    cmd.AddValue("num-ln", "Number of 6LNs (in addition to the single 6LBR)", numLn);
    cmd.Parse(argc, argv);

    // Create 1 6LBR and numLn 6LNs
    NodeContainer nodes;
    nodes.Create(1 + numLn); // node 0 = 6LBR, node 1-4 = 6LNs

    // Set constant positions
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(nodes);

    // Install LrWpanNetDevices
    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
    lrWpanHelper.CreateAssociatedPan(lrwpanDevices, 0);

    // Install Internet stack
    InternetStackHelper internetv6;
    internetv6.Install(nodes);

    // Install 6LoWPAN on top of LrWpan
    SixLowPanHelper sixlowpan;
    NetDeviceContainer devices = sixlowpan.Install(lrwpanDevices);

    // Configure 6LoWPAN ND
    // Node 0 = 6LBR
    sixlowpan.InstallSixLowPanNdBorderRouter(devices.Get(0), "2001::");
    sixlowpan.SetAdvertisedPrefix(devices.Get(0), Ipv6Prefix("2001::", 64));
    sixlowpan.AddAdvertisedContext(devices.Get(0), Ipv6Prefix("2001::", 64));

    // Nodes 1-n = 6LNs
    for (uint32_t i = 1; i <= numLn; ++i)
    {
        sixlowpan.InstallSixLowPanNdNode(devices.Get(i));
    }

    // Set up ping from each 6LN to 6LBR (link-local address of 6LBR is fe80::ff:fe00:1)
    for (uint32_t i = 1; i <= numLn; ++i)
    {
        std::ostringstream oss;
        oss << "fe80::ff:fe00:" << std::hex << (i + 1); // 6LN link-local address

        PingHelper ping6;
        ping6.SetAttribute("Count", UintegerValue(1));
        ping6.SetAttribute("Interval", TimeValue(Seconds(1.0)));
        ping6.SetAttribute("Size", UintegerValue(16));
        ping6.SetAttribute("Destination", AddressValue(Ipv6Address("fe80::ff:fe00:1"))); // 6LBR
        ping6.SetAttribute("InterfaceAddress", AddressValue(Ipv6Address(oss.str().c_str()))); // 6LN

        ApplicationContainer apps = ping6.Install(nodes.Get(i));
        Time startTime = Seconds(15.0); // wait for all nodes to finish registration
        Time stopTime = Seconds(20.0);

        apps.Start(startTime);
        apps.Stop(stopTime);
    }

    AsciiTraceHelper ascii;
    if (!disablePcap)
    {
        lrWpanHelper.EnablePcapAll(std::string("example-sixlowpan-nd-basic"), true);
    }

    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
