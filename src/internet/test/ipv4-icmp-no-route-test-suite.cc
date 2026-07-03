/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

/*
 * Regression test for @issueid{825} (IP-layer "no route -> ICMP
 * unreachable"): a router that receives a packet it must forward but for
 * which it has no matching route must generate an ICMP Destination
 * Unreachable (type 3) message back toward the source, so that a Ping (or any
 * other) application on the source sees the error.
 *
 * What this test does:
 *   It builds a minimal, fully deterministic two-hop topology
 *
 *       A (source) ----- R (router, two interfaces)        D = 10.9.9.0/24
 *                                                          (no route on R)
 *
 *   A has a default route pointing at R. R is configured as an IP forwarder but
 *   deliberately has NO route to the destination subnet D (10.9.9.0/24). A
 *   raw ICMP socket (IP protocol number 1) is opened on A to capture any ICMP
 *   error returned by R. A then sends a single packet (via a raw socket) to an
 *   address inside D. R receives the packet, fails to route it, and -- on a
 *   correct IP implementation -- must answer with an ICMP Destination
 *   Unreachable (type 3, code 0 net-unreachable) sourced from R back to A.
 *
 * Expected behaviour:
 *   A receives exactly one ICMP Destination Unreachable message from R.
 */

#include "ns3/boolean.h"
#include "ns3/iana-internet-protocol-numbers.h"
#include "ns3/icmpv4.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv4-raw-socket-factory.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/ipv4.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simple-net-device.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/socket.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <cstdint>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("Ipv4IcmpNoRouteTestSuite");

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Regression test for @issueid{825}: a router that cannot forward a
 *        packet (no matching route) must reply to the source with an ICMP
 *        Destination Unreachable (net-unreachable) message.
 *
 * Topology:
 *
 *     A (10.0.0.1) ---- net0 ---- (10.0.0.2) R (10.0.1.1) ---- net1 ---- (dangling)
 *
 *     - A: default route via R (10.0.0.2).
 *     - R: IP forwarder, has interface routes for 10.0.0.0/24 and 10.0.1.0/24
 *          but NO route for the destination subnet 10.9.9.0/24.
 *     - A opens a raw ICMP socket (protocol 1) to capture returned ICMP errors
 *       and sends a single raw packet to 10.9.9.9 (inside the un-routable D).
 *
 * Pre-fix observation: A receives 0 ICMP errors (R drops silently), so the
 * final assertion fails. Post-fix, A receives one ICMP type-3 message and the
 * test passes.
 */
class Ipv4IcmpNoRouteTestCase : public TestCase
{
  public:
    Ipv4IcmpNoRouteTestCase();
    ~Ipv4IcmpNoRouteTestCase() override;

  private:
    void DoRun() override;

    /**
     * @brief Send a single raw IPv4 packet from A toward the un-routable
     *        destination.
     * @param socket The sending raw socket on node A.
     * @param dst The (un-routable) destination address inside subnet D.
     */
    void SendData(Ptr<Socket> socket, Ipv4Address dst);

    /**
     * @brief Receive callback for the raw ICMP socket on node A.
     *
     * Parses the received packet (full IPv4 datagram delivered by the raw
     * socket: Ipv4Header followed by Icmpv4Header) and, if it is an ICMP
     * Destination Unreachable, records it.
     *
     * @param socket The receiving raw ICMP socket on node A.
     */
    void ReceiveIcmp(Ptr<Socket> socket);

    uint32_t m_icmpUnreachCount{0}; //!< Number of ICMP Dest Unreachable msgs received by A
    Ipv4Address m_icmpSource;       //!< Source address of the received ICMP error
};

Ipv4IcmpNoRouteTestCase::Ipv4IcmpNoRouteTestCase()
    : TestCase("ICMP net-unreachable on no-route forwarding failure (issue #825)")
{
}

