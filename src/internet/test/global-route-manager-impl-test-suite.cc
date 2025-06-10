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
#include "ns3/internet-module.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ipv4-global-routing.h"
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
    void BuildLsa();

    /**
     *  @brief Checks the Routing Table Entries for the expected output.
     * @param globalroutingprotocol The routing protocol for the node whose routing table is to be
     * checked.
     * @param dests The expected destinations.
     * @param gws The expected gateways.
     */
    void CheckRoutes(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
                     std::vector<Ipv4Address>& dests,
                     std::vector<Ipv4Address>& gws);

    NodeContainer nodes;                   //!< NodeContainer to hold the nodes in the topology
    std::vector<GlobalRoutingLSA*> m_lsas; //!< The LSAs for the topology
};

LinkRoutesTestCase::LinkRoutesTestCase()
    : TestCase("LinkRoutesTestCase")
{
}

void
LinkRoutesTestCase::CheckRoutes(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
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

    Ipv4GlobalRoutingHelper globalhelper;
    InternetStackHelper stack;
    stack.SetRoutingHelper(globalhelper);
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
}

void
LinkRoutesTestCase::BuildLsa()
{
    // Manually build the link state database; four routers (0-3), 3 point-to-point
    // links

    // Router 0
    auto lr0 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.2",  // link id -> router ID 0.0.0.2
                                           "10.1.1.1", // link data -> Local IP address of router 0
                                           1);         // metric

    auto lr1 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.1.2", // link id ->adjacent neighbor's IP address
                                           "255.255.255.252",
                                           1);

    auto lsa0 = new GlobalRoutingLSA();
    lsa0->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa0->SetLinkStateId("0.0.0.0");
    lsa0->SetAdvertisingRouter("0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    lsa0->AddLinkRecord(lr0);
    lsa0->AddLinkRecord(lr1);
    m_lsas.push_back(lsa0);

    // Router 1
    auto lr2 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.2",
                                           "10.1.2.1",
                                           1);

    auto lr3 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.2.2",
                                           "255.255.255.252",
                                           1);

    auto lsa1 = new GlobalRoutingLSA();
    lsa1->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa1->SetLinkStateId("0.0.0.1");
    lsa1->SetAdvertisingRouter("0.0.0.1");
    lsa1->AddLinkRecord(lr2);
    lsa1->AddLinkRecord(lr3);
    lsa1->SetNode(nodes.Get(1));
    m_lsas.push_back(lsa1);

    // Router 2
    auto lr4 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.0",
                                           "10.1.1.2",
                                           1);

    auto lr5 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.1.1",
                                           "255.255.255.252",
                                           1);

    auto lr6 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.1",
                                           "10.1.2.2",
                                           1);

    auto lr7 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.2.2",
                                           "255.255.255.252",
                                           1);

    auto lr8 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.3",
                                           "10.1.3.1",
                                           1);

    auto lr9 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.3.2",
                                           "255.255.255.252",
                                           1);

    auto lsa2 = new GlobalRoutingLSA();
    lsa2->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa2->SetLinkStateId("0.0.0.2");
    lsa2->SetAdvertisingRouter("0.0.0.2");
    lsa2->AddLinkRecord(lr4);
    lsa2->AddLinkRecord(lr5);
    lsa2->AddLinkRecord(lr6);
    lsa2->AddLinkRecord(lr7);
    lsa2->AddLinkRecord(lr8);
    lsa2->AddLinkRecord(lr9);
    lsa2->SetNode(nodes.Get(2));
    m_lsas.push_back(lsa2);

    // Router 3
    auto lr10 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                            "0.0.0.2",
                                            "10.1.3.2",
                                            1);

    auto lr11 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                            "10.1.3.1",
                                            "255.255.255.252",
                                            1);

    auto lsa3 = new GlobalRoutingLSA();
    lsa3->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa3->SetLinkStateId("0.0.0.3");
    lsa3->SetAdvertisingRouter("0.0.0.3");
    lsa3->AddLinkRecord(lr10);
    lsa3->AddLinkRecord(lr11);
    lsa3->SetNode(nodes.Get(3));
    m_lsas.push_back(lsa3);
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

    BuildLsa();

    // Test the database
    auto srmlsdb = new GlobalRouteManagerLSDB();
    srmlsdb->Insert(m_lsas[0]->GetLinkStateId(), m_lsas[0]);
    srmlsdb->Insert(m_lsas[1]->GetLinkStateId(), m_lsas[1]);
    srmlsdb->Insert(m_lsas[2]->GetLinkStateId(), m_lsas[2]);
    srmlsdb->Insert(m_lsas[3]->GetLinkStateId(), m_lsas[3]);

    NS_TEST_ASSERT_MSG_EQ(
        m_lsas[2],
        srmlsdb->GetLSA(m_lsas[2]->GetLinkStateId()),
        "The Ipv4Address is not stored as the link state ID"); // LSAs are mapped by router id as
                                                               // key here we check that they are
                                                               // indeed getting the right lsa for
                                                               // the right key

    // next, calculate routes based on the manually created LSDB
    auto srm = new GlobalRouteManagerImpl();
    srm->DebugUseLsdb(srmlsdb); // manually add in an LSDB

    srm->DebugSPFCalculate(m_lsas[0]->GetLinkStateId()); // fill routing table for node n0

    srm->DebugSPFCalculate(m_lsas[1]->GetLinkStateId()); // fill routing table for node n1

    srm->DebugSPFCalculate(m_lsas[2]->GetLinkStateId()); // fill routing table for node n2

    srm->DebugSPFCalculate(m_lsas[3]->GetLinkStateId()); // fill routing table for node n3

    Simulator::Run();

    //-----------------Now the tests------------------
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

    CheckRoutes(globalRouting2, expecteddests, expectedgws);

    Simulator::Run();

    Simulator::Stop(Seconds(3));
    Simulator::Destroy();

    // This delete clears the srm, which deletes the LSDB, which clears
    // all of the LSAs, which each destroys the attached LinkRecords.
    delete srm;
    // reset the router ID counter to zero so that it does not affect other tests
    //  that may run after this one in the same program run.
    GlobalRouteManager::ResetRouterId();
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
     *@brief Builds the LSAs for the topology. These LSAs are manually created and inserted into the
     * GlobalRouteManagerLSDB.Each node exports a router LSA. In addition,the designated router
     * also Exports the Network LSA.
     */
    void BuildLsa();

    NodeContainer nodes;                   //!< NodeContainer to hold the nodes in the topology
    std::vector<GlobalRoutingLSA*> m_lsas; //!< The LSAs for the topology
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
    Ipv4GlobalRoutingHelper globalhelper;
    InternetStackHelper stack;
    stack.SetRoutingHelper(globalhelper);
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
}

