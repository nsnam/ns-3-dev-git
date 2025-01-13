/*
 * Copyright (c) 2011 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "ns3/antenna-model.h"
#include "ns3/log.h"
#include "ns3/test.h"

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

using namespace ns3;

/**
 * @ingroup tests
 *
 * @brief Angles Test using one vector for initialization
 */
class OneVectorConstructorTestCase : public TestCase
{
  public:
    /**
     * Build the test name
     * @param v test parameter
     * @return the test name
     */
    static std::string BuildNameString(Vector v);
    /**
     * Constructor
     * @param v vector
     * @param a expected angle
     */
    OneVectorConstructorTestCase(Vector v, Angles a);

  private:
    void DoRun() override;

    Vector m_v; //!< vector
    Angles m_a; //!< expected angle
};

std::string
OneVectorConstructorTestCase::BuildNameString(Vector v)
{
    std::ostringstream oss;
    oss << " v = " << v;
    return oss.str();
}

OneVectorConstructorTestCase::OneVectorConstructorTestCase(Vector v, Angles a)
    : TestCase(BuildNameString(v)),
      m_v(v),
      m_a(a)
{
}

void
OneVectorConstructorTestCase::DoRun()
{
    Angles a(m_v);
    NS_TEST_EXPECT_MSG_EQ_TOL(a.GetAzimuth(), m_a.GetAzimuth(), 1e-10, "incorrect phi");
    NS_TEST_EXPECT_MSG_EQ_TOL(a.GetInclination(), m_a.GetInclination(), 1e-10, "incorrect theta");
}

/**
 * @ingroup tests
 *
 * @brief Angles Test using two vectors for initialization
 */
class TwoVectorsConstructorTestCase : public TestCase
{
  public:
    /**
     * Build the test name
     * @param v test parameter
     * @param o test parameter
     * @return the test name
     */
    static std::string BuildNameString(Vector v, Vector o);
    /**
     * Constructor
     * @param v point
     * @param o origin
     * @param a expected angle
     */
    TwoVectorsConstructorTestCase(Vector v, Vector o, Angles a);

  private:
    void DoRun() override;

    Vector m_v; //!< point
    Vector m_o; //!< origin
    Angles m_a; //!< expected angle
};

std::string
TwoVectorsConstructorTestCase::BuildNameString(Vector v, Vector o)
{
    std::ostringstream oss;
    oss << " v = " << v << ", o = " << o;
    return oss.str();
}

TwoVectorsConstructorTestCase::TwoVectorsConstructorTestCase(Vector v, Vector o, Angles a)
    : TestCase(BuildNameString(v, o)),
      m_v(v),
      m_o(o),
      m_a(a)
{
}

void
TwoVectorsConstructorTestCase::DoRun()
{
    Angles a(m_v, m_o);
    NS_TEST_EXPECT_MSG_EQ_TOL(a.GetAzimuth(), m_a.GetAzimuth(), 1e-10, "incorrect phi");
    NS_TEST_EXPECT_MSG_EQ_TOL(a.GetInclination(), m_a.GetInclination(), 1e-10, "incorrect theta");
}

using WrapToRangeFunction = std::function<double(double)>;

/**
 * @ingroup tests
 *
 * @brief  Test bounds for various WrapTo... methods (WrapTo180, WrapTo360, WrapToPi, and WrapTo2Pi)
 * by using a std::function wrapper
 */
class WrapToRangeTestCase : public TestCase
{
  public:
    /**
     * Build the test name
     * @param lowerBound the lower bound of the WrapTo... function
     * @param upperBound the upper bound of the WrapTo... function
     * @return the test name
     */
    static std::string BuildNameString(double lowerBound, double upperBound);
    /**
     * Constructor
     * @param wrapper for one of WrapTo180, WrapTo360, WrapToPi, and WrapTo2Pi
     * @param lowerBound the corresponding lower bound
     * @param upperBound the corresponding upper bound
     */
    WrapToRangeTestCase(WrapToRangeFunction wrapper, double lowerBound, double upperBound);

