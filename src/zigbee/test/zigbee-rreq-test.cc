/*
 * Copyright (c) 2024 Tokushima University, Tokushima, Japan
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

NS_LOG_COMPONENT_DEFINE("zigbee-rreq-test");

/**
 * @ingroup zigbee-test
 * @ingroup tests
 *
 * Zigbee RREQ transmission retries test case
 */
class ZigbeeRreqRetryTestCase : public TestCase
{
  public:
    ZigbeeRreqRetryTestCase();
    ~ZigbeeRreqRetryTestCase() override;

  private:
    /**
     * Called in response to a NLME-NETWORK-DISCOVERY.request
     *
     * @param testcase The pointer to the testcase
     * @param stack The zigbee stack of the device originating the callback
     * @param params The NLME-NETWORK-DISCOVERY.confirm parameters
     */
    static void NwkNetworkDiscoveryConfirm(ZigbeeRreqRetryTestCase* testcase,
                                           Ptr<ZigbeeStack> stack,
                                           NlmeNetworkDiscoveryConfirmParams params);

    /**
     * Called in response to a NLME-JOIN.request
     *
     * @param testcase The pointer to the testcase
     * @param stack The zigbee stack of the device originating the callback
     * @param params The NLME-JOIN.confirm parameters
     */
    static void NwkJoinConfirm(ZigbeeRreqRetryTestCase* testcase,
                               Ptr<ZigbeeStack> stack,
                               NlmeJoinConfirmParams params);

    /**
     * Called when a maximum RREQ retries are reached
     *
     * @param testcase The pointer to the testcase
     * @param stack The zigbee stack of the device originating the callback
     * @param rreqid The RREQ ID
     * @param dst The destination Mac16Address of the RREQ
     * @param rreqRetriesNum Maximum number of retries reached
     */
    static void RreqRetriesExhausted(ZigbeeRreqRetryTestCase* testcase,
                                     Ptr<ZigbeeStack> stack,
                                     uint8_t rreqid,
                                     Mac16Address dst,
                                     uint8_t rreqRetriesNum);
    void DoRun() override;

    std::vector<std::pair<uint32_t, uint32_t>>
        m_rreqRetriesExhaustedList; //!< A list containing
                                    //!< the RREQ ID and the allowed max retries every time the
                                    //!< maximum RREQ retries are reached
};

ZigbeeRreqRetryTestCase::ZigbeeRreqRetryTestCase()
    : TestCase("Zigbee: RREQ retries test")
{
}

ZigbeeRreqRetryTestCase::~ZigbeeRreqRetryTestCase()
{
}

