/*
 * Copyright (c) 2004 Francisco J. Ros
 * Copyright (c) 2007 INESC Porto
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors: Francisco J. Ros  <fjrm@dif.um.es>
 *          Gustavo J. A. M. Carneiro <gjc@inescporto.pt>
 */

#include "ns3/ipv4-header.h"
#include "ns3/olsr-repositories.h"
#include "ns3/olsr-routing-protocol.h"
#include "ns3/test.h"

/**
 * \ingroup olsr
 * \defgroup olsr-test olsr module tests
 */

using namespace ns3;
using namespace olsr;

/**
 * \ingroup olsr-test
 * \ingroup tests
 *
 * Testcase for MPR computation mechanism
 */
class OlsrMprTestCase : public TestCase
{
  public:
    OlsrMprTestCase();
    ~OlsrMprTestCase() override;
    void DoRun() override;
};

OlsrMprTestCase::OlsrMprTestCase()
    : TestCase("Check OLSR MPR computing mechanism")
{
}

OlsrMprTestCase::~OlsrMprTestCase()
{
}

void
OlsrMprTestCase::DoRun()
{
    Ptr<RoutingProtocol> protocol = CreateObject<RoutingProtocol>();
    protocol->m_mainAddress = Ipv4Address("10.0.0.1");
    OlsrState& state = protocol->m_state;

    /*
     *  1 -- 2
     *  |    |
     *  3 -- 4
     *
     * Node 1 must select only one MPR (2 or 3, doesn't matter)
     */
    NeighborTuple neighbor;
    neighbor.status = NeighborTuple::STATUS_SYM;
    neighbor.willingness = Willingness::DEFAULT;
    neighbor.neighborMainAddr = Ipv4Address("10.0.0.2");
    protocol->m_state.InsertNeighborTuple(neighbor);
    neighbor.neighborMainAddr = Ipv4Address("10.0.0.3");
    protocol->m_state.InsertNeighborTuple(neighbor);
    TwoHopNeighborTuple tuple;
    tuple.expirationTime = Seconds(3600);
    tuple.neighborMainAddr = Ipv4Address("10.0.0.2");
    tuple.twoHopNeighborAddr = Ipv4Address("10.0.0.4");
    protocol->m_state.InsertTwoHopNeighborTuple(tuple);
    tuple.neighborMainAddr = Ipv4Address("10.0.0.3");
    tuple.twoHopNeighborAddr = Ipv4Address("10.0.0.4");
    protocol->m_state.InsertTwoHopNeighborTuple(tuple);

    protocol->MprComputation();
    NS_TEST_EXPECT_MSG_EQ(state.GetMprSet().size(), 1, "An only address must be chosen.");
    /*
     *  1 -- 2 -- 5
     *  |    |
     *  3 -- 4
     *
     * Node 1 must select node 2 as MPR.
     */
    tuple.neighborMainAddr = Ipv4Address("10.0.0.2");
    tuple.twoHopNeighborAddr = Ipv4Address("10.0.0.5");
    protocol->m_state.InsertTwoHopNeighborTuple(tuple);

    protocol->MprComputation();
    MprSet mpr = state.GetMprSet();
    NS_TEST_EXPECT_MSG_EQ(mpr.size(), 1, "An only address must be chosen.");
    NS_TEST_EXPECT_MSG_EQ((mpr.find("10.0.0.2") != mpr.end()),
                          true,
                          "Node 1 must select node 2 as MPR");
    /*
     *  1 -- 2 -- 5
     *  |    |
     *  3 -- 4
     *  |
     *  6
     *
     * Node 1 must select nodes 2 and 3 as MPRs.
     */
    tuple.neighborMainAddr = Ipv4Address("10.0.0.3");
    tuple.twoHopNeighborAddr = Ipv4Address("10.0.0.6");
    protocol->m_state.InsertTwoHopNeighborTuple(tuple);

    protocol->MprComputation();
    mpr = state.GetMprSet();
    NS_TEST_EXPECT_MSG_EQ(mpr.size(), 2, "An only address must be chosen.");
    NS_TEST_EXPECT_MSG_EQ((mpr.find("10.0.0.2") != mpr.end()),
                          true,
                          "Node 1 must select node 2 as MPR");
    NS_TEST_EXPECT_MSG_EQ((mpr.find("10.0.0.3") != mpr.end()),
                          true,
                          "Node 1 must select node 3 as MPR");
    /*
     *  7 (Willingness::ALWAYS)
     *  |
     *  1 -- 2 -- 5
     *  |    |
     *  3 -- 4
     *  |
     *  6
     *
     * Node 1 must select nodes 2, 3 and 7 (since it is Willingness::ALWAYS) as MPRs.
     */
    neighbor.willingness = olsr::Willingness::ALWAYS;
    neighbor.neighborMainAddr = Ipv4Address("10.0.0.7");
    protocol->m_state.InsertNeighborTuple(neighbor);

    protocol->MprComputation();
    mpr = state.GetMprSet();
    NS_TEST_EXPECT_MSG_EQ(mpr.size(), 3, "An only address must be chosen.");
    NS_TEST_EXPECT_MSG_EQ((mpr.find("10.0.0.7") != mpr.end()),
                          true,
                          "Node 1 must select node 7 as MPR");
    /*
     *                        7 <- Willingness::ALWAYS
     *                        |
     *              9 -- 8 -- 1 -- 2 -- 5
     *                        |    |
     *                   ^    3 -- 4
     *                   |    |
     *   Willingness::NEVER   6
     *
     * Node 1 must select nodes 2, 3 and 7 (since it is Willingness::ALWAYS) as MPRs.
     * Node 1 must NOT select node 8 as MPR since it is Willingness::NEVER
     */
    neighbor.willingness = Willingness::NEVER;
    neighbor.neighborMainAddr = Ipv4Address("10.0.0.8");
    protocol->m_state.InsertNeighborTuple(neighbor);
    tuple.neighborMainAddr = Ipv4Address("10.0.0.8");
    tuple.twoHopNeighborAddr = Ipv4Address("10.0.0.9");
    protocol->m_state.InsertTwoHopNeighborTuple(tuple);

    protocol->MprComputation();
    mpr = state.GetMprSet();
    NS_TEST_EXPECT_MSG_EQ(mpr.size(), 3, "An only address must be chosen.");
    NS_TEST_EXPECT_MSG_EQ((mpr.find("10.0.0.9") == mpr.end()),
                          true,
                          "Node 1 must NOT select node 8 as MPR");
}

/**
 * \ingroup olsr-test
 * \ingroup tests
 *
 * OLSR protocol test suite
 */
class OlsrProtocolTestSuite : public TestSuite
{
  public:
    OlsrProtocolTestSuite();
};

OlsrProtocolTestSuite::OlsrProtocolTestSuite()
    : TestSuite("routing-olsr", UNIT)
{
    AddTestCase(new OlsrMprTestCase(), TestCase::QUICK);
}

static OlsrProtocolTestSuite g_olsrProtocolTestSuite; //!< Static variable for test initialization
