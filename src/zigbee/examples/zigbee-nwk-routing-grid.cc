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
 *
 *  Topology:
 *
 *  Grid Topology: 50 nodes separated by 30 m around them, 20 nodes per row for
 *  the first two rows
 *
 *         * * * * * * * * * * * * * * * * * * * *
 *         * * * * * * * * * * * * * * * * * * * *
 *         * * * * * * * * * *
 *
 *  This example is a more complex version of zigbee-nwk-routing.cc.
 *  The top left node is the coordinator while the rest of the nodes join
 *  the network sequentially and they initiate as routers.
 *
 *  After all devices join the network there are 3 alternatives for operations:
 *
 *  1. Send a data request (data transmission) with route discovery
 *  2. Send a route discovery
 *  3. Send a many-to-one route discovery
 *
 */

#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/mobility-module.h"
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

ZigbeeStackContainer zigbeeStacks;

/*static void
TraceRoute(Mac16Address src, Mac16Address dst)
{
    std::cout << "\n";
    std::cout << "Traceroute to destination [" << dst
              << "] (Time: " << Simulator::Now().As(Time::S) <<"):\n";
    Mac16Address target = src;
    uint32_t count = 1;
    while (target != Mac16Address("FF:FF") && target != dst)
    {
        Ptr<ZigbeeStack> zstack;

        for (auto i = zigbeeStacks.Begin(); i != zigbeeStacks.End(); i++)
        {
            zstack = *i;
            if (zstack->GetNwk()->GetNetworkAddress() == target)
            {
                break;
            }
        }

        bool neighbor = false;
        target = zstack->GetNwk()->FindRoute(dst, neighbor);
        if (target == Mac16Address("FF:FF"))
        {
            std::cout << count << ". Node " << zstack->GetNode()->GetId() << " ["
                      << zstack->GetNwk()->GetNetworkAddress() << " | "
                      << zstack->GetNwk()->GetIeeeAddress() << "]: "
                      << " Destination Unreachable\n";
        }
        else
        {
            std::cout << count << ". Node " << zstack->GetNode()->GetId() << " ["
                      << zstack->GetNwk()->GetNetworkAddress() << " | "
                      << zstack->GetNwk()->GetIeeeAddress() << "]: "
                      << "NextHop [" << target << "] ";
            if (neighbor)
            {
                std::cout << "(*Neighbor)\n";
            }
            else
            {
                std::cout << "\n";
            }
            count++;
        }
    }
    std::cout << "\n";
}*/

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
        std::cout << "    Network discovery confirm Received. Networks found "
                  << "(" << params.m_netDescList.size() << ")\n";

        for (const auto& netDescriptor : params.m_netDescList)
        {
            std::cout << "      ExtPanID: 0x" << std::hex << netDescriptor.m_extPanId << std::dec
                      << "\n"
                      << "      CH:  " << static_cast<uint32_t>(netDescriptor.m_logCh) << std::hex
                      << "\n"
                      << "      Pan Id: 0x" << netDescriptor.m_panId << std::hex << "\n"
                      << "      stackprofile: " << std::dec
                      << static_cast<uint32_t>(netDescriptor.m_stackProfile) << "\n"
                      << "      ----------------\n ";
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
        std::cout << " WARNING: Unable to discover networks | status: " << params.m_status << "\n";
    }
}

static void
NwkJoinConfirm(Ptr<ZigbeeStack> stack, NlmeJoinConfirmParams params)
{
    if (params.m_status == NwkStatus::SUCCESS)
    {
        std::cout << Simulator::Now().As(Time::S)
                  << " The device joined the network SUCCESSFULLY with short address [" << std::hex
                  << params.m_networkAddress << "] on the Extended PAN Id: " << std::hex
                  << params.m_extendedPanId << "\n"
                  << std::dec;

        // 3 - After dev  is associated, it should be started as a router
        //     (i.e. it becomes able to accept request from other devices to join the network)
        NlmeStartRouterRequestParams startRouterParams;
        Simulator::ScheduleNow(&ZigbeeNwk::NlmeStartRouterRequest,
                               stack->GetNwk(),
                               startRouterParams);
    }
    else
    {
        std::cout << Simulator::Now().As(Time::S)
                  << " The device FAILED to join the network with status " << params.m_status
                  << "\n";
    }
}

