/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Sascha Jopen <jopen@cs.uni-bonn.de>
 */

#include "ns3/click-internet-stack-helper.h"
#include "ns3/core-module.h"
#include "ns3/ipv4-click-routing.h"

#include <map>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("NsclickRouting");

int
main(int argc, char* argv[])
{
    std::string clickConfigFolder = "src/click/examples";

    CommandLine cmd(__FILE__);
    cmd.AddValue("clickConfigFolder",
                 "Base folder for click configuration files",
                 clickConfigFolder);
    cmd.Parse(argc, argv);

    //
    // Explicitly create the nodes required by the topology (shown above).
    //
    NS_LOG_INFO("Create a node.");
    NodeContainer n;
    n.Create(1);

    //
    // Install Click on the nodes
    //
    std::map<std::string, std::string> defines;
    // Strings, especially with blanks in it, have to be enclosed in quotation
    // marks, like in click configuration files.
    defines["OUTPUT"] = "\"Hello World!\"";

    ClickInternetStackHelper clickinternet;
    clickinternet.SetClickFile(n, clickConfigFolder + "/nsclick-defines.click");
    clickinternet.SetRoutingTableElement(n, "rt");
    clickinternet.SetDefines(n, defines);
    clickinternet.Install(n);

    //
    // Now, do the actual simulation.
    //
    NS_LOG_INFO("Run Simulation.");
    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_INFO("Done.");

    return 0;
}
