/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmRoceBth: simulator BTH (Base Transport Header) for the RoCEv2-style
 * baseline. Deliberate design decision (Phase 2, task 1 — "native stack vs
 * controlled bypass"):
 *
 *   The baseline uses a CONTROLLED BYPASS of the ns-3 network stack, in the
 *   spirit of real RDMA: packets are built by the NIC model and move over the
 *   ndm-topology path forwarder (which carries them hop by hop on real
 *   point-to-point links, with real serialization and queue drops). The
 *   header layout below is a compact simulator BTH, NOT the IEEE 2410 BTH
 *   byte-for-byte; it carries the same semantic fields the baseline needs
 *   (opcode, PSN, QP identity, immediate, ECN echo, ACK-request). This keeps
 *   the wire bytes inspectable (gate: packet captures must show real bytes)
 *   while avoiding a full IB transport emulation.
 *
 *   RoCEv2 framing on the wire (what the links actually carry):
 *     [PathHdr 8B (ndm-topology, per-hop)] [IPv6 40B] [UDP 8B] [NdmRoceBth 16B] [payload]
 *   For the point-to-point baseline the IPv6/UDP headers are appended by the
 *   endpoint (ndm-roce-endpoint) so captures show a real IPv6 packet; the
 *   forwarder moves the whole thing opaquely.
 *
 * PPS-safety (D7): plain value types; no timers here; all scheduling lives in
 * the QP objects (Simulator events only).
 */

#ifndef NDM_ROCE_BTH_H
#define NDM_ROCE_BTH_H

#include "ns3/header.h"

namespace ns3
{

/// NdmRoceBth opcodes (simulator RoCE baseline).
enum NdmRoceOpcode : uint8_t
{
    NDM_ROCE_WRITE = 1,      ///< RDMA WRITE: payload carried, no RC ACK payload
    NDM_ROCE_WRITE_WITH_IMM = 2, ///< last PSN of a message; carries 32-bit immediate
    NDM_ROCE_ACK = 3,         ///< cumulative ACK: m_psn = highest contiguous PSN acked
    NDM_ROCE_NACK = 4,        ///< m_psn = lowest unacked PSN (go-back-N trigger)
    NDM_ROCE_CNP = 5          ///< congestion notification (rate-based DCQCN)
};

/// BTH flags (byte 2).
enum NdmRoceBthFlags : uint8_t
{
    NDM_ROCE_BTH_ACK_REQ = 1 << 0, ///< sender requests an ACK for this PSN
    NDM_ROCE_BTH_ECN_MARKED = 1 << 1, ///< network marked this packet CE (link ECN)
    NDM_ROCE_BTH_ECN_ECHO = 1 << 2 ///< ACK echoes CE (receiver saw >=1 mark since last ACK)
};

/**
 * @brief Simulator BTH: 16 bytes on the wire.
 */
class NdmRoceBth : public Header
{
  public:
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    NdmRoceBth() = default;
    NdmRoceBth(uint8_t opcode, uint32_t psn, uint32_t destQp)
        : m_opcode(opcode),
          m_psn(psn),
          m_destQp(destQp)
    {
    }

    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    void SetFlags(uint8_t f)
    {
        m_flags = f;
    }

    uint8_t m_opcode{NDM_ROCE_WRITE};
    uint8_t m_flags{0};
    uint32_t m_psn{0};
    uint32_t m_destQp{0};
    uint32_t m_imm{0};
};

} // namespace ns3

#endif // NDM_ROCE_BTH_H
