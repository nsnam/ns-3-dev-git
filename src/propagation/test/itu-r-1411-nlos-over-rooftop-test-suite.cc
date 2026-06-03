/*
 * Copyright (c) 2011,2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/constant-position-mobility-model.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/itu-r-1411-nlos-over-rooftop-propagation-loss-model.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ItuR1411NlosOverRooftopPropagationLossModelTest");

/**
 * @ingroup propagation-tests
 *
 * @brief ItuR1411NlosOverRooftopPropagationLossModel Test Case
 *
 */
class ItuR1411NlosOverRooftopPropagationLossModelTestCase : public TestCase
{
  public:
    /**
     * Constructor
     *
     * @param freq carrier frequency in Hz
     * @param dist 2D distance between UT and BS in meters
     * @param hb height of BS in meters
     * @param hm height of UT in meters
     * @param env environment type
     * @param city city type
     * @param streetsOrientation street orientation in degrees [0,90]
     * @param rooftopHeight rooftop level height in meters
     * @param refValue reference loss value
     * @param name TestCase name
     */
    ItuR1411NlosOverRooftopPropagationLossModelTestCase(double freq,
                                                        double dist,
                                                        double hb,
                                                        double hm,
                                                        EnvironmentType env,
                                                        CitySize city,
                                                        double streetsOrientation,
                                                        double rooftopHeight,
                                                        double refValue,
                                                        std::string name);
    ~ItuR1411NlosOverRooftopPropagationLossModelTestCase() override;

  private:
    void DoRun() override;

    /**
     * Create a MobilityModel
     * @param index mobility model index
     * @return a new MobilityModel
     */
    Ptr<MobilityModel> CreateMobilityModel(uint16_t index);

    double m_freq;               //!< carrier frequency in Hz
    double m_dist;               //!< 2D distance between UT and BS in meters
    double m_hb;                 //!< height of BS in meters
    double m_hm;                 //!< height of UT in meters
    EnvironmentType m_env;       //!< environment type
    CitySize m_city;             //!< city type
    double m_streetsOrientation; //!< street orientation in degrees [0,90]
    double m_rooftopHeight;      //!< rooftop level height in meters
    double m_lossRef;            //!< reference loss
};

ItuR1411NlosOverRooftopPropagationLossModelTestCase::
    ItuR1411NlosOverRooftopPropagationLossModelTestCase(double freq,
                                                        double dist,
                                                        double hb,
                                                        double hm,
                                                        EnvironmentType env,
                                                        CitySize city,
                                                        double streetsOrientation,
                                                        double rooftopHeight,
                                                        double refValue,
                                                        std::string name)
    : TestCase(name),
      m_freq(freq),
      m_dist(dist),
      m_hb(hb),
      m_hm(hm),
      m_env(env),
      m_city(city),
      m_streetsOrientation(streetsOrientation),
      m_rooftopHeight(rooftopHeight),
      m_lossRef(refValue)
{
}

ItuR1411NlosOverRooftopPropagationLossModelTestCase::
    ~ItuR1411NlosOverRooftopPropagationLossModelTestCase()
{
}

void
ItuR1411NlosOverRooftopPropagationLossModelTestCase::DoRun()
{
    NS_LOG_FUNCTION(this);

    Ptr<MobilityModel> mma = CreateObject<ConstantPositionMobilityModel>();
    mma->SetPosition(Vector(0.0, 0.0, m_hb));

    Ptr<MobilityModel> mmb = CreateObject<ConstantPositionMobilityModel>();
    mmb->SetPosition(Vector(m_dist, 0.0, m_hm));

    Ptr<ItuR1411NlosOverRooftopPropagationLossModel> propagationLossModel =
        CreateObject<ItuR1411NlosOverRooftopPropagationLossModel>();
    propagationLossModel->SetAttribute("Frequency", DoubleValue(m_freq));
    propagationLossModel->SetAttribute("Environment", EnumValue(m_env));
    propagationLossModel->SetAttribute("CitySize", EnumValue(m_city));
    propagationLossModel->SetAttribute("StreetsOrientation", DoubleValue(m_streetsOrientation));
    propagationLossModel->SetAttribute("RooftopLevel", DoubleValue(m_rooftopHeight));

    double loss = propagationLossModel->GetLoss(mma, mmb);

    NS_LOG_INFO("Calculated loss: " << loss);
    NS_LOG_INFO("Theoretical loss: " << m_lossRef);

    NS_TEST_ASSERT_MSG_EQ_TOL(loss, m_lossRef, 0.1, "Wrong loss!");
}

