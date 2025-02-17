/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/**
 * This example shows the procedure to form a multi-hop network using a direct-join
 * procedure (a.k.a. orphaning procedure). The procedure requires a sequence of primitive
 * calls on a specific order in the indicated devices
 *
 *
 *  1-Network-Formation                       3-Join(dev1)                       6-Join(dev2)
 *  2-Direct-Join (dev1)                     4-Router-Start(dev1)
 *                                           5-Direct-Join (dev2)
 *
 *  Zigbee Coordinator(ZC)                           Router(ZR)                         End Device
 *  (dev 0) ------------------------------------ (dev 1) ---------------------------- (dev 2)
 *  [00:00:00:00:00:00:CA:FE]          [00:00:00:00:00:00:00:01]         [00:00:00:00:00:00:00:02]
 *  [00:00]                            [short addr assigned by ZC]       [short addr assigned by ZR]
 *
 *  1- Channel scanning, pan id selection, start network (initiation of zigbee coordinator)
 *  2- Manual registration of the joined device 1 into the zigbee coordinator
 *  3- Confirmation of the joined device 1 with the coordinator
 *  4- Initiate device 1 as a router
 *  5- Manual registration of device 2 into the zigbee router (device 1)
 *  6- Confirmation of the joined device 2 with the router.
 */

#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/zigbee-module.h"

#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;
using namespace ns3::zigbee;

NS_LOG_COMPONENT_DEFINE("ZigbeeDirectJoin");

static void
NwkDataIndication(Ptr<ZigbeeStack> stack, NldeDataIndicationParams params, Ptr<Packet> p)
{
    std::cout << "Received packet of size " << p->GetSize() << "\n";
}

static void
NwkNetworkFormationConfirm(Ptr<ZigbeeStack> stack, NlmeNetworkFormationConfirmParams params)
{
    std::cout << "NlmeNetworkFormationConfirmStatus = " << params.m_status << "\n";
}

static void
NwkDirectJoinConfirm(Ptr<ZigbeeStack> stack, NlmeDirectJoinConfirmParams params)
{
    std::cout << "NlmeDirectJoinConfirmStatus = " << params.m_status << "\n";
}

