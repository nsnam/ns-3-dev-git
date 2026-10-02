/*
 * Copyright 2014 University of Washington, Benjamin Cizdziel (tolerance-based ECEF test approach)
 *
 * SPDX-License-Identifier: GPL-2.0-only and NIST-Software
 */

/*
 * This test suite validates the LeoCircularOrbitMobilityModel, including
 * initial position computation, Earth rotation direction in the ECEF frame,
 * orbital offset, and on-demand orbital progress.
 */

#include "ns3/double.h"
#include "ns3/geographic-positions.h"
#include "ns3/leo-circular-orbit-mobility-model.h"
#include "ns3/leo-circular-orbit-position-allocator.h"
#include "ns3/leo-orbital-shell.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <cmath>
#include <numbers>
#include <sstream>

NS_LOG_COMPONENT_DEFINE("LeoMobilityTestSuite");

using namespace ns3;

/// Tolerance in meters for ECEF position comparisons
static constexpr double TOLERANCE = 10.0;

/// Tolerance in m/s for velocity comparisons
static constexpr double VELOCITY_TOLERANCE = 1e-3;

/// Tolerance in degrees for inclination comparisons
static constexpr double INCLINATION_TOLERANCE = 1e-3;

/// Earth sphere radius in km (must match the model)
static constexpr double EARTH_RADIUS_KM = GeographicPositions::EARTH_SPHERE_RADIUS / 1000.0;

/// Orbit radius in meters for altitude = 1000 km
static constexpr double ORBIT_RADIUS_M = (EARTH_RADIUS_KM + 1000.0) * 1000.0;

/// Geocentric gravitational constant in km^3/s^2 (must match the model)
static constexpr double GM_KM3_S2 = 398600.7;

/// Orbit height in km (geocentric radius) for altitude = 1000 km
static constexpr double ORBIT_HEIGHT_KM = EARTH_RADIUS_KM + 1000.0;

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that a satellite at time 0 is placed at the analytically expected
 * ECEF position for a given altitude, inclination, and longitude.
 *
 * With altitude = 1000 km, inclination = 45 degrees, longitude = 0 degrees, and
 * argument of latitude = 0 degrees, the satellite is at the ascending node:
 * on the equator at longitude 0, heading north. The expected ECEF position
 * at time 0 is:
 *   x = r
 *   y = 0
 *   z = 0
 * where r = (R_earth + 1000 km) in meters, and the velocity has a positive
 * z component.
 */
class LeoInitialPositionTestCase : public TestCase
{
  public:
    LeoInitialPositionTestCase();

  private:
    void DoRun() override;
};

LeoInitialPositionTestCase::LeoInitialPositionTestCase()
    : TestCase("LEO initial ECEF position at time zero")
{
}

void
LeoInitialPositionTestCase::DoRun()
{
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));  // km
    mob->SetAttribute("Inclination", DoubleValue(45.0)); // degrees
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    // Set longitude = 0 degrees, argument of latitude = 0 degrees
    mob->SetPosition(Vector(0.0, 0.0, 0.0));

    Vector pos = mob->GetPosition();

    double expectedX = ORBIT_RADIUS_M;
    double expectedY = 0.0;
    double expectedZ = 0.0;

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x, expectedX, TOLERANCE, "x coordinate at time 0 is incorrect");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.y, expectedY, TOLERANCE, "y coordinate at time 0 should be zero");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z, expectedZ, TOLERANCE, "z coordinate at time 0 should be zero");

    // The node is ascending: the satellite heads north
    NS_TEST_ASSERT_MSG_GT(mob->GetVelocity().z,
                          0.0,
                          "velocity at the ascending node should point north");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that Earth rotation causes the orbital plane to drift westward
 * in the ECEF frame.
 *
 * After exactly one orbital period T, the satellite returns to its starting
 * position on the orbit (the ascending node, argument of latitude = 0 again),
 * but the Earth has rotated eastward by angle delta = (T / 24h) * 2 * pi.  In
 * the ECEF frame, the ascending node longitude has drifted westward by delta.
 *
 * Expected ECEF position at time T (with longitude = 0 degrees at time zero):
 *   lon_T = -delta
 *   x = r * cos(lon_T)
 *   y = r * sin(lon_T)   [negative = westward drift]
 *   z = 0
 */
class LeoEarthRotationTestCase : public TestCase
{
  public:
    LeoEarthRotationTestCase();

  private:
    void DoRun() override;

    /**
     * @brief Check satellite position at the scheduled time
     * @param mob the mobility model to query
     * @param orbitalPeriod the orbital period used for expected value computation
     */
    void CheckPosition(Ptr<LeoCircularOrbitMobilityModel> mob, Time orbitalPeriod);
};

LeoEarthRotationTestCase::LeoEarthRotationTestCase()
    : TestCase("LEO orbital plane drifts westward due to Earth rotation")
{
}

