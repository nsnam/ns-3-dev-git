/*
 * Copyright (c) 2015 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Danilo Abrignani <danilo.abrignani@unibo.it>
 */

#include "ns3/buildings-helper.h"
#include "ns3/cc-helper.h"
#include "ns3/component-carrier.h"
#include "ns3/core-module.h"
// #include "ns3/config-store.h"

using namespace ns3;

void Print(ComponentCarrier cc);

int
main(int argc, char* argv[])
{
    CommandLine cmd(__FILE__);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::ComponentCarrier::UlBandwidth", UintegerValue(50));
    Config::SetDefault("ns3::ComponentCarrier::PrimaryCarrier", BooleanValue(true));

    // Parse again so you can override default values from the command line
    cmd.Parse(argc, argv);

    Ptr<CcHelper> cch = CreateObject<CcHelper>();
    cch->SetNumberOfComponentCarriers(2);

    std::map<uint8_t, ComponentCarrier> ccm = cch->EquallySpacedCcs();

    std::cout << " CcMap size " << ccm.size() << std::endl;
    for (auto it = ccm.begin(); it != ccm.end(); it++)
    {
        Print(it->second);
    }

    Simulator::Stop(Seconds(1.05));

    Simulator::Run();

    // GtkConfigStore config;
    // config.ConfigureAttributes ();

    Simulator::Destroy();
    return 0;
}

void
Print(ComponentCarrier cc)
{
    std::cout << " UlBandwidth " << uint16_t(cc.GetUlBandwidth()) << " DlBandwidth "
              << uint16_t(cc.GetDlBandwidth()) << " Dl Earfcn " << cc.GetDlEarfcn() << " Ul Earfcn "
              << cc.GetUlEarfcn() << " - Is this the Primary Channel? " << cc.IsPrimary()
              << std::endl;
}
