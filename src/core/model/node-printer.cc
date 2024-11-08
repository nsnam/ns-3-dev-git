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

#include <iomanip>

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
    if (Simulator::GetContext() == Simulator::NO_CONTEXT)
    {
        os << "-1";
    }
    else
    {
        os << Simulator::GetContext();
    }
}

} // namespace ns3
