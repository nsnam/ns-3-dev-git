/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmRoceQPair: sender-side RoCE queue pair (RC semantics).
 *
 * Contract:
 *  - The application posts Write messages with PostWrite(nbytes, imm).
 *    PSNs are assigned contiguously across messages; the final PSN of each
 *    message uses opcode WRITE_WITH_IMM and carries the 32-bit immediate.
 *  - In-order, reliable delivery via cumulative ACK + go-back-N retransmit
 *    (selective repeat is a later Phase-2 upgrade per PLAN). The oldest
 *    unacked PSN has an RTO timer (4 * RTT_est); on expiry the QP
 *    retransmits from the oldest unacked (go-back-N).
 *  - Duplicates / retransmissions are byte-identical (same PSN, same
 *    payload) — receiver dedup is by PSN.
 *  - Completion: m_notifyComplete fires exactly once when every PSN of
 *    every posted message has been cumulatively ACKed.
 *  - RTT sampling: on a cumulative ACK covering the oldest unacked PSN,
 *    sample = now - sendTime(oldest unacked); EWMA 0.9/0.1. Feeds the DCQCN
 *    window and the RTO.
 *  - PPS-safety (D7): state is std::vector (no hash containers); all timing
 *    is Simulator events; no wall clock.
 *
 * Donor: concepts from astra-network-ns3 rdma-queue-pair.{h,cc} (snd_nxt /
 * snd_una, window, baseRtt, per-CC state) — re-implemented natively, IPv6
 * world, ns-3.42 style. See donors/astra-network-ns3/PROVENANCE.md.
 */

#ifndef NDM_ROCE_QPAIR_H
#define NDM_ROCE_QPAIR_H

#include "ns3/data-rate.h"
#include "ns3/ndm-roce-bth.h"
#include "ns3/ndm-roce-dcqcn.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/ptr.h"
#include "ns3/simple-ref-count.h"

#include <cstdint>
#include <vector>

namespace ns3
{

class NdmRoceQPair : public SimpleRefCount<NdmRoceQPair>
{
  public:
    NdmRoceQPair(uint32_t localQpn, uint32_t peerQpn);
    ~NdmRoceQPair();

    uint32_t GetLocalQpn() const
    {
        return m_localQpn;
    }
    uint32_t GetPeerQpn() const
    {
        return m_peerQpn;
    }

    void SetMtu(uint32_t mtuPayloadBytes); ///< default 2048
    void SetLinkRate(DataRate rate);       ///< CC r_max
    void SetInitialRtt(Time rtt);

    /// Post a Write message. nbytes may be 0 (control-only message; still
    /// produces one WRITE_WITH_IMM PSN). The message bytes are copied and
    /// retained until cumulatively ACKed (retransmits rebuild from the
    /// copy — byte-identical retransmission). Returns the first PSN.
    uint32_t PostWrite(const uint8_t* data, uint32_t nbytes, uint32_t imm);

    // -- network-side events --------------------------------------------------
    /// Cumulative ACK of `psn` (highest contiguous delivered PSN at RX).
    void HandleAck(uint32_t psn, bool ecnEcho, Time now);
    /// NACK: lowest unacked PSN at RX; triggers go-back-N retransmit.
    void HandleNack(uint32_t psn, Time now);
    void HandleCnp(Time now);

    /// Start the send pump + CC slot loop (call after configuration).
    void Start(Time now);

    // -- gate evidence ---------------------------------------------------------
    uint64_t GetRetransmitCount() const
    {
        return m_retransmits;
    }
    uint64_t GetPacketsSent() const
    {
        return m_packetsSent;
    }
    uint64_t GetBytesAcked() const
    {
        return m_bytesAcked;
    }
    Time GetLastRttSample() const
    {
        return m_lastRttSample;
    }
    bool IsFinished() const
    {
        return m_finished;
    }

    /// Wire send: (packet = BTH + payload, destQp). Endpoint encapsulates
    /// (IPv6/UDP) and forwards over the path.
    Callback<void, Ptr<Packet>, uint32_t> m_sendTx;
    /// Exactly-once completion (all PSNs ACKed).
    Callback<void> m_notifyComplete;

  private:
    struct PendingMsg
    {
        std::vector<uint8_t> data; ///< message copy (retained until ACKed)
        uint32_t imm{0};
        uint32_t psns{0};  ///< PSNs this message occupies
        uint32_t psnStart{0};
    };

    void Pump();
    void OnRto();
    void SendPsn(uint32_t psn, bool isRetransmit, Time now);
    /// Build the BTH+payload packet for `psn` (from the message copy).
    Ptr<Packet> BuildPacket(uint32_t psn);
    void RetransmitFrom(uint32_t psn, Time now);
    /// Payload bytes carried by PSN `psn` (0 for a zero-length message).
    uint32_t FragmentSize(uint32_t psn) const;

    uint32_t m_localQpn{0};
    uint32_t m_peerQpn{0};
    uint32_t m_mtu{2048};

    NdmRoceDcQcn m_cc;

    uint32_t m_sndNxt{0};   ///< next PSN to (re)send
    uint32_t m_sndUna{0};   ///< oldest unacked PSN
    uint32_t m_totalPsn{0}; ///< number of PSNs posted so far
    uint64_t m_bytesPosted{0};

    std::vector<PendingMsg> m_msgs; ///< posted messages (oldest first)
    /// send time of each in-flight PSN, index = psn - m_sndUna.
    std::vector<Time> m_unackedSend;
    /// per-PSN "still fresh" (RTO armed only for first transmissions).
    std::vector<bool> m_unackedFresh;
    uint64_t m_flightBytes{0}; ///< payload bytes of unacked PSNs

    Time m_rttEst{MicroSeconds(10)};
    Time m_lastRttSample{Seconds(0)};
    uint64_t m_retransmits{0};
    uint64_t m_packetsSent{0};
    uint64_t m_bytesAcked{0};

    EventId m_rtoEvent;
    bool m_started{false};
    bool m_finished{false};
};

} // namespace ns3

#endif // NDM_ROCE_QPAIR_H
