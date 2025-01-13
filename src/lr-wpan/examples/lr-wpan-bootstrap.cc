/*
 * Copyright (c) 2022 Tokushima University, Japan.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/*
 * This example demonstrates the use of lr-wpan bootstrap (i.e. IEEE 802.15.4 Scan & Association).
 * For a full description of this process check IEEE 802.15.4-2011 Section 5.1.3.1 and Figure 18.
 *
 * In this example, we create a grid topology of 100 nodes.
 * Additionally, 2 coordinators are created and set in beacon-enabled mode.
 * Coordinator 1 = Channel 14, Pan ID 5 , Coordinator 2 = Channel 12, Pan ID 7.
 * Nodes start scanning channels 11-14 looking for beacons for a defined duration (PASSIVE SCAN).
 * The scanning start time is slightly different for each node to avoid a storm of association
 * requests. When a node scan is completed, an association request is send to one coordinator based
 * on the LQI results of the scan. A node may not find any beacons if the coordinator is outside its
 * communication range. An association request may not be send if LQI is too low for an association.
 *
 * The coordinator in PAN 5 runs in extended addressing mode and do not assign short addresses.
 * The coordinator in PAN 7 runs in short addressing mode and assign short addresses.
 *
 * At the end of the simulation, an animation is generated (lrwpan-bootstrap.xml), showing the
 * results of the association with each coordinator. This simulation can take a few seconds to
 * complete.
 */

#include "ns3/core-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/propagation-module.h"
#include "ns3/spectrum-module.h"

#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;

NodeContainer nodes;
NodeContainer coordinators;
AnimationInterface* anim = nullptr;

static void
UpdateAnimation()
{
    std::cout << Simulator::Now().As(Time::S) << " | Animation Updated, End of simulation.\n";
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        anim->UpdateNodeSize(i, 5, 5);
        Ptr<Node> node = nodes.Get(i);
        Ptr<NetDevice> netDevice = node->GetDevice(0);
        Ptr<LrWpanNetDevice> lrwpanDevice = DynamicCast<LrWpanNetDevice>(netDevice);
        int panId = lrwpanDevice->GetMac()->GetPanId();

        switch (panId)
        {
        case 5:
            anim->UpdateNodeColor(node, 0, 0, 255);
            break;
        case 7:
            anim->UpdateNodeColor(node, 0, 51, 102);
            break;
        default:
            break;
        }
    }
}

static void
ScanConfirm(Ptr<LrWpanNetDevice> device, MlmeScanConfirmParams params)
{
    // The algorithm to select which coordinator to associate is not
    // covered by the standard. In this case, we use the coordinator
    // with the highest LQI value obtained from a passive scan and make
    // sure this coordinator allows association.

    if (params.m_status == MacStatus::SUCCESS)
    {
        // Select the coordinator with the highest LQI from the PAN Descriptor List
        int maxLqi = 0;
        int panDescIndex = 0;
        if (!params.m_panDescList.empty())
        {
            for (uint32_t i = 0; i < params.m_panDescList.size(); i++)
            {
                if (params.m_panDescList[i].m_linkQuality > maxLqi)
                {
                    maxLqi = params.m_panDescList[i].m_linkQuality;
                    panDescIndex = i;
                }
            }

            // Only request association if the coordinator is permitting association at this moment.
            SuperframeField superframe(params.m_panDescList[panDescIndex].m_superframeSpec);
            if (superframe.IsAssocPermit())
            {
                std::string addressing;
                if (params.m_panDescList[panDescIndex].m_coorAddrMode == SHORT_ADDR)
                {
                    addressing = "Short";
                }
                else if (params.m_panDescList[panDescIndex].m_coorAddrMode == EXT_ADDR)
                {
                    addressing = "Ext";
                }

                std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId()
                          << " [" << device->GetMac()->GetShortAddress() << " | "
                          << device->GetMac()->GetExtendedAddress() << "]"
                          << " MLME-scan.confirm:  Selected PAN ID "
                          << params.m_panDescList[panDescIndex].m_coorPanId
                          << "| Coord addressing mode: " << addressing << " | LQI "
                          << static_cast<int>(params.m_panDescList[panDescIndex].m_linkQuality)
                          << "\n";

                if (params.m_panDescList[panDescIndex].m_linkQuality >= 127)
                {
                    MlmeAssociateRequestParams assocParams;
                    assocParams.m_chNum = params.m_panDescList[panDescIndex].m_logCh;
                    assocParams.m_chPage = params.m_panDescList[panDescIndex].m_logChPage;
                    assocParams.m_coordPanId = params.m_panDescList[panDescIndex].m_coorPanId;
                    assocParams.m_coordAddrMode = params.m_panDescList[panDescIndex].m_coorAddrMode;
                    CapabilityField capability;

                    if (params.m_panDescList[panDescIndex].m_coorAddrMode ==
                        AddressMode::SHORT_ADDR)
                    {
                        assocParams.m_coordAddrMode = AddressMode::SHORT_ADDR;
                        assocParams.m_coordShortAddr =
                            params.m_panDescList[panDescIndex].m_coorShortAddr;
                        capability.SetShortAddrAllocOn(true);
                    }
                    else if (assocParams.m_coordAddrMode == AddressMode::EXT_ADDR)
                    {
                        assocParams.m_coordAddrMode = AddressMode::EXT_ADDR;
                        assocParams.m_coordExtAddr =
                            params.m_panDescList[panDescIndex].m_coorExtAddr;
                        assocParams.m_coordShortAddr = Mac16Address("ff:fe");
                        capability.SetShortAddrAllocOn(false);
                    }
                    assocParams.m_capabilityInfo = capability.GetCapability();

                    Simulator::ScheduleNow(&LrWpanMac::MlmeAssociateRequest,
                                           device->GetMac(),
                                           assocParams);
                }
                else
                {
                    std::cout << Simulator::Now().As(Time::S) << " Node "
                              << device->GetNode()->GetId() << " ["
                              << device->GetMac()->GetShortAddress() << " | "
                              << device->GetMac()->GetExtendedAddress() << "]"
                              << " MLME-scan.confirm: Beacon found but link quality too low for "
                                 "association.\n";
                }
            }
        }
        else
        {
            std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId()
                      << " [" << device->GetMac()->GetShortAddress() << " | "
                      << device->GetMac()->GetExtendedAddress()
                      << "] MLME-scan.confirm: Beacon not found.\n";
        }
    }
    else
    {
        std::cout << Simulator::Now().As(Time::S) << " [" << device->GetMac()->GetShortAddress()
                  << " | " << device->GetMac()->GetExtendedAddress()
                  << "]  error occurred, scan failed.\n";
    }
}

