/*
 *   Copyright (c) 2023 University of Padova, Dep. of Information Engineering, SIGNET lab.
 *
 *   SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/boolean.h"
#include "ns3/circular-aperture-antenna-model.h"
#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"
#include "ns3/uniform-planar-array.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TestCircularApertureAntennaModel");

/**
 * @ingroup antenna-tests
 *
 * @brief CircularApertureAntennaModel Test Case
 *
 * Note: Since Clang libc++ does not support the Mathematical special functions (P0226R1) yet, this
 * class falls back to Boost's implementation of cyl_bessel_j whenever the above standard library is
 * in use. If neither is available in the host system, this class is not compiled.
 */
class CircularApertureAntennaModelTestCase : public TestCase
{
  public:
    CircularApertureAntennaModelTestCase();

    /**
     * @brief Description of a single test point
     *
     * Description of a test point, which is characterized
     * by the CircularApertureAntennaModel parameters,
     * the directions towards which the antenna gain
     * is to be tested, and the expected gain value.
     */
    struct TestPoint
    {
        /**
         * @brief Constructor
         *
         * @param antennaMaxGainDb the antenna maximum possible gain [dB]
         * @param antennaMinGainDb the antenna minimum possible gain [dB]
         * @param antennaCircularApertureRadius the radius of the parabolic aperture [m]
         * @param operatingFrequency operating frequency [Hz]
         * @param testAzimuth test azimuth [rad]
         * @param testInclination test inclination [rad]
         * @param expectedGain the expected gain value [dB]
         * @param forceGainBounds restrict or not gain range to [antennaMinGainDb, antennaMaxGainDb]
         */
        TestPoint(double antennaMaxGainDb,
                  double antennaMinGainDb,
                  double antennaCircularApertureRadius,
                  double operatingFrequency,
                  double testAzimuth,
                  double testInclination,
                  double expectedGain,
                  bool forceGainBounds)
            : m_antennaMaxGainDb(antennaMaxGainDb),
              m_antennaMinGainDb(antennaMinGainDb),
              m_antennaCircularApertureRadius(antennaCircularApertureRadius),
              m_operatingFrequency(operatingFrequency),
              m_testAzimuth(DegreesToRadians(testAzimuth)),
              m_testInclination(DegreesToRadians(testInclination)),
              m_expectedGain(expectedGain),
              m_forceGainBounds(forceGainBounds)
        {
        }

        double m_antennaMaxGainDb;              ///< the antenna maximum possible gain [dB]
        double m_antennaMinGainDb;              ///< the antenna minimum possible gain [dB]
        double m_antennaCircularApertureRadius; ///< the radius of the parabolic aperture [m]
        double m_operatingFrequency;            ///< operating frequency [Hz]
        double m_testAzimuth;                   ///< test azimuth [rad]
        double m_testInclination;               ///< test inclination [rad]
        double m_expectedGain;                  ///< the expected gain value [dB]
        bool m_forceGainBounds;                 ///< enable bounds checking for GetGainDb
    };

    /**
     * Generate a string containing all relevant parameters
     * @param testPoint the parameter configuration to be tested
     * @return the string containing all relevant parameters
     */
    static std::string BuildNameString(TestPoint testPoint);

    /**
     * Test the antenna gain for a specific parameter configuration,
     * by comparing the antenna gain obtained using CircularApertureAntennaModel::GetGainDb
     * and the one obtained using MATLAB.
     *
     * @param testPoint the parameter configuration to be tested
     */
    void TestAntennaGain(TestPoint testPoint);

  private:
    /**
     * Run the test
     */
    void DoRun() override;
};

CircularApertureAntennaModelTestCase::CircularApertureAntennaModelTestCase()
    : TestCase("Creating CircularApertureAntennaModelTestCase")
{
}

std::string
CircularApertureAntennaModelTestCase::BuildNameString(TestPoint testPoint)
{
    std::ostringstream oss;
    oss << " Maximum gain=" << testPoint.m_antennaMaxGainDb << "dB"
        << " minimum gain=" << testPoint.m_antennaMinGainDb << "dB"
        << ", antenna aperture radius=" << testPoint.m_antennaCircularApertureRadius << "m"
        << ", frequency" << testPoint.m_operatingFrequency << "Hz"
        << ", test inclination=" << RadiansToDegrees(testPoint.m_testInclination) << " deg"
        << ", test azimuth=" << RadiansToDegrees(testPoint.m_testAzimuth) << " deg";
    return oss.str();
}

