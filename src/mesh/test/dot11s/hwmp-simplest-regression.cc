/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev  <andreev@iitp.ru>
 */

#include "hwmp-simplest-regression.h"

#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mesh-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/peer-management-protocol.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-helper.h"

#include <sstream>

using namespace ns3;

HwmpSimplestRegressionTest::HwmpSimplestRegressionTest()
    : TestCase("Simplest HWMP regression test"),
      m_nodes(nullptr),
      m_time(Seconds(15)),
      m_sentPktsCounter(0),
      m_echoedPktsCounter(0)
{
}

HwmpSimplestRegressionTest::~HwmpSimplestRegressionTest()
{
    delete m_nodes;
}

void
HwmpSimplestRegressionTest::DoRun()
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
HwmpSimplestRegressionTest::CreateNodes()
{
    m_nodes = new NodeContainer;
    m_nodes->Create(2);
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(1 /*meter*/),
                                  "DeltaY",
                                  DoubleValue(0),
                                  "GridWidth",
                                  UintegerValue(2),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(*m_nodes);
    Simulator::Schedule(Seconds(10), &HwmpSimplestRegressionTest::ResetPosition, this);
}

void
HwmpSimplestRegressionTest::ResetPosition()
{
    Ptr<Object> object = m_nodes->Get(1);
    Ptr<MobilityModel> model = object->GetObject<MobilityModel>();
    if (!model)
    {
        return;
    }
    model->SetPosition(Vector(9000, 0, 0));
}

void
HwmpSimplestRegressionTest::InstallApplications()
{
    // client socket
    m_clientSocket =
        Socket::CreateSocket(m_nodes->Get(1), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_clientSocket->Bind();
    m_clientSocket->Connect(InetSocketAddress(m_interfaces.GetAddress(0), 9));
    m_clientSocket->SetRecvCallback(
        MakeCallback(&HwmpSimplestRegressionTest::HandleReadClient, this));
    Simulator::ScheduleWithContext(m_clientSocket->GetNode()->GetId(),
                                   Seconds(2),
                                   &HwmpSimplestRegressionTest::SendData,
                                   this,
                                   m_clientSocket);

    // server socket
    m_serverSocket =
        Socket::CreateSocket(m_nodes->Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_serverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
    m_serverSocket->SetRecvCallback(
        MakeCallback(&HwmpSimplestRegressionTest::HandleReadServer, this));
}

void
HwmpSimplestRegressionTest::CreateDevices()
{
    int64_t streamsUsed = 0;
    int64_t streamNumber = 0;
    int64_t streamIncrement = 1000;
    // 1. setup WiFi
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> chan = wifiChannel.Create();
    wifiPhy.SetChannel(chan);

    // 2. setup mesh
    MeshHelper mesh = MeshHelper::Default();
    mesh.SetStackInstaller("ns3::Dot11sStack");
    mesh.SetMacType("RandomStart", TimeValue(MilliSeconds(100)));
    mesh.SetNumberOfInterfaces(1);
    m_meshDevices = mesh.Install(wifiPhy, *m_nodes);
    // Two devices, ten streams per mesh device
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
HwmpSimplestRegressionTest::CheckResults()
{
    using Type = HwmpControlRecorder::Type;
    using Event = HwmpControlRecorder::Event;
    const auto addr0 = m_recorder.GetAddress(0);
    const auto addr1 = m_recorder.GetAddress(1);

    // Path discovery started by the server when it answers the first ARP request
    const auto preq = m_recorder.FindFirst([&](const Event& e) {
        return e.type == Type::PREQ && e.receiver == 1 && e.originator == addr0 &&
               e.target == addr1;
    });
    NS_TEST_ASSERT_MSG_NE(preq, nullptr, "No PREQ from mesh point 0 for mesh point 1");
    if (!preq)
    {
        return;
    }
    NS_TEST_EXPECT_MSG_GT_OR_EQ(preq->time, Seconds(2), "PREQ sent before data started");
    const auto prep = m_recorder.FindFirst([&](const Event& e) {
        return e.type == Type::PREP && e.receiver == 0 && e.originator == addr1 &&
               e.target == addr0;
    });
    NS_TEST_ASSERT_MSG_NE(prep, nullptr, "No PREP from mesh point 1 for mesh point 0");
    if (!prep)
    {
        return;
    }
    NS_TEST_EXPECT_MSG_GT(prep->time, preq->time, "PREP should answer the PREQ");

    // Path discovery started by the client after its route expired
    const auto renewal = m_recorder.FindFirst([&](const Event& e) {
        return e.type == Type::PREQ && e.receiver == 0 && e.originator == addr1 &&
               e.target == addr0;
    });
    NS_TEST_ASSERT_MSG_NE(renewal, nullptr, "No PREQ from mesh point 1 after route expiration");
    if (!renewal)
    {
        return;
    }
    // Data starts at 2s and the route expires after the default
    // Dot11MeshHWMPactivePathTimeout (5.12s)
    NS_TEST_EXPECT_MSG_GT(renewal->time, Seconds(7), "Route should expire after time 7");
    NS_TEST_EXPECT_MSG_EQ(m_recorder.Count([&](const Event& e) {
        return e.type == Type::PREP && e.receiver == 1 && e.originator == addr0 &&
               e.time > renewal->time;
    }),
                          1,
                          "Mesh point 0 should answer the renewed PREQ");
    NS_TEST_EXPECT_MSG_EQ(m_recorder.Count([](const Event& e) { return e.type == Type::PERR; }),
                          0,
                          "No PERR expected with a single hop");

    // Every packet sent in [2s, 10s) is echoed; nothing gets through after the stations move apart.
    // One packet every 50 ms in [2s, 10s) gives 160 packets
    NS_TEST_EXPECT_MSG_EQ(m_echoedPktsCounter, 160, "Unexpected number of echoed packets");

    // Peer link is closed after the stations become unreachable
    for (uint32_t i = 0; i < 2; ++i)
    {
        auto pmp = m_meshDevices.Get(i)->GetObject<dot11s::PeerManagementProtocol>();
        NS_TEST_EXPECT_MSG_EQ(pmp->GetPeerLinks().size(),
                              0,
                              "Mesh point " << i << " should have closed its peer link");
    }
}

void
HwmpSimplestRegressionTest::SendData(Ptr<Socket> socket)
{
    if ((Simulator::Now() < m_time) && (m_sentPktsCounter < 300))
    {
        socket->Send(Create<Packet>(100));
        m_sentPktsCounter++;
        Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                       MilliSeconds(50),
                                       &HwmpSimplestRegressionTest::SendData,
                                       this,
                                       socket);
    }
}

void
HwmpSimplestRegressionTest::HandleReadServer(Ptr<Socket> socket)
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
HwmpSimplestRegressionTest::HandleReadClient(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        m_echoedPktsCounter++;
    }
}