static void
AssociateIndication(Ptr<LrWpanNetDevice> device, MlmeAssociateIndicationParams params)
{
    // This is typically implemented by the coordinator next layer (3rd layer or higher).
    // The steps described below are out of the scope of the standard.

    // Here the 3rd layer should check:
    //    a) Whether or not the device was previously associated with this PAN
    //       (the coordinator keeps a list).
    //    b) The coordinator have sufficient resources available to allow the
    //       association.
    // If the association fails, status = 1 or 2 and assocShortAddr = FFFF.

    // In this example, the coordinator accepts every association request and have no association
    // limits. Furthermore, previous associated devices are not checked.

    // When short address allocation is on (set initially in the association request), the
    // coordinator is supposed to assign a short address. In here, we just do a dummy address
    // assign. The assigned short address is just a truncated version of the device existing
    // extended address (i.e the default short address).

    MlmeAssociateResponseParams assocRespParams;

    assocRespParams.m_extDevAddr = params.m_extDevAddr;
    assocRespParams.m_status = MacStatus::SUCCESS;
    CapabilityField capability;
    capability.SetCapability(params.capabilityInfo);

    if (capability.IsShortAddrAllocOn())
    {
        // Truncate the extended address and make an assigned
        // short address based on this. This mechanism is not described by the standard.
        // It is just implemented here as a quick and dirty way to assign short addresses.
        uint8_t buffer64MacAddr[8];
        uint8_t buffer16MacAddr[2];

        params.m_extDevAddr.CopyTo(buffer64MacAddr);
        buffer16MacAddr[1] = buffer64MacAddr[7];
        buffer16MacAddr[0] = buffer64MacAddr[6];

        Mac16Address shortAddr;
        shortAddr.CopyFrom(buffer16MacAddr);
        assocRespParams.m_assocShortAddr = shortAddr;
    }
    else
    {
        // If Short Address allocation flag is false, the device will
        // use its extended address to send data packets and short address will be
        // equal to ff:fe. See 802.15.4-2011 (Section 5.3.2.2)
        assocRespParams.m_assocShortAddr = Mac16Address("ff:fe");
    }

    Simulator::ScheduleNow(&LrWpanMac::MlmeAssociateResponse, device->GetMac(), assocRespParams);
}

