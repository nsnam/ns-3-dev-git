/*
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
