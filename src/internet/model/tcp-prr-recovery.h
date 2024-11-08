/*
 * Copyright (c) 2018 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Viyom Mittal <viyommittal@gmail.com>
 *         Vivek Jain <jain.vivek.anand@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 *
 */
#ifndef TCP_PRR_RECOVERY_H
#define TCP_PRR_RECOVERY_H

#include "tcp-recovery-ops.h"

namespace ns3
{

/**
 * @ingroup recoveryOps
 *
 * @brief An implementation of PRR
 *
 * PRR is an algorithm that determines TCP's sending rate in fast
 * recovery. PRR avoids excessive window reductions and aims for
 * the actual congestion window size at the end of recovery to be as
 * close as possible to the window determined by the congestion control
 * algorithm. PRR also improves accuracy of the amount of data sent
 * during loss recovery.
 */
class TcpPrrRecovery : public TcpClassicRecovery
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
    TcpPrrRecovery();

    /**
     * @brief Copy constructor
     * @param sock the object to copy
     */
    TcpPrrRecovery(const TcpPrrRecovery& sock);

    ~TcpPrrRecovery() override;

    std::string GetName() const override;

    void EnterRecovery(Ptr<TcpSocketState> tcb,
                       uint32_t dupAckCount,
                       uint32_t unAckDataCount,
                       uint32_t deliveredBytes) override;

    void DoRecovery(Ptr<TcpSocketState> tcb, uint32_t deliveredBytes, bool isDupAck) override;

    void ExitRecovery(Ptr<TcpSocketState> tcb) override;

    void UpdateBytesSent(uint32_t bytesSent) override;

    Ptr<TcpRecoveryOps> Fork() override;

  private:
    uint32_t m_prrDelivered{0};       //!< total bytes delivered during recovery phase
    uint32_t m_prrOut{0};             //!< total bytes sent during recovery phase
    uint32_t m_recoveryFlightSize{0}; //!< value of bytesInFlight at the start of recovery phase
};
} // namespace ns3

#endif /* TCP_PRR_RECOVERY_H */
