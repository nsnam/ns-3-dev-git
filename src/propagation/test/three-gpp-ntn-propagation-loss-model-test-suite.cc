/*
 * Copyright (c) 2023 SIGNET Lab, Department of Information Engineering,
 * University of Padova
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/channel-condition-model.h"
#include "ns3/config.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/double.h"
#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/geographic-positions.h"
#include "ns3/log.h"
#include "ns3/mobility-helper.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/three-gpp-propagation-loss-model.h"
#include "ns3/three-gpp-v2v-propagation-loss-model.h"

#include <cmath>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ThreeGppNTNPropagationLossModelsTest");

/**
 * @ingroup propagation-tests
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
 * @ingroup propagation-tests
 *
 * Test case checking that the NTN path loss models attenuate links between
 * two ground terminals.
 *
 * The TR 38.811 path loss model is defined for satellite-to-ground links with
 * elevation angles between 10 and 90 degrees. For two terminals on the ground,
 * the elevation angle of the link is slightly negative (each terminal lies
 * below the local horizon of the other, by half the central angle subtended at
 * the Earth's center), outside the domain of the elevation-dependent terms:
 * the atmospheric absorption term scales the zenith attenuation by the
 * cosecant of the elevation angle, which for a negative angle would turn into
 * a large negative loss, unbounded as the distance shrinks, and infinite at an
 * elevation angle of exactly zero (e.g., a terminal on the tangent plane of
 * the other). The models therefore return a defined large loss for links with
 * non-positive elevation angles. This test guards that behavior by asserting
 * that the total loss of a ground-to-ground link is finite and at least the
 * free-space path loss, which holds for any defined handling of the
 * out-of-domain geometry (the TR terms added to free-space loss are all
 * non-negative).
 */
class ThreeGppNTNPropagationLossModelGroundToGroundTestCase : public TestCase
{
  public:
    ThreeGppNTNPropagationLossModelGroundToGroundTestCase();

  private:
    void DoRun() override;
};

ThreeGppNTNPropagationLossModelGroundToGroundTestCase::
    ThreeGppNTNPropagationLossModelGroundToGroundTestCase()
    : TestCase("Ground-to-ground links through the NTN path loss models must attenuate")
{
}

void
ThreeGppNTNPropagationLossModelGroundToGroundTestCase::DoRun()
{
    // Create one PLM per NTN scenario and disable shadowing to obtain
    // deterministic results
    std::vector<Ptr<ThreeGppPropagationLossModel>> lossModels{
        CreateObject<ThreeGppNTNDenseUrbanPropagationLossModel>(),
        CreateObject<ThreeGppNTNUrbanPropagationLossModel>(),
        CreateObject<ThreeGppNTNSuburbanPropagationLossModel>(),
        CreateObject<ThreeGppNTNRuralPropagationLossModel>()};

    const double txPowDbm = 30.0;
    const double terminalHeight = 1.5;
    const std::vector<double> frequencies{2.0e9, 20.0e9};
    const std::vector<double> distances{100, 1000, 10000};

    for (auto& lossModel : lossModels)
    {
        lossModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
        // The LOS condition is fixed so that the test exercises GetLossLos
        // deterministically; the NLOS branch shares the same distance and
        // elevation-angle handling and only adds non-negative clutter loss
        lossModel->SetChannelConditionModel(CreateObject<AlwaysLosChannelConditionModel>());

        for (double frequency : frequencies)
        {
            lossModel->SetAttribute("Frequency", DoubleValue(frequency));

            for (double distance : distances)
            {
                NodeContainer nodes;
                nodes.Create(2);
                auto a = CreateObject<GeocentricConstantPositionMobilityModel>();
                nodes.Get(0)->AggregateObject(a);
                auto b = CreateObject<GeocentricConstantPositionMobilityModel>();
                nodes.Get(1)->AggregateObject(b);
                // Place both terminals on the sphere at the same altitude,
                // separated along a meridian. Topocentric SetPosition would
                // instead place the second terminal on the tangent plane of
                // the first, where the elevation angle is exactly zero
                a->SetGeographicPosition(Vector(0.0, 0.0, terminalHeight));
                double latOffsetDeg =
                    distance / GeographicPositions::EARTH_SPHERE_RADIUS * 180.0 / M_PI;
                b->SetGeographicPosition(Vector(latOffsetDeg, 0.0, terminalHeight));

                // Bound from the same 3D distance the model uses
                double dist3d = CalculateDistance(a->GetPosition(), b->GetPosition());
                double fsplDb =
                    32.45 + 20 * log10(frequency / 1e9) + 20 * log10(dist3d); // TR 38.811 FSPL
                double rxPowDbm = lossModel->CalcRxPower(txPowDbm, a, b);
                NS_TEST_EXPECT_MSG_EQ(std::isfinite(rxPowDbm),
                                      true,
                                      "Ground-to-ground link loss must be finite (model "
                                          << lossModel->GetInstanceTypeId().GetName()
                                          << ", frequency " << frequency / 1e9 << " GHz, distance "
                                          << distance << " m)");
                NS_TEST_EXPECT_MSG_LT_OR_EQ(
                    rxPowDbm,
                    txPowDbm - fsplDb + 0.5,
                    "Ground-to-ground link must attenuate by at least the free-space path loss"
                    " (model "
                        << lossModel->GetInstanceTypeId().GetName() << ", frequency "
                        << frequency / 1e9 << " GHz, distance " << distance << " m)");
            }
        }
    }

    Simulator::Destroy();
}