void
LeoEarthRotationTestCase::CheckPosition(Ptr<LeoCircularOrbitMobilityModel> mob, Time orbitalPeriod)
{
    Vector pos = mob->GetPosition();

    // Earth rotation angle during one orbital period
    double earthRotAngle =
        (orbitalPeriod.GetDouble() / Hours(24).GetDouble()) * 2.0 * std::numbers::pi;
    double lonAtT = -earthRotAngle; // westward drift

    double expectedX = ORBIT_RADIUS_M * std::cos(lonAtT);
    double expectedY = ORBIT_RADIUS_M * std::sin(lonAtT);
    double expectedZ = 0.0;

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x,
                              expectedX,
                              TOLERANCE,
                              "x coordinate at one orbital period is incorrect");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.y,
                              expectedY,
                              TOLERANCE,
                              "y coordinate should be negative (westward drift)");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z,
                              expectedZ,
                              TOLERANCE,
                              "z coordinate at one orbital period is incorrect");

    // Verify westward drift: y must be negative
    NS_TEST_ASSERT_MSG_LT(pos.y, 0.0, "y coordinate must be negative for westward drift");
}

void
LeoEarthRotationTestCase::DoRun()
{
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));
    mob->SetAttribute("Inclination", DoubleValue(45.0));
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    mob->SetPosition(Vector(0.0, 0.0, 0.0));

    // Compute the orbital period: T = 2*pi * sqrt(r^3 / GM)
    double r3 = ORBIT_HEIGHT_KM * ORBIT_HEIGHT_KM * ORBIT_HEIGHT_KM;
    Time orbitalPeriod = Seconds(2.0 * std::numbers::pi * std::sqrt(r3 / GM_KM3_S2));

    // Check position after exactly one orbital period
    Simulator::Schedule(orbitalPeriod,
                        &LeoEarthRotationTestCase::CheckPosition,
                        this,
                        mob,
                        orbitalPeriod);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that a non-zero initial longitude offsets the orbital plane
 * correctly at time 0.
 *
 * With longitude = 90 degrees east, inclination = 45 degrees, and
 * argument of latitude = 0 degrees, the satellite is at the ascending node
 * on the equator at longitude 90, so the expected position at time 0 is:
 *   x = r * cos(90 deg) = 0
 *   y = r * sin(90 deg) = r
 *   z = 0
 */
class LeoLongitudeOffsetTestCase : public TestCase
{
  public:
    LeoLongitudeOffsetTestCase();

  private:
    void DoRun() override;
};

LeoLongitudeOffsetTestCase::LeoLongitudeOffsetTestCase()
    : TestCase("LEO non-zero initial longitude offset")
{
}

void
LeoLongitudeOffsetTestCase::DoRun()
{
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));
    mob->SetAttribute("Inclination", DoubleValue(45.0));
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    // Set longitude = 90 degrees east
    mob->SetPosition(Vector(90.0, 0.0, 0.0));

    Vector pos = mob->GetPosition();

    double expectedX = 0.0;
    double expectedY = ORBIT_RADIUS_M;
    double expectedZ = 0.0;

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x,
                              expectedX,
                              TOLERANCE,
                              "x coordinate should be near zero for longitude = 90 deg");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.y,
                              expectedY,
                              TOLERANCE,
                              "y coordinate is incorrect for longitude = 90 deg");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z,
                              expectedZ,
                              TOLERANCE,
                              "z coordinate is incorrect for longitude = 90 deg");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that a non-zero orbital offset correctly rotates the satellite
 * along its orbital plane via the Rodrigues rotation in RotatePlane().
 *
 * An argument of latitude of 0 places the satellite at the ascending node; a
 * non-zero value rotates it around the orbital plane normal. The first three
 * test cases all use 0 degrees, meaning RotatePlane() acts as an identity and
 * is not exercised. This test uses 90 degrees, which should reach the
 * northernmost point of the orbit, to verify the Rodrigues rotation.
 *
 * RotatePlane() implements the Rodrigues rotation formula
 * (see https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula ):
 *   v' = (n . v) * n + cos(a) * ((n x v) x n) + sin(a) * (n x v)
 *
 * With altitude = 1000 km, inclination = 45 degrees, longitude = 0 degrees,
 * and argument of latitude = 90 degrees, the Rodrigues rotation yields:
 *
 * The orbital plane normal is n = (0, -sqrt(2)/2, sqrt(2)/2).
 * The ascending-node position is x = (r, 0, 0).
 * Since n . x = 0, the formula simplifies:
 *   result = cos(90 deg) * ((n x x) x n) + sin(90 deg) * (n x x)
 *          = 0 + r * (0, sqrt(2)/2, sqrt(2)/2)
 *
 * Expected ECEF position: r * (0, sqrt(2)/2, sqrt(2)/2), latitude 45 degrees
 * at longitude 90, where r = (R_earth + 1000 km) in meters.
 */
