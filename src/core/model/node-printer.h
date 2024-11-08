/*
 * Copyright (c) 2018 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */
#ifndef NODE_PRINTER_H
#define NODE_PRINTER_H

#include <ostream>

/**
 * @file
 * @ingroup logging
 * Declaration of ns3::NodePrinter function pointer type
 * and ns3::DefaultNodePrinter function.
 */

namespace ns3
{

/**
 * Function signature for prepending the node id
 * to a log message.
 *
 * @param [in,out] os The output stream to print on.
 */
typedef void (*NodePrinter)(std::ostream& os);

/**
 * @ingroup logging
 * Default node id printer implementation.
 *
 * @param [in,out] os The output stream to print the node id on.
 */
void DefaultNodePrinter(std::ostream& os);

} // namespace ns3

#endif /* NODE_H */
