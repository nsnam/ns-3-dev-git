/*
 * Copyright (c) 2023 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

/**
 * @file
 * @ingroup core-examples
 * @ingroup assert
 * Example program illustrating the use of NS_ASSERT_MSG()
 */

#include "ns3/core-module.h"

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("AssertExample");

int
main(int argc, char** argv)
{
    CommandLine cmd;
    cmd.Parse(argc, argv);

    std::cout << "NS_ASSERT_MSG example\n"
              << "  if an argument is given this example will assert.\n";

    NS_ASSERT_MSG(argc == 1, "An argument was given, so we assert");

    return 0;
}