class LeoOrbitalOffsetTestCase : public TestCase
{
  public:
    LeoOrbitalOffsetTestCase();

  private:
    void DoRun() override;
};

LeoOrbitalOffsetTestCase::LeoOrbitalOffsetTestCase()
    : TestCase("LEO non-zero orbital offset exercises RotatePlane")
{
}

void
LeoOrbitalOffsetTestCase::DoRun()
{
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));
    mob->SetAttribute("Inclination", DoubleValue(45.0));
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    // Set longitude = 0, argument of latitude = 90 degrees
    mob->SetPosition(Vector(0.0, 90.0, 0.0));

    Vector pos = mob->GetPosition();

    double expectedX = 0.0;
    double expectedY = ORBIT_RADIUS_M * std::cos(std::numbers::pi / 4.0);
    double expectedZ = ORBIT_RADIUS_M * std::sin(std::numbers::pi / 4.0);

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x,
                              expectedX,
                              TOLERANCE,
                              "x coordinate should be near zero for argument of latitude = 90 deg");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.y,
                              expectedY,
                              TOLERANCE,
                              "y coordinate is incorrect for argument of latitude = 90 deg");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z,
                              expectedZ,
                              TOLERANCE,
                              "z coordinate is incorrect for argument of latitude = 90 deg");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that on-demand orbital progress produces the expected position
 * after a quarter orbital period.
 *
 * Starting at the ascending node (argument of latitude = 0), after T/4 the
 * satellite should have advanced 90 degrees along its orbit, to its
 * northernmost point. This is equivalent to the LeoOrbitalOffsetTestCase
 * result (argument of latitude = 90 degrees at t=0), but verified through
 * time evolution rather than the initial value.
 *
 * At T/4, Earth rotation causes a small longitude drift that must be
 * accounted for in the expected position.
 */
class LeoOrbitalProgressTestCase : public TestCase
{
  public:
    LeoOrbitalProgressTestCase();

  private:
    void DoRun() override;

    /**
     * @brief Check satellite position at T/4
     * @param mob the mobility model to query
     * @param quarterPeriod T/4 for expected value computation
     */
    void CheckPosition(Ptr<LeoCircularOrbitMobilityModel> mob, Time quarterPeriod);
};

LeoOrbitalProgressTestCase::LeoOrbitalProgressTestCase()
    : TestCase("LEO on-demand orbital progress after quarter period")
{
}

void
LeoOrbitalProgressTestCase::CheckPosition(Ptr<LeoCircularOrbitMobilityModel> mob,
                                          Time quarterPeriod)
{
    Vector pos = mob->GetPosition();

    // After T/4, the satellite has advanced 90 degrees along its orbit.
    // Earth rotation causes a small longitude drift.
    double earthRotAngle =
        (quarterPeriod.GetDouble() / Hours(24).GetDouble()) * 2.0 * std::numbers::pi;
    double lonAtT = -earthRotAngle;

    // The ascending node, before the Rodrigues rotation, is on the equator at
    // the drifted longitude:
    //   x0 = r * cos(lon), y0 = r * sin(lon), z0 = 0
    // The plane normal at time t is:
    //   n = (sin(inc) * sin(lon), -sin(inc) * cos(lon), cos(inc))
    // After 90 degrees of Rodrigues rotation around n:
    //   Since n . x0 = 0 (the node lies in the plane),
    //   result = sin(pi/2) * (n x x0) = n x x0
    //          = r * (-cos(inc) * sin(lon), cos(inc) * cos(lon), sin(inc))
    // which is the northernmost point, at latitude inc and longitude lon + 90.
    // Computed numerically below.
    double inc = std::numbers::pi / 4.0;
    double r = ORBIT_RADIUS_M;

    double nx = std::sin(inc) * std::sin(lonAtT);
    double ny = -std::sin(inc) * std::cos(lonAtT);
    double nz = std::cos(inc);

    double x0 = r * std::cos(lonAtT);
    double y0 = r * std::sin(lonAtT);
    double z0 = 0.0;

    // n x x0
    double cx = ny * z0 - nz * y0;
    double cy = nz * x0 - nx * z0;
    double cz = nx * y0 - ny * x0;

    // After 90 degree rotation: result = sin(pi/2) * (n x x0) = (cx, cy, cz)
    double expectedX = cx;
    double expectedY = cy;
    double expectedZ = cz;

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x,
                              expectedX,
                              TOLERANCE,
                              "x coordinate after quarter orbit is incorrect");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.y,
                              expectedY,
                              TOLERANCE,
                              "y coordinate after quarter orbit is incorrect");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z,
                              expectedZ,
                              TOLERANCE,
                              "z coordinate after quarter orbit is incorrect");

    // Verify orbit radius is preserved
    double radius = std::sqrt(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
    NS_TEST_ASSERT_MSG_EQ_TOL(radius,
                              ORBIT_RADIUS_M,
                              TOLERANCE,
                              "orbit radius should be preserved after quarter period");
}

