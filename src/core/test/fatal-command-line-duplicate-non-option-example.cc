/*
 * Copyright (c) 2013 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/core-module.h"

#include <iomanip>
#include <iostream>
#include <string>

/**
 * @file
 * @ingroup core-examples
 * @ingroup commandline
 * Example program illustrate the abort due to duplicate non-option in ns3::CommandLine.
 */

using namespace ns3;

int
main(int argc, char* argv[])
{
    int intArg = 1;
    int intArg2 = 1;
    CommandLine cmd(__FILE__);
    cmd.Usage("This program is expected to abort due to duplicate non-option definition.");
    cmd.AddNonOption("intArg", "int argument", intArg);
    cmd.AddNonOption("intArg", "int argument", intArg2);
    cmd.Parse(argc, argv);
    return 0;
}
