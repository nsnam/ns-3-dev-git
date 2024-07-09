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

#include "tcp-prr-recovery.h"

#include "tcp-socket-state.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TcpPrrRecovery");
NS_OBJECT_ENSURE_REGISTERED(TcpPrrRecovery);

TypeId
TcpPrrRecovery::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TcpPrrRecovery")
                            .SetParent<TcpClassicRecovery>()
                            .AddConstructor<TcpPrrRecovery>()
                            .SetGroupName("Internet");
    return tid;
}

TcpPrrRecovery::TcpPrrRecovery()
    : TcpClassicRecovery()
{
    NS_LOG_FUNCTION(this);
}

TcpPrrRecovery::TcpPrrRecovery(const TcpPrrRecovery& recovery)
    : TcpClassicRecovery(recovery),
      m_prrDelivered(recovery.m_prrDelivered),
      m_prrOut(recovery.m_prrOut),
      m_recoveryFlightSize(recovery.m_recoveryFlightSize)
{
    NS_LOG_FUNCTION(this);
}

TcpPrrRecovery::~TcpPrrRecovery()
{
    NS_LOG_FUNCTION(this);
}

void
TcpPrrRecovery::EnterRecovery(Ptr<TcpSocketState> tcb,
                              uint32_t dupAckCount [[maybe_unused]],
                              uint32_t unAckDataCount,
                              uint32_t deliveredBytes)
{
    NS_LOG_FUNCTION(this << tcb << dupAckCount << unAckDataCount);

    m_prrOut = 0;
    m_prrDelivered = 0;
    m_recoveryFlightSize = tcb->m_bytesInFlight; // RFC 6675 pipe before recovery

    DoRecovery(tcb, deliveredBytes, true);
}

void
TcpPrrRecovery::DoRecovery(Ptr<TcpSocketState> tcb, uint32_t deliveredBytes, bool isDupAck)
{
    NS_LOG_FUNCTION(this << tcb << deliveredBytes);

    if (isDupAck && m_prrDelivered < m_recoveryFlightSize)
    {
        deliveredBytes += tcb->m_segmentSize;
    }
    if (deliveredBytes == 0)
    {
        return;
    }

    m_prrDelivered += deliveredBytes;

    int sendCount;
    if (tcb->m_bytesInFlight > tcb->m_ssThresh)
    {
        // Proportional Rate Reductions
        sendCount =
            std::ceil(m_prrDelivered * tcb->m_ssThresh * 1.0 / m_recoveryFlightSize) - m_prrOut;
    }
    else
    {
        // PRR-CRB by default
        int limit = std::max(m_prrDelivered - m_prrOut, deliveredBytes);

        // safeACK should be true iff ACK advances SND.UNA with no further loss indicated.
        // We approximate that here (given the current lack of RACK-TLP in ns-3)
        bool safeACK = tcb->m_isRetransDataAcked; // retransmit cumulatively ACKed?

        if (safeACK)
        {
            // PRR-SSRB when recovery makes good progress
            limit += tcb->m_segmentSize;
        }

        // Attempt to catch up, as permitted
        sendCount = std::min(limit, static_cast<int>(tcb->m_ssThresh - tcb->m_bytesInFlight));
    }

    /* Force a fast retransmit upon entering fast recovery */
    sendCount = std::max(sendCount, static_cast<int>(m_prrOut > 0 ? 0 : tcb->m_segmentSize));
    tcb->m_cWnd = tcb->m_bytesInFlight + sendCount;
    tcb->m_cWndInfl = tcb->m_cWnd;
}

void
TcpPrrRecovery::ExitRecovery(Ptr<TcpSocketState> tcb)
{
    NS_LOG_FUNCTION(this << tcb);
    tcb->m_cWndInfl = tcb->m_cWnd;
}

void
TcpPrrRecovery::UpdateBytesSent(uint32_t bytesSent)
{
    NS_LOG_FUNCTION(this << bytesSent);
    m_prrOut += bytesSent;
}

Ptr<TcpRecoveryOps>
TcpPrrRecovery::Fork()
{
    return CopyObject<TcpPrrRecovery>(this);
}

std::string
TcpPrrRecovery::GetName() const
{
    return "PrrRecovery";
}

} // namespace ns3