void
LanRoutesTestCase::BuildLsa()
{
    // we manually create the link state database, we could have used BuildRoutingTables() but this
    // way testing the GlobalRouteManagerImpl without the involvement of GLobalRouter makes it
    // easier to debug each individually

    // router 0
    auto lr0 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::TransitNetwork,
                                           "10.1.1.1",
                                           "10.1.1.1",
                                           1);
    auto lsa0 = new GlobalRoutingLSA();
    lsa0->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa0->AddLinkRecord(lr0);
    lsa0->SetLinkStateId("0.0.0.0");
    lsa0->SetAdvertisingRouter("0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    m_lsas.push_back(lsa0);

    // router 1
    auto lr1 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::TransitNetwork,
                                           "10.1.1.1",
                                           "10.1.1.2",
                                           1);
    auto lsa1 = new GlobalRoutingLSA();
    lsa1->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa1->AddLinkRecord(lr1);
    lsa1->SetLinkStateId("0.0.0.1");
    lsa1->SetAdvertisingRouter("0.0.0.1");
    lsa1->SetNode(nodes.Get(1));
    m_lsas.push_back(lsa1);

    // router 2
    auto lr2 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::TransitNetwork,
                                           "10.1.1.1",
                                           "10.1.1.3",
                                           1);
    auto lsa2 = new GlobalRoutingLSA();
    lsa2->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa2->AddLinkRecord(lr2);
    lsa2->SetLinkStateId("0.0.0.2");
    lsa2->SetAdvertisingRouter("0.0.0.2");
    lsa2->SetNode(nodes.Get(2));
    m_lsas.push_back(lsa2);

    // router0 is the designated router for the LAN. it also exports the network LSA
    auto lsa0network = new GlobalRoutingLSA();
    lsa0network->SetLSType(GlobalRoutingLSA::NetworkLSA);
    lsa0network->SetLinkStateId("10.1.1.1");
    lsa0network->SetAdvertisingRouter("0.0.0.0");
    lsa0network->AddAttachedRouter("10.1.1.1");
    lsa0network->AddAttachedRouter("10.1.1.2");
    lsa0network->AddAttachedRouter("10.1.1.3");
    lsa0network->SetNetworkLSANetworkMask("255.255.255.248");
    m_lsas.push_back(lsa0network); // note the index of the network lsa
}

