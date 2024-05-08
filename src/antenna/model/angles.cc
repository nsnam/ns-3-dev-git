/*
 * Copyright (c) 2011, 2012 CTTC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "angles.h"

#include <ns3/log.h>

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Angles");

bool Angles::m_printDeg = false;

/// Degrees to Radians conversion constant
const double DEG_TO_RAD = M_PI / 180.0;
/// Radians to Degrees conversion constant
const double RAD_TO_DEG = 180.0 / M_PI;

double
DegreesToRadians(double degrees)
{
    return degrees * DEG_TO_RAD;
}

double
RadiansToDegrees(double radians)
{
    return radians * RAD_TO_DEG;
}

std::vector<double>
DegreesToRadians(const std::vector<double>& degrees)
{
    std::vector<double> radians;
    radians.reserve(degrees.size());
    for (size_t i = 0; i < degrees.size(); i++)
    {
        radians.push_back(DegreesToRadians(degrees[i]));
    }
    return radians;
}

std::vector<double>
RadiansToDegrees(const std::vector<double>& radians)
{
    std::vector<double> degrees;
    degrees.reserve(radians.size());
    for (size_t i = 0; i < radians.size(); i++)
    {
        degrees.push_back(RadiansToDegrees(radians[i]));
    }
    return degrees;
}

double
WrapTo360(double a)
{
    static constexpr int64_t INT_RANGE = 100000000000;
    // Divide the input by 360.
    // Multiply it by INT_RANGE and store into an integer.
    int64_t b(a / (360.0) * INT_RANGE);
    // Clamp it between [-INT_RANGE / 2, INT_RANGE / 2)
    b = b % INT_RANGE;
    if (b < 0)
    {
        b += INT_RANGE;
    }
    else if (b >= INT_RANGE)
    {
        b -= INT_RANGE;
    }
    // Divide by INT_RANGE and multiply by 360.
    return b * (360.0) / INT_RANGE;
}

double
WrapTo180(double a)
{
    static constexpr int64_t INT_RANGE = 100000000000;
    // Divide the input by 360.
    // Multiply it by INT_RANGE and store into an integer.
    int64_t b(a / (360.0) * INT_RANGE);
    // Clamp it between [-INT_RANGE / 2, INT_RANGE / 2)
    b = b % INT_RANGE;
    if (b < -INT_RANGE / 2)
    {
        b += INT_RANGE;
    }
    else if (b >= INT_RANGE / 2)
    {
        b -= INT_RANGE;
    }
    // Divide by INT_RANGE and multiply by 360.
    return b * (360.0) / INT_RANGE;
}

double
WrapTo2Pi(double a)
{
    static constexpr int64_t INT_RANGE = 100000000000;
    // Divide the input by 2*M_PI.
    // Multiply it by INT_RANGE and store into an integer.
    int64_t b(a / (2 * M_PI) * INT_RANGE);
    // Clamp it between [-INT_RANGE / 2, INT_RANGE / 2)
    b = b % INT_RANGE;
    if (b < 0)
    {
        b += INT_RANGE;
    }
    else if (b >= INT_RANGE)
    {
        b -= INT_RANGE;
    }
    // Divide by INT_RANGE and multiply by 2*M_PI.
    return b * (2 * M_PI) / INT_RANGE;
}

double
WrapToPi(double a)
{
    static constexpr int64_t INT_RANGE = 100000000000;
    // Divide the input by 2*M_PI.
    // Multiply it by INT_RANGE and store into an integer.
    int64_t b(a / (2 * M_PI) * INT_RANGE);
    // Clamp it between [-INT_RANGE / 2, INT_RANGE / 2)
    b = b % INT_RANGE;
    if (b < -INT_RANGE / 2)
    {
        b += INT_RANGE;
    }
    else if (b >= INT_RANGE / 2)
    {
        b -= INT_RANGE;
    }
    // Divide by INT_RANGE and multiply by 2*M_PI.
    return b * (2 * M_PI) / INT_RANGE;
}

std::ostream&
operator<<(std::ostream& os, const Angles& a)
{
    double azim;
    double incl;
    std::string unit;

    if (Angles::m_printDeg)
    {
        azim = RadiansToDegrees(a.m_azimuth);
        incl = RadiansToDegrees(a.m_inclination);
        unit = "deg";
    }
    else
    {
        azim = a.m_azimuth;
        incl = a.m_inclination;
        unit = "rad";
    }

    os << "(" << azim << ", " << incl << ") " << unit;
    return os;
}

std::istream&
operator>>(std::istream& is, Angles& a)
{
    char c;
    is >> a.m_azimuth >> c >> a.m_inclination;
    if (c != ':')
    {
        is.setstate(std::ios_base::failbit);
    }
    return is;
}

Angles::Angles()
    : Angles(NAN, NAN)
{
}

Angles::Angles(double azimuth, double inclination)
    : m_azimuth(azimuth),
      m_inclination(inclination)
{
    NormalizeAngles();
}

Angles::Angles(Vector v)
    : m_azimuth(std::atan2(v.y, v.x)),
      m_inclination(std::acos(v.z / v.GetLength()))
{
    // azimuth and inclination angles for zero-length vectors are not defined
    if (v.x == 0.0 && v.y == 0.0 && v.z == 0.0)
    {
        // assume x and length equals to 1 mm to avoid nans
        m_azimuth = std::atan2(v.y, 0.001);
        m_inclination = std::acos(v.z / 0.001);
    }

    NormalizeAngles();
}

Angles::Angles(Vector v, Vector o)
    : Angles(v - o)
{
}

void
Angles::SetAzimuth(double azimuth)
{
    m_azimuth = azimuth;
    NormalizeAngles();
}

void
Angles::SetInclination(double inclination)
{
    m_inclination = inclination;
    NormalizeAngles();
}

double
Angles::GetAzimuth() const
{
    return m_azimuth;
}

double
Angles::GetInclination() const
{
    return m_inclination;
}

void
Angles::NormalizeAngles()
{
    CheckIfValid();

    // Normalize azimuth angle
    if (std::isnan(m_azimuth))
    {
        return;
    }

    m_azimuth = WrapToPi(m_azimuth);
}

void
Angles::CheckIfValid() const
{
    if (std::isfinite(m_inclination) || std::isfinite(m_azimuth))
    {
        NS_ASSERT_MSG(0.0 <= m_inclination && m_inclination <= M_PI,
                      "m_inclination=" << m_inclination << " not valid, should be in [0, pi] rad");
    }
    else
    {
        // infinite or nan inclination or azimuth angle
        NS_LOG_WARN("Undefined angle: " << *this);
    }
}

} // namespace ns3
