/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 NITK Surathkal
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
 * Authors: Siddharth V <siddharthvdn@gmail.com>
 *          Kaushik S Kalmady <kaushikskalmady@gmail.com>
 *          Aneesh Aithal<aneesh297@gmail.com>
 */

#include "ns3/test.h"
#include "ns3/log.h"
#include "ns3/tcp-congestion-ops.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-compound.h"

#include "math.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("TcpCompoundTestSuite");

/**
 * \brief TcpCompound congestion control algorithm test
 */
class TcpCompoundTest : public TestCase
{
public:
  /**
   * \brief Constructor.
   * \param cWnd Congestion window.
   * \param lwnd Loss-based Congestion window.
   * \param dwnd Delay-based Congestion window.
   * \param segmentSize Segment size.
   * \param ssThresh Slow Start Threshold.
   * \param rtt The RTT.
   * \param segmentsAcked Number of segments ACKed.
   * \param nextTxSeq Next Tx sequence number.
   * \param lastAckedSeq Last ACKed sequence number.
   * \param name Test description.
   */
  TcpCompoundTest (uint32_t cWnd,
                   uint32_t lwnd,
                   uint32_t dwnd,
                   uint32_t segmentSize,
                   uint32_t ssThresh,
                   Time rtt,
                   uint32_t segmentsAcked,
                   SequenceNumber32 nextTxSeq,
                   SequenceNumber32 lastAckedSeq,
                   const std::string &name);

private:
  virtual void DoRun (void);
  /**
   * \brief Increases the TCP window.
   * \param cong The congestion control.
   */
  void IncreaseWindow (Ptr<TcpCompound> cong);
  /**
   * brief Get and check the SSH threshold.
   * \param cong The congestion control.
   */
  void GetSsThresh (Ptr<TcpCompound> cong);

  uint32_t m_cWnd;        //!< Congestion window.
  uint32_t m_lwnd;        //!< Loss-based Congestion window.
  uint32_t m_dwnd;        //!< Delay-based Congestion window.
  uint32_t m_segmentSize; //!< Segment size.
  uint32_t m_ssThresh;    //!< Slow Start Threshold.
  Time m_rtt;             //!< RTT.
  uint32_t m_segmentsAcked; //!< Number of segments ACKed.
  SequenceNumber32 m_nextTxSeq; //!< Next Tx sequence number.
  SequenceNumber32 m_lastAckedSeq;  //!< Last ACKed sequence number.

  Ptr<TcpSocketState> m_state;  //!< TCP socket state.
};

TcpCompoundTest::TcpCompoundTest (uint32_t cWnd,
                                  uint32_t lwnd,
                                  uint32_t dwnd,
                                  uint32_t segmentSize,
                                  uint32_t ssThresh,
                                  Time rtt,
                                  uint32_t segmentsAcked,
                                  SequenceNumber32 nextTxSeq,
                                  SequenceNumber32 lastAckedSeq,
                                  const std::string &name)
  : TestCase (name),
    m_cWnd (cWnd),
    m_lwnd (lwnd),
    m_dwnd (dwnd),
    m_segmentSize (segmentSize),
    m_ssThresh (ssThresh),
    m_rtt (rtt),
    m_segmentsAcked (segmentsAcked),
    m_nextTxSeq (nextTxSeq),
    m_lastAckedSeq (lastAckedSeq)
{
}

void
TcpCompoundTest::DoRun ()
{
  m_state = CreateObject<TcpSocketState> ();

  m_state->m_cWnd = m_cWnd;
  m_state->m_segmentSize = m_segmentSize;
  m_state->m_ssThresh = m_ssThresh;
  m_state->m_nextTxSequence = m_nextTxSeq;
  m_state->m_lastAckedSeq = m_lastAckedSeq;

  Ptr<TcpCompound> cong = CreateObject <TcpCompound> ();

  // Set baseRtt to 100 ms
  cong->PktsAcked (m_state, m_segmentsAcked, MilliSeconds (100));

  // Re-set Compound to assign a new value of minRtt
  cong->CongestionStateSet (m_state, TcpSocketState::CA_OPEN);
  cong->PktsAcked (m_state, m_segmentsAcked, m_rtt);

  // 2 more calls to PktsAcked to increment cntRtt beyond 2
  cong->PktsAcked (m_state, m_segmentsAcked, m_rtt);
  cong->PktsAcked (m_state, m_segmentsAcked, m_rtt);

  // Update cwnd using Compound algorithm
  cong->IncreaseWindow (m_state, m_segmentsAcked);

  // Our calculation of cwnd
  IncreaseWindow (cong);

  NS_TEST_ASSERT_MSG_EQ (m_state->m_cWnd.Get (), m_cWnd,
                         "CWnd has not updated correctly");
  NS_TEST_ASSERT_MSG_EQ (m_state->m_ssThresh.Get (), m_ssThresh,
                         "SsThresh has not updated correctly");
}

