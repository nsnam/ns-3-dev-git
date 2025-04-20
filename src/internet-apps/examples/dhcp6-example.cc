/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

/*
 * Network layout:
 * The following devices have one CSMA interface each -
 * S0 - DHCPv6 server
 * N0, N1 - DHCPv6 clients
 * R0 - router
 *                ┌-------------------------------------------------┐
 *                | DHCPv6 Clients                                  |
 *                |                                                 |
 *                |                                Static address   |
 *                |                                2001:cafe::42:2  |
 *                |   ┌──────┐       ┌──────┐        ┌──────┐       |
 *                |   │  N0  │       │  N1  │        │  N2  │       |
 *                |   └──────┘       └──────┘        └──────┘       |
 *                |       │              │               │          |
 *                └-------│--------------│---------------│----------┘
 *  DHCPv6 Server         │              │               │
 *        ┌──────┐        │              │               │      ┌──────┐
 *        │  S0  │────────┴──────────────┴───────────────┴──────│  R0  │Router
 *        └──────┘                                              └──────┘
 * Notes:
 * 1. The DHCPv6 server is not assigned any static address as it operates only
 *    in the link-local domain.
 * 2. N2 has a statically assigned address to demonstrate the operation of the
 *    DHCPv6 Decline message.
 * 3. The server is usually on the router in practice, but we demonstrate in
 *    this example a standalone server.
 * 4. Linux uses fairly large values for address lifetimes (in thousands of
 *    seconds). In this example, we have set shorter lifetimes for the purpose
 *    of observing the Renew messages within a shorter simulation run.
 * 5. The nodes use two interfaces each for the purpose of demonstrating
 *    DHCPv6 operation when multiple interfaces are present on the client or
 *    server nodes.
 *
 * This example demonstrates how to set up a simple DHCPv6 server and two DHCPv6
 * clients. The clients begin to request an address lease using a Solicit
 * message only after receiving a Router Advertisement containing the 'M' bit
 * from the router, R0.
 *
 * The server responds with an Advertise message with all available address
 * offers, and the client sends a Request message to the server for these
 * addresses. The server then sends a Reply message to the client, which
 * performs Duplicate Address Detection to check if any other node on the link
 * already uses this address.
 * If the address is in use by any other node, the client sends a Decline
 * message to the server. If the address is not in use, the client begins using
 * this address.
 * At the end of the address lease lifetime, the client sends a Renew message
 * to the server, which renews the lease and allows the client to continue using
 * the same address.
 *
 * The user may enable packet traces in this example to observe the following
 * message exchanges:
 * 1. Solicit - Advertise - Request - Reply
 * 2. Solicit - Advertise - Request - Reply - Decline
 * 3. Renew - Reply
 *
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-apps-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ssid.h"
#include "ns3/wifi-helper.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Dhcp6Example");

void
SetInterfaceDown(Ptr<Node> node, uint32_t interface)
{
    node->GetObject<Ipv6>()->SetDown(interface);
}

void
SetInterfaceUp(Ptr<Node> node, uint32_t interface)
{
    node->GetObject<Ipv6>()->SetUp(interface);
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);

    bool verbose = false;
    bool enablePcap = false;
    cmd.AddValue("verbose", "Turn on the logs", verbose);
    cmd.AddValue("enablePcap", "Enable/Disable pcap file generation", enablePcap);

    cmd.Parse(argc, argv);
    GlobalValue::Bind("ChecksumEnabled", BooleanValue(true));

    if (verbose)
    {
        LogComponentEnable("Dhcp6Server", LOG_LEVEL_INFO);
        LogComponentEnable("Dhcp6Client", LOG_LEVEL_INFO);
    }

    Time stopTime = Seconds(25.0);

    NS_LOG_INFO("Create nodes.");
    NodeContainer nonRouterNodes;
    nonRouterNodes.Create(4);
    Ptr<Node> router = CreateObject<Node>();
    NodeContainer all(nonRouterNodes, router);

    NS_LOG_INFO("Create channels.");
    CsmaHelper csma;
    csma.SetChannelAttribute("DataRate", StringValue("5Mbps"));
    csma.SetChannelAttribute("Delay", StringValue("2ms"));
    NetDeviceContainer devices = csma.Install(all); // all nodes

    InternetStackHelper internetv6;
    internetv6.Install(all);

    NS_LOG_INFO("Create networks and assign IPv6 Addresses.");

    Ipv6AddressHelper ipv6;
    ipv6.SetBase(Ipv6Address("2001:cafe::"), Ipv6Prefix(64));
    NetDeviceContainer nonRouterDevices;
    nonRouterDevices.Add(devices.Get(0)); // The server node, S0.
    nonRouterDevices.Add(devices.Get(1)); // The first client node, N0.
    nonRouterDevices.Add(devices.Get(2)); // The second client node, N1.
    nonRouterDevices.Add(devices.Get(3)); // The third client node, N2.
    Ipv6InterfaceContainer i = ipv6.AssignWithoutAddress(nonRouterDevices);

    NS_LOG_INFO("Assign static IP address to the third node.");
    Ptr<Ipv6> ipv6proto = nonRouterNodes.Get(3)->GetObject<Ipv6>();
    int32_t ifIndex = ipv6proto->GetInterfaceForDevice(devices.Get(3));
    Ipv6InterfaceAddress ipv6Addr =
        Ipv6InterfaceAddress(Ipv6Address("2001:cafe::42:2"), Ipv6Prefix(128));
    ipv6proto->AddAddress(ifIndex, ipv6Addr);

    NS_LOG_INFO("Assign static IP address to the router node.");
    NetDeviceContainer routerDevice;
    routerDevice.Add(devices.Get(4)); // CSMA interface of the node R0.
    Ipv6InterfaceContainer r1 = ipv6.Assign(routerDevice);
    r1.SetForwarding(0, true);

    NS_LOG_INFO("Create Radvd applications.");
    RadvdHelper radvdHelper;

    /* Set up unsolicited RAs */
    radvdHelper.AddAnnouncedPrefix(r1.GetInterfaceIndex(0), Ipv6Address("2001:cafe::1"), 64);
    radvdHelper.GetRadvdInterface(r1.GetInterfaceIndex(0))->SetManagedFlag(true);

    NS_LOG_INFO("Create DHCP applications.");
    Dhcp6Helper dhcp6Helper;

    NS_LOG_INFO("Set timers to desired values.");
    dhcp6Helper.SetServerAttribute("RenewTime", StringValue("10s"));
    dhcp6Helper.SetServerAttribute("RebindTime", StringValue("16s"));
    dhcp6Helper.SetServerAttribute("PreferredLifetime", StringValue("18s"));
    dhcp6Helper.SetServerAttribute("ValidLifetime", StringValue("20s"));

    // DHCPv6 clients
    NodeContainer nodes = NodeContainer(nonRouterNodes.Get(1), nonRouterNodes.Get(2));
    ApplicationContainer dhcpClients = dhcp6Helper.InstallDhcp6Client(nodes);
    dhcpClients.Start(Seconds(1.0));
    dhcpClients.Stop(stopTime);

    // DHCP server
    NetDeviceContainer serverNetDevices;
    serverNetDevices.Add(nonRouterDevices.Get(0));
    ApplicationContainer dhcpServerApp = dhcp6Helper.InstallDhcp6Server(serverNetDevices);

    Ptr<Dhcp6Server> server = DynamicCast<Dhcp6Server>(dhcpServerApp.Get(0));
    server->AddSubnet(Ipv6Address("2001:cafe::"),
                      Ipv6Prefix(64),
                      Ipv6Address("2001:cafe::42:1"),
                      Ipv6Address("2001:cafe::42:ffff"));

    dhcpServerApp.Start(Seconds(0.0));
    dhcpServerApp.Stop(stopTime);

    ApplicationContainer radvdApps = radvdHelper.Install(router);
    radvdApps.Start(Seconds(1.0));
    radvdApps.Stop(stopTime);

    Simulator::Stop(stopTime + Seconds(2.0));

    // Schedule N0 interface to go down and come up to see the effects of
    // link state change.
    Simulator::Schedule(Seconds(4), &SetInterfaceDown, nonRouterNodes.Get(1), 1);
    Simulator::Schedule(Seconds(5), &SetInterfaceUp, nonRouterNodes.Get(1), 1);

    if (enablePcap)
    {
        csma.EnablePcapAll("dhcp6-csma");
    }

    NS_LOG_INFO("Run Simulation.");
    Simulator::Run();

    NS_LOG_INFO("Done.");
    Simulator::Destroy();
    return 0;
}
