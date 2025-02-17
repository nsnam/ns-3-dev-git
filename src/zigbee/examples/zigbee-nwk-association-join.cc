/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/**
 * This example shows the procedure to form a multi-hop network using an association-based join.
 * The procedure requires a sequence of primitive calls on a specific order in the indicated
 * devices.
 *
 *
 *  Network Extended PAN id: 0X000000000000CA:FE (based on the PAN coordinator address)
 *
 *  1-Network-Formation(dev0)                2-Network Discovery(dev1)      4-Network
 * Discovery(dev2) 3-Router Start(dev1)
 *
 *  Zigbee Coordinator(ZC)                      Router(ZR1)                         Router(ZR2)
 *  (dev 0) ------------------------------------ (dev 1) ---------------------------- (dev 2)
 *  [00:00:00:00:00:00:CA:FE]          [00:00:00:00:00:00:00:01]         [00:00:00:00:00:00:00:02]
 *  [00:00]                            [short addr assigned by ZC]       [short addr assigned by
 * ZR1]
 *
 *  1- Channel scanning, pan id selection, start network (initiation of zigbee coordinator)
 *  2-
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

NS_LOG_COMPONENT_DEFINE("ZigbeeAssociationJoin");

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
NwkNetworkDiscoveryConfirm(Ptr<ZigbeeStack> stack, NlmeNetworkDiscoveryConfirmParams params)
{
    // See Zigbee Specification r22.1.0, 3.6.1.4.1
    // This method implements a simplistic version of the method implemented
    // in a zigbee APL layer. In this layer a candidate Extended PAN Id must
    // be selected and a NLME-JOIN.request must be issued.

    if (params.m_status == NwkStatus::SUCCESS)
    {
        std::cout << " Network discovery confirm Received. Networks found:\n";

        for (const auto& netDescriptor : params.m_netDescList)
        {
            std::cout << " ExtPanID: 0x" << std::hex << netDescriptor.m_extPanId << std::dec
                      << " CH:  " << static_cast<uint32_t>(netDescriptor.m_logCh) << std::hex
                      << " Pan Id: 0x" << netDescriptor.m_panId << " stackprofile " << std::dec
                      << static_cast<uint32_t>(netDescriptor.m_stackProfile) << "\n";
        }

        NlmeJoinRequestParams joinParams;

        zigbee::CapabilityInformation capaInfo;
        capaInfo.SetDeviceType(ROUTER);
        capaInfo.SetAllocateAddrOn(true);

        joinParams.m_rejoinNetwork = zigbee::JoiningMethod::ASSOCIATION;
        joinParams.m_capabilityInfo = capaInfo.GetCapability();
        joinParams.m_extendedPanId = params.m_netDescList[0].m_extPanId;

        Simulator::ScheduleNow(&ZigbeeNwk::NlmeJoinRequest, stack->GetNwk(), joinParams);
    }
    else
    {
        NS_ABORT_MSG("Unable to discover networks | status: " << params.m_status);
    }
}

static void
NwkJoinConfirm(Ptr<ZigbeeStack> stack, NlmeJoinConfirmParams params)
{
    if (params.m_status == NwkStatus::SUCCESS)
    {
        std::cout << Simulator::Now().As(Time::S)
                  << " The device joined the network SUCCESSFULLY with short address " << std::hex
                  << params.m_networkAddress << " on the Extended PAN Id: " << std::hex
                  << params.m_extendedPanId << "\n"
                  << std::dec;

        // 3 - After dev 1 is associated, it should be started as a router
        //     (i.e. it becomes able to accept request from other devices to join the network)
        NlmeStartRouterRequestParams startRouterParams;
        Simulator::ScheduleNow(&ZigbeeNwk::NlmeStartRouterRequest,
                               stack->GetNwk(),
                               startRouterParams);
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
    // LogComponentEnable("LrWpanMac", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_DEBUG);

    NodeContainer nodes;
    nodes.Create(3);

    //// Configure MAC

    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
    Ptr<LrWpanNetDevice> dev0 = lrwpanDevices.Get(0)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = lrwpanDevices.Get(1)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = lrwpanDevices.Get(2)->GetObject<LrWpanNetDevice>();

    dev0->GetMac()->SetExtendedAddress("00:00:00:00:00:00:CA:FE");
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

    Ptr<ConstantPositionMobilityModel> dev0Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(dev0Mobility);

    Ptr<ConstantPositionMobilityModel> dev1Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev1Mobility->SetPosition(Vector(0, 10, 0));
    dev1->GetPhy()->SetMobility(dev1Mobility);

    Ptr<ConstantPositionMobilityModel> dev2Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev2Mobility->SetPosition(Vector(0, 20, 0)); // try with distance of 95
    dev2->GetPhy()->SetMobility(dev2Mobility);

    // NWK callbacks hooks

    zstack0->GetNwk()->SetNlmeNetworkFormationConfirmCallback(
        MakeBoundCallback(&NwkNetworkFormationConfirm, zstack0));

    zstack0->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack0));

    zstack1->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack1));

    zstack2->GetNwk()->SetNldeDataIndicationCallback(
        MakeBoundCallback(&NwkDataIndication, zstack2));

    zstack1->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack1));

    zstack2->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack2));

    zstack2->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack2));

    zstack1->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack1));

    zstack2->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack2));

    // 1 - Initiate the Zigbee coordinator, start the network
    NlmeNetworkFormationRequestParams netFormParams;
    netFormParams.m_scanChannelList.channelPageCount = 1;
    netFormParams.m_scanChannelList.channelsField[0] = ALL_CHANNELS;
    netFormParams.m_scanDuration = 0;
    netFormParams.m_superFrameOrder = 15;
    netFormParams.m_beaconOrder = 15;

    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(1),
                                   &ZigbeeNwk::NlmeNetworkFormationRequest,
                                   zstack0->GetNwk(),
                                   netFormParams);

    // 2- Let the dev1 (Router) discovery the coordinator and join the network, after
    //    this, it will become router itself(call to NLME-START-ROUTER.request).
    NlmeNetworkDiscoveryRequestParams netDiscParams;
    netDiscParams.m_scanChannelList.channelPageCount = 1;
    netDiscParams.m_scanChannelList.channelsField[0] = 0x7800;
    netDiscParams.m_scanDuration = 14;
    Simulator::ScheduleWithContext(zstack1->GetNode()->GetId(),
                                   Seconds(3),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack1->GetNwk(),
                                   netDiscParams);

    // 4- (Note: Look for step 3 in NetworkDiscoveryRequestConfirm)
    // Let the dev2 discover the PAN coordinator or a router and join the network,
    // after this, it will become a router itself

    NlmeNetworkDiscoveryRequestParams netDiscParams2;
    netDiscParams2.m_scanChannelList.channelPageCount = 1;
    netDiscParams2.m_scanChannelList.channelsField[0] = 0x7800;
    netDiscParams2.m_scanDuration = 14;
    Simulator::ScheduleWithContext(zstack2->GetNode()->GetId(),
                                   Seconds(1020),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack2->GetNwk(),
                                   netDiscParams2);

    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
