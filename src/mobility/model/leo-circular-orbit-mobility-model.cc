// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

/**
 * @file
 * @ingroup leo
 * Implementation of LeoCircularOrbitMobilityModel.
 */

#include "leo-circular-orbit-mobility-model.h"

#include "geographic-positions.h"
#include "math.h"

#include "ns3/angles.h"
#include "ns3/double.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("LeoCircularOrbitMobilityModel");

NS_OBJECT_ENSURE_REGISTERED(LeoCircularOrbitMobilityModel);

TypeId
LeoCircularOrbitMobilityModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LeoCircularOrbitMobilityModel")
            .SetParent<GeocentricConstantPositionMobilityModel>()
            .SetGroupName("Leo")
            .AddConstructor<LeoCircularOrbitMobilityModel>()
            .AddAttribute("Altitude",
                          "A height from the earth's surface in kilometers",
                          DoubleValue(1000.0),
                          MakeDoubleAccessor(&LeoCircularOrbitMobilityModel::SetAltitudeKm,
                                             &LeoCircularOrbitMobilityModel::GetAltitudeKm),
                          MakeDoubleChecker<double>())
            // TODO check value limits
            .AddAttribute("Inclination",
                          "The inclination of the orbital plane in degrees",
                          DoubleValue(10.0),
                          MakeDoubleAccessor(&LeoCircularOrbitMobilityModel::SetInclination,
                                             &LeoCircularOrbitMobilityModel::GetInclination),
                          MakeDoubleChecker<double>())
            .AddAttribute("Speed",
                          "Orbital speed in meters per second; negative to compute from gravity",
                          DoubleValue(-1.0),
                          MakeDoubleAccessor(&LeoCircularOrbitMobilityModel::m_orbitalSpeed),
                          MakeDoubleChecker<double>())
            .AddAttribute("Resolution",
                          "Time interval between CourseChange notifications (zero disables). "
                          "Determines how frequently the mobility model notifies course changes.",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&LeoCircularOrbitMobilityModel::m_resolutionTimeStep),
                          MakeTimeChecker());
    return tid;
}

LeoCircularOrbitMobilityModel::LeoCircularOrbitMobilityModel()
    : GeocentricConstantPositionMobilityModel(),
      m_longitude(0.0),
      m_argumentOfLatitude(0.0),
      m_orbitalSpeed(-1.0)
{
    /**
     * @todo The current implementation uses a periodic event to trigger
     * NotifyCourseChange(). As suggested by reviewers, a more idiomatic approach
     * would be to rely on the underlying MobilityModel trace source. However,
     * due to the continuous nature of LEO orbit changes, the periodic
     * approximation is currently used.
     */
    NS_LOG_FUNCTION(this);
}

LeoCircularOrbitMobilityModel::~LeoCircularOrbitMobilityModel()
{
    NS_LOG_FUNCTION(this);
    if (m_courseChangeEvent.IsPending())
    {
        m_courseChangeEvent.Cancel();
    }
}

/// meters in a kilometer
constexpr double M_PER_KM = 1000;

double
LeoCircularOrbitMobilityModel::GetSpeed() const
{
    if (m_orbitalSpeed >= 0.0)
    {
        return m_orbitalSpeed;
    }
    else
    {
        // sqrt((m^3/s^2) / m) => sqrt(m^2/s^2) => m/s = m/s
        return sqrt(GeographicPositions::LEO_EARTH_GGC / (m_orbitHeight * M_PER_KM));
    }
}

Vector
LeoCircularOrbitMobilityModel::DoGetVelocity() const
{
    Time t = Simulator::Now();
    Vector3D pos = DoGetPosition();
    double radius = pos.GetLength();
    Vector3D heading = CrossProduct(PlaneNorm(t), pos);
    return (GetSpeed() / radius) * heading;
}

Vector3D
LeoCircularOrbitMobilityModel::PlaneNorm(Time t) const
{
    double lon = CalcLongitude(t);
    return Vector3D(sin(-m_inclination) * cos(lon),
                    sin(-m_inclination) * sin(lon),
                    cos(m_inclination));
}

Vector3D
LeoCircularOrbitMobilityModel::RotatePlane(double theta, const Vector3D& v, Time t) const
{
    Vector3D k = PlaneNorm(t);
    // Canonical Rodrigues' rotation formula:
    // v_rot = v cos(theta) + (k x v) sin(theta) + k(k . v)(1 - cos(theta))
    // where k is the unit rotation axis.
    const double cosTheta = cos(theta);
    const double sinTheta = sin(theta);
    return (v * cosTheta) + (CrossProduct(k, v) * sinTheta) + (k * (k * v) * (1 - cosTheta));
}

double
LeoCircularOrbitMobilityModel::CalcLongitude(Time t) const
{
    return m_longitude - (t * 2 * M_PI / Days(1)).GetDouble();
}

