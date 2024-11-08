/*
 * Copyright (c) 2010 Lalith Suresh
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Lalith Suresh <suresh.lalith@gmail.com>
 */

#include "ns3/click-internet-stack-helper.h"
#include "ns3/ipv4-click-routing.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/simple-net-device.h"
#include "ns3/test.h"

#include <click/simclick.h>
#include <string>

using namespace ns3;

/**
 * @ingroup click
 * @defgroup click-tests click module tests
 */

/**
 * @file
 * @ingroup click-tests
 * Click test suite.
 */

/**
 * @ingroup click-tests
 * Add Click Internet stack.
 *
 * @param node Node.
 */
static void
AddClickInternetStack(Ptr<Node> node)
{
    ClickInternetStackHelper internet;
    internet.SetClickFile(node, "src/click/test/nsclick-test-lan-single-interface.click");
    internet.Install(node);
}

/**
 * @ingroup click-tests
 * Add network device.
 *
 * @param node Node.
 * @param macaddr MAC address.
 * @param ipv4addr IPv4 address.
 * @param ipv4mask IPv4 mask.
 */
static void
AddNetworkDevice(Ptr<Node> node, Mac48Address macaddr, Ipv4Address ipv4addr, Ipv4Mask ipv4mask)
{
    Ptr<SimpleNetDevice> rxDev1;

    rxDev1 = CreateObject<SimpleNetDevice>();
    rxDev1->SetAddress(Mac48Address(macaddr));
    node->AddDevice(rxDev1);

    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    uint32_t netdev_idx = ipv4->AddInterface(rxDev1);
    Ipv4InterfaceAddress ipv4Addr = Ipv4InterfaceAddress(ipv4addr, ipv4mask);
    ipv4->AddAddress(netdev_idx, ipv4Addr);
    ipv4->SetUp(netdev_idx);
}

/**
 * @ingroup click-tests
 * Click interface ID from name test.
 */
class ClickIfidFromNameTest : public TestCase
{
  public:
    ClickIfidFromNameTest();
    void DoRun() override;
};

ClickIfidFromNameTest::ClickIfidFromNameTest()
    : TestCase("Test SIMCLICK_IFID_FROM_NAME")
{
}

void
ClickIfidFromNameTest::DoRun()
{
    Ptr<Node> node = CreateObject<Node>();
    AddClickInternetStack(node);
    AddNetworkDevice(node,
                     Mac48Address("00:00:00:00:00:01"),
                     Ipv4Address("10.1.1.1"),
                     Ipv4Mask("255.255.255.0"));
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ptr<Ipv4ClickRouting> click = DynamicCast<Ipv4ClickRouting>(ipv4->GetRoutingProtocol());
    click->DoInitialize();

    int ret;

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IFID_FROM_NAME, "tap0");
    NS_TEST_EXPECT_MSG_EQ(ret, 0, "tap0 is interface 0");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IFID_FROM_NAME, "tun0");
    NS_TEST_EXPECT_MSG_EQ(ret, 0, "tun0 is interface 0");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IFID_FROM_NAME, "eth0");
    NS_TEST_EXPECT_MSG_EQ(ret, 1, "Eth0 is interface 1");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IFID_FROM_NAME, "tap1");
    NS_TEST_EXPECT_MSG_EQ(ret, 0, "tap1 is interface 0");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IFID_FROM_NAME, "tun1");
    NS_TEST_EXPECT_MSG_EQ(ret, 0, "tun1 is interface 0");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IFID_FROM_NAME, "eth1");
    NS_TEST_EXPECT_MSG_EQ(ret, -1, "No eth1 on node");
}

/**
 * @ingroup click-tests
 * Click IP MAC address from name test.
 */
class ClickIpMacAddressFromNameTest : public TestCase
{
  public:
    ClickIpMacAddressFromNameTest();
    void DoRun() override;
};

ClickIpMacAddressFromNameTest::ClickIpMacAddressFromNameTest()
    : TestCase("Test SIMCLICK_IPADDR_FROM_NAME")
{
}

