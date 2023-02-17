/*
 * Copyright (c) 2023 Lawrence Livermore National Laboratory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

/**
 * \file
 * \ingroup core-examples
 * \ingroup assert
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
