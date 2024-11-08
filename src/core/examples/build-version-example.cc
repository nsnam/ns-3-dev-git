/*
 * Copyright (c) 2018 Lawrence Livermore National Laboratory
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Peter D. Barnes, Jr. <pdbarnes@llnl.gov>
 */

#include "ns3/command-line.h"
#include "ns3/version.h"

#include <iomanip>
#include <iostream>
#include <string>

/**
 * @file
 * @ingroup core-examples
 * Example program illustrating use of ns3::Version.
 */

using namespace ns3;

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Usage("Version class example program.\n"
              "\n"
              "This program demonstrates the various outputs from the Version class");
    cmd.Parse(argc, argv);

    std::cout << std::endl;
    std::cout << cmd.GetName() << ":" << std::endl;

    // Print the source version used to build this example
    std::cout << "Program Version (according to CommandLine): ";
    cmd.PrintVersion(std::cout);
    std::cout << std::endl;

    Version version;
    std::cout << "Version fields:\n"
              << "LongVersion:        " << version.LongVersion() << "\n"
              << "ShortVersion:       " << version.ShortVersion() << "\n"
              << "BuildSummary:       " << version.BuildSummary() << "\n"
              << "VersionTag:         " << version.VersionTag() << "\n"
              << "Major:              " << version.Major() << "\n"
              << "Minor:              " << version.Minor() << "\n"
              << "Patch:              " << version.Patch() << "\n"
              << "ReleaseCandidate:   " << version.ReleaseCandidate() << "\n"
              << "ClosestAncestorTag: " << version.ClosestAncestorTag() << "\n"
              << "TagDistance:        " << version.TagDistance() << "\n"
              << "CommitHash:         " << version.CommitHash() << "\n"
              << "BuildProfile:       " << version.BuildProfile() << "\n"
              << "WorkingTree:        " << (version.DirtyWorkingTree() ? "dirty" : "clean")
              << std::endl;

    return 0;
}
