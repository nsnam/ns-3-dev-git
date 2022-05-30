/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2022 Tokushima University, Japan
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
 * Author: Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/packet.h>

#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lr-wpan-mac-test");
/**
 * \ingroup lr-wpan-test
 * \ingroup tests
 *
 * \brief Test PHY going to TRX_OFF after CSMA failure (MAC->RxOnWhenIdle(false))
 */
class TestRxOffWhenIdleAfterCsmaFailure : public TestCase
{
public:
  TestRxOffWhenIdleAfterCsmaFailure ();
  virtual ~TestRxOffWhenIdleAfterCsmaFailure ();

private:

  /**
   * Function called when a Data indication is invoked
   * \param params MCPS data indication parameters
   * \param p packet
   */
  void DataIndication (McpsDataIndicationParams params, Ptr<Packet> p);
  /**
   * Function called when a Data confirm is invoked (After Tx Attempt)
   * \param params MCPS data confirm parameters
   */
  void DataConfirm (McpsDataConfirmParams params);
  /**
   * Function called when a the PHY state changes in Dev0 [00:01]
   * \param context context
   * \param now time at which the function is called
   * \param oldState old PHY state
   * \param newState new PHY state
   */
  void StateChangeNotificationDev0 (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState);
  /**
   * Function called when a the PHY state changes in Dev2 [00:03]
   * \param context context
   * \param now time at which the function is called
   * \param oldState old PHY state
   * \param newState new PHY state
   */
  void StateChangeNotificationDev2 (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState);

  virtual void DoRun (void);

  LrWpanPhyEnumeration m_dev0State; //!< Stores the PHY state of device 0 [00:01]

};


TestRxOffWhenIdleAfterCsmaFailure::TestRxOffWhenIdleAfterCsmaFailure ()
  : TestCase ("Test PHY going to TRX_OFF state after CSMA failure")
{}

TestRxOffWhenIdleAfterCsmaFailure::~TestRxOffWhenIdleAfterCsmaFailure ()
{}

void
TestRxOffWhenIdleAfterCsmaFailure::DataIndication (McpsDataIndicationParams params, Ptr<Packet> p)
{
  NS_LOG_DEBUG ("Received packet of size " << p->GetSize ());
}

void
TestRxOffWhenIdleAfterCsmaFailure::DataConfirm (McpsDataConfirmParams params)
{
  if (params.m_status == LrWpanMcpsDataConfirmStatus::IEEE_802_15_4_SUCCESS)
    {
      NS_LOG_DEBUG ("LrWpanMcpsDataConfirmStatus = Success");
    }
  else if (params.m_status == LrWpanMcpsDataConfirmStatus::IEEE_802_15_4_CHANNEL_ACCESS_FAILURE)
    {
      NS_LOG_DEBUG ("LrWpanMcpsDataConfirmStatus =  Channel Access Failure");
    }
}

void
TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev0 (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState)
{
  NS_LOG_DEBUG (Simulator::Now ().As (Time::S) << context << "PHY state change at " << now.As (Time::S)
                                                << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter (oldState)
                                                << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter (newState));

  m_dev0State = newState;
}

void
TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev2 (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState)
{
  NS_LOG_DEBUG (Simulator::Now ().As (Time::S) << context << "PHY state change at " << now.As (Time::S)
                                                << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter (oldState)
                                                << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter (newState));
}

