/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo <marco.miozzo@cttc.es>
 */

#include "ns3/buildings-helper.h"
#include "ns3/config-store.h"
#include "ns3/core-module.h"
#include "ns3/lte-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/string.h"

#include <fstream>
// #include "ns3/gtk-config-store.h"

using namespace ns3;

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

    // ConfigStore inputConfig;
    // inputConfig.ConfigureDefaults ();

    // parse again so you can override default values from the command line
    // cmd.Parse (argc, argv);

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    // Uncomment to enable logging
    // lteHelper->EnableLogComponents ();

    lteHelper->SetAttribute("FadingModel", StringValue("ns3::TraceFadingLossModel"));

    std::ifstream ifTraceFile;
    ifTraceFile.open("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad",
                     std::ifstream::in);
    if (ifTraceFile.good())
    {
        // script launched by test.py
        lteHelper->SetFadingModelAttribute(
            "TraceFilename",
            StringValue("../../src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }
    else
    {
        // script launched as an example
        lteHelper->SetFadingModelAttribute(
            "TraceFilename",
            StringValue("src/lte/model/fading-traces/fading_trace_EPA_3kmph.fad"));
    }

    // these parameters have to be set only in case of the trace format
    // differs from the standard one, that is
    // - 10 seconds length trace
    // - 10,000 samples
    // - 0.5 seconds for window size
    // - 100 RB
    lteHelper->SetFadingModelAttribute("TraceLength", TimeValue(Seconds(10)));
    lteHelper->SetFadingModelAttribute("SamplesNum", UintegerValue(10000));
    lteHelper->SetFadingModelAttribute("WindowSize", TimeValue(Seconds(0.5)));
    lteHelper->SetFadingModelAttribute("RbNum", UintegerValue(100));

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
    enbDevs = lteHelper->InstallEnbDevice(enbNodes);
    ueDevs = lteHelper->InstallUeDevice(ueNodes);

    // Attach a UE to a eNB
    lteHelper->Attach(ueDevs, enbDevs.Get(0));

    // Activate an EPS bearer
    EpsBearer::Qci q = EpsBearer::GBR_CONV_VOICE;
    EpsBearer bearer(q);
    lteHelper->ActivateDataRadioBearer(ueDevs, bearer);

    Simulator::Stop(Seconds(0.005));

    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}
