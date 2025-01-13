/*
 * Copyright (c) 2012 CTTC
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "parabolic-antenna-model.h"

#include "antenna-model.h"

#include "ns3/double.h"
#include "ns3/log.h"

#include <cmath>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ParabolicAntennaModel");

NS_OBJECT_ENSURE_REGISTERED(ParabolicAntennaModel);

TypeId
ParabolicAntennaModel::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ParabolicAntennaModel")
            .SetParent<AntennaModel>()
            .SetGroupName("Antenna")
            .AddConstructor<ParabolicAntennaModel>()
            .AddAttribute("Beamwidth",
                          "The 3dB beamwidth (degrees)",
                          DoubleValue(60),
                          MakeDoubleAccessor(&ParabolicAntennaModel::SetBeamwidth,
                                             &ParabolicAntennaModel::GetBeamwidth),
                          MakeDoubleChecker<double>(0, 180))
            .AddAttribute("Orientation",
                          "The angle (degrees) that expresses the orientation of the antenna on "
                          "the x-y plane relative to the x axis",
                          DoubleValue(0.0),
                          MakeDoubleAccessor(&ParabolicAntennaModel::SetOrientation,
                                             &ParabolicAntennaModel::GetOrientation),
                          MakeDoubleChecker<double>(-360, 360))
            .AddAttribute("MaxAttenuation",
                          "The maximum attenuation (dB) of the antenna radiation pattern.",
                          DoubleValue(20.0),
                          MakeDoubleAccessor(&ParabolicAntennaModel::m_maxAttenuation),
                          MakeDoubleChecker<double>());
    return tid;
}

void
ParabolicAntennaModel::SetBeamwidth(double beamwidthDegrees)
{
    NS_LOG_FUNCTION(this << beamwidthDegrees);
    m_beamwidthRadians = DegreesToRadians(beamwidthDegrees);
}

double
ParabolicAntennaModel::GetBeamwidth() const
{
    return RadiansToDegrees(m_beamwidthRadians);
}

void
ParabolicAntennaModel::SetOrientation(double orientationDegrees)
{
    NS_LOG_FUNCTION(this << orientationDegrees);
    m_orientationRadians = DegreesToRadians(orientationDegrees);
}

double
ParabolicAntennaModel::GetOrientation() const
{
    return RadiansToDegrees(m_orientationRadians);
}

double
ParabolicAntennaModel::GetGainDb(Angles a)
{
    NS_LOG_FUNCTION(this << a);
    // azimuth angle w.r.t. the reference system of the antenna
    double phi = a.GetAzimuth() - m_orientationRadians;

    // make sure phi is in (-pi, pi]
    while (phi <= -M_PI)
    {
        phi += M_PI + M_PI;
    }
    while (phi > M_PI)
    {
        phi -= M_PI + M_PI;
    }

    NS_LOG_LOGIC("phi = " << phi);

    double gainDb = -std::min(12 * pow(phi / m_beamwidthRadians, 2), m_maxAttenuation);

    NS_LOG_LOGIC("gain = " << gainDb);
    return gainDb;
}

} // namespace ns3
