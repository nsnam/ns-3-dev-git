/*
 * Copyright (c) 2023 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 */

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/building.h"
#include "ns3/buildings-channel-condition-model.h"
#include "ns3/channel-condition-model.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/mobility-building-info.h"
#include "ns3/mobility-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/three-gpp-propagation-loss-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BuildingsPenetrationLossesTest");

/**
 * \ingroup propagation-tests
 *
 * Test case for the 3GPP channel O2I building penetration losses.
 * It considers pre-determined scenarios and based on the outdoor/indoor
 * condition (O2O/O2I) checks whether the calculated received power
 * is aligned with the calculation in 3GPP TR 38.901.
 *
 * For O2O condition the calculation is done according to Table 7.4.1-1
 * and we check if the calculated received power is equal to the expected
 * value.
 * For the O2I condition, the calculation is done considering the building
 * penetration losses based on the material of the building walls.
 * In this case, we check if the calculated received power is less than the
 * received value (thus ensure that losses have been included).
 */

class BuildingsPenetrationLossesTestCase : public TestCase
{
  public:
    /**
     * Constructor
     */
    BuildingsPenetrationLossesTestCase();

    /**
     * Destructor
     */
    ~BuildingsPenetrationLossesTestCase() override;

  private:
    /**
     * Builds the simulation scenario and perform the tests
     */
    void DoRun() override;

    /**
     * Struct containing the parameters for each test
     */
    struct TestVector
    {
        Vector m_positionA; //!< the position of the first node
        Vector m_positionB; //!< the position of the second node
        double m_frequency; //!< carrier frequency in Hz
        TypeId m_condModel; //!< the type ID of the channel condition model to be used
        TypeId m_propModel; //!< the type ID of the propagation loss model to be used
        double m_pt;        //!< transmitted power in dBm
        double m_pr;        //!< received power in dBm
    };

    TestVectors<TestVector> m_testVectors;         //!< array containing all the test vectors
    Ptr<ThreeGppPropagationLossModel> m_propModel; //!< the propagation loss model
    double m_tolerance;                            //!< tolerance
};

BuildingsPenetrationLossesTestCase::BuildingsPenetrationLossesTestCase()
    : TestCase("Test case for BuildingsPenetrationLosses"),
      m_testVectors(),
      m_tolerance(2e-3)
{
}

BuildingsPenetrationLossesTestCase::~BuildingsPenetrationLossesTestCase()
{
}