  protected:
    /**
     * The given wrapper shall wrap an angle into the expected range
     * @param wrapPoint an angle
     */
    void CheckWrappingPoint(double wrapPoint);

  private:
    void DoRun() override;

    WrapToRangeFunction m_wrapper; //!< the wrapper function
    double m_lowerBound;           //!< the corresponding lower bound
    double m_upperBound;           //!< the corresponding upper bound
};

std::string
WrapToRangeTestCase::BuildNameString(double lowerBound, double upperBound)
{
    std::ostringstream oss;
    oss << "WrapTo [" << lowerBound << ", " << upperBound << ")";
    return oss.str();
}

WrapToRangeTestCase::WrapToRangeTestCase(WrapToRangeFunction wrapper,
                                         double lowerBound,
                                         double upperBound)
    : TestCase(BuildNameString(lowerBound, upperBound)),
      m_wrapper(wrapper),
      m_lowerBound(lowerBound),
      m_upperBound(upperBound)
{
}

void
WrapToRangeTestCase::DoRun()
{
    CheckWrappingPoint(m_lowerBound);
    CheckWrappingPoint(m_upperBound);
}

void
WrapToRangeTestCase::CheckWrappingPoint(double wrapPoint)
{
    constexpr int STEP_NUM = 100;
    double directions[] = {std::numeric_limits<double>::lowest(),
                           std::numeric_limits<double>::max()};
    for (double dir : directions)
    {
        int i = 0;
        for (double x = wrapPoint; i < STEP_NUM; x = std::nextafter(x, dir), ++i)
        {
            // If asserts are enabled, this test will crash with an assert instead of failing
            double result = m_wrapper(x);
            NS_TEST_EXPECT_MSG_EQ((m_lowerBound <= result),
                                  true,
                                  "Invalid wrap (too low) " << x << " maps to " << result << " and "
                                                            << result - m_lowerBound);
            NS_TEST_EXPECT_MSG_EQ((result < m_upperBound),
                                  true,
                                  "Invalid wrap (too high) " << x << " maps to " << result
                                                             << " and " << result - m_lowerBound);
        }
    }
}

/**
 * @ingroup tests
 *
 * @brief Test the output for WrapToRangeFunction
 */
class WrapToRangeFunctionalTestCase : public TestCase
{
  public:
    /**
     * Build the test name
     * @param angle the angle
     * @param wrappedAngle the expected result
     * @return the test name
     */
    static std::string BuildNameString(double angle, double wrappedAngle);
    /**
     * Constructor
     * @param wrapper one WrapToRangeFunction
     * @param angle the angle
     * @param wrappedAngle the expected result
     */
    WrapToRangeFunctionalTestCase(WrapToRangeFunction wrapper, double angle, double wrappedAngle);

  private:
    void DoRun() override;

    WrapToRangeFunction m_wrapper; //!< the wrapper function
    double m_angle;                //!< the input angle
    double m_wrappedAngle;         //!< the expected wrapper angle
};

std::string
WrapToRangeFunctionalTestCase::BuildNameString(double angle, double wrappedAngle)
{
    std::ostringstream oss;
    oss << "Wrap " << angle << " to " << wrappedAngle;
    return oss.str();
}

WrapToRangeFunctionalTestCase::WrapToRangeFunctionalTestCase(WrapToRangeFunction wrapper,
                                                             double angle,
                                                             double wrappedAngle)
    : TestCase(BuildNameString(angle, wrappedAngle)),
      m_wrapper(wrapper),
      m_angle(angle),
      m_wrappedAngle(wrappedAngle)
{
}

void
WrapToRangeFunctionalTestCase::DoRun()
{
    NS_TEST_EXPECT_MSG_EQ_TOL(m_wrapper(m_angle),
                              m_wrappedAngle,
                              1e-6,
                              "Invalid wrap " << m_angle << " wrapped to " << m_wrapper(m_angle)
                                              << " instead of " << m_wrappedAngle);
}

