/*
 * Copyright (c) 2022 University of Padova, Dep. of Information Engineering, SIGNET lab.
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
 * Author: Mattia Sandri <mattia.sandri@unipd.it>
 */

#include "circular-aperture-antenna-model.h"

// The host system uses Clang libc++, which does not support the Mathematical special functions
// (P0226R1) and Boost's implementation of cyl_bessel_j has been found.
#ifdef NEED_AND_HAVE_BOOST_BESSEL_FUNC
#include <boost/math/special_functions/bessel.hpp>
#endif

#include "antenna-model.h"

#include <ns3/double.h>
#include <ns3/log.h>

#include <math.h>

/**
 *  \file
 *  \ingroup antenna
 *  Class CircularApertureAntennaModel implementation.
 */

namespace
{
constexpr double C = 299792458.0; ///< speed of light in vacuum, in m/s
} // namespace

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("CircularApertureAntennaModel");

NS_OBJECT_ENSURE_REGISTERED(CircularApertureAntennaModel);

TypeId
CircularApertureAntennaModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::CircularApertureAntennaModel")
            .SetParent<AntennaModel>()
            .SetGroupName("Antenna")
            .AddConstructor<CircularApertureAntennaModel>()
            .AddAttribute("AntennaCircularApertureRadius",
                          "The radius of the aperture of the antenna, in meters",
                          DoubleValue(0.5),
                          MakeDoubleAccessor(&CircularApertureAntennaModel::SetApertureRadius),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("OperatingFrequency",
                          "The operating frequency in Hz of the antenna",
                          DoubleValue(2e9),
                          MakeDoubleAccessor(&CircularApertureAntennaModel::SetOperatingFrequency),
                          MakeDoubleChecker<double>(0.0))
            .AddAttribute("AntennaMinGainDb",
                          "The minimum gain value in dB of the antenna",
                          DoubleValue(-100.0),
                          MakeDoubleAccessor(&CircularApertureAntennaModel::SetMinGain),
                          MakeDoubleChecker<double>())
            .AddAttribute("AntennaMaxGainDb",
                          "The maximum gain value in dB of the antenna",
                          DoubleValue(1),
                          MakeDoubleAccessor(&CircularApertureAntennaModel::SetMaxGain),
                          MakeDoubleChecker<double>(0.0));
    return tid;
}

void
CircularApertureAntennaModel::SetApertureRadius(double aMeter)
{
    NS_LOG_FUNCTION(this << aMeter);
    NS_ASSERT_MSG(aMeter > 0, "Setting invalid aperture radius: " << aMeter);
    m_apertureRadiusMeter = aMeter;
}

double
CircularApertureAntennaModel::GetApertureRadius() const
{
    return m_apertureRadiusMeter;
}

void
CircularApertureAntennaModel::SetOperatingFrequency(double freqHz)
{
    NS_LOG_FUNCTION(this << freqHz);
    NS_ASSERT_MSG(freqHz > 0, "Setting invalid operating frequency: " << freqHz);
    m_operatingFrequencyHz = freqHz;
}

double
CircularApertureAntennaModel::GetOperatingFrequency() const
{
    return m_operatingFrequencyHz;
}

void
CircularApertureAntennaModel::SetMaxGain(double gainDb)
{
    NS_LOG_FUNCTION(this << gainDb);
    m_maxGain = gainDb;
}

double
CircularApertureAntennaModel::GetMaxGain() const
{
    return m_maxGain;
}

void
CircularApertureAntennaModel::SetMinGain(double gainDb)
{
    NS_LOG_FUNCTION(this << gainDb);
    m_minGain = gainDb;
}

double
CircularApertureAntennaModel::GetMinGain() const
{
    return m_minGain;
}

double
CircularApertureAntennaModel::GetGainDb(Angles a)
{
    NS_LOG_FUNCTION(this << a);

    // In 3GPP TR 38.811 v15.4.0, Section 6.4.1, the gain depends on a single angle only.
    // We assume that this angle represents the angle between the vectors corresponding
    // to the cartesian coordinates of the provided spherical coordinates, and the spherical
    // coordinates (r = 1, azimuth = 0, elevation = PI/2)
    double theta1 = a.GetInclination();
    double theta2 = M_PI_2; // reference direction

    // Convert to ISO range: the input azimuth angle phi is in [-pi,pi],
    // while the ISO convention for spherical to cartesian coordinates
    // assumes phi in [0,2*pi].
    double phi1 = M_PI + a.GetAzimuth();
    double phi2 = M_PI; // reference direction

    // Convert the spherical coordinates of the boresight and the incoming ray
    // to Cartesian coordinates
    Vector p1(sin(theta1) * cos(phi1), sin(theta1) * sin(phi1), cos(theta1));
    Vector p2(sin(theta2) * cos(phi2), sin(theta2) * sin(phi2), cos(theta2));

    // Calculate the angle between the antenna boresight and the incoming ray
    double theta = acos(p1 * p2);

    double gain = 0;
    if (theta == 0)
    {
        gain = m_maxGain;
    }
    // return value of std::arccos is in [0, PI] deg
    else if (theta >= M_PI_2)
    {
        // This is an approximation. 3GPP TR 38.811 does not provide indications
        // on the antenna field pattern outside its PI degrees FOV.
        gain = m_minGain;
    }
    else // 0 < theta < |PI/2|
    {
        // 3GPP TR 38.811 v15.4.0, Section 6.4.1
        double k = (2 * M_PI * m_operatingFrequencyHz) / C;
        double kasintheta = k * m_apertureRadiusMeter * sin(theta);
// If needed, fall back to Boost cyl_bessel_j
#ifdef NEED_AND_HAVE_BOOST_BESSEL_FUNC
        gain = boost::math::cyl_bessel_j(1, kasintheta) / kasintheta;
// Otherwise, use the std implementation
#else
        gain = std::cyl_bessel_j(1, kasintheta) / kasintheta;
#endif
        gain = 10 * log10(4 * gain * gain) + m_maxGain;
    }

    return gain;
}

} // namespace ns3