Ipv4IcmpNoRouteTestCase::~Ipv4IcmpNoRouteTestCase()
{
}

void
Ipv4IcmpNoRouteTestCase::SendData(Ptr<Socket> socket, Ipv4Address dst)
{
    // A plain payload; the IP layer adds the Ipv4Header with protocol 1 (ICMP)
    // because the raw socket was created with the ICMP protocol number.
    Ptr<Packet> p = Create<Packet>(8);
    Address realTo = InetSocketAddress(dst, 0);
    socket->SendTo(p, 0, realTo);
    NS_LOG_DEBUG("A sent " << p->GetSize() << " bytes to un-routable " << dst);
}

void
Ipv4IcmpNoRouteTestCase::ReceiveIcmp(Ptr<Socket> socket)
{
    Address from;
    Ptr<Packet> p = socket->RecvFrom(0xffffffff, 0, from);
    if (!p)
    {
        return;
    }

    // The raw socket delivers the full IPv4 datagram: Ipv4Header + ICMP.
    Ipv4Header ipv4;
    if (p->GetSize() < ipv4.GetSerializedSize())
    {
        return;
    }
    p->RemoveHeader(ipv4);

    if (ipv4.GetProtocol() != iana::internetprotocolnumbers::ICMP)
    {
        return;
    }

    Icmpv4Header icmp;
    p->RemoveHeader(icmp);

    if (icmp.GetType() == Icmpv4Header::ICMPV4_DEST_UNREACH)
    {
        m_icmpUnreachCount++;
        m_icmpSource = ipv4.GetSource();
        NS_LOG_DEBUG("A received ICMP Destination Unreachable from " << ipv4.GetSource());
    }
}