/**
 * @ingroup propagation-tests
 *
 * @brief ItuR1411NlosOverRooftopPropagationLossModel TestSuite
 *
 */
class ItuR1411NlosOverRooftopPropagationLossModelTestSuite : public TestSuite
{
  public:
    ItuR1411NlosOverRooftopPropagationLossModelTestSuite();
};

ItuR1411NlosOverRooftopPropagationLossModelTestSuite::
    ItuR1411NlosOverRooftopPropagationLossModelTestSuite()
    : TestSuite("itu-r-1411-nlos-over-rooftop", Type::SYSTEM)
{
    LogComponentEnable("ItuR1411NlosOverRooftopPropagationLossModelTest", LOG_LEVEL_ALL);

    // reference values obtained with the octave scripts in src/propagation/test/reference/
    AddTestCase(new ItuR1411NlosOverRooftopPropagationLossModelTestCase(
                    2.1140e9,
                    900,
                    30,
                    1,
                    UrbanEnvironment,
                    LargeCity,
                    45.0,
                    20.0,
                    143.68,
                    "f=2114Mhz, dist=900, urban large city"),
                TestCase::Duration::QUICK);
    AddTestCase(new ItuR1411NlosOverRooftopPropagationLossModelTestCase(
                    1.865e9,
                    500,
                    30,
                    1,
                    UrbanEnvironment,
                    LargeCity,
                    45.0,
                    20.0,
                    132.84,
                    "f=1865Mhz, dist=500, urban large city"),
                TestCase::Duration::QUICK);
    // The following three cases exercise the formula paths corrected for @issueid{1164}.
    // Reference values computed from src/propagation/test/reference/
    // loss_ITU1411_NLOS_over_rooftop.m with the matching parameters.
    // Lori branch for street orientation phi >= 55 degrees (eq. 13).
    AddTestCase(new ItuR1411NlosOverRooftopPropagationLossModelTestCase(
                    2.1140e9,
                    900,
                    30,
                    1,
                    UrbanEnvironment,
                    LargeCity,
                    80.0,
                    20.0,
                    141.59,
                    "f=2114Mhz, dist=900, urban large city, streets orientation 80 deg"),
                TestCase::Duration::QUICK);
    // kd branch for base station below rooftop level (eq. 20, kd scaled by rooftop height).
    AddTestCase(new ItuR1411NlosOverRooftopPropagationLossModelTestCase(
                    2.1140e9,
                    500,
                    15,
                    1,
                    UrbanEnvironment,
                    LargeCity,
                    45.0,
                    40.0,
                    163.87,
                    "f=2114Mhz, dist=500, urban large city, BS below rooftop"),
                TestCase::Duration::QUICK);
    // kf branch for metropolitan centre (large city), f <= 2 GHz (coefficient 1.5).
    AddTestCase(new ItuR1411NlosOverRooftopPropagationLossModelTestCase(
                    1.865e9,
                    100,
                    30,
                    1,
                    UrbanEnvironment,
                    LargeCity,
                    45.0,
                    20.0,
                    112.67,
                    "f=1865Mhz, dist=100, urban large city, metropolitan kf"),
                TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static ItuR1411NlosOverRooftopPropagationLossModelTestSuite g_ituR1411NlosOverRooftopTestSuite;
