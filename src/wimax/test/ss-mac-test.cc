/*
 *  Copyright (c) 2007,2008, 2009 INRIA, UDcast
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
 * Author: Mohamed Amine Ismail <amine.ismail@sophia.inria.fr>
 *                              <amine.ismail@udcast.com>
 */
#include "ns3/log.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/simulator.h"
#include "ns3/test.h"
#include "ns3/wimax-helper.h"

using namespace ns3;

/**
 * \ingroup wimax-test
 * \ingroup tests
 *
 * \brief Test the network entry procedure.
 * Create a network with a BS and 10 SS and check that all the SS perform the
 * network entry correctly
 *
 */
class Ns3WimaxNetworkEntryTestCase : public TestCase
{
  public:
    Ns3WimaxNetworkEntryTestCase();
    ~Ns3WimaxNetworkEntryTestCase() override;

  private:
    void DoRun() override;
};

Ns3WimaxNetworkEntryTestCase::Ns3WimaxNetworkEntryTestCase()
    : TestCase("Test the network entry procedure")
{
}

Ns3WimaxNetworkEntryTestCase::~Ns3WimaxNetworkEntryTestCase()
{
}

void
Ns3WimaxNetworkEntryTestCase::DoRun()
{
    WimaxHelper::SchedulerType scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
    NodeContainer ssNodes;
    NodeContainer bsNodes;

    ssNodes.Create(10);
    bsNodes.Create(1);

    WimaxHelper wimax;

    NetDeviceContainer ssDevs;
    NetDeviceContainer bsDevs;

    ssDevs = wimax.Install(ssNodes,
                           WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION,
                           WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                           scheduler);
    bsDevs = wimax.Install(bsNodes,
                           WimaxHelper::DEVICE_TYPE_BASE_STATION,
                           WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                           scheduler);
    Simulator::Stop(Seconds(1));
    Simulator::Run();
    for (int i = 0; i < 10; i++)
    {
        NS_TEST_EXPECT_MSG_EQ(
            ssDevs.Get(i)->GetObject<SubscriberStationNetDevice>()->IsRegistered(),
            true,
            "SS[" << i << "] IsNotRegistered");
    }
    Simulator::Destroy();
}

/**
 * \ingroup wimax-test
 * \ingroup tests
 *
 * \brief Test if the management connections are correctly setup.
 * Create a network with a BS and 10 SS and check that the management
 * connections are correctly setup for all SS
 *
 */
class Ns3WimaxManagementConnectionsTestCase : public TestCase
{
  public:
    Ns3WimaxManagementConnectionsTestCase();
    ~Ns3WimaxManagementConnectionsTestCase() override;

  private:
    void DoRun() override;
};

Ns3WimaxManagementConnectionsTestCase::Ns3WimaxManagementConnectionsTestCase()
    : TestCase("Test if the management connections are correctly setup")
{
}

Ns3WimaxManagementConnectionsTestCase::~Ns3WimaxManagementConnectionsTestCase()
{
}

void
Ns3WimaxManagementConnectionsTestCase::DoRun()
{
    WimaxHelper::SchedulerType scheduler = WimaxHelper::SCHED_TYPE_SIMPLE;
    NodeContainer ssNodes;
    NodeContainer bsNodes;

    ssNodes.Create(10);
    bsNodes.Create(1);

    WimaxHelper wimax;

    NetDeviceContainer ssDevs;
    NetDeviceContainer bsDevs;

    ssDevs = wimax.Install(ssNodes,
                           WimaxHelper::DEVICE_TYPE_SUBSCRIBER_STATION,
                           WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                           scheduler);
    bsDevs = wimax.Install(bsNodes,
                           WimaxHelper::DEVICE_TYPE_BASE_STATION,
                           WimaxHelper::SIMPLE_PHY_TYPE_OFDM,
                           scheduler);
    Simulator::Stop(Seconds(1));
    Simulator::Run();
    for (int i = 0; i < 10; i++)
    {
        NS_TEST_EXPECT_MSG_EQ(ssDevs.Get(i)
                                  ->GetObject<SubscriberStationNetDevice>()
                                  ->GetAreManagementConnectionsAllocated(),
                              true,
                              "Management connections for SS[" << i << "] are not allocated");
    }
    Simulator::Destroy();
}

/**
 * \ingroup wimax-test
 * \ingroup tests
 *
 * \brief Ns3 Wimax SS Mac Test Suite
 */
class Ns3WimaxSSMacTestSuite : public TestSuite
{
  public:
    Ns3WimaxSSMacTestSuite();
};

Ns3WimaxSSMacTestSuite::Ns3WimaxSSMacTestSuite()
    : TestSuite("wimax-ss-mac-layer", Type::UNIT)
{
    AddTestCase(new Ns3WimaxNetworkEntryTestCase, TestCase::Duration::QUICK);
    AddTestCase(new Ns3WimaxManagementConnectionsTestCase, TestCase::Duration::QUICK);
}

static Ns3WimaxSSMacTestSuite ns3WimaxSSMacTestSuite; ///< the test suite