void
Ipv4IcmpNoRouteTestCase::DoRun()
{
    // Make the run fully deterministic.
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    // Node A (source) and node R (router with two interfaces).
    Ptr<Node> nodeA = CreateObject<Node>();
    Ptr<Node> nodeR = CreateObject<Node>();

    NodeContainer aAndR(nodeA, nodeR);

    // net0: A <-> R (subnet 10.0.0.0/24).
    SimpleNetDeviceHelper net0Helper;
    net0Helper.SetNetDevicePointToPointMode(true);
    NetDeviceContainer net0 = net0Helper.Install(aAndR);

    // net1: a second interface on R only (subnet 10.0.1.0/24). R has a dangling
    // device on this link so that it owns two interfaces, but has no route to
    // the destination subnet D = 10.9.9.0/24.
    SimpleNetDeviceHelper net1Helper;
    net1Helper.SetNetDevicePointToPointMode(true);
    NetDeviceContainer net1 = net1Helper.Install(NodeContainer(nodeR));

    InternetStackHelper internet;
    internet.Install(aAndR);

    Ptr<Ipv4> ipv4;
    uint32_t ifIndex;

    // --- Node A addressing: single interface 10.0.0.1/24 ---
    ipv4 = nodeA->GetObject<Ipv4>();
    ifIndex = ipv4->AddInterface(net0.Get(0));
    ipv4->AddAddress(ifIndex,
                     Ipv4InterfaceAddress(Ipv4Address("10.0.0.1"), Ipv4Mask("255.255.255.0")));
    ipv4->SetUp(ifIndex);

    // --- Node R addressing: 10.0.0.2/24 (toward A) and 10.0.1.1/24 (net1) ---
    ipv4 = nodeR->GetObject<Ipv4>();
    uint32_t rIfToA = ipv4->AddInterface(net0.Get(1));
    ipv4->AddAddress(rIfToA,
                     Ipv4InterfaceAddress(Ipv4Address("10.0.0.2"), Ipv4Mask("255.255.255.0")));
    ipv4->SetUp(rIfToA);

    uint32_t rIfNet1 = ipv4->AddInterface(net1.Get(0));
    ipv4->AddAddress(rIfNet1,
                     Ipv4InterfaceAddress(Ipv4Address("10.0.1.1"), Ipv4Mask("255.255.255.0")));
    ipv4->SetUp(rIfNet1);

    // R must act as an IP forwarder for RouteInput()/RouteInputError() to run.
    ipv4->SetAttribute("IpForward", BooleanValue(true));

    // --- Routing ---
    Ipv4StaticRoutingHelper staticRoutingHelper;

    // A: default route via R (10.0.0.2) out of A's only interface.
    Ptr<Ipv4StaticRouting> aRouting =
        staticRoutingHelper.GetStaticRouting(nodeA->GetObject<Ipv4>());
    aRouting->SetDefaultRoute(Ipv4Address("10.0.0.2"), 1);

    // R: intentionally NO route to the destination subnet D (10.9.9.0/24).
    // R only has its directly-connected interface routes (added automatically
    // when the interfaces are brought up). Forwarding a packet destined to D
    // therefore fails to route on R, exercising RouteInputError().

    // --- Raw ICMP socket on A to capture any returned ICMP error ---
    Ptr<SocketFactory> rawFactory = nodeA->GetObject<Ipv4RawSocketFactory>();
    Ptr<Socket> aIcmpSocket = rawFactory->CreateSocket();
    aIcmpSocket->SetAttribute("Protocol", UintegerValue(iana::internetprotocolnumbers::ICMP));
    NS_TEST_EXPECT_MSG_EQ(aIcmpSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 0)),
                          0,
                          "Failed to bind the raw ICMP socket on A");
    aIcmpSocket->SetRecvCallback(MakeCallback(&Ipv4IcmpNoRouteTestCase::ReceiveIcmp, this));

    // --- Sending socket on A: a raw ICMP socket to inject a forwarded packet
    //     toward the un-routable destination 10.9.9.9 (inside subnet D). ---
    Ptr<Socket> aSendSocket = rawFactory->CreateSocket();
    aSendSocket->SetAttribute("Protocol", UintegerValue(iana::internetprotocolnumbers::ICMP));
    NS_TEST_EXPECT_MSG_EQ(aSendSocket->Bind(InetSocketAddress(Ipv4Address("10.0.0.1"), 0)),
                          0,
                          "Failed to bind the raw sending socket on A");

    Simulator::ScheduleWithContext(nodeA->GetId(),
                                   Seconds(1.0),
                                   &Ipv4IcmpNoRouteTestCase::SendData,
                                   this,
                                   aSendSocket,
                                   Ipv4Address("10.9.9.9"));

    Simulator::Stop(Seconds(5.0));
    Simulator::Run();
    Simulator::Destroy();

    // A receives exactly one ICMP Destination Unreachable message, sourced
    // from the router R (10.0.0.2).
    NS_TEST_EXPECT_MSG_EQ(m_icmpUnreachCount,
                          1,
                          "Source A should receive exactly one ICMP Destination Unreachable "
                          "(net-unreachable) from the router on a no-route forwarding failure "
                          "(issue #825)");

    NS_TEST_EXPECT_MSG_EQ(m_icmpSource,
                          Ipv4Address("10.0.0.2"),
                          "The ICMP Destination Unreachable should be sourced from the router R "
                          "(10.0.0.2)");
}

/**
 * @ingroup internet-test
 * @ingroup tests
 *
 * @brief Test suite for the ICMP no-route regression (@issueid{825}).
 */
class Ipv4IcmpNoRouteTestSuite : public TestSuite
{
  public:
    Ipv4IcmpNoRouteTestSuite()
        : TestSuite("ipv4-icmp-no-route", Type::UNIT)
    {
        AddTestCase(new Ipv4IcmpNoRouteTestCase, TestCase::Duration::QUICK);
    }
};

static Ipv4IcmpNoRouteTestSuite g_ipv4IcmpNoRouteTestSuite; //!< Static variable for test
                                                            //!< initialization
