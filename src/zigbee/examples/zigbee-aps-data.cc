/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

/**
 * Mesh routing example with APS layer data transmission in a simple topology.
 *
 * This example use a Zigbee stack formed by NWK and APS layers.
 * The APS layer is used to transmit data, the creation of the necessary routes is
 * handle automatically by the NWK layer. There is no ZDO, therefore we need to manually
 * use the NWK to form the network and establish the router devices.
 *
 * The example demonstrates the following:
 * - Sending data using the APS layer (Unicast and Groupcast destinations).
 * - Using the APS layer to establish groupcast groups and endpoints.
 *
 *
 *  Network Extended PAN id: 0X000000000000CA:FE (based on the PAN coordinator address)
 *
 *  Devices Addresses:
 *
 *  [Coordinator] ZC  (dev0 | Node 0): [00:00:00:00:00:00:CA:FE]  [00:00]
 *  [Router 1]    ZR1 (dev1 | Node 1): [00:00:00:00:00:00:00:01]  [short addr assigned by ZC]
 *  [Router 2]    ZR2 (dev2 | Node 2): [00:00:00:00:00:00:00:02]  [short addr assigned by ZR1]
 *  [Router 3]    ZR3 (dev3 | Node 3): [00:00:00:00:00:00:00:03]  [short addr assigned by ZR2]
 *  [Router 4]    ZR4 (dev4 | Node 4): [00:00:00:00:00:00:00:04]  [short addr assigned by ZR1]
 *
 *  Topology:
 *
 *  ZC--------ZR1------------ZR2----------ZR3 (GroupID Member: [01:23] | Endpoints: 3)
 *             |
 *             |
 *            ZR4 (GroupID Member: [01:23] | Endpoints: 3, 9)
 *
 *
 *  In example, Both ZR4 and ZR3 are part of the multicast group [01:23].
 *  ZR4 has two endpoints in the group, 3 and 9. ZR3 has only one endpoint in the group, 3.
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

NS_LOG_COMPONENT_DEFINE("ZigbeeRouting");

ZigbeeStackContainer zigbeeStacks;

static void
ApsDataIndication(Ptr<ZigbeeStack> stack, ApsdeDataIndicationParams params, Ptr<Packet> p)
{
    std::cout << Simulator::Now().As(Time::S) << " Node " << stack->GetNode()->GetId() << " | ";
    if (params.m_dstAddrMode == ApsDstAddressMode::DST_ADDR16_DST_ENDPOINT_PRESENT)
    {
        std::cout << "UCST | ";
    }
    else if (params.m_dstAddrMode == ApsDstAddressMode::GROUP_ADDR_DST_ENDPOINT_NOT_PRESENT)
    {
        std::cout << "GROUPCAST | ";
    }
    else
    {
        std::cout << "Unknown | ";
    }
    std::cout << "ApsdeDataIndication: Received DATA packet with size " << p->GetSize()
              << " for destination EndPoint " << params.m_dstEndPoint << "\n";
}

static void
NwkNetworkFormationConfirm(Ptr<ZigbeeStack> stack, NlmeNetworkFormationConfirmParams params)
{
    std::cout << Simulator::Now().As(Time::S) << " Node " << stack->GetNode()->GetId() << " | "
              << "NlmeNetworkFormationConfirmStatus = " << params.m_status << "\n";
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
        std::cout << Simulator::Now().As(Time::S) << " Node " << stack->GetNode()->GetId() << " | "
                  << " Network discovery confirm Received. Networks found ("
                  << params.m_netDescList.size() << ")\n";

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
        std::cout << Simulator::Now().As(Time::S) << " Node " << stack->GetNode()->GetId() << " | "
                  << " The device joined the network SUCCESSFULLY with short address " << std::hex
                  << params.m_networkAddress << " on the Extended PAN Id: 0x" << std::hex
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

static void
NwkRouteDiscoveryConfirm(Ptr<ZigbeeStack> stack, NlmeRouteDiscoveryConfirmParams params)
{
    std::cout << "NlmeRouteDiscoveryConfirmStatus = " << params.m_status << "\n";
}

static void
SendDataUcst(Ptr<ZigbeeStack> stackSrc, Ptr<ZigbeeStack> stackDst)
{
    // Send data from a device with stackSrc to device with stackDst.

    // We do not know what network address will be assigned after the JOIN procedure
    // but we can request the network address from stackDst (the destination device) when
    // we intend to send data. If a route do not exist, we will search for a route
    // before transmitting data (Mesh routing).

    Ptr<Packet> p = Create<Packet>(5);
    // Src and Dst Endpoints must not be 0, because this is reserved for the ZDO.
    // Other Endpoint numbers can help to differentiate between different applications
    // running in the same node (similar to the concept of a port in TCP/IP).
    // Likewise, because we currently do not have ZDO or ZCL or AF, clusterId
    // and profileId numbers are non-sensical.
    ApsdeDataRequestParams dataReqParams;
    ZigbeeApsTxOptions txOptions;
    dataReqParams.m_useAlias = false;
    // Default, use 16 bit address destination (No option), equivalent to 0x00
    dataReqParams.m_txOptions = txOptions.GetTxOptions();
    dataReqParams.m_srcEndPoint = 4; // Arbitrary value, must not be 0
    dataReqParams.m_clusterId = 5;   // Arbitrary value
    dataReqParams.m_profileId = 2;   // Arbitrary value

    dataReqParams.m_dstAddrMode = ApsDstAddressMode::DST_ADDR16_DST_ENDPOINT_PRESENT;
    dataReqParams.m_dstAddr16 = stackDst->GetNwk()->GetNetworkAddress();
    dataReqParams.m_dstEndPoint = 3;

    Simulator::ScheduleNow(&ZigbeeAps::ApsdeDataRequest, stackSrc->GetAps(), dataReqParams, p);
}

static void
SendDataGcst(Ptr<ZigbeeStack> stackSrc)
{
    Ptr<Packet> p = Create<Packet>(5);
    // Src and Dst Endpoints must not be 0, because this is reserved for the ZDO.
    // Other Endpoint numbers can help to differentiate between different applications
    // running in the same node (similar to the concept of a port in TCP/IP).
    // Likewise, because we currently do not have ZDO or ZCL or AF, clusterId
    // and profileId numbers are non-sensical.
    ApsdeDataRequestParams dataReqParams;
    ZigbeeApsTxOptions txOptions;
    dataReqParams.m_useAlias = false;
    // Default, use 16 bit address destination (No option), equivalent to 0x00
    dataReqParams.m_txOptions = txOptions.GetTxOptions();
    dataReqParams.m_srcEndPoint = 4; // Arbitrary value, must not be 0
    dataReqParams.m_clusterId = 5;   // Arbitrary value
    dataReqParams.m_profileId = 2;   // Arbitrary value

    dataReqParams.m_dstAddrMode = ApsDstAddressMode::GROUP_ADDR_DST_ENDPOINT_NOT_PRESENT;
    dataReqParams.m_dstAddr16 = Mac16Address("01:23"); // The destination group address

    Simulator::ScheduleNow(&ZigbeeAps::ApsdeDataRequest, stackSrc->GetAps(), dataReqParams, p);
}

int
main(int argc, char* argv[])
{
    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    // Enable logs for further details
    // LogComponentEnable("ZigbeeNwk", LOG_LEVEL_DEBUG);
    // LogComponentEnable("ZigbeeAps", LOG_LEVEL_DEBUG);

    RngSeedManager::SetSeed(3);
    RngSeedManager::SetRun(4);

    NodeContainer nodes;
    nodes.Create(5);

    //// Configure MAC

    LrWpanHelper lrWpanHelper;
    NetDeviceContainer lrwpanDevices = lrWpanHelper.Install(nodes);
    Ptr<LrWpanNetDevice> dev0 = lrwpanDevices.Get(0)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev1 = lrwpanDevices.Get(1)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev2 = lrwpanDevices.Get(2)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev3 = lrwpanDevices.Get(3)->GetObject<LrWpanNetDevice>();
    Ptr<LrWpanNetDevice> dev4 = lrwpanDevices.Get(4)->GetObject<LrWpanNetDevice>();

    // Device must ALWAYS have IEEE Address (Extended address) assigned.
    // Network address (short address) are assigned by the the JOIN mechanism
    dev0->GetMac()->SetExtendedAddress("00:00:00:00:00:00:CA:FE");
    dev1->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:01");
    dev2->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:02");
    dev3->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:03");
    dev4->GetMac()->SetExtendedAddress("00:00:00:00:00:00:00:04");

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
    dev3->SetChannel(channel);
    dev4->SetChannel(channel);

    // Configure the Zigbee stack, by default both the NWK and the APS are present

    ZigbeeHelper zigbeeHelper;
    ZigbeeStackContainer zigbeeStackContainer = zigbeeHelper.Install(lrwpanDevices);

    Ptr<ZigbeeStack> zstack0 = zigbeeStackContainer.Get(0)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack1 = zigbeeStackContainer.Get(1)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack2 = zigbeeStackContainer.Get(2)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack3 = zigbeeStackContainer.Get(3)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack4 = zigbeeStackContainer.Get(4)->GetObject<ZigbeeStack>();

    // Add the stacks to a container to later on print routes.
    zigbeeStacks.Add(zstack0);
    zigbeeStacks.Add(zstack1);
    zigbeeStacks.Add(zstack2);
    zigbeeStacks.Add(zstack3);
    zigbeeStacks.Add(zstack4);

    // Assign streams to the zigbee stacks to obtain
    // reprodusable results from random events occurring inside the stack.
    zstack0->GetNwk()->AssignStreams(0);
    zstack1->GetNwk()->AssignStreams(10);
    zstack2->GetNwk()->AssignStreams(20);
    zstack3->GetNwk()->AssignStreams(30);
    zstack4->GetNwk()->AssignStreams(40);

    //// Configure Nodes Mobility

    Ptr<ConstantPositionMobilityModel> dev0Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(dev0Mobility);

    Ptr<ConstantPositionMobilityModel> dev1Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev1Mobility->SetPosition(Vector(90, 0, 0));
    dev1->GetPhy()->SetMobility(dev1Mobility);

    Ptr<ConstantPositionMobilityModel> dev2Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev2Mobility->SetPosition(Vector(170, 0, 0));
    dev2->GetPhy()->SetMobility(dev2Mobility);

    Ptr<ConstantPositionMobilityModel> dev3Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev3Mobility->SetPosition(Vector(250, 0, 0));
    dev3->GetPhy()->SetMobility(dev3Mobility);

    Ptr<ConstantPositionMobilityModel> dev4Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev4Mobility->SetPosition(Vector(90, 50, 0));
    dev4->GetPhy()->SetMobility(dev4Mobility);

    // NWK callbacks hooks
    // These hooks are usually directly connected to the ZDO.
    // In this case, there is no ZDO, therefore, we connect the event outputs
    // of all devices directly to our static functions in this example.

    zstack0->GetNwk()->SetNlmeNetworkFormationConfirmCallback(
        MakeBoundCallback(&NwkNetworkFormationConfirm, zstack0));
    zstack0->GetNwk()->SetNlmeRouteDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkRouteDiscoveryConfirm, zstack0));

    zstack1->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack1));
    zstack2->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack2));
    zstack3->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack3));
    zstack4->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, zstack4));

    zstack1->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack1));
    zstack2->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack2));
    zstack3->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack3));
    zstack4->GetNwk()->SetNlmeJoinConfirmCallback(MakeBoundCallback(&NwkJoinConfirm, zstack4));

    // APS callback hooks

    zstack0->GetAps()->SetApsdeDataIndicationCallback(
        MakeBoundCallback(&ApsDataIndication, zstack0));
    zstack1->GetAps()->SetApsdeDataIndicationCallback(
        MakeBoundCallback(&ApsDataIndication, zstack1));
    zstack2->GetAps()->SetApsdeDataIndicationCallback(
        MakeBoundCallback(&ApsDataIndication, zstack2));
    zstack3->GetAps()->SetApsdeDataIndicationCallback(
        MakeBoundCallback(&ApsDataIndication, zstack3));
    zstack4->GetAps()->SetApsdeDataIndicationCallback(
        MakeBoundCallback(&ApsDataIndication, zstack4));

    // NWK Layer
    // We do not have a ZDO, therefore we need to manually
    // initiate the network formation and discovery procedures
    // using the NWK layer.

    // 1 - Initiate the Zigbee coordinator, start the network
    // ALL_CHANNELS = 0x07FFF800 (Channels 11~26)
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

    // 2- Schedule devices sequentially find and join the network.
    //    After this procedure, each device make a NLME-START-ROUTER.request to become a router

    NlmeNetworkDiscoveryRequestParams netDiscParams;
    netDiscParams.m_scanChannelList.channelPageCount = 1;
    netDiscParams.m_scanChannelList.channelsField[0] = 0x00007800; // BitMap: Channels 11~14
    netDiscParams.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack1->GetNode()->GetId(),
                                   Seconds(3),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack1->GetNwk(),
                                   netDiscParams);

    NlmeNetworkDiscoveryRequestParams netDiscParams2;
    netDiscParams2.m_scanChannelList.channelPageCount = 1;
    netDiscParams2.m_scanChannelList.channelsField[0] = 0x00007800; // BitMap: Channels 11~14
    netDiscParams2.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack2->GetNode()->GetId(),
                                   Seconds(4),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack2->GetNwk(),
                                   netDiscParams2);

    NlmeNetworkDiscoveryRequestParams netDiscParams3;
    netDiscParams2.m_scanChannelList.channelPageCount = 1;
    netDiscParams2.m_scanChannelList.channelsField[0] = 0x00007800; // BitMap: Channels 11~14
    netDiscParams2.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack3->GetNode()->GetId(),
                                   Seconds(5),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack3->GetNwk(),
                                   netDiscParams3);

    NlmeNetworkDiscoveryRequestParams netDiscParams4;
    netDiscParams4.m_scanChannelList.channelPageCount = 1;
    netDiscParams4.m_scanChannelList.channelsField[0] = 0x00007800; // BitMap: Channels 11~14
    netDiscParams4.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack4->GetNode()->GetId(),
                                   Seconds(6),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack4->GetNwk(),
                                   netDiscParams4);

    // APS Layer
    // 4- Establish the Groupcast groups and its endpoints
    // Add group [01:23] and endpoint 3 to Devices 3 and 4
    // Also add endpoint 9 to device 4 in the same group.
    ApsmeGroupRequestParams groupParams;
    groupParams.m_groupAddress = Mac16Address("01:23");
    groupParams.m_endPoint = 3;
    Simulator::ScheduleWithContext(zstack3->GetNode()->GetId(),
                                   Seconds(7),
                                   &ZigbeeAps::ApsmeAddGroupRequest,
                                   zstack3->GetAps(),
                                   groupParams);

    Simulator::ScheduleWithContext(zstack4->GetNode()->GetId(),
                                   Seconds(7),
                                   &ZigbeeAps::ApsmeAddGroupRequest,
                                   zstack4->GetAps(),
                                   groupParams);

    groupParams.m_endPoint = 9; // Add endpoint 9 to the same group
    Simulator::ScheduleWithContext(zstack4->GetNode()->GetId(),
                                   Seconds(7),
                                   &ZigbeeAps::ApsmeAddGroupRequest,
                                   zstack4->GetAps(),
                                   groupParams);

    // 4- Send data using the APS layer

    // GROUPCAST
    // Transmit data to all endpoints in devices that are
    // part of the group [01:23] (i.e. endpoints in the ZR3 and ZR4).
    Simulator::Schedule(Seconds(9), &SendDataGcst, zstack0);

    // UNICAST
    // Transmit data to a specific endpoint in a device
    // In this case, we send data to the endpoint 3 of ZR3.
    // We require the destination address, but we do not know this apriori,
    // therefore we can request it from zstack3 on runtime.
    Simulator::Schedule(Seconds(10), &SendDataUcst, zstack0, zstack3);

    // Print the contents of the routing table in the initiator device (ZC)
    Ptr<OutputStreamWrapper> stream = Create<OutputStreamWrapper>(&std::cout);
    Simulator::Schedule(Seconds(11), &ZigbeeNwk::PrintRoutingTable, zstack0->GetNwk(), stream);

    Simulator::Stop(Seconds(20));
    Simulator::Run();
    Simulator::Destroy();
    return 0;
}
