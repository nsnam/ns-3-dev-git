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
                              uint32_t deliveredBytes,
                              uint32_t bytesSacked)
{
    NS_LOG_FUNCTION(this << tcb << dupAckCount << unAckDataCount << deliveredBytes << bytesSacked);

    m_prrOut = 0;
    m_prrDelivered = 0;
    // RFC 9937 Section 6.1 (and RFC 6937 line 296): RecoverFS is based on the
    // FlightSize (SND.NXT - SND.UNA) at the start of recovery, NOT the RFC 6675
    // pipe (FlightSize - sacked - lost).  unAckDataCount carries SND.NXT - SND.UNA.
    // Using the smaller pipe here inflates the proportional send count and makes
    // PRR overshoot ssThresh at recovery exit.
    m_recoveryFlightSize = unAckDataCount;

    // RFC 9937 Section 6.1 SACK refinement.  Bytes SACKed before entering
    // recovery are already delivered and must not be counted as flight to be
    // recovered, so they are subtracted; bytes newly SACKed and newly
    // cumulatively acknowledged by the triggering ACK are added back:
    //   RecoverFS = SND.NXT - SND.UNA - (bytes SACKed in scoreboard)
    //               + (bytes newly SACKed) + (bytes newly cumulatively acked)
    // deliveredBytes already carries (newly SACKed + newly cumulatively acked)
    // (it is the caller's SACK-derived DeliveredData), and bytesSacked is the
    // post-ACK scoreboard SACKed total, so subtracting bytesSacked and adding
    // deliveredBytes nets out to removing only the pre-existing SACKed bytes.
    // Applied only with SACK: without SACK there is no real scoreboard (the
    // reno-sack estimate is handled per-ACK in DoRecovery), so RecoverFS stays
    // at the plain FlightSize.  Signed math guards a transient underflow, and
    // the result is clamped to at least 1 SMSS to keep it a valid divisor.
    if (tcb->m_sackEnabled)
    {
        int64_t recoverFs = static_cast<int64_t>(unAckDataCount) -
                            static_cast<int64_t>(bytesSacked) +
                            static_cast<int64_t>(deliveredBytes);
        m_recoveryFlightSize =
            static_cast<uint32_t>(std::max<int64_t>(recoverFs, tcb->m_segmentSize));
    }

    NS_LOG_INFO("Enter recovery: cWnd " << tcb->m_cWnd << " ssThresh " << tcb->m_ssThresh
                                        << " recoveryFlightSize " << m_recoveryFlightSize
                                        << " unAckDataCount " << unAckDataCount << " bytesSacked "
                                        << bytesSacked << " deliveredBytes " << deliveredBytes);

    DoRecovery(tcb, deliveredBytes, true);
}