double
LeoCircularOrbitMobilityModel::GetAngularVelocity() const
{
    double omega;
    double orbitMeters = m_orbitHeight * M_PER_KM;
    if (m_orbitalSpeed >= 0.0)
    {
        // If the user set up a different orbital speed
        omega = std::abs(m_orbitalSpeed) / (orbitMeters);
    }
    else
    {
        // If the user did not setup an angular velocity, assume it based on earths gravity
        // omega = sqrt(GM / r^3); units: sqrt(m^3/s^2 / m^3) = rad/s
        omega = std::sqrt(GeographicPositions::LEO_EARTH_GGC /
                          (orbitMeters * orbitMeters * orbitMeters));
    }
    // Retrograde orbit: inclination > 90 degrees
    if (m_inclination > M_PI / 2.0)
    {
        omega = -omega;
    }
    return omega;
}

Vector
LeoCircularOrbitMobilityModel::CalcPosition(Time t) const
{
    double lon = CalcLongitude(t);
    // account for orbit longitude and earth rotation offset
    Vector3D x = (m_orbitHeight * M_PER_KM *
                  Vector3D(cos(m_inclination) * cos(lon),
                           cos(m_inclination) * sin(lon),
                           sin(m_inclination)));

    double progressAngleRad = m_argumentOfLatitude + GetAngularVelocity() * t.GetSeconds();
    return RotatePlane(progressAngleRad, x, t);
}

void
LeoCircularOrbitMobilityModel::NotifyCourseChangeAndReschedule()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("CourseChange pos = " << CalcPosition(Simulator::Now()));
    NotifyCourseChange();
    m_courseChangeEvent =
        Simulator::Schedule(m_resolutionTimeStep,
                            &LeoCircularOrbitMobilityModel::NotifyCourseChangeAndReschedule,
                            this);
}

void
LeoCircularOrbitMobilityModel::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    if (m_resolutionTimeStep > Seconds(0))
    {
        NS_LOG_INFO("Scheduling first course change event at " << m_resolutionTimeStep.As(Time::S));
        m_courseChangeEvent =
            Simulator::Schedule(m_resolutionTimeStep,
                                &LeoCircularOrbitMobilityModel::NotifyCourseChangeAndReschedule,
                                this);
    }
    GeocentricConstantPositionMobilityModel::DoInitialize();
}

void
LeoCircularOrbitMobilityModel::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_courseChangeEvent.Cancel();
    GeocentricConstantPositionMobilityModel::DoDispose();
}

// The below two methods return the same ECEF position because this model's
// state is contained in orbital data members only; it never populates the
// base class m_position (lat/lon/alt) member variable.  Without these overrides,
// the methods would return the unused (0,0,0) m_position.  It is not clear
// whether DoGetPosition() will continue to return CalcPosition() in the future;
// coordinate system handling may be refactored.

Vector
LeoCircularOrbitMobilityModel::DoGetGeocentricPosition() const
{
    // This model uses ECEF coordinates, which is consistent with GeocentricEcefMobilityModel.
    return CalcPosition(Simulator::Now());
}

Vector
LeoCircularOrbitMobilityModel::DoGetPosition() const
{
    return CalcPosition(Simulator::Now());
}

void
LeoCircularOrbitMobilityModel::DoSetPosition(const Vector& position)
{
    NS_LOG_FUNCTION(this << position);

    const auto [longitudeDeg, argumentOfLatitudeDeg, satIndex] = position;
    m_longitude = DegreesToRadians(longitudeDeg);
    m_argumentOfLatitude = DegreesToRadians(argumentOfLatitudeDeg);
    NS_LOG_INFO("Set orbital position: longitude = "
                << longitudeDeg << " deg, argument of latitude = " << argumentOfLatitudeDeg
                << " deg, altitude = " << GetAltitudeKm()
                << " km, inclination = " << GetInclination()
                << " deg, angular velocity = " << GetAngularVelocity() << " rad/s");

    NotifyCourseChange();
}

double
LeoCircularOrbitMobilityModel::GetAltitude() const
{
    return GetAltitudeKm() * M_PER_KM;
}

void
LeoCircularOrbitMobilityModel::SetAltitude(double h)
{
    NS_LOG_FUNCTION(this << h);
    SetAltitudeKm(h / M_PER_KM);
}

double
LeoCircularOrbitMobilityModel::GetAltitudeKm() const
{
    return m_orbitHeight - (GeographicPositions::EARTH_SPHERE_RADIUS / M_PER_KM);
}

void
LeoCircularOrbitMobilityModel::SetAltitudeKm(double h)
{
    NS_LOG_FUNCTION(this << h);
    m_orbitHeight = (GeographicPositions::EARTH_SPHERE_RADIUS / M_PER_KM) + h;
}

double
LeoCircularOrbitMobilityModel::GetInclination() const
{
    return RadiansToDegrees(m_inclination);
}

void
LeoCircularOrbitMobilityModel::SetInclination(double incl)
{
    NS_LOG_FUNCTION(this << incl);
    m_inclination = DegreesToRadians(incl);
}

}; // namespace ns3