void
ClickIpMacAddressFromNameTest::DoRun()
{
    Ptr<Node> node = CreateObject<Node>();
    AddClickInternetStack(node);
    AddNetworkDevice(node,
                     Mac48Address("00:00:00:00:00:01"),
                     Ipv4Address("10.1.1.1"),
                     Ipv4Mask("255.255.255.0"));
    AddNetworkDevice(node,
                     Mac48Address("00:00:00:00:00:02"),
                     Ipv4Address("10.1.1.2"),
                     Ipv4Mask("255.255.255.0"));
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ptr<Ipv4ClickRouting> click = DynamicCast<Ipv4ClickRouting>(ipv4->GetRoutingProtocol());
    click->DoInitialize();

    char* buf = new char[255];

    simclick_sim_command(click->m_simNode, SIMCLICK_IPADDR_FROM_NAME, "eth0", buf, 255);
    NS_TEST_EXPECT_MSG_EQ(std::string(buf), "10.1.1.1", "eth0 has IP 10.1.1.1");

    simclick_sim_command(click->m_simNode, SIMCLICK_MACADDR_FROM_NAME, "eth0", buf, 255);
    NS_TEST_EXPECT_MSG_EQ(std::string(buf),
                          "00:00:00:00:00:01",
                          "eth0 has Mac Address 00:00:00:00:00:01");

    simclick_sim_command(click->m_simNode, SIMCLICK_IPADDR_FROM_NAME, "eth1", buf, 255);
    NS_TEST_EXPECT_MSG_EQ(std::string(buf), "10.1.1.2", "eth1 has IP 10.1.1.2");

    simclick_sim_command(click->m_simNode, SIMCLICK_MACADDR_FROM_NAME, "eth1", buf, 255);
    NS_TEST_EXPECT_MSG_EQ(std::string(buf),
                          "00:00:00:00:00:02",
                          "eth0 has Mac Address 00:00:00:00:00:02");

    // Not sure how to test the below case, because the Ipv4ClickRouting code is to ASSERT for such
    // inputs simclick_sim_command (click->m_simNode, SIMCLICK_IPADDR_FROM_NAME, "eth2", buf, 255);
    // NS_TEST_EXPECT_MSG_EQ (buf, nullptr, "No eth2");

    simclick_sim_command(click->m_simNode, SIMCLICK_IPADDR_FROM_NAME, "tap0", buf, 255);
    NS_TEST_EXPECT_MSG_EQ(std::string(buf), "127.0.0.1", "tun0 has IP 127.0.0.1");

    simclick_sim_command(click->m_simNode, SIMCLICK_MACADDR_FROM_NAME, "tap0", buf, 255);
    NS_TEST_EXPECT_MSG_EQ(std::string(buf), "00:00:00:00:00:00", "tun0 has IP 127.0.0.1");

    delete[] buf;
}

/**
 * @ingroup click-tests
 * Click trivial test.
 */
class ClickTrivialTest : public TestCase
{
  public:
    ClickTrivialTest();
    void DoRun() override;
};

ClickTrivialTest::ClickTrivialTest()
    : TestCase("Test SIMCLICK_GET_NODE_NAME and SIMCLICK_IF_READY")
{
}

void
ClickTrivialTest::DoRun()
{
    Ptr<Node> node = CreateObject<Node>();
    AddClickInternetStack(node);
    AddNetworkDevice(node,
                     Mac48Address("00:00:00:00:00:01"),
                     Ipv4Address("10.1.1.1"),
                     Ipv4Mask("255.255.255.0"));
    Ptr<Ipv4> ipv4 = node->GetObject<Ipv4>();
    Ptr<Ipv4ClickRouting> click = DynamicCast<Ipv4ClickRouting>(ipv4->GetRoutingProtocol());
    click->SetNodeName("myNode");
    click->DoInitialize();

    int ret = 0;
    char* buf = new char[255];

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_GET_NODE_NAME, buf, 255);
    NS_TEST_EXPECT_MSG_EQ(std::string(buf), "myNode", "Node name is Node");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IF_READY, 0);
    NS_TEST_EXPECT_MSG_EQ(ret, 1, "tap0 is ready");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IF_READY, 1);
    NS_TEST_EXPECT_MSG_EQ(ret, 1, "eth0 is ready");

    ret = simclick_sim_command(click->m_simNode, SIMCLICK_IF_READY, 2);
    NS_TEST_EXPECT_MSG_EQ(ret, 0, "eth1 does not exist, so return 0");

    delete[] buf;
}

/**
 * @ingroup click-tests
 * Click interface ID from name test.
 */
class ClickIfidFromNameTestSuite : public TestSuite
{
  public:
    ClickIfidFromNameTestSuite()
        : TestSuite("routing-click", Type::UNIT)
    {
        AddTestCase(new ClickTrivialTest, TestCase::Duration::QUICK);
        AddTestCase(new ClickIfidFromNameTest, TestCase::Duration::QUICK);
        AddTestCase(new ClickIpMacAddressFromNameTest, TestCase::Duration::QUICK);
    }
};

/// Static variable for test initialization
static ClickIfidFromNameTestSuite g_ipv4ClickRoutingTestSuite;
