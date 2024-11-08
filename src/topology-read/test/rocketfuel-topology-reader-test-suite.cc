/*
 * Copyright (c) 2010 Hajime Tazaki
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Hajime Tazaki (tazaki@sfc.wide.ad.jp)
 */

//-----------------------------------------------------------------------------
// Unit tests
//-----------------------------------------------------------------------------

#include "ns3/abort.h"
#include "ns3/attribute.h"
#include "ns3/log.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/rocketfuel-topology-reader.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

/**
 * @defgroup topology-test Topology module tests
 * @ingroup topology
 * @ingroup tests
 */

/**
 * @file
 * @ingroup topology-test
 * ns3::RockefuelTopologyReader test suite.
 */

/**
 * @ingroup topology-test

 * @brief Rocketfuel Topology Reader Weights Test
 */
class RocketfuelTopologyReaderWeightsTest : public TestCase
{
  public:
    RocketfuelTopologyReaderWeightsTest();

  private:
    void DoRun() override;
};

RocketfuelTopologyReaderWeightsTest::RocketfuelTopologyReaderWeightsTest()
    : TestCase("RocketfuelTopologyReaderWeightsTest")
{
}

void
RocketfuelTopologyReaderWeightsTest::DoRun()
{
    Ptr<RocketfuelTopologyReader> inFile;
    NodeContainer nodes;

    std::string input("./src/topology-read/examples/RocketFuel_toposample_1239_weights.txt");

    inFile = CreateObject<RocketfuelTopologyReader>();
    inFile->SetFileName(input);

    if (inFile)
    {
        nodes = inFile->Read();
    }

    NS_TEST_ASSERT_MSG_NE(nodes.GetN(), 0, "Problems reading node information the topology file..");

    NS_TEST_ASSERT_MSG_NE(inFile->LinksSize(), 0, "Problems reading the topology file.");

    NS_TEST_EXPECT_MSG_EQ(nodes.GetN(), 315, "nodes");
    NS_TEST_EXPECT_MSG_EQ(inFile->LinksSize(), 972, "links");
    Simulator::Destroy();
}

/**
 * @ingroup topology-test
 * @brief Rocketfuel Topology Reader Maps Test
 */
class RocketfuelTopologyReaderMapsTest : public TestCase
{
  public:
    RocketfuelTopologyReaderMapsTest();

  private:
    void DoRun() override;
};

RocketfuelTopologyReaderMapsTest::RocketfuelTopologyReaderMapsTest()
    : TestCase("RocketfuelTopologyReaderMapsTest")
{
}

void
RocketfuelTopologyReaderMapsTest::DoRun()
{
    Ptr<RocketfuelTopologyReader> inFile;
    NodeContainer nodes;

    std::string input("./src/topology-read/examples/RocketFuel_sample_4755.r0.cch_maps.txt");

    inFile = CreateObject<RocketfuelTopologyReader>();
    inFile->SetFileName(input);

    if (inFile)
    {
        nodes = inFile->Read();
    }

    NS_TEST_ASSERT_MSG_NE(nodes.GetN(), 0, "Problems reading node information the topology file..");

    NS_TEST_ASSERT_MSG_NE(inFile->LinksSize(), 0, "Problems reading the topology file.");

    NS_TEST_EXPECT_MSG_EQ(nodes.GetN(), 12, "nodes");
    NS_TEST_EXPECT_MSG_EQ(inFile->LinksSize(), 24, "links");
    Simulator::Destroy();
}

/**
 * @ingroup topology-test
 *
 * @brief Rocketfuel Topology Reader TestSuite
 */
class RocketfuelTopologyReaderTestSuite : public TestSuite
{
  public:
    RocketfuelTopologyReaderTestSuite();

  private:
};

RocketfuelTopologyReaderTestSuite::RocketfuelTopologyReaderTestSuite()
    : TestSuite("rocketfuel-topology-reader", Type::UNIT)
{
    AddTestCase(new RocketfuelTopologyReaderWeightsTest(), TestCase::Duration::QUICK);
    AddTestCase(new RocketfuelTopologyReaderMapsTest(), TestCase::Duration::QUICK);
}

/**
 * @ingroup topology-test
 * Static variable for test initialization
 */
static RocketfuelTopologyReaderTestSuite g_rocketfuelTopologyReaderTestSuite;
