/*
 * Copyright (c) 2023 SIGNET Lab, Department of Information Engineering,
 * University of Padova
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
#include "ns3/channel-condition-model.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/double.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-v2v-propagation-loss-model.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeGppNTNPropagationLossModelsTest");

/**
 * \ingroup propagation-tests
 *
 * Test case for the ThreeGppNTNPropagationLossModel classes.
 * It computes the path loss between two nodes and compares it with the value
 * obtained using the results provided in 3GPP TR 38.821.
 */
class ThreeGppNTNPropagationLossModelTestCase : public TestCase
{
  public:
    ThreeGppNTNPropagationLossModelTestCase();

    /**
     * Description of a single test point
     */
    struct TestPoint
    {
        /**
         * @brief Constructor
         *
         * @param distance  2D distance between the test nodes
         * @param isLos     whether to compute the path loss for a channel LOS condition
         * @param frequency carrier frequency in Hz
         * @param pwrRxDbm  expected received power in dBm
         * @param lossModel the propagation loss model to test
         */
        TestPoint(double distance,
                  bool isLos,
                  double frequency,
                  double pwrRxDbm,
                  Ptr<ThreeGppPropagationLossModel> lossModel)
            : m_distance(distance),
              m_isLos(isLos),
              m_frequency(frequency),
              m_pwrRxDbm(pwrRxDbm),
              m_propagationLossModel(lossModel)
        {
        }

        double m_distance;  //!< 2D distance between test nodes, in meters
        bool m_isLos;       //!< if true LOS, if false NLOS
        double m_frequency; //!< carrier frequency in Hz
        double m_pwrRxDbm;  //!< received power in dBm
        Ptr<ThreeGppPropagationLossModel> m_propagationLossModel; //!< the path loss model to test
    };

  private:
    /**
     * Build the simulation scenario and run the tests
     */
    void DoRun() override;

    /**
     * Test the channel gain for a specific parameter configuration,
     * by comparing the antenna gain obtained using CircularApertureAntennaModel::GetGainDb
     * and the one of manually computed test instances.
     *
     * @param testPoint the parameter configuration to be tested
     */
    void TestChannelGain(TestPoint testPoint);
};

ThreeGppNTNPropagationLossModelTestCase::ThreeGppNTNPropagationLossModelTestCase()
    : TestCase("Creating ThreeGppNTNPropagationLossModelTestCase")

{
}

void
ThreeGppNTNPropagationLossModelTestCase::DoRun()
{
    // Create the PLMs and disable shadowing to obtain deterministic results
    Ptr<ThreeGppNTNDenseUrbanPropagationLossModel> denseUrbanModel =
        CreateObject<ThreeGppNTNDenseUrbanPropagationLossModel>();
    denseUrbanModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
    Ptr<ThreeGppNTNUrbanPropagationLossModel> urbanModel =
        CreateObject<ThreeGppNTNUrbanPropagationLossModel>();
    urbanModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
    Ptr<ThreeGppNTNSuburbanPropagationLossModel> suburbanModel =
        CreateObject<ThreeGppNTNSuburbanPropagationLossModel>();
    suburbanModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
    Ptr<ThreeGppNTNRuralPropagationLossModel> ruralModel =
        CreateObject<ThreeGppNTNRuralPropagationLossModel>();
    ruralModel->SetAttribute("ShadowingEnabled", BooleanValue(false));

    //  Vector of test points
    std::vector<TestPoint> testPoints = {
        // LOS, test points are identical for all path loss models, since the LOS path loss
        // is independent from the specific class.
        // Dense-Urban LOS
        {35786000, true, 20.0e9, -209.915, denseUrbanModel},
        {35786000, true, 30.0e9, -213.437, denseUrbanModel},
        {35786000, true, 2.0e9, -191.744, denseUrbanModel},
        {600000, true, 20.0e9, -174.404, denseUrbanModel},
        {600000, true, 30.0e9, -177.925, denseUrbanModel},
        {600000, true, 2.0e9, -156.233, denseUrbanModel},
        {1200000, true, 20.0e9, -180.424, denseUrbanModel},
        {1200000, true, 30.0e9, -183.946, denseUrbanModel},
        {1200000, true, 2.0e9, -162.253, denseUrbanModel},
        // Urban LOS
        {35786000, true, 20.0e9, -209.915, urbanModel},
        {35786000, true, 30.0e9, -213.437, urbanModel},
        {35786000, true, 2.0e9, -191.744, urbanModel},
        {600000, true, 20.0e9, -174.404, urbanModel},
        {600000, true, 30.0e9, -177.925, urbanModel},
        {600000, true, 2.0e9, -156.233, urbanModel},
        {1200000, true, 20.0e9, -180.424, urbanModel},
        {1200000, true, 30.0e9, -183.946, urbanModel},
        {1200000, true, 2.0e9, -162.253, urbanModel},
        // Suburban LOS
        {35786000, true, 20.0e9, -209.915, suburbanModel},
        {35786000, true, 30.0e9, -213.437, suburbanModel},
        {35786000, true, 2.0e9, -191.744, suburbanModel},
        {600000, true, 20.0e9, -174.404, suburbanModel},
        {600000, true, 30.0e9, -177.925, suburbanModel},
        {600000, true, 2.0e9, -156.233, suburbanModel},
        {1200000, true, 20.0e9, -180.424, suburbanModel},
        {1200000, true, 30.0e9, -183.946, suburbanModel},
        {1200000, true, 2.0e9, -162.253, suburbanModel},
        // Rural LOS
        {35786000, true, 20.0e9, -209.915, ruralModel},
        {35786000, true, 30.0e9, -213.437, ruralModel},
        {35786000, true, 2.0e9, -191.744, ruralModel},
        {600000, true, 20.0e9, -174.404, ruralModel},
        {600000, true, 30.0e9, -177.925, ruralModel},
        {600000, true, 2.0e9, -156.233, ruralModel},
        {1200000, true, 20.0e9, -180.424, ruralModel},
        {1200000, true, 30.0e9, -183.946, ruralModel},
        {1200000, true, 2.0e9, -162.253, ruralModel}};

    // Call TestChannelGain on each test point
    for (auto& point : testPoints)
    {
        TestChannelGain(point);
    }
}

