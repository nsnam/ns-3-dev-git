// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

/**
 * @file
 * @ingroup leo
 * Implementation of LeoCircularOrbitAllocator.
 */

#include "leo-circular-orbit-position-allocator.h"

#include "math.h"

#include "ns3/double.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(LeoCircularOrbitAllocator);

LeoCircularOrbitAllocator::LeoCircularOrbitAllocator()
{
}

LeoCircularOrbitAllocator::~LeoCircularOrbitAllocator()
{
}

TypeId
LeoCircularOrbitAllocator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::LeoCircularOrbitPositionAllocator")
            .SetParent<PositionAllocator>()
            .SetGroupName("Leo")
            .AddConstructor<LeoCircularOrbitAllocator>()
            .AddAttribute("NumOrbits",
                          "The number of orbits",
                          UintegerValue(1),
                          MakeUintegerAccessor(&LeoCircularOrbitAllocator::GetNumOrbits,
                                               &LeoCircularOrbitAllocator::SetNumOrbits),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("NumSatellites",
                          "The number of satellites per orbit",
                          UintegerValue(1),
                          MakeUintegerAccessor(&LeoCircularOrbitAllocator::GetNumSatellites,
                                               &LeoCircularOrbitAllocator::SetNumSatellites),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("PhasingFactor",
                          "Walker Delta phasing factor F; staggers satellites in "
                          "adjacent planes by F * 360 / T degrees, where "
                          "T = NumOrbits * NumSatellites",
                          UintegerValue(0),
                          MakeUintegerAccessor(&LeoCircularOrbitAllocator::GetPhasingFactor,
                                               &LeoCircularOrbitAllocator::SetPhasingFactor),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("RaanSpanDeg",
                          "Total RAAN span in degrees over which orbital planes "
                          "are distributed (360 for Walker Delta, 180 for Walker Star)",
                          DoubleValue(360.0),
                          MakeDoubleAccessor(&LeoCircularOrbitAllocator::GetRaanSpanDeg,
                                             &LeoCircularOrbitAllocator::SetRaanSpanDeg),
                          MakeDoubleChecker<double>(0.0, 360.0));
    return tid;
}

int64_t
LeoCircularOrbitAllocator::AssignStreams(int64_t stream)
{
    return 0;
}

Vector
LeoCircularOrbitAllocator::GetNext() const
{
    auto params = GetNextOrbitPosition();
    return Vector{params.longitude, params.argumentOfLatitude, params.satelliteIndex};
}

LeoOrbitPosition
LeoCircularOrbitAllocator::GetNextOrbitPosition() const
{
    double phasingOffset = m_config.m_phasingFactor * m_lastOrbit * 360.0 /
                           (m_config.m_numOrbits * m_config.m_numSatellites);
    LeoOrbitPosition params{m_config.m_raanSpanDeg * (m_lastOrbit / (double)m_config.m_numOrbits),
                            360.0 * (m_lastSatellite / (double)m_config.m_numSatellites) +
                                phasingOffset,
                            (double)m_lastSatellite};

    m_lastSatellite = (m_lastSatellite + 1) % m_config.m_numSatellites;
    if (!m_lastSatellite)
    {
        m_lastOrbit = (m_lastOrbit + 1) % m_config.m_numOrbits;
    }

    return params;
}

}; // namespace ns3
