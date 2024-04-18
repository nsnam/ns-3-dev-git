/*
 * Copyright (c) 2023 Tokushima University, Japan.
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

/*
 * [00:00:00:00:00:00:00:01 | 00:01]     [00:00:00:00:00:00:00:02 | ff:ff]
 *  PAN Coordinator 1 (PAN: 5)                      End Device
 *       |---------------------100m-----------------------|
 *  Channel 12                                (Orphan Scan channels 11-14)
 *
 *
 * This example demonstrate the usage of the MAC MLME-SCAN.request (ORPHAN scan) primitive as
 * described by IEEE 802.15.4-2011 (See Figures 14 and 15).
 *
 * Orphan scan is used typically on devices as a result of repeated communication failures
 * (For example, lossing too many ACK). An orphan scan represents the attempt of a device to
 * relocate its coordinator. In some situations, it can be used by devices higher layers to not only
 * rejoin a network but also join a network for the first time (Like in the joining through
 * orphaning mechanism described in Zigbee networks).
 *
 * In this example, the end device is set to scan 4 channels (11~14) for a period of
 * macResponseWaitTime until it finally gets in contact with the coordinator.
 * On contact, the coordinator responds to the device (via coordinator realignment command)
 * an assign it a short address. The detailed sequence of events is as following:
 *
 * 1) [Time 2s] The coordinator start a network in channel 12.
 * 2) [Time 3s] The end device start orphan scan and transmits a orphan
 *    notification cmd on channel 11.
 * 3) No response is received in channel 11, therefore, the device ends scanning on
 *    channel 11 after macResponseWaitTime and repeats step 2 in channel 12.
 * 4) [Time 3.00269s] The orphan notification command is received by the coordinator in
 *    channel 12. The coordinator verify the requesting device and replies to the device
 *    with a coordinator realignment command containing the assigned short address [DE:AF].
 * 5) [Time 3.00646s] The device receives the coordinator realignment command, update its
 *    macPanId, macShortAddress, macCoordShortAddress and macCoordExtAddress.
 * 6) Scanning of the remaining channels 13 and 14 is cancelled.
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
ScanConfirm(Ptr<LrWpanNetDevice> device, MlmeScanConfirmParams params)
{
    if (params.m_status == MacStatus::SUCCESS)
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress()
                  << "] MLME-SCAN.confirm: Active scan status SUCCESSFUL "
                  << "(Coordinator found and address assigned) \n";
    }
    else if (params.m_status == MacStatus::NO_BEACON)
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress()
                  << "] MLME-SCAN.confirm: Could not locate coordinator "
                  << "(Coord realignment command not received) "
                  << "status: " << params.m_status << "\n";
    }
    else
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress()
                  << "] MLME-SCAN.confirm: An error occurred during scanning, "
                  << "status: " << params.m_status << "\n";
    }
}

static void
OrphanIndication(Ptr<LrWpanNetDevice> device, MlmeOrphanIndicationParams params)
{
    // The steps taken by the coordinator on the event of an orphan indication
    // are meant to be implemented by the next higher layer and are out of the scope of the
    // standard. In this example, we simply accept the request , assign a fixed short address
    // [DE:AF] and respond to the requesting device using a MLME-ORPHAN.response.

    std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
              << device->GetMac()->GetShortAddress() << " | "
              << device->GetMac()->GetExtendedAddress()
              << "] MLME-ORPHAN.indication: Orphan Notification received, processing...\n";

    MlmeOrphanResponseParams respParams;
    respParams.m_assocMember = true;
    respParams.m_orphanAddr = params.m_orphanAddr;
    respParams.m_shortAddr = Mac16Address("DE:AF");

    Simulator::ScheduleNow(&LrWpanMac::MlmeOrphanResponse, device->GetMac(), respParams);
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC));

    // Create 2 PAN coordinator nodes, and 1 end device
    Ptr<Node> coord1 = CreateObject<Node>();
    Ptr<Node> endNode = CreateObject<Node>();

    Ptr<LrWpanNetDevice> coord1NetDevice = CreateObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> endNodeNetDevice = CreateObject<LrWpanNetDevice>();

    // PAN Coordinators configurations require to set both, the EUI-64 (extended address)
    // and to assign their own short address.
    coord1NetDevice->GetMac()->SetExtendedAddress(Mac64Address("00:00:00:00:00:00:00:01"));
    coord1NetDevice->GetMac()->SetShortAddress(Mac16Address("00:01"));

    // Other devices must have only its EUI-64 and later on, their short address is
    // potentially assigned by the coordinator.
    endNodeNetDevice->GetMac()->SetExtendedAddress(Mac64Address("00:00:00:00:00:00:00:02"));

    // Configure Spectrum channel
    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();
    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    coord1NetDevice->SetChannel(channel);
    endNodeNetDevice->SetChannel(channel);

    coord1->AddDevice(coord1NetDevice);
    endNode->AddDevice(endNodeNetDevice);

    // Mobility
    Ptr<ConstantPositionMobilityModel> coord1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    coord1Mobility->SetPosition(Vector(0, 0, 0));
    coord1NetDevice->GetPhy()->SetMobility(coord1Mobility);

    Ptr<ConstantPositionMobilityModel> endNodeMobility =
        CreateObject<ConstantPositionMobilityModel>();
    endNodeMobility->SetPosition(Vector(100, 0, 0));
    endNodeNetDevice->GetPhy()->SetMobility(endNodeMobility);

    // MAC layer Callbacks hooks
    endNodeNetDevice->GetMac()->SetMlmeScanConfirmCallback(
        MakeBoundCallback(&ScanConfirm, endNodeNetDevice));

    coord1NetDevice->GetMac()->SetMlmeOrphanIndicationCallback(
        MakeBoundCallback(&OrphanIndication, coord1NetDevice));

    /////////////////
    // ORPHAN SCAN //
    /////////////////

    // PAN coordinator N0 (PAN 5) is set to channel 12 in non-beacon mode
    // but answer to beacon request and orphan notification commands.
    MlmeStartRequestParams params;
    params.m_panCoor = true;
    params.m_PanId = 5;
    params.m_bcnOrd = 15;
    params.m_sfrmOrd = 15;
    params.m_logCh = 12;
    Simulator::ScheduleWithContext(1,
                                   Seconds(2.0),
                                   &LrWpanMac::MlmeStartRequest,
                                   coord1NetDevice->GetMac(),
                                   params);

    // End device N1 is set to scan 4 channels looking for the presence of a coordinator.
    // On each channel, a single orphan notification command is sent and a response is
    // waited for a maximum time of macResponseWaitTime. If a reply is received from a
    // coordinator within this time (coordinator realignment command), the programmed scans on
    // other channels is suspended.
    // Scan Channels are represented by bits 0-26  (27 LSB)
    //                       ch 14  ch 11
    //                           |  |
    // 0x7800  = 0000000000000000111100000000000
    MlmeScanRequestParams scanParams;
    scanParams.m_chPage = 0;
    scanParams.m_scanChannels = 0x7800;
    scanParams.m_scanType = MLMESCAN_ORPHAN;
    Simulator::ScheduleWithContext(1,
                                   Seconds(3.0),
                                   &LrWpanMac::MlmeScanRequest,
                                   endNodeNetDevice->GetMac(),
                                   scanParams);

    Simulator::Stop(Seconds(2000));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