static void
NwkJoinConfirm(Ptr<ZigbeeStack> stack, NlmeJoinConfirmParams params)
{
    if (params.m_status == NwkStatus::SUCCESS)
    {
        std::cout << " The device join the network SUCCESSFULLY with short address "
                  << params.m_networkAddress << "\n";
    }
    else
    {
        std::cout << " The device FAILED to join the network with status " << params.m_status
                  << "\n";
    }
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    LogComponentEnable("ZigbeeNwk", LOG_LEVEL_DEBUG);

    NodeContainer nodes;
    nodes.Create(3);

    //// Configure MAC

    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
    Ptr<LrWpanNetDevice> dev0 = lrwpanDevices.Get(0)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = lrwpanDevices.Get(1)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = lrwpanDevices.Get(2)->GetObject<LrWpanNetDevice>();

    dev0->GetMac()->SetExtendedAddress("00:00:00:00:00:00:CA:FE");
    // dev0->GetMac()->SetShortAddress("00:00");

    dev1->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:01");
    dev2->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:02");

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

    //// Configure NWK

    ZigbeeHelper zigbee;
    ZigbeeStackContainer zigbeeStackContainer = zigbee.Install(lrwpanDevices);

    Ptr<ZigbeeStack> zstack0 = zigbeeStackContainer.Get(0)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack1 = zigbeeStackContainer.Get(1)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack2 = zigbeeStackContainer.Get(2)->GetObject<ZigbeeStack>();

    //// Configure Nodes Mobility

    Ptr<ConstantPositionMobilityModel> sender0Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(sender0Mobility);

    Ptr<ConstantPositionMobilityModel> sender1Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender1Mobility->SetPosition(Vector(0, 10, 0));
    dev1->GetPhy()->SetMobility(sender1Mobility);

    Ptr<ConstantPositionMobilityModel> sender2Mobility =
        CreateObject<ConstantPositionMobilityModel>();
    sender2Mobility->SetPosition(Vector(0, 20, 0));
    dev2->GetPhy()->SetMobility(sender2Mobility);

    // NWK callbacks hooks

    zstack0->GetNwk()->SetNlmeNetworkFormationConfirmCallback(
        MakeBoundCallback(&NwkNetworkFormationConfirm, zstack0));

    zstack0->GetNwk()->SetNlmeDirectJoinConfirmCallback(
        MakeBoundCallback(&NwkDirectJoinConfirm, zstack0));

    zstack0->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack0));

    zstack1->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack1));

    zstack2->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack2));

    zstack1->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack1));

    zstack2->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack2));

    // 1 - Initiate the Zigbee coordinator, start the network
    NlmeNetworkFormationRequestParams netFormParams;
    netFormParams.m_scanChannelList.channelPageCount = 1;
    netFormParams.m_scanChannelList.channelsField[0] = ALL_CHANNELS;
    netFormParams.m_scanDuration = 0;
    netFormParams.m_superFrameOrder = 15;
    netFormParams.m_beaconOrder = 15;

    Simulator::ScheduleWithContext(0,
                                   Seconds(0),
                                   &ZigbeeNwk::NlmeNetworkFormationRequest,
                                   zstack0->GetNwk(),
                                   netFormParams);

    // Configure the capability information used in the joining devices.
    CapabilityInformation capaInfo;
    capaInfo.SetDeviceType(zigbee::MacDeviceType::ROUTER);
    capaInfo.SetAllocateAddrOn(true);

    // 2- Register dev 1 (Mac64Addr .....00:01) to Zigbee coordinator (dev 0) directly
    NlmeDirectJoinRequestParams directParams;
    directParams.m_capabilityInfo = capaInfo.GetCapability();
    directParams.m_deviceAddr = Mac64Address("00:00:00:00:00:00:00:01");

    Simulator::ScheduleWithContext(0,
                                   Seconds(5),
                                   &ZigbeeNwk::NlmeDirectJoinRequest,
                                   zstack0->GetNwk(),
                                   directParams);

    // 3- Use join request(type= DIRECT_OR_REJOIN) to initiate an orphaning procedure and request
    //    the information registered in the coordinator in the previous step.
    // Notes :
    // - ScanDuration is fixed for DIRECT_OR_REJOIN type (macResponseWaitTime)
    //   and therefore, the scanDuration parameter is ignored.
    // - Future communications can fail if extendendPanId is set incorrectly,
    //   this value is the value of the PAN coordinator extended address (IEEEAddress).
    //   This value cannot be verified during a DIRECT_OR_REJOIN join type.
    NlmeJoinRequestParams joinParams;
    joinParams.m_rejoinNetwork = zigbee::JoiningMethod::DIRECT_OR_REJOIN;
    joinParams.m_scanChannelList.channelPageCount = 1;
    joinParams.m_scanChannelList.channelsField[0] = ALL_CHANNELS;
    joinParams.m_capabilityInfo = capaInfo.GetCapability();
    joinParams.m_extendedPanId = Mac64Address("00:00:00:00:00:00:CA:FE").ConvertToInt();

    Simulator::ScheduleWithContext(1,
                                   MilliSeconds(5500),
                                   &ZigbeeNwk::NlmeJoinRequest,
                                   zstack1->GetNwk(),
                                   joinParams);

    // 4 - Use start-router on device 1, to initiate it as router
    //     (i.e. it becomes able to accept request from other devices to join the network)
    NlmeStartRouterRequestParams startRouterParams;
    Simulator::ScheduleWithContext(1,
                                   MilliSeconds(5600),
                                   &ZigbeeNwk::NlmeStartRouterRequest,
                                   zstack1->GetNwk(),
                                   startRouterParams);

    NlmeDirectJoinRequestParams directParams2;
    directParams2.m_capabilityInfo = capaInfo.GetCapability();
    directParams2.m_deviceAddr = Mac64Address("00:00:00:00:00:00:00:02");

    Simulator::ScheduleWithContext(1,
                                   MilliSeconds(6000),
                                   &ZigbeeNwk::NlmeDirectJoinRequest,
                                   zstack1->GetNwk(),
                                   directParams2);

    NlmeJoinRequestParams joinParams2;
    joinParams2.m_rejoinNetwork = zigbee::JoiningMethod::DIRECT_OR_REJOIN;
    joinParams2.m_scanChannelList.channelPageCount = 1;
    joinParams2.m_scanChannelList.channelsField[0] = ALL_CHANNELS;
    joinParams2.m_capabilityInfo = capaInfo.GetCapability();
    joinParams2.m_extendedPanId = Mac64Address("00:00:00:00:00:00:CA:FE").ConvertToInt();

    Simulator::ScheduleWithContext(2,
                                   MilliSeconds(6100),
                                   &ZigbeeNwk::NlmeJoinRequest,
                                   zstack2->GetNwk(),
                                   joinParams2);

    Simulator::Stop(Seconds(10));
    Simulator::Run();

    Simulator::Destroy();
    return 0;
}
