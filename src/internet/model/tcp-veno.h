/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

#ifndef TCPVENO_H
#define TCPVENO_H

#include "tcp-congestion-ops.h"

namespace ns3
{

class TcpSocketState;

/**
 * @ingroup congestionOps
 *
 * @brief An implementation of TCP Veno
 *
 * TCP Veno enhances Reno algorithm for more effectively dealing with random
 * packet loss in wireless access networks by employing Vegas's method in
 * estimating the backlog at the bottleneck queue to distinguish between
 * congestive and non-congestive states.
 *
 * The backlog (the number of packets accumulated at the bottleneck queue) is
 * calculated using Equation (1):
 *
 *         N = Actual * (RTT - BaseRTT) = Diff * BaseRTT        (1)
 * where
 *         Diff = Expected - Actual = cwnd/BaseRTT - cwnd/RTT
 *
 * Veno makes decision on cwnd modification based on the calculated N and its
 * predefined threshold beta.
 *
 * Specifically, it refines the additive increase algorithm of Reno so that the
 * connection can stay longer in the stable state by incrementing cwnd by
 * 1/cwnd for every other new ACK received after the available bandwidth has
 * been fully utilized, i.e. when N exceeds beta.  Otherwise, Veno increases
 * its cwnd by 1/cwnd upon every new ACK receipt as in Reno.
 *
 * In the multiplicative decrease algorithm, when Veno is in the non-congestive
 * state, i.e. when N is less than beta, Veno decrements its cwnd by only 1/5
 * because the loss encountered is more likely a corruption-based loss than a
 * congestion-based.  Only when N is greater than beta, Veno halves its sending
 * rate as in Reno.
 *
 * More information: http://dx.doi.org/10.1109/JSAC.2002.807336
 */

class TcpVeno : public TcpNewReno
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Create an unbound tcp socket.
     */
    TcpVeno();

    /**
     * @brief Copy constructor
     * @param sock the object to copy
     */
    TcpVeno(const TcpVeno& sock);
    ~TcpVeno() override;

    std::string GetName() const override;

    /**
     * @brief Perform RTT sampling needed to execute Veno algorithm
     *
     * The function filters RTT samples from the last RTT to find
     * the current smallest propagation delay + queueing delay (m_minRtt).
     * We take the minimum to avoid the effects of delayed ACKs.
     *
     * The function also min-filters all RTT measurements seen to find the
     * propagation delay (m_baseRtt).
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     * @param rtt last RTT
     *
     */
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;

    /**
     * @brief Enable/disable Veno depending on the congestion state
     *
     * We only start a Veno when we are in normal congestion state (CA_OPEN state).
     *
     * @param tcb internal congestion state
     * @param newState new congestion state to which the TCP is going to switch
     */
    void CongestionStateSet(Ptr<TcpSocketState> tcb,
                            const TcpSocketState::TcpCongState_t newState) override;

    /**
     * @brief Adjust cwnd following Veno additive increase algorithm
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments ACKed
     */
    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

    /**
     * @brief Get slow start threshold during Veno multiplicative-decrease phase
     *
     * @param tcb internal congestion state
     * @param bytesInFlight bytes in flight
     *
     * @return the slow start threshold value
     */
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

    Ptr<TcpCongestionOps> Fork() override;

  protected:
  private:
    /**
     * @brief Enable Veno algorithm to start Veno sampling
     *
     * Veno algorithm is enabled in the following situations:
     * 1. at the establishment of a connection
     * 2. after an RTO
     * 3. after fast recovery
     * 4. when an idle connection is restarted
     *
     */
    void EnableVeno();

    /**
     * @brief Turn off Veno
     */
    void DisableVeno();

  private:
    Time m_baseRtt;      //!< Minimum of all RTT measurements seen during connection
    Time m_minRtt;       //!< Minimum of RTTs measured within last RTT
    uint32_t m_cntRtt;   //!< Number of RTT measurements during last RTT
    bool m_doingVenoNow; //!< If true, do Veno for this RTT
    uint32_t m_diff;     //!< Difference between expected and actual throughput
    bool m_inc;          //!< If true, cwnd needs to be incremented
    uint32_t m_ackCnt;   //!< Number of received ACK
    uint32_t m_beta;     //!< Threshold for congestion detection
};

} // namespace ns3

#endif // TCPVENO_H
