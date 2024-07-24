/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include "bug-772.h"

#include "ns3/abort.h"
#include "ns3/aodv-helper.h"
#include "ns3/boolean.h"
#include "ns3/config.h"
#include "ns3/data-rate.h"
#include "ns3/double.h"
#include "ns3/inet-socket-address.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/mobility-model.h"
#include "ns3/pcap-file.h"
#include "ns3/pcap-test.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-helper.h"

#include <sstream>

using namespace ns3;

//-----------------------------------------------------------------------------
// UdpChainTest
//-----------------------------------------------------------------------------
Bug772ChainTest::Bug772ChainTest(const char* const prefix,
                                 const char* const proto,
                                 Time t,
                                 uint32_t size)
    : TestCase("Bug 772 UDP and TCP chain regression test"),
      m_nodes(nullptr),
      m_prefix(prefix),
      m_proto(proto),
      m_time(t),
      m_size(size),
      m_step(120),
      m_port(9),
      m_receivedPackets(0)
{
}

Bug772ChainTest::~Bug772ChainTest()
{
    delete m_nodes;
}

void
Bug772ChainTest::SendData(Ptr<Socket> socket)
{
    if (Simulator::Now() < m_time)
    {
        socket->Send(Create<Packet>(1000));
        Simulator::ScheduleWithContext(socket->GetNode()->GetId(),
                                       Seconds(0.25),
                                       &Bug772ChainTest::SendData,
                                       this,
                                       socket);
    }
}

void
Bug772ChainTest::HandleRead(Ptr<Socket> socket)
{
    m_receivedPackets++;
}

void
Bug772ChainTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(2);

    // Default of 3 will cause packet loss
    Config::SetDefault("ns3::ArpCache::PendingQueueSize", UintegerValue(10));

    CreateNodes();
    CreateDevices();

    Simulator::Stop(m_time + Seconds(1)); // Allow buffered packets to clear
    Simulator::Run();
    Simulator::Destroy();

    CheckResults();

    delete m_nodes, m_nodes = nullptr;
}

void
Bug772ChainTest::CreateNodes()
{
    m_nodes = new NodeContainer;
    m_nodes->Create(m_size);
    MobilityHelper mobility;
    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX",
                                  DoubleValue(0.0),
                                  "MinY",
                                  DoubleValue(0.0),
                                  "DeltaX",
                                  DoubleValue(m_step),
                                  "DeltaY",
                                  DoubleValue(0),
                                  "GridWidth",
                                  UintegerValue(m_size),
                                  "LayoutType",
                                  StringValue("RowFirst"));
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(*m_nodes);
}

void
Bug772ChainTest::CreateDevices()
{
    int64_t streamsUsed = 0;
    // 1. Setup WiFi
    WifiMacHelper wifiMac;
    wifiMac.SetType("ns3::AdhocWifiMac");
    YansWifiPhyHelper wifiPhy;
    wifiPhy.DisablePreambleDetectionModel();
    // This test suite output was originally based on YansErrorRateModel
    wifiPhy.SetErrorRateModel("ns3::YansErrorRateModel");
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> chan = wifiChannel.Create();
    wifiPhy.SetChannel(chan);
    wifiPhy.Set(
        "TxGain",
        DoubleValue(1.0)); // this configuration should go away in future revision to the test
    wifiPhy.Set(
        "RxGain",
        DoubleValue(1.0)); // this configuration should go away in future revision to the test
    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211a);
    wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                 "DataMode",
                                 StringValue("OfdmRate6Mbps"),
                                 "RtsCtsThreshold",
                                 StringValue("2200"));
    NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, *m_nodes);

    // Assign fixed stream numbers to wifi and channel random variables
    streamsUsed += WifiHelper::AssignStreams(devices, streamsUsed);
    // Assign 6 streams per device
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (devices.GetN() * 2), "Stream assignment mismatch");
    streamsUsed += wifiChannel.AssignStreams(chan, streamsUsed);
    // Assign 0 streams per channel for this configuration
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (devices.GetN() * 2), "Stream assignment mismatch");

    // 2. Setup TCP/IP & AODV
    AodvHelper aodv; // Use default parameters here
    InternetStackHelper internetStack;
    internetStack.SetRoutingHelper(aodv);
    internetStack.Install(*m_nodes);
    streamsUsed += internetStack.AssignStreams(*m_nodes, streamsUsed);
    // Expect to use (3*m_size) more streams for internet stack random variables
    NS_TEST_ASSERT_MSG_EQ(streamsUsed,
                          ((devices.GetN() * 3) + (3 * m_size)),
                          "Stream assignment mismatch");
    streamsUsed += aodv.AssignStreams(*m_nodes, streamsUsed);
    // Expect to use m_size more streams for AODV
    NS_TEST_ASSERT_MSG_EQ(streamsUsed,
                          ((devices.GetN() * 3) + (3 * m_size) + m_size),
                          "Stream assignment mismatch");
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // 3. Setup UDP source and sink
    m_sendSocket = Socket::CreateSocket(m_nodes->Get(0), TypeId::LookupByName(m_proto));
    m_sendSocket->Bind();
    m_sendSocket->Connect(InetSocketAddress(interfaces.GetAddress(m_size - 1), m_port));
    m_sendSocket->SetAllowBroadcast(true);
    Simulator::ScheduleWithContext(m_sendSocket->GetNode()->GetId(),
                                   Seconds(1),
                                   &Bug772ChainTest::SendData,
                                   this,
                                   m_sendSocket);

    m_recvSocket = Socket::CreateSocket(m_nodes->Get(m_size - 1), TypeId::LookupByName(m_proto));
    m_recvSocket->Bind(InetSocketAddress(Ipv4Address::GetAny(), m_port));
    m_recvSocket->Listen();
    m_recvSocket->ShutdownSend();
    m_recvSocket->SetRecvCallback(MakeCallback(&Bug772ChainTest::HandleRead, this));
}

void
Bug772ChainTest::CheckResults()
{
    // We should have sent 8 packets (every 0.25 seconds from time 1 to time 3)
    // Check that the received packet count is 8
    NS_TEST_EXPECT_MSG_EQ(m_receivedPackets, 8, "Did not receive expected 8 packets");
}
