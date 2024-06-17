/*
 * Copyright (c) 2016 Magister Solutions
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Lauri Sormunen <lauri.sormunen@magister.fi>
 */

#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeGppHttpExample");

void
ServerConnectionEstablished(Ptr<const ThreeGppHttpServer>, Ptr<Socket>)
{
    NS_LOG_INFO("Client has established a connection to the server.");
}

void
MainObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated a main object of " << size << " bytes.");
}

void
EmbeddedObjectGenerated(uint32_t size)
{
    NS_LOG_INFO("Server generated an embedded object of " << size << " bytes.");
}

void
ServerTx(Ptr<const Packet> packet)
{
    NS_LOG_INFO("Server sent a packet of " << packet->GetSize() << " bytes.");
}

void
ClientRx(Ptr<const Packet> packet, const Address& address)
{
    NS_LOG_INFO("Client received a packet of " << packet->GetSize() << " bytes from " << address);
}

void
ClientMainObjectReceived(Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::MAIN_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received a main object of " << p->GetSize()
                                                                         << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse a main object. ");
    }
}

void
ClientEmbeddedObjectReceived(Ptr<const ThreeGppHttpClient>, Ptr<const Packet> packet)
{
    Ptr<Packet> p = packet->Copy();
    ThreeGppHttpHeader header;
    p->RemoveHeader(header);
    if (header.GetContentLength() == p->GetSize() &&
        header.GetContentType() == ThreeGppHttpHeader::EMBEDDED_OBJECT)
    {
        NS_LOG_INFO("Client has successfully received an embedded object of " << p->GetSize()
                                                                              << " bytes.");
    }
    else
    {
        NS_LOG_INFO("Client failed to parse an embedded object. ");
    }
}

void
ClientPageReceived(Ptr<const ThreeGppHttpClient> client,
                   const Time& time,
                   uint32_t numObjects,
                   uint32_t numBytes)
{
    NS_LOG_INFO("Client " << client << " has received a page that took " << time.As(Time::MS)
                          << " ms to load with " << numObjects << " objects and " << numBytes
                          << " bytes.");
}

int
main(int argc, char* argv[])
{
    double simTimeSec = 300;
    CommandLine cmd(__FILE__);
    cmd.AddValue("SimulationTime", "Length of simulation in seconds.", simTimeSec);
    cmd.Parse(argc, argv);

    Time::SetResolution(Time::NS);
    LogComponentEnableAll(LOG_PREFIX_TIME);
    // LogComponentEnableAll (LOG_PREFIX_FUNC);
    // LogComponentEnable ("ThreeGppHttpClient", LOG_INFO);
    /// LogComponentEnable ("ThreeGppHttpServer", LOG_INFO);
    LogComponentEnable("ThreeGppHttpExample", LOG_INFO);

    // Setup two nodes
    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer devices;
    devices = pointToPoint.Install(nodes);

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    Ipv4Address serverAddress = interfaces.GetAddress(1);

    // Create HTTP server helper
    ThreeGppHttpServerHelper serverHelper(serverAddress);

    // Install HTTP server
    ApplicationContainer serverApps = serverHelper.Install(nodes.Get(1));
    Ptr<ThreeGppHttpServer> httpServer = serverApps.Get(0)->GetObject<ThreeGppHttpServer>();

    // Example of connecting to the trace sources
    httpServer->TraceConnectWithoutContext("ConnectionEstablished",
                                           MakeCallback(&ServerConnectionEstablished));
    httpServer->TraceConnectWithoutContext("MainObject", MakeCallback(&MainObjectGenerated));
    httpServer->TraceConnectWithoutContext("EmbeddedObject",
                                           MakeCallback(&EmbeddedObjectGenerated));
    httpServer->TraceConnectWithoutContext("Tx", MakeCallback(&ServerTx));

    // Setup HTTP variables for the server
    PointerValue varPtr;
    httpServer->GetAttribute("Variables", varPtr);
    Ptr<ThreeGppHttpVariables> httpVariables = varPtr.Get<ThreeGppHttpVariables>();
    httpVariables->SetMainObjectSizeMean(102400);  // 100kB
    httpVariables->SetMainObjectSizeStdDev(40960); // 40kB

    // Create HTTP client helper
    ThreeGppHttpClientHelper clientHelper(serverAddress);

    // Install HTTP client
    ApplicationContainer clientApps = clientHelper.Install(nodes.Get(0));
    Ptr<ThreeGppHttpClient> httpClient = clientApps.Get(0)->GetObject<ThreeGppHttpClient>();

    // Example of connecting to the trace sources
    httpClient->TraceConnectWithoutContext("RxMainObject", MakeCallback(&ClientMainObjectReceived));
    httpClient->TraceConnectWithoutContext("RxEmbeddedObject",
                                           MakeCallback(&ClientEmbeddedObjectReceived));
    httpClient->TraceConnectWithoutContext("Rx", MakeCallback(&ClientRx));
    httpClient->TraceConnectWithoutContext("RxPage", MakeCallback(&ClientPageReceived));

    // Stop browsing after 30 minutes
    clientApps.Stop(Seconds(simTimeSec));

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
