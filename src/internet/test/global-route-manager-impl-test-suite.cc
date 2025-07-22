/*
 * Copyright 2007 University of Washington
 * Copyright (C) 1999, 2000 Kunihiro Ishiguro, Toshiaki Takada
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Tom Henderson (tomhend@u.washington.edu)
 *
 * Modified By: Shashwat Patni (shashwatpatni25@gmail.com)
 *
 * Kunihiro Ishigura, Toshiaki Takada (GNU Zebra) are attributed authors
 * of the quagga 0.99.7/src/ospfd/ospf_spf.c code which was ported here
 */

#include "ns3/candidate-queue.h"
#include "ns3/config.h"
#include "ns3/global-route-manager-impl.h"
#include "ns3/global-routing.h"
#include "ns3/internet-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv6-global-routing-helper.h"
#include "ns3/network-module.h"
#include "ns3/node-container.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simple-net-device-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

#include <cstdlib> // for rand()
using namespace ns3;

NS_LOG_COMPONENT_DEFINE("GlobalRouteManagerImplTestSuite");

//
// This test suite is designed to check the Working of the GlobalRouteManagerImpl
// For that reason, the tests in this suite manually build LSAs for each topology
// we manually call DebugSPFCalculate() to fill routing tables of the node we are interested in.
//
//  TestCase 1: LinkRoutesTestCase
//  This test case tests that:
//  - GLobalRouteManagerLSDB stores the LSAs with key as the correct link state ID
//  - GlobalRouteManagerImpl checks for stubnodes and computes default routes for them
//  - HostRoutes are computed correctly for point-to-point links
//
//  TestCase 2: LanRoutesTestCase
//  This test case tests that:
//  - Network LSAs are handled by the GlobalRouteManagerImpl
//  - GlobalRouteManagerImpl computes the routes correctly for a LAN topology
//
//  TestCase 3: RandomEcmpRoutesTestCase
//  This test case tests that:
//  - GlobalRouteManagerImpl computes ECMP routes correctly.
//  - Those random routes are in fact used by the GlobalRouting protocol
//

/**
 * @ingroup internet
 * @ingroup tests
 * @defgroup internet-test internet module tests
 */

/**
 * @ingroup internet-test
 *
 * @brief Global Route Manager Test
 */
class LinkRoutesTestCase : public TestCase
{
  public:
    LinkRoutesTestCase();
    void DoSetup() override;
    void DoRun() override;

  private:
    /**
     *@brief Builds the LSAs for the topology. These LSAs are manually created and inserted into the
     * GlobalRouteManagerLSDB.Each node exports a router LSA.
     */
    void BuildLsav4();

    /**
     *@brief Builds the LSAs for the topology. These LSAs are manually created and inserted into the
     * GlobalRouteManagerLSDB.Each node exports a router LSA.
     */
    void BuildLsav6();

    /**
     *  @brief Checks the Routing Table Entries for the expected output.
     * @param globalroutingprotocol The routing protocol for the node whose routing table is to be
     * checked.
     * @param dests The expected destinations.
     * @param gws The expected gateways.
     */
    void CheckRoutesv4(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
                       std::vector<Ipv4Address>& dests,
                       std::vector<Ipv4Address>& gws);

    /**
     *  @brief Checks the Routing Table Entries for the expected output.
     * @param globalroutingprotocol The routing protocol for the node whose routing table is to be
     * checked.
     * @param dests The expected destinations.
     * @param gws The expected gateways.
     */
    void CheckRoutesv6(Ptr<Ipv6GlobalRouting>& globalroutingprotocol,
                       std::vector<Ipv6Address>& dests,
                       std::vector<Ipv6Address>& gws);

    NodeContainer nodes; //!< NodeContainer to hold the nodes in the topology
    std::vector<GlobalRoutingLSA<Ipv4Manager>*> m_lsasv4; //!< The LSAs for the topology
    std::vector<GlobalRoutingLSA<Ipv6Manager>*> m_lsasv6; //!< The LSAs for the topology
};

LinkRoutesTestCase::LinkRoutesTestCase()
    : TestCase("LinkRoutesTestCase")
{
}

void
LinkRoutesTestCase::CheckRoutesv4(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
                                  std::vector<Ipv4Address>& dests,
                                  std::vector<Ipv4Address>& gws)
{
    // check each individual Routing Table Entry for its destination and gateway
    for (uint32_t i = 0; i < globalroutingprotocol->GetNRoutes(); i++)
    {
        Ipv4RoutingTableEntry* route = globalroutingprotocol->GetRoute(i);
        NS_LOG_DEBUG("dest " << route->GetDest() << " gw " << route->GetGateway());
        NS_TEST_ASSERT_MSG_EQ(route->GetDest(), dests[i], "Error-- wrong destination");
        NS_TEST_ASSERT_MSG_EQ(route->GetGateway(), gws[i], "Error-- wrong gateway");
    }
}

void
LinkRoutesTestCase::CheckRoutesv6(Ptr<Ipv6GlobalRouting>& globalroutingprotocol,
                                  std::vector<Ipv6Address>& dests,
                                  std::vector<Ipv6Address>& gws)
{
    // check each individual Routing Table Entry for its destination and gateway
    for (uint32_t i = 0; i < globalroutingprotocol->GetNRoutes(); i++)
    {
        Ipv6RoutingTableEntry* route = globalroutingprotocol->GetRoute(i);
        NS_LOG_DEBUG("dest " << route->GetDest() << " gw " << route->GetGateway());
        NS_TEST_ASSERT_MSG_EQ(route->GetDest(), dests[i], "Error-- wrong destination");
        NS_TEST_ASSERT_MSG_EQ(route->GetGateway(), gws[i], "Error-- wrong gateway");
    }
}

void
LinkRoutesTestCase::DoSetup()
{
    // Simple p2p links. n0,n1 and n3 are stub nodes
    //
    //
    //   n0
    //      \ link 0
    //       \          link 2
    //        n2 -------------------------n3
    //       /
    //      / link 1
    //    n1
    //
    //  link0:  n0->10.1.1.1/30, n2-> 10.1.1.2/30
    //  link1:  n1-> 10.1.2.1/30, n2->10.1.2.2/30
    //  link2:  n2->10.1.3.1/30, n3-> 10.1.3.2/30
    //
    //
    //

    nodes.Create(4);

    Ipv4GlobalRoutingHelper globalhelperv4;
    Ipv6GlobalRoutingHelper globalhelperv6;
    InternetStackHelper stack;
    stack.SetRoutingHelper(globalhelperv4);
    stack.SetRoutingHelper(globalhelperv6);
    stack.Install(nodes);

    SimpleNetDeviceHelper devHelper;
    devHelper.SetNetDevicePointToPointMode(true);
    Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel>();
    NetDeviceContainer d02 = devHelper.Install(nodes.Get(0), channel1);
    d02.Add(devHelper.Install(nodes.Get(2), channel1));
    Ptr<SimpleChannel> channel2 = CreateObject<SimpleChannel>();
    NetDeviceContainer d12 = devHelper.Install(nodes.Get(1), channel2);
    d12.Add(devHelper.Install(nodes.Get(2), channel2));
    Ptr<SimpleChannel> channel3 = CreateObject<SimpleChannel>();
    NetDeviceContainer d23 = devHelper.Install(nodes.Get(2), channel3);
    d23.Add(devHelper.Install(nodes.Get(3), channel3));

    // Assign IP addresses to the devices
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer i02 = address.Assign(d02);
    address.SetBase("10.1.2.0", "255.255.255.252");
    Ipv4InterfaceContainer i12 = address.Assign(d12);
    address.SetBase("10.1.3.0", "255.255.255.252");
    Ipv4InterfaceContainer i23 = address.Assign(d23);

    Ipv6AddressHelper addressv6;
    addressv6.SetBase("2001:1::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i02v6 = addressv6.Assign(d02);
    addressv6.SetBase("2001:2::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i12v6 = addressv6.Assign(d12);
    addressv6.SetBase("2001:3::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i23v6 = addressv6.Assign(d23);
}

void
LinkRoutesTestCase::BuildLsav4()
{
    // Manually build the link state database; four routers (0-3), 3 point-to-point
    // links

    // Router 0
    auto lr0 = new GlobalRoutingLinkRecord<Ipv4Manager>(
        GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
        "0.0.0.2",  // link id -> router ID 0.0.0.2
        "10.1.1.1", // link data -> Local IP address of router 0
        1);         // metric

    auto lr1 = new GlobalRoutingLinkRecord<Ipv4Manager>(
        GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
        "10.1.1.2", // link id ->adjacent neighbor's IP address
        "255.255.255.252",
        1);

    auto lsa0 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa0->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa0->SetLinkStateId("0.0.0.0");
    lsa0->SetAdvertisingRouter("0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    lsa0->AddLinkRecord(lr0);
    lsa0->AddLinkRecord(lr1);
    m_lsasv4.push_back(lsa0);

    // Router 1
    auto lr2 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.2",
                                                 "10.1.2.1",
                                                 1);

    auto lr3 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.2.2",
                                                 "255.255.255.252",
                                                 1);

    auto lsa1 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa1->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa1->SetLinkStateId("0.0.0.1");
    lsa1->SetAdvertisingRouter("0.0.0.1");
    lsa1->AddLinkRecord(lr2);
    lsa1->AddLinkRecord(lr3);
    lsa1->SetNode(nodes.Get(1));
    m_lsasv4.push_back(lsa1);

    // Router 2
    auto lr4 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.0",
                                                 "10.1.1.2",
                                                 1);

    auto lr5 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.1.1",
                                                 "255.255.255.252",
                                                 1);

    auto lr6 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.1",
                                                 "10.1.2.2",
                                                 1);

    auto lr7 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.2.2",
                                                 "255.255.255.252",
                                                 1);

    auto lr8 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.3",
                                                 "10.1.3.1",
                                                 1);

    auto lr9 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.3.2",
                                                 "255.255.255.252",
                                                 1);

    auto lsa2 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa2->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa2->SetLinkStateId("0.0.0.2");
    lsa2->SetAdvertisingRouter("0.0.0.2");
    lsa2->AddLinkRecord(lr4);
    lsa2->AddLinkRecord(lr5);
    lsa2->AddLinkRecord(lr6);
    lsa2->AddLinkRecord(lr7);
    lsa2->AddLinkRecord(lr8);
    lsa2->AddLinkRecord(lr9);
    lsa2->SetNode(nodes.Get(2));
    m_lsasv4.push_back(lsa2);

    // Router 3
    auto lr10 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.2",
                                                 "10.1.3.2",
                                                 1);

    auto lr11 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.3.1",
                                                 "255.255.255.252",
                                                 1);

    auto lsa3 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa3->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa3->SetLinkStateId("0.0.0.3");
    lsa3->SetAdvertisingRouter("0.0.0.3");
    lsa3->AddLinkRecord(lr10);
    lsa3->AddLinkRecord(lr11);
    lsa3->SetNode(nodes.Get(3));
    m_lsasv4.push_back(lsa3);
}

