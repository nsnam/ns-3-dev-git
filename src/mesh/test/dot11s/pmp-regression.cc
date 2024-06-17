/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */
#include "pmp-regression.h"

#include "ns3/double.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/mesh-helper.h"
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

/// Unique PCAP file name prefix
const char* const PREFIX = "pmp-regression-test";

PeerManagementProtocolRegressionTest::PeerManagementProtocolRegressionTest()
    : TestCase("PMP regression test"),
      m_nodes(nullptr),
      m_time(Seconds(1))
{
}

PeerManagementProtocolRegressionTest::~PeerManagementProtocolRegressionTest()
{
    delete m_nodes;
}

void
PeerManagementProtocolRegressionTest::DoRun()
{
    RngSeedManager::SetSeed(1);
    RngSeedManager::SetRun(1);
    CreateNodes();
    CreateDevices();

    Simulator::Stop(m_time);
    Simulator::Run();
    Simulator::Destroy();

    CheckResults();

    delete m_nodes, m_nodes = nullptr;
}

void
PeerManagementProtocolRegressionTest::CreateNodes()
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
}

void
PeerManagementProtocolRegressionTest::CreateDevices()
{
    int64_t streamsUsed = 0;
    // 1. setup WiFi
    YansWifiPhyHelper wifiPhy;
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default();
    Ptr<YansWifiChannel> chan = wifiChannel.Create();
    wifiPhy.SetChannel(chan);
    // 2. setup mesh
    MeshHelper mesh = MeshHelper::Default();
    mesh.SetStackInstaller("ns3::Dot11sStack");
    mesh.SetMacType("RandomStart", TimeValue(Seconds(0.1)));
    mesh.SetNumberOfInterfaces(1);
    NetDeviceContainer meshDevices = mesh.Install(wifiPhy, *m_nodes);
    // Two devices, 10 streams per device (one for mac, one for phy,
    // two for plugins, five for regular mac wifi DCF, and one for MeshPointDevice)
    streamsUsed += mesh.AssignStreams(meshDevices, 0);
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (meshDevices.GetN() * 10), "Stream assignment mismatch");
    streamsUsed += wifiChannel.AssignStreams(chan, streamsUsed);
    // 3. write PCAP if needed
    wifiPhy.EnablePcapAll(CreateTempDirFilename(PREFIX));
}

void
PeerManagementProtocolRegressionTest::CheckResults()
{
    for (int i = 0; i < 2; ++i)
    {
        NS_PCAP_TEST_EXPECT_EQ(PREFIX << "-" << i << "-1.pcap");
    }
}
