/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/test.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/simulator.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/point-to-point-channel.h"

using namespace ns3;

static const Time testStartTime = Seconds (1.0);
static const Time tolerance = MilliSeconds(5);

/**
 * \brief Test class for PointToPoint model
 *
 * It tries to send one packet from one NetDevice to another, over a
 * PointToPointChannel.
 */
class PointToPointTest : public TestCase
{
public:
  /**
   * \brief Create the test
   */
  PointToPointTest (Time forwardDelay, Time backwardDelay);

  /**
   * \brief Run the test
   */
  virtual void DoRun (void);



protected:

  Time m_forwardDelay;
  Time m_backwardDelay;

  Time m_packetArrival[2];

  Ptr<PointToPointNetDevice> m_devA;
  Ptr<PointToPointNetDevice> m_devB;

  /**
   * \brief Send one packet to the device specified
   *
   * \param device NetDevice to send to
   */
  void SendOnePacket (Ptr<PointToPointNetDevice> device);
  bool RecvOnePacket ( Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t protocol, const Address & sender);
};

PointToPointTest::PointToPointTest (Time forwardDelay, Time backwardDelay)
  : TestCase ("PointToPoint"),
    m_forwardDelay(forwardDelay),
    m_backwardDelay(backwardDelay)
{
}

void
PointToPointTest::SendOnePacket (Ptr<PointToPointNetDevice> device)
{
  Ptr<Packet> p = Create<Packet> ();
  device->Send (p, device->GetBroadcast (), 0x800);
}

bool
PointToPointTest::RecvOnePacket ( Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t protocol, const Address & sender)
{
//    if(dev == this->de)
    static int counter=0;
    m_packetArrival[counter] = Simulator::Now();

    if(counter==0) {
        Simulator::ScheduleNow( &PointToPointTest::SendOnePacket, this, m_devB);
    }

    counter++;
    return true;
}

void
PointToPointTest::DoRun (void)
{
  Ptr<Node> nodeA = CreateObject<Node> ();
  Ptr<Node> nodeB = CreateObject<Node> ();
  m_devA = CreateObject<PointToPointNetDevice> ();
  m_devB = CreateObject<PointToPointNetDevice> ();
  Ptr<PointToPointChannel> channel = CreateObject<PointToPointChannel> ();
  channel->SetAttribute("Delay", TimeValue(m_forwardDelay));
  channel->SetAttribute("AlternateDelay", TimeValue(m_backwardDelay));

  m_devA->Attach (channel);
  m_devA->SetAddress (Mac48Address::Allocate ());
  m_devA->SetQueue (CreateObject<DropTailQueue> ());
  m_devB->Attach (channel);
  m_devB->SetAddress (Mac48Address::Allocate ());
  m_devB->SetQueue (CreateObject<DropTailQueue> ());


  nodeA->AddDevice (m_devA);
  nodeB->AddDevice (m_devB);

  m_devA->SetReceiveCallback( MakeCallback(&PointToPointTest::RecvOnePacket, this) );
  m_devB->SetReceiveCallback( MakeCallback(&PointToPointTest::RecvOnePacket, this) );

  Simulator::Schedule (testStartTime, &PointToPointTest::SendOnePacket, this, m_devA);

//  NS_TEST_ASSERT_MSG_EQ(true, false, "toto");
  Simulator::Run ();

  Simulator::Destroy ();

  std::cout << "1st packet arrival=" << m_packetArrival[0].As(Time::MS)
            << " (=" << (testStartTime + m_forwardDelay).As(Time::MS) << " ?)"
            << std::endl;
  std::cout << "2nd packet arrival=" << m_packetArrival[1].As(Time::MS)
            << " to compare with :" << testStartTime + m_forwardDelay + m_backwardDelay
            << std::endl;

  NS_TEST_ASSERT_MSG_EQ_TOL( m_packetArrival[0], testStartTime + m_forwardDelay, tolerance, "Forward delay out of bounds");
  NS_TEST_ASSERT_MSG_EQ_TOL( m_packetArrival[1], testStartTime + m_forwardDelay + m_backwardDelay, tolerance, "Backward delay out of bounds");

}

/**
 * \brief TestSuite for PointToPoint module
 */
class PointToPointTestSuite : public TestSuite
{
public:
  /**
   * \brief Constructor
   */
  PointToPointTestSuite ();
};

PointToPointTestSuite::PointToPointTestSuite ()
  : TestSuite ("devices-point-to-point", UNIT)
{
  AddTestCase (new PointToPointTest(MilliSeconds(50), MilliSeconds(150)), TestCase::QUICK);
}

static PointToPointTestSuite g_pointToPointTestSuite; //!< The testsuite
