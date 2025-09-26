// Copyright (c) Tim Schubert
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Tim Schubert <ns-3-leo@timschubert.net>
// Porting: Thiago Miyazaki <miyathiago@gmail.com> <t.miyazaki@unesp.br>

#include "ns3/core-module.h"
#include "ns3/leo-orbit-node-helper.h"
#include "ns3/mobility-module.h"

#include <fstream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LeoCircularOrbitTracingExample");

/**
 * Print simulation time, node Id, position and velocity associated with a mobility model
 * @param mob the mobility model for the satellite node
 */
void
CourseChange(Ptr<const MobilityModel> mob)
{
    const Vector pos = mob->GetPosition();
    Ptr<const Node> node = mob->GetObject<Node>();
    std::cout << Simulator::Now() << ":" << node->GetId() << ":" << pos.x << ":" << pos.y << ":"
              << pos.z << ":" << mob->GetVelocity().GetLength() << std::endl;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    std::string orbitFile;
    std::string traceFile;
    uint32_t duration = 60;    // seconds
    uint32_t precision = 1000; // milliseconds
    cmd.AddValue("orbitFile", "CSV file with orbit parameters", orbitFile);
    cmd.AddValue("traceFile", "CSV file to store mobility trace in", traceFile);
    cmd.AddValue("precision", "Mobility model time precision in milliseconds", precision);
    cmd.AddValue("duration", "Duration of the simulation in seconds", duration);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::LeoCircularOrbitMobilityModel::Precision",
                       TimeValue(MilliSeconds(precision)));

    LeoOrbitNodeHelper orbit(Time(MilliSeconds(precision)));
    NodeContainer satellites;
    if (!orbitFile.empty())
    {
        satellites = orbit.Install(orbitFile);
    }
    else
    {
        satellites = orbit.Install({LeoOrbit(1200, 20, 32, 16), LeoOrbit(1180, 30, 12, 10)});
    }

    Config::ConnectWithoutContext("/NodeList/*/$ns3::MobilityModel/CourseChange",
                                  MakeCallback(&CourseChange));

    std::streambuf* coutbuf = std::cout.rdbuf();
    // redirect cout if traceFile is specified
    std::ofstream out;
    out.open(traceFile);
    if (out.is_open())
    {
        std::cout.rdbuf(out.rdbuf());
    }

    std::cout << "Time:Satellite:x:y:z:Speed" << std::endl;

    Simulator::Stop(Time(Seconds(duration)));
    Simulator::Run();
    Simulator::Destroy();

    out.close();
    std::cout.rdbuf(coutbuf);
}