static void
NwkRouteDiscoveryConfirm(Ptr<ZigbeeStack> stack, NlmeRouteDiscoveryConfirmParams params)
{
    std::cout << "NlmeRouteDiscoveryConfirmStatus = " << params.m_status << "\n";
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    // LogComponentEnable("ZigbeeNwk", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanCsmaCa", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanMac", LOG_LEVEL_DEBUG);
    // LogComponentEnable("LrWpanPhy", LOG_LEVEL_DEBUG);

    NodeContainer nodes;
    nodes.Create(50);

    MobilityHelper mobility;
    // mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
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

    Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel>();
    Ptr<LogDistancePropagationLossModel> propModel =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<ConstantSpeedPropagationDelayModel> delayModel =
        CreateObject<ConstantSpeedPropagationDelayModel>();

    channel->AddPropagationLossModel(propModel);
    channel->SetPropagationDelayModel(delayModel);

    LrWpanHelper lrWpanHelper;
    lrWpanHelper.SetChannel(channel);

    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);

    // Set the extended address to all devices (EUI-64)
    lrWpanHelper.SetExtendedAddresses(lrwpanDevices);

    ZigbeeHelper zigbeeHelper;
    zigbeeStacks = zigbeeHelper.Install(lrwpanDevices);

    for (auto i = zigbeeStacks.Begin(); i != zigbeeStacks.End(); i++)
    {
        Ptr<ZigbeeStack> zstack = *i;
        // NLME-NETWORK-FORMATION.Confirm
        zstack->GetNwk()->SetNlmeNetworkFormationConfirmCallback(
            MakeBoundCallback(&NwkNetworkFormationConfirm, zstack));
        // NLDE-DATA.Indication
        zstack->GetNwk()->SetNldeDataIndicationCallback(
            MakeBoundCallback(&NwkDataIndication, zstack));
        // NLDE-NETWORK-DISCOVERY.Confirm
        zstack->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
            MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack));
        // NLME-JOIN.Confirm
        zstack->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack));
        // NLME-ROUTE-DISCOVERY.Confirm
        zstack->GetNwk()->SetNlmeRouteDiscoveryConfirmCallback(
            MakeBoundCallback(&NwkRouteDiscoveryConfirm, zstack));
    }

    for (auto i = zigbeeStacks.Begin(); i != zigbeeStacks.End(); i++)
    {
        int index = std::distance(zigbeeStacks.Begin(), i);
        Ptr<ZigbeeStack> zstack = *i;

        // Assign streams to the zigbee stacks based on the index to obtain
        // reprodusable results from random events occurring inside the stack.
        // For example, to obtain the same assigned short address in each device.
        zstack->GetNwk()->AssignStreams(index);

        if (index == 0)
        {
            // 1 - Initiate the Zigbee coordinator, start the network
            NlmeNetworkFormationRequestParams netFormParams;
            netFormParams.m_scanChannelList.channelPageCount = 1;
            netFormParams.m_scanChannelList.channelsField[0] = ALL_CHANNELS;
            netFormParams.m_scanDuration = 0;
            netFormParams.m_superFrameOrder = 15;
            netFormParams.m_beaconOrder = 15;

            Simulator::ScheduleWithContext(zstack->GetNode()->GetId(),
                                           Seconds(index * 0.5),
                                           &ZigbeeNwk::NlmeNetworkFormationRequest,
                                           zstack->GetNwk(),
                                           netFormParams);
        }
        else
        {
            // 2- Let devices discovery the coordinator or routers and join the network, after
            //    this, it will become router itself(call to NLME-START-ROUTER.request). We
            //    continue doing the same with the rest of the devices which will discover the
            //    previously added routers and join the network
            NlmeNetworkDiscoveryRequestParams netDiscParams;
            netDiscParams.m_scanChannelList.channelPageCount = 1;
            netDiscParams.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
            netDiscParams.m_scanDuration = 0;

            Simulator::ScheduleWithContext(zstack->GetNode()->GetId(),
                                           Seconds(2 + index * 10), // 2+index * 0.5
                                           &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                           zstack->GetNwk(),
                                           netDiscParams);
        }
    }

    // 5- Alternative 1: Data request with route discovery
    Ptr<Packet> p = Create<Packet>(5);
    NldeDataRequestParams dataReqParams;
    dataReqParams.m_dstAddrMode = UCST_BCST;
    dataReqParams.m_dstAddr = Mac16Address("30:56");
    dataReqParams.m_nsduHandle = 25;
    dataReqParams.m_discoverRoute = ENABLE_ROUTE_DISCOVERY;
    Simulator::ScheduleWithContext(zigbeeStacks.Get(0)->GetNode()->GetId(),
                                   Seconds(500),
                                   &ZigbeeNwk::NldeDataRequest,
                                   zigbeeStacks.Get(0)->GetNwk(),
                                   dataReqParams,
                                   p);

    // 5- Alternative 2: Route discovery without data transmission
    /* NlmeRouteDiscoveryRequestParams routeDiscParams;
     routeDiscParams.m_dstAddrMode = UCST_BCST;
     routeDiscParams.m_dstAddr = Mac16Address("30:56");
     Simulator::ScheduleWithContext(zigbeeStacks.Get(0)->GetNode()->GetId(),
                                    Seconds(500),
                                    &ZigbeeNwk::NlmeRouteDiscoveryRequest,
                                    zigbeeStacks.Get(0)->GetNwk(),
                                    routeDiscParams);

     // Results can be check with the Trace route.
     // Note: Make sure the route is formed before using traceroute
     Simulator::Schedule(Seconds(501),
                         &TraceRoute,
                         Mac16Address("00:00"),
                         Mac16Address("30:56"));*/

    /*
    // 5- Alternative 3: Trigger a Many-To-One route discovery with the first node
    // as the concentrator
    NlmeRouteDiscoveryRequestParams routeDiscParams;
    routeDiscParams.m_dstAddrMode = NO_ADDRESS;
    Simulator::ScheduleWithContext(zigbeeStacks.Get(0)->GetNode()->GetId(),
                                   Seconds(500),
                                   &ZigbeeNwk::NlmeRouteDiscoveryRequest,
                                   zigbeeStacks.Get(0)->GetNwk(),
                                   routeDiscParams);

    Simulator::Schedule(Seconds(501),
                        &TraceRoute,
                        Mac16Address("b6:24"),
                        Mac16Address("00:00"));*/

    // Printing Tables:

    /*Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
    Simulator::ScheduleWithContext(zigbeeStacks.Get(7)->GetNode()->GetId(),
                                   Seconds(502),
                                   &ZigbeeNwk::PrintRoutingTable,
                                   zigbeeStacks.Get(7)->GetNwk(),
                                   stream);

    Simulator::ScheduleWithContext(zigbeeStacks.Get(7)->GetNode()->GetId(),
                                   Seconds(502),
                                   &ZigbeeNwk::PrintRouteDiscoveryTable,
                                   zigbeeStacks.Get(7)->GetNwk(),
                                   stream);

    Simulator::ScheduleWithContext(zigbeeStacks.Get(7)->GetNode()->GetId(),
                                   Seconds(502),
                                   &ZigbeeNwk::PrintNeighborTable,
                                   zigbeeStacks.Get(7)->GetNwk(),
                                   stream);*/

    Simulator::Stop(Seconds(1500));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