void
LinkRoutesTestCase::BuildLsav6()
{
    // Manually build the link state database; four routers (0-3), 3 point-to-point
    // links

    // Router 0
    auto lr0 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
        "::ffff:0.0.0.2",        // link id -> router ID 0.0.0.2
        "2001:1::200:ff:fe00:1", // link data -> Local GlobalUnicast IP address of router 0
        "fe80::200:ff:fe00:1",   // Local Link Local IP address of router 0
        1);                      // metric

    // only need to do this once for prefix of 64 bits
    uint8_t buf[16];
    auto prefix = Ipv6Prefix(64);
    prefix.GetBytes(buf); // frown

    auto lr1 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
        "2001:1::200:ff:fe00:2", // link id ->adjacent neighbor's IP address
        Ipv6Address(buf), // Link Data -> Prefix converted to Ipv6Address associated with neighbours
                          // Ip address
        1);

    auto lsa0 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa0->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa0->SetLinkStateId("::ffff:0.0.0.0");
    lsa0->SetAdvertisingRouter("::ffff:0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    lsa0->AddLinkRecord(lr0);
    lsa0->AddLinkRecord(lr1);
    m_lsasv6.push_back(lsa0);

    // Router 1
    auto lr2 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
        "::ffff:0.0.0.2",        // link id -> router ID
        "2001:2::200:ff:fe00:3", // link data -> Local Global Unicast IP address of router 1
        "fe80::200:ff:fe00:3",   // link loc data -> Local Link Local IP address of router 1
        1);

    auto lr3 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
        "2001:2::200:ff:fe00:4", // link id ->adjacent neighbor's IP address
        Ipv6Address(buf), // Link Data -> Prefix converted to Ipv6Address associated with neighbours
                          // Ip address
        1);

    auto lsa1 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa1->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa1->SetLinkStateId("::ffff:0.0.0.1");
    lsa1->SetAdvertisingRouter("::ffff:0.0.0.1");
    lsa1->AddLinkRecord(lr2);
    lsa1->AddLinkRecord(lr3);
    lsa1->SetNode(nodes.Get(1));
    m_lsasv6.push_back(lsa1);

    // Router 2
    auto lr4 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
        "::ffff:0.0.0.0",        // link id -> router ID
        "2001:1::200:ff:fe00:2", // link data -> Local Global Unicast IP address of router 2
        "fe80::200:ff:fe00:2",   // link loc data -> Local Link Local IP address of router 2
        1);

    auto lr5 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
        "2001:1::200:ff:fe00:1", // link id ->adjacent neighbor's IP address
        Ipv6Address(buf), // Link Data -> Prefix converted to Ipv6Address associated with neighbours
                          // Ip address
        1);

    auto lr6 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.1",
                                                 "2001:2::200:ff:fe00:4",
                                                 "fe80::200:ff:fe00:4",
                                                 1);

    auto lr7 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:2::200:ff:fe00:3",
                                                 Ipv6Address(buf),
                                                 1);

    auto lr8 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.3",
                                                 "2001:3::200:ff:fe00:5",
                                                 "fe80::200:ff:fe00:5",
                                                 1);

    auto lr9 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:3::200:ff:fe00:6",
                                                 Ipv6Address(buf),
                                                 1);

    auto lsa2 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa2->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa2->SetLinkStateId("::ffff:0.0.0.2");
    lsa2->SetAdvertisingRouter("::ffff:0.0.0.2");
    lsa2->AddLinkRecord(lr4);
    lsa2->AddLinkRecord(lr5);
    lsa2->AddLinkRecord(lr6);
    lsa2->AddLinkRecord(lr7);
    lsa2->AddLinkRecord(lr8);
    lsa2->AddLinkRecord(lr9);
    lsa2->SetNode(nodes.Get(2));
    m_lsasv6.push_back(lsa2);

    // Router 3
    auto lr10 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.2",
                                                 "2001:3::200:ff:fe00:6",
                                                 "fe80::200:ff:fe00:6",
                                                 1);

    auto lr11 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:3::200:ff:fe00:5",
                                                 Ipv6Address(buf),
                                                 1);

    auto lsa3 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa3->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa3->SetLinkStateId("::ffff:0.0.0.3");
    lsa3->SetAdvertisingRouter("::ffff:0.0.0.3");
    lsa3->AddLinkRecord(lr10);
    lsa3->AddLinkRecord(lr11);
    lsa3->SetNode(nodes.Get(3));
    m_lsasv6.push_back(lsa3);
}