/**
 * @ingroup propagation-tests
 *
 * Test case checking that the NTN path loss models return a finite loss for
 * two terminals at the same position, for which the elevation angle is
 * undefined (NaN). The NaN previously propagated into the quantized
 * elevation angle used as a table key by the channel condition and path
 * loss models, aborting the simulation. Each model is paired with the
 * channel condition model of its scenario so that the LOS probability
 * lookup is exercised as well.
 */
class ThreeGppNTNPropagationLossModelCoincidentTestCase : public TestCase
{
  public:
    ThreeGppNTNPropagationLossModelCoincidentTestCase();

  private:
    void DoRun() override;
};

ThreeGppNTNPropagationLossModelCoincidentTestCase::
    ThreeGppNTNPropagationLossModelCoincidentTestCase()
    : TestCase("NTN path loss models must return a finite loss for coincident terminals")
{
}

void
ThreeGppNTNPropagationLossModelCoincidentTestCase::DoRun()
{
    std::vector<std::pair<Ptr<ThreeGppPropagationLossModel>, Ptr<ChannelConditionModel>>> models{
        {CreateObject<ThreeGppNTNDenseUrbanPropagationLossModel>(),
         CreateObject<ThreeGppNTNDenseUrbanChannelConditionModel>()},
        {CreateObject<ThreeGppNTNUrbanPropagationLossModel>(),
         CreateObject<ThreeGppNTNUrbanChannelConditionModel>()},
        {CreateObject<ThreeGppNTNSuburbanPropagationLossModel>(),
         CreateObject<ThreeGppNTNSuburbanChannelConditionModel>()},
        {CreateObject<ThreeGppNTNRuralPropagationLossModel>(),
         CreateObject<ThreeGppNTNRuralChannelConditionModel>()}};

    const double txPowDbm = 30.0;
    const std::vector<double> frequencies{2.0e9, 20.0e9};

    for (auto& [lossModel, condModel] : models)
    {
        lossModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
        lossModel->SetChannelConditionModel(condModel);

        for (double frequency : frequencies)
        {
            lossModel->SetAttribute("Frequency", DoubleValue(frequency));

            NodeContainer nodes;
            nodes.Create(2);
            auto a = CreateObject<GeocentricConstantPositionMobilityModel>();
            nodes.Get(0)->AggregateObject(a);
            auto b = CreateObject<GeocentricConstantPositionMobilityModel>();
            nodes.Get(1)->AggregateObject(b);
            a->SetGeographicPosition(Vector(0.0, 0.0, 1.5));
            b->SetGeographicPosition(Vector(0.0, 0.0, 1.5));

            double rxPowDbm = lossModel->CalcRxPower(txPowDbm, a, b);
            NS_TEST_EXPECT_MSG_EQ(std::isfinite(rxPowDbm),
                                  true,
                                  "Coincident link loss must be finite (model "
                                      << lossModel->GetInstanceTypeId().GetName() << ", frequency "
                                      << frequency / 1e9 << " GHz)");
            NS_TEST_EXPECT_MSG_LT(rxPowDbm,
                                  txPowDbm,
                                  "Coincident link must not amplify (model "
                                      << lossModel->GetInstanceTypeId().GetName() << ", frequency "
                                      << frequency / 1e9 << " GHz)");
        }
    }

    Simulator::Destroy();
}

