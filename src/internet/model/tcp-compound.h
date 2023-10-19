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

#ifndef TCPCompound_H
#define TCPCompound_H

#include "ns3/tcp-congestion-ops.h"

namespace ns3 {

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of TCP Compound
 *
 * TCP Compound is a pure delay-based congestion control algorithm implementing a proactive
 * scheme that tries to prevent packet drops by maintaining a small backlog at the
 * bottleneck queue.
 *
 * Compound continuously measures the actual throughput a connection achieves as shown in
 * Equation (1) and compares it with the expected throughput calculated in Equation (2).
 * The difference between these 2 sending rates in Equation (3) reflects the amount of
 * extra packets being queued at the bottleneck.
 *
 *              actual = cwnd / RTT        (1)
 *              expected = cwnd / BaseRTT  (2)
 *              diff = expected - actual   (3)
 *
 * To avoid congestion, Compound linearly increases/decreases its congestion window to ensure
 * the diff value fall between the 2 predefined thresholds, alpha and beta.
 * diff and another threshold, gamma, are used to determine when Compound should change from
 * its slow-start mode to linear increase/decrease mode.
 *
 * Following the implementation of Compound in Linux, we use 2, 4, and 1 as the default values
 * of alpha, beta, and gamma, respectively.
 *
 * More information: http://dx.doi.org/10.1109/49.464716
 */

class TcpCompound : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  TcpCompound (void);

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpCompound (const TcpCompound& sock);
  virtual ~TcpCompound (void);

  virtual std::string GetName () const;

  /**
   * \brief Compute RTTs needed to execute Compound algorithm
   *
   * The function filters RTT samples from the last RTT to find
   * the current smallest propagation delay + queueing delay (minRtt).
   * We take the minimum to avoid the effects of delayed ACKs.
   *
   * The function also min-filters all RTT measurements seen to find the
   * propagation delay (baseRtt).
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   * \param rtt last RTT
   *
   */
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);

  /**
   * \brief Enable/disable Compound algorithm depending on the congestion state
   *
   * We only start a Compound cycle when we are in normal congestion state (CA_OPEN state).
   *
   * \param tcb internal congestion state
   * \param newState new congestion state to which the TCP is going to switch
   */
  virtual void CongestionStateSet (Ptr<TcpSocketState> tcb,
                                   const TcpSocketState::TcpCongState_t newState);

  /**
   * \brief Adjust cwnd following Compound linear increase/decrease algorithm
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   */
  virtual void IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

  /**
   * \brief Get slow start threshold following Compound principle
   *
   * \param tcb internal congestion state
   * \param bytesInFlight bytes in flight
   *
   * \return the slow start threshold value
   */
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);

  virtual Ptr<TcpCongestionOps> Fork ();

protected:
private:
  /**
   * \brief Enable Compound algorithm to start taking Compound samples
   *
   * Compound algorithm is enabled in the following situations:
   * 1. at the establishment of a connection
   * 2. after an RTO
   * 3. after fast recovery
   * 4. when an idle connection is restarted
   *
   * \param tcb internal congestion state
   */
  void EnableCompound (Ptr<TcpSocketState> tcb);

  /**
   * \brief Stop taking Compound samples
   */
  void DisableCompound ();

private:
  double m_alpha;                                       //!< Parameter used in multiplicative increase
  double m_beta;                                        //!< Parameter used in multiplicative decrease
  double m_eta;                                         //!< Parameter used in additive increase of m_dwnd
  double m_k;                                           //!< Exponent used in multiplicative increase
  double m_lamda;                                               //!< Weight assigned to new sample in gamma calculation
  uint32_t m_lwnd;                  //!< Locally maintained loss-based congestion window
  uint32_t m_dwnd;                  //!< Locally maintained delay-based congestion window
  uint32_t m_diffReno;                          //!< diff value calculated for gamma autotuning
  bool m_diffRenoValid;      //!< If true, do gamma autotuning upon loss

  uint32_t m_gammaLow;                  //!< Gamma threshold, upper bound of packets in network
  uint32_t m_gammaHigh;                  //!< Gamma threshold, upper bound of packets in network
  uint32_t m_gamma;                  //!< Gamma threshold, upper bound of packets in network

  Time m_baseRtt;                    //!< Minimum of all Compound RTT measurements seen during connection
  Time m_minRtt;                     //!< Minimum of all RTT measurements within last RTT
  uint32_t m_cntRtt;                 //!< Number of RTT measurements during last RTT
  Time m_srtt;                    //!< Smoothened RTT

  bool m_doingCompoundNow;           //!< If true, do Compound for this RTT
  SequenceNumber32 m_begSndNxt;      //!< Right edge during last RTT
};

} // namespace ns3

#endif // TCPCompound_H