void
ZigbeeRreqRetryTestCase::NwkNetworkDiscoveryConfirm(ZigbeeRreqRetryTestCase* testcase,
                                                    Ptr<ZigbeeStack> stack,
                                                    NlmeNetworkDiscoveryConfirmParams params)
{
    // See Zigbee Specification r22.1.0, 3.6.1.4.1
    // This method implements a simplistic version of the method implemented
    // in a zigbee APL layer. In this layer a candidate Extended PAN Id must
    // be selected and a NLME-JOIN.request must be issued.

    if (params.m_status == NwkStatus::SUCCESS)
    {
        std::cout << " Network discovery confirm Received. Networks found ("
                  << params.m_netDescList.size() << "):\n";

        for (const auto& netDescriptor : params.m_netDescList)
        {
            std::cout << " ExtPanID: 0x" << std::hex << netDescriptor.m_extPanId << "\n"
                      << std::dec << " CH:  " << static_cast<uint32_t>(netDescriptor.m_logCh)
                      << "\n"
                      << std::hex << " Pan ID: 0x" << netDescriptor.m_panId << "\n"
                      << " Stack profile: " << std::dec
                      << static_cast<uint32_t>(netDescriptor.m_stackProfile) << "\n"
                      << "--------------------\n";
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

void
ZigbeeRreqRetryTestCase::NwkJoinConfirm(ZigbeeRreqRetryTestCase* testcase,
                                        Ptr<ZigbeeStack> stack,
                                        NlmeJoinConfirmParams params)
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

void
ZigbeeRreqRetryTestCase::RreqRetriesExhausted(ZigbeeRreqRetryTestCase* testcase,
                                              Ptr<ZigbeeStack> stack,
                                              uint8_t rreqid,
                                              Mac16Address dst,
                                              uint8_t rreqRetriesNum)
{
    NS_LOG_DEBUG("RREQ retries (" << static_cast<uint32_t>(rreqRetriesNum)
                                  << ") exhausted for destination [" << dst << "] and RREQ ID "
                                  << static_cast<uint32_t>(rreqid));

    testcase->m_rreqRetriesExhaustedList.emplace_back(
        std::pair(stack->GetNode()->GetId(), rreqRetriesNum));
}

void
ZigbeeRreqRetryTestCase::DoRun()
{
    // RREQ retries exhaustion test
    // This test verifies that the necessary number of Route Requests are issued during
    // zigbee's route discovery process in which a Route Reply (RREP) is not received.
    // In this test, a coordinator and 2 routers are set in a line.
    // The coordinator issues a route discovery primitive (NLME-ROUTE-DISCOVERY.request)
    // to a non-existent destination (0d:10). Route request are retried a number of times
    // depending on the origin of the request (initial request or relayed request)
    //   Topology:
    //
    //     ========= RREQ(x3) =====>    ======== RREQ(x2) ===>     ==== RREQ(x2) ====>
    //  ZC--------------------------ZR1-------------------------ZR2
    // (Node 0)                   (Node 1)                    (Node 2)
    //
    // The number of retries depend on the NWK layer constants:
    // nwkcInitialRREQRetries (Default: 3)
    // nwkcRREQRetries (Default: 2)

    LogComponentEnableAll(LogLevel(LOG_PREFIX_TIME | LOG_PREFIX_FUNC | LOG_PREFIX_NODE));
    LogComponentEnable("zigbee-rreq-test", LOG_LEVEL_DEBUG);

    RngSeedManager::SetSeed(3);
    RngSeedManager::SetRun(4);

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
    dev1Mobility->SetPosition(Vector(90, 0, 0)); // 110 ,0,0
    dev1->GetPhy()->SetMobility(dev1Mobility);

    Ptr<ConstantPositionMobilityModel> dev2Mobility = CreateObject<ConstantPositionMobilityModel>();
    dev2Mobility->SetPosition(Vector(170, 0, 0)); // 190,0,0
    dev2->GetPhy()->SetMobility(dev2Mobility);

    // NWK callbacks hooks, these hooks are usually directly connected to the APS layer
    // where all these calls inform the result of a request originated the APS layer.
    // In this test these are used to complete the Join process and router initialization
    zstack1->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&ZigbeeRreqRetryTestCase::NwkNetworkDiscoveryConfirm, this, zstack1));
    zstack2->GetNwk()->SetNlmeNetworkDiscoveryConfirmCallback(
        MakeBoundCallback(&ZigbeeRreqRetryTestCase::NwkNetworkDiscoveryConfirm, this, zstack2));

    zstack1->GetNwk()->SetNlmeJoinConfirmCallback(
        MakeBoundCallback(&ZigbeeRreqRetryTestCase::NwkJoinConfirm, this, zstack1));
    zstack2->GetNwk()->SetNlmeJoinConfirmCallback(
        MakeBoundCallback(&ZigbeeRreqRetryTestCase::NwkJoinConfirm, this, zstack2));

    // Connect to traces in the NWK layer
    zstack0->GetNwk()->TraceConnectWithoutContext(
        "rreqRetriesExhausted",
        MakeBoundCallback(&ZigbeeRreqRetryTestCase::RreqRetriesExhausted, this, zstack0));

    zstack1->GetNwk()->TraceConnectWithoutContext(
        "rreqRetriesExhausted",
        MakeBoundCallback(&ZigbeeRreqRetryTestCase::RreqRetriesExhausted, this, zstack1));

    zstack2->GetNwk()->TraceConnectWithoutContext(
        "rreqRetriesExhausted",
        MakeBoundCallback(&ZigbeeRreqRetryTestCase::RreqRetriesExhausted, this, zstack2));

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
    //    this, it will become router itself(call to NLME-START-ROUTER.request). We
    //    continue doing the same with the rest of the devices which will discover the
    //    previously added routers and join the network
    NlmeNetworkDiscoveryRequestParams netDiscParams;
    netDiscParams.m_scanChannelList.channelPageCount = 1;
    netDiscParams.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
    netDiscParams.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack1->GetNode()->GetId(),
                                   Seconds(3),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack1->GetNwk(),
                                   netDiscParams);

    NlmeNetworkDiscoveryRequestParams netDiscParams2;
    netDiscParams2.m_scanChannelList.channelPageCount = 1;
    netDiscParams2.m_scanChannelList.channelsField[0] = 0x00007800; // 0x00000800;
    netDiscParams2.m_scanDuration = 2;
    Simulator::ScheduleWithContext(zstack2->GetNode()->GetId(),
                                   Seconds(4),
                                   &ZigbeeNwk::NlmeNetworkDiscoveryRequest,
                                   zstack2->GetNwk(),
                                   netDiscParams2);

    // 5- Find a route to the given device short address
    NlmeRouteDiscoveryRequestParams routeDiscParams;
    routeDiscParams.m_dstAddr = Mac16Address("0d:10");
    Simulator::ScheduleWithContext(zstack0->GetNode()->GetId(),
                                   Seconds(8),
                                   &ZigbeeNwk::NlmeRouteDiscoveryRequest,
                                   zstack0->GetNwk(),
                                   routeDiscParams);
    Simulator::Run();

    for (auto rreqRetriesExhaustedElement : m_rreqRetriesExhaustedList)
    {
        if (rreqRetriesExhaustedElement.first == 0)
        {
            // For Node with id 0 (Coordinator/RREQ originator) the number of retries is
            // nwkcInitialRREQRetries (Default: 3)
            NS_TEST_EXPECT_MSG_EQ(rreqRetriesExhaustedElement.second,
                                  3,
                                  "The number of RREQ retries from an originator should be 3");
        }
        else
        {
            // For other node other that the RREQ originator the number of retries is
            // nwkcRREQRetries (Default: 2)
            NS_TEST_EXPECT_MSG_EQ(rreqRetriesExhaustedElement.second,
                                  2,
                                  "The number of RREQ retries from a relaying node should be 2");
        }
    }

    Simulator::Destroy();
}

/**
 * @ingroup zigbee-test
 * @ingroup tests
 *
 * Zigbee RREQ TestSuite
 */
class ZigbeeRreqTestSuite : public TestSuite
{
  public:
    ZigbeeRreqTestSuite();
};

ZigbeeRreqTestSuite::ZigbeeRreqTestSuite()
    : TestSuite("zigbee-rreq-test", Type::UNIT)
{
    AddTestCase(new ZigbeeRreqRetryTestCase, TestCase::Duration::QUICK);
}

static ZigbeeRreqTestSuite zigbeeRreqTestSuite; //!< Static variable for test initialization
