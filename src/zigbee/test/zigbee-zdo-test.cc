/*
 * Copyright (c) 2026 Tokushima University, Tokushima, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "ns3/constant-position-mobility-model.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/test.h"
#include "ns3/zigbee-helper.h"
#include "ns3/zigbee-stack-container.h"
#include "ns3/zigbee-stack.h"

#include <iomanip>
#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;
using namespace ns3::zigbee;

NS_LOG_COMPONENT_DEFINE("zigbee-zdo-test");

/**
 * @ingroup zigbee-test
 * @ingroup tests
 *
 * Zigbee ZDO Commands test
 */
class ZigbeeZdoCommandsTestCase : public TestCase
{
  public:
    ZigbeeZdoCommandsTestCase();
    ~ZigbeeZdoCommandsTestCase() override;

  private:
    /**
     * Callback for ZDO NWK_addr_rsp indication
     * This callback is called when a NWK_addr_rsp is received.
     *
     * @param testcase The ZigbeeZdoCommandsTestCase instance
     * @param stack The Zigbee stack that received the indication
     * @param params The parameters of the NWK_addr_rsp indication
     */
    static void ZdoNwkAddrRspInd(ZigbeeZdoCommandsTestCase* testcase,
                                 Ptr<ZigbeeStack> stack,
                                 NwkAddrRespIndParams params);

    /**
     * Callback for NLME-NETWORK-DISCOVERY.confirm
     * This callback is called when a network discovery has been performed.
     *
     * @param testcase The ZigbeeApsDataTestCase instance
     * @param stack The Zigbee stack that received the confirmation
     * @param params The parameters of the NLME-NETWORK-DISCOVERY.confirm
     */
    static void NwkNetworkDiscoveryConfirm(ZigbeeZdoCommandsTestCase* testcase,
                                           Ptr<ZigbeeStack> stack,
                                           NlmeNetworkDiscoveryConfirmParams params);

    void DoRun() override;

    Mac16Address m_requestedNwkAddr; //!< The 16-bit address requested by the ZDO command.
};

ZigbeeZdoCommandsTestCase::ZigbeeZdoCommandsTestCase()
    : TestCase("Zigbee: ZDO commands test")
{
}

ZigbeeZdoCommandsTestCase::~ZigbeeZdoCommandsTestCase()
{
}

void
ZigbeeZdoCommandsTestCase::ZdoNwkAddrRspInd(ZigbeeZdoCommandsTestCase* testcase,
                                            Ptr<ZigbeeStack> stack,
                                            NwkAddrRespIndParams params)
{
    if (params.m_status == ZdpStatus::SUCCESS)
    {
        testcase->m_requestedNwkAddr = params.m_nwkAddrRemoteDev;

        std::cout << "NWK_addr_rsp received | status: SUCCESS "
                  << " | IEEE address: " << params.m_ieeeAddrRemoteDev
                  << " | NWK address: " << params.m_nwkAddrRemoteDev << std::endl;
    }
    else
    {
        std::cout << "NWK_addr_rsp not received or received with error status\n";
    }
}

void
ZigbeeZdoCommandsTestCase::NwkNetworkDiscoveryConfirm(ZigbeeZdoCommandsTestCase* testcase,
                                                      Ptr<ZigbeeStack> stack,
                                                      NlmeNetworkDiscoveryConfirmParams params)
{
    if (params.m_status == NwkStatus::SUCCESS)
    {
        NlmeJoinRequestParams joinParams;

        zigbee::CapabilityInformation capaInfo;
        capaInfo.SetDeviceType(MacDeviceType::ENDDEVICE);
        capaInfo.SetAllocateAddrOn(true);

        joinParams.m_rejoinNetwork = JoiningMethod::ASSOCIATION;
        joinParams.m_capabilityInfo = capaInfo.GetCapability();
        joinParams.m_extendedPanId = params.m_netDescList[0].m_extPanId;

        Simulator::ScheduleNow(&ZigbeeNwk::NlmeJoinRequest, stack->GetNwk(), joinParams);
    }
    else
    {
        NS_ABORT_MSG("Unable to discover networks | status: " << params.m_status);
    }
}

