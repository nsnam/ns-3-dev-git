/*
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
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
 * @ingroup systempath
 * Example program illustrating use of ns3::SystemPath
 */

using namespace ns3;
using namespace ns3::SystemPath;

int
main(int argc, char* argv[])
{
    std::string path = "/usr/share/dict/";

    CommandLine cmd;
    cmd.Usage("SystemPath examples.\n");

    cmd.AddValue("path", "Path to demonstrate SystemPath functions.", path);
    cmd.Parse(argc, argv);

    // Show initial values:
    std::cout << std::endl;
    std::cout << cmd.GetName() << ":" << std::endl;

    std::cout << "FindSelfDirectory:   " << FindSelfDirectory() << std::endl;

    std::cout << "Demonstration path:  " << path << std::endl;
    std::cout << "Exists?              " << (Exists(path) ? "yes" : "no") << std::endl;

    auto foo = Append(path, "foo");
    std::cout << "Append 'foo':        " << foo << std::endl;
    std::cout << "Exists?              " << (Exists(foo) ? "yes" : "no") << std::endl;

    std::cout << "Split path:\n";
    auto items = Split(path);
    for (const auto& item : items)
    {
        std::cout << "    '" << item << "'\n";
    }
    std::cout << std::endl;

    std::cout << "Successive Joins: \n";
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        auto partial = Join(items.begin(), it);
        std::cout << "    '" << partial << "'\n";
    }

    std::cout << "Files in the directory: \n";
    auto files = ReadFiles(path);
    for (const auto& item : files)
    {
        std::cout << "    '" << item << "'\n";
    }

    return 0;
}