/**
 * @ingroup propagation-tests
 *
 * Test case for the attributes disabling the scintillation losses in the NTN
 * path loss models.
 *
 * The TR 38.811 scintillation losses are applied unconditionally in their
 * respective frequency ranges: ionospheric (Sec 6.6.6.1-4) below 6 GHz and
 * tropospheric (Sec 6.6.6.2) at 6 GHz and above. The
 * IonosphericScintillationLossEnabled and TroposphericScintillationLossEnabled
 * attributes (default true) allow either term to be excluded, e.g. to match
 * study assumptions that zero ionospheric scintillation such as TR 38.821
 * Table 6.1.3.2-1. For each of the four scenario models, over a 90 degree
 * elevation LEO-600 link, this test verifies that disabling the term active
 * in the tested band recovers exactly the analytic loss (6.22/f_GHz^1.5 dB
 * ionospheric at 2 GHz; 0.12 dB tropospheric at 20 GHz) and that the toggle
 * of the term inactive in that band is a no-op.
 */
class ThreeGppNTNPropagationLossModelScintillationToggleTestCase : public TestCase
{
  public:
    ThreeGppNTNPropagationLossModelScintillationToggleTestCase();

  private:
    void DoRun() override;

    /**
     * Compute the received power over a 90 degree elevation satellite link.
     *
     * @param lossModel the NTN path loss model to evaluate
     * @param frequency the carrier frequency in Hz
     * @param ionoEnabled value for the IonosphericScintillationLossEnabled attribute
     * @param tropoEnabled value for the TroposphericScintillationLossEnabled attribute
     * @return the received power in dBm for a 0 dBm transmission
     */
    double ComputeRxPower(Ptr<ThreeGppPropagationLossModel> lossModel,
                          double frequency,
                          bool ionoEnabled,
                          bool tropoEnabled);
};

ThreeGppNTNPropagationLossModelScintillationToggleTestCase::
    ThreeGppNTNPropagationLossModelScintillationToggleTestCase()
    : TestCase("Attributes disabling the NTN scintillation losses")
{
}

double
ThreeGppNTNPropagationLossModelScintillationToggleTestCase::ComputeRxPower(
    Ptr<ThreeGppPropagationLossModel> lossModel,
    double frequency,
    bool ionoEnabled,
    bool tropoEnabled)
{
    NodeContainer nodes;
    nodes.Create(2);
    auto groundMobility = CreateObject<GeocentricConstantPositionMobilityModel>();
    nodes.Get(0)->AggregateObject(groundMobility);
    auto satMobility = CreateObject<GeocentricConstantPositionMobilityModel>();
    nodes.Get(1)->AggregateObject(satMobility);
    // Satellite at the zenith of the ground terminal: 90 degree elevation
    groundMobility->SetGeographicPosition(Vector(0.0, 0.0, 1.5));
    satMobility->SetGeographicPosition(Vector(0.0, 0.0, 600e3));

    lossModel->SetAttribute("Frequency", DoubleValue(frequency));
    lossModel->SetAttribute("IonosphericScintillationLossEnabled", BooleanValue(ionoEnabled));
    lossModel->SetAttribute("TroposphericScintillationLossEnabled", BooleanValue(tropoEnabled));

    return lossModel->CalcRxPower(0.0, groundMobility, satMobility);
}