static void
CommStatusIndication(Ptr<LrWpanNetDevice> device, MlmeCommStatusIndicationParams params)
{
    // Used by coordinator higher layer to inform results of a
    // association procedure from its mac layer.This is implemented by other protocol stacks
    // and is only here for demonstration purposes.
    switch (params.m_status)
    {
    case MacStatus::TRANSACTION_EXPIRED:
        std::cout << Simulator::Now().As(Time::S) << " Coordinator " << device->GetNode()->GetId()
                  << " [" << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-comm-status.indication: Transaction for device " << params.m_dstExtAddr
                  << " EXPIRED in pending transaction list\n";
        break;
    case MacStatus::NO_ACK:
        std::cout << Simulator::Now().As(Time::S) << " Coordinator " << device->GetNode()->GetId()
                  << " [" << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-comm-status.indication: NO ACK from " << params.m_dstExtAddr
                  << " device registered in the pending transaction list\n";
        break;

    case MacStatus::CHANNEL_ACCESS_FAILURE:
        std::cout << Simulator::Now().As(Time::S) << " Coordinator " << device->GetNode()->GetId()
                  << " [" << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-comm-status.indication: CHANNEL ACCESS problem in transaction for "
                  << params.m_dstExtAddr << " registered in the pending transaction list\n";
        break;

    default:
        break;
    }
}

static void
AssociateConfirm(Ptr<LrWpanNetDevice> device, MlmeAssociateConfirmParams params)
{
    // Used by device higher layer to inform the results of a
    // association procedure from its mac layer.This is implemented by other protocol stacks
    // and is only here for demonstration purposes.
    if (params.m_status == MacStatus::SUCCESS)
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-associate.confirm: Association with coordinator successful."
                  << " (PAN: " << device->GetMac()->GetPanId()
                  << " | CoordShort: " << device->GetMac()->GetCoordShortAddress()
                  << " | CoordExt: " << device->GetMac()->GetCoordExtAddress() << ")\n";
    }
    else if (params.m_status == MacStatus::NO_ACK)
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-associate.confirm: Association with coordinator FAILED (NO ACK).\n";
    }
    else
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-associate.confirm: Association with coordinator FAILED.\n";
    }
}

