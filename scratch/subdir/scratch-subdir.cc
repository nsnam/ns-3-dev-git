/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

// This example shows how to create new simulations that are implemented in
// multiple files and headers. The structure of this simulation project
// is as follows:
//
// scratch/
// |  subdir/
// |  |  - scratch-subdir.cc                   // Main simulation file
// |  |  - scratch-subdir-additional-header.h  // Additional header
// |  |  - scratch-subdir-additional-header.cc // Additional header implementation
//
// This file contains the main() function, which calls an external function
// defined in the "scratch-subdir-additional-header.h" header file and
// implemented in "scratch-subdir-additional-header.cc".

#include "scratch-subdir-additional-header.h"

#include "ns3/core-module.h"

#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ScratchSubdir");

int
main(int argc, char* argv[])
{
    std::string message = ScratchSubdirGetMessage();
    NS_LOG_UNCOND(message);

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
