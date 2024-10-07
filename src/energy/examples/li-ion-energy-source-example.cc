/*
 * Copyright (c) 2010 Andrea Sacco
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Andrea Sacco <andrea.sacco85@gmail.com>
 */

#include "ns3/command-line.h"
#include "ns3/energy-source-container.h"
#include "ns3/li-ion-energy-source.h"
#include "ns3/log.h"
#include "ns3/simple-device-energy-model.h"
#include "ns3/simulator.h"

using namespace ns3;
using namespace ns3::energy;

/**
 * In this simple example, we show how to create and drain energy from a
 * LiIonEnergySource.
 * We make a series of discharge calls to the energy source class with
 * different current drain and duration until all the energy is depleted
 * from the cell.
 *
 * Every 20 seconds it is printed out the actual cell voltage to verify
 * that it follows the discharge curve of the datasheet [1].
 *
 * At the end of the example it is verified that after the energy depletion
 * call, the cell voltage is below the threshold voltage.
 *
 * References:
 * [1] Panasonic CGR18650DA Datasheet,
 * http://www.panasonic.com/industrial/includes/pdf/Panasonic_LiIon_CGR18650DA.pdf
 */

// NS_DEPRECATED_3_43() - tag for future removal
// LiIonEnergySource was deprecated in commit
// https://gitlab.com/nsnam/ns-3-dev/-/commit/086913b0
//
// The new battery model is illustrated in
// `src/energy/examples/generic-battery-discharge-example.cc`

NS_WARNING_PUSH_DEPRECATED;

static void
PrintCellInfo(Ptr<LiIonEnergySource> es)
{
    NS_WARNING_POP;
    std::cout << "At " << Simulator::Now().As(Time::S)
              << " Cell voltage: " << es->GetSupplyVoltage()
              << " V Remaining Capacity: " << es->GetRemainingEnergy() / (3.6 * 3600) << " Ah"
              << std::endl;

    if (!Simulator::IsFinished())
    {
        Simulator::Schedule(Seconds(20), &PrintCellInfo, es);
    }
}

int
main(int argc, char** argv)
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // uncomment below to see the energy consumption details
    // LogComponentEnable ("LiIonEnergySource", LOG_LEVEL_DEBUG);

    Ptr<Node> node = CreateObject<Node>();

    Ptr<SimpleDeviceEnergyModel> sem = CreateObject<SimpleDeviceEnergyModel>();
    Ptr<EnergySourceContainer> esCont = CreateObject<EnergySourceContainer>();
    NS_WARNING_PUSH_DEPRECATED;
    Ptr<LiIonEnergySource> es = CreateObject<LiIonEnergySource>();
    NS_WARNING_POP;
    esCont->Add(es);
    es->SetNode(node);
    sem->SetEnergySource(es);
    es->AppendDeviceEnergyModel(sem);
    sem->SetNode(node);
    node->AggregateObject(esCont);

    Time now = Simulator::Now();

    // discharge at 2.33 A for 1700 seconds
    sem->SetCurrentA(2.33);
    now += Seconds(1701);

    // discharge at 4.66 A for 628 seconds
    Simulator::Schedule(now, &SimpleDeviceEnergyModel::SetCurrentA, sem, 4.66);
    now += Seconds(600);

    PrintCellInfo(es);

    Simulator::Stop(now);
    Simulator::Run();
    Simulator::Destroy();

    // the cell voltage should be under 3.3v
    DoubleValue v;
    es->GetAttribute("ThresholdVoltage", v);
    NS_ASSERT(es->GetSupplyVoltage() <= v.Get());

    return 0;
}
