/*
 * Copyright (c) 2016 ResiliNets, ITTC, University of Kansas
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Truc Anh N. Nguyen <annguyen@ittc.ku.edu>
 *          Keerthi Ganta <keerthig@ittc.ku.edu>
 *          Md Moshfequr Rahman <moshfequr@ittc.ku.edu>
 *          Amir Modarresi <amodarresi@ittc.ku.edu>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 */

#ifndef TCPSCALABLE_H
#define TCPSCALABLE_H

#include "tcp-congestion-ops.h"

namespace ns3
{

class TcpSocketState;

/**
 * @ingroup congestionOps
 *
 * @brief An implementation of TCP Scalable
 *
 * Scalable improves TCP performance to better utilize the available bandwidth
 * of a highspeed wide area network by altering NewReno congestion window
 * adjustment algorithm.
 *
 * When congestion has not been detected, for each ACK received in an RTT,
 * Scalable increases its cwnd per:
 *
 *         cwnd = cwnd + 0.01                  (1)
 *
 * Following Linux implementation of Scalable, we use 50 instead of 100 to
 * account for delayed ACK.
 *
 * On the first detection of congestion in a given RTT, cwnd is reduced based
 * on the following equation:
 *
 *         cwnd = cwnd - ceil(0.125 * cwnd)       (2)
 *
 * More information: http://doi.acm.org/10.1145/956981.956989
 */

class TcpScalable : public TcpNewReno
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
    TcpScalable();

    /**
     * @brief Copy constructor
     * @param sock the object to copy
     */
    TcpScalable(const TcpScalable& sock);
    ~TcpScalable() override;

    std::string GetName() const override;

    /**
     * @brief Get slow start threshold following Scalable principle (Equation 2)
     *
     * @param tcb internal congestion state
     * @param bytesInFlight bytes in flight
     *
     * @return the slow start threshold value
     */
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;

    Ptr<TcpCongestionOps> Fork() override;

  protected:
    /**
     * @brief Congestion avoidance of TcpScalable (Equation 1)
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments acked
     */
    void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

  private:
    uint32_t m_ackCnt;   //!< Number of received ACK
    uint32_t m_aiFactor; //!< Additive increase factor
    double m_mdFactor;   //!< Multiplicative decrease factor
};

} // namespace ns3

#endif // TCPSCALABLE_H
