/*
 * Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>
 */

#include "ns3/application-container.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/epc-tft.h"
#include "ns3/eps-bearer.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/ipv4-static-routing-helper.h"
#include "ns3/ipv4-static-routing.h"
#include "ns3/lte-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/nstime.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/point-to-point-epc-helper.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/position-allocator.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/uinteger.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("LteHandoverSpectrumModelTest");

/**
 * @ingroup lte-test
 *
 * @brief Regression test for @issueid{447}: a UE handing over between cells must
 * not abort with the spectrum-model mismatch assertion
 * (m_spectrumModel == x.m_spectrumModel) in SpectrumValue.
 *
 * During a handover the UE PHY is reconfigured to the target cell, which
 * changes the spectrum model of its noise/interference PSDs. A downlink
 * control reception associated with the previous configuration can complete
 * after the reconfiguration, so LteUePhy::GenerateMixedCqiReport() must not
 * divide two SpectrumValue objects with different spectrum models.
 *
 * The test runs a compact mobile multi-UE A3-RSRP handover scenario with the
 * control and data error models enabled (which trigger the handover/RLF
 * activity that exposes the race) and verifies that the simulation runs to
 * completion.
 */
class LteHandoverSpectrumModelTestCase : public TestCase
{
  public:
    LteHandoverSpectrumModelTestCase();
    ~LteHandoverSpectrumModelTestCase() override;

  private:
    void DoRun() override;
};

LteHandoverSpectrumModelTestCase::LteHandoverSpectrumModelTestCase()
    : TestCase("Handover does not abort on spectrum-model mismatch (issue #447)")
{
}

LteHandoverSpectrumModelTestCase::~LteHandoverSpectrumModelTestCase()
{
}

