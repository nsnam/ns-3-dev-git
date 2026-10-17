// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#ifndef LEO_ORBITAL_SHELL_H
#define LEO_ORBITAL_SHELL_H

#include "ns3/uinteger.h"

/**
 * @file
 * @ingroup leo
 */

namespace ns3
{
class LeoOrbitalShell;

/**
 * @brief Serialize an orbit parameters in csv format
 *
 * Serialized orbit parameters as a comma-separated list in this order:
 *   - altitude (km, from earth surface)
 *   - inclination (degrees relative to equator)
 *   - number of orbital planes
 *   - number of satellites per plane
 *
 * Planes are evenly spaced by 360 degrees divided by the number of planes, starting from the prime
 * meridian for Walker Delta, or 180 degrees for Walker Star.
 *
 * Example: "700,98,2,12" (700 km altitude, 98 degrees of inclination, 2 planes, 12
 * satellites/plane).
 *
 * @return the stream
 * @param os the stream
 * @param [in] orbit is an Orbit
 */
std::ostream& operator<<(std::ostream& os, const LeoOrbitalShell& orbit);

/**
 * @brief Deserialize an orbit in csv format
 *
 * Deserialized orbit parameters as a comma-separated list in this order:
 *   - altitude (km, from earth surface)
 *   - inclination (degrees relative to equator)
 *   - number of orbital planes
 *   - number of satellites per plane
 *   - Walker Delta phasing factor (optional, default 0)
 *   - RAAN span in degrees (optional, default 360)
 *
 *
 * Planes are evenly spaced by 360 degrees divided by the number of planes, starting from the prime
 * meridian for Walker Delta, or 180 degrees for Walker Star.
 *
 * Example: "700,98,2,12" (700 km altitude, 98 degrees of inclination, 2 planes, 12
 * satellites/plane).
 *
 * If deserialization fails due to unexpected fields/comments/text, the orbit is initialized with
 * all zero fields. Input stream error flags are cleared to continue parsing, so comments can be
 * added, except in lines with orbital parameters.
 *
 * @return the stream
 * @param is the stream
 * @param [out] orbit is the Orbit
 */
std::istream& operator>>(std::istream& is, LeoOrbitalShell& orbit);

/**
 * @ingroup leo
 * @brief Orbit definition
 */
class LeoOrbitalShell
{
  public:
    /// constructor
    LeoOrbitalShell() = default;

    /**
     * @brief Constructor
     * @param a altitude (km, from earth surface)
     * @param i inclination (degrees)
     * @param p number of planes
     * @param s number of satellites per plane
     * @param f Walker Delta phasing factor (default 0)
     * @param r RAAN span in degrees (360 for Walker Delta, 180 for Walker Star)
     */
    LeoOrbitalShell(double a,
                    double i,
                    std::size_t p,
                    std::size_t s,
                    double f = 0,
                    double r = 360.0)
        : alt(a),
          inc(i),
          planes(p),
          sats(s),
          phasing(f),
          raanSpanDeg(r)
    {
    }

    /// Altitude of orbit (km, from earth surface)
    double alt;
    /// Inclination of orbit (degrees)
    double inc;
    /// Number of planes with that altitude and inclination
    std::size_t planes;
    /// Number of satellites in those planes
    std::size_t sats;
    /// Walker Delta phasing factor F (0 means no inter-plane stagger)
    double phasing{0};
    /// RAAN span in degrees (360 for Walker Delta, 180 for Walker Star)
    double raanSpanDeg{360.0};
};

}; // namespace ns3

#endif