void
ZigbeeZdoCommandsTestCase::DoRun()
{
    // Transmit commands and receiving a response using Zigbee's ZDO
    // The topogy includes 3 devices with the following formation:

    // Zigbee Coordinator ------- Router -------> Zigbee EndDevice

    // In this test a ZDO NWK_Addr_req command is sent to the network to
    // inquire the 16 bit address of the device with the 64-bit address
    // [00:00:00:00:00:00:00:02] which correspond to the End Device.
    // Upon receiving  this request, the EndDevice responds to the coordinator.
    // The coordinator updates its nwkAddressMap internally and reports the
    // results of the request.

    RngSeedManager::SetSeed(3);
    RngSeedManager::SetRun(4);

    NodeContainer nodes;
    nodes.Create(3);

    //// Add the PHY and MAC, configure the channel

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

    // Add Zigbee stack with  NWK | APS | ZDO
    ZigbeeHelper zigbeeHelper;
    ZigbeeStackContainer zigbeeStackContainer = zigbeeHelper.Install(lrwpanDevices);

    Ptr<ZigbeeStack> zstack0 = zigbeeStackContainer.Get(0)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack1 = zigbeeStackContainer.Get(1)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack2 = zigbeeStackContainer.Get(2)->GetObject<ZigbeeStack>();

    // reprodusable results from random events occurring inside the stack (selected discretely)
    zstack0->GetNwk()->AssignStreams(1);
    zstack1->GetNwk()->AssignStreams(20);
    zstack2->GetNwk()->AssignStreams(25);

    //// Configure Nodes Mobility

    Ptr<ConstantPositionMobilityModel> dev0Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev0Mobility->SetPosition(Vector(0, 0, 0));
    dev0->GetPhy()->SetMobility(dev0Mobility);

    Ptr<ConstantPositionMobilityModel> dev1Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev1Mobility->SetPosition(Vector(50, 0, 0));
    dev1->GetPhy()->SetMobility(dev1Mobility);

    Ptr<ConstantPositionMobilityModel> dev2Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev2Mobility->SetPosition(Vector(0, 50, 0));
    dev2->GetPhy()->SetMobility(dev2Mobility);

    // Configure ZDO hooks to obtain responses from ZDO commands.
    zstack0->GetZdo()->SetZdoNwkAddrRspIndCallback(
        MakeBoundCallback(&ZdoNwkAddrRspInd, this, zstack0));

    // Configure NWK hooks
    // We have ZDO, but we do not have a Commission Manager
    // (entity to simplify and handle of Network formation and Joins)
    // Therefore, we use the NWK directly to perform association.
    zstack1->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, this, zstack1));
    zstack2->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&NwkNetworkDiscoveryConfirm, this, zstack2));

    // Configure NWK hooks (for managing Network Joining)

    // 1 - Initiate the Zigbee coordinator on a channel
    NlmeNetworkFormationRequestParams netFormParams;
    netFormParams.m_scanChannelList.channelPageCount = 1;
    netFormParams.m_scanChannelList.channelsField[0] = 0x00001800; // BitMap: channel 11 and 12
    netFormParams.m_scanDuration = 0;
    netFormParams.m_superFrameOrder = 15;
    netFormParams.m_beaconOrder = 15;

    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(1),
                                   &ZigbeeNwk::NlmeNetworkFormationRequest,
                                   zstack0->GetNwk(),
                                   netFormParams);

    NlmeNetworkDiscoveryRequestParams netDiscParams;
    netDiscParams.m_scanChannelList.channelPageCount = 1;
    netDiscParams.m_scanChannelList.channelsField[0] = 0x00000800; // BitMap: Channels 11
    netDiscParams.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack1->GetNode()->GetId(),
                                   Seconds(2),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack1->GetNwk(),
                                   netDiscParams);

    NlmeNetworkDiscoveryRequestParams netDiscParams2;
    netDiscParams.m_scanChannelList.channelPageCount = 1;
    netDiscParams.m_scanChannelList.channelsField[0] = 0x00000800; // BitMap: Channels 11~14
    netDiscParams.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack2->GetNode()->GetId(),
                                   Seconds(3),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack2->GetNwk(),
                                   netDiscParams2);

    // Dev 0 (Coordinator) sends a NWK_Addr_req to dev2 (last end device) to inquire its
    // nwkAddress (16-bit address) based on its known IEEE Addr.
    NwkAddrRequestParams zdoReqParams;
    zdoReqParams.m_dstAddr = Mac16Address("FF:FF");
    zdoReqParams.m_ieeeAddr = Mac64Address("00:00:00:00:00:00:00:02");
    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(4),
                                   &ZigbeeZdo::NwkAddrRequest,
                                   zstack0->GetZdo(),
                                   zdoReqParams);

    // Dev 0 (Coordinator) sends a NWK_Addr_req to all devices to
    // inquire the nwkAddress (16-bit address) of the device with IEEE Addr 00:00:00:00:00:00:00:03.
    NwkAddrRequestParams zdoNwkAddrReqParams;
    zdoNwkAddrReqParams.m_requestType = NwkAddrRequestType::SINGLE_DEVICE_RESPONSE;
    zdoNwkAddrReqParams.m_ieeeAddr = Mac64Address("00:00:00:00:00:00:00:02");
    zdoNwkAddrReqParams.m_dstAddr = Mac16Address("FF:FF");
    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(5),
                                   &ZigbeeZdo::NwkAddrRequest,
                                   zstack0->GetZdo(),
                                   zdoNwkAddrReqParams);

    Simulator::Run();

    // Verify the dev2 64-bit address is correctly assigned.
    NS_TEST_EXPECT_MSG_EQ(zstack2->GetNwk()->GetIeeeAddress().ConvertToInt(),
                          Mac64Address("00:00:00:00:00:00:00:02").ConvertToInt(),
                          "Dev2 (stack2) is not using the correct 64-bit IEEE address");

    // Check that the coordinator received the correct results after a NWK_Addr_req
    NS_TEST_EXPECT_MSG_EQ(zstack2->GetNwk()->GetNetworkAddress().ConvertToInt(),
                          m_requestedNwkAddr.ConvertToInt(),
                          "Error: The obtained address via NWK_Addr_req do not correspond to the "
                          "address used by the device");

    // Check that the coordinator updated its nwkAddressMap after receiving the NWK_Addr_rsp from
    // dev2
    Mac16Address registeredAddress;
    zstack0->GetNwk()->GetNwkAddrByIeeeAddr(Mac64Address("00:00:00:00:00:00:00:02"),
                                            registeredAddress);

    NS_TEST_EXPECT_MSG_EQ(
        zstack2->GetNwk()->GetNetworkAddress().ConvertToInt(),
        registeredAddress.ConvertToInt(),
        "Error: The nwkAddressMap do not have a matching entry even after a NWK_Addr_req CMD");

    Simulator::Destroy();
}

/**
 * @ingroup zigbee-test
 * @ingroup tests
 *
 * Zigbee ZDO TestSuite
 */
class ZigbeeZdoTestSuite : public TestSuite
{
  public:
    ZigbeeZdoTestSuite();
};

ZigbeeZdoTestSuite::ZigbeeZdoTestSuite()
    : TestSuite("zigbee-zdo-test", Type::UNIT)
{
    AddTestCase(new ZigbeeZdoCommandsTestCase, TestCase::Duration::QUICK);
}

static ZigbeeZdoTestSuite zigbeeZdoTestSuite; //!< Static variable for test initialization
