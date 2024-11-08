/*
 * Copyright (c) 2006,2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;

/**
 * Function called when there is a course change
 * @param context event context
 * @param position a pointer to the mobility model
 */
static void
CourseChange(std::string context, Ptr<const MobilityModel> position)
{
    Vector pos = position->GetPosition();
    std::cout << Simulator::Now() << ", pos=" << position << ", x=" << pos.x << ", y=" << pos.y
              << ", z=" << pos.z << std::endl;
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    NodeContainer c;
    c.Create(10000);

    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                  "X",
                                  StringValue("100.0"),
                                  "Y",
                                  StringValue("100.0"),
                                  "Rho",
                                  StringValue("ns3::UniformRandomVariable[Min=0|Max=30]"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(c);

    Config::Connect("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback(&CourseChange));

    Simulator::Stop(Seconds(100));

    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