void
LanRoutesTestCase::DoRun()
{
    BuildLsa();
    // insert the LSAs into the GlobalRouteManagerLSDB
    auto srmlsdb = new GlobalRouteManagerLSDB();

    srmlsdb->Insert(m_lsas[0]->GetLinkStateId(), m_lsas[0]);
    srmlsdb->Insert(m_lsas[1]->GetLinkStateId(), m_lsas[1]);
    srmlsdb->Insert(m_lsas[2]->GetLinkStateId(), m_lsas[2]);
    srmlsdb->Insert(m_lsas[3]->GetLinkStateId(), m_lsas[3]);

    // create the GlobalRouteManagerImpl
    auto srm = new GlobalRouteManagerImpl();
    srm->DebugUseLsdb(srmlsdb);

    srm->DebugSPFCalculate(m_lsas[0]->GetLinkStateId()); // fill the routing table for node 0

    // now the tests-----------------------

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

    Simulator::Run();

    Simulator::Stop(Seconds(3));
    Simulator::Destroy();
    // This delete clears the srm, which deletes the LSDB, which clears
    // all of the LSAs, which each destroys the attached LinkRecords.
    delete srm;
    GlobalRouteManager::ResetRouterId();
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
    void BuildLsa();

    /**
     * @brief Helper function that checks the output of the routing path
     * and increments the corresponding route counter.
     * @param route The routing path to check
     */
    void IncrementCount(Ptr<Ipv4Route>& route);
    /**
     * @brief function that checks the routing table entries for the expected output.
     * @param globalroutingprotocol The routing protocol for the node whose routing table is to be
     * checked.
     * @param dests The expected destinations.
     * @param gws The expected gateways.
     */
    void CheckRoutes(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
                     std::vector<Ipv4Address>& dests,
                     std::vector<Ipv4Address>& gws);
    uint32_t route1 = 0; //!< Counter to keep track of the number of times route1 is used
    uint32_t route2 = 0; //!< Counter to keep track of the number of times route2 is used
    NodeContainer nodes; //!< NodeContainer to hold the nodes in the topology
    std::vector<GlobalRoutingLSA*> m_lsas; //!< The LSAs for the topology
};

RandomEcmpTestCase::RandomEcmpTestCase()
    : TestCase("RandomEcmpTestCase")
{
}

void
RandomEcmpTestCase::DoSetup()
{
    Config::SetDefault("ns3::Ipv4GlobalRouting::RandomEcmpRouting", BooleanValue(true));
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
    InternetStackHelper stack;
    stack.SetRoutingHelper(globalhelper);
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
}