void
ThreeGppNTNPropagationLossModelScintillationToggleTestCase::DoRun()
{
    // One PLM per NTN scenario, with shadowing disabled for deterministic
    // results; the LOS and NLOS branches share the scintillation term, so
    // the LOS branch suffices
    std::vector<Ptr<ThreeGppPropagationLossModel>> lossModels{
        CreateObject<ThreeGppNTNDenseUrbanPropagationLossModel>(),
        CreateObject<ThreeGppNTNUrbanPropagationLossModel>(),
        CreateObject<ThreeGppNTNSuburbanPropagationLossModel>(),
        CreateObject<ThreeGppNTNRuralPropagationLossModel>()};

    // Analytic scintillation losses at 90 degree elevation (TR 38.811):
    // ionospheric 6.22/f_GHz^1.5 dB at 2 GHz, tropospheric 0.12 dB at 20 GHz
    const double ionoLossDb = 6.22 / std::pow(2.0, 1.5);
    const double tropoLossDb = 0.12;

    for (auto& lossModel : lossModels)
    {
        lossModel->SetAttribute("ShadowingEnabled", BooleanValue(false));
        lossModel->SetChannelConditionModel(CreateObject<AlwaysLosChannelConditionModel>());
        const auto modelName = lossModel->GetInstanceTypeId().GetName();

        // At 2 GHz only the ionospheric term applies
        double baseline = ComputeRxPower(lossModel, 2.0e9, true, true);
        NS_TEST_EXPECT_MSG_EQ_TOL(ComputeRxPower(lossModel, 2.0e9, false, true) - baseline,
                                  ionoLossDb,
                                  1e-6,
                                  "Disabling the ionospheric scintillation loss must recover it"
                                  " (model "
                                      << modelName << ")");
        NS_TEST_EXPECT_MSG_EQ_TOL(ComputeRxPower(lossModel, 2.0e9, true, false),
                                  baseline,
                                  1e-6,
                                  "The tropospheric toggle must be a no-op below 6 GHz (model "
                                      << modelName << ")");

        // At 20 GHz only the tropospheric term applies
        baseline = ComputeRxPower(lossModel, 20.0e9, true, true);
        NS_TEST_EXPECT_MSG_EQ_TOL(ComputeRxPower(lossModel, 20.0e9, true, false) - baseline,
                                  tropoLossDb,
                                  1e-6,
                                  "Disabling the tropospheric scintillation loss must recover it"
                                  " (model "
                                      << modelName << ")");
        NS_TEST_EXPECT_MSG_EQ_TOL(ComputeRxPower(lossModel, 20.0e9, false, true),
                                  baseline,
                                  1e-6,
                                  "The ionospheric toggle must be a no-op at 6 GHz and above"
                                  " (model "
                                      << modelName << ")");
    }

    Simulator::Destroy();
}

/**
 * @ingroup propagation-tests
 *
 * @brief 3GPP NTN Propagation models TestSuite
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
    AddTestCase(new ThreeGppNTNPropagationLossModelGroundToGroundTestCase(), Duration::QUICK);
    AddTestCase(new ThreeGppNTNPropagationLossModelCoincidentTestCase(), Duration::QUICK);
    AddTestCase(new ThreeGppNTNPropagationLossModelScintillationToggleTestCase(), Duration::QUICK);
}

/// Static variable for test initialization
static ThreeGppNTNPropagationLossModelsTestSuite g_propagationLossModelsTestSuite;