void
TestRxOffWhenIdleAfterCsmaFailure::DoRun ()
{
  //  [00:01]      [00:02]     [00:03]
  //   Node 0------>Node1<------Node2 (interferer)
  //
  // Test Setup:
  //
  // Start the test with a transmission from node 3 to node 1,
  // soon after, node 0 will attempt to transmit a packet to node 1 as well but
  // it will fail because node 2 is still transmitting.
  //
  // The test confirms that the PHY in node 0 goes to TRX_OFF
  // after its CSMA failure because its MAC has been previously
  // set with flag RxOnWhenIdle (false). To ensure that node 0 CSMA
  // do not attempt to do multiple backoffs delays in its CSMA,
  // macMinBE and MacMaxCSMABackoffs has been set to 0.

  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_FUNC);
  LogComponentEnable ("LrWpanMac", LOG_LEVEL_DEBUG);
  LogComponentEnable ("LrWpanCsmaCa", LOG_LEVEL_DEBUG);

  // Create 3 nodes, and a NetDevice for each one
  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  Ptr<Node> interferenceNode = CreateObject <Node> ();


  Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
  Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice> ();
  Ptr<LrWpanNetDevice> dev2 = CreateObject<LrWpanNetDevice> ();


  dev0->SetAddress (Mac16Address ("00:01"));
  dev1->SetAddress (Mac16Address ("00:02"));
  dev2->SetAddress (Mac16Address ("00:03"));

  // Each device must be attached to the same channel
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  dev2->SetChannel (channel);

  // To complete configuration, a LrWpanNetDevice must be added to a node
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);
  interferenceNode->AddDevice (dev2);

  // Trace state changes in the phy
  dev0->GetPhy ()->TraceConnect ("TrxState", std::string ("[address 00:01]"),
                                 MakeCallback (&TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev0,this));
  dev2->GetPhy ()->TraceConnect ("TrxState", std::string ("[address 00:03]"),
                                 MakeCallback (&TestRxOffWhenIdleAfterCsmaFailure::StateChangeNotificationDev2,this));

  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,0,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);
  Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();

  sender1Mobility->SetPosition (Vector (0,1,0));
  dev1->GetPhy ()->SetMobility (sender1Mobility);

  Ptr<ConstantPositionMobilityModel> sender3Mobility = CreateObject<ConstantPositionMobilityModel> ();

  sender3Mobility->SetPosition (Vector (0,2,0));
  dev2->GetPhy ()->SetMobility (sender3Mobility);

  McpsDataConfirmCallback cb0;
  cb0 = MakeCallback (&TestRxOffWhenIdleAfterCsmaFailure::DataConfirm,this);
  dev0->GetMac ()->SetMcpsDataConfirmCallback (cb0);

  McpsDataIndicationCallback cb1;
  cb1 = MakeCallback (&TestRxOffWhenIdleAfterCsmaFailure::DataIndication,this);
  dev0->GetMac ()->SetMcpsDataIndicationCallback (cb1);

  McpsDataConfirmCallback cb2;
  cb2 = MakeCallback (&TestRxOffWhenIdleAfterCsmaFailure::DataConfirm,this);
  dev1->GetMac ()->SetMcpsDataConfirmCallback (cb2);

  McpsDataIndicationCallback cb3;
  cb3 = MakeCallback (&TestRxOffWhenIdleAfterCsmaFailure::DataIndication,this);
  dev1->GetMac ()->SetMcpsDataIndicationCallback (cb3);

  dev0->GetMac ()->SetRxOnWhenIdle (false);
  dev1->GetMac ()->SetRxOnWhenIdle (false);


  // set CSMA min beacon exponent (BE) to 0 to make all backoff delays == to 0 secs.
  dev0->GetCsmaCa ()->SetMacMinBE (0);
  dev2->GetCsmaCa ()->SetMacMinBE (0);

  // Only try once to do a backoff period before giving up.
  dev0->GetCsmaCa ()->SetMacMaxCSMABackoffs (0);
  dev2->GetCsmaCa ()->SetMacMaxCSMABackoffs (0);



  // The below should trigger two callbacks when end-to-end data is working
  // 1) DataConfirm callback is called
  // 2) DataIndication callback is called with value of 50
  Ptr<Packet> p0 = Create<Packet> (50);  // 50 bytes of dummy data
  McpsDataRequestParams params;
  params.m_dstPanId = 0;

  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstAddr = Mac16Address ("00:02");

  params.m_msduHandle = 0;

  // Send the packet that will be rejected due to channel access failure
  Simulator::ScheduleWithContext (1, Seconds (0.00033),
                                  &LrWpanMac::McpsDataRequest,
                                  dev0->GetMac (), params, p0);

  // Send interference packet
  Ptr<Packet> p2 = Create<Packet> (60);  // 60 bytes of dummy data
  params.m_dstAddr = Mac16Address ("00:02");

  Simulator::ScheduleWithContext (2, Seconds (0.0),
                                  &LrWpanMac::McpsDataRequest,
                                  dev2->GetMac (), params, p2);


  Simulator::Run ();

  NS_TEST_EXPECT_MSG_EQ (m_dev0State, LrWpanPhyEnumeration::IEEE_802_15_4_PHY_TRX_OFF,
                         "Error, dev0 [00:01] PHY should be in TRX_OFF after CSMA failure");

  Simulator::Destroy ();

}


/**
 * \ingroup lr-wpan-test
 * \ingroup tests
 *
 * \brief LrWpan MAC TestSuite
 */
class LrWpanMacTestSuite : public TestSuite
{
public:
  LrWpanMacTestSuite ();
};

LrWpanMacTestSuite::LrWpanMacTestSuite ()
  : TestSuite ("lr-wpan-mac-test", UNIT)
{
  AddTestCase (new TestRxOffWhenIdleAfterCsmaFailure, TestCase::QUICK);
}


static LrWpanMacTestSuite g_lrWpanMacTestSuite; //!< Static variable for test initialization
