/*
 *  Copyright (c) 2009 INRIA, UDcast
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *         Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 *
 */
#include "ns3/cs-parameters.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipcs-classifier-record.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/service-flow.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/wimax-helper.h"

using namespace ns3;

/**
 * @ingroup wimax-test
 * @ingroup tests
 *
 * @brief Test the service flow creation.
 */
class Ns3WimaxSfCreationTestCase : public TestCase
{
  public:
    Ns3WimaxSfCreationTestCase();
    ~Ns3WimaxSfCreationTestCase() override;

  private:
    void DoRun() override;
};

Ns3WimaxSfCreationTestCase::Ns3WimaxSfCreationTestCase()
    : TestCase("Test the service flow tlv implementation.")
{
}

Ns3WimaxSfCreationTestCase::~Ns3WimaxSfCreationTestCase()
{
}

void
Ns3WimaxSfCreationTestCase::DoRun()
{
    // default values
    int duration = 2;
    WimaxHelper::SchedulerType scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;

    NodeContainer ssNodes;
    NodeContainer bsNodes;

    ssNodes.Create(1);
    bsNodes.Create(1);

    WimaxHelper wimax;

    NetDeviceContainer ssDevs;
    NetDeviceContainer bsDevs;

    ssDevs = wimax.Install(ssNodes,
                           WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION,
                           WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                           scheduler);
    bsDevs = wimax.Install(bsNodes,
                           WimaxHelper::DEVICE_TYPE_BASE_STATION,
                           WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                           scheduler);

    ssDevs.Get(0)->GetObject<SubscriberStationNetDevice>()->SetModulationType(
        WimaxPhy::MODULATION_TYPE_QAM16_12);
    bsDevs.Get(0)->GetObject<BaseStationNetDevice>();

    InternetStackHelper stack;
    stack.Install(bsNodes);
    stack.Install(ssNodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer SSinterfaces = address.Assign(ssDevs);
    Ipv4InterfaceContainer BSinterface = address.Assign(bsDevs);

    // Create one UGS Downlink service flow between the ss and the bs
    auto DlServiceFlowUgs = new ServiceFlow(ServiceFlow::SF_DIRECTION_DOWN);
    IpcsClassifierRecord DlClassifierUgs(Ipv4Address("0.0.0.0"),
                                         Ipv4Mask("0.0.0.0"),
                                         Ipv4Address("0.0.0.0"),
                                         Ipv4Mask("0.0.0.0"),
                                         3000,
                                         3000,
                                         0,
                                         35000,
                                         17,
                                         1);
    CsParameters DlcsParam(CsParameters::ADD, DlClassifierUgs);
    DlServiceFlowUgs->SetConvergenceSublayerParam(DlcsParam);
    DlServiceFlowUgs->SetCsSpecification(ServiceFlow::IPV4);
    DlServiceFlowUgs->SetServiceSchedulingType(ServiceFlow::SF_TYPE_UGS);
    DlServiceFlowUgs->SetMaxSustainedTrafficRate(1000000);
    DlServiceFlowUgs->SetMinReservedTrafficRate(1000000);
    DlServiceFlowUgs->SetMinTolerableTrafficRate(1000000);
    DlServiceFlowUgs->SetMaximumLatency(10);
    DlServiceFlowUgs->SetMaxTrafficBurst(1000);
    DlServiceFlowUgs->SetTrafficPriority(1);

    // Create one UGS Uplink service flow between the ss and the bs
    auto UlServiceFlowUgs = new ServiceFlow(ServiceFlow::SF_DIRECTION_UP);
    IpcsClassifierRecord UlClassifierUgs(Ipv4Address("0.0.0.0"),
                                         Ipv4Mask("0.0.0.0"),
                                         Ipv4Address("0.0.0.0"),
                                         Ipv4Mask("0.0.0.0"),
                                         0,
                                         35000,
                                         3000,
                                         3000,
                                         17,
                                         1);
    CsParameters UlcsParam(CsParameters::ADD, UlClassifierUgs);
    UlServiceFlowUgs->SetConvergenceSublayerParam(UlcsParam);
    UlServiceFlowUgs->SetCsSpecification(ServiceFlow::IPV4);
    UlServiceFlowUgs->SetServiceSchedulingType(ServiceFlow::SF_TYPE_UGS);
    UlServiceFlowUgs->SetMaxSustainedTrafficRate(1000000);
    UlServiceFlowUgs->SetMinReservedTrafficRate(1000000);
    UlServiceFlowUgs->SetMinTolerableTrafficRate(1000000);
    UlServiceFlowUgs->SetMaximumLatency(10);
    UlServiceFlowUgs->SetMaxTrafficBurst(1000);
    UlServiceFlowUgs->SetTrafficPriority(1);
    ssDevs.Get(0)->GetObject<SubscriberStationNetDevice>()->AddServiceFlow(DlServiceFlowUgs);
    ssDevs.Get(0)->GetObject<SubscriberStationNetDevice>()->AddServiceFlow(UlServiceFlowUgs);

    Simulator::Stop(Seconds(duration));
    Simulator::Run();
    Simulator::Destroy();
}

/**
 * @ingroup wimax-test
 * @ingroup tests
 *
 * @brief Ns3 Wimax Service Flow Test Suite
 */
class Ns3WimaxServiceFlowTestSuite : public TestSuite
{
  public:
    Ns3WimaxServiceFlowTestSuite();
};

Ns3WimaxServiceFlowTestSuite::Ns3WimaxServiceFlowTestSuite()
    : TestSuite("wimax-service-flow", Type::UNIT)
{
    AddTestCase(new Ns3WimaxSfCreationTestCase, TestCase::Duration::QUICK);
}

static Ns3WimaxServiceFlowTestSuite ns3WimaxServiceFlowTestSuite; ///< the test suite
