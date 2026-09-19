/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmRxGuard: the per-device receive error model that enforces a link's
 * failure state for packets already in flight (scheduled Receive events)
 * and chains the optional stochastic loss model.
 *
 * Stock ns3::PointToPointNetDevice::Receive drops a packet when its
 * receive error model reports it corrupt, so attaching this guard is the
 * whole mechanism — no core patch, no tag mutation.
 *
 * Semantics (mirrors NdmPointToPointChannel on the transmit side):
 *  - link DOWN + DROP/FLUSH: every receive while down is dropped
 *    (in-flight loss).
 *  - link DOWN + DELIVER_THEN_DROP: a receive is dropped only if its
 *    transmission started at/after the detection time. The receive occurs
 *    exactly `delay` after the transmit, so launch time = Now() - delay.
 *  - link UP: delegate to the loss model (if any).
 */

#ifndef NDM_RX_GUARD_H
#define NDM_RX_GUARD_H

#include "ns3/error-model.h"
#include "ns3/nstime.h"

namespace ns3
{

class NdmLink;
class NdmLinkLossModel;

class NdmRxGuard : public ErrorModel
{
  public:
    static TypeId GetTypeId();

    NdmRxGuard();
    ~NdmRxGuard() override;

    /// Back-pointer to the link (state + detection time + delay).
    void SetLink(Ptr<NdmLink> link);
    /// Optional stochastic loss applied while the link is UP.
    void SetLossModel(Ptr<NdmLinkLossModel> loss);

    bool IsCorrupt(Ptr<const Packet> packet) override;

  private:
    Ptr<NdmLink> m_link;
    Ptr<NdmLinkLossModel> m_loss;
};

} // namespace ns3

#endif // NDM_RX_GUARD_H
