/*
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/core-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("ScratchSimulator");

int
main(int argc, char* argv[])
{
    NS_LOG_UNCOND("Scratch Simulator");

    Simulator::Run();
    Simulator::Destroy();

    return 0;
}
