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
#include "ns3/peer-link.h"
#include "ns3/peer-management-protocol.h"
#include "ns3/random-variable-stream.h"
#include "ns3/rng-seed-manager.h"
#include "ns3/simulator.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/yans-wifi-helper.h"

#include <sstream>

using namespace ns3;

PeerManagementProtocolRegressionTest::PeerManagementProtocolRegressionTest()
    : TestCase("PMP regression test"),
      m_nodes(nullptr),
      m_time(Seconds(1)),
      m_linkClose(0)
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
    CheckResults();
    Simulator::Destroy();

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
    m_meshDevices = mesh.Install(wifiPhy, *m_nodes);
    // Two devices, 10 streams per device (one for mac, one for phy,
    // two for plugins, five for regular mac wifi DCF, and one for MeshPointDevice)
    streamsUsed += mesh.AssignStreams(m_meshDevices, 0);
    NS_TEST_ASSERT_MSG_EQ(streamsUsed, (m_meshDevices.GetN() * 10), "Stream assignment mismatch");
    streamsUsed += wifiChannel.AssignStreams(chan, streamsUsed);
    // 3. trace peer link events
    m_linkOpen.resize(m_meshDevices.GetN());
    for (uint32_t i = 0; i < m_meshDevices.GetN(); ++i)
    {
        auto pmp = m_meshDevices.Get(i)->GetObject<dot11s::PeerManagementProtocol>();
        NS_TEST_ASSERT_MSG_NE(pmp, nullptr, "No peer management protocol on mesh point " << i);
        pmp->TraceConnectWithoutContext(
            "LinkOpen",
            MakeCallback(&PeerManagementProtocolRegressionTest::LinkOpen, this).Bind(i));
        pmp->TraceConnectWithoutContext(
            "LinkClose",
            MakeCallback(&PeerManagementProtocolRegressionTest::LinkClose, this));
    }
}

void
PeerManagementProtocolRegressionTest::LinkOpen(uint32_t index,
                                               Mac48Address localIface,
                                               Mac48Address peerIface)
{
    m_linkOpen[index].emplace_back(localIface, peerIface);
}

void
PeerManagementProtocolRegressionTest::LinkClose(Mac48Address localIface, Mac48Address peerIface)
{
    m_linkClose++;
}

void
PeerManagementProtocolRegressionTest::CheckResults()
{
    for (uint32_t i = 0; i < 2; ++i)
    {
        auto self = Mac48Address::ConvertFrom(m_meshDevices.Get(i)->GetAddress());
        auto peer = Mac48Address::ConvertFrom(m_meshDevices.Get(1 - i)->GetAddress());

        NS_TEST_ASSERT_MSG_EQ(m_linkOpen[i].size(),
                              1,
                              "Mesh point " << i << " should open exactly one peer link");
        if (m_linkOpen[i].size() == 1)
        {
            NS_TEST_EXPECT_MSG_EQ(m_linkOpen[i][0].first, self, "Wrong local interface");
            NS_TEST_EXPECT_MSG_EQ(m_linkOpen[i][0].second, peer, "Wrong peer interface");
        }

        auto links =
            m_meshDevices.Get(i)->GetObject<dot11s::PeerManagementProtocol>()->GetPeerLinks();
        NS_TEST_ASSERT_MSG_EQ(links.size(),
                              1,
                              "Mesh point " << i << " should have one established peer link");
        if (links.size() == 1)
        {
            NS_TEST_EXPECT_MSG_EQ(links[0]->GetPeerAddress(), peer, "Peer link to wrong peer");
        }
    }
    NS_TEST_EXPECT_MSG_EQ(m_linkClose, 0, "No peer link should be closed");
}
