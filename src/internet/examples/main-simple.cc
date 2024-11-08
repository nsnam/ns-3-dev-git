/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@cutebugs.net>
 */

#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"

#include <iostream>

using namespace ns3;

/**
 * Generates traffic.
 *
 * The first call sends a packet of the specified size, and then
 * the function is scheduled to send a packet of (size-50) after 0.5s.
 * The process is iterated until the packet size is zero.
 *
 * @param socket output socket
 * @param size packet size
 */
static void
GenerateTraffic(Ptr<Socket> socket, int32_t size)
{
    if (size <= 0)
    {
        socket->Close();
        return;
    }

    std::cout << "at=" << Simulator::Now().GetSeconds() << "s, tx bytes=" << size << std::endl;
    socket->Send(Create<Packet>(size));
    Simulator::Schedule(Seconds(0.5), &GenerateTraffic, socket, size - 50);
}

/**
 * Prints the packets received by a socket
 * @param socket input socket
 */
static void
SocketPrinter(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    while ((packet = socket->Recv()))
    {
        std::cout << "at=" << Simulator::Now().GetSeconds() << "s, rx bytes=" << packet->GetSize()
                  << std::endl;
    }
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    NodeContainer c;
    c.Create(1);

    InternetStackHelper internet;
    internet.Install(c);

    TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
    Ptr<Socket> sink = Socket::CreateSocket(c.Get(0), tid);
    InetSocketAddress local = InetSocketAddress(Ipv4Address::GetAny(), 80);
    sink->Bind(local);

    Ptr<Socket> source = Socket::CreateSocket(c.Get(0), tid);
    InetSocketAddress remote = InetSocketAddress(Ipv4Address::GetLoopback(), 80);
    source->Connect(remote);

    GenerateTraffic(source, 500);
    sink->SetRecvCallback(MakeCallback(&SocketPrinter));

    Simulator::Run();

    Simulator::Destroy();

    return 0;
}
