/*
 * Copyright (c) 2014 Piotr Gawlowicz
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Piotr Gawlowicz <gawlowicz.p@gmail.com>
 *
 */

#include "ns3/buildings-helper.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"

using namespace ns3;

/*
 * This example show how to configure and how Uplink Power Control works.
 */

int
main(int argc, char* argv[])
{
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(false));

    double eNbTxPower = 30;
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(eNbTxPower));
    Config::SetDefault("ns3::LteUePhy::TxPower", DoubleValue(10.0));
    Config::SetDefault("ns3::LteUePhy::EnableUplinkPowerControl", BooleanValue(true));

    Config::SetDefault("ns3::LteUePowerControl::ClosedLoop", BooleanValue(true));
    Config::SetDefault("ns3::LteUePowerControl::AccumulationEnabled", BooleanValue(true));
    Config::SetDefault("ns3::LteUePowerControl::Alpha", DoubleValue(1.0));

    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();

    uint16_t bandwidth = 25;
    double d1 = 0;

    // Create Nodes: eNodeB and UE
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);
    ueNodes.Create(1);
    NodeContainer allNodes = NodeContainer(enbNodes, ueNodes);

    /*   the topology is the following:
     *
     *   eNB1-------------------------UE
     *                  d1
     */

    // Install Mobility Model
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0.0, 0.0, 0.0)); // eNB1
    positionAlloc->Add(Vector(d1, 0.0, 0.0));  // UE1

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator(positionAlloc);
    mobility.Install(allNodes);

    // Create Devices and install them in the Nodes (eNB and UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");

    lteHelper->SetEnbDeviceAttribute("DlBandwidth", UintegerValue(bandwidth));
    lteHelper->SetEnbDeviceAttribute("UlBandwidth", UintegerValue(bandwidth));

    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate a data radio bearer
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    Simulator::Stop(Seconds(0.500));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