void
CircularApertureAntennaModelTestCase::TestAntennaGain(TestPoint testPoint)
{
    Ptr<CircularApertureAntennaModel> antenna =
        CreateObjectWithAttributes<CircularApertureAntennaModel>(
            "AntennaMaxGainDb",
            DoubleValue(testPoint.m_antennaMaxGainDb),
            "AntennaMinGainDb",
            DoubleValue(testPoint.m_antennaMinGainDb),
            "AntennaCircularApertureRadius",
            DoubleValue(testPoint.m_antennaCircularApertureRadius),
            "OperatingFrequency",
            DoubleValue(testPoint.m_operatingFrequency),
            "ForceGainBounds",
            BooleanValue(testPoint.m_forceGainBounds));

    Ptr<UniformPlanarArray> upa =
        CreateObjectWithAttributes<UniformPlanarArray>("AntennaElement",
                                                       PointerValue(antenna),
                                                       "NumColumns",
                                                       UintegerValue(1),
                                                       "NumRows",
                                                       UintegerValue(1));

    auto [fieldPhi, fieldTheta] =
        upa->GetElementFieldPattern(Angles(testPoint.m_testAzimuth, testPoint.m_testInclination),
                                    0);
    // Compute the antenna gain as the squared sum of the field pattern components
    double gainDb = 10 * log10(fieldPhi * fieldPhi + fieldTheta * fieldTheta);
    auto log = BuildNameString(testPoint);
    NS_LOG_INFO(log);
    NS_TEST_EXPECT_MSG_EQ_TOL(gainDb, testPoint.m_expectedGain, 0.1, log);
}

void
CircularApertureAntennaModelTestCase::DoRun()
{
    // Vector of test points
    std::vector<TestPoint> testPoints = {
        // MaxGainDb  MinGainDb  Radius (m)  Freq (Hz)   Azimuth (deg)  Incl (deg)  ExpGain (dB)
        // Test invariant: gain always equal to max gain at boresight (inclination 90, azimuth = 0)
        // for different frequency
        {30, -30, 0.5, 2e9, 0, 90, 30, false},
        {30, -30, 2, 20e9, 0, 90, 30, false},
        // Test invariant: gain always equal to max gain at boresight (inclination 90, azimuth = 0)
        // for different max gain
        {20, -30, 0.5, 2e9, 0, 90, 20, false},
        {10, -30, 2, 20e9, 0, 90, 10, false},
        // Test invariant: gain always equal to min gain outside of |theta| < 90 deg
        // for different frequency
        {30, -100, 0.5, 2e9, 0, 0, -100, false},
        {30, -100, 2, 20e9, 0, 0, -100, false},
        // Test invariant: gain always equal to min gain outside of |theta| < 90 deg
        // for different orientations
        {30, -100, 0.5, 2e9, 180, 90, -100, false},
        {30, -100, 2, 20e9, -180, 90, -100, false},
        // Fixed elevation to boresight (90deg) and azimuth varying in [-90, 0] deg with steps of 10
        // degrees
        {0, -50, 0.10707, 28000000000, -90, 90, -50, false},
        {0, -50, 0.10707, 28000000000, -80, 90, -49.8022, false},
        {0, -50, 0.10707, 28000000000, -70, 90, -49.1656, false},
        {0, -50, 0.10707, 28000000000, -60, 90, -60.9132, false},
        {0, -50, 0.10707, 28000000000, -60, 90, -50, true},
        {0, -50, 0.10707, 28000000000, -50, 90, -59.2368, false},
        {0, -50, 0.10707, 28000000000, -50, 90, -50, true},
        {0, -50, 0.10707, 28000000000, -40, 90, -44.6437, false},
        {0, -50, 0.10707, 28000000000, -30, 90, -43.9686, false},
        {0, -50, 0.10707, 28000000000, -20, 90, -36.3048, false},
        {0, -50, 0.10707, 28000000000, -10, 90, -30.5363, false},
        {0, -50, 0.10707, 28000000000, 0, 90, 0, false},
        // Fixed azimuth to boresight (0 deg) and azimuth varying in [0, 90] deg with steps of 9
        // degrees
        {0, -50, 0.10707, 28e9, 0, 0, -50, false},
        {0, -50, 0.10707, 28e9, 0, 9, -49.7256, false},
        {0, -50, 0.10707, 28e9, 0, 18, -52.9214, false},
        {0, -50, 0.10707, 28e9, 0, 18, -50, true},
        {0, -50, 0.10707, 28e9, 0, 27, -48.6077, false},
        {0, -50, 0.10707, 28e9, 0, 36, -60.684, false},
        {0, -50, 0.10707, 28e9, 0, 36, -50, true},
        {0, -50, 0.10707, 28e9, 0, 45, -55.1468, false},
        {0, -50, 0.10707, 28e9, 0, 45, -50, true},
        {0, -50, 0.10707, 28e9, 0, 54, -42.9648, false},
        {0, -50, 0.10707, 28e9, 0, 63, -45.6472, false},
        {0, -50, 0.10707, 28e9, 0, 72, -48.6378, false},
        {0, -50, 0.10707, 28e9, 0, 81, -35.1613, false},
        {0, -50, 0.10707, 28e9, 0, 90, 0, false}};

    // Call TestAntennaGain on each test point
    for (auto& point : testPoints)
    {
        TestAntennaGain(point);
    }
}

/**
 * @ingroup antenna-tests
 *
 * @brief UniformPlanarArray Test Suite
 */
class CircularApertureAntennaModelTestSuite : public TestSuite
{
  public:
    CircularApertureAntennaModelTestSuite();
};

CircularApertureAntennaModelTestSuite::CircularApertureAntennaModelTestSuite()
    : TestSuite("circular-aperture-antenna-test", Type::UNIT)
{
    AddTestCase(new CircularApertureAntennaModelTestCase());
}

static CircularApertureAntennaModelTestSuite staticCircularApertureAntennaModelTestSuiteInstance;
