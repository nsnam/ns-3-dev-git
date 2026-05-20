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
 * orbital offset = 0 degrees, the expected ECEF position at time 0 is:
 *   x = r * cos(45 deg) = r * sqrt(2)/2
 *   y = 0
 *   z = r * sin(45 deg) = r * sqrt(2)/2
 * where r = (R_earth + 1000 km) in meters.
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

    // Set longitude = 0 degrees, orbital offset = 0 degrees
    mob->SetPosition(Vector(0.0, 0.0, 0.0));

    Vector pos = mob->GetPosition();

    double expectedX = ORBIT_RADIUS_M * std::cos(std::numbers::pi / 4.0);
    double expectedY = 0.0;
    double expectedZ = ORBIT_RADIUS_M * std::sin(std::numbers::pi / 4.0);

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x, expectedX, TOLERANCE, "x coordinate at time 0 is incorrect");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.y, expectedY, TOLERANCE, "y coordinate at time 0 should be zero");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z, expectedZ, TOLERANCE, "z coordinate at time 0 is incorrect");

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
 * position on the orbit (orbital offset = 0 again), but the Earth has rotated
 * eastward by angle delta = (T / 24h) * 2 * pi.  In the ECEF frame, the
 * ascending node longitude has drifted westward by delta.
 *
 * Expected ECEF position at time T (with longitude = 0 degrees at time zero):
 *   lon_T = -delta
 *   x = r * cos(45 deg) * cos(lon_T)
 *   y = r * cos(45 deg) * sin(lon_T)   [negative = westward drift]
 *   z = r * sin(45 deg)
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

    double expectedX = ORBIT_RADIUS_M * std::cos(std::numbers::pi / 4.0) * std::cos(lonAtT);
    double expectedY = ORBIT_RADIUS_M * std::cos(std::numbers::pi / 4.0) * std::sin(lonAtT);
    double expectedZ = ORBIT_RADIUS_M * std::sin(std::numbers::pi / 4.0);

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
 * orbital offset = 0 degrees, the expected position at time 0 is:
 *   x = r * cos(45 deg) * cos(90 deg) = 0
 *   y = r * cos(45 deg) * sin(90 deg) = r * sqrt(2)/2
 *   z = r * sin(45 deg)               = r * sqrt(2)/2
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
    double expectedY = ORBIT_RADIUS_M * std::cos(std::numbers::pi / 4.0);
    double expectedZ = ORBIT_RADIUS_M * std::sin(std::numbers::pi / 4.0);

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
 * An offset of 0 places the satellite at the ascending node; a non-zero
 * offset rotates it around the orbital plane normal. The first three test
 * cases all use offset = 0 degrees, meaning RotatePlane() acts as an
 * identity and is not exercised. This test uses offset = 90 degrees to
 * verify the Rodrigues rotation.
 *
 * RotatePlane() implements the Rodrigues rotation formula
 * (see https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula ):
 *   v' = (n . v) * n + cos(a) * ((n x v) x n) + sin(a) * (n x v)
 *
 * With altitude = 1000 km, inclination = 45 degrees, longitude = 0 degrees,
 * and orbital offset = 90 degrees, the Rodrigues rotation yields:
 *
 * The orbital plane normal is n = (-sqrt(2)/2, 0, sqrt(2)/2).
 * The ascending-node position is x = r * (sqrt(2)/2, 0, sqrt(2)/2).
 * Since n . x = 0, the formula simplifies:
 *   result = cos(90 deg) * ((n x x) x n) + sin(90 deg) * (n x x)
 *          = 0 + (0, r, 0)
 *          = (0, r, 0)
 *
 * Expected ECEF position: (0, r, 0) where r = (R_earth + 1000 km) in meters.
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

    // Set longitude = 0, orbital offset = 90 degrees
    mob->SetPosition(Vector(0.0, 90.0, 0.0));

    Vector pos = mob->GetPosition();

    double expectedX = 0.0;
    double expectedY = ORBIT_RADIUS_M;
    double expectedZ = 0.0;

    NS_TEST_ASSERT_MSG_EQ_TOL(pos.x,
                              expectedX,
                              TOLERANCE,
                              "x coordinate should be near zero for orbital offset = 90 deg");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.y,
                              expectedY,
                              TOLERANCE,
                              "y coordinate should equal orbit radius for orbital offset = 90 deg");
    NS_TEST_ASSERT_MSG_EQ_TOL(pos.z,
                              expectedZ,
                              TOLERANCE,
                              "z coordinate should be near zero for orbital offset = 90 deg");

    Simulator::Destroy();
}

/**
 * @ingroup mobility-test
 * @ingroup leo
 *
 * @brief Test that on-demand orbital progress produces the expected position
 * after a quarter orbital period.
 *
 * Starting at offset = 0 (ascending node), after T/4 the satellite should
 * have advanced 90 degrees along its orbit. This is equivalent to the
 * LeoOrbitalOffsetTestCase result (offset = 90 degrees at t=0), but
 * verified through time evolution rather than initial offset.
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

    // At offset = 0 degrees, the ascending-node position before Rodrigues rotation is:
    //   x0 = r * cos(inc) * cos(lon), y0 = r * cos(inc) * sin(lon), z0 = r * sin(inc)
    // After 90 degrees of Rodrigues rotation around the plane normal n:
    //   Since n . x0 = 0 (ascending node is perpendicular to normal),
    //   result = sin(pi/2) * (n x x0) = n x x0
    // The plane normal at time t is:
    //   n = (-sin(inc) * cos(lon), -sin(inc) * sin(lon), cos(inc))
    double inc = std::numbers::pi / 4.0;
    double r = ORBIT_RADIUS_M;

    // Compute n x x0 analytically:
    // n = (-sin(inc) * cos(lon), -sin(inc) * sin(lon), cos(inc))
    // x0 = (r * cos(inc) * cos(lon), r * cos(inc) * sin(lon), r * sin(inc))
    // n . x0 = 0 (verified: -sin * cos^2 * cos^2(lon) - sin * cos^2 * sin^2(lon) + cos * sin = 0
    //           when sin * cos = sin * cos, which holds for any inc)
    // n x x0 = r * (cos(inc) * sin(lon) * cos(inc) - sin(inc) * (-sin(inc) * sin(lon)),
    //              sin(inc) * (-sin(inc) * cos(lon)) - (-sin(inc) * cos(lon)) * cos(inc)  ... )
    // This simplifies to (0, r, 0) only at lon = 0 degrees. At lon != 0, it is more complex.
    // Use a direct numerical computation instead.
    double nx = -std::sin(inc) * std::cos(lonAtT);
    double ny = -std::sin(inc) * std::sin(lonAtT);
    double nz = std::cos(inc);

    double x0 = r * std::cos(inc) * std::cos(lonAtT);
    double y0 = r * std::cos(inc) * std::sin(lonAtT);
    double z0 = r * std::sin(inc);

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

    // The satellite should be at the ascending node with the correct inclination
    double expectedX = ORBIT_RADIUS_M * std::cos(std::numbers::pi / 4.0);
    double expectedZ = ORBIT_RADIUS_M * std::sin(std::numbers::pi / 4.0);

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
    AddTestCase(new LeoPhasingFactorTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoWalkerStarTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitConstructorTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitSerializationTestCase, TestCase::Duration::QUICK);
    AddTestCase(new LeoOrbitPositionStructTestCase, TestCase::Duration::QUICK);
}

/// Static variable for test initialization
static LeoMobilityTestSuite g_leoMobilityTestSuite;
