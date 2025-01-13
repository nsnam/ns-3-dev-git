/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#include "ns3/buildings-helper.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
// #include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Change the position of a node.
 *
 * This function will move a node with a x coordinate greater than 10 m
 * to a x equal to 5 m, and less than or equal to 10 m to 10 Km
 *
 * @param node The node to move.
 */
static void
ChangePosition(Ptr<Node> node)
{
    Ptr<MobilityModel> mobility = node->GetObject<MobilityModel>();
    Vector pos = mobility->GetPosition();

    if (pos.x <= 10.0)
    {
        pos.x = 100000.0; // force CQI to 0
    }
    else
    {
        pos.x = 5.0;
    }
    mobility->SetPosition(pos);
}

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    // to save a template default attribute file run it like this:
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Save
    // --ns3::ConfigStore::FileFormat=RawText"
    //
    // to load a previously created default attribute file
    // ./ns3 run src/lte/examples/lena-first-sim --command-template="%s
    // --ns3::ConfigStore::Filename=input-defaults.txt --ns3::ConfigStore::Mode=Load
    // --ns3::ConfigStore::FileFormat=RawText"

    ConfigStore inputConfig;
    inputConfig.ConfigureDefaults();

    // parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    lteHelper->SetAttribute("PathlossModel", StringValue("ns3::FriisSpectrumPropagationLossModel"));
    // Uncomment to enable logging
    // lteHelper->EnableLogComponents ();

    // Create Nodes: eNodeB and UE
    NodeContainer enbNodes;
    NodeContainer ueNodes;
    enbNodes.Create(1);
    ueNodes.Create(1);

    // Install Mobility Model
    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(enbNodes);
    BuildingsHelper::Install(enbNodes);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(ueNodes);
    BuildingsHelper::Install(ueNodes);

    // Create Devices and install them in the Nodes (eNB and UE)
    NetDeviceContainer enbDevs;
    NetDeviceContainer ueDevs;
    //   lteHelper->SetSchedulerType ("ns3::RrFfMacScheduler");
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    lteHelper->SetSchedulerAttribute("CqiTimerThreshold", UintegerValue(3));
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    lteHelper->EnableRlcTraces();
    lteHelper->EnableMacTraces();

    // Attach a UE to a eNB
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    Simulator::Schedule(Seconds(0.010), &ChangePosition, ueNodes.Get(0));
    Simulator::Schedule(Seconds(0.020), &ChangePosition, ueNodes.Get(0));

    // Activate a data radio bearer
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    Simulator::Stop(Seconds(0.030));

    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
