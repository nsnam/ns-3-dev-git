/*
 * Copyright (c) 2018 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "node-printer.h"

#include "log.h"
#include "simulator.h" // GetContext()

#include <charconv>
#include <iterator>

/**
 * @file
 * @ingroup simulator
 * ns3::DefaultNodePrinter implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NodePrinter");

void
DefaultNodePrinter(std::ostream& os)
{
    auto context = Simulator::GetContext();
    if (context == Simulator::NO_CONTEXT)
    {
        os << "-1";
    }
    else
    {
        // std::to_chars is faster than the ostream integer inserter.
        char buf[16];
        auto [end, ec] = std::to_chars(std::begin(buf), std::end(buf), context);
        os.write(buf, end - buf);
    }
}

} // namespace ns3