void
LinkRoutesTestCase::DoRun()
{
    //  This test is for checking the individual working of GlobalRouteManagerImpl
    //  and GlobalRouteManagerLSDB, so we will not use the GlobalRoutingHelper
    //  to create the routing tables, but instead we will manually create the LSAs
    //  and insert them into the GlobalRouteManagerLSDB, and then use the
    //  GlobalRouteManagerImpl to calculate the routes based on the LSDB.
    //  This is a manual setup of the LSAs, which would normally be done by the
    //  GlobalRoutingHelper.

    BuildLsav4();

    // Test the database
    auto srmlsdb = new GlobalRouteManagerLSDB<Ipv4Manager>();
    srmlsdb->Insert(m_lsasv4[0]->GetLinkStateId(), m_lsasv4[0]);
    srmlsdb->Insert(m_lsasv4[1]->GetLinkStateId(), m_lsasv4[1]);
    srmlsdb->Insert(m_lsasv4[2]->GetLinkStateId(), m_lsasv4[2]);
    srmlsdb->Insert(m_lsasv4[3]->GetLinkStateId(), m_lsasv4[3]);

    NS_TEST_ASSERT_MSG_EQ(
        m_lsasv4[2],
        srmlsdb->GetLSA(m_lsasv4[2]->GetLinkStateId()),
        "The Ipv4Address is not stored as the link state ID"); // LSAs are mapped by router id as
                                                               // key here we check that they are
                                                               // indeed getting the right lsa for
                                                               // the right key

    // next, calculate routes based on the manually created LSDB
    auto srm = new GlobalRouteManagerImpl<Ipv4Manager>();
    srm->DebugUseLsdb(srmlsdb); // manually add in an LSDB

    srm->DebugSPFCalculate(m_lsasv4[0]->GetLinkStateId()); // fill routing table for node n0

    srm->DebugSPFCalculate(m_lsasv4[1]->GetLinkStateId()); // fill routing table for node n1

    srm->DebugSPFCalculate(m_lsasv4[2]->GetLinkStateId()); // fill routing table for node n2

    srm->DebugSPFCalculate(m_lsasv4[3]->GetLinkStateId()); // fill routing table for node n3

    BuildLsav6();

    // Test the database
    auto srmlsdbv6 = new GlobalRouteManagerLSDB<Ipv6Manager>();
    srmlsdbv6->Insert(m_lsasv6[0]->GetLinkStateId(), m_lsasv6[0]);
    srmlsdbv6->Insert(m_lsasv6[1]->GetLinkStateId(), m_lsasv6[1]);
    srmlsdbv6->Insert(m_lsasv6[2]->GetLinkStateId(), m_lsasv6[2]);
    srmlsdbv6->Insert(m_lsasv6[3]->GetLinkStateId(), m_lsasv6[3]);

    NS_TEST_ASSERT_MSG_EQ(
        m_lsasv6[2],
        srmlsdbv6->GetLSA(m_lsasv6[2]->GetLinkStateId()),
        "The Ipv6Address is not stored as the link state ID"); // LSAs are mapped by router id as
                                                               // key here we check that they are
                                                               // indeed getting the right lsa for
                                                               // the right key

    // next, calculate routes based on the manually created LSDB
    auto srmv6 = new GlobalRouteManagerImpl<Ipv6Manager>();
    srmv6->DebugUseLsdb(srmlsdbv6); // manually add in an LSDB

    srmv6->DebugSPFCalculate(m_lsasv6[0]->GetLinkStateId()); // fill ipv6 routing table for node n0

    srmv6->DebugSPFCalculate(m_lsasv6[1]->GetLinkStateId()); // fill ipv6 routing table for node n1

    srmv6->DebugSPFCalculate(m_lsasv6[2]->GetLinkStateId()); // fill ipv6 routing table for node n2

    srmv6->DebugSPFCalculate(m_lsasv6[3]->GetLinkStateId()); // fill ipv6 routing table for node n3

    //-----------------Now the tests for Ipv4------------------
    // Test 1: Check if the SPF calculate Sets default routes for Stub nodes
    Ptr<Ipv4L3Protocol> ip0 = nodes.Get(0)->GetObject<Ipv4L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip0, nullptr, "Error-- no Ipv4 object at node 0");
    Ptr<Ipv4RoutingProtocol> routing0 = ip0->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing0, nullptr, "Error-- no Ipv4 routing protocol object at node 0");
    Ptr<Ipv4GlobalRouting> globalRouting0 = routing0->GetObject<Ipv4GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting0, nullptr, "Error-- no Ipv4GlobalRouting object at node 0");

    // Check that the right number of entries are in the routing table
    uint32_t nRoutes0 = globalRouting0->GetNRoutes();
    NS_TEST_ASSERT_MSG_EQ(nRoutes0, 1, "Error-- default route not found for stub node");

    Ipv4RoutingTableEntry* route = nullptr;
    route = globalRouting0->GetRoute(0);
    // the only route is the default route on this node
    NS_TEST_ASSERT_MSG_EQ(route->GetDest(),
                          Ipv4Address("0.0.0.0"),
                          "Error-- wrong destination for default route");
    NS_TEST_ASSERT_MSG_EQ(route->GetGateway(), Ipv4Address("10.1.1.2"), "Error-- wrong gateway");

    // Test 2: Check if SPFCalculate sets the correct routes for node 2
    Ptr<Ipv4L3Protocol> ip2 = nodes.Get(2)->GetObject<Ipv4L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip2, nullptr, "Error-- no Ipv4 object at node 2");
    Ptr<Ipv4RoutingProtocol> routing2 = ip2->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing2, nullptr, "Error-- no Ipv4 routing protocol object at node 2");
    Ptr<Ipv4GlobalRouting> globalRouting2 = routing2->GetObject<Ipv4GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting2, nullptr, "Error-- no Ipv4GlobalRouting object at node 2");

    // check that the correct number of routes were built
    uint32_t nRoutes2 = globalRouting2->GetNRoutes();
    NS_LOG_DEBUG("LinkRoutesTest nRoutes2 " << nRoutes2);
    NS_TEST_ASSERT_MSG_EQ(nRoutes2, 6, "Error--- Incorrect number of routes found on node 2");

    // check that all the routes in the routing table are correct for node 2
    std::vector<Ipv4Address> expecteddests;
    std::vector<Ipv4Address> expectedgws;
    expecteddests.emplace_back("10.1.1.1");
    expecteddests.emplace_back("10.1.2.1");
    expecteddests.emplace_back("10.1.3.2");
    expecteddests.emplace_back("10.1.1.0");
    expecteddests.emplace_back("10.1.2.0");
    expecteddests.emplace_back("10.1.3.0");

    expectedgws.emplace_back("10.1.1.1");
    expectedgws.emplace_back("10.1.2.1");
    expectedgws.emplace_back("10.1.3.2");
    expectedgws.emplace_back("10.1.1.1");
    expectedgws.emplace_back("10.1.2.1");
    expectedgws.emplace_back("10.1.3.2");

    CheckRoutesv4(globalRouting2, expecteddests, expectedgws);

    //-----------------Now the tests for Ipv6------------------
    // Test 1: Check if the SPF calculate Sets default routes for Stub nodes
    Ptr<Ipv6L3Protocol> ip0v6 = nodes.Get(0)->GetObject<Ipv6L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip0v6, nullptr, "Error-- no Ipv6 object at node 0");
    Ptr<Ipv6RoutingProtocol> routing0v6 = ip0v6->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing0v6, nullptr, "Error-- no Ipv6 routing protocol object at node 0");
    Ptr<Ipv6GlobalRouting> globalRouting0v6 = routing0v6->GetObject<Ipv6GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting0v6,
                          nullptr,
                          "Error-- no Ipv6GlobalRouting object at node 0");

    // Check that the right number of entries are in the routing table
    uint32_t nRoutes0v6 = globalRouting0v6->GetNRoutes();
    NS_TEST_ASSERT_MSG_EQ(nRoutes0v6, 1, "Error-- default route not found for stub node");

    Ipv6RoutingTableEntry* routev6 = nullptr;
    routev6 = globalRouting0v6->GetRoute(0);
    // the only route is the default route on this node
    NS_TEST_ASSERT_MSG_EQ(routev6->GetDest(),
                          Ipv6Address("::"),
                          "Error-- wrong destination for default route");
    NS_TEST_ASSERT_MSG_EQ(routev6->GetGateway(),
                          Ipv6Address("fe80::200:ff:fe00:2"),
                          "Error-- wrong gateway");

    // Test 2: Check if SPFCalculate sets the correct routes for node 2
    Ptr<Ipv6L3Protocol> ip2v6 = nodes.Get(2)->GetObject<Ipv6L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip2v6, nullptr, "Error-- no Ipv6 object at node 2");
    Ptr<Ipv6RoutingProtocol> routing2v6 = ip2v6->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing2v6, nullptr, "Error-- no Ipv6 routing protocol object at node 2");
    Ptr<Ipv6GlobalRouting> globalRouting2v6 = routing2v6->GetObject<Ipv6GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting2v6,
                          nullptr,
                          "Error-- no Ipv6GlobalRouting object at node 2");

    // check that the correct number of routes were built
    uint32_t nRoutes2v6 = globalRouting2v6->GetNRoutes();
    NS_LOG_DEBUG("LinkRoutesTest nRoutes2 " << nRoutes2v6);
    NS_TEST_ASSERT_MSG_EQ(nRoutes2v6, 6, "Error--- Incorrect number of routes found on node 2");

    // check that all the routes in the routing table are correct for node 2
    std::vector<Ipv6Address> expecteddestsv6;
    std::vector<Ipv6Address> expectedgwsv6;
    expecteddestsv6.emplace_back("2001:1::200:ff:fe00:1");
    expecteddestsv6.emplace_back("2001:2::200:ff:fe00:3");
    expecteddestsv6.emplace_back("2001:3::200:ff:fe00:6");
    expecteddestsv6.emplace_back("2001:1::");
    expecteddestsv6.emplace_back("2001:2::");
    expecteddestsv6.emplace_back("2001:3::");

    expectedgwsv6.emplace_back("fe80::200:ff:fe00:1");
    expectedgwsv6.emplace_back("fe80::200:ff:fe00:3");
    expectedgwsv6.emplace_back("fe80::200:ff:fe00:6");
    expectedgwsv6.emplace_back("fe80::200:ff:fe00:1");
    expectedgwsv6.emplace_back("fe80::200:ff:fe00:3");
    expectedgwsv6.emplace_back("fe80::200:ff:fe00:6");

    CheckRoutesv6(globalRouting2v6, expecteddestsv6, expectedgwsv6);

    Simulator::Run();

    Simulator::Stop(Seconds(3));
    Simulator::Destroy();

    // This delete clears the srm, which deletes the LSDB, which clears
    // all of the LSAs, which each destroys the attached LinkRecords.
    delete srm;
    delete srmv6;
    // reset the router ID counter to zero so that it does not affect other tests
    //  that may run after this one in the same program run.
    GlobalRouteManager<Ipv4Manager>::ResetRouterId();
    GlobalRouteManager<Ipv6Manager>::ResetRouterId();
}

