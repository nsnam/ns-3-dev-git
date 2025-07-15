/*
 * Copyright (c) 2025 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/hexagonal-wraparound-model.h"
#include "ns3/mobility-helper.h"
#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/waypoint-mobility-model.h"

using namespace ns3;

/**
 * @ingroup propagation-test
 *
 * @brief Wraparound Model Test
 */
class WraparoundModelTest : public TestCase
{
  public:
    /**
     * Constructor for test case
     * @param name description of test case
     * @param numRings number of rings used in the deployment
     * @param alternativeConfig wraparound is configured manually when false, or via constructor
     * when true.
     */
    WraparoundModelTest(std::string name, int numRings, bool alternativeConfig)
        : TestCase(name),
          m_numRings(numRings),
          m_alternativeConfig(alternativeConfig)
    {
    }

  private:
    int m_numRings;           ///< number of wrings to wraparound
    bool m_alternativeConfig; ///< indicate whether to configure isd and numSites via constructor or
                              ///< manually
  private:
    void DoRun() override;
};

size_t
GetNearestSite(std::vector<Vector3D>& sites, size_t numSites, Vector3D virtualPos)
{
    double minDist = std::numeric_limits<double>::max();
    size_t minSite = 0;
    for (size_t siteI = 0; siteI < sites.size() && siteI < numSites; siteI++)
    {
        auto dist = CalculateDistance(sites.at(siteI), virtualPos);
        if (dist < minDist)
        {
            minDist = dist;
            minSite = siteI;
        }
    }
    return minSite;
}

