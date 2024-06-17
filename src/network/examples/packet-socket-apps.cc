/*
 * Copyright (c) 2014 Universita' di Firenze
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

// Network topology
//
//       n0              n1
//       |               |
//       =================
//         SimpleChannel
//
// - Packets flows from n0 to n1
//
// This example shows how to use the PacketSocketServer and PacketSocketClient
// to send non-IP packets over a SimpleNetDevice

#include "ns3/core-module.h"
#include "ns3/network-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    bool verbose = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("verbose", "turn on log components", verbose);
    cmd.Parse(argc, argv);

    if (verbose)
    {
        LogComponentEnable("PacketSocketServer", LOG_LEVEL_ALL);
        LogComponentEnable("PacketSocketClient", LOG_LEVEL_ALL);
        LogComponentEnable("SimpleNetDevice", LOG_LEVEL_ALL);
    }

    NodeContainer nodes;
    nodes.Create(2);

    ns3::PacketMetadata::Enable();

    PacketSocketHelper packetSocket;

    // give packet socket powers to nodes.
    packetSocket.Install(nodes);

    Ptr<SimpleNetDevice> txDev;
    txDev = CreateObject<SimpleNetDevice>();
    nodes.Get(0)->AddDevice(txDev);

    Ptr<SimpleNetDevice> rxDev;
    rxDev = CreateObject<SimpleNetDevice>();
    nodes.Get(1)->AddDevice(rxDev);

    Ptr<SimpleChannel> channel = CreateObject<SimpleChannel>();
    txDev->SetChannel(channel);
    rxDev->SetChannel(channel);
    txDev->SetNode(nodes.Get(0));
    rxDev->SetNode(nodes.Get(1));

    PacketSocketAddress socketAddr;
    socketAddr.SetSingleDevice(txDev->GetIfIndex());
    socketAddr.SetPhysicalAddress(rxDev->GetAddress());
    // Arbitrary protocol type.
    // Note: PacketSocket doesn't have any L4 multiplexing or demultiplexing
    //       The only mux/demux is based on the protocol field
    socketAddr.SetProtocol(1);

    Ptr<PacketSocketClient> client = CreateObject<PacketSocketClient>();
    client->SetRemote(socketAddr);
    nodes.Get(0)->AddApplication(client);

    Ptr<PacketSocketServer> server = CreateObject<PacketSocketServer>();
    server->SetLocal(socketAddr);
    nodes.Get(1)->AddApplication(server);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