void
LeoOrbitalProgressTestCase::DoRun()
{
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));
    mob->SetAttribute("Inclination", DoubleValue(45.0));
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    mob->SetPosition(Vector(0.0, 0.0, 0.0));

    // Compute the quarter orbital period: T/4 = (pi/2) * sqrt(r^3 / GM)
    double r3 = ORBIT_HEIGHT_KM * ORBIT_HEIGHT_KM * ORBIT_HEIGHT_KM;
    Time quarterPeriod = Seconds(0.5 * std::numbers::pi * std::sqrt(r3 / GM_KM3_S2));

    Simulator::Schedule(quarterPeriod,
                        &LeoOrbitalProgressTestCase::CheckPosition,
                        this,
                        mob,
                        quarterPeriod);

    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that GetVelocity() is the rate of change of GetPosition().
 *
 * GetPosition() returns ECEF coordinates, in which the orbital plane drifts
 * westward as the Earth rotates. The velocity is compared with a central
 * difference of the position, taken 100 ms before and after the sample time;
 * the difference is within about 1e-5 m/s of the derivative at this step.
 *
 * On an equatorial orbit the speed is also known in closed form: the orbital
 * speed minus the speed of the Earth's rotation at the orbit radius.
 */
class LeoVelocityTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param inclination inclination of the orbital plane in degrees
     * @param argumentOfLatitude initial argument of latitude in degrees
     */
    LeoVelocityTestCase(double inclination, double argumentOfLatitude);

  private:
    void DoRun() override;

    /**
     * Build the test case name.
     *
     * @param inclination inclination of the orbital plane in degrees
     * @param argumentOfLatitude initial argument of latitude in degrees
     * @return the test case name
     */
    static std::string BuildName(double inclination, double argumentOfLatitude);

    double m_inclination;        ///< Inclination of the orbital plane in degrees
    double m_argumentOfLatitude; ///< Initial argument of latitude in degrees
};

LeoVelocityTestCase::LeoVelocityTestCase(double inclination, double argumentOfLatitude)
    : TestCase(BuildName(inclination, argumentOfLatitude)),
      m_inclination(inclination),
      m_argumentOfLatitude(argumentOfLatitude)
{
}

std::string
LeoVelocityTestCase::BuildName(double inclination, double argumentOfLatitude)
{
    std::ostringstream oss;
    oss << "LEO velocity is the rate of change of the position, inclination " << inclination
        << " deg, argument of latitude " << argumentOfLatitude << " deg";
    return oss.str();
}

void
LeoVelocityTestCase::DoRun()
{
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));
    mob->SetAttribute("Inclination", DoubleValue(m_inclination));
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    mob->SetPosition(Vector(0.0, m_argumentOfLatitude, 0.0));

    const Time sampleTime = Seconds(100);
    const Time halfStep = MilliSeconds(100);
    Vector before;
    Vector after;
    Vector velocity;
    Simulator::Schedule(sampleTime - halfStep, [&]() { before = mob->GetPosition(); });
    Simulator::Schedule(sampleTime, [&]() { velocity = mob->GetVelocity(); });
    Simulator::Schedule(sampleTime + halfStep, [&]() { after = mob->GetPosition(); });

    Simulator::Run();
    Simulator::Destroy();

    Vector expected = (after - before) * (1.0 / (2 * halfStep.GetSeconds()));

    NS_TEST_ASSERT_MSG_EQ_TOL(velocity.x,
                              expected.x,
                              VELOCITY_TOLERANCE,
                              "x component of the velocity differs from the position difference");
    NS_TEST_ASSERT_MSG_EQ_TOL(velocity.y,
                              expected.y,
                              VELOCITY_TOLERANCE,
                              "y component of the velocity differs from the position difference");
    NS_TEST_ASSERT_MSG_EQ_TOL(velocity.z,
                              expected.z,
                              VELOCITY_TOLERANCE,
                              "z component of the velocity differs from the position difference");

    if (m_inclination == 0.0)
    {
        double earthRotationSpeed =
            2.0 * std::numbers::pi / Hours(24).GetSeconds() * ORBIT_RADIUS_M;
        NS_TEST_ASSERT_MSG_EQ_TOL(velocity.GetLength(),
                                  mob->GetSpeed() - earthRotationSpeed,
                                  VELOCITY_TOLERANCE,
                                  "speed on an equatorial orbit should be the orbital speed "
                                  "minus the Earth rotation speed at the orbit radius");
    }
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that the orbit has the configured inclination, which
 * also fixes its direction: prograde below 90 degrees, retrograde above.
 *
 * The inclination of the orbit is the angle between the Earth's axis and the
 * orbital angular momentum r x v, where v is the velocity in the
 * non-rotating frame. That velocity is obtained from a central difference
 * of GetPosition() (ECEF), taken 100 ms before and after the sample time,
 * plus the velocity of the Earth's rotation at the satellite's position.
 */
