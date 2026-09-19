/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NDM identity model (Phase 1, task 1).
 *
 * Explicit host/GPU/NIC/port/link/rail/plane identities. Adjacency in
 * NdmTopology is keyed by (node, port) — i.e. by NdmPortIdentity — so that
 * parallel physical links between the same pair of nodes are first-class
 * (each gets its own port on each end). This deliberately avoids the donor
 * bug that keyed adjacency by neighbor node pair, which overwrites parallel
 * link state (audit: astra-network-ns3 scratch/common.h:670-672).
 */

#ifndef NDM_IDENTITY_H
#define NDM_IDENTITY_H

#include <cstdint>
#include <iostream>

namespace ns3
{

/// Kind of a topology node. GPU and NV_SWITCH are declared now (used from
/// Phase 5's NVSwitch mode); Phase 1 populates HOST and SWITCH.
enum class NdmNodeKind : uint8_t
{
    HOST,     ///< end host (one or more NIC ports)
    GPU,      ///< GPU-resident endpoint (rail-attached NICs, one per rail)
    SWITCH,   ///< leaf/spine/ToR switch
    NV_SWITCH ///< intra-node NVSwitch (Phase 5)
};

/// Identity of a node: its ns-3 node id plus kind and semantic index
/// (host/gpu ordinal, or switch ordinal within its kind).
struct NdmNodeIdentity
{
    uint32_t node{0};      //!< ns-3 Node id
    NdmNodeKind kind{NdmNodeKind::HOST};
    uint32_t semantic{0};  //!< kind-specific ordinal
};

/// Identity of a port: (node, port). The adjacency key — never a neighbor
/// node pair. Two links between the same node pair occupy different ports.
struct NdmPortIdentity
{
    uint32_t node{0};
    uint32_t port{0};

    bool operator==(const NdmPortIdentity& o) const
    {
        return node == o.node && port == o.port;
    }
    bool operator!=(const NdmPortIdentity& o) const
    {
        return !(*this == o);
    }
    bool operator<(const NdmPortIdentity& o) const
    {
        return node != o.node ? node < o.node : port < o.port;
    }
};

/// Identity of a link. `index` is unique within one topology; `rail` and
/// `plane` are -1 when not applicable. `crossPlane` marks an explicit
/// cross-plane interconnect (multi-plane topology).
struct NdmLinkId
{
    uint64_t index{0};
    int32_t rail{-1};
    int32_t plane{-1};
    bool crossPlane{false};

    bool operator==(const NdmLinkId& o) const
    {
        return index == o.index;
    }
    bool operator!=(const NdmLinkId& o) const
    {
        return !(*this == o);
    }
    bool operator<(const NdmLinkId& o) const
    {
        return index < o.index;
    }
};

/// In-flight packet policy for a failure event (AGENTS.md D6).
///  - DROP: the link dies at detection time; in-flight packets are lost and
///    no further transmission succeeds (queued packets are lost as the
///    device drains into the dead channel).
///  - FLUSH: like DROP, but the endpoint queues are explicitly flushed at
///    detection time (purge is observable in queue accounting).
///  - DELIVER_THEN_DROP: packets already on the wire and packets queued at
///    detection time complete delivery; only later transmissions are
///    dropped.
enum class InFlightPolicy : uint8_t
{
    DROP,
    FLUSH,
    DELIVER_THEN_DROP
};

inline std::ostream&
operator<<(std::ostream& os, const NdmPortIdentity& p)
{
    return os << "(node=" << p.node << ", port=" << p.port << ")";
}

inline std::ostream&
operator<<(std::ostream& os, const NdmLinkId& l)
{
    return os << "{index=" << l.index << ", rail=" << l.rail << ", plane=" << l.plane
              << ", crossPlane=" << l.crossPlane << "}";
}

inline std::ostream&
operator<<(std::ostream& os, const NdmNodeIdentity& n)
{
    return os << "{node=" << n.node << ", kind=" << uint8_t(n.kind) << ", semantic="
              << n.semantic << "}";
}

} // namespace ns3

#endif // NDM_IDENTITY_H
