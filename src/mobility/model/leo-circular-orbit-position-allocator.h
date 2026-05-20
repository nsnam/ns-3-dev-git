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
 * @brief Represents a position in a Walker constellation as a pair of angular
 * coordinates.
 *
 * The fields are in degrees to match the LeoCircularOrbitMobilityModel
 * SetPosition() interface (longitude/argument of latitude in degrees).
 */
struct LeoOrbitPosition
{
    double longitude;          ///< Longitude of the ascending node in degrees
    double argumentOfLatitude; ///< Argument of latitude (orbital offset) in degrees
    double satelliteIndex;     ///< Satellite index within the plane (kept for compat)
};

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
     * @brief Gets the next position on the longitude grid
     *
     * If all positions have been returned once, the first position is returned
     * again and so on.
     *
     * @return LeoOrbitPosition with longitude of the ascending node in degrees
     *         (x), orbital offset / argument of latitude in degrees (y), and
     *         the satellite index within the plane (z).
     */
    LeoOrbitPosition GetNextOrbitPosition() const;

    /**
     * @brief Gets the next position on the longitude grid (base class compat)
     *
     * If all positions have been returned once, the first position is returned
     * again and so on.
     *
     * @return Vector where x is the longitude of the ascending node in
     *         degrees, y is the orbital offset in degrees, and z is the
     *         satellite index within the plane.
     */
    Vector GetNext() const override;

    // Inherited
    int64_t AssignStreams(int64_t stream) override;

    /**
     * @brief Set the number of orbits (planes) in the constellation.
     *
     * @param num Number of orbits.
     */
    void SetNumOrbits(uint64_t num)
    {
        m_config.m_numOrbits = num;
    }

    /**
     * @brief Get the number of orbits (planes) in the constellation.
     *
     * @return Number of orbits.
     */
    uint64_t GetNumOrbits() const
    {
        return m_config.m_numOrbits;
    }

    /**
     * @brief Set the number of satellites per orbit.
     *
     * @param num Number of satellites.
     */
    void SetNumSatellites(uint64_t num)
    {
        m_config.m_numSatellites = num;
    }

    /**
     * @brief Get the number of satellites per orbit.
     *
     * @return Number of satellites per orbit.
     */
    uint64_t GetNumSatellites() const
    {
        return m_config.m_numSatellites;
    }

    /**
     * @brief Set the phasing factor (angular spacing between adjacent orbits).
     *
     * @param factor Phasing factor value.
     */
    void SetPhasingFactor(uint16_t factor)
    {
        m_config.m_phasingFactor = factor;
    }

    /**
     * @brief Get the phasing factor.
     *
     * @return Phasing factor.
     */
    uint16_t GetPhasingFactor() const
    {
        return m_config.m_phasingFactor;
    }

    /**
     * @brief Set the RAAN (Right Ascension of the Ascending Node) span in degrees.
     *
     * @param deg RAAN span in degrees.
     */
    void SetRaanSpanDeg(double deg)
    {
        m_config.m_raanSpanDeg = deg;
    }

    /**
     * @brief Get the RAAN span in degrees.
     *
     * @return RAAN span in degrees.
     */
    double GetRaanSpanDeg() const
    {
        return m_config.m_raanSpanDeg;
    }

  private:
    /**
     * @brief Configuration parameters for the LEO circular orbit allocator.
     */
    struct Config
    {
        uint64_t m_numOrbits;     ///< Number of orbits (planes)
        uint64_t m_numSatellites; ///< Number of satellites per orbit
        uint16_t m_phasingFactor; ///< Phasing factor (angular spacing)
        double m_raanSpanDeg;     ///< RAAN span in degrees
    } m_config;                   ///< Configuration parameters

    /// The last orbit that has been assigned
    mutable uint64_t m_lastOrbit{0};
    /// The last position inside the orbit that has been assigned
    mutable uint64_t m_lastSatellite{0};
};

}; // namespace ns3

#endif
