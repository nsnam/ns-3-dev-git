/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmRoceRxQPair: receiver-side RoCE queue pair.
 *
 * Contract:
 *  - In-order delivery to the application: a PSN equal to the next expected
 *    is delivered; a lower PSN is a duplicate (retransmit) and triggers a
 *    cumulative re-ACK only (never re-delivered — exactly-once at the app);
 *    a higher PSN (gap) triggers a NACK carrying the next expected PSN
 *    (go-back-N at the sender).
 *  - Cumulative ACKs: one ACK per in-order data packet carrying the ACK_REQ
 *    flag, psn = the PSN just delivered. If any in-order packet since the
 *    last ACK was ECN-marked, the ACK sets the ECN_ECHO flag (informational
 *    in the rate-based baseline; the CNP carries the rate reaction).
 *  - CNP generation (DCQCN, receiver side): each ECN-marked data packet
 *    increments a counter; at threshold (default 1) a CNP is sent to the
 *    peer's TX QP; at most one CNP per ACK window (dedup — the donor's
 *    m_first_cnp rule, re-implemented).
 *  - PPS-safety: plain counters; no timers in the baseline (the donor's
 *    NACK timer is unnecessary because the sender's RTO covers loss).
 */

#ifndef NDM_ROCE_RX_QPAIR_H
#define NDM_ROCE_RX_QPAIR_H

#include "ns3/ndm-roce-bth.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/simple-ref-count.h"

#include <cstdint>

namespace ns3
{

class NdmRoceRxQPair : public SimpleRefCount<NdmRoceRxQPair>
{
  public:
    NdmRoceRxQPair(uint32_t localQpn, uint32_t peerQpn);

    uint32_t GetLocalQpn() const
    {
        return m_localQpn;
    }

    void SetCnpThreshold(uint32_t t)
    {
        m_cnpThreshold = t;
    }

    /// Handle a data packet (BTH already parsed; `payload` is the packet
    /// with the BTH removed, possibly empty for a zero-length message).
    void HandleData(const NdmRoceBth& bth, Ptr<Packet> payload, Time now);

    // -- gate evidence ---------------------------------------------------------
    uint64_t GetDeliveredPackets() const
    {
        return m_delivered;
    }
    uint64_t GetDuplicates() const
    {
        return m_duplicates;
    }
    uint64_t GetNacksSent() const
    {
        return m_nacksSent;
    }
    uint64_t GetCnpsSent() const
    {
        return m_cnpsSent;
    }
    uint64_t GetBytesDelivered() const
    {
        return m_bytesDelivered;
    }
    bool HasGaps() const
    {
        return m_nacksSent > 0;
    }

    /// Deliver (psn, payload, imm, isLastMessagePsn) to the application.
    Callback<void, uint32_t, Ptr<Packet>, uint32_t, bool> m_deliver;
    /// Wire send of a control packet (BTH only), addressed to the peer's
    /// TX QP number.
    Callback<void, Ptr<Packet>, uint32_t> m_sendCtrl;

  private:
    void SendCtrl(uint8_t opcode, uint32_t psn, uint8_t flags);

    uint32_t m_localQpn{0};
    uint32_t m_peerQpn{0};
    uint32_t m_nextPsn{0};
    uint32_t m_cnpThreshold{1};

    uint64_t m_delivered{0};
    uint64_t m_duplicates{0};
    uint64_t m_nacksSent{0};
    uint64_t m_cnpsSent{0};
    uint64_t m_bytesDelivered{0};

    uint32_t m_ecnSinceLastAck{0};
    bool m_cnpSinceLastAck{false};
};

} // namespace ns3

#endif // NDM_ROCE_RX_QPAIR_H
