/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#include "ns3/li-ion-energy-source.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;
using namespace ns3::energy;

// NS_DEPRECATED_3_43() - tag for future removal
// LiIonEnergySource was deprecated in commit
// https://gitlab.com/nsnam/ns-3-dev/-/commit/086913b0
//
// The new battery model is illustrated in
// `src/energy/examples/generic-battery-discharge-example.cc`
//
// A GenericBatteryModelTestCase is being developed in MR 2181
// https://gitlab.com/nsnam/ns-3-dev/-/merge_requests/2181
// which will be in
// `src/energy/test/generic-battery-model-test.cc`

NS_LOG_COMPONENT_DEFINE("LiIonEnergySourceTestSuite");

/**
 * @ingroup energy-tests
 *
 * @brief LiIon battery Test
 */
class LiIonEnergyTestCase : public TestCase
{
  public:
    LiIonEnergyTestCase();
    ~LiIonEnergyTestCase() override;

    void DoRun() override;

    Ptr<Node> m_node; //!< Node to aggreagte the source to.
};

LiIonEnergyTestCase::LiIonEnergyTestCase()
    : TestCase("Li-Ion energy source test case")
{
}

LiIonEnergyTestCase::~LiIonEnergyTestCase()
{
    m_node = nullptr;
}

void
LiIonEnergyTestCase::DoRun()
{
    m_node = CreateObject<Node>();

    Ptr<SimpleDeviceEnergyModel> sem = CreateObject<SimpleDeviceEnergyModel>();

    NS_WARNING_PUSH_DEPRECATED;
    Ptr<LiIonEnergySource> es = CreateObject<LiIonEnergySource>();
    NS_WARNING_POP;

    es->SetNode(m_node);
    sem->SetEnergySource(es);
    es->AppendDeviceEnergyModel(sem);
    m_node->AggregateObject(es);

    Time now = Simulator::Now();

    // discharge at 2.33 A for 1700 seconds
    sem->SetCurrentA(2.33);
    now += Seconds(1701);

    Simulator::Stop(now);
    Simulator::Run();
    Simulator::Destroy();

    NS_TEST_ASSERT_MSG_EQ_TOL(es->GetSupplyVoltage(), 3.6, 1.0e-3, "Incorrect consumed energy!");
}

/**
 * @ingroup energy-tests
 *
 * @brief LiIon battery TestSuite
 */
class LiIonEnergySourceTestSuite : public TestSuite
{
  public:
    LiIonEnergySourceTestSuite();
};

LiIonEnergySourceTestSuite::LiIonEnergySourceTestSuite()
    : TestSuite("li-ion-energy-source", Type::UNIT)
{
    AddTestCase(new LiIonEnergyTestCase, TestCase::Duration::QUICK);
}

/// create an instance of the test suite
static LiIonEnergySourceTestSuite g_liIonEnergySourceTestSuite;
