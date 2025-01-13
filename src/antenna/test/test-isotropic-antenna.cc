/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/isotropic-antenna-model.h"
#include "ns3/log.h"
#include "ns3/test.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

using namespace ns3;

/**
 * @ingroup antenna-tests
 *
 * @brief IsotropicAntennaModel Test
 */
class IsotropicAntennaModelTestCase : public TestCase
{
  public:
    /**
     * Build the test name
     * @param a Antenna angle
     * @return the test name
     */
    static std::string BuildNameString(Angles a);
    /**
     * Constructor
     * @param a Antenna angle
     * @param expectedGainDb Expected antenna gain
     */
    IsotropicAntennaModelTestCase(Angles a, double expectedGainDb);

  private:
    void DoRun() override;

    Angles m_a;            //!< Antenna angle
    double m_expectedGain; //!< Expected gain
};

std::string
IsotropicAntennaModelTestCase::BuildNameString(Angles a)
{
    std::ostringstream oss;
    oss << "theta=" << a.GetInclination() << " , phi=" << a.GetAzimuth();
    return oss.str();
}

IsotropicAntennaModelTestCase::IsotropicAntennaModelTestCase(Angles a, double expectedGainDb)
    : TestCase(BuildNameString(a)),
      m_a(a),
      m_expectedGain(expectedGainDb)
{
}

void
IsotropicAntennaModelTestCase::DoRun()
{
    Ptr<IsotropicAntennaModel> a = CreateObject<IsotropicAntennaModel>();
    double actualGain = a->GetGainDb(m_a);
    NS_TEST_EXPECT_MSG_EQ_TOL(actualGain,
                              m_expectedGain,
                              0.01,
                              "wrong value of the radiation pattern");
}

/**
 * @ingroup antenna-tests
 *
 * @brief IsotropicAntennaModel TestSuite
 */
class IsotropicAntennaModelTestSuite : public TestSuite
{
  public:
    IsotropicAntennaModelTestSuite();
};

IsotropicAntennaModelTestSuite::IsotropicAntennaModelTestSuite()
    : TestSuite("isotropic-antenna-model", Type::UNIT)
{
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(0, 0), 0.0), TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(0, M_PI), 0.0), TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(0, M_PI_2), 0.0),
                TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(M_PI, 0), 0.0), TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(M_PI, M_PI), 0.0),
                TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(M_PI, M_PI_2), 0.0),
                TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(M_PI_2, 0), 0.0),
                TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(M_PI_2, M_PI), 0.0),
                TestCase::Duration::QUICK);
    AddTestCase(new IsotropicAntennaModelTestCase(Angles(M_PI_2, M_PI_2), 0.0),
                TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static IsotropicAntennaModelTestSuite g_staticIsotropicAntennaModelTestSuiteInstance;
