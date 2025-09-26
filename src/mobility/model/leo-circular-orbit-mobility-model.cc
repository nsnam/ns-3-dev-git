// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#include "leo-circular-orbit-mobility-model.h"

#include "math.h"

#include "ns3/double.h"
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
            .SetParent<MobilityModel>()
            .SetGroupName("Leo")
            .AddConstructor<LeoCircularOrbitMobilityModel>()
            .AddAttribute("Altitude",
                          "A height from the earth's surface in kilometers",
                          DoubleValue(1000.0),
                          MakeDoubleAccessor(&LeoCircularOrbitMobilityModel::SetAltitude,
                                             &LeoCircularOrbitMobilityModel::GetAltitude),
                          MakeDoubleChecker<double>())
            // TODO check value limits
            .AddAttribute("Inclination",
                          "The inclination of the orbital plane in degrees",
                          DoubleValue(10.0),
                          MakeDoubleAccessor(&LeoCircularOrbitMobilityModel::SetInclination,
                                             &LeoCircularOrbitMobilityModel::GetInclination),
                          MakeDoubleChecker<double>())
            .AddAttribute("Precision",
                          "The time precision with which to compute position updates. 0 means "
                          "arbitrary precision",
                          TimeValue(Seconds(1)),
                          MakeTimeAccessor(&LeoCircularOrbitMobilityModel::m_precision),
                          MakeTimeChecker());
    return tid;
}

LeoCircularOrbitMobilityModel::LeoCircularOrbitMobilityModel()
    : GeocentricConstantPositionMobilityModel(),
      m_longitude(0.0),
      m_offset(0.0),
      m_position()
{
    NS_LOG_FUNCTION_NOARGS();
}

LeoCircularOrbitMobilityModel::~LeoCircularOrbitMobilityModel()
{
}

double
LeoCircularOrbitMobilityModel::GetSpeed() const
{
    return sqrt(LEO_EARTH_GM_KM_E10 / m_orbitHeight) * 1e5;
}

Vector
LeoCircularOrbitMobilityModel::DoGetVelocity() const
{
    Time t = Simulator::Now();
    Vector3D pos = DoGetPosition();
    pos = Vector3D(pos.x / pos.GetLength(), pos.y / pos.GetLength(), pos.z / pos.GetLength());
    Vector3D heading = CrossProduct(PlaneNorm(t), pos);
    return GetSpeed() * heading;
}

Vector3D
LeoCircularOrbitMobilityModel::PlaneNorm(Time t) const
{
    double lat = CalcLatitude(t);
    return Vector3D(sin(-m_inclination) * cos(lat),
                    sin(-m_inclination) * sin(lat),
                    cos(m_inclination));
}

Vector3D
LeoCircularOrbitMobilityModel::RotatePlane(double a, const Vector3D& x, Time t) const
{
    Vector3D n = PlaneNorm(t);

    return ((n * x) * n) + (cos(a) * CrossProduct(CrossProduct(n, x), n)) +
           (sin(a) * CrossProduct(n, x));
}

double
LeoCircularOrbitMobilityModel::CalcLatitude(Time t) const
{
    return m_longitude + ((t.GetDouble() / Hours(24).GetDouble()) * 2 * M_PI);
}

Vector
LeoCircularOrbitMobilityModel::CalcPosition(Time t) const
{
    double lat = CalcLatitude(t);
    // account for orbit latitude and earth rotation offset
    Vector3D x = (m_orbitHeight * 1000 *
                  Vector3D(cos(m_inclination) * cos(lat),
                           cos(m_inclination) * sin(lat),
                           sin(m_inclination)));

    return RotatePlane(m_progressVector->at(m_nodeIndexAtProgressVector), x, t);
}

Vector
LeoCircularOrbitMobilityModel::UpdateNodePositionAndScheduleEvent()
{
    m_position = CalcPosition(Simulator::Now());

    NotifyCourseChange();

    // Advances the node position in the Progress Vector or resets its index when an orbital period
    // has been completed.
    if (static_cast<uint16_t>(m_progressVector->size()) == m_nodeIndexAtProgressVector + 1)
    {
        m_nodeIndexAtProgressVector = 0;
    }
    else
    {
        m_nodeIndexAtProgressVector++;
    }

    if (m_precision > Seconds(0))
    {
        Simulator::Schedule(m_precision,
                            &LeoCircularOrbitMobilityModel::UpdateNodePositionAndScheduleEvent,
                            this);
    }

    return m_position;
}

Vector
LeoCircularOrbitMobilityModel::DoGetGeocentricPosition() const
{
    return m_position;
}

Vector
LeoCircularOrbitMobilityModel::DoGetPosition() const
{
    if (m_precision == Time(0))
    {
        // Notice: NotifyCourseChange () will not be called.
        return CalcPosition(Simulator::Now());
    }
    return m_position;
}

void
LeoCircularOrbitMobilityModel::DoSetPosition(const Vector& position)
{
    // Use first element of position vector as latitude, second for longitude
    // this works nicely with MobilityHelper and GetPosition will still get the
    // correct position, but be aware that it will not be the same as supplied to
    // SetPosition
    m_longitude = position.x;
    m_offset = position.y;
}

double
LeoCircularOrbitMobilityModel::GetAltitude() const
{
    return m_orbitHeight - LEO_EARTH_RAD_KM;
}

void
LeoCircularOrbitMobilityModel::SetAltitude(double h)
{
    m_orbitHeight = LEO_EARTH_RAD_KM + h;
}

double
LeoCircularOrbitMobilityModel::GetInclination() const
{
    return (m_inclination / M_PI) * 180.0;
}

void
LeoCircularOrbitMobilityModel::SetInclination(double incl)
{
    NS_ASSERT_MSG(incl != 0.0, "Plane must not be orthogonal to axis");
    m_inclination = (incl / 180.0) * M_PI;
}

void
LeoCircularOrbitMobilityModel::SetNodeIndexAtProgressVector(uint64_t index)
{
    m_nodeIndexAtProgressVector = index;
}

void
LeoCircularOrbitMobilityModel::SetProgressVectorPointer(
    const std::shared_ptr<std::vector<double>>& ptr)
{
    m_progressVector = ptr;
}
}; // namespace ns3