void
RandomEcmpTestCase::CheckRoutes(Ptr<Ipv4GlobalRouting>& globalroutingprotocol,
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
RandomEcmpTestCase::IncrementCount(Ptr<Ipv4Route>& route)
{
    if (route->GetGateway() == Ipv4Address("10.1.1.2"))
    {
        route1++;
    }
    else if (route->GetGateway() == Ipv4Address("10.1.2.2"))
    {
        route2++;
    }
}

void
RandomEcmpTestCase::BuildLsa()
{
    // we manually create the link state database, we could have used BuildRoutingTables() but this
    // way testing the GlobalRouteManagerImpl without the involvement of GLobalRouter makes it
    // easier to debug each individually

    // router 0
    auto lr0 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.1",
                                           "10.1.1.1",
                                           1);
    auto lr1 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.1.2",
                                           "255.255.255.252",
                                           1);

    auto lr2 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.2",
                                           "10.1.2.1",
                                           1);
    auto lr3 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.2.2",
                                           "255.255.255.252",
                                           1);

    auto lsa0 = new GlobalRoutingLSA();
    lsa0->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa0->SetStatus(GlobalRoutingLSA::LSA_SPF_NOT_EXPLORED);
    lsa0->SetLinkStateId("0.0.0.0");
    lsa0->SetAdvertisingRouter("0.0.0.0");
    lsa0->SetNode(nodes.Get(0));
    lsa0->AddLinkRecord(lr0);
    lsa0->AddLinkRecord(lr1);
    lsa0->AddLinkRecord(lr2);
    lsa0->AddLinkRecord(lr3);
    m_lsas.push_back(lsa0);

    // router 1
    auto lr4 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.0",
                                           "10.1.1.2",
                                           1);
    auto lr5 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.1.1",
                                           "255.255.255.252",
                                           1);

    auto lr6 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.3",
                                           "10.1.3.1",
                                           1);
    auto lr7 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.3.2",
                                           "255.255.255.252",
                                           1);

    auto lsa1 = new GlobalRoutingLSA();
    lsa1->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa1->SetStatus(GlobalRoutingLSA::LSA_SPF_NOT_EXPLORED);
    lsa1->SetLinkStateId("0.0.0.1");
    lsa1->SetAdvertisingRouter("0.0.0.1");
    lsa1->SetNode(nodes.Get(1));
    lsa1->AddLinkRecord(lr4);
    lsa1->AddLinkRecord(lr5);
    lsa1->AddLinkRecord(lr6);
    lsa1->AddLinkRecord(lr7);
    m_lsas.push_back(lsa1);

    // router 2
    auto lr8 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                           "0.0.0.0",
                                           "10.1.2.2",
                                           1);
    auto lr9 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                           "10.1.2.1",
                                           "255.255.255.252",
                                           1);
    auto lr10 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                            "0.0.0.3",
                                            "10.1.4.1",
                                            1);
    auto lr11 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                            "10.1.4.2",
                                            "255.255.255.252",
                                            1);

    auto lsa2 = new GlobalRoutingLSA();
    lsa2->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa2->SetStatus(GlobalRoutingLSA::LSA_SPF_NOT_EXPLORED);
    lsa2->SetLinkStateId("0.0.0.2");
    lsa2->SetAdvertisingRouter("0.0.0.2");
    lsa2->SetNode(nodes.Get(2));
    lsa2->AddLinkRecord(lr8);
    lsa2->AddLinkRecord(lr9);
    lsa2->AddLinkRecord(lr10);
    lsa2->AddLinkRecord(lr11);
    m_lsas.push_back(lsa2);

    // router 3
    auto lr12 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                            "0.0.0.1",
                                            "10.1.3.2",
                                            1);
    auto lr13 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                            "10.1.3.1",
                                            "255.255.255.252",
                                            1);
    auto lr14 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::PointToPoint,
                                            "0.0.0.2",
                                            "10.1.4.2",
                                            1);
    auto lr15 = new GlobalRoutingLinkRecord(GlobalRoutingLinkRecord::StubNetwork,
                                            "10.1.4.1",
                                            "255.255.255.252",
                                            1);

    auto lsa3 = new GlobalRoutingLSA();
    lsa3->SetLSType(GlobalRoutingLSA::RouterLSA);
    lsa3->SetStatus(GlobalRoutingLSA::LSA_SPF_NOT_EXPLORED);
    lsa3->SetLinkStateId("0.0.0.3");
    lsa3->SetAdvertisingRouter("0.0.0.3");
    lsa3->SetNode(nodes.Get(3));
    lsa3->AddLinkRecord(lr12);
    lsa3->AddLinkRecord(lr13);
    lsa3->AddLinkRecord(lr14);
    lsa3->AddLinkRecord(lr15);
    m_lsas.push_back(lsa3);
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

    BuildLsa();

    // insert the LSAs into the GlobalRouteManagerLSDB
    auto srmlsdb = new GlobalRouteManagerLSDB();
    srmlsdb->Insert(m_lsas[0]->GetLinkStateId(), m_lsas[0]);
    srmlsdb->Insert(m_lsas[1]->GetLinkStateId(), m_lsas[1]);
    srmlsdb->Insert(m_lsas[2]->GetLinkStateId(), m_lsas[2]);
    srmlsdb->Insert(m_lsas[3]->GetLinkStateId(), m_lsas[3]);

    // create the GlobalRouteManagerImpl
    auto srm = new GlobalRouteManagerImpl();
    srm->DebugUseLsdb(srmlsdb);

    // we manually call the DebugSPFCalculate to fill the routing tables for node 0
    srm->DebugSPFCalculate(m_lsas[0]->GetLinkStateId());

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
    CheckRoutes(globalRouting0, expectedDests, expectedNextHops);

    // Test 2: Check that the equal cost routes are being used at least once.
    // we need to call RouteOutput() at node 0 and check the output
    // route from the table. The thing to check here is that different equal cost routes are
    // returned across different calls to this method.

    Socket::SocketErrno errno_;
    Ptr<NetDevice> oif(nullptr);
    Ptr<Packet> packet = Create<Packet>();
    Ipv4Header ipHeader;
    ipHeader.SetSource(Ipv4Address("10.1.1.1"));
    ipHeader.SetDestination(Ipv4Address("10.1.4.2"));

    for (uint32_t i = 0; i < 10; i++)
    {
        Ptr<Ipv4Route> route = globalRouting0->RouteOutput(packet, ipHeader, oif, errno_);
        IncrementCount(route);
    }

    Simulator::Run();

    NS_TEST_ASSERT_MSG_GT_OR_EQ(
        route1,
        1,
        "The routing path for node 0 to node 3 does not match the expected output, "
        "Equal Cost MultiPath (ECMP) routes are not being used correctly");
    NS_TEST_ASSERT_MSG_GT_OR_EQ(
        route2,
        1,
        "The routing path for node 0 to node 3 does not match the expected output, "
        "Equal Cost MultiPath (ECMP) routes are not being used correctly");

    // end of test--------------------

    Simulator::Stop(Seconds(5));

    Simulator::Destroy();

    RngSeedManager::SetSeed(oldseed);
    RngSeedManager::SetRun(oldrun);
    // This delete clears the srm, which deletes the LSDB, which clears
    // all of the LSAs, which each destroys the attached LinkRecords.
    delete srm;
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
