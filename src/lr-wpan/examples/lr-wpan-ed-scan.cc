/*
 * Copyright (c) 2022 Tokushima University, Japan.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/*         [00:01]                 [00:02]                                 [00:03]
 *  PAN 5 Coordinator             End Device                           PAN 7 Coordinator
 *    N0 (dev0)   ----------------N1 (dev1) ------------------------------ N2 (dev2)
 *   Channel 12             ED Scan Channels 11-14                         Channel 14
 *
 *       |--------10 m----------------|----------30 m -----------------------|
 *
 * This example demonstrate the usage of the MAC ED Scan primitive  as
 * described by IEEE 802.15.4-2011.
 *
 * At the beginning of the simulation, PAN coordinators N0 (Channel 12) and N2 (Channel 14)
 * start transmitting beacon frames on their respective channels.  At the same time,
 * end device N1 will scan through channels  (11,12,13 and 14) looking for energy.
 * i.e. multiple energy scans on each channel (PLME-ED calls). The results of the Max energy
 * read registered for each channel is shown after the last channel scan is finished.
 *
 * The radio uses the default Sensibility and Rx Power provided by the
 * LogDistancePropagationLossModel. The simulation might take a few seconds to complete.
 *
 * ED range: 0 - 255
 */

#include <ns3/constant-position-mobility-model.h>
#include <ns3/core-module.h>
#include <ns3/log.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/packet.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>

#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;

static void
ScanConfirm(MlmeScanConfirmParams params)
{
    if (params.m_status == MacStatus::SUCCESS && params.m_scanType == MLMESCAN_ED)
    {
        std::cout << Simulator::Now().As(Time::S) << "| Scan status SUCCESSFUL\n";
        std::cout << "Results for Energy Scan:"
                  << "\nPage: " << params.m_chPage << "\n";
        for (std::size_t i = 0; i < params.m_energyDetList.size(); i++)
        {
            std::cout << "Channel " << static_cast<uint32_t>(i + 11) << ": "
                      << +params.m_energyDetList[i] << "\n";
        }
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC));

    // Create 2 PAN coordinator nodes, and 1 end device
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    Ptr<Node> n2 = CreateObject<Node>();

    Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = CreateObject<LrWpanNetDevice>();

    dev0->SetAddress(Mac16Address("00:01"));
    dev1->SetAddress(Mac16Address("00:02"));
    dev2->SetAddress(Mac16Address("00:03"));

    // Configure Spectrum channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    dev0->SetChannel(channel);
    dev1->SetChannel(channel);
    dev2->SetChannel(channel);

    n0->AddDevice(dev0);
    n1->AddDevice(dev1);
    n2->AddDevice(dev2);

    // MAC layer Callbacks hooks
    MlmeScanConfirmCallback scb;
    scb = MakeCallback(&ScanConfirm);
    dev1->GetMac()->SetMlmeScanConfirmCallback(scb);

    Ptr<ConstantPositionMobilityModel> PanCoordinatorN0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    PanCoordinatorN0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(PanCoordinatorN0Mobility);

    Ptr<ConstantPositionMobilityModel> endDeviceN1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    endDeviceN1Mobility->SetPosition(Vector(10, 0, 0));
    dev1->GetPhy()->SetMobility(endDeviceN1Mobility);

    Ptr<ConstantPositionMobilityModel> PanCoordinatorN2Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    PanCoordinatorN2Mobility->SetPosition(Vector(40, 0, 0));
    dev2->GetPhy()->SetMobility(PanCoordinatorN2Mobility);

    // PAN coordinator N0 starts transmit beacons on channel 12
    MlmeStartRequestParams params;
    params.m_panCoor = true;
    params.m_PanId = 5;
    params.m_bcnOrd = 3;
    params.m_sfrmOrd = 3;
    params.m_logCh = 12;
    Simulator::ScheduleWithContext(1,
                                   Seconds(2.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   dev0->GetMac(),
                                   params);

    // PAN coordinator N2 transmit beacons on channel 14
    MlmeStartRequestParams params2;
    params2.m_panCoor = true;
    params2.m_PanId = 7;
    params2.m_bcnOrd = 3;
    params2.m_sfrmOrd = 3;
    params2.m_logCh = 14;
    Simulator::ScheduleWithContext(1,
                                   Seconds(2.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   dev2->GetMac(),
                                   params2);

    // Device 1 initiate channels scan on channels 11, 12, 13, and 14 looking for energy
    // Scan Channels represented by bits 0-26  (27 LSB)
    //                       ch 14  ch 11
    //                           |  |
    // 0x7800  = 0000000000000000111100000000000
    // scanDuration per channel = aBaseSuperframeDuration * (2^14+1) = 251.65 secs
    MlmeScanRequestParams scanParams;
    scanParams.m_chPage = 0;
    scanParams.m_scanChannels = 0x7800;
    scanParams.m_scanDuration = 14;
    scanParams.m_scanType = MLMESCAN_ED;
    Simulator::ScheduleWithContext(1,
                                   Seconds(2.0),
                                   &LrWpanMac::MlmeScanRequest,
                                   dev1->GetMac(),
                                   scanParams);

    Simulator::Stop(Seconds(2000));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