void
BuildingsPenetrationLossesTestCase::DoRun()
{
    // create the test vector
    TestVector testVector;

    // tests for the RMa scenario
    testVector.m_positionA = Vector(0, 0, 35.0); // O2I case
    testVector.m_positionB = Vector(10, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppRmaPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -77.3784;
    m_testVectors.Add(testVector);

    testVector.m_positionA = Vector(0, 0, 35.0); // O2I case
    testVector.m_positionB = Vector(100, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppRmaPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -87.2965;
    m_testVectors.Add(testVector);

    testVector.m_positionA = Vector(0, 0, 35.0); // O2O case
    testVector.m_positionB = Vector(1000, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppRmaPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -108.5577;
    m_testVectors.Add(testVector);

    // tests for the UMa scenario
    testVector.m_positionA = Vector(0, 0, 25.0); // O2I case
    testVector.m_positionB = Vector(10, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppUmaPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -72.9380;
    m_testVectors.Add(testVector);

    testVector.m_positionA = Vector(0, 0, 25.0); // O2I case
    testVector.m_positionB = Vector(100, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppUmaPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -86.2362;
    m_testVectors.Add(testVector);

    testVector.m_positionA = Vector(0, 0, 25.0); // O2O case
    testVector.m_positionB = Vector(1000, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppUmaPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -109.7252;
    m_testVectors.Add(testVector);

    // tests for the UMi scenario
    testVector.m_positionA = Vector(0, 0, 10.0); // O2I case
    testVector.m_positionB = Vector(10, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppUmiStreetCanyonPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -69.8591;
    m_testVectors.Add(testVector);

    testVector.m_positionA = Vector(0, 0, 10.0); // O2I case
    testVector.m_positionB = Vector(100, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppUmiStreetCanyonPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -88.4122;
    m_testVectors.Add(testVector);

    testVector.m_positionA = Vector(0, 0, 10.0); // O2O case
    testVector.m_positionB = Vector(1000, 0, 1.5);
    testVector.m_frequency = 5.0e9;
    testVector.m_propModel = ThreeGppUmiStreetCanyonPropagationLossModel::GetTypeId();
    testVector.m_pt = 0.0;
    testVector.m_pr = -119.3114;
    m_testVectors.Add(testVector);

    // create the factory for the propagation loss model
    ObjectFactory propModelFactory;

    Ptr<Building> building = Create<Building>();
    building->SetExtWallsType(Building::ExtWallsType_t::ConcreteWithWindows);
    building->SetNRoomsX(1);
    building->SetNRoomsY(1);
    building->SetNFloors(2);
    building->SetBoundaries(Box(0.0, 100.0, 0.0, 10.0, 0.0, 5.0));

    // create the two nodes
    NodeContainer nodes;
    nodes.Create(2);

    // create the mobility models
    Ptr<MobilityModel> a = CreateObject<ConstantPositionMobilityModel>();
    Ptr<MobilityModel> b = CreateObject<ConstantPositionMobilityModel>();

    // aggregate the nodes and the mobility models
    nodes.Get(0)->AggregateObject(a);
    nodes.Get(1)->AggregateObject(b);

    Ptr<MobilityBuildingInfo> buildingInfoA = CreateObject<MobilityBuildingInfo>();
    Ptr<MobilityBuildingInfo> buildingInfoB = CreateObject<MobilityBuildingInfo>();
    a->AggregateObject(buildingInfoA); // operation usually done by BuildingsHelper::Install
    buildingInfoA->MakeConsistent(a);

    b->AggregateObject(buildingInfoB); // operation usually done by BuildingsHelper::Install
    buildingInfoB->MakeConsistent(b);

    Ptr<ChannelConditionModel> condModel = CreateObject<BuildingsChannelConditionModel>();

    for (std::size_t i = 0; i < m_testVectors.GetN(); i++)
    {
        TestVector testVector = m_testVectors.Get(i);
        a->SetPosition(testVector.m_positionA);
        b->SetPosition(testVector.m_positionB);

        Ptr<ChannelCondition> cond = condModel->GetChannelCondition(a, b);

        propModelFactory.SetTypeId(testVector.m_propModel);
        m_propModel = propModelFactory.Create<ThreeGppPropagationLossModel>();
        m_propModel->SetAttribute("Frequency", DoubleValue(testVector.m_frequency));
        m_propModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
        m_propModel->SetChannelConditionModel(condModel);

        bool isAIndoor = buildingInfoA->IsIndoor();
        bool isBIndoor = buildingInfoB->IsIndoor();

        if (!isAIndoor && !isBIndoor) // a and b are outdoor
        {
            cond->SetLosCondition(ChannelCondition::LosConditionValue::LOS);
            cond->SetO2iCondition(ChannelCondition::O2iConditionValue::O2O);

            // check rcv power to be equal to the calculated value without losses
            NS_TEST_EXPECT_MSG_EQ_TOL(m_propModel->CalcRxPower(testVector.m_pt, a, b),
                                      testVector.m_pr,
                                      m_tolerance,
                                      "rcv power is not equal expected value");
        }
        else
        {
            cond->SetLosCondition(ChannelCondition::LosConditionValue::LOS);
            cond->SetO2iCondition(ChannelCondition::O2iConditionValue::O2I);
            cond->SetO2iLowHighCondition(ChannelCondition::O2iLowHighConditionValue::LOW);

            // check rcv power to be lower than the calculated without losses
            NS_TEST_EXPECT_MSG_LT(m_propModel->CalcRxPower(testVector.m_pt, a, b),
                                  testVector.m_pr,
                                  "rcv power is not less than calculated value");
        }
        m_propModel = nullptr;
    }
    Simulator::Destroy();
}

/**
 * \ingroup propagation-tests
 *
 * Test suite for the buildings penetration losses
 */
class BuildingsPenetrationLossesTestSuite : public TestSuite
{
  public:
    BuildingsPenetrationLossesTestSuite();
};

BuildingsPenetrationLossesTestSuite::BuildingsPenetrationLossesTestSuite()
    : TestSuite("buildings-penetration-losses", Type::UNIT)
{
    AddTestCase(new BuildingsPenetrationLossesTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static BuildingsPenetrationLossesTestSuite g_buildingsPenetrationLossesTestSuite;