void
WraparoundModelTest::DoRun()
{
    int numSites = 0;
    switch (m_numRings)
    {
    case 0:
        numSites = 1;
        break;
    case 1:
        numSites = 7;
        break;
    case 3:
        numSites = 19;
        break;
    default:
        NS_TEST_EXPECT_MSG_EQ(false,
                              true,
                              "Wraparound does not support " << m_numRings << " rings");
        break;
    }

    NodeContainer userNodes(1);
    NodeContainer siteNodes(numSites);

    MobilityHelper mobilityHelper;
    mobilityHelper.SetMobilityModel("ns3::ConstantPositionMobilityModel");

    // Site positions
    // clang-format off
    std::vector<Vector3D> sitePositions = {
        Vector3D(    0,     0, 30), // Site 01
        Vector3D( 1000,     0, 30), // Site 02
        Vector3D(  500,   866, 30), // Site 03
        Vector3D( -500,   866, 30), // Site 04
        Vector3D(-1000,     0, 30), // Site 05
        Vector3D( -500,  -866, 30), // Site 06
        Vector3D(  500,  -866, 30), // Site 07
        Vector3D( 2000,     0, 30), // Site 08
        Vector3D( 1500,   866, 30), // Site 09
        Vector3D(  500,  1732, 30), // Site 10
        Vector3D( -500,  1732, 30), // Site 11
        Vector3D(-1500,   866, 30), // Site 12
        Vector3D(-2000,     0, 30), // Site 13
        Vector3D(-1500,  -866, 30), // Site 14
        Vector3D( -500, -1732, 30), // Site 15
        Vector3D(  500, -1732, 30), // Site 16
        Vector3D( 1500,  -866, 30), // Site 17
        Vector3D( 1000,  1732, 30), // Site 18
        Vector3D(-1000,  1732, 30), // Site 19
    };
    // clang-format on

    Ptr<HexagonalWraparoundModel> wraparoundModel =
        m_alternativeConfig ? CreateObject<HexagonalWraparoundModel>()
                            : CreateObject<HexagonalWraparoundModel>(1000, numSites);

    if (m_alternativeConfig)
    {
        wraparoundModel->SetSiteDistance(1000);
        wraparoundModel->SetNumSites(numSites);
        auto sitePositionsSlice =
            std::vector<Vector3D>(sitePositions.begin(), sitePositions.begin() + numSites);
        wraparoundModel->SetSitePositions(sitePositionsSlice);
    }

    // Install mobility models
    mobilityHelper.Install(userNodes);
    mobilityHelper.Install(siteNodes);

    // Add site positions and wraparound model
    for (auto i = 0; i < numSites; i++)
    {
        auto siteMm = siteNodes.Get(i)->GetObject<MobilityModel>();
        siteMm->SetPosition(sitePositions.at(i));
        if (!m_alternativeConfig)
        {
            wraparoundModel->AddSitePosition(sitePositions.at(i));
        }
    }

    // Retrieve user node mobility to move it around
    auto userMm = userNodes.Get(0)->GetObject<MobilityModel>();

    // Test cases (target site number, expected UE virtual site, real user position, expected
    // wrapped position)
    std::vector<std::tuple<size_t, size_t, Vector3D, Vector3D>> testPoint;
    // clang-format off
    switch (m_numRings)
    {
    case 3: // Cases involving sites 8-19
        testPoint.emplace_back( 7,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back( 8,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back( 9,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(10,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(11,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(12,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(13,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(14,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(15,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(16,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(17,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(18,  0, Vector3D(    0,     0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back( 7,  0, Vector3D(  100,     0, 0), Vector3D(  100,     0, 0));
        testPoint.emplace_back( 8,  0, Vector3D(  500,     0, 0), Vector3D(  500,     0, 0));
        testPoint.emplace_back( 9,  1, Vector3D( 1000,     0, 0), Vector3D( 1000,     0, 0));
        testPoint.emplace_back(10,  2, Vector3D( 1000,  1000, 0), Vector3D( 1000,  1000, 0));
        testPoint.emplace_back(11,  0, Vector3D( -500,     0, 0), Vector3D( -500,     0, 0));
        testPoint.emplace_back(12,  0, Vector3D(    0,  -500, 0), Vector3D(    0,  -500, 0));
        testPoint.emplace_back(13,  5, Vector3D(-1000, -1000, 0), Vector3D(-1000, -1000, 0));
        testPoint.emplace_back(14, 13, Vector3D( 2000, -1000, 0), Vector3D(-2330, -1500, 0));
        testPoint.emplace_back(15, 16, Vector3D(-1000,  2000, 0), Vector3D( 1598, -1500, 0));
        testPoint.emplace_back(16, 15, Vector3D( 2000,  2000, 0), Vector3D(  267, -2000, 0));
        testPoint.emplace_back(17, 10, Vector3D(-2000, -2000, 0), Vector3D( -268,  2000, 0));
        testPoint.emplace_back(18, 11, Vector3D(-2000,  1500, 0), Vector3D(-2000,  1500, 0));
        break;
    case 1: // Cases involving sites 2-7
        testPoint.emplace_back(1, 0, Vector3D(    0,    0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(2, 0, Vector3D(    0,    0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(3, 0, Vector3D(    0,    0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(4, 0, Vector3D(    0,    0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(5, 0, Vector3D(    0,    0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(6, 0, Vector3D(    0,    0, 0), Vector3D(    0,     0, 0));
        testPoint.emplace_back(1, 1, Vector3D( 1000,    0, 0), Vector3D( 1000,     0, 0));
        testPoint.emplace_back(2, 3, Vector3D( 1000, -500, 0), Vector3D( -732,  1500, 0));
        testPoint.emplace_back(3, 4, Vector3D( 1000,  500, 0), Vector3D(-1598,     0, 0));
        testPoint.emplace_back(4, 4, Vector3D(-1000,    0, 0), Vector3D(-1000,     0, 0));
        testPoint.emplace_back(5, 6, Vector3D(-1000,  500, 0), Vector3D(  732, -1500, 0));
        testPoint.emplace_back(6, 1, Vector3D(-1000, -500, 0), Vector3D( 1598,     0, 0));
        break;
    case 0: // Cases involving site 1. No wraparound.
        testPoint.emplace_back(0, 0, Vector3D(    0,     0, 0), Vector3D(   0,      0, 0));
        testPoint.emplace_back(0, 0, Vector3D( 1000,  1000, 0), Vector3D( 1000,  1000, 0));
        testPoint.emplace_back(0, 0, Vector3D( 1000, -1000, 0), Vector3D( 1000, -1000, 0));
        testPoint.emplace_back(0, 0, Vector3D(-1000,  1000, 0), Vector3D(-1000,  1000, 0));
        testPoint.emplace_back(0, 0, Vector3D(-1000, -1000, 0), Vector3D(-1000, -1000, 0));
        testPoint.emplace_back(0, 0, Vector3D( 2000,  2000, 0), Vector3D( 2000,  2000, 0));
        testPoint.emplace_back(0, 0, Vector3D( 2000, -2000, 0), Vector3D( 2000, -2000, 0));
        testPoint.emplace_back(0, 0, Vector3D(-2000,  2000, 0), Vector3D(-2000,  2000, 0));
        testPoint.emplace_back(0, 0, Vector3D(-2000, -2000, 0), Vector3D(-2000, -2000, 0));
    default:
        break;
    }
    // clang-format on

    for (auto [siteNumber, expectedVirtualUserSite, realUserPos, expectedVirtualPos] : testPoint)
    {
        userMm->SetPosition(realUserPos);
        auto siteMm = siteNodes.Get(siteNumber)->GetObject<MobilityModel>();
        auto virtualPos =
            wraparoundModel->GetVirtualPosition(userMm->GetPosition(), siteMm->GetPosition());
        auto nearestSiteToVirtualUser = GetNearestSite(sitePositions, numSites, virtualPos);
        NS_TEST_EXPECT_MSG_EQ(nearestSiteToVirtualUser,
                              expectedVirtualUserSite,
                              "Virtual position leads to from real site "
                                  << GetNearestSite(sitePositions, numSites, realUserPos)
                                  << " to incorrect site " << nearestSiteToVirtualUser);
        NS_TEST_EXPECT_MSG_LT_OR_EQ(CalculateDistance(virtualPos, expectedVirtualPos),
                                    5,
                                    "Expected " << expectedVirtualPos << " and obtained "
                                                << virtualPos << " virtual positions differ");
    }
    // Move user node and check virtual positions correspond to the expected sites
    Simulator::Stop(MilliSeconds(1));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup propagation-test
 *
 * @brief Wraparound Model Test Suite
 */
static struct WraparoundModelTestSuite : public TestSuite
{
    WraparoundModelTestSuite()
        : TestSuite("wraparound-model", Type::UNIT)
    {
        std::vector<bool> alternativeConfig{false, true};
        for (auto altConf : alternativeConfig)
        {
            // Supported ring numbers
            AddTestCase(new WraparoundModelTest("Check wraparound with 0 rings", 0, altConf),
                        TestCase::Duration::QUICK);
            AddTestCase(new WraparoundModelTest("Check wraparound with 1 rings", 1, altConf),
                        TestCase::Duration::QUICK);
            AddTestCase(new WraparoundModelTest("Check wraparound with 3 rings", 3, altConf),
                        TestCase::Duration::QUICK);
        }
    }
} g_WraparoundModelTestSuite; ///< the test suite
