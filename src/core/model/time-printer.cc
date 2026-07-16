/*
 * Copyright (c) 2018 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "time-printer.h"

#include "log.h"
#include "nstime.h"
#include "simulator.h" // Now()

#include <charconv>
#include <cmath>
#include <iomanip>
#include <iterator>
#include <limits>

/**
 * @file
 * @ingroup time
 * ns3::DefaultTimePrinter implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TimePrinter");

void
DefaultTimePrinter(std::ostream& os)
{
    // Digits after the decimal point when printing seconds, indexed by
    // Time::Unit (Y, D, H, MIN, S, MS, US, NS, PS, FS); the default C++
    // precision of 5 is kept for the coarser resolutions.
    static constexpr int precisions[Time::LAST] = {5, 5, 5, 5, 5, 5, 6, 9, 12, 15};
    const int precision = precisions[Time::GetResolution()];

    // Faster than ostream formatting.  The buffer holds the fixed notation
    // of any double (up to max_exponent10 + 1 integer digits), the sign, the
    // decimal point, the widest precision and the unit, so to_chars cannot
    // run out of room.
    double seconds = Simulator::Now().GetSeconds();
    char buf[std::numeric_limits<double>::max_exponent10 + 1 + 1 + 1 + 15 + 1];
    char* p = buf;
    if (!std::signbit(seconds))
    {
        *p++ = '+';
    }
    auto [end, ec] =
        std::to_chars(p, std::end(buf) - 1, seconds, std::chars_format::fixed, precision);
    if (ec != std::errc())
    {
        // Not reachable with the buffer above; kept as a safety net which
        // must not assert, since NS_FATAL_ERROR itself calls this printer.
        std::ios_base::fmtflags ff = os.flags();
        std::streamsize oldPrecision = os.precision();
        os << std::showpos << std::fixed << std::setprecision(precision) << seconds << 's';
        os.precision(oldPrecision);
        os.flags(ff);
        return;
    }
    *end++ = 's';
    os.write(buf, end - buf);
}

} // namespace ns3
