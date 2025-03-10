/*
 * Copyright (c) 2019 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/abort.h"
#include "ns3/building-position-allocator.h"
#include "ns3/building.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/pointer.h"
#include "ns3/random-walk-2d-outdoor-mobility-model.h"
#include "ns3/simulator.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OutdoorRandomWalkTest");

/**
 * @ingroup building-test
 *
 * Test case for the class OutdoorRandomWalkTestCase. It checks if the
 * positions visited by the user are outside buildings
 */
class OutdoorRandomWalkTestCase : public TestCase
{
  public:
    OutdoorRandomWalkTestCase();
    ~OutdoorRandomWalkTestCase() override;

  private:
    void DoRun() override;

    /**
     * Check that the position is the expected one
     * @param model Mobility model
     */
    void CheckPositionOutdoor(Ptr<RandomWalk2dOutdoorMobilityModel> model);

    std::vector<Ptr<Building>> m_buildings; //!< Buildings
};

OutdoorRandomWalkTestCase::OutdoorRandomWalkTestCase()
    : TestCase("Test case for the BuildingsChannelConditionModel"),
      m_buildings()
{
}

OutdoorRandomWalkTestCase::~OutdoorRandomWalkTestCase()
{
}

void
OutdoorRandomWalkTestCase::CheckPositionOutdoor(Ptr<RandomWalk2dOutdoorMobilityModel> model)
{
    auto position = model->GetPosition();
    for (auto building : m_buildings)
    {
        NS_TEST_ASSERT_MSG_EQ(building->IsInside(position),
                              false,
                              "Position " << position << " is inside");
    }
}

void
OutdoorRandomWalkTestCase::DoRun()
{
    // Samples to test and time steps
    constexpr double TEST_STEP_S = 10; // s
    constexpr int MAX_CHECKS = 1000;

    // create a grid of buildings
    double buildingSizeX = 100; // m
    double buildingSizeY = 50;  // m
    double streetWidth = 25;    // m
    double buildingHeight = 10; // m
    uint32_t numBuildingsX = 20;
    uint32_t numBuildingsY = 20;
    double maxAxisX = (buildingSizeX + streetWidth) * numBuildingsX;
    double maxAxisY = (buildingSizeY + streetWidth) * numBuildingsY;

    for (uint32_t buildingIdX = 0; buildingIdX < numBuildingsX; ++buildingIdX)
    {
        for (uint32_t buildingIdY = 0; buildingIdY < numBuildingsY; ++buildingIdY)
        {
            Ptr<Building> building;
            building = CreateObject<Building>();

            building->SetBoundaries(Box(buildingIdX * (buildingSizeX + streetWidth),
                                        buildingIdX * (buildingSizeX + streetWidth) + buildingSizeX,
                                        buildingIdY * (buildingSizeY + streetWidth),
                                        buildingIdY * (buildingSizeY + streetWidth) + buildingSizeY,
                                        0.0,
                                        buildingHeight));
            building->SetNRoomsX(1);
            building->SetNRoomsY(1);
            building->SetNFloors(1);
            m_buildings.push_back(building);
        }
    }

    // create one node
    NodeContainer nodes;
    nodes.Create(1);

    // set the RandomWalk2dOutdoorMobilityModel mobility model
    MobilityHelper mobility;
    mobility.SetMobilityModel(
        "ns3::RandomWalk2dOutdoorMobilityModel",
        "Bounds",
        RectangleValue(Rectangle(-streetWidth, maxAxisX, -streetWidth, maxAxisY)),
        "Mode",
        EnumValue(RandomWalk2dOutdoorMobilityModel::MODE_TIME),
        "Time",
        TimeValue(Seconds(TEST_STEP_S * MAX_CHECKS)));
    // create an OutdoorPositionAllocator and set its boundaries to match those of the mobility
    // model
    Ptr<OutdoorPositionAllocator> position = CreateObject<OutdoorPositionAllocator>();
    Ptr<UniformRandomVariable> xPos = CreateObject<UniformRandomVariable>();
    xPos->SetAttribute("Min", DoubleValue(-streetWidth));
    xPos->SetAttribute("Max", DoubleValue(maxAxisX));
    Ptr<UniformRandomVariable> yPos = CreateObject<UniformRandomVariable>();
    yPos->SetAttribute("Min", DoubleValue(-streetWidth));
    yPos->SetAttribute("Max", DoubleValue(maxAxisY));
    position->SetAttribute("X", PointerValue(xPos));
    position->SetAttribute("Y", PointerValue(yPos));
    mobility.SetPositionAllocator(position);
    // install the mobility model
    mobility.Install(nodes.Get(0));

    auto mobilityModel = nodes.Get(0)->GetObject<RandomWalk2dOutdoorMobilityModel>();

    // get MAX_CHECKS positions, check if they are outdoors
    for (int i = 0; i < MAX_CHECKS; i++)
    {
        Simulator::Schedule(Seconds(i * TEST_STEP_S),
                            &OutdoorRandomWalkTestCase::CheckPositionOutdoor,
                            this,
                            mobilityModel);
    }

    Simulator::Stop(Seconds(MAX_CHECKS * TEST_STEP_S + 1));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup building-test
 *
 * Test suite for the buildings channel condition model
 */
class OutdoorRandomWalkTestSuite : public TestSuite
{
  public:
    OutdoorRandomWalkTestSuite();
};

OutdoorRandomWalkTestSuite::OutdoorRandomWalkTestSuite()
    : TestSuite("outdoor-random-walk-model", Type::UNIT)
{
    AddTestCase(new OutdoorRandomWalkTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static OutdoorRandomWalkTestSuite OutdoorRandomWalkTestSuite;