/**
 * @ingroup internet-test
 *
 * @brief This test case is to check if NetworkRoutes are being built correctly, i.e if route
 * computation works for a LAN Topology.
 */
class LanRoutesTestCase : public TestCase
{
  public:
    LanRoutesTestCase();
    void DoSetup() override;
    void DoRun() override;

  private:
    /**
     *@brief Builds the ipv4 LSAs for the topology. These LSAs are manually created and inserted
     *into the GlobalRouteManagerLSDB.Each node exports a router LSA. In addition,the designated
     *router also Exports the Network LSA.
     */
    void BuildLsav4();

    /**
     *@brief Builds the ipv6 LSAs for the topology. These LSAs are manually created and inserted
     *into the GlobalRouteManagerLSDB.Each node exports a router LSA. In addition,the designated
     *router also Exports the Network LSA.
     */
    void BuildLsav6();

    NodeContainer nodes; //!< NodeContainer to hold the nodes in the topology
    std::vector<GlobalRoutingLSA<Ipv4Manager>*> m_lsasv4; //!< The ipv4 LSAs for the topology
    std::vector<GlobalRoutingLSA<Ipv6Manager>*> m_lsasv6; //!< The ipv6 LSAs for the topology
};

LanRoutesTestCase::LanRoutesTestCase()
    : TestCase("LanRoutesTestCase")
{
}

void
LanRoutesTestCase::DoSetup()
{
    // Simple Csma Network with three nodes
    //
    //      n0                       n1
    //      |    (shared csma/cd)   |
    //      -------------------------
    //                |
    //                n2
    //
    //       n0:10.1.1.1/29
    //       n1:10.1.1.2/29
    //       n2:10.1.1.3/29
    nodes.Create(3);
    Ipv4GlobalRoutingHelper globalhelperv4;
    Ipv6GlobalRoutingHelper globalhelperv6;
    InternetStackHelper stack;
    stack.SetRoutingHelper(globalhelperv4);
    stack.SetRoutingHelper(globalhelperv6);
    stack.Install(nodes);
    SimpleNetDeviceHelper devHelper;
    Ptr<SimpleChannel> channel = CreateObject<SimpleChannel>();
    NetDeviceContainer d012 = devHelper.Install(nodes.Get(0), channel);
    d012.Add(devHelper.Install(nodes.Get(1), channel));
    d012.Add(devHelper.Install(nodes.Get(2), channel));

    // assign IP addresses to the devices
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.248");
    Ipv4InterfaceContainer i012 = address.Assign(d012);

    Ipv6AddressHelper addressv6;
    addressv6.SetBase("2001:1::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i012v6 = addressv6.Assign(d012);
}

void
LanRoutesTestCase::BuildLsav4()
{
    // we manually create the link state database, we could have used BuildRoutingTables() but this
    // way testing the GlobalRouteManagerImpl without the involvement of GLobalRouter makes it
    // easier to debug each individually

    // router 0
    auto lr0 = new GlobalRoutingLinkRecord<Ipv4Manager>(
        GlobalRoutingLinkRecord<Ipv4Manager>::TransitNetwork,
        "10.1.1.1",
        "10.1.1.1",
        1);
    auto lsa0 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa0->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa0->AddLinkRecord(lr0);
    lsa0->SetLinkStateId("0.0.0.0");
    lsa0->SetAdvertisingRouter("0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    m_lsasv4.push_back(lsa0);

    // router 1
    auto lr1 = new GlobalRoutingLinkRecord<Ipv4Manager>(
        GlobalRoutingLinkRecord<Ipv4Manager>::TransitNetwork,
        "10.1.1.1",
        "10.1.1.2",
        1);
    auto lsa1 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa1->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa1->AddLinkRecord(lr1);
    lsa1->SetLinkStateId("0.0.0.1");
    lsa1->SetAdvertisingRouter("0.0.0.1");
    lsa1->SetNode(nodes.Get(1));
    m_lsasv4.push_back(lsa1);

    // router 2
    auto lr2 = new GlobalRoutingLinkRecord<Ipv4Manager>(
        GlobalRoutingLinkRecord<Ipv4Manager>::TransitNetwork,
        "10.1.1.1",
        "10.1.1.3",
        1);
    auto lsa2 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa2->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa2->AddLinkRecord(lr2);
    lsa2->SetLinkStateId("0.0.0.2");
    lsa2->SetAdvertisingRouter("0.0.0.2");
    lsa2->SetNode(nodes.Get(2));
    m_lsasv4.push_back(lsa2);

    // router0 is the designated router for the LAN. it also exports the network LSA
    auto lsa0network = new GlobalRoutingLSA<Ipv4Manager>();
    lsa0network->SetLSType(GlobalRoutingLSA<Ipv4Manager>::NetworkLSA);
    lsa0network->SetLinkStateId("10.1.1.1");
    lsa0network->SetAdvertisingRouter("0.0.0.0");
    lsa0network->AddAttachedRouter("10.1.1.1");
    lsa0network->AddAttachedRouter("10.1.1.2");
    lsa0network->AddAttachedRouter("10.1.1.3");
    lsa0network->SetNetworkLSANetworkMask("255.255.255.248");
    m_lsasv4.push_back(lsa0network); // note the index of the network lsa
}

void
LanRoutesTestCase::BuildLsav6()
{
    // we manually create the link state database, we could have used BuildRoutingTables() but this
    // way testing the GlobalRouteManagerImpl without the involvement of GLobalRouter makes it
    // easier to debug each individually

    // router 0
    auto lr0 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::TransitNetwork,
        "2001:1::200:ff:fe00:1", // link id ->  Link ID is the ip address of the designated router
        "2001:1::200:ff:fe00:1", // link data is this routers own Global Unicast ip address
        "fe80::200:ff:fe00:1",   // the link local data is the link local address of this router for
                                 // this link
        1);
    auto lsa0 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa0->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa0->AddLinkRecord(lr0);
    lsa0->SetLinkStateId("::ffff:0.0.0.0");
    lsa0->SetAdvertisingRouter("::ffff:0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    m_lsasv6.push_back(lsa0);

    // router 1
    auto lr1 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::TransitNetwork,
        "2001:1::200:ff:fe00:1", // link id ->  Link ID is the ip address of the designated router
        "2001:1::200:ff:fe00:2", // link data is this routers own Global Unicast ip address
        "fe80::200:ff:fe00:2",   // the link local data is the link local address of this router for
                                 // this link
        1);
    auto lsa1 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa1->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa1->AddLinkRecord(lr1);
    lsa1->SetLinkStateId("::ffff:0.0.0.1");
    lsa1->SetAdvertisingRouter("::ffff:0.0.0.1");
    lsa1->SetNode(nodes.Get(1));
    m_lsasv6.push_back(lsa1);

    // router 2
    auto lr2 = new GlobalRoutingLinkRecord<Ipv6Manager>(
        GlobalRoutingLinkRecord<Ipv6Manager>::TransitNetwork,
        "2001:1::200:ff:fe00:1",
        "2001:1::200:ff:fe00:3",
        "fe80::200:ff:fe00:3",
        1);
    auto lsa2 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa2->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa2->AddLinkRecord(lr2);
    lsa2->SetLinkStateId("::ffff:0.0.0.2");
    lsa2->SetAdvertisingRouter("::ffff:0.0.0.2");
    lsa2->SetNode(nodes.Get(2));
    m_lsasv6.push_back(lsa2);

    // router0 is the designated router for the LAN. it also exports the network LSA
    auto lsa0network = new GlobalRoutingLSA<Ipv6Manager>();
    lsa0network->SetLSType(GlobalRoutingLSA<Ipv6Manager>::NetworkLSA);
    lsa0network->SetLinkStateId("2001:1::200:ff:fe00:1");
    lsa0network->SetAdvertisingRouter("::ffff:0.0.0.0");
    lsa0network->AddAttachedRouter("2001:1::200:ff:fe00:1");
    lsa0network->AddAttachedRouter("2001:1::200:ff:fe00:2");
    lsa0network->AddAttachedRouter("2001:1::200:ff:fe00:3");

    lsa0network->SetNetworkLSANetworkMask(Ipv6Prefix(64));
    m_lsasv6.push_back(lsa0network); // note the index of the network lsa
}

void
LanRoutesTestCase::DoRun()
{
    BuildLsav4();
    // insert the LSAs into the GlobalRouteManagerLSDB
    auto srmlsdb = new GlobalRouteManagerLSDB<Ipv4Manager>();

    srmlsdb->Insert(m_lsasv4[0]->GetLinkStateId(), m_lsasv4[0]);
    srmlsdb->Insert(m_lsasv4[1]->GetLinkStateId(), m_lsasv4[1]);
    srmlsdb->Insert(m_lsasv4[2]->GetLinkStateId(), m_lsasv4[2]);
    srmlsdb->Insert(m_lsasv4[3]->GetLinkStateId(), m_lsasv4[3]);

    // create the GlobalRouteManagerImpl
    auto srmv4 = new GlobalRouteManagerImpl<Ipv4Manager>();
    srmv4->DebugUseLsdb(srmlsdb);

    srmv4->DebugSPFCalculate(m_lsasv4[0]->GetLinkStateId()); // fill the routing table for node 0

    // now for ipv6
    BuildLsav6();
    // insert the LSAs into the GlobalRouteManagerLSDB
    auto srmlsdbv6 = new GlobalRouteManagerLSDB<Ipv6Manager>();

    srmlsdbv6->Insert(m_lsasv6[0]->GetLinkStateId(), m_lsasv6[0]);
    srmlsdbv6->Insert(m_lsasv6[1]->GetLinkStateId(), m_lsasv6[1]);
    srmlsdbv6->Insert(m_lsasv6[2]->GetLinkStateId(), m_lsasv6[2]);
    srmlsdbv6->Insert(m_lsasv6[3]->GetLinkStateId(), m_lsasv6[3]);

    // create the GlobalRouteManagerImpl
    auto srmv6 = new GlobalRouteManagerImpl<Ipv6Manager>();
    srmv6->DebugUseLsdb(srmlsdbv6);

    srmv6->DebugSPFCalculate(m_lsasv6[0]->GetLinkStateId()); // fill the routing table for node 0

    // now the tests for IPv4-----------------------

    Ptr<Ipv4L3Protocol> ip0 = nodes.Get(0)->GetObject<Ipv4L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip0, nullptr, "Error-- no Ipv4 object at node 0");
    Ptr<Ipv4RoutingProtocol> routing0 = ip0->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing0, nullptr, "Error-- no Ipv4 routing protocol object at node 0");
    Ptr<Ipv4GlobalRouting> globalRouting0 = routing0->GetObject<Ipv4GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting0, nullptr, "Error-- no Ipv4GlobalRouting object at node 0");

    // The only route to check is the network route
    uint32_t nRoutes0 = globalRouting0->GetNRoutes();
    NS_TEST_ASSERT_MSG_EQ(nRoutes0, 1, "Error-- Network route not found for node 0");
    Ipv4RoutingTableEntry* route = globalRouting0->GetRoute(0);
    NS_TEST_ASSERT_MSG_EQ(route->GetDest(),
                          Ipv4Address("10.1.1.0"),
                          "Error-- wrong destination for network route");
    NS_TEST_ASSERT_MSG_EQ(route->GetGateway(),
                          Ipv4Address("0.0.0.0"),
                          "Error-- wrong gateway for network route");

    // now the tests for IPv6-----------------------

    Ptr<Ipv6L3Protocol> ip0v6 = nodes.Get(0)->GetObject<Ipv6L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip0v6, nullptr, "Error-- no Ipv6 object at node 0");
    Ptr<Ipv6RoutingProtocol> routing0v6 = ip0v6->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing0v6, nullptr, "Error-- no Ipv6 routing protocol object at node 0");
    Ptr<Ipv6GlobalRouting> globalRouting0v6 = routing0v6->GetObject<Ipv6GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting0v6,
                          nullptr,
                          "Error-- no Ipv6GlobalRouting object at node 0");

    // The only route to check is the network route
    uint32_t nRoutes0v6 = globalRouting0v6->GetNRoutes();
    NS_TEST_ASSERT_MSG_EQ(nRoutes0v6, 1, "Error-- Network route not found for node 0");
    Ipv6RoutingTableEntry* routev6 = globalRouting0v6->GetRoute(0);
    NS_TEST_ASSERT_MSG_EQ(routev6->GetDest(),
                          Ipv6Address("2001:1::"),
                          "Error-- wrong destination for network route");
    NS_TEST_ASSERT_MSG_EQ(routev6->GetGateway(),
                          Ipv6Address("::"),
                          "Error-- wrong gateway for network route");

    Simulator::Run();

    Simulator::Stop(Seconds(3));
    Simulator::Destroy();
    // This delete clears the srm, which deletes the LSDB, which clears
    // all of the LSAs, which each destroys the attached LinkRecords.
    delete srmv4;
    delete srmv6;
    GlobalRouteManager<Ipv4Manager>::ResetRouterId();
    GlobalRouteManager<Ipv6Manager>::ResetRouterId();
}