/**
 * @ingroup tests
 *
 * @brief Angles TestSuite
 */
class AnglesTestSuite : public TestSuite
{
  public:
    AnglesTestSuite();
};

AnglesTestSuite::AnglesTestSuite()
    : TestSuite("angles", Type::UNIT)
{
    AddTestCase(new OneVectorConstructorTestCase(Vector(1, 0, 0), Angles(0, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(-1, 0, 0), Angles(M_PI, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 1, 0), Angles(M_PI_2, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, -1, 0), Angles(-M_PI_2, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 0, 1), Angles(0, 0)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 0, -1), Angles(0, M_PI)),
                TestCase::Duration::QUICK);

    AddTestCase(new OneVectorConstructorTestCase(Vector(2, 0, 0), Angles(0, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(-2, 0, 0), Angles(M_PI, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 2, 0), Angles(M_PI_2, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, -2, 0), Angles(-M_PI_2, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 0, 2), Angles(0, 0)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 0, -2), Angles(0, M_PI)),
                TestCase::Duration::QUICK);

    AddTestCase(new OneVectorConstructorTestCase(Vector(1, 0, 1), Angles(0, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(1, 0, -1), Angles(0, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(1, 1, 0), Angles(M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(1, -1, 0), Angles(-M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(-1, 0, 1), Angles(M_PI, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(-1, 0, -1), Angles(M_PI, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(-1, 1, 0), Angles(3 * M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(-1, -1, 0), Angles(-3 * M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 1, 1), Angles(M_PI_2, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, 1, -1), Angles(M_PI_2, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, -1, 1), Angles(-M_PI_2, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new OneVectorConstructorTestCase(Vector(0, -1, -1), Angles(-M_PI_2, 3 * M_PI_4)),
                TestCase::Duration::QUICK);

    AddTestCase(
        new OneVectorConstructorTestCase(Vector(1, 1, std::sqrt(2)), Angles(M_PI_4, M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new OneVectorConstructorTestCase(Vector(1, 1, -std::sqrt(2)), Angles(M_PI_4, 3 * M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new OneVectorConstructorTestCase(Vector(1, -1, std::sqrt(2)), Angles(-M_PI_4, M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new OneVectorConstructorTestCase(Vector(-1, 1, std::sqrt(2)), Angles(3 * M_PI_4, M_PI_4)),
        TestCase::Duration::QUICK);

    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(1, 0, 0), Vector(0, 0, 0), Angles(0, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(-1, 0, 0), Vector(0, 0, 0), Angles(M_PI, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(0, 1, 0), Vector(0, 0, 0), Angles(M_PI_2, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, -1, 0),
                                                  Vector(0, 0, 0),
                                                  Angles(-M_PI_2, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, 0, 1), Vector(0, 0, 0), Angles(0, 0)),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(0, 0, -1), Vector(0, 0, 0), Angles(0, M_PI)),
        TestCase::Duration::QUICK);

    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(2, 0, 0), Vector(0, 0, 0), Angles(0, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(-2, 0, 0), Vector(0, 0, 0), Angles(M_PI, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(0, 2, 0), Vector(0, 0, 0), Angles(M_PI_2, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, -2, 0),
                                                  Vector(0, 0, 0),
                                                  Angles(-M_PI_2, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, 0, 2), Vector(0, 0, 0), Angles(0, 0)),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(0, 0, -2), Vector(0, 0, 0), Angles(0, M_PI)),
        TestCase::Duration::QUICK);

    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(1, 0, 1), Vector(0, 0, 0), Angles(0, M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(1, 0, -1), Vector(0, 0, 0), Angles(0, 3 * M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(1, 1, 0), Vector(0, 0, 0), Angles(M_PI_4, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(1, -1, 0),
                                                  Vector(0, 0, 0),
                                                  Angles(-M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(-1, 0, 1), Vector(0, 0, 0), Angles(M_PI, M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(-1, 0, -1),
                                                  Vector(0, 0, 0),
                                                  Angles(M_PI, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(-1, 1, 0),
                                                  Vector(0, 0, 0),
                                                  Angles(3 * M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(-1, -1, 0),
                                                  Vector(0, 0, 0),
                                                  Angles(-3 * M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(0, 1, 1), Vector(0, 0, 0), Angles(M_PI_2, M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, 1, -1),
                                                  Vector(0, 0, 0),
                                                  Angles(M_PI_2, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, -1, 1),
                                                  Vector(0, 0, 0),
                                                  Angles(-M_PI_2, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, -1, -1),
                                                  Vector(0, 0, 0),
                                                  Angles(-M_PI_2, 3 * M_PI_4)),
                TestCase::Duration::QUICK);

    AddTestCase(new TwoVectorsConstructorTestCase(Vector(1, 1, std::sqrt(2)),
                                                  Vector(0, 0, 0),
                                                  Angles(M_PI_4, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(1, 1, -std::sqrt(2)),
                                                  Vector(0, 0, 0),
                                                  Angles(M_PI_4, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(1, -1, std::sqrt(2)),
                                                  Vector(0, 0, 0),
                                                  Angles(-M_PI_4, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(-1, 1, std::sqrt(2)),
                                                  Vector(0, 0, 0),
                                                  Angles(3 * M_PI_4, M_PI_4)),
                TestCase::Duration::QUICK);

    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(3, 2, 2), Vector(2, 2, 2), Angles(0, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(1, 2, 2), Vector(2, 2, 2), Angles(M_PI, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(2, 3, 2), Vector(2, 2, 2), Angles(M_PI_2, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(-1, 2, 2),
                                                  Vector(-1, 3, 2),
                                                  Angles(-M_PI_2, M_PI_2)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(4, -2, 7), Vector(4, -2, 6), Angles(0, 0)),
                TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(0, -5, -1), Vector(0, -5, 0), Angles(0, M_PI)),
        TestCase::Duration::QUICK);

    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(-2, 2, -1), Vector(-4, 2, -1), Angles(0, M_PI_2)),
        TestCase::Duration::QUICK);
    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(2, 2, 0), Vector(4, 2, 0), Angles(M_PI, M_PI_2)),
        TestCase::Duration::QUICK);

    AddTestCase(
        new TwoVectorsConstructorTestCase(Vector(-1, 4, 4), Vector(-2, 4, 3), Angles(0, M_PI_4)),
        TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0, -2, -6),
                                                  Vector(-1, -2, -5),
                                                  Angles(0, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(77, 3, 43),
                                                  Vector(78, 2, 43),
                                                  Angles(3 * M_PI_4, M_PI_2)),
                TestCase::Duration::QUICK);

    AddTestCase(new TwoVectorsConstructorTestCase(Vector(24, -2, -6 - std::sqrt(2)),
                                                  Vector(23, -3, -6),
                                                  Angles(M_PI_4, 3 * M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new TwoVectorsConstructorTestCase(Vector(0.5, 11.45, std::sqrt(2) - 1),
                                                  Vector(-0.5, 12.45, -1),
                                                  Angles(-M_PI_4, M_PI_4)),
                TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeTestCase(WrapTo180, -180, 180), TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeTestCase(WrapToPi, -M_PI, M_PI), TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeTestCase(WrapTo360, 0, 360), TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeTestCase(WrapTo2Pi, 0, 2 * M_PI), TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo180, -182.2, 177.8),
                TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo180, -179, -179),
                TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo180, 181, -179), TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo180, 360.6, 0.6),
                TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo360, -182.8, 177.2),
                TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo360, -179, 181), TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo360, 181, 181), TestCase::Duration::QUICK);
    AddTestCase(new WrapToRangeFunctionalTestCase(WrapTo360, 360.2, 0.2),
                TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static AnglesTestSuite g_staticAnglesTestSuiteInstance;