void
ThreeGppNTNPropagationLossModelTestCase::TestChannelGain(TestPoint testPoint)
{
    // Create the nodes for BS and UT
    NodeContainer nodes;
    nodes.Create(2);

    // Create the mobility models
    Ptr<MobilityModel> a = CreateObject<GeocentricConstantPositionMobilityModel>();
    nodes.Get(0)->AggregateObject(a);
    Ptr<MobilityModel> b = CreateObject<GeocentricConstantPositionMobilityModel>();
    nodes.Get(1)->AggregateObject(b);

    // Set fixed position of one of the nodes
    Vector posA = Vector(0.0, 0.0, 0.0);
    a->SetPosition(posA);
    Vector posB = Vector(0.0, 0.0, testPoint.m_distance);
    b->SetPosition(posB);

    // Declare condition model
    Ptr<ChannelConditionModel> conditionModel;

    // Set the channel condition using a deterministic channel condition model
    if (testPoint.m_isLos)
    {
        conditionModel = CreateObject<AlwaysLosChannelConditionModel>();
    }
    else
    {
        conditionModel = CreateObject<NeverLosChannelConditionModel>();
    }

    testPoint.m_propagationLossModel->SetChannelConditionModel(conditionModel);
    testPoint.m_propagationLossModel->SetAttribute("Frequency", DoubleValue(testPoint.m_frequency));
    NS_TEST_EXPECT_MSG_EQ_TOL(testPoint.m_propagationLossModel->CalcRxPower(0.0, a, b),
                              testPoint.m_pwrRxDbm,
                              5e-3,
                              "Obtained unexpected received power");

    Simulator::Destroy();
}

/**
 * \ingroup propagation-tests
 *
 * \brief 3GPP NTN Propagation models TestSuite
 *
 * This TestSuite tests the following models:
 *   - ThreeGppNTNDenseUrbanPropagationLossModel
 *   - ThreeGppNTNUrbanPropagationLossModel
 *   - ThreeGppNTNSuburbanPropagationLossModel
 *   - ThreeGppNTNRuralPropagationLossModel
 */
class ThreeGppNTNPropagationLossModelsTestSuite : public TestSuite
{
  public:
    ThreeGppNTNPropagationLossModelsTestSuite();
};

ThreeGppNTNPropagationLossModelsTestSuite::ThreeGppNTNPropagationLossModelsTestSuite()
    : TestSuite("three-gpp-ntn-propagation-loss-model", Type::UNIT)
{
    AddTestCase(new ThreeGppNTNPropagationLossModelTestCase(), Duration::QUICK);
}

/// Static variable for test initialization
static ThreeGppNTNPropagationLossModelsTestSuite g_propagationLossModelsTestSuite;
