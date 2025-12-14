/*
 * Copyright (c) 2020 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/command-line.h"
#include "ns3/system-path.h"

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
    std::string path = FindSelfDirectory();

    CommandLine cmd(__FILE__);
    cmd.Usage("SystemPath examples.\n");

    cmd.AddValue("path", "Path to demonstrate SystemPath functions.", path);
    cmd.Parse(argc, argv);

    // Print system path information
    std::cout << "Example name             : " << cmd.GetName() << "\n";
    std::cout << "FindSelf                 : " << FindSelf() << "\n";
    std::cout << "FindSelfDirectory        : " << FindSelfDirectory() << "\n";
    std::cout << "Temporary directory name : " << MakeTemporaryDirectoryName() << "\n";
    std::cout << "\n";

    std::cout << "Path                     : " << path << "\n";
    std::cout << "Exists                   : " << std::boolalpha << Exists(path) << "\n";
    std::cout << "Valid system path        : " << CreateValidSystemPath(path) << "\n";
    std::cout << "\n";

    auto pathFoo = Append(path, "foo");
    std::cout << "Append 'foo' to path     : " << pathFoo << "\n";
    std::cout << "Exists                   : " << std::boolalpha << Exists(pathFoo) << "\n";
    std::cout << "\n";

    std::cout << "Split path:\n";
    auto items = Split(path);
    for (const auto& item : items)
    {
        std::cout << "    '" << item << "'\n";
    }
    std::cout << "\n";

    std::cout << "Successive joins of the split path:\n";
    for (auto it = items.begin(); it != items.end(); ++it)
    {
        auto partial = Join(items.begin(), it);
        std::cout << "    '" << partial << "'\n";
    }
    std::cout << "\n";

    std::cout << "Files in the path directory:\n";
    auto files = ReadFiles(path);
    for (const auto& item : files)
    {
        std::cout << "    '" << item << "'\n";
    }

    return 0;
}
