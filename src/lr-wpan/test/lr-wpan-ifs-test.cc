/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Ritsumeikan University, Shiga, Japan
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
 * Author:
 *  Alberto Gallegos Ramonet <ramonet@fc.ritsumei.ac.jp>
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
#include "ns3/rng-seed-manager.h"

#include <iostream>
#include <iomanip>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("lr-wpan-ifs-test");

/**
 * \ingroup lr-wpan-test
 * \ingroup tests
 *
 * \brief LrWpan Dataframe transmission with Interframe Space
 */
class LrWpanDataIfsTestCase : public TestCase
{
public:
  LrWpanDataIfsTestCase ();
  virtual ~LrWpanDataIfsTestCase ();



private:
  static void DataConfirm (LrWpanDataIfsTestCase *testcase,
                           Ptr<LrWpanNetDevice> dev,
                           McpsDataConfirmParams params);

  static void DataReceived (LrWpanDataIfsTestCase *testcase,
                            Ptr<LrWpanNetDevice> dev,
                            Ptr<const Packet>);

  static void MacState (LrWpanDataIfsTestCase *testcase,
                        Ptr<LrWpanNetDevice> dev,
                        LrWpanMacState oldValue,
                        LrWpanMacState newValue);



  virtual void DoRun (void);
  Time m_lastTxTime; //!< The time of the last transmitted packet
  Time m_ackRxTime; //!< The time of the received acknoledgment.
  Time m_endIfs; //!< The time where the Interframe Space ended.


};


LrWpanDataIfsTestCase::LrWpanDataIfsTestCase ()
  : TestCase ("Lrwpan: IFS with and without ACK")
{

}

LrWpanDataIfsTestCase::~LrWpanDataIfsTestCase ()
{

}

void
LrWpanDataIfsTestCase::DataConfirm (LrWpanDataIfsTestCase *testcase, Ptr<LrWpanNetDevice> dev, McpsDataConfirmParams params)
{
  std::cout << Simulator::Now ().GetSeconds () << " | Dataframe Sent\n";
  testcase->m_lastTxTime = Simulator::Now ();

}

void
LrWpanDataIfsTestCase::DataReceived (LrWpanDataIfsTestCase *testcase,Ptr<LrWpanNetDevice> dev, Ptr<const Packet> p)
{
  Ptr<Packet> RxPacket = p->Copy ();
  LrWpanMacHeader receivedMacHdr;
  RxPacket->RemoveHeader (receivedMacHdr);

  NS_ASSERT (receivedMacHdr.IsAcknowledgment ());
  testcase->m_ackRxTime = Simulator::Now ();

  std::cout << Simulator::Now ().GetSeconds () << " | ACK received\n";
}

void
LrWpanDataIfsTestCase::MacState (LrWpanDataIfsTestCase *testcase, Ptr<LrWpanNetDevice> dev,LrWpanMacState oldValue, LrWpanMacState newValue)
{
  // Check the time after the MAC layer go back to IDLE state
  // (i.e. after the packet has been sent and the IFS is finished)

  if (newValue == LrWpanMacState::MAC_IDLE)
    {
      testcase->m_endIfs = Simulator::Now ();
      std::cout << Simulator::Now ().GetSeconds () << " | MAC layer is free\n";
    }

}

