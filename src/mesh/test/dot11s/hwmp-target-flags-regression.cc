/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev  <andreev@iitp.ru>
 */

#include "hwmp-target-flags-regression.h"

#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mesh-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-helper.h"

#include <sstream>

using namespace ns3;

HwmpDoRfRegressionTest::HwmpDoRfRegressionTest()
    : TestCase("HWMP target flags regression test"),
      m_nodes(nullptr),
      m_time(Seconds(5)),
      m_sentPktsCounterA(0),
      m_sentPktsCounterB(0),
      m_sentPktsCounterC(0),
      m_echoedPktsCounter(0)
{
}

HwmpDoRfRegressionTest::~HwmpDoRfRegressionTest()
{
    delete m_nodes;
}

void
HwmpDoRfRegressionTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    CreateNodes();
    CreateDevices();
    InstallApplications();

    Simulator::Stop(m_time);
    Simulator::Run();
    CheckResults();
    Simulator::Destroy();

    delete m_nodes, m_nodes = nullptr;
}

void
HwmpDoRfRegressionTest::CreateNodes()
{
    m_nodes = new NodeContainer;
    m_nodes->Create(4);
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(100),
                                  "DeltaY",
                                  DoubleValue(0),
                                  "GridWidth",
                                  UintegerValue(4),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(*m_nodes);
}

