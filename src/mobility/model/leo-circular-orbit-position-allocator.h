// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#ifndef LEO_CIRCULAR_ORBIT_POSITION_ALLOCATOR_H
#define LEO_CIRCULAR_ORBIT_POSITION_ALLOCATOR_H

#include "position-allocator.h"

/**
 * @file
 * @ingroup leo
 *
 * Declaration of LeoCircularOrbitAllocator
 */

namespace ns3
{

/**
 * @ingroup leo
 * @brief Allocate pairs of longitude and latitude (offset within orbit) for
 * use in LeoCircularOrbitMobilityModel
 */
class LeoCircularOrbitAllocator : public PositionAllocator
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /// constructor
    LeoCircularOrbitAllocator();
    /// destructor
    ~LeoCircularOrbitAllocator() override;

    /**
     * @brief Gets the next position on the latitude longitude grid
     *
     * If all positions have been returned once, the first position is returned
     * again and so on.
     *
     * @return the next latitude, longitude pair
     */
    Vector GetNext() const override;
    int64_t AssignStreams(int64_t stream) override;

  private:
    /// Number of orbits two distribute the satellites on
    uint64_t m_numOrbits;
    /// Number of satellites per orbit
    uint64_t m_numSatellites;

    /// The last orit that has been assigned
    mutable uint64_t m_lastOrbit;
    /// The last position inside the orbit that has been assigned
    mutable uint64_t m_lastSatellite;
};

}; // namespace ns3

#endif