void
TcpPrrRecovery::DoRecovery(Ptr<TcpSocketState> tcb, uint32_t deliveredBytes, bool isDupAck)
{
    NS_LOG_FUNCTION(this << tcb << deliveredBytes);

    // RFC 9937 Section 6.2 (and RFC 6937): the "+1 SMSS per duplicate ACK"
    // estimate of DeliveredData applies only to connections *without* SACK.
    // With SACK (the ns-3 default), the caller already supplies the SACK-derived
    // DeliveredData (change in SND.UNA plus newly SACKed bytes), so adding a
    // segment here would double-count and inflate prr_delivered.
    // The m_prrDelivered < m_recoveryFlightSize guard is the RFC's Savage99
    // anti-inflation mitigation (RFC 9937 Section 6.2): without SACK, PRR
    // disallows incrementing DeliveredData once the total delivered in the
    // episode would exceed RecoverFS.  This bound is spec-mandated, not
    // incidental; do not drop it.
    if (isDupAck && !tcb->m_sackEnabled && m_prrDelivered < m_recoveryFlightSize)
    {
        deliveredBytes += tcb->m_segmentSize;
    }
    if (deliveredBytes == 0)
    {
        return;
    }

    m_prrDelivered += deliveredBytes;

    int64_t sendCount;
    if (tcb->m_bytesInFlight > tcb->m_ssThresh)
    {
        // Proportional Rate Reduction (RFC 6937)
        // Use 64-bit arithmetic to prevent uint32_t * uint32_t overflow
        uint64_t dividend =
            static_cast<uint64_t>(m_prrDelivered) * static_cast<uint64_t>(tcb->m_ssThresh);
        // Integer ceiling division: ceil(a/b) = a/b + (a%b != 0)
        uint64_t quotient = dividend / static_cast<uint64_t>(m_recoveryFlightSize) +
                            (dividend % static_cast<uint64_t>(m_recoveryFlightSize) != 0);
        sendCount = static_cast<int64_t>(quotient) - static_cast<int64_t>(m_prrOut);

        NS_LOG_DEBUG("PRR: prrDelivered " << m_prrDelivered << " ssThresh " << tcb->m_ssThresh
                                          << " recoveryFlightSize " << m_recoveryFlightSize
                                          << " dividend " << dividend << " quotient " << quotient
                                          << " prrOut " << m_prrOut << " sendCount " << sendCount);
    }
    else
    {
        // PRR-CRB by default
        // Use signed arithmetic to prevent unsigned subtraction wrap
        int64_t limit =
            std::max<int64_t>(static_cast<int64_t>(m_prrDelivered) - static_cast<int64_t>(m_prrOut),
                              static_cast<int64_t>(deliveredBytes));

        // safeACK should be true iff ACK advances SND.UNA with no further loss indicated.
        // We approximate that here (given the current lack of RACK-TLP in ns-3)
        bool safeACK = tcb->m_isRetransDataAcked; // retransmit cumulatively ACKed?

        if (safeACK)
        {
            // PRR-SSRB when recovery makes good progress
            limit += static_cast<int64_t>(tcb->m_segmentSize);
        }

        // Attempt to catch up, as permitted
        sendCount = std::min<int64_t>(limit,
                                      static_cast<int64_t>(tcb->m_ssThresh) -
                                          static_cast<int64_t>(tcb->m_bytesInFlight));

        NS_LOG_DEBUG("PRR CRB/SSRB: prrDelivered "
                     << m_prrDelivered << " prrOut " << m_prrOut << " deliveredBytes "
                     << deliveredBytes << " safeACK " << safeACK << " limit " << limit
                     << " ssThresh " << tcb->m_ssThresh << " bytesInFlight " << tcb->m_bytesInFlight
                     << " sendCount " << sendCount);
    }

    /* Force a fast retransmit upon entering fast recovery */
    sendCount =
        std::max<int64_t>(sendCount, static_cast<int64_t>(m_prrOut > 0 ? 0 : tcb->m_segmentSize));
    tcb->m_cWnd = tcb->m_bytesInFlight + static_cast<uint32_t>(sendCount);
    tcb->m_cWndInfl = tcb->m_cWnd;

    NS_LOG_DEBUG("Recovery progress ["
                 << (tcb->m_bytesInFlight > tcb->m_ssThresh ? "PRR" : "CRB/SSRB")
                 << "]: deliveredBytes " << deliveredBytes << " isDupAck " << isDupAck
                 << " prrDelivered " << m_prrDelivered << " prrOut " << m_prrOut
                 << " recoveryFlightSize " << m_recoveryFlightSize << " sendCount " << sendCount
                 << " bytesInFlight " << tcb->m_bytesInFlight << " ssThresh " << tcb->m_ssThresh
                 << " -> cWnd " << tcb->m_cWnd);
}

void
TcpPrrRecovery::ExitRecovery(Ptr<TcpSocketState> tcb)
{
    NS_LOG_FUNCTION(this << tcb);

    NS_LOG_INFO("Exit recovery: cWnd " << tcb->m_cWnd << " ssThresh " << tcb->m_ssThresh
                                       << " prrDelivered " << m_prrDelivered << " prrOut "
                                       << m_prrOut);

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