void
LrWpanDataIfsTestCase::DoRun ()
{
  // Test of Interframe Spaces (IFS)

  // The MAC layer needs a finite amount of time to process the data received from the PHY.
  // To allow this, to successive transmitted frames must be separated for at least one IFS.
  // The IFS size depends on the transmitted frame. This test verifies that the IFS is correctly
  // implemented and its size correspond to the situations described by the standard.
  // For more info see IEEE 802.15.4-2011 Section 5.1.1.3


  // Create 2 nodes, and a NetDevice for each one
  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();

  Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
  Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice> ();

  dev0->SetAddress (Mac16Address ("00:01"));
  dev1->SetAddress (Mac16Address ("00:02"));

  // Each device must be attached to the same channel
  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  dev0->SetChannel (channel);
  dev1->SetChannel (channel);

  // To complete configuration, a LrWpanNetDevice must be added to a node
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);

  // Connect to trace files in the MAC layer
  dev0->GetMac ()->TraceConnectWithoutContext ("MacStateValue", MakeBoundCallback (&LrWpanDataIfsTestCase::MacState, this, dev0));
  dev0->GetMac ()->TraceConnectWithoutContext ("MacRx", MakeBoundCallback (&LrWpanDataIfsTestCase::DataReceived, this, dev0));

  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,0,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);
  Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  // Configure position 10 m distance
  sender1Mobility->SetPosition (Vector (0,10,0));
  dev1->GetPhy ()->SetMobility (sender1Mobility);

  McpsDataConfirmCallback cb0;
  cb0 = MakeBoundCallback (&LrWpanDataIfsTestCase::DataConfirm, this, dev0);
  dev0->GetMac ()->SetMcpsDataConfirmCallback (cb0);

  Ptr<Packet> p0 = Create<Packet> (2);
  McpsDataRequestParams params;
  params.m_dstPanId = 0;

  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstAddr = Mac16Address ("00:02");
  params.m_msduHandle = 0;

  Time ifsSize;

  ////////////////////////  SIFS ///////////////////////////

  Simulator::ScheduleWithContext (1, Seconds (0.0),
                                  &LrWpanMac::McpsDataRequest,
                                  dev0->GetMac (), params, p0);


  Simulator::Run ();

  // MPDU = MAC header (11 bytes) + MSDU (2 bytes)+ MAC trailer (2 bytes)  = 15)
  // MPDU (15 bytes) < 18 bytes therefore IFS = SIFS
  // SIFS = 12 symbols (192 Microseconds on a 2.4Ghz O-QPSK PHY)
  ifsSize = m_endIfs - m_lastTxTime;
  NS_TEST_EXPECT_MSG_EQ (ifsSize, Time (MicroSeconds (192)), "Wrong Short InterFrame Space (SIFS) Size after dataframe Tx");

  ////////////////////////  LIFS ///////////////////////////

  p0 = Create<Packet> (6);

  Simulator::ScheduleWithContext (1, Seconds (0.0),
                                  &LrWpanMac::McpsDataRequest,
                                  dev0->GetMac (), params, p0);


  Simulator::Run ();

  // MPDU = MAC header (11 bytes) + MSDU (6 bytes)+ MAC trailer (2 bytes)  = 19)
  // MPDU (19 bytes) > 18 bytes therefore IFS = LIFS
  // LIFS = 20 symbols (640 Microseconds on a 2.4Ghz O-QPSK PHY)
  ifsSize = m_endIfs - m_lastTxTime;
  NS_TEST_EXPECT_MSG_EQ (ifsSize, Time (MicroSeconds (640)), "Wrong Long InterFrame Space (LIFS) Size after dataframe Tx");

  //////////////////////// SIFS after ACK //////////////////

  params.m_txOptions = TX_OPTION_ACK;
  p0 = Create<Packet> (2);

  Simulator::ScheduleWithContext (1, Seconds (0.0),
                                  &LrWpanMac::McpsDataRequest,
                                  dev0->GetMac (), params, p0);

  Simulator::Run ();

  // MPDU = MAC header (11 bytes) + MSDU (2 bytes)+ MAC trailer (2 bytes)  = 15)
  // MPDU (15 bytes) < 18 bytes therefore IFS = SIFS
  // SIFS = 12 symbols (192 Microseconds on a 2.4Ghz O-QPSK PHY)
  ifsSize = m_endIfs - m_ackRxTime;
  NS_TEST_EXPECT_MSG_EQ (ifsSize, Time (MicroSeconds (192)), "Wrong Short InterFrame Space (SIFS) Size after ACK Rx");

  ////////////////////////  LIFS after ACK //////////////////

  params.m_txOptions = TX_OPTION_ACK;
  p0 = Create<Packet> (6);

  Simulator::ScheduleWithContext (1, Seconds (0.0),
                                  &LrWpanMac::McpsDataRequest,
                                  dev0->GetMac (), params, p0);


  Simulator::Run ();

  // MPDU = MAC header (11 bytes) + MSDU (6 bytes)+ MAC trailer (2 bytes)  = 19)
  // MPDU (19 bytes) > 18 bytes therefore IFS = LIFS
  // LIFS = 20 symbols (640 Microseconds on a 2.4Ghz O-QPSK PHY)
  ifsSize = m_endIfs - m_ackRxTime;
  NS_TEST_EXPECT_MSG_EQ (ifsSize, Time (MicroSeconds (640)), "Wrong Long InterFrame Space (LIFS) Size after ACK Rx");

  Simulator::Destroy ();

}


/**
 * \ingroup lr-wpan-test
 * \ingroup tests
 *
 * \brief LrWpan IFS TestSuite
 */

class LrWpanIfsTestSuite : public TestSuite
{
public:
  LrWpanIfsTestSuite ();
};

LrWpanIfsTestSuite::LrWpanIfsTestSuite ()
  : TestSuite ("lr-wpan-ifs-test", UNIT)
{
  AddTestCase (new LrWpanDataIfsTestCase, TestCase::QUICK);
}

static LrWpanIfsTestSuite lrWpanIfsTestSuite; //!< Static variable for test initialization




