/*
 * Copyright (c) 2025 Tokushima University, Tokushima, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "ns3/constant-position-mobility-model.h"
#include "ns3/core-module.h"
#include "ns3/log.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/packet.h"
#include "ns3/propagation-delay-model.h"
#include "ns3/propagation-loss-model.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/single-model-spectrum-channel.h"
#include "ns3/zigbee-module.h"

#include <iomanip>
#include <iostream>

using namespace ns3;
using namespace ns3::lrwpan;
using namespace ns3::zigbee;

NS_LOG_COMPONENT_DEFINE("zigbee-aps-data-test");

/**
 * @ingroup zigbee-test
 * @ingroup tests
 *
 * Zigbee RREQ transmission retries test case
 */
class ZigbeeApsDataTestCase : public TestCase
{
  public:
    ZigbeeApsDataTestCase();
    ~ZigbeeApsDataTestCase() override;

  private:
    /**
     * Callback for APSDE-DATA.indication
     * This callback is called when a data packet is received by the APS layer.
     *
     * @param testcase The ZigbeeApsDataTestCase instance
     * @param stack The Zigbee stack that received the data
     * @param params The parameters of the APSDE-DATA.indication
     * @param asdu The received packet
     */
    static void ApsDataIndication(ZigbeeApsDataTestCase* testcase,
                                  Ptr<ZigbeeStack> stack,
                                  ApsdeDataIndicationParams params,
                                  Ptr<Packet> asdu);

    /**
     * Callback for NLME-NETWORK-DISCOVERY.confirm
     * This callback is called when a network discovery has been performed.
     *
     * @param testcase The ZigbeeApsDataTestCase instance
     * @param stack The Zigbee stack that received the confirmation
     * @param params The parameters of the NLME-NETWORK-DISCOVERY.confirm
     */
    static void NwkNetworkDiscoveryConfirm(ZigbeeApsDataTestCase* testcase,
                                           Ptr<ZigbeeStack> stack,
                                           NlmeNetworkDiscoveryConfirmParams params);

    /**
     * Send data to a unicast destination.
     * This function sends a data packet from stackSrc to stackDst.
     *
     * @param stackSrc The source Zigbee stack
     * @param stackDst The destination Zigbee stack
     */
    static void SendDataUcstDst(Ptr<ZigbeeStack> stackSrc, Ptr<ZigbeeStack> stackDst);

    void DoRun() override;

    uint16_t m_dstEndpoint; //!< The destination endpoint
};

ZigbeeApsDataTestCase::ZigbeeApsDataTestCase()
    : TestCase("Zigbee: APS layer data test")
{
    m_dstEndpoint = 0;
}

ZigbeeApsDataTestCase::~ZigbeeApsDataTestCase()
{
}

void
ZigbeeApsDataTestCase::ApsDataIndication(ZigbeeApsDataTestCase* testcase,
                                         Ptr<ZigbeeStack> stack,
                                         ApsdeDataIndicationParams params,
                                         Ptr<Packet> asdu)
{
    testcase->m_dstEndpoint = params.m_dstEndPoint;
}

