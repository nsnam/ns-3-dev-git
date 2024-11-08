/*
 * Copyright (c) 2016 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Charitha Sangaraju <charitha29193@gmail.com>
 *          Nandita G <gm.nandita@gmail.com>
 *          Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#ifndef TCPLP_H
#define TCPLP_H

#include "tcp-congestion-ops.h"

#include "ns3/traced-value.h"

namespace ns3
{

class TcpSocketState;

/**
 * @ingroup congestionOps
 *
 * @brief TCP-LP (Low Priority) congestion control algorithm
 */
class TcpLp : public TcpNewReno
{
  public:
    /**
     * @brief Get the type ID.
     *
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Creates an unbound tcp socket.
     *
     */
    TcpLp();

    /**
     * @brief Copy constructor
     *
     * @param sock the object to copy
     */
    TcpLp(const TcpLp& sock);

    ~TcpLp() override;

    /**
     * @brief Timing information on received ACK
     *
     * The function is called every time an ACK is received.
     * It determines the state of TcpLp and adjusts the congestion window accordingly.
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments acked
     * @param rtt last rtt
     */
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;

    std::string GetName() const override;

    Ptr<TcpCongestionOps> Fork() override;

  protected:
    /**
     * @brief Invokes Congestion Avoidance of TcpNewReno if TcpLp is not within inference.
     *
     * @param tcb internal congestion state
     * @param segmentsAcked count of segments acked
     */
    void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

  private:
    /**
     * @brief Describes the state of TcpLp.
     *
     */
    enum State
    {
        LP_VALID_OWD = (1 << 1),  /**< Calculated One-Way Delay is valid */
        LP_WITHIN_THR = (1 << 3), /**< TcpLp is within Threshold */
        LP_WITHIN_INF = (1 << 4), /**< TcpLp is within Inference */
    };

    uint32_t m_flag;      //!< TcpLp state flag
    uint32_t m_sOwd;      //!< Smoothed One-Way Delay
    uint32_t m_owdMin;    //!< Minimum One-Way Delay
    uint32_t m_owdMax;    //!< Maximum One-Way Delay
    uint32_t m_owdMaxRsv; //!< Reserved Maximum One-Way Delay
    Time m_lastDrop;      //!< Last time when cwnd was reduced
    Time m_inference;     //!< Current inference period

  private:
    /**
     * @brief Calculates One-Way Delay using Sender and Receiver timestamps.
     *
     * @param tcb internal congestion state
     * @return One-Way Delay
     */
    uint32_t OwdCalculator(Ptr<TcpSocketState> tcb);

    /**
     * @brief Estimates minimum and maximum One-Way Delays and calculates the smoothed One-Way
     * Delay.
     *
     * @param tcb internal congestion state
     */
    void RttSample(Ptr<TcpSocketState> tcb);
};

} // namespace ns3

#endif // TCPLP_H
