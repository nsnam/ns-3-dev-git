/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmTopology: the NDM graph.
 *
 * - Explicit node identities (kind + semantic index) and (node, port)
 *   adjacency keys (see ndm-identity.h for why port keying matters).
 * - Every link is a real ns-3 point-to-point link (stock devices,
 *   NdmPointToPointChannel, per-device NdmRxGuard), so all traffic —
 *   today's path-forwarding test model, later RoCE/MRC — traverses real
 *   devices, queues and delays.
 * - Path queries (FindPaths) are deterministic BFS over currently-valid
 *   (UP) links, ordered by link index. They are a *query* service, not a
 *   routing protocol: no component here recomputes global routes on a
 *   failure (that timing is the failure framework's job, D6).
 *
 * PPS-safety (D7): no wall clock, no hash-ordered iteration on hot paths
 * (adjacency maps are std::map), no threads.
 */

#ifndef NDM_TOPOLOGY_H
#define NDM_TOPOLOGY_H

#include "ns3/ndm-identity.h"
#include "ns3/ndm-link.h"

#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/node-list.h"

#include <map>
#include <vector>

namespace ns3
{

class NdmLinkLossModel;
class PointToPointNetDevice;
class NdmPointToPointChannel;

class NdmTopology : public Object
{
  public:
    static TypeId GetTypeId();

    NdmTopology();
    ~NdmTopology() override;

    /// A directed path: m_nodes = [n0, n1, ..., n_k] and m_links[i]
    /// connects m_nodes[i] to m_nodes[i+1].
    struct NdmPath
    {
        std::vector<uint32_t> m_nodes;
        std::vector<NdmLinkId> m_links;
        /// True if every link on the path is currently UP.
        bool IsValid(const NdmTopology& topo) const;
    };

    // -- construction ---------------------------------------------------------
    /// Create a node of the given kind; node id == creation order.
    Ptr<Node> AddNode(NdmNodeKind kind, uint32_t semantic);
    /// Create a link between two ports; ports are assigned as the next free
    /// port on each node (device index at creation time). Returns the link.
    Ptr<NdmLink> AddLink(uint32_t nodeA, uint32_t nodeB,
                         DataRate bps, Time delay,
                         int32_t rail, int32_t plane, bool crossPlane,
                         uint32_t queueMaxPackets,
                         Ptr<NdmLinkLossModel> loss);

    // -- queries ---------------------------------------------------------------
    Ptr<Node> GetNode(uint32_t node) const;
    NdmNodeIdentity GetNodeIdentity(uint32_t node) const;
    uint32_t GetNodeCount() const;
    uint32_t GetLinkCount() const;
    Ptr<NdmLink> GetLink(const NdmLinkId& id) const;
    std::vector<Ptr<NdmLink>> GetNodeLinks(uint32_t node) const;   // sorted by link index
    std::vector<NdmLinkId> GetNodeLinkIds(uint32_t node) const;

    /// Port identities of the two ends of a link.
    NdmPortIdentity GetLinkPortA(const NdmLinkId& id) const;
    NdmPortIdentity GetLinkPortB(const NdmLinkId& id) const;

    // -- graph metrics (gate G-topo) --------------------------------------------
    uint32_t GetDegree(uint32_t node) const;
    uint32_t GetMaxDegree() const;
    /// Max shortest path (in links) over all node pairs, physical links only.
    uint32_t GetDiameter() const;
    /// Sum over node pairs of (parallelLinkCount - 1); 1 per extra parallel link.
    uint32_t GetParallelLinkCount() const;

    // -- paths -----------------------------------------------------------------
    /// Deterministic BFS over currently-UP links; empty if unreachable.
    /// Ordered: shortest first, then lexicographic by link index.
    std::vector<NdmPath> FindPaths(uint32_t src, uint32_t dst) const;

  private:
    NodeList m_nodes;
    std::map<uint32_t, NdmNodeIdentity> m_identities;

    std::map<NdmLinkId, Ptr<NdmLink>> m_links;
    // (node, port) -> link: the adjacency key (never a neighbor node pair).
    std::map<NdmPortIdentity, NdmLinkId> m_portToLink;

    uint64_t m_nextLinkId{0};
};

} // namespace ns3

#endif // NDM_TOPOLOGY_H
