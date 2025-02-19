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
#include "ns3/log.h"
#include "ns3/okumura-hata-propagation-loss-model.h"
#include "ns3/string.h"
#include "ns3/test.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("OkumuraHataPropagationLossModelTest");

/**
 * @ingroup propagation-tests
 *
 * @brief OkumuraHataPropagationLossModel Test Case
 */
class OkumuraHataPropagationLossModelTestCase : public TestCase
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
     * @param refValue reference loss value
     * @param name TestCase name
     */
    OkumuraHataPropagationLossModelTestCase(double freq,
                                            double dist,
                                            double hb,
                                            double hm,
                                            EnvironmentType env,
                                            CitySize city,
                                            double refValue,
                                            std::string name);
    ~OkumuraHataPropagationLossModelTestCase() override;

  private:
    void DoRun() override;

    /**
     * Create a MobilityModel
     * @param index mobility model index
     * @return a new MobilityModel
     */
    Ptr<MobilityModel> CreateMobilityModel(uint16_t index);

    double m_freq;         //!< carrier frequency in Hz
    double m_dist;         //!< 2D distance between UT and BS in meters
    double m_hb;           //!< height of BS in meters
    double m_hm;           //!< height of UT in meters
    EnvironmentType m_env; //!< environment type
    CitySize m_city;       //!< city type
    double m_lossRef;      //!< reference loss
};

OkumuraHataPropagationLossModelTestCase::OkumuraHataPropagationLossModelTestCase(
    double freq,
    double dist,
    double hb,
    double hm,
    EnvironmentType env,
    CitySize city,
    double refValue,
    std::string name)
    : TestCase(name),
      m_freq(freq),
      m_dist(dist),
      m_hb(hb),
      m_hm(hm),
      m_env(env),
      m_city(city),
      m_lossRef(refValue)
{
}

OkumuraHataPropagationLossModelTestCase::~OkumuraHataPropagationLossModelTestCase()
{
}

void
OkumuraHataPropagationLossModelTestCase::DoRun()
{
    NS_LOG_FUNCTION(this);

    Ptr<MobilityModel> mma = CreateObject<ConstantPositionMobilityModel>();
    mma->SetPosition(Vector(0.0, 0.0, m_hb));

    Ptr<MobilityModel> mmb = CreateObject<ConstantPositionMobilityModel>();
    mmb->SetPosition(Vector(m_dist, 0.0, m_hm));

    Ptr<OkumuraHataPropagationLossModel> propagationLossModel =
        CreateObject<OkumuraHataPropagationLossModel>();
    propagationLossModel->SetAttribute("Frequency", DoubleValue(m_freq));
    propagationLossModel->SetAttribute("Environment", EnumValue(m_env));
    propagationLossModel->SetAttribute("CitySize", EnumValue(m_city));

    double loss = propagationLossModel->GetLoss(mma, mmb);

    NS_LOG_INFO("Calculated loss: " << loss);
    NS_LOG_INFO("Theoretical loss: " << m_lossRef);

    NS_TEST_ASSERT_MSG_EQ_TOL(loss, m_lossRef, 0.1, "Wrong loss!");
}

/**
 * @ingroup propagation-tests
 *
 * @brief OkumuraHataPropagationLossModel TestSuite
 *
 * This TestSuite tests the following cases:
 *   - UrbanEnvironment - Large City (original OH and COST231 OH)
 *   - UrbanEnvironment - Small City (original OH and COST231 OH)
 *   - SubUrbanEnvironment (original OH only)
 *   - OpenAreasEnvironment (original OH only)
 */
class OkumuraHataPropagationLossModelTestSuite : public TestSuite
{
  public:
    OkumuraHataPropagationLossModelTestSuite();
};

OkumuraHataPropagationLossModelTestSuite::OkumuraHataPropagationLossModelTestSuite()
    : TestSuite("okumura-hata", Type::SYSTEM)
{
    LogComponentEnable("OkumuraHataPropagationLossModelTest", LOG_LEVEL_ALL);
    // reference values obtained with the octave scripts in src/propagation/test/reference/

    double freq = 869e6; // this will use the original OH model
    AddTestCase(new OkumuraHataPropagationLossModelTestCase(freq,
                                                            2000,
                                                            30,
                                                            1,
                                                            UrbanEnvironment,
                                                            LargeCity,
                                                            137.93,
                                                            "original OH Urban Large city"),
                TestCase::Duration::QUICK);
    AddTestCase(new OkumuraHataPropagationLossModelTestCase(freq,
                                                            2000,
                                                            30,
                                                            1,
                                                            UrbanEnvironment,
                                                            SmallCity,
                                                            137.88,
                                                            "original OH Urban small city"),
                TestCase::Duration::QUICK);
    AddTestCase(new OkumuraHataPropagationLossModelTestCase(freq,
                                                            2000,
                                                            30,
                                                            1,
                                                            SubUrbanEnvironment,
                                                            LargeCity,
                                                            128.03,
                                                            "original OH SubUrban"),
                TestCase::Duration::QUICK);
    AddTestCase(new OkumuraHataPropagationLossModelTestCase(freq,
                                                            2000,
                                                            30,
                                                            1,
                                                            OpenAreasEnvironment,
                                                            LargeCity,
                                                            109.66,
                                                            "original OH OpenAreas"),
                TestCase::Duration::QUICK);

    freq = 2.1140e9; // this will use the extended COST231 OH model
    AddTestCase(new OkumuraHataPropagationLossModelTestCase(freq,
                                                            2000,
                                                            30,
                                                            1,
                                                            UrbanEnvironment,
                                                            LargeCity,
                                                            148.55,
                                                            "COST231 OH Urban Large city"),
                TestCase::Duration::QUICK);
    AddTestCase(
        new OkumuraHataPropagationLossModelTestCase(freq,
                                                    2000,
                                                    30,
                                                    1,
                                                    UrbanEnvironment,
                                                    SmallCity,
                                                    150.64,
                                                    "COST231 OH Urban small city and suburban"),
        TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static OkumuraHataPropagationLossModelTestSuite g_okumuraHataTestSuite;
