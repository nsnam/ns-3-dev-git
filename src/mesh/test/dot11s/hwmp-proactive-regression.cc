/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev  <andreev@iitp.ru>
 */

#include "hwmp-proactive-regression.h"

#include "ns3/abort.h"
#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-interface-container.h"
#include "ns3/log.h"
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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HwmpProactiveRegression");

HwmpProactiveRegressionTest::HwmpProactiveRegressionTest()
    : TestCase("HWMP proactive regression test"),
      m_nodes(nullptr),
      m_time(Seconds(5)),
      m_dataStart(Seconds(2.5)),
      m_sentPktsCounter(0),
      m_echoedPktsCounter(0)
{
}

HwmpProactiveRegressionTest::~HwmpProactiveRegressionTest()
{
    delete m_nodes;
}

void
HwmpProactiveRegressionTest::DoRun()
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
HwmpProactiveRegressionTest::CreateNodes()
{
    m_nodes = new NodeContainer;
    m_nodes->Create(5);
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
                                  UintegerValue(5),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(*m_nodes);
}

void
HwmpProactiveRegressionTest::InstallApplications()
{
    // client socket
    m_clientSocket =
        Socket::CreateSocket(m_nodes->Get(4), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_clientSocket->Bind();
    m_clientSocket->Connect(InetSocketAddress(m_interfaces.GetAddress(0), 9));
    m_clientSocket->SetRecvCallback(
        MakeCallback(&HwmpProactiveRegressionTest::HandleReadClient, this));
    Simulator::ScheduleWithContext(m_clientSocket->GetNode()->GetId(),
                                   m_dataStart,
                                   &HwmpProactiveRegressionTest::SendData,
                                   this,
                                   m_clientSocket);

    // server socket
    m_serverSocket =
        Socket::CreateSocket(m_nodes->Get(0), TypeId::LookupByName("ns3::UdpSocketFactory"));
    m_serverSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), 9));
    m_serverSocket->SetRecvCallback(
        MakeCallback(&HwmpProactiveRegressionTest::HandleReadServer, this));
}

void
HwmpProactiveRegressionTest::CreateDevices()
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
    // The middle node will have address 00:00:00:00:00:03,
    // so we set this to the root as per the comment in the header file.
    mesh.SetStackInstaller("ns3::Dot11sStack",
                           "Root",
                           Mac48AddressValue(Mac48Address("00:00:00:00:00:03")));
    mesh.SetMacType("RandomStart", TimeValue(Seconds(0.1)));
    mesh.SetNumberOfInterfaces(1);
    m_meshDevices = mesh.Install(wifiPhy, *m_nodes);
    // Five devices, 10 streams per device
    streamsUsed = mesh.AssignStreams(m_meshDevices, streamNumber);
    streamNumber += streamIncrement;
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (m_meshDevices.GetN() * 10), "Stream mismatch");
    // No streams used here, by default, so streamsUsed should not change
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
HwmpProactiveRegressionTest::CheckResults()
{
    using Type = HwmpControlRecorder::Type;
    using Event = HwmpControlRecorder::Event;
    const uint32_t rootIndex = 2;
    const auto root = m_recorder.GetAddress(rootIndex);

    // Only the root sends PREQs, and they are proactive (broadcast target)
    NS_TEST_EXPECT_MSG_GT(m_recorder.Count([](const Event& e) { return e.type == Type::PREQ; }),
                          0,
                          "Root should send proactive PREQs");
    NS_TEST_EXPECT_MSG_EQ(m_recorder.Count([&](const Event& e) {
        return e.type == Type::PREQ &&
               (e.originator != root || e.target != Mac48Address::GetBroadcast());
    }),
                          0,
                          "No reactive path discovery should be initiated");

    // Every other mesh point answers the root's PREQ with a PREP, and all
    // proactive paths are in place before data transfer starts
    for (uint32_t i = 0; i < 5; ++i)
    {
        if (i == rootIndex)
        {
            continue;
        }
        const auto addr = m_recorder.GetAddress(i);
        const auto preq = m_recorder.FindFirst([&](const Event& e) {
            return e.type == Type::PREQ && e.receiver == i && e.originator == root;
        });
        NS_TEST_EXPECT_MSG_NE(preq, nullptr, "Mesh point " << i << " received no root PREQ");
        const auto prep = m_recorder.FindFirst([&](const Event& e) {
            return e.type == Type::PREP && e.receiver == rootIndex && e.originator == addr &&
                   e.target == root;
        });
        NS_TEST_EXPECT_MSG_NE(prep, nullptr, "Root received no PREP from mesh point " << i);
        if (!preq || !prep)
        {
            continue;
        }
        NS_TEST_EXPECT_MSG_GT(prep->time,
                              preq->time,
                              "PREP from mesh point " << i << " should answer the root PREQ");
        NS_TEST_EXPECT_MSG_LT(prep->time,
                              m_dataStart,
                              "Proactive path to mesh point " << i << " not set up before data");
    }
    NS_TEST_EXPECT_MSG_EQ(m_recorder.Count([](const Event& e) { return e.type == Type::PERR; }),
                          0,
                          "No PERR expected");

    // Data is forwarded by the proactive path
    NS_TEST_EXPECT_MSG_GT(m_sentPktsCounter, 0, "No packets sent");
    NS_TEST_EXPECT_MSG_EQ(m_echoedPktsCounter, m_sentPktsCounter, "Every packet should be echoed");
}

void
HwmpProactiveRegressionTest::SendData(Ptr<Socket> socket)
{
    if ((Simulator::Now() < m_time) && (m_sentPktsCounter < 300))
    {
        socket->Send(Create<Packet>(100));
        m_sentPktsCounter++;
        NS_LOG_INFO("Send packet; total sent = " << m_sentPktsCounter);
        Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                       Seconds(0.5),
                                       &HwmpProactiveRegressionTest::SendData,
                                       this,
                                       socket);
    }
}

void
HwmpProactiveRegressionTest::HandleReadServer(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        NS_LOG_INFO("Receive packet from " << InetSocketAddress::ConvertFrom(from).GetIpv4()
                                           << "; return to sender");
        packet->RemoveAllPacketTags();
        packet->RemoveAllByteTags();

        socket->SendTo(packet, 0, from);
    }
}

void
HwmpProactiveRegressionTest::HandleReadClient(Ptr<Socket> socket)
{
    Ptr<Packet> packet;
    Address from;
    while ((packet = socket->RecvFrom(from)))
    {
        m_echoedPktsCounter++;
        NS_LOG_INFO("Receive packet from " << InetSocketAddress::ConvertFrom(from).GetIpv4());
    }
}