static void
PollConfirm(Ptr<LrWpanNetDevice> device, MlmePollConfirmParams params)
{
    if (params.m_status == MacStatus::CHANNEL_ACCESS_FAILURE)
    {
        std::cout
            << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
            << device->GetMac()->GetShortAddress() << " | "
            << device->GetMac()->GetExtendedAddress() << "]"
            << " MLME-poll.confirm:  CHANNEL ACCESS problem when sending a data request command.\n";
    }
    else if (params.m_status == MacStatus::NO_ACK)
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-poll.confirm: Data Request Command FAILED (NO ACK).\n";
    }
    else if (params.m_status != MacStatus::SUCCESS)
    {
        std::cout << Simulator::Now().As(Time::S) << " Node " << device->GetNode()->GetId() << " ["
                  << device->GetMac()->GetShortAddress() << " | "
                  << device->GetMac()->GetExtendedAddress() << "]"
                  << " MLME-poll.confirm: Data Request command FAILED.\n";
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));

    nodes.Create(100);
    coordinators.Create(2);

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(30.0),
                                  "DeltaY",
                                  DoubleValue(30.0),
                                  "GridWidth",
                                  UintegerValue(20),
                                  "LayoutType",
                                  StringValue("RowFirst"));

    mobility.Install(nodes);

    Ptr<ListPositionAllocator> listPositionAlloc = CreateObject<ListPositionAllocator>();
    listPositionAlloc->Add(Vector(210, 50, 0)); // Coordinator 1 mobility (210,50,0)
    listPositionAlloc->Add(Vector(360, 50, 0)); // Coordinator 2 mobility (360,50,0)

    mobility.SetPositionAllocator(listPositionAlloc);
    mobility.Install(coordinators);

    LrWpanHelper lrWpanHelper;
    lrWpanHelper.SetPropagationDelayModel("ns3::ConstantSpeedPropagationDelayModel");
    lrWpanHelper.AddPropagationLossModel("ns3::LogDistancePropagationLossModel");

    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
    lrwpanDevices.Add(lrWpanHelper.Install(coordinators));

    // Set the extended address to all devices (EUI-64)
    lrWpanHelper.SetExtendedAddresses(lrwpanDevices);

    // Devices hooks & MAC MLME-scan primitive set
    for (auto i = nodes.Begin(); i != nodes.End(); i++)
    {
        Ptr<Node> node = *i;
        Ptr<NetDevice> netDevice = node->GetDevice(0);
        Ptr<LrWpanNetDevice> lrwpanDevice = DynamicCast<LrWpanNetDevice>(netDevice);
        lrwpanDevice->GetMac()->SetMlmeScanConfirmCallback(
            MakeBoundCallback(&ScanConfirm, lrwpanDevice));
        lrwpanDevice->GetMac()->SetMlmeAssociateConfirmCallback(
            MakeBoundCallback(&AssociateConfirm, lrwpanDevice));
        lrwpanDevice->GetMac()->SetMlmePollConfirmCallback(
            MakeBoundCallback(&PollConfirm, lrwpanDevice));

        // Devices initiate channels scan on channels 11, 12, 13, and 14 looking for beacons
        // Scan Channels represented by bits 0-26  (27 LSB)
        //                       ch 14  ch 11
        //                           |  |
        // 0x7800  = 0000000000000000111100000000000

        MlmeScanRequestParams scanParams;
        scanParams.m_chPage = 0;
        scanParams.m_scanChannels = 0x7800;
        scanParams.m_scanDuration = 14;
        scanParams.m_scanType = MLMESCAN_PASSIVE;

        // We start the scanning process 100 milliseconds apart for each device
        // to avoid a storm of association requests with the coordinators
        Time jitter = Seconds(2) + MilliSeconds(std::distance(nodes.Begin(), i) * 100);
        Simulator::ScheduleWithContext(node->GetId(),
                                       jitter,
                                       &LrWpanMac::MlmeScanRequest,
                                       lrwpanDevice->GetMac(),
                                       scanParams);
    }

    // Coordinator hooks
    for (auto i = coordinators.Begin(); i != coordinators.End(); i++)
    {
        Ptr<Node> coor = *i;
        Ptr<NetDevice> netDevice = coor->GetDevice(0);
        Ptr<LrWpanNetDevice> lrwpanDevice = DynamicCast<LrWpanNetDevice>(netDevice);
        lrwpanDevice->GetMac()->SetMlmeAssociateIndicationCallback(
            MakeBoundCallback(&AssociateIndication, lrwpanDevice));
        lrwpanDevice->GetMac()->SetMlmeCommStatusIndicationCallback(
            MakeBoundCallback(&CommStatusIndication, lrwpanDevice));
    }

    Ptr<Node> coor1 = coordinators.Get(0);
    Ptr<NetDevice> netDeviceCoor1 = coor1->GetDevice(0);
    Ptr<LrWpanNetDevice> coor1Device = DynamicCast<LrWpanNetDevice>(netDeviceCoor1);

    Ptr<Node> coor2 = coordinators.Get(1);
    Ptr<NetDevice> netDeviceCoor2 = coor2->GetDevice(0);
    Ptr<LrWpanNetDevice> coor2Device = DynamicCast<LrWpanNetDevice>(netDeviceCoor2);

    // Coordinators require that their short address is explicitly set.
    // Either FF:FE to indicate that only extended addresses will be used in the following
    // data communications or any other value (except for FF:FF) to indicate that the coordinator
    // will use the short address in these communications.
    // The default short address for all devices is FF:FF (unassigned/no associated).

    // coor1 (PAN 5) = extended addressing mode coor2 (PAN 7) = short addressing mode
    coor1Device->GetMac()->SetShortAddress(Mac16Address("FF:FE"));
    coor2Device->GetMac()->SetShortAddress(Mac16Address("CA:FE"));

    // PAN coordinator 1 (PAN 5) transmits beacons on channel 12
    MlmeStartRequestParams params;
    params.m_panCoor = true;
    params.m_PanId = 5;
    params.m_bcnOrd = 3;
    params.m_sfrmOrd = 3;
    params.m_logCh = 12;

    Simulator::ScheduleWithContext(coor1Device->GetNode()->GetId(),
                                   Seconds(2),
                                   &LrWpanMac::MlmeStartRequest,
                                   coor1Device->GetMac(),
                                   params);

    // PAN coordinator N2 (PAN 7) transmits beacons on channel 14
    MlmeStartRequestParams params2;
    params2.m_panCoor = true;
    params2.m_PanId = 7;
    params2.m_bcnOrd = 3;
    params2.m_sfrmOrd = 3;
    params2.m_logCh = 14;

    Simulator::ScheduleWithContext(coor2Device->GetNode()->GetId(),
                                   Seconds(2),
                                   &LrWpanMac::MlmeStartRequest,
                                   coor2Device->GetMac(),
                                   params2);

    anim = new AnimationInterface("lrwpan-bootstrap.xml");
    anim->SkipPacketTracing();
    anim->UpdateNodeDescription(coordinators.Get(0), "Coordinator (PAN 5)");
    anim->UpdateNodeDescription(coordinators.Get(1), "Coordinator (PAN 7)");
    anim->UpdateNodeColor(coordinators.Get(0), 0, 0, 255);
    anim->UpdateNodeColor(coordinators.Get(1), 0, 51, 102);
    anim->UpdateNodeSize(nodes.GetN(), 9, 9);
    anim->UpdateNodeSize(nodes.GetN() + 1, 9, 9);

    Simulator::Schedule(Seconds(1499), &UpdateAnimation);
    Simulator::Stop(Seconds(1500));
    Simulator::Run();

    Simulator::Destroy();
    delete anim;
    return 0;
}
