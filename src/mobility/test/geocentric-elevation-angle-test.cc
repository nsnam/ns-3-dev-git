/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/geocentric-constant-position-mobility-model.h"
#include "ns3/geographic-positions.h"
#include "ns3/test.h"

#include <cmath>

using namespace ns3;

/**
 * @ingroup mobility-test
 *
 * @brief Tests GeocentricConstantPositionMobilityModel::GetElevationAngle (@issueid{1308}).
 *
 * The elevation angle is measured at the lower terminal, with the local
 * vertical given by the geocentric radial direction (the model uses a perfect
 * sphere). The angle must be signed (negative when the other terminal is
 * below the local horizon), independent of the argument order, and identical
 * for mirrored geometries in either hemisphere. This test pins that behavior.
 */
class GeocentricElevationAngleTestCase : public TestCase
{
  public:
    GeocentricElevationAngleTestCase()
        : TestCase("GeocentricConstantPositionMobilityModel elevation angle")
    {
    }

  private:
    void DoRun() override;

    /// Build a model at the given geographic position.
    /// @param lat latitude (deg)
    /// @param lon longitude (deg)
    /// @param alt altitude (m)
    /// @return the configured mobility model
    static Ptr<GeocentricConstantPositionMobilityModel> Make(double lat, double lon, double alt)
    {
        auto m = CreateObject<GeocentricConstantPositionMobilityModel>();
        m->SetGeographicPosition(Vector(lat, lon, alt));
        return m;
    }
};

void
GeocentricElevationAngleTestCase::DoRun()
{
    constexpr double TOL = 1e-2; // degrees
    const double geoAlt = 35786e3;
    const double leoAlt = 600e3;

    // A satellite directly above the ground station is at zenith: 90 degrees.
    auto gs = Make(45.0, 0.0, 0.0);
    auto overhead = Make(45.0, 0.0, geoAlt);
    NS_TEST_ASSERT_MSG_EQ_TOL(gs->GetElevationAngle(overhead),
                              90.0,
                              TOL,
                              "Satellite at zenith should have 90 degrees elevation");

    // GEO satellite over the equator seen from a ground station at 45 N.
    auto geoSat = Make(0.0, 0.0, geoAlt);
    double elevFromGs = gs->GetElevationAngle(geoSat);
    NS_TEST_ASSERT_MSG_EQ_TOL(elevFromGs, 38.1771, TOL, "Wrong elevation for GEO over the equator");
    // The result must be independent of the argument order.
    NS_TEST_ASSERT_MSG_EQ_TOL(geoSat->GetElevationAngle(gs),
                              elevFromGs,
                              TOL,
                              "Elevation angle must not depend on the argument order");

    // A terminal below the local horizon must yield a negative elevation.
    auto equatorGs = Make(0.0, 0.0, 0.0);
    auto farSat = Make(0.0, 120.0, leoAlt);
    NS_TEST_ASSERT_MSG_EQ_TOL(equatorGs->GetElevationAngle(farSat),
                              -58.5127,
                              TOL,
                              "Below-horizon terminal should have a negative elevation");

    // Southern-hemisphere mirror of the GEO case: the geometry is symmetric,
    // so the elevation must match the northern result.
    auto southGs = Make(-45.0, 0.0, 0.0);
    NS_TEST_ASSERT_MSG_EQ_TOL(southGs->GetElevationAngle(geoSat),
                              elevFromGs,
                              TOL,
                              "Southern-hemisphere GEO elevation must mirror the northern one");

    // Oblique LEO geometry at high latitude and its southern mirror: the
    // endpoints have genuinely different local verticals, so hemisphere
    // symmetry is a strong discriminator for the vertex selection.
    auto northHighGs = Make(60.0, 0.0, 0.0);
    auto northLeo = Make(50.0, 0.0, leoAlt);
    double northElev = northHighGs->GetElevationAngle(northLeo);
    NS_TEST_ASSERT_MSG_EQ_TOL(northElev, 22.2040, TOL, "Wrong elevation for oblique LEO geometry");
    auto southHighGs = Make(-60.0, 0.0, 0.0);
    auto southLeo = Make(-50.0, 0.0, leoAlt);
    NS_TEST_ASSERT_MSG_EQ_TOL(southHighGs->GetElevationAngle(southLeo),
                              northElev,
                              TOL,
                              "North and south mirror geometries must have equal elevation");

    // Longitude rotational invariance: shifting both terminals by the same
    // longitude must not change the (purely relative) geometry.
    auto rotatedGs = Make(60.0, 130.0, 0.0);
    auto rotatedLeo = Make(50.0, 130.0, leoAlt);
    NS_TEST_ASSERT_MSG_EQ_TOL(rotatedGs->GetElevationAngle(rotatedLeo),
                              northElev,
                              TOL,
                              "Elevation must be invariant under a common longitude shift");

    // Exact horizon: place the satellite so the line of sight grazes the
    // sphere and the ground station sees it at exactly 0 degrees, pinning the
    // sign boundary between visible and below-horizon terminals.
    const double horizonLon = std::acos(GeographicPositions::EARTH_SPHERE_RADIUS /
                                        (GeographicPositions::EARTH_SPHERE_RADIUS + leoAlt)) *
                              180.0 / M_PI;
    auto horizonSat = Make(0.0, horizonLon, leoAlt);
    NS_TEST_ASSERT_MSG_EQ_TOL(equatorGs->GetElevationAngle(horizonSat),
                              0.0,
                              TOL,
                              "Satellite at the geometric horizon should have 0 degrees elevation");

    // Zenith sanity sweep across latitudes. Zenith is degenerate (both radial
    // directions coincide), so this is an invariant check.
    for (double lat : {-80.0, -30.0, 0.0, 30.0, 80.0})
    {
        NS_TEST_ASSERT_MSG_EQ_TOL(Make(lat, 0.0, 0.0)->GetElevationAngle(Make(lat, 0.0, leoAlt)),
                                  90.0,
                                  TOL,
                                  "Zenith satellite should have 90 degrees elevation at latitude "
                                      << lat);
    }
}

/**
 * @ingroup mobility-test
 *
 * @brief Geocentric elevation angle test suite.
 */
class GeocentricElevationAngleTestSuite : public TestSuite
{
  public:
    GeocentricElevationAngleTestSuite()
        : TestSuite("geocentric-elevation-angle", Type::UNIT)
    {
        AddTestCase(new GeocentricElevationAngleTestCase, TestCase::Duration::QUICK);
    }
};

static GeocentricElevationAngleTestSuite
    g_geocentricElevationAngleTestSuite; //!< Static variable for test initialization
