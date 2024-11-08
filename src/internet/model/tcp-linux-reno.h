/*
 * Copyright (c) 2019 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Apoorva Bharagava <apoorvabhargava13@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */

#ifndef TCPLINUXRENO_H
#define TCPLINUXRENO_H

#include "tcp-congestion-ops.h"
#include "tcp-socket-state.h"

namespace ns3
{

/**
 * @ingroup congestionOps
 *
 * @brief Reno congestion control algorithm
 *
 * This class implement the Reno congestion control type
 * and it mimics the one implemented in the Linux kernel.
 */
class TcpLinuxReno : public TcpCongestionOps
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    TcpLinuxReno();

    /**
     * @brief Copy constructor.
     * @param sock object to copy.
     */
    TcpLinuxReno(const TcpLinuxReno& sock);

    ~TcpLinuxReno() override;

    std::string GetName() const override;

    void IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
    uint32_t GetSsThresh(Ptr<const TcpSocketState> tcb, uint32_t bytesInFlight) override;
    Ptr<TcpCongestionOps> Fork() override;

  protected:
    /**
     * Slow start phase handler
     * @param tcb Transmission Control Block of the connection
     * @param segmentsAcked count of segments acked
     * @return Number of segments acked minus the difference between the receiver and sender Cwnd
     */
    virtual uint32_t SlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);
    /**
     * Congestion avoidance phase handler
     * @param tcb Transmission Control Block of the connection
     * @param segmentsAcked count of segments acked
     */
    virtual void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked);

    /**
     * TcpSocketBase follows the Linux way of setting a flag 'isCwndLimited'
     * when BytesInFlight() >= cwnd.  This flag, if true, will suppress
     * additive increase window updates for this class.  However, some
     * derived classes using the IncreaseWindow() method may not want this
     * behavior, so this method exists to allow subclasses to set it to false.
     *
     * @param value Value to set whether the isCwndLimited condition
     *        suppresses window updates
     */
    void SetSuppressIncreaseIfCwndLimited(bool value);

  private:
    uint32_t m_cWndCnt{0}; //!< Linear increase counter
    // The below flag exists for classes derived from TcpLinuxReno (such as
    // TcpDctcp) that may not want to suppress cwnd increase, unlike Linux's
    // tcp_reno_cong_avoid()
    bool m_suppressIncreaseIfCwndLimited{
        true}; //!< Suppress window increase if TCP is not cwnd limited
};

} // namespace ns3

#endif // TCPLINUXRENO_H
