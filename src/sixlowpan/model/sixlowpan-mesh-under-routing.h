/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#ifndef SIXLOWPAN_MESH_UNDER_ROUTING_H
#define SIXLOWPAN_MESH_UNDER_ROUTING_H

#include "ns3/address.h"
#include "ns3/callback.h"
#include "ns3/object.h"
#include "ns3/packet.h"

#include <cstdint>
#include <deque>
#include <map>

namespace ns3
{

/**
 * @ingroup sixlowpan
 *
 * @brief Abstract base class for 6LoWPAN mesh-under forwarding policies.
 *
 * This class decouples the mesh-under forwarding decision from
 * SixLowPanNetDevice. Concrete policies inherit from this class and
 * implement the OnPacketForward() method, which is invoked by the
 * device whenever a new (non-duplicate) mesh-under packet arrives.
 *
 * The base class owns the per-originator duplicate-detection cache.
 * Concrete policies may override OnDuplicateReceived() to react to
 * duplicates (e.g., to influence if and when the packet must be
 * forwarded).
 *
 * The forwarding mechanism is encapsulated in a ForwardCallback that
 * is supplied by the device. The policy invokes the callback when
 * (and if) it decides the packet should be rebroadcast.
 *
 * The device orchestrates duplicate handling: it calls IsDuplicate(),
 * then either OnDuplicateReceived() (duplicate) or RecordPacket()
 * followed by OnPacketForward() (new packet). This contract is
 * intentional, so that a policy can base its forwarding decisions
 * on the duplicate signals without owning the receive path.
 */
class SixLowPanMeshUnderRouting : public Object
{
  public:
    /**
     * @brief Callback used by a policy to forward a packet.
     *
     * The callback is provided by SixLowPanNetDevice and hands the
     * packet to the underlying NetDevice for broadcast transmission.
     * Policies invoke it when they decide to forward.
     */
    using ForwardCallback = Callback<void, Ptr<Packet>>;

    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    SixLowPanMeshUnderRouting();
    ~SixLowPanMeshUnderRouting() override;

    // Delete copy constructor and assignment operator to avoid misuse.
    SixLowPanMeshUnderRouting(const SixLowPanMeshUnderRouting&) = delete;
    SixLowPanMeshUnderRouting& operator=(const SixLowPanMeshUnderRouting&) = delete;

    /**
     * @brief Check whether a (originator, sequence) pair has already been seen.
     *
     * @param originator The MESH header originator address.
     * @param seqNo The BC0 header sequence number.
     * @return true if the pair is already cached, false otherwise.
     */
    bool IsDuplicate(const Address& originator, uint8_t seqNo) const;

    /**
     * @brief Record a (originator, sequence) pair in the cache.
     *
     * If the per-originator cache already holds MeshCacheLength
     * entries, the oldest entry is dropped (FIFO).
     *
     * @param originator The MESH header originator address.
     * @param seqNo The BC0 header sequence number.
     */
    void RecordPacket(const Address& originator, uint8_t seqNo);

    /**
     * @brief React to a duplicate packet reception.
     *
     * Default implementation is a no-op. Subclasses may override to
     * react to duplicates (e.g., to modify the decisions on if / when to
     * forward the packet).
     *
     * @param originator The MESH header originator address.
     * @param seqNo The BC0 header sequence number.
     */
    virtual void OnDuplicateReceived(const Address& originator, uint8_t seqNo);

    /**
     * @brief Decide whether (and when) to forward a new mesh-under packet.
     *
     * The device calls this method when a new, non-duplicate mesh-under
     * packet must be considered for forwarding. The policy decides if
     * and when to invoke @p forwardCb to perform the actual rebroadcast.
     *
     * @param packet The mesh frame ready for retransmission (MESH and
     *               BC0 headers applied, hop count already decremented
     *               by the device).
     * @param originator The MESH header originator address.
     * @param seqNo The BC0 header sequence number.
     * @param hopsLeft The hop-count value the next-hop receiver
     *                 will see (already decremented by the device).
     * @param forwardCb The callback that performs the actual forward.
     */
    virtual void OnPacketForward(Ptr<Packet> packet,
                                 const Address& originator,
                                 uint8_t seqNo,
                                 uint8_t hopsLeft,
                                 ForwardCallback forwardCb) = 0;

    /**
     * @brief Assign a fixed random stream number to RNGs used by the policy.
     *
     * @param stream First stream index to use.
     * @return The number of stream indices assigned by this policy.
     */
    virtual int64_t AssignStreams(int64_t stream) = 0;

  protected:
    void DoDispose() override;

  private:
    /// Per-originator FIFO of recently seen BC0 sequence numbers.
    std::map<Address /* OriginatorAddress */, std::deque<uint8_t /* SequenceNumber */>> m_seenPkts;

    /// Maximum cache size per originator. Oldest entry dropped beyond this (FIFO).
    uint16_t m_meshCacheLength{10};
};

} // namespace ns3

#endif /* SIXLOWPAN_MESH_UNDER_ROUTING_H */
