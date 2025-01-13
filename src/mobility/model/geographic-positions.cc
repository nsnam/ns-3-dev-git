/*
 * Copyright (c) 2014 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Benjamin Cizdziel <ben.cizdziel@gmail.com>
 */

#include "geographic-positions.h"

#include "ns3/angles.h"
#include "ns3/log.h"

#include <cmath>

NS_LOG_COMPONENT_DEFINE("GeographicPositions");

namespace
{
/**
 * GRS80 and WGS84 sources
 *
 * Moritz, H. "Geodetic Reference System 1980." GEODETIC REFERENCE SYSTEM 1980.
 * <https://web.archive.org/web/20170712034716/http://www.gfy.ku.dk/~iag/HB2000/part4/grs80_corr.htm>.
 *
 * "Department of Defense World Geodetic System 1984." National Imagery and
 * Mapping Agency, 1 Jan. 2000.
 * <https://web.archive.org/web/20200730231853/http://earth-info.nga.mil/GandG/publications/tr8350.2/wgs84fin.pdf>.
 */

/**
 * @brief  Lambda function for computing the curvature
 */
auto curvature = [](double e, double ph) { return sqrt(1 - e * e * sin(ph) * sin(ph)); };
} // namespace

namespace ns3
{

Vector
GeographicPositions::GeographicToCartesianCoordinates(double latitude,
                                                      double longitude,
                                                      double altitude,
                                                      EarthSpheroidType sphType)
{
    NS_LOG_FUNCTION_NOARGS();
    double latitudeRadians = DegreesToRadians(latitude);
    double longitudeRadians = DegreesToRadians(longitude);

    // Retrieve radius, first eccentricity and flattening according to the specified Earth's model
    auto [a, e, f] = GetRadiusEccentFlat(sphType);

    double Rn = a / curvature(e, latitudeRadians); // radius of curvature
    double x = (Rn + altitude) * cos(latitudeRadians) * cos(longitudeRadians);
    double y = (Rn + altitude) * cos(latitudeRadians) * sin(longitudeRadians);
    double z = ((1 - e * e) * Rn + altitude) * sin(latitudeRadians);
    Vector cartesianCoordinates = Vector(x, y, z);
    return cartesianCoordinates;
}

Vector
GeographicPositions::CartesianToGeographicCoordinates(Vector pos, EarthSpheroidType sphType)
{
    NS_LOG_FUNCTION(pos << sphType);

    // Retrieve radius, first eccentricity and flattening according to the specified Earth's model
    auto [a, e, f] = GetRadiusEccentFlat(sphType);

    Vector lla;
    Vector tmp;
    lla.y = atan2(pos.y, pos.x); // longitude (rad), in +/- pi

    double e2 = e * e;
    // sqrt (pos.x^2 + pos.y^2)
    double p = CalculateDistance(pos, {0, 0, pos.z});
    lla.x = atan2(pos.z, p * (1 - e2)); // init latitude (rad), in +/- pi

    do
    {
        tmp = lla;
        double N = a / sqrt(1 - e2 * sin(tmp.x) * sin(tmp.x));
        double v = p / cos(tmp.x);
        lla.z = v - N; // altitude
        lla.x = atan2(pos.z, p * (1 - e2 * N / v));
    }
    // 1 m difference is approx 1 / 30 arc seconds = 9.26e-6 deg
    while (fabs(lla.x - tmp.x) > DegreesToRadians(0.00000926));

    lla.x = RadiansToDegrees(lla.x);
    lla.y = RadiansToDegrees(lla.y);

    // canonicalize (latitude) x in [-90, 90] and (longitude) y in [-180, 180)
    if (lla.x > 90.0)
    {
        lla.x = 180 - lla.x;
        lla.y += lla.y < 0 ? 180 : -180;
    }
    else if (lla.x < -90.0)
    {
        lla.x = -180 - lla.x;
        lla.y += lla.y < 0 ? 180 : -180;
    }
    if (lla.y == 180.0)
    {
        lla.y = -180;
    }

    // make sure lat/lon in the right range to double check canonicalization
    // and conversion routine
    NS_ASSERT_MSG(-180.0 <= lla.y, "Conversion error: longitude too negative");
    NS_ASSERT_MSG(180.0 > lla.y, "Conversion error: longitude too positive");
    NS_ASSERT_MSG(-90.0 <= lla.x, "Conversion error: latitude too negative");
    NS_ASSERT_MSG(90.0 >= lla.x, "Conversion error: latitude too positive");

    return lla;
}

Vector
GeographicPositions::GeographicToTopocentricCoordinates(Vector pos,
                                                        Vector refPoint,
                                                        EarthSpheroidType sphType)
{
    NS_LOG_FUNCTION(pos << sphType);

    double phi = DegreesToRadians(pos.x);
    double lambda = DegreesToRadians(pos.y);
    double h = pos.z;
    double phi0 = DegreesToRadians(refPoint.x);
    double lambda0 = DegreesToRadians(refPoint.y);
    double h0 = refPoint.z;
    // Retrieve radius, first eccentricity and flattening according to the specified Earth's model
    auto [a, e, f] = GetRadiusEccentFlat(sphType);

    // the radius of curvature in the prime vertical at latitude
    double v = a / curvature(e, phi);
    // the radius of curvature in the prime vertical at latitude
    // of the reference point
    double v0 = a / curvature(e, phi0);

    double U = (v + h) * cos(phi) * sin(lambda - lambda0);
    double V = (v + h) * (sin(phi) * cos(phi0) - cos(phi) * sin(phi0) * cos(lambda - lambda0)) +
               e * e * (v0 * sin(phi0) - v * sin(phi)) * cos(phi0);
    double W = (v + h) * (sin(phi) * sin(phi0) + cos(phi) * cos(phi0) * cos(lambda - lambda0)) +
               e * e * (v0 * sin(phi0) - v * sin(phi)) * sin(phi0) - (v0 + h0);

    Vector topocentricCoordinates = Vector(U, V, W);
    return topocentricCoordinates;
}

Vector
GeographicPositions::TopocentricToGeographicCoordinates(Vector pos,
                                                        Vector refPoint,
                                                        EarthSpheroidType sphType)
{
    NS_LOG_FUNCTION(pos << sphType);

    double U = pos.x;
    double V = pos.y;
    double W = pos.z;
    double phi0 = DegreesToRadians(refPoint.x);
    double lambda0 = DegreesToRadians(refPoint.y);
    double h0 = refPoint.z;
    // Retrieve radius, first eccentricity and flattening according to the specified Earth's model
    auto [a, e, f] = GetRadiusEccentFlat(sphType);

    // the radius of curvature in the prime vertical at latitude
    // of the reference point
    double v0 = a / curvature(e, phi0);

    double X0 = (v0 + h0) * cos(phi0) * cos(lambda0);
    double Y0 = (v0 + h0) * cos(phi0) * sin(lambda0);
    double Z0 = ((1 - e * e) * v0 + h0) * sin(phi0);

    double X = X0 - U * sin(lambda0) - V * sin(phi0) * cos(lambda0) + W * cos(phi0) * cos(lambda0);
    double Y = Y0 + U * cos(lambda0) - V * sin(phi0) * sin(lambda0) + W * cos(phi0) * sin(lambda0);
    double Z = Z0 + V * cos(phi0) + W * sin(phi0);

    double epsilon = e * e / (1 - e * e);
    double b = a * (1 - f);
    double p = sqrt(X * X + Y * Y);
    double q = atan2((Z * a), (p * b));

    double phi = atan2((Z + epsilon * b * pow(sin(q), 3)), (p - e * e * a * pow(cos(q), 3)));
    double lambda = atan2(Y, X);

    double v = a / curvature(e, phi);
    double h = (p / cos(phi)) - v;

    phi = RadiansToDegrees(phi);
    lambda = RadiansToDegrees(lambda);

    Vector geographicCoordinates = Vector(phi, lambda, h);
    return geographicCoordinates;
}

std::list<Vector>
GeographicPositions::RandCartesianPointsAroundGeographicPoint(double originLatitude,
                                                              double originLongitude,
                                                              double maxAltitude,
                                                              int numPoints,
                                                              double maxDistFromOrigin,
                                                              Ptr<UniformRandomVariable> uniRand)
{
    NS_LOG_FUNCTION_NOARGS();
    // fixes divide by zero case and limits latitude bounds
    if (originLatitude >= 90)
    {
        NS_LOG_WARN("origin latitude must be less than 90. setting to 89.999");
        originLatitude = 89.999;
    }
    else if (originLatitude <= -90)
    {
        NS_LOG_WARN("origin latitude must be greater than -90. setting to -89.999");
        originLatitude = -89.999;
    }

    // restricts maximum altitude from being less than zero (below earth's surface).
    // sets maximum altitude equal to zero if parameter is set to be less than zero.
    if (maxAltitude < 0)
    {
        NS_LOG_WARN("maximum altitude must be greater than or equal to 0. setting to 0");
        maxAltitude = 0;
    }

    double originLatitudeRadians = DegreesToRadians(originLatitude);
    double originLongitudeRadians = DegreesToRadians(originLongitude);
    double originColatitude = (M_PI_2)-originLatitudeRadians;

    // maximum alpha allowed (arc length formula)
    double a = maxDistFromOrigin / EARTH_SPHERE_RADIUS;
    if (a > M_PI)
    {
        // pi is largest alpha possible (polar angle from origin that
        // points can be generated within)
        a = M_PI;
    }

    std::list<Vector> generatedPoints;
    for (int i = 0; i < numPoints; i++)
    {
        // random distance from North Pole (towards center of earth)
        double d = uniRand->GetValue(0, EARTH_SPHERE_RADIUS - EARTH_SPHERE_RADIUS * cos(a));
        // random angle in latitude slice (wrt Prime Meridian), radians
        double phi = uniRand->GetValue(0, M_PI * 2);
        // random angle from Center of Earth (wrt North Pole), radians
        double alpha = acos((EARTH_SPHERE_RADIUS - d) / EARTH_SPHERE_RADIUS);

        // shift coordinate system from North Pole referred to origin point referred
        // reference: http://en.wikibooks.org/wiki/General_Astronomy/Coordinate_Systems
        double theta = M_PI_2 - alpha; // angle of elevation of new point wrt
                                       // origin point (latitude in coordinate
                                       // system referred to origin point)
        double randPointLatitude = asin(sin(theta) * cos(originColatitude) +
                                        cos(theta) * sin(originColatitude) * sin(phi));
        // declination
        double intermedLong = asin((sin(randPointLatitude) * cos(originColatitude) - sin(theta)) /
                                   (cos(randPointLatitude) * sin(originColatitude)));
        // right ascension
        intermedLong = intermedLong + M_PI_2; // shift to longitude 0

        // flip / mirror point if it has phi in quadrant II or III (wasn't
        // resolved correctly by arcsin) across longitude 0
        if (phi > (M_PI_2) && phi <= (3 * M_PI_2))
        {
            intermedLong = -intermedLong;
        }

        // shift longitude to be referenced to origin
        double randPointLongitude = intermedLong + originLongitudeRadians;

        // random altitude above earth's surface
        double randAltitude = uniRand->GetValue(0, maxAltitude);

        // convert coordinates from geographic to cartesian
        Vector pointPosition = GeographicPositions::GeographicToCartesianCoordinates(
            RadiansToDegrees(randPointLatitude),
            RadiansToDegrees(randPointLongitude),
            randAltitude,
            SPHERE);

        // add generated coordinate points to list
        generatedPoints.push_back(pointPosition);
    }
    return generatedPoints;
}

std::tuple<double, double, double>
GeographicPositions::GetRadiusEccentFlat(EarthSpheroidType type)
{
    double a; // radius, in meters
    double e; // first eccentricity
    double f; // first flattening

    switch (type)
    {
    case SPHERE:
        a = EARTH_SPHERE_RADIUS;
        e = EARTH_SPHERE_ECCENTRICITY;
        f = EARTH_SPHERE_FLATTENING;
        break;
    case GRS80:
        a = EARTH_SEMIMAJOR_AXIS;
        e = EARTH_GRS80_ECCENTRICITY;
        f = EARTH_GRS80_FLATTENING;
        break;
    case WGS84:
        a = EARTH_SEMIMAJOR_AXIS;
        e = EARTH_WGS84_ECCENTRICITY;
        f = EARTH_WGS84_FLATTENING;
        break;
    default:
        NS_FATAL_ERROR("The specified earth model is not supported!");
    }

    return std::make_tuple(a, e, f);
}

} // namespace ns3