class LeoOrbitInclinationTestCase : public TestCase
{
  public:
    /**
     * Constructor.
     *
     * @param inclination inclination of the orbital plane in degrees
     */
    LeoOrbitInclinationTestCase(double inclination);

  private:
    void DoRun() override;

    /**
     * Build the test case name.
     *
     * @param inclination inclination of the orbital plane in degrees
     * @return the test case name
     */
    static std::string BuildName(double inclination);

    double m_inclination; ///< Inclination of the orbital plane in degrees
};

LeoOrbitInclinationTestCase::LeoOrbitInclinationTestCase(double inclination)
    : TestCase(BuildName(inclination)),
      m_inclination(inclination)
{
}

std::string
LeoOrbitInclinationTestCase::BuildName(double inclination)
{
    std::ostringstream oss;
    oss << "LEO orbit has the configured inclination, inclination " << inclination << " deg";
    return oss.str();
}

void
LeoOrbitInclinationTestCase::DoRun()
{
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));
    mob->SetAttribute("Inclination", DoubleValue(m_inclination));
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    mob->SetPosition(Vector(0.0, 0.0, 0.0));

    const Time sampleTime = Seconds(100);
    const Time halfStep = MilliSeconds(100);
    Vector before;
    Vector position;
    Vector after;
    Simulator::Schedule(sampleTime - halfStep, [&]() { before = mob->GetPosition(); });
    Simulator::Schedule(sampleTime, [&]() { position = mob->GetPosition(); });
    Simulator::Schedule(sampleTime + halfStep, [&]() { after = mob->GetPosition(); });

    Simulator::Run();
    Simulator::Destroy();

    Vector ecefVelocity = (after - before) * (1.0 / (2 * halfStep.GetSeconds()));
    double earthRotationRate = 2.0 * std::numbers::pi / Hours(24).GetSeconds();
    Vector velocity = ecefVelocity + earthRotationRate * CrossProduct(Vector(0, 0, 1), position);
    Vector angularMomentum = CrossProduct(position, velocity);
    double measuredInclination =
        std::atan2(std::hypot(angularMomentum.x, angularMomentum.y), angularMomentum.z) * 180.0 /
        std::numbers::pi;

    NS_TEST_ASSERT_MSG_EQ_TOL(measuredInclination,
                              m_inclination,
                              INCLINATION_TOLERANCE,
                              "the orbit does not have the configured inclination");
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that the Walker Delta phasing factor correctly staggers
 * satellites in adjacent orbital planes.
 *
 * With NumOrbits = 2, NumSatellites = 2, and PhasingFactor = 1 (Walker
 * 4/2/1), the first satellite in plane 1 should have an orbital offset
 * that is 1 * 1 * 360 / 4 = 90 degrees greater than the first satellite
 * in plane 0.
 *
 * The allocator returns positions in order: plane 0 sat 0, plane 0 sat 1,
 * plane 1 sat 0, plane 1 sat 1.  We verify that the y-component (orbital
 * offset in degrees) of the third call includes the 90-degree phasing
 * offset.
 */
class LeoPhasingFactorTestCase : public TestCase
{
  public:
    LeoPhasingFactorTestCase();

  private:
    void DoRun() override;
};

LeoPhasingFactorTestCase::LeoPhasingFactorTestCase()
    : TestCase("LEO Walker Delta phasing factor staggers adjacent planes")
{
}

void
LeoPhasingFactorTestCase::DoRun()
{
    auto allocator = CreateObject<LeoCircularOrbitAllocator>();
    allocator->SetAttribute("NumOrbits", UintegerValue(2));
    allocator->SetAttribute("NumSatellites", UintegerValue(2));
    allocator->SetAttribute("PhasingFactor", UintegerValue(1));

    // Plane 0, satellite 0: longitude = 0, offset = 0
    Vector pos0 = allocator->GetNext();
    NS_TEST_ASSERT_MSG_EQ_TOL(pos0.x, 0.0, 0.01, "Plane 0 longitude should be 0");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos0.y, 0.0, 0.01, "Plane 0 sat 0 offset should be 0");

    // Plane 0, satellite 1: longitude = 0, offset = 180
    Vector pos1 = allocator->GetNext();
    NS_TEST_ASSERT_MSG_EQ_TOL(pos1.x, 0.0, 0.01, "Plane 0 longitude should be 0");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos1.y, 180.0, 0.01, "Plane 0 sat 1 offset should be 180");

    // Plane 1, satellite 0: longitude = 180, offset = 0 + 90 (phasing) = 90
    Vector pos2 = allocator->GetNext();
    NS_TEST_ASSERT_MSG_EQ_TOL(pos2.x, 180.0, 0.01, "Plane 1 longitude should be 180");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos2.y, 90.0, 0.01, "Plane 1 sat 0 offset should be 90 (phasing)");

    // Plane 1, satellite 1: longitude = 180, offset = 180 + 90 (phasing) = 270
    Vector pos3 = allocator->GetNext();
    NS_TEST_ASSERT_MSG_EQ_TOL(pos3.x, 180.0, 0.01, "Plane 1 longitude should be 180");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos3.y,
                              270.0,
                              0.01,
                              "Plane 1 sat 1 offset should be 270 (180 + 90 phasing)");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that LeoOrbitalShell constructor correctly handles Delta and Star parameters.
 */
