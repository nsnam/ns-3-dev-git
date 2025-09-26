// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#ifndef LEO_ORBIT_H
#define LEO_ORBIT_H

#include "ns3/uinteger.h"

/**
 * @file
 * @ingroup leo
 */

namespace ns3
{
class LeoOrbit;

/**
 * @brief Serialize an orbit
 * @return the stream
 * @param os the stream
 * @param [in] orbit is an Orbit
 */
std::ostream& operator<<(std::ostream& os, const LeoOrbit& orbit);

/**
 * @brief Deserialize an orbit
 * @return the stream
 * @param is the stream
 * @param [out] orbit is the Orbit
 */
std::istream& operator>>(std::istream& is, LeoOrbit& orbit);

/**
 * @ingroup leo
 * @brief Orbit definition
 */
class LeoOrbit
{
  public:
    /// constructor
    LeoOrbit();

    /**
     * @brief Constructor
     * @param a altitude
     * @param i inclination
     * @param p number planes
     * @param s number of satellites in plane
     */
    LeoOrbit(double a, double i, double p, double s)
        : alt(a),
          inc(i),
          planes(p),
          sats(s)
    {
    }

    /// destructor
    virtual ~LeoOrbit();
    /// Altitude of orbit
    double alt;
    /// Inclination of orbit
    double inc;
    /// Number of planes with that altitude and inclination
    uint16_t planes;
    /// Number of satellites in those planes
    uint16_t sats;
};

}; // namespace ns3

#endif
