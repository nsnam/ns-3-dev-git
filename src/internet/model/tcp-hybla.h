/*
 * Copyright (c) 2014 Natale Patriciello <natale.patriciello@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */
#ifndef TCPHYBLA_H
#define TCPHYBLA_H

#include "tcp-congestion-ops.h"

#include "ns3/traced-value.h"

namespace ns3
{

class TcpSocketState;

/**
 * @ingroup congestionOps
 *
 * @brief Implementation of the TCP Hybla algorithm
 *
 * The key idea behind TCP Hybla is to obtain for long RTT connections the same
 * instantaneous transmission rate of a reference TCP connection with lower RTT.
 * With analytical steps, it is shown that this goal can be achieved by
 * modifying the time scale, in order for the throughput to be independent from
 * the RTT. This independence is obtained through the use of a coefficient rho.
 *
 * This coefficient is used to calculate both the slow start threshold
 * and the congestion window when in slow start and in congestion avoidance,
 * respectively.
 *
 * More information: http://dl.acm.org/citation.cfm?id=2756518
 */
class TcpHybla : public TcpNewReno
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
    TcpHybla();

    /**
     * @brief Copy constructor
     * @param sock the object to copy
     */
    TcpHybla(const TcpHybla& sock);

    ~TcpHybla() override;

    // Inherited
    void PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) override;
    std::string GetName() const override;
    Ptr<TcpCongestionOps> Fork() override;

  protected:
    uint32_t SlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;
    void CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked) override;

  private:
    TracedValue<double> m_rho; //!< Rho parameter
    Time m_rRtt;               //!< Reference RTT
    double m_cWndCnt;          //!< cWnd integer-to-float counter

  private:
    /**
     * @brief Recalculate algorithm parameters
     * @param tcb the socket state.
     */
    void RecalcParam(const Ptr<TcpSocketState>& tcb);
};

} // namespace ns3

#endif // TCPHYBLA_H