class LeoOrbitConstructorTestCase : public TestCase
{
  public:
    LeoOrbitConstructorTestCase();

  private:
    void DoRun() override;
};

LeoOrbitConstructorTestCase::LeoOrbitConstructorTestCase()
    : TestCase("LEO Orbit constructor handles parameters")
{
}

void
LeoOrbitConstructorTestCase::DoRun()
{
    // Walker Delta
    LeoOrbitalShell delta(1000.0, 50.0, 2, 12, 1.0);
    NS_TEST_ASSERT_MSG_EQ_TOL(delta.alt, 1000.0, 0.01, "Altitude mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(delta.inc, 50.0, 0.01, "Inclination mismatch");
    NS_TEST_ASSERT_MSG_EQ(delta.planes, 2, "Planes mismatch");
    NS_TEST_ASSERT_MSG_EQ(delta.sats, 12, "Sats mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(delta.phasing, 1.0, 0.01, "Phasing mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(delta.raanSpanDeg, 360.0, 0.01, "RAAN span mismatch");

    // Walker Star
    LeoOrbitalShell star(1000.0, 50.0, 2, 12, 1.0, 180.0);
    NS_TEST_ASSERT_MSG_EQ_TOL(star.alt, 1000.0, 0.01, "Altitude mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(star.inc, 50.0, 0.01, "Inclination mismatch");
    NS_TEST_ASSERT_MSG_EQ(star.planes, 2, "Planes mismatch");
    NS_TEST_ASSERT_MSG_EQ(star.sats, 12, "Sats mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(star.phasing, 1.0, 0.01, "Phasing mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(star.raanSpanDeg, 180.0, 0.01, "RAAN span mismatch");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that Walker Star constellations (RaanSpanDeg = 180) space
 * orbital planes over 180 degrees instead of 360 degrees.
 *
 * With NumOrbits = 2, NumSatellites = 1, and RaanSpanDeg = 180, the two
 * planes should be separated by 90 degrees (180 / 2) rather than
 * 180 degrees (360 / 2) as in Walker Delta.
 */
class LeoWalkerStarTestCase : public TestCase
{
  public:
    LeoWalkerStarTestCase();

  private:
    void DoRun() override;
};

LeoWalkerStarTestCase::LeoWalkerStarTestCase()
    : TestCase("LEO Walker Star spaces planes over 180 degrees")
{
}

void
LeoWalkerStarTestCase::DoRun()
{
    auto allocator = CreateObject<LeoCircularOrbitAllocator>();
    allocator->SetAttribute("NumOrbits", UintegerValue(2));
    allocator->SetAttribute("NumSatellites", UintegerValue(1));
    allocator->SetAttribute("RaanSpanDeg", DoubleValue(180.0));

    // Plane 0: RAAN = 0
    Vector pos0 = allocator->GetNext();
    NS_TEST_ASSERT_MSG_EQ_TOL(pos0.x, 0.0, 0.01, "Plane 0 RAAN should be 0");

    // Plane 1: RAAN = 90 (180 / 2)
    Vector pos1 = allocator->GetNext();
    NS_TEST_ASSERT_MSG_EQ_TOL(pos1.x,
                              90.0,
                              0.01,
                              "Plane 1 RAAN should be 90 (180 / 2) for Walker Star");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that LeoCircularOrbitAllocator::GetNextOrbitPosition() returns
 * the correct structured values matching GetNext() and DoSetPosition().
 */
class LeoOrbitPositionStructTestCase : public TestCase
{
  public:
    LeoOrbitPositionStructTestCase();

  private:
    void DoRun() override;
};

LeoOrbitPositionStructTestCase::LeoOrbitPositionStructTestCase()
    : TestCase("LEO allocator GetNextOrbitPosition returns correct struct")
{
}

void
LeoOrbitPositionStructTestCase::DoRun()
{
    auto allocator = CreateObject<LeoCircularOrbitAllocator>();
    allocator->SetAttribute("NumOrbits", UintegerValue(2));
    allocator->SetAttribute("NumSatellites", UintegerValue(2));
    allocator->SetAttribute("PhasingFactor", UintegerValue(1));
    allocator->SetAttribute("RaanSpanDeg", DoubleValue(360.0));

    // Plane 0, satellite 0: longitude = 0, argumentOfLatitude = 0
    auto params0 = allocator->GetNextOrbitPosition();
    NS_TEST_ASSERT_MSG_EQ_TOL(params0.longitude, 0.0, 0.01, "Plane 0 longitude should be 0");
    NS_TEST_ASSERT_MSG_EQ_TOL(params0.argumentOfLatitude,
                              0.0,
                              0.01,
                              "Plane 0 sat 0 argLat should be 0");
    NS_TEST_ASSERT_MSG_EQ_TOL(params0.satelliteIndex, 0.0, 0.01, "Plane 0 sat 0 index should be 0");

    // Plane 0, satellite 1: longitude = 0, argumentOfLatitude = 180
    auto params1 = allocator->GetNextOrbitPosition();
    NS_TEST_ASSERT_MSG_EQ_TOL(params1.longitude, 0.0, 0.01, "Plane 0 longitude should be 0");
    NS_TEST_ASSERT_MSG_EQ_TOL(params1.argumentOfLatitude,
                              180.0,
                              0.01,
                              "Plane 0 sat 1 argLat should be 180");

    // Plane 1, satellite 0: longitude = 180, argumentOfLatitude = 0 + 90 (phasing) = 90
    auto params2 = allocator->GetNextOrbitPosition();
    NS_TEST_ASSERT_MSG_EQ_TOL(params2.longitude, 180.0, 0.01, "Plane 1 longitude should be 180");
    NS_TEST_ASSERT_MSG_EQ_TOL(params2.argumentOfLatitude,
                              90.0,
                              0.01,
                              "Plane 1 sat 0 argLat should be 90");

    // Plane 1, satellite 1: longitude = 180, argumentOfLatitude = 180 + 90 = 270
    auto params3 = allocator->GetNextOrbitPosition();
    NS_TEST_ASSERT_MSG_EQ_TOL(params3.longitude, 180.0, 0.01, "Plane 1 longitude should be 180");
    NS_TEST_ASSERT_MSG_EQ_TOL(params3.argumentOfLatitude,
                              270.0,
                              0.01,
                              "Plane 1 sat 1 argLat should be 270");

    // Verify backward compat: GetNext() produces same fields as GetNextOrbitPosition
    auto allocCompat = CreateObject<LeoCircularOrbitAllocator>();
    allocCompat->SetAttribute("NumOrbits", UintegerValue(2));
    allocCompat->SetAttribute("NumSatellites", UintegerValue(2));
    allocCompat->SetAttribute("PhasingFactor", UintegerValue(1));
    allocCompat->SetAttribute("RaanSpanDeg", DoubleValue(360.0));

    auto vecFirst = allocCompat->GetNext();
    NS_TEST_ASSERT_MSG_EQ_TOL(vecFirst.x, 0.0, 0.01, "GetNext x (longitude) should be 0");
    NS_TEST_ASSERT_MSG_EQ_TOL(vecFirst.y, 0.0, 0.01, "GetNext y (argLat) should be 0");
    NS_TEST_ASSERT_MSG_EQ_TOL(vecFirst.z, 0.0, 0.01, "GetNext z (satIndex) should be 0");

    // Test DoSetPosition structured bindings: create a mobility model
    // and verify it correctly extracts longitude/argumentOfLatitude
    auto node = CreateObject<Node>();
    auto mob = CreateObject<LeoCircularOrbitMobilityModel>();
    mob->SetAttribute("Altitude", DoubleValue(1000.0));
    mob->SetAttribute("Inclination", DoubleValue(45.0));
    mob->SetAttribute("Resolution", TimeValue(Seconds(0)));
    node->AggregateObject(mob);

    // Use GetNextOrbitPosition to get the first position, then SetPosition with it
    auto paramsFirst = allocator->GetNextOrbitPosition();
    Vector setPositionVec{paramsFirst.longitude, paramsFirst.argumentOfLatitude, 0.0};
    mob->SetPosition(setPositionVec);

    // Verify internal state by checking GetPosition
    Vector pos = mob->GetPosition();

    // The satellite should be at the ascending node, on the equator at longitude 0
    double expectedX = ORBIT_RADIUS_M;
    double expectedZ = 0.0;

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x,
                              expectedX,
                              TOLERANCE,
                              "x coordinate after SetPosition is incorrect");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z,
                              expectedZ,
                              TOLERANCE,
                              "z coordinate after SetPosition is incorrect");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test LeoOrbitalShell serialization and deserialization round-trip.
 *
 * Verifies that a LeoOrbitalShell serialized to CSV text can be deserialized back
 * and that all six fields (altitude, inclination, planes, sats, phasing,
 * RAAN span) are preserved exactly.
 */
class LeoOrbitSerializationTestCase : public TestCase
{
  public:
    LeoOrbitSerializationTestCase();

  private:
    void DoRun() override;
};

LeoOrbitSerializationTestCase::LeoOrbitSerializationTestCase()
    : TestCase("LEO Orbit serialization round-trip preserves all fields")
{
}

void
LeoOrbitSerializationTestCase::DoRun()
{
    // Walker Delta orbit with all six fields set
    LeoOrbitalShell original(700.0, 98.0, 2, 12, 3.0, 360.0);

    // Serialize to text
    std::ostringstream oss;
    oss << original;
    std::string csvText = oss.str();

    // Expected format: "700,98,2,12,3,360"
    NS_TEST_ASSERT_MSG_EQ(csvText, "700,98,2,12,3,360", "Serialized CSV text mismatch");

    // Deserialize back
    std::istringstream iss(csvText);
    LeoOrbitalShell restored;
    iss >> restored;

    // Verify all fields match
    NS_TEST_ASSERT_MSG_EQ_TOL(restored.alt,
                              original.alt,
                              0.01,
                              "Altitude mismatch after round-trip");
    NS_TEST_ASSERT_MSG_EQ_TOL(restored.inc,
                              original.inc,
                              0.01,
                              "Inclination mismatch after round-trip");
    NS_TEST_ASSERT_MSG_EQ(restored.planes, original.planes, "Planes mismatch after round-trip");
    NS_TEST_ASSERT_MSG_EQ(restored.sats, original.sats, "Sats mismatch after round-trip");
    NS_TEST_ASSERT_MSG_EQ_TOL(restored.phasing,
                              original.phasing,
                              0.01,
                              "Phasing mismatch after round-trip");
    NS_TEST_ASSERT_MSG_EQ_TOL(restored.raanSpanDeg,
                              original.raanSpanDeg,
                              0.01,
                              "RAAN span mismatch after round-trip");

    // Test Walker Star orbit (raanSpanDeg = 180)
    LeoOrbitalShell starOriginal(550.0, 53.0, 4, 8, 2.0, 180.0);
    std::ostringstream oss2;
    oss2 << starOriginal;
    std::string csvText2 = oss2.str();

    std::istringstream iss2(csvText2);
    LeoOrbitalShell starRestored;
    iss2 >> starRestored;

    NS_TEST_ASSERT_MSG_EQ_TOL(starRestored.alt, starOriginal.alt, 0.01, "Star altitude mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(starRestored.inc,
                              starOriginal.inc,
                              0.01,
                              "Star inclination mismatch");
    NS_TEST_ASSERT_MSG_EQ(starRestored.planes, starOriginal.planes, "Star planes mismatch");
    NS_TEST_ASSERT_MSG_EQ(starRestored.sats, starOriginal.sats, "Star sats mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(starRestored.phasing,
                              starOriginal.phasing,
                              0.01,
                              "Star phasing mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(starRestored.raanSpanDeg,
                              starOriginal.raanSpanDeg,
                              0.01,
                              "Star RAAN span mismatch");

    // Test default values (only 4 columns)
    LeoOrbitalShell defaultOriginal(800.0, 45.0, 1, 6); // phasing=0, raanSpanDeg=360 by default
    std::ostringstream oss3;
    oss3 << defaultOriginal;
    std::string csvText3 = oss3.str();

    std::istringstream iss3(csvText3);
    LeoOrbitalShell defaultRestored;
    iss3 >> defaultRestored;

    NS_TEST_ASSERT_MSG_EQ_TOL(defaultRestored.alt,
                              defaultOriginal.alt,
                              0.01,
                              "Default altitude mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(defaultRestored.inc,
                              defaultOriginal.inc,
                              0.01,
                              "Default inclination mismatch");
    NS_TEST_ASSERT_MSG_EQ(defaultRestored.planes,
                          defaultOriginal.planes,
                          "Default planes mismatch");
    NS_TEST_ASSERT_MSG_EQ(defaultRestored.sats, defaultOriginal.sats, "Default sats mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(defaultRestored.phasing,
                              defaultOriginal.phasing,
                              0.01,
                              "Default phasing mismatch");
    NS_TEST_ASSERT_MSG_EQ_TOL(defaultRestored.raanSpanDeg,
                              defaultOriginal.raanSpanDeg,
                              0.01,
                              "Default RAAN span mismatch");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief LEO mobility test suite
 */
class LeoMobilityTestSuite : public TestSuite
{
  public:
    LeoMobilityTestSuite();
};

LeoMobilityTestSuite::LeoMobilityTestSuite()
    : TestSuite("leo-mobility", Type::UNIT)
{
    AddTestCase(new LeoInitialPositionTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoEarthRotationTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoLongitudeOffsetTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitalOffsetTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitalProgressTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoVelocityTestCase(0.0, 0.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoVelocityTestCase(45.0, 0.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoVelocityTestCase(45.0, 90.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoVelocityTestCase(98.0, 0.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitInclinationTestCase(45.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitInclinationTestCase(98.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitInclinationTestCase(135.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitInclinationTestCase(180.0), TestCase::Duration::QUICK);
    AddTestCase(new LeoPhasingFactorTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoWalkerStarTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitConstructorTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitSerializationTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitPositionStructTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static LeoMobilityTestSuite g_leoMobilityTestSuite;