void
TcpCompoundTest::IncreaseWindow (Ptr<TcpCompound> cong)
{
  Time baseRtt = MilliSeconds (100);
  uint32_t segCwnd = m_cWnd / m_segmentSize;

  // Calculate expected throughput
  uint64_t expectedCwnd;
  expectedCwnd = (uint64_t) segCwnd * (double) baseRtt.GetMilliSeconds () / (double) m_rtt.GetMilliSeconds ();

  // Calculate the difference between actual and expected throughput
  uint32_t diff;
  diff = segCwnd - expectedCwnd;

  // Get the alpha,beta, k, eta and gamma attributes
  DoubleValue alpha, beta, k, eta;
  UintegerValue gamma;
  cong->GetAttribute ("Alpha", alpha);
  cong->GetAttribute ("Beta", beta);
  cong->GetAttribute ("k", k);
  cong->GetAttribute ("eta", eta);
  cong->GetAttribute ("Gamma", gamma);

  if ((m_cWnd < m_ssThresh) && (diff < gamma.Get ()))
    { // Execute Reno slow start
      if (m_segmentsAcked >= 1)
        {
          m_cWnd += m_segmentSize;
          m_segmentsAcked--;
        }
    }
  else if (diff < gamma.Get ())
    {
      m_cWnd += static_cast<uint32_t> (alpha.Get () * pow (m_cWnd, k.Get ()));

      double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd;
      adder = std::max (1.0, adder);
      m_lwnd += static_cast<uint32_t> (adder);

      m_dwnd = m_cWnd - m_lwnd;
    }
  else if (diff >= gamma.Get ())
    {
      double adder = static_cast<double> (m_segmentSize * m_segmentSize) / m_cWnd;
      adder = std::max (1.0, adder);

      m_lwnd += static_cast<uint32_t> (adder);

      uint32_t dwndInPackets = static_cast<uint32_t> (m_dwnd / m_segmentSize);
      dwndInPackets = static_cast<uint32_t> (std::max (0.0, static_cast<double> (dwndInPackets) - eta.Get () * diff));
      m_dwnd = dwndInPackets * m_segmentSize;

      m_cWnd = m_lwnd + m_dwnd;
    }
}

void
TcpCompoundTest::GetSsThresh (Ptr<TcpCompound> cong)
{
  DoubleValue beta;
  cong->GetAttribute ("Beta", beta);

  uint32_t new_window = (1 - beta.Get ()) * (m_cWnd);

  m_lwnd = m_lwnd / 2;

  m_dwnd = static_cast<uint32_t> (std::max (new_window - m_lwnd, static_cast<uint32_t> (0)));

  m_ssThresh = std::max (new_window, 2 * m_segmentSize);
}


/**
 * \ingroup internet-test
 * \ingroup tests
 *
 * \brief TCP Compound TestSuite
 */
class TcpCompoundTestSuite : public TestSuite
{
public:
  TcpCompoundTestSuite () : TestSuite ("tcp-compound-test", UNIT)
  {
    AddTestCase (new TcpCompoundTest (38 * 1446, 38 * 1446, 0, 1446, 40 * 1446, MilliSeconds (106), 1, SequenceNumber32 (2893), SequenceNumber32 (5785),
                                      "Compound test on cWnd and ssThresh when in slow start and diff > gamma"),
                 TestCase::QUICK);
    AddTestCase (new TcpCompoundTest (5 * 536, 5 * 536, 0, 536, 10 * 536, MilliSeconds (118), 1, SequenceNumber32 (3216), SequenceNumber32 (3753),
                                      "Compound test on cWnd and ssThresh when in slow start and diff < gamma"),
                 TestCase::QUICK);
    AddTestCase (new TcpCompoundTest (60 * 346, 60 * 346, 0, 346, 40 * 346, MilliSeconds (206), 1, SequenceNumber32 (20761), SequenceNumber32 (21107),
                                      "Compound test on cWnd and ssThresh when diff > beta"),
                 TestCase::QUICK);
    AddTestCase (new TcpCompoundTest (15 * 1446, 15 * 1446, 0, 1446, 10 * 1446, MilliSeconds (106), 1, SequenceNumber32 (21691), SequenceNumber32 (24583),
                                      "Compound test on cWnd and ssThresh when diff < alpha"),
                 TestCase::QUICK);
    AddTestCase (new TcpCompoundTest (20 * 746, 20 * 746, 0, 746, 10 * 746, MilliSeconds (109), 1, SequenceNumber32 (14921), SequenceNumber32 (15667),
                                      "Compound test on cWnd and ssThresh when alpha <= diff <= beta"),
                 TestCase::QUICK);
  }
};

static TcpCompoundTestSuite g_TcpCompoundTest; //!< Static variable for test initialization