void
LteHandoverSpectrumModelTestCase::DoRun()
{
    // Deterministic run.
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);

    const double simTime = 5.0; // the pre-fix abort occurs at ~2.4 s
    const uint16_t numberOfUes = 30;
    const uint16_t numberOfCells = 5;
    const double speed = 16.0;

    Config::SetDefault("ns3::LteRlcUm::MaxTxBufferSize", UintegerValue(60 * 1024));
    Config::SetDefault("ns3::LteHelper::UseIdealRrc", BooleanValue(false));
    Config::SetDefault("ns3::LteSpectrumPhy::CtrlErrorModelEnabled", BooleanValue(true));
    Config::SetDefault("ns3::LteSpectrumPhy::DataErrorModelEnabled", BooleanValue(true));
    // Radio link failure timers.
    Config::SetDefault("ns3::LteUeRrc::N310", UintegerValue(1));
    Config::SetDefault("ns3::LteUeRrc::N311", UintegerValue(1));
    Config::SetDefault("ns3::LteUeRrc::T310", TimeValue(Seconds(1)));

    Ptr<LteHelper> lteHelper = CreateObject<LteHelper>();
    Ptr<PointToPointEpcHelper> epcHelper = CreateObject<PointToPointEpcHelper>();
    lteHelper->SetEpcHelper(epcHelper);
    lteHelper->SetPathlossModelType(TypeId::LookupByName("ns3::LogDistancePropagationLossModel"));
    lteHelper->SetSchedulerType("ns3::PfFfMacScheduler");
    lteHelper->SetHandoverAlgorithmType("ns3::A3RsrpHandoverAlgorithm");
    lteHelper->SetHandoverAlgorithmAttribute("Hysteresis", DoubleValue(3.0));
    lteHelper->SetHandoverAlgorithmAttribute("TimeToTrigger", TimeValue(MilliSeconds(256)));

    // Internet / remote host.
    Ptr<Node> pgw = epcHelper->GetPgwNode();
    NodeContainer remoteHostContainer;
    remoteHostContainer.Create(1);
    Ptr<Node> remoteHost = remoteHostContainer.Get(0);
    InternetStackHelper internet;
    internet.Install(remoteHostContainer);
    PointToPointHelper p2ph;
    p2ph.SetDeviceAttribute("DataRate", DataRateValue(DataRate("100Gb/s")));
    p2ph.SetDeviceAttribute("Mtu", UintegerValue(1500));
    p2ph.SetChannelAttribute("Delay", TimeValue(Seconds(0.010)));
    NetDeviceContainer internetDevices = p2ph.Install(pgw, remoteHost);
    Ipv4AddressHelper ipv4h;
    ipv4h.SetBase("1.0.0.0", "255.0.0.0");
    Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign(internetDevices);
    Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress(1);
    Ipv4StaticRoutingHelper ipv4RoutingHelper;
    Ptr<Ipv4StaticRouting> remoteHostStaticRouting =
        ipv4RoutingHelper.GetStaticRouting(remoteHost->GetObject<Ipv4>());
    remoteHostStaticRouting->AddNetworkRouteTo(Ipv4Address("7.0.0.0"), Ipv4Mask("255.0.0.0"), 1);

    // eNodeBs: one macro plus several small cells along a line.
    NodeContainer enbNodes;
    enbNodes.Create(numberOfCells);
    Ptr<ListPositionAllocator> enbPosition = CreateObject<ListPositionAllocator>();
    enbPosition->Add(Vector(500, 0, 0)); // macro
    enbPosition->Add(Vector(10, 0, 0));
    enbPosition->Add(Vector(30, 0, 0));
    enbPosition->Add(Vector(50, 0, 0));
    enbPosition->Add(Vector(70, 0, 0));
    MobilityHelper enbMobility;
    enbMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    enbMobility.SetPositionAllocator(enbPosition);
    enbMobility.Install(enbNodes);

    NetDeviceContainer enbDevs;
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(43.0));
    enbDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(0)));
    Config::SetDefault("ns3::LteEnbPhy::TxPower", DoubleValue(20.0));
    for (uint16_t j = 1; j < numberOfCells; j++)
    {
        enbDevs.Add(lteHelper->InstallEnbDevice(enbNodes.Get(j)));
    }

    // UEs moving along the line of small cells.
    NodeContainer ueNodes;
    ueNodes.Create(numberOfUes);
    MobilityHelper ueMobility;
    ueMobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    ueMobility.Install(ueNodes);
    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        ueNodes.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));
        ueNodes.Get(i)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(
            Vector(speed, 0, 0));
    }
    NetDeviceContainer ueDevs = lteHelper->InstallUeDevice(ueNodes);
    internet.Install(ueNodes);
    Ipv4InterfaceContainer ueIpIfaces = epcHelper->AssignUeIpv4Address(NetDeviceContainer(ueDevs));

    for (uint16_t i = 0; i < numberOfUes; i++)
    {
        lteHelper->Attach(ueDevs.Get(i));
    }

    // Downlink/uplink traffic and a dedicated bearer per UE. The scenario
    // mirrors the reporter's repro (sinks at both ends, a short packet
    // interval) which exposes the handover activity that triggers the bug.
    Ptr<UniformRandomVariable> startTime = CreateObject<UniformRandomVariable>();
    startTime->SetAttribute("Min", DoubleValue(0));
    startTime->SetAttribute("Max", DoubleValue(0.010));
    uint16_t dlPort = 10000;
    uint16_t ulPort = 20000;
    for (uint16_t u = 0; u < numberOfUes; ++u)
    {
        Ptr<Node> ue = ueNodes.Get(u);
        Ptr<Ipv4StaticRouting> ueStaticRouting =
            ipv4RoutingHelper.GetStaticRouting(ue->GetObject<Ipv4>());
        ueStaticRouting->SetDefaultRoute(epcHelper->GetUeDefaultGatewayAddress(), 1);

        ++dlPort;
        ++ulPort;

        UdpClientHelper dlClientHelper(ueIpIfaces.GetAddress(u), dlPort);
        dlClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
        dlClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
        ApplicationContainer dlApps = dlClientHelper.Install(remoteHost);
        PacketSinkHelper dlSink("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), dlPort));
        dlApps.Add(dlSink.Install(ue));

        UdpClientHelper ulClientHelper(remoteHostAddr, ulPort);
        ulClientHelper.SetAttribute("Interval", TimeValue(MilliSeconds(1)));
        ulClientHelper.SetAttribute("MaxPackets", UintegerValue(1000000));
        ApplicationContainer ulApps = ulClientHelper.Install(ue);
        PacketSinkHelper ulSink("ns3::UdpSocketFactory",
                                InetSocketAddress(Ipv4Address::GetAny(), ulPort));
        ulApps.Add(ulSink.Install(remoteHost));

        Ptr<EpcTft> tft = Create<EpcTft>();
        EpcTft::PacketFilter dlpf;
        dlpf.localPortStart = dlPort;
        dlpf.localPortEnd = dlPort;
        tft->Add(dlpf);
        EpcTft::PacketFilter ulpf;
        ulpf.remotePortStart = ulPort;
        ulpf.remotePortEnd = ulPort;
        tft->Add(ulpf);
        EpsBearer bearer(EpsBearer::NGBR_IMS);
        lteHelper->ActivateDedicatedEpsBearer(ueDevs.Get(u), bearer, tft);

        Time appStart = Seconds(startTime->GetValue());
        dlApps.Start(appStart);
        ulApps.Start(appStart);
    }

    lteHelper->AddX2Interface(enbNodes);

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    // Reaching this point means the simulation completed without the
    // spectrum-model mismatch assertion aborting it (@issueid{447}).
    NS_TEST_ASSERT_MSG_EQ(Simulator::Now().GetSeconds() >= simTime - 1e-6,
                          true,
                          "Simulation did not run to completion");

    Simulator::Destroy();
}

/**
 * @ingroup lte-test
 *
 * @brief Test suite for the handover spectrum-model regression test.
 */
class LteHandoverSpectrumModelTestSuite : public TestSuite
{
  public:
    LteHandoverSpectrumModelTestSuite();
};

LteHandoverSpectrumModelTestSuite::LteHandoverSpectrumModelTestSuite()
    : TestSuite("lte-handover-spectrum-model", Type::SYSTEM)
{
    AddTestCase(new LteHandoverSpectrumModelTestCase(), TestCase::Duration::EXTENSIVE);
}

/// Static variable for test initialization
static LteHandoverSpectrumModelTestSuite g_lteHandoverSpectrumModelTestSuite;
