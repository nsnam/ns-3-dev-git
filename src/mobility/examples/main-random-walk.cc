/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

/**
 * Function called when there is a course change
 * @param context event context
 * @param mobility a pointer to the mobility model
 */
static void
CourseChange(std::string context, Ptr<const MobilityModel> mobility)
{
    Vector pos = mobility->GetPosition();
    Vector vel = mobility->GetVelocity();
    std::cout << Simulator::Now() << ", model=" << mobility << ", POS: x=" << pos.x
              << ", y=" << pos.y << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
              << ", z=" << vel.z << std::endl;
}

int
main(int argc, char* argv[])
{
    Config::SetDefault("ns3::RandomWalk2dMobilityModel::Mode", StringValue("Time"));
    Config::SetDefault("ns3::RandomWalk2dMobilityModel::Time", StringValue("2s"));
    Config::SetDefault("ns3::RandomWalk2dMobilityModel::Speed",
                       StringValue("ns3::ConstantRandomVariable[Constant=1.0]"));
    Config::SetDefault("ns3::RandomWalk2dMobilityModel::Bounds", StringValue("0|200|0|200"));

    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    NodeContainer c;
    c.Create(100);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                  "X",
                                  StringValue("100.0"),
                                  "Y",
                                  StringValue("100.0"),
                                  "Rho",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Mode",
                              StringValue("Time"),
                              "Time",
                              StringValue("2s"),
                              "Speed",
                              StringValue("ns3::ConstantRandomVariable[Constant=1.0]"),
                              "Bounds",
                              StringValue("0|200|0|200"));
    mobility.InstallAll();
    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChange));

    Simulator::Stop(Seconds(100));

    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
