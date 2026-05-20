// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

/**
 * @file
 * @ingroup leo
 * Implementation of LeoOrbitalShell serialization helpers.
 */

#include "leo-orbital-shell.h"

#include "ns3/csv-reader.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("LeoOrbitalShell");

namespace ns3
{

std::ostream&
operator<<(std::ostream& os, const LeoOrbitalShell& orbit)
{
    os << orbit.alt << "," << orbit.inc << "," << orbit.planes << "," << orbit.sats << ","
       << orbit.phasing << "," << orbit.raanSpanDeg;
    return os;
}

std::istream&
operator>>(std::istream& is, LeoOrbitalShell& orbit)
{
    // Use CsvReader to parse the CSV from the input stream.
    CsvReader csv(is);
    orbit = LeoOrbitalShell{}; // Reset to default values first.
    if (!csv.FetchNextRow())
    {
        return is;
    }
    if (csv.ColumnCount() < 4)
    {
        NS_LOG_WARN("Skipping row " << csv.RowNumber() << ": expected at least 4 columns, got "
                                    << csv.ColumnCount());
        return is;
    }
    bool ok = csv.GetValue(0, orbit.alt);
    ok &= csv.GetValue(1, orbit.inc);
    ok &= csv.GetValue(2, orbit.planes);
    ok &= csv.GetValue(3, orbit.sats);
    if (!ok)
    {
        return is;
    }
    // Optional 5th column: Walker Delta phasing factor.
    csv.GetValue(4, orbit.phasing);
    // Optional 6th column: RAAN span in degrees.
    csv.GetValue(5, orbit.raanSpanDeg);
    return is;
}

}; // namespace ns3