/**
 * @ingroup internet-test
 *
 * @brief The purpose of this test is to check if Equal Cost MultiPath (ECMP) Routes are being built
 * and used correctly.
 */
class RandomEcmpTestCase : public TestCase
{
  public:
    RandomEcmpTestCase();

    void DoSetup() override;
    void DoRun() override;

  private:
    /**
     *@brief Builds the LSAs for the topology. These LSAs are manually created and inserted into the
     *       GlobalRouteManagerLSDB.Each node exports a router LSA.
     */
    void BuildLsav4();

    /**
     *@brief Builds the LSAs for the topology. These LSAs are manually created and inserted into the
     *       GlobalRouteManagerLSDB.Each node exports a router LSA.
     */
    void BuildLsav6();

    /**
     * @brief Helper function that checks the output of the routing path
     * and increments the corresponding route counter.
     * @param route The routing path to check
     */
    void IncrementCountv4(Ptr<Ipv4Route>& route);

    /**
     * @brief Helper function that checks the output of the routing path
     * and increments the corresponding route counter.
     * @param route The routing path to check
     */
    void IncrementCountv6(Ptr<Ipv6Route>& route);
    /**
     * @brief function that checks the routing table entries for the expected output.
     * @param globalroutingprotocol The routing protocol for the node whose routing table is to be
     * checked.
     * @param dests The expected destinations.
     * @param gws The expected gateways.
     */
    void CheckRoutesv4(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
                       std::vector<Ipv4Address>& dests,
                       std::vector<Ipv4Address>& gws);

    /**
     * @brief function that checks the routing table entries for the expected output.
     * @param globalroutingprotocol The routing protocol for the node whose routing table is to be
     * checked.
     * @param dests The expected destinations.
     * @param gws The expected gateways.
     */
    void CheckRoutesv6(Ptr<Ipv6GlobalRouting>& globalroutingprotocol,
                       std::vector<Ipv6Address>& dests,
                       std::vector<Ipv6Address>& gws);
    uint32_t route1v4 = 0; //!< Counter to keep track of the number of times route1 is used for ipv4
    uint32_t route2v4 = 0; //!< Counter to keep track of the number of times route2 is used for ipv4
    uint32_t route1v6 = 0; //!< Counter to keep track of the number of times route1 is used for ipv6
    uint32_t route2v6 = 0; //!< Counter to keep track of the number of times route2 is used for ipv6
    NodeContainer nodes;   //!< NodeContainer to hold the nodes in the topology
    std::vector<GlobalRoutingLSA<Ipv4Manager>*> m_lsasv4; //!< The LSAs for the topology
    std::vector<GlobalRoutingLSA<Ipv6Manager>*> m_lsasv6; //!< The LSAs for the topology
};

RandomEcmpTestCase::RandomEcmpTestCase()
    : TestCase("RandomEcmpTestCase")
{
}

