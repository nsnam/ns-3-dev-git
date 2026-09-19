/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmLink: one physical point-to-point link of the NDM topology.
 *
 * A link owns its channel (NdmPointToPointChannel) and its two stock
 * ns3::PointToPointNetDevices (one per end). Failure effects (down state,
 * in-flight policy, queue flush) are applied to this object by
 * NdmFailureScheduler (Phase 1); the channel and the per-device receive
 * guard (NdmRxGuard) read the state back, so the enforcement happens at
 * the point-to-point layer and is visible to any traffic model.
 *
 * Counters are gate evidence for G-fail (in-flight policy honored, no
 * delivery after T+detection, queue flush observable).
 */

#ifndef NDM_LINK_H
#define NDM_LINK_H

#include "ndm-identity.h"

#include "ns3/data-rate.h"
#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/ptr.h"

namespace ns3
{

class NetDevice;
class PointToPointNetDevice;
class NdmPointToPointChannel;
class NdmLinkLossModel;

class NdmLink : public Object
{
  public:
    static TypeId GetTypeId();

    NdmLink();
    ~NdmLink() override;

    enum class State : uint8_t
    {
        UP,
        DOWN
    };

    // -- identity (set by NdmTopology::AddLink) ----------------------------
    void SetId(const NdmLinkId& id);
    const NdmLinkId& GetId() const;
    void SetEnds(const NdmPortIdentity& portA, const NdmPortIdentity& portB);
    const NdmPortIdentity& GetPortA() const;
    const NdmPortIdentity& GetPortB() const;
    NdmPortIdentity GetOtherPort(const NdmPortIdentity& port) const;

    // -- devices / channel --------------------------------------------------
    void SetDevices(Ptr<PointToPointNetDevice> devA, Ptr<PointToPointNetDevice> devB);
    void SetChannel(Ptr<NdmPointToPointChannel> channel);
    Ptr<PointToPointNetDevice> GetDevice(const NdmPortIdentity& port) const;
    Ptr<Node> GetNode(const NdmPortIdentity& port) const;
    Ptr<NdmPointToPointChannel> GetChannel() const;

    // -- configuration -------------------------------------------------------
    void SetDataRate(DataRate bps);
    void SetDelay(Time delay);
    Time GetDelay() const;
    /// Attach an optional stochastic loss model (Bernoulli/burst) that is
    /// consulted by the receive guard while the link is UP.
    void SetLossModel(Ptr<NdmLinkLossModel> loss);

    // -- failure state (driven by NdmFailureScheduler) -----------------------
    State GetState() const;
    bool IsDown() const;
    /// Enter the DOWN state at detection time `tDetect` under `policy`.
    ///  - FLUSH additionally flushes (counts) both endpoint queues here.
    ///  - DELIVER_THEN_DROP records the per-end queue depths as a drain
    ///    allowance: the channel lets exactly that many queued packets per
    ///    direction complete, then blocks.
    void SetDownAt(Time tDetect, InFlightPolicy policy);
    /// Leave the DOWN state; the link is usable again from `tReady`.
    void SetReadyAt(Time tReady);
    InFlightPolicy GetPolicy() const;
    Time GetTDown() const;
    /// Per-end DELIVER_THEN_DROP drain allowance (consumed by the channel on
    /// each transmission attempt from that end). Returns 1 if the
    /// transmission may proceed (allowance decremented), 0 if it must be
    /// blocked. Only meaningful while down under DELIVER_THEN_DROP.
    uint32_t ConsumeDtdAllowance(Ptr<PointToPointNetDevice> endDev);

    // -- gate-evidence counters ----------------------------------------------
    uint64_t GetTxBlockedCount() const;     //!< transmissions blocked by the channel while DOWN
    uint64_t GetRxLostInFlightCount() const; //!< in-flight packets dropped by the RX guard
    uint64_t GetQueueFlushedCount() const;  //!< packets flushed from endpoint queues (FLUSH)
    uint64_t GetQueueDepthSum(uint32_t& portADepth, uint32_t& portBDepth) const;

  private:
    NdmLinkId m_id;
    NdmPortIdentity m_portA;
    NdmPortIdentity m_portB;

    Ptr<PointToPointNetDevice> m_devA;
    Ptr<PointToPointNetDevice> m_devB;
    Ptr<NdmPointToPointChannel> m_channel;
    Ptr<NdmLinkLossModel> m_loss;

    State m_state{State::UP};
    InFlightPolicy m_policy{InFlightPolicy::DROP};
    Time m_tDown{Seconds(0)};
    Time m_delay{Seconds(0)};
    uint32_t m_dtdAllowanceA{0};
    uint32_t m_dtdAllowanceB{0};

    uint64_t m_txBlocked{0};
    uint64_t m_rxLostInFlight{0};
    uint64_t m_queueFlushed{0};

    // counter sinks for the guard/channel (private, exposed via getters)
    friend class NdmPointToPointChannel;
    friend class NdmRxGuard;
    void CountTxBlocked();
    void CountRxLostInFlight();
    void CountQueueFlushed(uint32_t n);
};

} // namespace ns3

#endif // NDM_LINK_H
