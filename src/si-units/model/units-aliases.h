#ifndef UNITS_ALIASES_H
#define UNITS_ALIASES_H

/*
 * Copyright (c) 2024
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Jiwoong Lee <porce@berkeley.edu>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include <cstdint>
#include <limits>

// clang-format off
namespace ns3
{

constexpr auto ONE_KILO = 1000L;               ///< Kilo
constexpr auto ONE_MEGA = ONE_KILO * ONE_KILO; ///< Mega
constexpr auto ONE_GIGA = ONE_MEGA * ONE_KILO; ///< Giga
constexpr auto ONE_TERA = ONE_GIGA * ONE_KILO; ///< Tera
constexpr auto ONE_PETA = ONE_TERA * ONE_KILO; ///< Peta

// Small numbers
constexpr double EPSILON = std::numeric_limits<double>::epsilon(); ///< Minimum resolution
constexpr double PPM = 0.000001; ///< Part Per Million. Accommodate a 6 digit below point

// Weak-type aliases to assist 2-pass migration

using mWatt_u = double;       ///< mWatt weak type
using Watt_u = double;        ///< Watt weak type
using dBW_u = double;         ///< dBW weak type
using dBm_u = double;         ///< dBm weak type
using dB_u = double;          ///< dB weak type
using dBr_u = dB_u;           ///< dBr weak type
using Hz_u = double;          ///< Hz weak type
using MHz_u = double;         ///< MHz weak type
using meter_u = double;       ///< meter weak type
using ampere_u = double;      ///< ampere weak type
using volt_u = double;        ///< volt weak type
using degree_u = double;      ///< degree weak type (angle)
using joule_u = double;       ///< joule weak type
using dBm_per_Hz_u = double;  ///< dBm/Hz weak type
using dBm_per_MHz_u = double; ///< dBm/MHz weak type

using ppm_t = double;  ///< Parts per million
using volt_u = double; ///< Voltage weak type

// The time unit also defines the resolution.
// Use these time units where ns3::Time is not viable.
using sec_u = int64_t;  ///< seconds
using msec_u = int64_t; ///< milliseconds
using usec_u = int64_t; ///< microseconds
using nsec_u = int64_t; ///< nanoseconds
using psec_u = int64_t; ///< picoseconds

} // namespace ns3

// clang-format on

#endif // UNITS_ALIASES_H
