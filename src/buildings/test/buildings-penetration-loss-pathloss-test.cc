/*
 * Copyright (c) 2023 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
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
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/three-gpp-propagation-loss-model.h"

#include <numeric>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("BuildingsPenetrationLossesTest");

using O2ILH = ChannelCondition::O2iLowHighConditionValue;

/**
 * @ingroup propagation-tests
 *
 * Test case for the 3GPP channel building and vehicular O2I penetration losses.
 * It considers pre-determined scenarios and based on the outdoor/indoor
 * condition (O2O/O2I) checks whether the calculated received power
 * is aligned with the calculation in 3GPP TR 38.901.
 *
 * For O2O condition the calculation is done according to Table 7.4.1-1
 * and we check if the calculated received power is equal to the expected
 * value. O2O condition is also tested when unblocked (LOS),
 * or blocked (NLOS) by a building.

 * For the O2I condition with frequencies equal or above 6GHz,
 * the calculation is done considering the building penetration losses
 * based on the material of the building walls, as specified
 * in Table 7.4.3-1 and Table 7.4.3-2.
 * For the O2I condition with frequencies below 6GHz, the calculation
 * is done considering the backward compatibility model in Table 7.4.3-3.
 * In both cases, we check if the calculated received power is within
 * the expected normal distribution.
 *
 * All cases are tested with one node at 0 or 30 km/h, representing pedestrian or vehicles.
 * In case a vehicle is part of a channel link, its O2I losses are applied to the channel,
 * following procedure in 3GPP TR 38901 Section 7.4.3.2.
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
        Vector m_positionA;      //!< the position of the first node
        Vector m_positionB;      //!< the position of the second node
        double m_ueVelocity;     //!< velocity in km/h
        double m_frequency;      //!< carrier frequency in Hz
        TypeId m_propModel;      //!< the type ID of the propagation loss model to be used
        O2ILH m_o2iLossType;     //!< type of O2I loss (low/high)
        double m_expectedMean;   //!< expected mean in dBm
        double m_expectedStddev; //!< expected deviation in dBm
    };

    std::vector<TestVector> m_testVectors;         //!< array containing all the test vectors
    Ptr<ThreeGppPropagationLossModel> m_propModel; //!< the propagation loss model
};

BuildingsPenetrationLossesTestCase::BuildingsPenetrationLossesTestCase()
    : TestCase("Test case for BuildingsPenetrationLosses"),
      m_testVectors()
{
}

BuildingsPenetrationLossesTestCase::~BuildingsPenetrationLossesTestCase()
{
}

void
BuildingsPenetrationLossesTestCase::DoRun()
{
    const auto UMi = ThreeGppUmiStreetCanyonPropagationLossModel::GetTypeId();
    const auto UMa = ThreeGppUmaPropagationLossModel::GetTypeId();
    const auto RMa = ThreeGppRmaPropagationLossModel::GetTypeId();
    // create the test vector
    // clang-format off
    m_testVectors = {
        // PosA, PosB, B velocity, frequency, propagation model, O2I loss type, channel condition, mean and standard deviation of expected gaussian
        // 6GHz, pedestrian, Low O2I loss
        {{0, 0, 35.0}, {  10, 0, 1.5},  0, 6e9, RMa,  O2ILH::LOW,  -93.0, 4.5}, // RMa O2I
        {{0, 0, 35.0}, { 100, 0, 1.5},  0, 6e9, RMa,  O2ILH::LOW, -112.0, 4.5}, // RMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 6e9, RMa,  O2ILH::LOW,  -97.0, 0.1}, // RMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 6e9, RMa,  O2ILH::LOW, -110.0, 0.1}, // RMa O2O blocked
        {{0, 0, 25.0}, {  10, 0, 1.5},  0, 6e9, UMa,  O2ILH::LOW, -101.0, 5.3}, // UMa O2I
        {{0, 0, 25.0}, { 100, 0, 1.5},  0, 6e9, UMa,  O2ILH::LOW, -125.0, 5.3}, // UMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 6e9, UMa,  O2ILH::LOW,  -96.0, 0.1}, // UMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 6e9, UMa,  O2ILH::LOW, -117.0, 0.1}, // UMa O2O blocked
        {{0, 0, 10.0}, {  10, 0, 1.5},  0, 6e9, UMi,  O2ILH::LOW,  -96.0, 5.3}, // UMi O2I
        {{0, 0, 10.0}, { 100, 0, 1.5},  0, 6e9, UMi,  O2ILH::LOW, -127.0, 5.0}, // UMi O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 6e9, UMi,  O2ILH::LOW,  -99.0, 0.1}, // UMi O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 6e9, UMi,  O2ILH::LOW, -119.0, 0.1}, // UMi O2O blocked
        // 6GHz, pedestrian, High O2I loss
        {{0, 0, 35.0}, {  10, 0, 1.5},  0, 6e9, RMa, O2ILH::HIGH,  -93.0, 4.5}, // RMa O2I
        {{0, 0, 35.0}, { 100, 0, 1.5},  0, 6e9, RMa, O2ILH::HIGH, -112.0, 4.5}, // RMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 6e9, RMa, O2ILH::HIGH,  -97.0, 0.1}, // RMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 6e9, RMa, O2ILH::HIGH, -110.0, 0.1}, // RMa O2O blocked
        {{0, 0, 25.0}, {  10, 0, 1.5},  0, 6e9, UMa, O2ILH::HIGH, -101.0, 5.3}, // UMa O2I
        {{0, 0, 25.0}, { 100, 0, 1.5},  0, 6e9, UMa, O2ILH::HIGH, -125.0, 5.3}, // UMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 6e9, UMa, O2ILH::HIGH,  -96.0, 0.1}, // UMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 6e9, UMa, O2ILH::HIGH, -117.0, 0.1}, // UMa O2O blocked
        {{0, 0, 10.0}, {  10, 0, 1.5},  0, 6e9, UMi, O2ILH::HIGH,  -95.0, 5.3}, // UMi O2I
        {{0, 0, 10.0}, { 100, 0, 1.5},  0, 6e9, UMi, O2ILH::HIGH, -127.0, 5.3}, // UMi O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 6e9, UMi, O2ILH::HIGH,  -99.0, 0.1}, // UMi O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 6e9, UMi, O2ILH::HIGH, -119.0, 0.1}, // UMi O2O blocked
        // 6GHz, vehicles, High O2I loss
        {{0, 0, 35.0}, {  10, 0, 1.5}, 30, 6e9, RMa, O2ILH::HIGH,  -94.0, 4.5}, // RMa O2I
        {{0, 0, 35.0}, { 100, 0, 1.5}, 30, 6e9, RMa, O2ILH::HIGH, -112.0, 4.5}, // RMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5}, 30, 6e9, RMa, O2ILH::HIGH,  -98.0, 5.3}, // RMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5}, 30, 6e9, RMa, O2ILH::HIGH, -110.0, 5.3}, // RMa O2O blocked
        {{0, 0, 25.0}, {  10, 0, 1.5}, 30, 6e9, UMa, O2ILH::HIGH, -101.0, 5.3}, // UMa O2I
        {{0, 0, 25.0}, { 100, 0, 1.5}, 30, 6e9, UMa, O2ILH::HIGH, -125.0, 5.3}, // UMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5}, 30, 6e9, UMa, O2ILH::HIGH,  -96.0, 5.3}, // UMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5}, 30, 6e9, UMa, O2ILH::HIGH, -117.0, 5.3}, // UMa O2O blocked
        {{0, 0, 10.0}, {  10, 0, 1.5}, 30, 6e9, UMi, O2ILH::HIGH,  -95.0, 5.3}, // UMi O2I
        {{0, 0, 10.0}, { 100, 0, 1.5}, 30, 6e9, UMi, O2ILH::HIGH, -127.0, 5.3}, // UMi O2I
        {{0, 0, 10.0}, { 50, 15, 1.5}, 30, 6e9, UMi, O2ILH::HIGH, -100.0, 5.3}, // UMi O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5}, 30, 6e9, UMi, O2ILH::HIGH, -119.0, 5.3}, // UMi O2O blocked
        // Sub-6GHz, pedestrian, legacy O2I
        {{0, 0, 35.0}, {  10, 0, 1.5},  0, 5e9, RMa, O2ILH::HIGH,  -97.0, 0.1}, // RMa O2I
        {{0, 0, 35.0}, { 100, 0, 1.5},  0, 5e9, RMa, O2ILH::HIGH, -115.0, 0.1}, // RMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 5e9, RMa, O2ILH::HIGH,  -96.0, 0.1}, // RMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 5e9, RMa, O2ILH::HIGH, -108.0, 0.1}, // RMa O2O blocked
        {{0, 0, 25.0}, {  10, 0, 1.5},  0, 5e9, UMa, O2ILH::HIGH, -108.0, 3.6}, // UMa O2I
        {{0, 0, 25.0}, { 100, 0, 1.5},  0, 5e9, UMa, O2ILH::HIGH, -132.0, 3.6}, // UMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 5e9, UMa, O2ILH::HIGH,  -94.0, 0.1}, // UMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 5e9, UMa, O2ILH::HIGH, -117.0, 0.1}, // UMa O2O blocked
        {{0, 0, 10.0}, {  10, 0, 1.5},  0, 5e9, UMi, O2ILH::HIGH, -103.0, 3.6}, // UMi O2I
        {{0, 0, 10.0}, { 100, 0, 1.5},  0, 5e9, UMi, O2ILH::HIGH, -134.0, 3.6}, // UMi O2I
        {{0, 0, 10.0}, { 50, 15, 1.5},  0, 5e9, UMi, O2ILH::HIGH,  -98.0, 0.1}, // UMi O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5},  0, 5e9, UMi, O2ILH::HIGH, -119.0, 0.1}, // UMi O2O blocked
        // Sub-6GHz, vehicles, legacy O2I
        {{0, 0, 35.0}, {  10, 0, 1.5}, 30, 5e9, RMa, O2ILH::HIGH,  -97.0, 0.1}, // RMa O2I
        {{0, 0, 35.0}, { 100, 0, 1.5}, 30, 5e9, RMa, O2ILH::HIGH, -115.0, 0.1}, // RMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5}, 30, 5e9, RMa, O2ILH::HIGH,  -96.0, 5.0}, // RMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5}, 30, 5e9, RMa, O2ILH::HIGH, -108.0, 5.0}, // RMa O2O blocked
        {{0, 0, 25.0}, {  10, 0, 1.5}, 30, 5e9, UMa, O2ILH::HIGH, -108.0, 3.6}, // UMa O2I
        {{0, 0, 25.0}, { 100, 0, 1.5}, 30, 5e9, UMa, O2ILH::HIGH, -132.0, 3.6}, // UMa O2I
        {{0, 0, 10.0}, { 50, 15, 1.5}, 30, 5e9, UMa, O2ILH::HIGH, -103.0, 5.0}, // UMa O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5}, 30, 5e9, UMa, O2ILH::HIGH, -117.0, 5.0}, // UMa O2O blocked
        {{0, 0, 10.0}, {  10, 0, 1.5}, 30, 5e9, UMi, O2ILH::HIGH, -102.0, 3.6}, // UMi O2I
        {{0, 0, 10.0}, { 100, 0, 1.5}, 30, 5e9, UMi, O2ILH::HIGH, -134.0, 3.6}, // UMi O2I
        {{0, 0, 10.0}, { 50, 15, 1.5}, 30, 5e9, UMi, O2ILH::HIGH,  -98.0, 5.0}, // UMi O2O unblocked
        {{0, 0, 10.0}, {1000, 0, 1.5}, 30, 5e9, UMi, O2ILH::HIGH, -119.0, 5.0}, // UMi O2O blocked
    };
    // clang-format on
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    for (std::size_t i = 0; i < m_testVectors.size(); i++)
    {
        RngSeedManager::ResetNextStreamIndex();
        // create the factory for the propagation loss model
        ObjectFactory propModelFactory;

        auto building = CreateObject<Building>();
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
        Ptr<MobilityModel> b = CreateObject<ConstantVelocityMobilityModel>();
        Ptr<ConstantVelocityMobilityModel> bcv = DynamicCast<ConstantVelocityMobilityModel>(b);

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
        int64_t streamOffset = 0;
        streamOffset += condModel->AssignStreams(streamOffset);

        // Configure test case setup
        const auto& testVector = m_testVectors.at(i);
        a->SetPosition(testVector.m_positionA);
        b->SetPosition(testVector.m_positionB);
        bcv->SetVelocity({testVector.m_ueVelocity / 3.6, 0, 0});
        bool isAIndoor = buildingInfoA->IsIndoor();
        bool isBIndoor = buildingInfoB->IsIndoor();

        Ptr<ChannelCondition> cond = condModel->GetChannelCondition(a, b);
        cond->SetO2iLowHighCondition(testVector.m_o2iLossType);
        if (!isAIndoor && !isBIndoor) // a and b are outdoor
        {
            cond->SetO2iCondition(ChannelCondition::O2iConditionValue::O2O);
        }
        else
        {
            cond->SetO2iCondition(ChannelCondition::O2iConditionValue::O2I);
        }

        propModelFactory.SetTypeId(testVector.m_propModel);
        m_propModel = propModelFactory.Create<ThreeGppPropagationLossModel>();
        m_propModel->SetAttribute("Frequency", DoubleValue(testVector.m_frequency));
        m_propModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
        streamOffset += m_propModel->AssignStreams(streamOffset);
        streamOffset += m_propModel->GetChannelConditionModel()->AssignStreams(streamOffset);
        m_propModel->SetChannelConditionModel(condModel);
        streamOffset += m_propModel->AssignStreams(streamOffset);
        m_propModel->GetChannelConditionModel()->AssignStreams(streamOffset);
        std::vector<double> samples(1000);
        for (auto& sample : samples)
        {
            // Compute total loss
            sample = m_propModel->CalcRxPower(0.0, a, b);
            // Trigger another random variable usage
            m_propModel->ClearO2iLossCacheMap();
        }

        auto getMeanAndStddev = [](const std::vector<double> vec) {
            // Calculate the mean
            double sum = std::accumulate(vec.begin(), vec.end(), 0.0);
            double mean = sum / vec.size();

            // Calculate variance and standard deviation
            double sq_sum = std::inner_product(vec.begin(), vec.end(), vec.begin(), 0.0);
            double stddev = std::sqrt((sq_sum / vec.size()) - mean * mean);
            return std::make_pair(mean, stddev);
        };

        auto [mean, stddev] = getMeanAndStddev(samples);
        NS_TEST_ASSERT_MSG_EQ_TOL(mean,
                                  testVector.m_expectedMean,
                                  -testVector.m_expectedMean * 0.1,
                                  "Test vector " << i << ", mean outside expected range");
        NS_TEST_ASSERT_MSG_EQ_TOL(stddev,
                                  testVector.m_expectedStddev,
                                  std::max(0.1, testVector.m_expectedStddev * 0.1),
                                  "Test vector " << i
                                                 << ", standard deviation outside expected range");
        m_propModel = nullptr;
        Simulator::Destroy();
    }
}

/**
 * @ingroup propagation-tests
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
