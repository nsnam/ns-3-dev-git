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
 *  Many to one routing example in a grid topology.
 *
 *  Topology:
 *
 *  Grid Topology: 50 nodes separated by 30 m around them, 20 nodes per row for
 *  the first two rows
 *
 *      (Node 0)
 *         |
 *         v
 *         * * * * * * * * * * * * * * * * * * * *
 *         * * * * * * * * * * * * * * * * * * * *
 *         * * * * * * * * * *  <--- (Node 49)
 *
 *  This example is a more complex version of zigbee-nwk-routing.cc.
 *  The top left node is the coordinator while the rest of the nodes join
 *  the network sequentially and they initiate as routers.
 *
 *  After all devices join the network a MANY-TO-ONE route discovery is issued
 *  to create the routes from all the nodes towards the concentrator (Node 0).
 *
 *  At the end of the example, the contents of all the tables (Neighbor, Discovery, routing)
 *  in the source node (Node 49) are displayed.
 *
 *  Also the trace route from Node 49 to Node 0 is displayed.
 *  No data is transmitted in this example.
 *  All devices are routers excepting the first node which is the coordinator.
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

static void
TraceRoute(Mac16Address src, Mac16Address dst)
{
    std::cout << "\n";
    std::cout << "Traceroute to destination [" << dst << "] (Time: " << Simulator::Now().As(Time::S)
              << "):\n";
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
}

static void
CreateManyToOneRoutes(Ptr<ZigbeeStack> zigbeeStackConcentrator, Ptr<ZigbeeStack> zigbeeStackSrc)
{
    // Generate all the routes to the concentrator device
    NlmeRouteDiscoveryRequestParams routeDiscParams;
    routeDiscParams.m_dstAddrMode = NO_ADDRESS;
    Simulator::ScheduleNow(&ZigbeeNwk::NlmeRouteDiscoveryRequest,
                           zigbeeStackConcentrator->GetNwk(),
                           routeDiscParams);

    // Give a few seconds to allow the creation of the route and
    // then print the route trace and tables from the source
    Simulator::Schedule(Seconds(3),
                        &TraceRoute,
                        zigbeeStackSrc->GetNwk()->GetNetworkAddress(),
                        zigbeeStackConcentrator->GetNwk()->GetNetworkAddress());

    // Print the content of the source device tables (Neighbor, Discovery, Routing)
    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
    Simulator::Schedule(Seconds(4),
                        &ZigbeeNwk::PrintNeighborTable,
                        zigbeeStackSrc->GetNwk(),
                        stream);

    Simulator::Schedule(Seconds(4),
                        &ZigbeeNwk::PrintRoutingTable,
                        zigbeeStackSrc->GetNwk(),
                        stream);

    Simulator::Schedule(Seconds(4),
                        &ZigbeeNwk::PrintRouteDiscoveryTable,
                        zigbeeStackSrc->GetNwk(),
                        stream);
}

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
    // Enable logs for further details
    // LogComponentEnable("ZigbeeNwk", LOG_LEVEL_DEBUG);

    NodeContainer nodes;
    nodes.Create(50);

    MobilityHelper mobility;
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
    // Device must ALWAYS have IEEE Address (Extended address) assigned.
    // Network address (short address) are assigned by the the JOIN mechanism
    // In this case we use the helper to assign a sequential extended addresses
    // to all nodes in the simulation.
    lrWpanHelper.SetExtendedAddresses(lrwpanDevices);

    ZigbeeHelper zigbeeHelper;
    zigbeeStacks = zigbeeHelper.Install(lrwpanDevices);

    // NWK callbacks hooks
    // These hooks are usually directly connected to the APS layer
    // In this case, there is no APS layer, therefore, we connect the event outputs
    // of all devices directly to our static functions in this example.
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
            // ALL_CHANNELS = 0x07FFF800 (Channels 11~26)
            NlmeNetworkFormationRequestParams netFormParams;
            netFormParams.m_scanChannelList.channelPageCount = 1;
            netFormParams.m_scanChannelList.channelsField[0] = ALL_CHANNELS;
            netFormParams.m_scanDuration = 0;
            netFormParams.m_superFrameOrder = 15;
            netFormParams.m_beaconOrder = 15;

            Simulator::ScheduleWithContext(zstack->GetNode()->GetId(),
                                           MilliSeconds(index * 500),
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
            netDiscParams.m_scanChannelList.channelsField[0] = 0x00007800; // BitMap: Channels 11~14
            netDiscParams.m_scanDuration = 0;

            Simulator::ScheduleWithContext(zstack->GetNode()->GetId(),
                                           Seconds(2 + index * 10),
                                           &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                           zstack->GetNwk(),
                                           netDiscParams);
        }
    }

    // 3- Create the routes towards the concentrator (Node 0)
    // Print the trace route from Node 49 to the concentrator (Node 0)
    // Print the contents of tables in the source of trace (Node 49)
    Simulator::Schedule(Seconds(500),
                        &CreateManyToOneRoutes,
                        zigbeeStacks.Get(0),
                        zigbeeStacks.Get(49));

    Simulator::Stop(Seconds(1500));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
