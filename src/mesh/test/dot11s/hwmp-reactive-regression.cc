/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 */

#include "hwmp-reactive-regression.h"

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
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-helper.h"

#include <sstream>

HwmpReactiveRegressionTest::HwmpReactiveRegressionTest()
    : TestCase("HWMP on-demand regression test"),
      m_nodes(nullptr),
      m_time(Seconds(10)),
      m_sentPktsCounter(0),
      m_echoedPktsCounter(0)
{
}

HwmpReactiveRegressionTest::~HwmpReactiveRegressionTest()
{
    delete m_nodes;
}

void
HwmpReactiveRegressionTest::DoRun()
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
HwmpReactiveRegressionTest::CreateNodes()
{
    m_nodes = new NodeContainer;
    m_nodes->Create(6);
    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector(0, 0, 0));
    positionAlloc->Add(Vector(0, 120, 0));
    positionAlloc->Add(Vector(0, 240, 0));
    positionAlloc->Add(Vector(0, 360, 0));
    positionAlloc->Add(Vector(0, 480, 0));
    positionAlloc->Add(Vector(0, 600, 0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(*m_nodes);
    Simulator::Schedule(Seconds(5), &HwmpReactiveRegressionTest::ResetPosition, this);
}

void
HwmpReactiveRegressionTest::InstallApplications()
{
    // client socket
    m_clientSocket =
        Socket::CreateSocket(m_nodes->Get(5), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_clientSocket->Bind();
    m_clientSocket->Connect(InetSocketAddress(m_interfaces.GetAddress(0), 9));
    m_clientSocket->SetRecvCallback(
        MakeCallback(&HwmpReactiveRegressionTest::HandleReadClient, this));
    Simulator::ScheduleWithContext(m_clientSocket->GetNode()->GetId(),
                                   Seconds(2),
                                   &HwmpReactiveRegressionTest::SendData,
                                   this,
                                   m_clientSocket);

    // server socket
    m_serverSocket =
        Socket::CreateSocket(m_nodes->Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_serverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
    m_serverSocket->SetRecvCallback(
        MakeCallback(&HwmpReactiveRegressionTest::HandleReadServer, this));
}

void
HwmpReactiveRegressionTest::CreateDevices()
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
    // This test was written prior to the preamble detection model
    wifiPhy.DisablePreambleDetectionModel();

    // 2. setup mesh
    MeshHelper mesh = MeshHelper::Default();
    mesh.SetStackInstaller("ns3::Dot11sStack");
    mesh.SetMacType("RandomStart", TimeValue(MilliSeconds(100)));
    mesh.SetNumberOfInterfaces(1);
    m_meshDevices = mesh.Install(wifiPhy, *m_nodes);
    // Six devices, 10 streams per device
    streamsUsed = mesh.AssignStreams(m_meshDevices, streamNumber);
    streamNumber += streamIncrement;
    NS_TEST_EXPECT_MSG_EQ(streamsUsed, (m_meshDevices.GetN() * 10), "Stream assignment mismatch");
    streamsUsed = wifiChannel.AssignStreams(chan, streamNumber);
    streamNumber += streamIncrement;
    NS_TEST_EXPECT_MSG_EQ(streamsUsed, 0, "No stream assignments expected for WifiChannel");

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
HwmpReactiveRegressionTest::CheckResults()
{
    using Type = HwmpControlRecorder::Type;
    using Event = HwmpControlRecorder::Event;
    const auto server = m_recorder.GetAddress(0);
    const auto client = m_recorder.GetAddress(5);

    // The PREQ of the server for the client is relayed hop by hop to the client
    for (uint32_t i = 1; i < 6; ++i)
    {
        NS_TEST_EXPECT_MSG_GT_OR_EQ(m_recorder.Count([&](const Event& e) {
            return e.type == Type::PREQ && e.receiver == i && e.transmitter == i - 1 &&
                   e.originator == server && e.target == client && e.time < Seconds(5);
        }),
                                    1,
                                    "Mesh point " << i << " should receive the PREQ from "
                                                  << i - 1);
    }

    // The PREP of the client is unicast hop by hop back to the server
    Time last;
    for (int32_t i = 4; i >= 0; --i)
    {
        const auto prep = m_recorder.FindFirst([&](const Event& e) {
            return e.type == Type::PREP && e.receiver == static_cast<uint32_t>(i) &&
                   e.transmitter == static_cast<uint32_t>(i + 1) && e.originator == client &&
                   e.target == server;
        });
        NS_TEST_ASSERT_MSG_NE(prep, nullptr, "Mesh point " << i << " should receive the PREP");
        if (!prep)
        {
            return;
        }
        NS_TEST_EXPECT_MSG_GT(prep->time, last, "PREP should travel towards the server");
        last = prep->time;
    }

    // Every packet sent before mesh point 3 disappears at 5s is echoed, none afterwards:
    // one packet every 500 ms from 2s gives 6 packets in [2s, 5s)
    NS_TEST_EXPECT_MSG_EQ(m_echoedPktsCounter, 6, "Unexpected number of echoed packets");

    // The neighbors of mesh point 3 report the broken path towards both ends of the chain
    NS_TEST_EXPECT_MSG_GT(m_recorder.Count([](const Event& e) {
        return e.type == Type::PERR && e.receiver == 5 && e.transmitter == 4 && e.time > Seconds(5);
    }),
                          0,
                          "Mesh point 5 should receive a PERR from mesh point 4");
    NS_TEST_EXPECT_MSG_GT(m_recorder.Count([](const Event& e) {
        return e.type == Type::PERR && e.receiver == 0 && e.transmitter == 1 && e.time > Seconds(5);
    }),
                          0,
                          "Mesh point 0 should receive a PERR from mesh point 1");

    // The client retries path discovery, but its PREQs cannot cross the gap
    const auto retries = m_recorder.Count([&](const Event& e) {
        return e.type == Type::PREQ && e.originator == client && e.target == server &&
               e.time > Seconds(5);
    });
    NS_TEST_EXPECT_MSG_GT(retries, 1, "Client should retry path discovery");
    NS_TEST_EXPECT_MSG_EQ(m_recorder.Count([&](const Event& e) {
        return e.type == Type::PREQ && e.originator == client && e.time > Seconds(5) &&
               e.receiver != 4;
    }),
                          0,
                          "Retried PREQs should only reach mesh point 4");
    NS_TEST_EXPECT_MSG_EQ(m_recorder.Count([](const Event& e) {
        return e.type == Type::PREP && e.time > Seconds(5);
    }),
                          0,
                          "No path can be found after mesh point 3 disappears");
}

void
HwmpReactiveRegressionTest::ResetPosition()
{
    Ptr<Object> object = m_nodes->Get(3);
    Ptr<MobilityModel> model = object->GetObject<MobilityModel>();
    if (!model)
    {
        return;
    }
    model->SetPosition(Vector(9000, 0, 0));
}

void
HwmpReactiveRegressionTest::SendData(Ptr<Socket> socket)
{
    if ((Simulator::Now() < m_time) && (m_sentPktsCounter < 300))
    {
        socket->Send(Create<Packet>(20));
        m_sentPktsCounter++;
        Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                       MilliSeconds(500),
                                       &HwmpReactiveRegressionTest::SendData,
                                       this,
                                       socket);
    }
}

void
HwmpReactiveRegressionTest::HandleReadServer(Ptr<Socket> socket)
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
HwmpReactiveRegressionTest::HandleReadClient(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        m_echoedPktsCounter++;
    }
}