void
ZigbeeApsDataTestCase::NwkNetworkDiscoveryConfirm(ZigbeeApsDataTestCase* testcase,
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
ZigbeeApsDataTestCase::SendDataUcstDst(Ptr<ZigbeeStack> stackSrc, Ptr<ZigbeeStack> stackDst)
{
    // UCST transmission to a single 16-bit address destination
    // Data is transmitted from device stackSrc to device stackDst.

    Ptr<Packet> p = Create<Packet>(5);

    // Because we currently do not have ZDO or ZCL or AF, clusterId
    // and profileId numbers are non-sensical.
    ApsdeDataRequestParams dataReqParams;
    // creates a BitMap with transmission options
    // Default, use 16 bit address destination (No option), equivalent to bitmap 0x00
    ZigbeeApsTxOptions txOptions;
    dataReqParams.m_txOptions = txOptions.GetTxOptions();
    dataReqParams.m_useAlias = false;
    dataReqParams.m_srcEndPoint = 3;
    dataReqParams.m_clusterId = 5; // Arbitrary value
    dataReqParams.m_profileId = 2; // Arbitrary value
    dataReqParams.m_dstAddrMode = ApsDstAddressMode::DST_ADDR16_DST_ENDPOINT_PRESENT;
    dataReqParams.m_dstAddr16 = stackDst->GetNwk()->GetNetworkAddress();
    dataReqParams.m_dstEndPoint = 4;

    Simulator::ScheduleNow(&ZigbeeAps::ApsdeDataRequest, stackSrc->GetAps(), dataReqParams, p);
}

void
ZigbeeApsDataTestCase::DoRun()
{
    // Transmit data using the APS layer.

    // Zigbee Coordinator --------------> Zigbee EndDevice(destination endpoint:4)

    // This test transmit a single packet to an enddevice with endpoint 4.
    // The data transmission is done using the mode DST_ADDR16_DST_ENDPOINT_PRESENT (Mode 0x02).
    // No fragmentations or Acknowledge is used
    // Verification that the devices have join the network is performed
    // (i.e., All devices have valid network addresses).

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

    // Add Zigbee stack with  NWK and APS

    ZigbeeHelper zigbeeHelper;
    ZigbeeStackContainer zigbeeStackContainer = zigbeeHelper.Install(lrwpanDevices);

    Ptr<ZigbeeStack> zstack0 = zigbeeStackContainer.Get(0)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack1 = zigbeeStackContainer.Get(1)->GetObject<ZigbeeStack>();
    Ptr<ZigbeeStack> zstack2 = zigbeeStackContainer.Get(2)->GetObject<ZigbeeStack>();

    // reprodusable results from random events occurring inside the stack.
    zstack0->GetNwk()->AssignStreams(0);
    zstack1->GetNwk()->AssignStreams(10);
    zstack2->GetNwk()->AssignStreams(20);

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

    // Configure APS hooks
    zstack1->GetAps()->SetApsdeDataIndicationCallback(
        MakeBoundCallback(&ApsDataIndication, this, zstack1));

    zstack2->GetAps()->SetApsdeDataIndicationCallback(
        MakeBoundCallback(&ApsDataIndication, this, zstack2));

    // Configure NWK hooks
    // We do not have ZDO, we are required to use the NWK
    // directly to perform association.
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

    // Send data to a single UCST destination (16-bit address)
    // The destination address is unknown until compilation, we extract
    // it from the stack directly.
    Simulator::Schedule(Seconds(4), &SendDataUcstDst, zstack0, zstack2);

    Simulator::Run();

    // Check that devices actually joined the network and have different 16-bit addresses.

    NS_TEST_EXPECT_MSG_NE(zstack1->GetNwk()->GetNetworkAddress(),
                          Mac16Address("FF:FF"),
                          "The dev 1 was unable to join the network");

    NS_TEST_EXPECT_MSG_NE(zstack2->GetNwk()->GetNetworkAddress(),
                          Mac16Address("FF:FF"),
                          "The dev 1 was unable to join the network");

    NS_TEST_EXPECT_MSG_NE(zstack0->GetNwk()->GetNetworkAddress(),
                          zstack1->GetNwk()->GetNetworkAddress(),
                          "Error, devices 0 and 1 have the same 16 bit MAC address");

    NS_TEST_EXPECT_MSG_NE(zstack1->GetNwk()->GetNetworkAddress(),
                          zstack2->GetNwk()->GetNetworkAddress(),
                          "Error, devices 1 and 2 have the same 16 bit MAC address");

    // Check that the packet was received to the correct preconfigured destination endpoint.

    NS_TEST_EXPECT_MSG_EQ(m_dstEndpoint,
                          4,
                          "Packet was not received in the correct destination endpoint");

    zstack1->GetAps()->SetApsdeDataIndicationCallback(
        MakeNullCallback<void, ApsdeDataIndicationParams, Ptr<Packet>>());
    zstack2->GetAps()->SetApsdeDataIndicationCallback(
        MakeNullCallback<void, ApsdeDataIndicationParams, Ptr<Packet>>());
    Simulator::Destroy();
}

/**
 * @ingroup zigbee-test
 * @ingroup tests
 *
 * Zigbee APS Data TestSuite
 */
class ZigbeeApsDataTestSuite : public TestSuite
{
  public:
    ZigbeeApsDataTestSuite();
};

ZigbeeApsDataTestSuite::ZigbeeApsDataTestSuite()
    : TestSuite("zigbee-aps-data-test", Type::UNIT)
{
    AddTestCase(new ZigbeeApsDataTestCase, TestCase::Duration::QUICK);
}

static ZigbeeApsDataTestSuite zigbeeApsDataTestSuite; //!< Static variable for test initialization
