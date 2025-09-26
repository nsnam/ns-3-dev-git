// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#include "leo-circular-orbit-position-allocator.h"

#include "math.h"

#include "ns3/integer.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(LeoCircularOrbitAllocator);

LeoCircularOrbitAllocator::LeoCircularOrbitAllocator()
    : m_lastOrbit(0),
      m_lastSatellite(0)
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
                          IntegerValue(1),
                          MakeIntegerAccessor(&LeoCircularOrbitAllocator::m_numOrbits),
                          MakeIntegerChecker<uint16_t>())
            .AddAttribute("NumSatellites",
                          "The number of satellites per orbit",
                          IntegerValue(1),
                          MakeIntegerAccessor(&LeoCircularOrbitAllocator::m_numSatellites),
                          MakeIntegerChecker<uint16_t>());
    return tid;
}

int64_t
LeoCircularOrbitAllocator::AssignStreams(int64_t stream)
{
    return -1;
}

Vector
LeoCircularOrbitAllocator::GetNext() const
{
    Vector next = Vector(2 * M_PI * (m_lastOrbit / (double)m_numOrbits),
                         2 * M_PI * (m_lastSatellite / (double)m_numSatellites),
                         m_lastSatellite);

    if (m_lastSatellite + 1 == m_numSatellites)
    {
        m_lastOrbit = (m_lastOrbit + 1) % m_numOrbits;
    }
    m_lastSatellite = (m_lastSatellite + 1) % m_numSatellites;

    return next;
}

}; // namespace ns3
