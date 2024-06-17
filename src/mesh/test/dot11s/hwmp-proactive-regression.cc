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
#include "ns3/mesh-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/pcap-test.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-helper.h"

#include <sstream>

using namespace ns3;

/// Unique PCAP file name prefix
const char* const PREFIX = "hwmp-proactive-regression-test";

HwmpProactiveRegressionTest::HwmpProactiveRegressionTest()
    : TestCase("HWMP proactive regression test"),
      m_nodes(nullptr),
      m_time(Seconds(5)),
      m_sentPktsCounter(0)
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
    Simulator::Destroy();

    CheckResults();

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
                                   Seconds(2.5),
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
    NetDeviceContainer meshDevices = mesh.Install(wifiPhy, *m_nodes);
    // Five devices, 10 streams per device
    streamsUsed += mesh.AssignStreams(meshDevices, streamsUsed);
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (meshDevices.GetN() * 10), "Stream mismatch");
    // No streams used here, by default, so streamsUsed should not change
    streamsUsed += wifiChannel.AssignStreams(chan, streamsUsed);
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (meshDevices.GetN() * 10), "Stream mismatch");

    // 3. setup TCP/IP
    InternetStackHelper internetStack;
    internetStack.Install(*m_nodes);
    streamsUsed += internetStack.AssignStreams(*m_nodes, streamsUsed);
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    m_interfaces = address.Assign(meshDevices);
    // 4. write PCAP if needed
    wifiPhy.EnablePcapAll(CreateTempDirFilename(PREFIX));
}

void
HwmpProactiveRegressionTest::CheckResults()
{
    for (int i = 0; i < 5; ++i)
    {
        NS_PCAP_TEST_EXPECT_EQ(PREFIX << "-" << i << "-1.pcap");
    }
}

void
HwmpProactiveRegressionTest::SendData(Ptr<Socket> socket)
{
    if ((Simulator::Now() < m_time) && (m_sentPktsCounter < 300))
    {
        socket->Send(Create<Packet>(100));
        m_sentPktsCounter++;
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
    }
}
