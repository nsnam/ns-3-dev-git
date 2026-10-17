/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#ifndef SIXLOWPAN_SIMPLE_FLOODING_H
#define SIXLOWPAN_SIMPLE_FLOODING_H

#include "sixlowpan-mesh-under-routing.h"

#include "ns3/random-variable-stream.h"

namespace ns3
{

/**
 * @ingroup sixlowpan
 *
 * @brief Default mesh-under forwarding policy: unconditional rebroadcast with jitter.
 *
 * SixLowPanSimpleFlooding preserves the historical 6LoWPAN mesh-under
 * behavior of unconditionally rebroadcasting every received (non-duplicate)
 * mesh-under packet after a uniform random jitter (default Uniform[0, 10ms]).
 *
 * No suppression. No listening. No counters. This policy is the default,
 * so that the policy framework preserves the behavior of the previous
 * in-device flooding implementation when no policy is explicitly
 * selected by the user.
 */
class SixLowPanSimpleFlooding : public SixLowPanMeshUnderRouting
{
  public:
    /**
     * @brief Get the type ID.
     * @return The object TypeId.
     */
    static TypeId GetTypeId();

    SixLowPanSimpleFlooding();
    ~SixLowPanSimpleFlooding() override;

    // Delete copy constructor and assignment operator to avoid misuse.
    SixLowPanSimpleFlooding(const SixLowPanSimpleFlooding&) = delete;
    SixLowPanSimpleFlooding& operator=(const SixLowPanSimpleFlooding&) = delete;

    void OnPacketForward(Ptr<Packet> packet,
                         const Address& originator,
                         uint8_t seqNo,
                         uint8_t hopsLeft,
                         ForwardCallback forwardCb) override;

    int64_t AssignStreams(int64_t stream) override;

  protected:
    void DoDispose() override;

  private:
    /// Random variable (in milliseconds) used to schedule each rebroadcast.
    Ptr<RandomVariableStream> m_jitter;
};

} // namespace ns3

#endif /* SIXLOWPAN_SIMPLE_FLOODING_H */