void
RandomEcmpTestCase::DoSetup()
{
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
    Config::SetDefault("ns3::Ipv6GlobalRouting::RandomEcmpRouting", BooleanValue(true));

    /*
    //   Creating a Simple topology with 4 nodes and 3 links
    //
    //
    //
    //          ------n1------
    //         /              \
    //        /                \
    //       n0                n3
    //        \                /
    //         \              /
    //          ------n2------
    //
    //    Link n0-n1: 10.1.1.1/30,10.1.1.2/30
    //    Link n0-n2: 10.1.2.1/30,10.1.2.2/30
    //    Link n1-n3: 10.1.3.1/30,10.1.3.2/30
    //    Link n2-n3: 10.1.4.1/30,10.1.4.2/30
    */
    nodes.Create(4);

    Ipv4GlobalRoutingHelper globalhelper;
    Ipv6GlobalRoutingHelper globalhelperv6;
    InternetStackHelper stack;
    stack.SetRoutingHelper(globalhelper);
    stack.SetRoutingHelper(globalhelperv6);
    stack.Install(nodes);
    SimpleNetDeviceHelper devHelper;
    devHelper.SetNetDevicePointToPointMode(true);
    Ptr<SimpleChannel> channel1 = CreateObject<SimpleChannel>();
    NetDeviceContainer d01 = devHelper.Install(nodes.Get(0), channel1);
    d01.Add(devHelper.Install(nodes.Get(1), channel1));
    Ptr<SimpleChannel> channel2 = CreateObject<SimpleChannel>();
    NetDeviceContainer d23 = devHelper.Install(nodes.Get(2), channel2);
    d23.Add(devHelper.Install(nodes.Get(3), channel2));
    Ptr<SimpleChannel> channel3 = CreateObject<SimpleChannel>();
    NetDeviceContainer d02 = devHelper.Install(nodes.Get(0), channel3);
    d02.Add(devHelper.Install(nodes.Get(2), channel3));
    Ptr<SimpleChannel> channel4 = CreateObject<SimpleChannel>();
    NetDeviceContainer d13 = devHelper.Install(nodes.Get(1), channel4);
    d13.Add(devHelper.Install(nodes.Get(3), channel4));

    // Assign IP addresses to the devices
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.252");
    Ipv4InterfaceContainer i01 = address.Assign(d01);

    address.SetBase("10.1.2.0", "255.255.255.252");
    Ipv4InterfaceContainer i02 = address.Assign(d02);

    address.SetBase("10.1.3.0", "255.255.255.252");
    Ipv4InterfaceContainer i13 = address.Assign(d13);

    address.SetBase("10.1.4.0", "255.255.255.252");
    Ipv4InterfaceContainer i23 = address.Assign(d23);

    Ipv6AddressHelper addressv6;
    addressv6.SetBase("2001:1::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i01v6 = addressv6.Assign(d01);
    addressv6.SetBase("2001:2::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i02v6 = addressv6.Assign(d02);
    addressv6.SetBase("2001:3::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i13v6 = addressv6.Assign(d13);
    addressv6.SetBase("2001:4::", Ipv6Prefix(64));
    Ipv6InterfaceContainer i23v6 = addressv6.Assign(d23);
}

void
RandomEcmpTestCase::CheckRoutesv4(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
                                  std::vector<Ipv4Address>& dests,
                                  std::vector<Ipv4Address>& gws)
{
    // check each individual routes destination and gateway
    for (uint32_t i = 0; i < globalroutingprotocol->GetNRoutes(); i++)
    {
        Ipv4RoutingTableEntry* route = globalroutingprotocol->GetRoute(i);
        NS_LOG_DEBUG("dest " << route->GetDest() << " gw " << route->GetGateway());
        NS_TEST_ASSERT_MSG_EQ(route->GetDest(), dests[i], "Error-- wrong destination");
        NS_TEST_ASSERT_MSG_EQ(route->GetGateway(), gws[i], "Error-- wrong gateway");
    }
}

void
RandomEcmpTestCase::CheckRoutesv6(Ptr<Ipv6GlobalRouting>& globalroutingprotocol,
                                  std::vector<Ipv6Address>& dests,
                                  std::vector<Ipv6Address>& gws)
{
    // check each individual routes destination and gateway
    for (uint32_t i = 0; i < globalroutingprotocol->GetNRoutes(); i++)
    {
        Ipv6RoutingTableEntry* route = globalroutingprotocol->GetRoute(i);
        NS_LOG_DEBUG("dest " << route->GetDest() << " gw " << route->GetGateway());
        NS_TEST_ASSERT_MSG_EQ(route->GetDest(), dests[i], "Error-- wrong destination");
        NS_TEST_ASSERT_MSG_EQ(route->GetGateway(), gws[i], "Error-- wrong gateway");
    }
}

void
RandomEcmpTestCase::IncrementCountv4(Ptr<Ipv4Route>& route)
{
    if (route->GetGateway() == Ipv4Address("10.1.1.2"))
    {
        route1v4++;
    }
    else if (route->GetGateway() == Ipv4Address("10.1.2.2"))
    {
        route2v4++;
    }
}

void
RandomEcmpTestCase::IncrementCountv6(Ptr<Ipv6Route>& route)
{
    if (route->GetGateway() == Ipv6Address("fe80::200:ff:fe00:2"))
    {
        route1v6++;
    }
    else if (route->GetGateway() == Ipv6Address("fe80::200:ff:fe00:6"))
    {
        route2v6++;
    }
}

void
RandomEcmpTestCase::BuildLsav4()
{
    // we manually create the link state database, we could have used BuildRoutingTables() but this
    // way testing the GlobalRouteManagerImpl without the involvement of GLobalRouter makes it
    // easier to debug each individually

    // router 0
    auto lr0 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.1",
                                                 "10.1.1.1",
                                                 1);
    auto lr1 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.1.2",
                                                 "255.255.255.252",
                                                 1);

    auto lr2 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.2",
                                                 "10.1.2.1",
                                                 1);
    auto lr3 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.2.2",
                                                 "255.255.255.252",
                                                 1);

    auto lsa0 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa0->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa0->SetStatus(GlobalRoutingLSA<Ipv4Manager>::LSA_SPF_NOT_EXPLORED);
    lsa0->SetLinkStateId("0.0.0.0");
    lsa0->SetAdvertisingRouter("0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    lsa0->AddLinkRecord(lr0);
    lsa0->AddLinkRecord(lr1);
    lsa0->AddLinkRecord(lr2);
    lsa0->AddLinkRecord(lr3);
    m_lsasv4.push_back(lsa0);

    // router 1
    auto lr4 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.0",
                                                 "10.1.1.2",
                                                 1);
    auto lr5 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.1.1",
                                                 "255.255.255.252",
                                                 1);

    auto lr6 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.3",
                                                 "10.1.3.1",
                                                 1);
    auto lr7 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.3.2",
                                                 "255.255.255.252",
                                                 1);

    auto lsa1 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa1->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa1->SetStatus(GlobalRoutingLSA<Ipv4Manager>::LSA_SPF_NOT_EXPLORED);
    lsa1->SetLinkStateId("0.0.0.1");
    lsa1->SetAdvertisingRouter("0.0.0.1");
    lsa1->SetNode(nodes.Get(1));
    lsa1->AddLinkRecord(lr4);
    lsa1->AddLinkRecord(lr5);
    lsa1->AddLinkRecord(lr6);
    lsa1->AddLinkRecord(lr7);
    m_lsasv4.push_back(lsa1);

    // router 2
    auto lr8 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.0",
                                                 "10.1.2.2",
                                                 1);
    auto lr9 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.2.1",
                                                 "255.255.255.252",
                                                 1);
    auto lr10 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.3",
                                                 "10.1.4.1",
                                                 1);
    auto lr11 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.4.2",
                                                 "255.255.255.252",
                                                 1);

    auto lsa2 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa2->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa2->SetStatus(GlobalRoutingLSA<Ipv4Manager>::LSA_SPF_NOT_EXPLORED);
    lsa2->SetLinkStateId("0.0.0.2");
    lsa2->SetAdvertisingRouter("0.0.0.2");
    lsa2->SetNode(nodes.Get(2));
    lsa2->AddLinkRecord(lr8);
    lsa2->AddLinkRecord(lr9);
    lsa2->AddLinkRecord(lr10);
    lsa2->AddLinkRecord(lr11);
    m_lsasv4.push_back(lsa2);

    // router 3
    auto lr12 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.1",
                                                 "10.1.3.2",
                                                 1);
    auto lr13 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.3.1",
                                                 "255.255.255.252",
                                                 1);
    auto lr14 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::PointToPoint,
                                                 "0.0.0.2",
                                                 "10.1.4.2",
                                                 1);
    auto lr15 =
        new GlobalRoutingLinkRecord<Ipv4Manager>(GlobalRoutingLinkRecord<Ipv4Manager>::StubNetwork,
                                                 "10.1.4.1",
                                                 "255.255.255.252",
                                                 1);

    auto lsa3 = new GlobalRoutingLSA<Ipv4Manager>();
    lsa3->SetLSType(GlobalRoutingLSA<Ipv4Manager>::RouterLSA);
    lsa3->SetStatus(GlobalRoutingLSA<Ipv4Manager>::LSA_SPF_NOT_EXPLORED);
    lsa3->SetLinkStateId("0.0.0.3");
    lsa3->SetAdvertisingRouter("0.0.0.3");
    lsa3->SetNode(nodes.Get(3));
    lsa3->AddLinkRecord(lr12);
    lsa3->AddLinkRecord(lr13);
    lsa3->AddLinkRecord(lr14);
    lsa3->AddLinkRecord(lr15);
    m_lsasv4.push_back(lsa3);
}