void
HwmpDoRfRegressionTest::InstallApplications()
{
    // server socket A
    m_serverSocketA =
        Socket::CreateSocket(m_nodes->Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_serverSocketA->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
    m_serverSocketA->SetRecvCallback(MakeCallback(&HwmpDoRfRegressionTest::HandleReadServer, this));

    // server socket B
    m_serverSocketB =
        Socket::CreateSocket(m_nodes->Get(3), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_serverSocketB->Bind(InetSocketAddress(Ipv4Address::GetAny(), 10));
    m_serverSocketB->SetRecvCallback(MakeCallback(&HwmpDoRfRegressionTest::HandleReadServer, this));

    // client socket A
    m_clientSocketA =
        Socket::CreateSocket(m_nodes->Get(1), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_clientSocketA->Bind();
    m_clientSocketA->Connect(InetSocketAddress(m_interfaces.GetAddress(0), 9));
    m_clientSocketA->SetRecvCallback(MakeCallback(&HwmpDoRfRegressionTest::HandleReadClient, this));
    Simulator::ScheduleWithContext(m_clientSocketA->GetNode()->GetId(),
                                   Seconds(2.2),
                                   &HwmpDoRfRegressionTest::SendDataA,
                                   this,
                                   m_clientSocketA);

    // client socket B
    m_clientSocketB =
        Socket::CreateSocket(m_nodes->Get(2), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_clientSocketB->Bind();
    m_clientSocketB->Connect(InetSocketAddress(m_interfaces.GetAddress(0), 9));
    m_clientSocketB->SetRecvCallback(MakeCallback(&HwmpDoRfRegressionTest::HandleReadClient, this));
    Simulator::ScheduleWithContext(m_clientSocketB->GetNode()->GetId(),
                                   Seconds(2),
                                   &HwmpDoRfRegressionTest::SendDataB,
                                   this,
                                   m_clientSocketB);

    // client socket C
    m_clientSocketB =
        Socket::CreateSocket(m_nodes->Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_clientSocketB->Bind();
    m_clientSocketB->Connect(InetSocketAddress(m_interfaces.GetAddress(3), 10));
    m_clientSocketB->SetRecvCallback(MakeCallback(&HwmpDoRfRegressionTest::HandleReadClient, this));
    Simulator::ScheduleWithContext(m_clientSocketB->GetNode()->GetId(),
                                   Seconds(2.4),
                                   &HwmpDoRfRegressionTest::SendDataC,
                                   this,
                                   m_clientSocketB);
}

void
HwmpDoRfRegressionTest::CreateDevices()
{
    int64_t streamsUsed = 0;
    int64_t streamNumber = 0;
    int64_t streamIncrement = 1000;
    // 1. setup WiFi
    YansWifiPhyHelper wifiPhy;
    // This test suite output was originally based on YansErrorRateModel
    wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> chan = wifiChannel.Create();
    wifiPhy.SetChannel(chan);
    // This scenario setup predates the preamble detection model
    wifiPhy.DisablePreambleDetectionModel();

    // 2. setup mesh
    MeshHelper mesh = MeshHelper::Default();
    mesh.SetStackInstaller("ns3::Dot11sStack");
    mesh.SetMacType("RandomStart", TimeValue(Seconds(0.1)));
    mesh.SetNumberOfInterfaces(1);
    m_meshDevices = mesh.Install(wifiPhy, *m_nodes);
    // Four devices, ten streams per mac
    streamsUsed = mesh.AssignStreams(m_meshDevices, streamNumber);
    streamNumber += streamIncrement;
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (m_meshDevices.GetN() * 10), "Stream assignment mismatch");
    streamsUsed = wifiChannel.AssignStreams(chan, streamNumber);
    streamNumber += streamIncrement;
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, 0, "No stream assignments expected for WifiChannel");

    // 3. setup TCP/IP
    InternetStackHelper internetStack;
    internetStack.SetIpv6StackInstall(false);
    internetStack.Install(*m_nodes);
    internetStack.AssignStreams(*m_nodes, streamNumber);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    m_interfaces = address.Assign(m_meshDevices);
    m_recorder.Connect(m_meshDevices);
}

void
HwmpDoRfRegressionTest::CheckResults()
{
    using Type = HwmpControlRecorder::Type;
    using Event = HwmpControlRecorder::Event;
    const auto addr0 = m_recorder.GetAddress(0);
    const auto addr1 = m_recorder.GetAddress(1);
    const auto addr3 = m_recorder.GetAddress(3);

    // Mesh point 1 learns the route to mesh point 0 from the first PREQ/PREP exchange,
    // so it never needs a path discovery of its own
    NS_TEST_EXPECT_MSG_EQ(m_recorder.Count([&](const Event& e) {
        return e.type == Type::PREQ && e.originator == addr1;
    }),
                          0,
                          "Mesh point 1 should not start a path discovery");

    // Mesh point 3 asks for mesh point 0 with DO=0, RF=1
    const auto preq = m_recorder.FindFirst([&](const Event& e) {
        return e.type == Type::PREQ && e.receiver == 2 && e.transmitter == 3 &&
               e.originator == addr3 && e.target == addr0;
    });
    NS_TEST_ASSERT_MSG_NE(preq, nullptr, "Mesh point 2 should receive the PREQ of mesh point 3");
    if (!preq)
    {
        return;
    }
    NS_TEST_EXPECT_MSG_EQ(preq->doFlag, false, "DO flag should be cleared");
    NS_TEST_EXPECT_MSG_EQ(preq->rfFlag, true, "RF flag should be set");

    // Mesh point 2 knows a route to mesh point 0 and replies on its behalf
    const auto prep = m_recorder.FindFirst([&](const Event& e) {
        return e.type == Type::PREP && e.receiver == 3 && e.transmitter == 2 &&
               e.originator == addr0 && e.target == addr3;
    });
    NS_TEST_ASSERT_MSG_NE(prep, nullptr, "Mesh point 2 should send an intermediate PREP");
    if (!prep)
    {
        return;
    }
    NS_TEST_EXPECT_MSG_GT(prep->time, preq->time, "Intermediate PREP should answer the PREQ");

    // ... and forwards the PREQ with DO=1, RF=0
    const auto forwarded = m_recorder.FindFirst([&](const Event& e) {
        return e.type == Type::PREQ && e.receiver == 1 && e.transmitter == 2 &&
               e.originator == addr3 && e.target == addr0;
    });
    NS_TEST_ASSERT_MSG_NE(forwarded, nullptr, "Mesh point 2 should forward the PREQ");
    if (!forwarded)
    {
        return;
    }
    NS_TEST_EXPECT_MSG_EQ(forwarded->doFlag, true, "Forwarded PREQ should have DO flag set");
    NS_TEST_EXPECT_MSG_EQ(forwarded->rfFlag, false, "Forwarded PREQ should have RF flag cleared");

    // Every packet of the three streams is echoed
    NS_TEST_EXPECT_MSG_GT(m_sentPktsCounterA, 0, "No packets sent by client A");
    NS_TEST_EXPECT_MSG_GT(m_sentPktsCounterB, 0, "No packets sent by client B");
    NS_TEST_EXPECT_MSG_GT(m_sentPktsCounterC, 0, "No packets sent by client C");
    NS_TEST_EXPECT_MSG_EQ(m_echoedPktsCounter,
                          m_sentPktsCounterA + m_sentPktsCounterB + m_sentPktsCounterC,
                          "Every packet should be echoed");
}

void
HwmpDoRfRegressionTest::SendDataA(Ptr<Socket> socket)
{
    if ((Simulator::Now() < m_time) && (m_sentPktsCounterA < 300))
    {
        socket->Send(Create<Packet>(100));
        m_sentPktsCounterA++;
        Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                       Seconds(1),
                                       &HwmpDoRfRegressionTest::SendDataA,
                                       this,
                                       socket);
    }
}

void
HwmpDoRfRegressionTest::SendDataB(Ptr<Socket> socket)
{
    if ((Simulator::Now() < m_time) && (m_sentPktsCounterA < 300))
    {
        socket->Send(Create<Packet>(100));
        m_sentPktsCounterB++;
        Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                       Seconds(1),
                                       &HwmpDoRfRegressionTest::SendDataB,
                                       this,
                                       socket);
    }
}

void
HwmpDoRfRegressionTest::SendDataC(Ptr<Socket> socket)
{
    if ((Simulator::Now() < m_time) && (m_sentPktsCounterA < 300))
    {
        socket->Send(Create<Packet>(100));
        m_sentPktsCounterC++;
        Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                       Seconds(1),
                                       &HwmpDoRfRegressionTest::SendDataC,
                                       this,
                                       socket);
    }
}

void
HwmpDoRfRegressionTest::HandleReadServer(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        packet->RemoveAllPacketTags();
        packet->RemoveAllByteTags();

        socket->SendTo(packet, 0, from);
    }
}

void
HwmpDoRfRegressionTest::HandleReadClient(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        m_echoedPktsCounter++;
    }
}
