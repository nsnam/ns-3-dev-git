/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmTopologyHelper: parameterized topology generators (Phase 1, task 2).
 *
 * All generators build on NdmTopology::AddLink, so every generated link is
 * a real ns-3 point-to-point link with its own (node, port) identities.
 *
 * Construction summary (all counts/degrees are asserted in the unit tests,
 * gate G-topo):
 *
 *  CreateFatTree(k, d)              — 2-tier leaf/spine fat-tree
 *    k even >= 4, d even >= 2
 *    ToRs   L = d*(k/2);  hosts H = d*(k/2)*(k/2);  spines S = d*k/4
 *    ToR(p, j):  k/2 host downlinks + k/2 spine uplinks (degree k)
 *    spine s:    k downlinks, to ToRs (s*k + t) mod L, t in [0, k)
 *      (round-robin; uniform: every ToR gets exactly k/2 distinct spines)
 *    diameter 4 (host-ToR-spine-ToR-host)
 *
 *  CreateMultiRail(pods, gpus, rails, spines, nicPerRail)
 *    one independent 2-tier network per rail r:
 *      GPU(p, g) -- ToR_r(p) -- Spine_r(s)  (complete ToR<->Spine per rail)
 *    links carry rail = r; GPU degree = rails*nicPerRail;
 *    nicPerRail > 1 creates parallel links (adjacency must survive that).
 *
 *  CreateMultiPlane(planes, kind, hosts, k, d, crossPlane)
 *    P independent planes over one shared host set:
 *      RING:     host_i -- S_{p,i}, ring S_{p,i} -- S_{p,(i+1) mod hosts}
 *      FAT_TREE: plane p embeds CreateFatTree(k, d) switches over the same
 *                shared hosts (host count must equal d*(k/2)^2)
 *    plane links carry plane = p; optional cross-plane links join equal
 *    switch ordinals of adjacent planes (crossPlane = true, rail/plane -1).
 */

#ifndef NDM_TOPOLOGY_HELPER_H
#define NDM_TOPOLOGY_HELPER_H

#include "ns3/ndm-link-loss-model.h"
#include "ns3/ndm-topology.h"

#include "ns3/data-rate.h"
#include "ns3/nstime.h"

#include <cstdint>

namespace ns3
{

class NdmTopologyHelper
{
  public:
    enum class PlaneKind : uint8_t
    {
        RING,
        FAT_TREE
    };

    /// Per-link physical parameters shared by all generator links.
    struct Opts
    {
        DataRate bps{DataRate("1Gbps")};
        Time delay{MilliSeconds(1)};
        uint32_t queueMaxPackets{1000};
        // Stochastic loss (lossy Ethernet, D5). NONE keeps links lossless-queue.
        NdmLinkLossModel::Mode lossMode{NdmLinkLossModel::Mode::NONE};
        double lossProbability{0.0};
        uint32_t burstLen{1};
        /// Base seed; each link i gets RNG seed `seed + i` (determinism D7).
        uint64_t seed{0};
    };

    static Ptr<NdmTopology> CreateFatTree(uint32_t k, uint32_t d, const Opts& opts);
    static Ptr<NdmTopology> CreateMultiRail(uint32_t pods,
                                            uint32_t gpusPerPod,
                                            uint32_t rails,
                                            uint32_t spinesPerRail,
                                            uint32_t nicPerRail,
                                            const Opts& opts);
    static Ptr<NdmTopology> CreateMultiPlane(uint32_t planes,
                                             PlaneKind kind,
                                             uint32_t ringHosts,
                                             uint32_t fatTreeK,
                                             uint32_t fatTreeD,
                                             bool crossPlane,
                                             const Opts& opts);

    /// Build the loss model for link index `linkIndex` from the options
    /// (fresh RngStream seeded seed+linkIndex). Returns nullptr for NONE.
    static Ptr<NdmLinkLossModel> MakeLinkLoss(const Opts& opts, uint64_t linkIndex);
};

} // namespace ns3

#endif // NDM_TOPOLOGY_HELPER_H