void
RandomEcmpTestCase::BuildLsav6()
{
    // we manually create the link state database, we could have used BuildRoutingTables() but this
    // way testing the GlobalRouteManagerImpl without the involvement of GLobalRouter makes it
    // easier to debug each individually

    // router 0
    auto lr0 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.1",
                                                 "2001:1::200:ff:fe00:1",
                                                 "fe80::200:ff:fe00:1",
                                                 1);
    // only need to do this once for prefix of 64 bits
    uint8_t buf[16];
    auto prefix = Ipv6Prefix(64);
    prefix.GetBytes(buf); // frown

    auto lr1 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:1::200:ff:fe00:2",
                                                 Ipv6Address(buf),
                                                 1);
    auto lr2 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.2",
                                                 "2001:2::200:ff:fe00:5",
                                                 "fe80::200:ff:fe00:5",
                                                 1);
    auto lr3 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:2::200:ff:fe00:6",
                                                 Ipv6Address(buf),
                                                 1);

    auto lsa0 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa0->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa0->SetStatus(GlobalRoutingLSA<Ipv6Manager>::LSA_SPF_NOT_EXPLORED);
    lsa0->SetLinkStateId("::ffff:0.0.0.0");
    lsa0->SetAdvertisingRouter("::ffff:0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    lsa0->AddLinkRecord(lr0);
    lsa0->AddLinkRecord(lr1);
    lsa0->AddLinkRecord(lr2);
    lsa0->AddLinkRecord(lr3);
    m_lsasv6.push_back(lsa0);

    // router 1
    auto lr4 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.0",
                                                 "2001:1::200:ff:fe00:2",
                                                 "fe80::200:ff:fe00:2",
                                                 1);
    auto lr5 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:1::200:ff:fe00:1",
                                                 Ipv6Address(buf),
                                                 1);

    auto lr6 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.3",
                                                 "2001:3::200:ff:fe00:7",
                                                 "fe80::200:ff:fe00:7",
                                                 1);
    auto lr7 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:3::200:ff:fe00:8",
                                                 Ipv6Address(buf),
                                                 1);

    auto lsa1 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa1->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa1->SetStatus(GlobalRoutingLSA<Ipv6Manager>::LSA_SPF_NOT_EXPLORED);
    lsa1->SetLinkStateId("::ffff:0.0.0.1");
    lsa1->SetAdvertisingRouter("::ffff:0.0.0.1");
    lsa1->SetNode(nodes.Get(1));
    lsa1->AddLinkRecord(lr4);
    lsa1->AddLinkRecord(lr5);
    lsa1->AddLinkRecord(lr6);
    lsa1->AddLinkRecord(lr7);
    m_lsasv6.push_back(lsa1);

    // router 2
    auto lr8 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.0",
                                                 "2001:2::200:ff:fe00:6",
                                                 "fe80::200:ff:fe00:6",
                                                 1);
    auto lr9 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:2::200:ff:fe00:5",
                                                 Ipv6Address(buf),
                                                 1);
    auto lr10 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.3",
                                                 "2001:4::200:ff:fe00:3",
                                                 "fe80::200:ff:fe00:3",
                                                 1);
    auto lr11 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:4::200:ff:fe00:4",
                                                 Ipv6Address(buf),
                                                 1);

    auto lsa2 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa2->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa2->SetStatus(GlobalRoutingLSA<Ipv6Manager>::LSA_SPF_NOT_EXPLORED);
    lsa2->SetLinkStateId("::ffff:0.0.0.2");
    lsa2->SetAdvertisingRouter("::ffff:0.0.0.2");
    lsa2->SetNode(nodes.Get(2));
    lsa2->AddLinkRecord(lr8);
    lsa2->AddLinkRecord(lr9);
    lsa2->AddLinkRecord(lr10);
    lsa2->AddLinkRecord(lr11);
    m_lsasv6.push_back(lsa2);

    // router 3
    auto lr12 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.1",
                                                 "2001:3::200:ff:fe00:8",
                                                 "fe80::200:ff:fe00:8",
                                                 1);
    auto lr13 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:3::200:ff:fe00:7",
                                                 Ipv6Address(buf),
                                                 1);
    auto lr14 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::PointToPoint,
                                                 "::ffff:0.0.0.2",
                                                 "2001:4::200:ff:fe00:4",
                                                 "fe80::200:ff:fe00:4",
                                                 1);
    auto lr15 =
        new GlobalRoutingLinkRecord<Ipv6Manager>(GlobalRoutingLinkRecord<Ipv6Manager>::StubNetwork,
                                                 "2001:4::200:ff:fe00:3",
                                                 Ipv6Address(buf),
                                                 1);

    auto lsa3 = new GlobalRoutingLSA<Ipv6Manager>();
    lsa3->SetLSType(GlobalRoutingLSA<Ipv6Manager>::RouterLSA);
    lsa3->SetStatus(GlobalRoutingLSA<Ipv6Manager>::LSA_SPF_NOT_EXPLORED);
    lsa3->SetLinkStateId("::ffff:0.0.0.3");
    lsa3->SetAdvertisingRouter("::ffff:0.0.0.3");
    lsa3->SetNode(nodes.Get(3));
    lsa3->AddLinkRecord(lr12);
    lsa3->AddLinkRecord(lr13);
    lsa3->AddLinkRecord(lr14);
    lsa3->AddLinkRecord(lr15);
    m_lsasv6.push_back(lsa3);
}

