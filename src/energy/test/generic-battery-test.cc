/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 * Based on the works of Andrea Sacco (2010)
 */

#include "ns3/core-module.h"
#include "ns3/energy-module.h"

using namespace ns3;
using namespace ns3::energy;

/**
 * @ingroup energy-tests
 *
 * @brief Discharge a battery test
 */
class DischargeBatteryTestCase : public TestCase
{
  public:
    DischargeBatteryTestCase();

    void DoRun() override;

    Ptr<Node> m_node; //!< Node to aggregate the source to.
};

DischargeBatteryTestCase::DischargeBatteryTestCase()
    : TestCase("Discharge a Li-Ion Panasonic CGR18650DA battery")
{
}

void
DischargeBatteryTestCase::DoRun()
{
    // This test demonstrates that the battery reach its cutoff voltage in a little less than 1
    // hour. When discharged with a constant current of 2.33 A (Equivalent to 1C).
    // Note: The cutoff voltage is only reached within this time for the specified battery
    // (PANASONIC CGR18650DA Li-Ion).

    Ptr<Node> node = CreateObject<Node>();
    GenericBatteryModelHelper batteryHelper;
    Ptr<GenericBatteryModel> batteryModel =
        DynamicCast<GenericBatteryModel>(batteryHelper.Install(node, PANASONIC_CGR18650DA_LION));

    Ptr<SimpleDeviceEnergyModel> consumptionEnergyModel = CreateObject<SimpleDeviceEnergyModel>();
    consumptionEnergyModel->SetEnergySource(batteryModel);
    batteryModel->AppendDeviceEnergyModel(consumptionEnergyModel);
    consumptionEnergyModel->SetNode(node);

    // Needed to initialize battery model
    batteryModel->UpdateEnergySource();

    // Discharge the battery with a constant current of 2.33 A (1C)
    consumptionEnergyModel->SetCurrentA(2.33);

    Simulator::Stop(Seconds(3459));
    Simulator::Run();

    NS_TEST_ASSERT_MSG_EQ_TOL(batteryModel->GetSupplyVoltage(),
                              3.0,
                              1.0e-2,
                              "Cutoff voltage not reached");
    node->Dispose();
    consumptionEnergyModel->Dispose();
    batteryModel->Dispose();
    Simulator::Destroy();
}

/**
 * @ingroup energy-tests
 *
 * @brief Generic battery TestSuite
 */
class GenericBatteryTestSuite : public TestSuite
{
  public:
    GenericBatteryTestSuite();
};

GenericBatteryTestSuite::GenericBatteryTestSuite()
    : TestSuite("generic-battery-test", Type::UNIT)
{
    AddTestCase(new DischargeBatteryTestCase, TestCase::Duration::QUICK);
}

/// create an instance of the test suite
static GenericBatteryTestSuite g_genericBatteryTestSuite;
