/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmPathForwarder: packet-triggered path forwarding test-traffic model.
 *
 * Used by the failure-framework gates (G-fail) to move payload over a
 * registered path: the sender puts a small on-wire header (pathId, hop) in
 * the packet; each intermediate node's forwarder forwards the packet on
 * the next link of the path; the destination node delivers.
 *
 * Design notes:
 *  - Pure packet-triggered: no timers, no threads (PPS-safe, D7); every
 *    state transition is caused by a packet arrival.
 *  - Path state is stored (registered), not carried: the on-wire header is
 *    8 bytes, not the path itself.
 *  - One forwarder per node (it owns the receive callbacks of the node's
 *    link devices). Later phases' real transports (RoCE/MRC) replace this
 *    model; the identity/path abstractions stay.
 */

#ifndef NDM_PATH_FORWARDER_H
#define NDM_PATH_FORWARDER_H

#include "ns3/ndm-topology.h"

#include "ns3/address.h"
#include "ns3/header.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

#include <map>
#include <vector>

namespace ns3
{

class NdmPathForwarder : public Object
{
  public:
    static TypeId GetTypeId();

    NdmPathForwarder();
    ~NdmPathForwarder() override;

    /// Wire the forwarder to all links of node `nodeIndex` (relative index
    /// into the topology) in `topo`.
    void Attach(Ptr<NdmTopology> topo, uint32_t nodeIndex);

    /// Register a path (validity not required at registration; Send checks
    /// link state hop by hop). Returns the path id used in on-wire headers.
    uint32_t RegisterPath(const NdmTopology::NdmPath& path);
    const NdmTopology::NdmPath& GetPath(uint32_t pathId) const;
    uint32_t GetPathCount() const;

    /// Send a payload of `payloadBytes` along the path (hop 0).
    /// Returns false without sending if the first link is down.
    bool Send(uint32_t pathId, uint32_t payloadBytes);

    // -- gate evidence --------------------------------------------------------
    uint32_t GetSentCount() const;
    uint32_t GetForwardedCount() const;
    uint32_t GetDeliveredCount() const;
    uint32_t GetDroppedMidPathCount() const; // next hop's link was down
    const std::vector<Time>& GetDeliveryTimes() const;
    const std::vector<uint32_t>& GetDeliveryPathIds() const;
    /// Per-link receive record: how many packets arrived over each link and
    /// the time of the last such arrival.
    struct LinkRx
    {
        uint64_t count{0};
        Time firstTime{Seconds(0)};
        Time lastTime{Seconds(0)};
    };
    const std::map<NdmLinkId, LinkRx>& GetLinkRx() const;

    /// Fired at the destination node: (pathId, payloadBytes, arrival time).
    TracedCallback<uint32_t, uint32_t, Time> m_deliveryTraced;

  private:
    /// On-wire path header: which registered path, which hop just completed.
    struct PathHdr
        : public Header
    {
        static TypeId GetTypeId();
        PathHdr() = default;
        PathHdr(uint32_t pathId, uint32_t hop)
            : m_pathId(pathId),
              m_hop(hop)
        {
        }
        uint32_t m_pathId{0};
        uint32_t m_hop{0};
        uint32_t GetSerializedSize() const override
        {
            return 8;
        }
        void Serialize(Buffer::Iterator start) const override
        {
            start.WriteHtonU32(m_pathId);
            start.WriteHtonU32(m_hop);
        }
        uint32_t Deserialize(Buffer::Iterator start) override
        {
            m_pathId = start.ReadNtohU32();
            m_hop = start.ReadNtohU32();
            return 8;
        }
    };

    // ReceiveCallback signature (3.42: bool, Ptr<NetDevice>, Ptr<const Packet>,
    // uint16_t, const Address&). Returns true (handled) / false (drop).
    bool HandleRx(Ptr<NetDevice>, Ptr<const Packet> p, uint16_t, const Address&);

    Ptr<NdmTopology> m_topo;
    Ptr<Node> m_node;
    std::map<Ptr<NetDevice>, NdmLinkId> m_devToLink;
    std::map<NdmLinkId, Ptr<NetDevice>> m_linkToDev;
    std::vector<NdmTopology::NdmPath> m_paths;

    uint32_t m_sent{0};
    uint32_t m_forwarded{0};
    uint32_t m_delivered{0};
    uint32_t m_droppedMidPath{0};
    std::vector<Time> m_deliveryTimes;
    std::vector<uint32_t> m_deliveryPathIds;
    std::map<NdmLinkId, LinkRx> m_linkRx;
};

} // namespace ns3

#endif // NDM_PATH_FORWARDER_H