void
RandomEcmpTestCase::DoRun()
{
    // We need a deterministic output to pass the test. So we set a fixed seed and run for our
    // UniformRandomVariable
    uint32_t oldseed = RngSeedManager::GetSeed();
    uint32_t oldrun = RngSeedManager::GetRun();
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    BuildLsav4();

    // insert the LSAs into the GlobalRouteManagerLSDB
    auto srmlsdbv4 = new GlobalRouteManagerLSDB<Ipv4Manager>();
    srmlsdbv4->Insert(m_lsasv4[0]->GetLinkStateId(), m_lsasv4[0]);
    srmlsdbv4->Insert(m_lsasv4[1]->GetLinkStateId(), m_lsasv4[1]);
    srmlsdbv4->Insert(m_lsasv4[2]->GetLinkStateId(), m_lsasv4[2]);
    srmlsdbv4->Insert(m_lsasv4[3]->GetLinkStateId(), m_lsasv4[3]);

    // create the GlobalRouteManagerImpl
    auto srmv4 = new GlobalRouteManagerImpl<Ipv4Manager>();
    srmv4->DebugUseLsdb(srmlsdbv4);

    // we manually call the DebugSPFCalculate to fill the routing tables for node 0
    srmv4->DebugSPFCalculate(m_lsasv4[0]->GetLinkStateId());

    // now for ipv6
    BuildLsav6();

    // insert the LSAs into the GlobalRouteManagerLSDB
    auto srmlsdbv6 = new GlobalRouteManagerLSDB<Ipv6Manager>();
    srmlsdbv6->Insert(m_lsasv6[0]->GetLinkStateId(), m_lsasv6[0]);
    srmlsdbv6->Insert(m_lsasv6[1]->GetLinkStateId(), m_lsasv6[1]);
    srmlsdbv6->Insert(m_lsasv6[2]->GetLinkStateId(), m_lsasv6[2]);
    srmlsdbv6->Insert(m_lsasv6[3]->GetLinkStateId(), m_lsasv6[3]);

    // create the GlobalRouteManagerImpl
    auto srmv6 = new GlobalRouteManagerImpl<Ipv6Manager>();
    srmv6->DebugUseLsdb(srmlsdbv6);
    // we manually call the DebugSPFCalculate to fill the routing tables for node 0
    srmv6->DebugSPFCalculate(m_lsasv6[0]->GetLinkStateId());

    // now the tests-----------------------

    Ptr<Ipv4L3Protocol> ip0 = nodes.Get(0)->GetObject<Ipv4L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip0, nullptr, "Error-- no Ipv4 object at node 0");
    Ptr<Ipv4RoutingProtocol> routing0 = ip0->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing0, nullptr, "Error-- no Ipv4 routing protocol object at node 0");
    Ptr<Ipv4GlobalRouting> globalRouting0 = routing0->GetObject<Ipv4GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting0, nullptr, "Error-- no Ipv4GlobalRouting object at node 0");

    // assign streams to the UniformRandomVariable
    globalRouting0->AssignStreams(0);
    // check that the correct number of routes were built

    uint32_t nRoutes0 = globalRouting0->GetNRoutes();
    NS_TEST_ASSERT_MSG_EQ(nRoutes0, 14, "Error-- incorrect number of routes found on node 0");

    // check that the routes are correct
    std::vector<Ipv4Address> expectedDests;
    std::vector<Ipv4Address> expectedNextHops;

    // add all the expected destinations and gateways. This is pretty verbose
    // Please Note: The order of the following line is important because we check the routing table
    // entries in order. The test will fail if we reorder the vector entries.

    // For Routing table of Node 0 we check the following Destinations and Gateways
    expectedDests.emplace_back("10.1.1.2");
    expectedDests.emplace_back("10.1.3.1");
    expectedDests.emplace_back("10.1.2.2");
    expectedDests.emplace_back("10.1.4.1");
    expectedDests.emplace_back("10.1.3.2");
    expectedDests.emplace_back("10.1.3.2");
    expectedDests.emplace_back("10.1.4.2");
    expectedDests.emplace_back("10.1.4.2");
    expectedDests.emplace_back("10.1.1.0");
    expectedDests.emplace_back("10.1.3.0");
    expectedDests.emplace_back("10.1.3.0");
    expectedDests.emplace_back("10.1.4.0");
    expectedDests.emplace_back("10.1.4.0");
    expectedDests.emplace_back("10.1.2.0");

    expectedNextHops.emplace_back("10.1.1.2");
    expectedNextHops.emplace_back("10.1.1.2");
    expectedNextHops.emplace_back("10.1.2.2");
    expectedNextHops.emplace_back("10.1.2.2");
    expectedNextHops.emplace_back("10.1.1.2");
    expectedNextHops.emplace_back("10.1.2.2");
    expectedNextHops.emplace_back("10.1.1.2");
    expectedNextHops.emplace_back("10.1.2.2");
    expectedNextHops.emplace_back("10.1.1.2");
    expectedNextHops.emplace_back("10.1.1.2");
    expectedNextHops.emplace_back("10.1.2.2");
    expectedNextHops.emplace_back("10.1.1.2");
    expectedNextHops.emplace_back("10.1.2.2");
    expectedNextHops.emplace_back("10.1.2.2");

    // Test 1: Check the Routing Table of Node 0
    CheckRoutesv4(globalRouting0, expectedDests, expectedNextHops);

    // Test 2: Check that the equal cost routes are being used at least once.
    // we need to call RouteOutput() at node 0 and check the output
    // route from the table. The thing to check here is that different equal cost routes are
    // returned across different calls to this method.

    Socket::SocketErrno errno_;
    Ptr<NetDevice> oif(nullptr);
    Ptr<Packet> packetv6 = Create<Packet>();
    Ipv4Header ipHeader;
    ipHeader.SetSource(Ipv4Address("10.1.1.1"));
    ipHeader.SetDestination(Ipv4Address("10.1.4.2"));

    for (uint32_t i = 0; i < 10; i++)
    {
        Ptr<Ipv4Route> route = globalRouting0->RouteOutput(packetv6, ipHeader, oif, errno_);
        IncrementCountv4(route);
    }

    // now the tests for IPv6-----------------------

    Ptr<Ipv6L3Protocol> ip0v6 = nodes.Get(0)->GetObject<Ipv6L3Protocol>();
    NS_TEST_ASSERT_MSG_NE(ip0v6, nullptr, "Error-- no Ipv6 object at node 0");
    Ptr<Ipv6RoutingProtocol> routing0v6 = ip0v6->GetRoutingProtocol();
    NS_TEST_ASSERT_MSG_NE(routing0v6, nullptr, "Error-- no Ipv6 routing protocol object at node 0");
    Ptr<Ipv6GlobalRouting> globalRouting0v6 = routing0v6->GetObject<Ipv6GlobalRouting>();
    NS_TEST_ASSERT_MSG_NE(globalRouting0v6,
                          nullptr,
                          "Error-- no Ipv6GlobalRouting object at node 0");

    // assign streams to the UniformRandomVariable
    globalRouting0v6->AssignStreams(0);
    // check that the correct number of routes were built
    uint32_t nRoutes0v6 = globalRouting0v6->GetNRoutes();
    NS_TEST_ASSERT_MSG_EQ(nRoutes0v6, 14, "Error-- incorrect number of routes found on node 0");

    // check that the routes are correct
    std::vector<Ipv6Address> expectedDestsv6;
    std::vector<Ipv6Address> expectedNextHopsv6;

    // add all the expected destinations and gateways. This is pretty verbose
    // Please Note: The order of the following line is important because we check the routing table
    // entries in order. The test will fail if we reorder the vector entries.

    // For Routing table of Node 0 we check the following Destinations and Gateways
    expectedDestsv6.emplace_back("2001:1::200:ff:fe00:2");
    expectedDestsv6.emplace_back("2001:3::200:ff:fe00:7");
    expectedDestsv6.emplace_back("2001:2::200:ff:fe00:6");
    expectedDestsv6.emplace_back("2001:4::200:ff:fe00:3");
    expectedDestsv6.emplace_back("2001:3::200:ff:fe00:8");
    expectedDestsv6.emplace_back("2001:3::200:ff:fe00:8");
    expectedDestsv6.emplace_back("2001:4::200:ff:fe00:4");
    expectedDestsv6.emplace_back("2001:4::200:ff:fe00:4");
    expectedDestsv6.emplace_back("2001:1::");
    expectedDestsv6.emplace_back("2001:3::");
    expectedDestsv6.emplace_back("2001:3::");
    expectedDestsv6.emplace_back("2001:4::");
    expectedDestsv6.emplace_back("2001:4::");
    expectedDestsv6.emplace_back("2001:2::");

    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:2");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:2");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:6");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:6");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:2");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:6");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:2");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:6");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:2");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:2");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:6");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:2");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:6");
    expectedNextHopsv6.emplace_back("fe80::200:ff:fe00:6");
    // Test 1: Check the Routing Table of Node 0
    CheckRoutesv6(globalRouting0v6, expectedDestsv6, expectedNextHopsv6);

    // Test 2: Check that the equal cost routes are being used at least once.
    // we need to call RouteOutput() at node 0 and check the output
    // route from the table. The thing to check here is that different equal cost routes are
    // returned across different calls to this method.

    Ptr<Packet> packet = Create<Packet>();
    Ipv6Header ipHeaderv6;
    ipHeaderv6.SetSource(Ipv6Address("2001:1::200:ff:fe00:1"));
    ipHeaderv6.SetDestination(Ipv6Address("2001:4::200:ff:fe00:4"));

    for (uint32_t i = 0; i < 10; i++)
    {
        Ptr<Ipv6Route> route = globalRouting0v6->RouteOutput(packet, ipHeaderv6, oif, errno_);
        IncrementCountv6(route);
    }

    Simulator::Run();

    NS_TEST_ASSERT_MSG_GT_OR_EQ(
        route1v4,
        1,
        "The routing path for node 0 to node 3 does not match the expected output, "
        "Equal Cost MultiPath (ECMP) routes are not being used correctly for ipv4");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(
        route2v4,
        1,
        "The routing path for node 0 to node 3 does not match the expected output, "
        "Equal Cost MultiPath (ECMP) routes are not being used correctly for ipv4");

    NS_TEST_ASSERT_MSG_GT_OR_EQ(
        route1v6,
        1,
        "The routing path for node 0 to node 3 does not match the expected output, "
        "Equal Cost MultiPath (ECMP) routes are not being used correctly for ipv6");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(
        route2v6,
        1,
        "The routing path for node 0 to node 3 does not match the expected output, "
        "Equal Cost MultiPath (ECMP) routes are not being used correctly for ipv6");

    // end of test--------------------

    Simulator::Stop(Seconds(5));

    Simulator::Destroy();

    RngSeedManager::SetSeed(oldseed);
    RngSeedManager::SetRun(oldrun);
    // This delete clears the srm, which deletes the LSDB, which clears
    // all of the LSAs, which each destroys the attached LinkRecords.
    delete srmv4;
    delete srmv6;
}

/**
 * @ingroup internet-test
 *
 * @brief Global Route Manager TestSuite
 */
class GlobalRouteManagerImplTestSuite : public TestSuite
{
  public:
    GlobalRouteManagerImplTestSuite();

  private:
};

GlobalRouteManagerImplTestSuite::GlobalRouteManagerImplTestSuite()
    : TestSuite("global-route-manager-impl", Type::UNIT)
{
    AddTestCase(new LinkRoutesTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new LanRoutesTestCase(), TestCase::Duration::QUICK);
    AddTestCase(new RandomEcmpTestCase(), TestCase::Duration::QUICK);
}

static GlobalRouteManagerImplTestSuite
    g_globalRoutingManagerImplTestSuite; //!< Static variable for test initialization
