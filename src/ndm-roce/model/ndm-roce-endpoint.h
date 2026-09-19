/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmRoceEndpoint: node-level RoCE binding.
 *
 * Binds the QP layer to an ndm-topology node:
 *  - Owns the local TX/RX QP tables (one pair per connection).
 *  - Wire framing (controlled-bypass decision, Phase 2 task 1):
 *        [IPv6 40B][UDP 8B (port 4791, RoCEv2)][NdmRoceBth 16B][payload]
 *    moved over the node's NdmPathForwarder on a registered path (the
 *    forwarder adds the per-hop PathHdr; the point-to-point link adds PPP
 *    + serialization). No ns-3 stack sockets are involved — documented
 *    controlled bypass, as in real RDMA.
 *  - Dispatch: packets delivered by the forwarder are decapsulated and
 *    routed by BTH destQP: data -> RX QP; ACK/NACK/CNP -> TX QP.
 *
 * One endpoint per node (the forwarder's delivery callback is per-node);
 * Attach asserts this.
 *
 * PPS-safety (D7): all state is QP tables (std::map on control paths —
 * setup, not hot path); data-path dispatch is a single map lookup per
 * packet; no threads, no wall clock.
 */

#ifndef NDM_ROCE_ENDPOINT_H
#define NDM_ROCE_ENDPOINT_H

#include "ns3/ipv6-address.h"
#include "ns3/ndm-path-forwarder.h"
#include "ns3/ndm-roce-qpair.h"
#include "ns3/ndm-roce-rx-qpair.h"
#include "ns3/node.h"
#include "ns3/ref-count.h"

#include <map>

namespace ns3
{

class NdmRoceEndpoint : public RefCount
{
  public:
    static constexpr uint16_t kRoceUdpPort = 4791; ///< RoCEv2 UDP port

    NdmRoceEndpoint();
    ~NdmRoceEndpoint();

    /// Wire the endpoint to the node's forwarder. `pathId` is the outgoing
    /// data path; `retPathId` the return path for ACK/NACK/CNP.
    void Attach(Ptr<Node> node, Ptr<NdmPathForwarder> fwd,
                uint32_t pathId, uint32_t retPathId);
    void SetAddresses(Ipv6Address local, Ipv6Address peer);

    /// Create and bind a QP pair for one connection (local<->peer).
    Ptr<NdmRoceQPair> CreateConnection(uint32_t localQpn, uint32_t peerQpn);

    Ptr<NdmRoceRxQPair> GetRxQp(uint32_t qpn) const;

    /// Gate evidence: count of control packets transmitted.
    uint64_t GetControlSentCount() const
    {
        return m_ctrlSent;
    }
    uint64_t GetControlRxCount() const
    {
        return m_ctrlRx;
    }

  private:
    void OnTxSend(Ptr<Packet> pkt, uint32_t destQp);
    void OnCtrlSend(Ptr<Packet> pkt, uint32_t destQp);
    void OnDelivered(uint32_t pathId, Ptr<const Packet> p, Time t);

    Ptr<Node> m_node;
    Ptr<NdmPathForwarder> m_fwd;
    uint32_t m_pathId{0};
    uint32_t m_retPathId{0};
    Ipv6Address m_local{Ipv6Address("fe80::1")};
    Ipv6Address m_peer{Ipv6Address("fe80::2")};

    std::map<uint32_t, Ptr<NdmRoceQPair>> m_tx;
    std::map<uint32_t, Ptr<NdmRoceRxQPair>> m_rx;

    uint64_t m_ctrlSent{0};
    uint64_t m_ctrlRx{0};
};

} // namespace ns3

#endif // NDM_ROCE_ENDPOINT_H
