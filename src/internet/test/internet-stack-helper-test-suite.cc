/*
 * Copyright (c) 2023 Universita' di Firenze
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "ns3/internet-stack-helper.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/test.h"

#include <string>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("InternetStackHelperTestSuite");

/**
 * \ingroup internet-test
 *
 * \brief InternetStackHelper Test
 */
class InternetStackHelperTestCase : public TestCase
{
  public:
    InternetStackHelperTestCase();

  private:
    void DoRun() override;
    void DoTeardown() override;
};

InternetStackHelperTestCase::InternetStackHelperTestCase()
    : TestCase("InternetStackHelperTestCase")
{
}

void
InternetStackHelperTestCase::DoRun()
{
    // Checks:
    // 1. IPv4 only, add IPv4 + IPv6 (result, IPv4 + IPv6)
    // 2. IPv6 only, add IPv4 + IPv6 (result, IPv4 + IPv6)
    // 3. IPv4 + IPv6, add IPv4 + IPv6 (result, IPv4 + IPv6)

    Ptr<Node> nodeIpv4Only = CreateObject<Node>();
    Ptr<Node> nodeIpv6Only = CreateObject<Node>();
    Ptr<Node> nodeIpv46 = CreateObject<Node>();

    InternetStackHelper internet;

    internet.SetIpv4StackInstall(true);
    internet.SetIpv6StackInstall(false);
    internet.Install(nodeIpv4Only);

    internet.SetIpv4StackInstall(false);
    internet.SetIpv6StackInstall(true);
    internet.Install(nodeIpv6Only);

    internet.SetIpv4StackInstall(true);
    internet.SetIpv6StackInstall(true);
    internet.Install(nodeIpv46);

    // Check that the three nodes have only the intended IP stack.
    NS_TEST_EXPECT_MSG_NE(nodeIpv4Only->GetObject<Ipv4>(),
                          nullptr,
                          "IPv4 not found on IPv4-only node (should have been there)");
    NS_TEST_EXPECT_MSG_EQ(nodeIpv4Only->GetObject<Ipv6>(),
                          nullptr,
                          "IPv6 found on IPv4-only node (should not have been there)");

    NS_TEST_EXPECT_MSG_EQ(nodeIpv6Only->GetObject<Ipv4>(),
                          nullptr,
                          "IPv4 found on IPv6-only node (should not have been there)");
    NS_TEST_EXPECT_MSG_NE(nodeIpv6Only->GetObject<Ipv6>(),
                          nullptr,
                          "IPv6 not found on IPv6-only node (should have been there)");

    NS_TEST_EXPECT_MSG_NE(nodeIpv46->GetObject<Ipv4>(),
                          nullptr,
                          "IPv4 not found on dual stack node (should have been there)");
    NS_TEST_EXPECT_MSG_NE(nodeIpv46->GetObject<Ipv6>(),
                          nullptr,
                          "IPv6 not found on dual stack node (should have been there)");

    // Now we install IPv4 and IPv6 on the IPv4-only node
    // IPv4 is already there, no error should happen.
    internet.Install(nodeIpv4Only);
    // Now we install IPv4 and IPv6 on the IPv6-only node,
    // IPv6 is already there, no error should happen.
    internet.Install(nodeIpv6Only);
    // Now we install IPv4 and IPv6 on the dual stack node
    // IPv4 and IPv6 are already there, no error should happen.
    internet.Install(nodeIpv46);

    // Check that the three nodes have both IPv4 and IPv6.
    NS_TEST_EXPECT_MSG_NE(
        nodeIpv4Only->GetObject<Ipv4>(),
        nullptr,
        "IPv4 not found on IPv4-only, now dual stack node (should have been there)");
    NS_TEST_EXPECT_MSG_NE(
        nodeIpv4Only->GetObject<Ipv6>(),
        nullptr,
        "IPv6 not found on IPv4-only, now dual stack node (should have been there)");

    NS_TEST_EXPECT_MSG_NE(
        nodeIpv6Only->GetObject<Ipv4>(),
        nullptr,
        "IPv4 not found on IPv6-only, now dual stack node (should have been there)");
    NS_TEST_EXPECT_MSG_NE(
        nodeIpv6Only->GetObject<Ipv6>(),
        nullptr,
        "IPv6 not found on IPv6-only, now dual stack node (should have been there)");

    NS_TEST_EXPECT_MSG_NE(nodeIpv46->GetObject<Ipv4>(),
                          nullptr,
                          "IPv4 not found on dual stack node (should have been there)");
    NS_TEST_EXPECT_MSG_NE(nodeIpv46->GetObject<Ipv6>(),
                          nullptr,
                          "IPv6 not found on dual stack node (should have been there)");
}

void
InternetStackHelperTestCase::DoTeardown()
{
    Simulator::Destroy();
}

/**
 * \ingroup internet-test
 *
 * \brief InternetStackHelper TestSuite
 */
class InternetStackHelperTestSuite : public TestSuite
{
  public:
    InternetStackHelperTestSuite()
        : TestSuite("internet-stack-helper", UNIT)
    {
        AddTestCase(new InternetStackHelperTestCase(), TestCase::QUICK);
    }
};

static InternetStackHelperTestSuite
    g_internetStackHelperTestSuite; //!< Static variable for test initialization
