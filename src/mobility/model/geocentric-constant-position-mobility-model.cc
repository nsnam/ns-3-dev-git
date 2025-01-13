/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mattia Sandri <mattia.sandri@unipd.it>
 */
#include "geocentric-constant-position-mobility-model.h"

#include "ns3/angles.h"

#include <math.h>

/**
 *  @file
 *  @ingroup mobility
 *  Class GeocentricConstantPositionMobilityModel implementation.
 */

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(GeocentricConstantPositionMobilityModel);

TypeId
GeocentricConstantPositionMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::GeocentricConstantPositionMobilityModel")
            .SetParent<MobilityModel>()
            .SetGroupName("Mobility")
            .AddConstructor<GeocentricConstantPositionMobilityModel>()
            .AddAttribute(
                "PositionLatLongAlt",
                "The geographic position, in degrees (lat/lon) and meter (alt), in the order: "
                "latitude, longitude and "
                "altitude",
                Vector3DValue({0, 0, 0}),
                MakeVector3DAccessor(&GeocentricConstantPositionMobilityModel::m_position),
                MakeVector3DChecker())
            .AddAttribute("GeographicReferencePoint",
                          "The point, in meters, taken as reference when converting from "
                          "geographic to topographic.",
                          Vector3DValue({0, 0, 0}),
                          MakeVector3DAccessor(
                              &GeocentricConstantPositionMobilityModel::m_geographicReferencePoint),
                          MakeVector3DChecker());
    return tid;
}

Vector
GeocentricConstantPositionMobilityModel::GetGeographicPosition() const
{
    return DoGetGeographicPosition();
}

void
GeocentricConstantPositionMobilityModel::SetGeographicPosition(const Vector& latLonAlt)
{
    DoSetGeographicPosition(latLonAlt);
}

Vector
GeocentricConstantPositionMobilityModel::GetGeocentricPosition() const
{
    return DoGetGeocentricPosition();
}

void
GeocentricConstantPositionMobilityModel::SetGeocentricPosition(const Vector& position)
{
    DoSetGeocentricPosition(position);
}

double
GeocentricConstantPositionMobilityModel::GetElevationAngle(
    Ptr<const GeocentricConstantPositionMobilityModel> other)
{
    return DoGetElevationAngle(other);
}

void
GeocentricConstantPositionMobilityModel::SetCoordinateTranslationReferencePoint(
    const Vector& position)
{
    DoSetCoordinateTranslationReferencePoint(position);
}

Vector
GeocentricConstantPositionMobilityModel::GetCoordinateTranslationReferencePoint() const
{
    return DoGetCoordinateTranslationReferencePoint();
}

Vector
GeocentricConstantPositionMobilityModel::GetPosition() const
{
    return DoGetPosition();
}

void
GeocentricConstantPositionMobilityModel::SetPosition(const Vector& position)
{
    return DoSetPosition(position);
}

double
GeocentricConstantPositionMobilityModel::GetDistanceFrom(
    Ptr<const GeocentricConstantPositionMobilityModel> other) const
{
    return DoGetDistanceFrom(other);
}

Vector
GeocentricConstantPositionMobilityModel::DoGetPosition() const
{
    Vector topographicCoordinates =
        GeographicPositions::GeographicToTopocentricCoordinates(m_position,
                                                                m_geographicReferencePoint,
                                                                GeographicPositions::SPHERE);
    return topographicCoordinates;
}

void
GeocentricConstantPositionMobilityModel::DoSetPosition(const Vector& position)
{
    Vector geographicCoordinates =
        GeographicPositions::TopocentricToGeographicCoordinates(position,
                                                                m_geographicReferencePoint,
                                                                GeographicPositions::SPHERE);
    m_position = geographicCoordinates;
    NotifyCourseChange();
}

double
GeocentricConstantPositionMobilityModel::DoGetDistanceFrom(
    Ptr<const GeocentricConstantPositionMobilityModel> other) const
{
    Vector cartesianCoordA =
        GeographicPositions::GeographicToCartesianCoordinates(m_position.x,
                                                              m_position.y,
                                                              m_position.z,
                                                              GeographicPositions::SPHERE);
    Vector cartesianCoordB = other->DoGetGeocentricPosition();

    double distance = (cartesianCoordA - cartesianCoordB).GetLength();

    return distance;
}

Vector
GeocentricConstantPositionMobilityModel::DoGetGeographicPosition() const
{
    return m_position;
}

void
GeocentricConstantPositionMobilityModel::DoSetGeographicPosition(const Vector& latLonAlt)
{
    NS_ASSERT_MSG((latLonAlt.x >= -90) && (latLonAlt.x <= 90),
                  "Latitude must be between -90 deg and +90 deg");
    NS_ASSERT_MSG(latLonAlt.z >= 0, "Altitude must be higher or equal than 0 meters");

    m_position = latLonAlt;
    // Normalize longitude to [-180, 180]
    m_position.y = WrapTo180(m_position.y);
    NotifyCourseChange();
}

Vector
GeocentricConstantPositionMobilityModel::DoGetGeocentricPosition() const
{
    Vector geocentricCoordinates =
        GeographicPositions::GeographicToCartesianCoordinates(m_position.x,
                                                              m_position.y,
                                                              m_position.z,
                                                              GeographicPositions::SPHERE);
    return geocentricCoordinates;
}

void
GeocentricConstantPositionMobilityModel::DoSetGeocentricPosition(const Vector& position)
{
    Vector geographicCoordinates =
        GeographicPositions::CartesianToGeographicCoordinates(position,
                                                              GeographicPositions::SPHERE);
    m_position = geographicCoordinates;
    NotifyCourseChange();
}

double
GeocentricConstantPositionMobilityModel::DoGetElevationAngle(
    Ptr<const GeocentricConstantPositionMobilityModel> other)
{
    Vector me = this->DoGetGeocentricPosition();
    Vector them = other->DoGetGeocentricPosition();

    // a is assumed to be the terminal with the lowest altitude
    Vector& a = (me.z < them.z ? me : them);
    Vector& b = (me.z < them.z ? them : me);

    Vector bMinusA = b - a;
    double numerator = std::abs(a * bMinusA);
    double denominator = a.GetLength() * bMinusA.GetLength();
    double x = numerator / denominator;

    // Enforce the appropriate domain range ([-1, 1]) for the argument of std::asin.
    x = std::min(x, 1.0);
    x = std::max(x, -1.0);

    // asin returns radians, we convert to degrees
    double elevAngle = std::abs((180.0 * M_1_PI) * asin(x));

    return elevAngle;
}

void
GeocentricConstantPositionMobilityModel::DoSetCoordinateTranslationReferencePoint(
    const Vector& refPoint)
{
    m_geographicReferencePoint = refPoint;
}

Vector
GeocentricConstantPositionMobilityModel::DoGetCoordinateTranslationReferencePoint() const
{
    return m_geographicReferencePoint;
}

Vector
GeocentricConstantPositionMobilityModel::DoGetVelocity() const
{
    return Vector(0.0, 0.0, 0.0);
}

} // namespace ns3
