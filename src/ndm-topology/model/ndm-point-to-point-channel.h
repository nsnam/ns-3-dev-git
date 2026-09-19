/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmPointToPointChannel: PointToPointChannel that enforces the link's
 * failure state at transmit start.
 *
 * Enforcement contract (together with NdmRxGuard on the receive side):
 *  - DOWN + DROP / FLUSH: every TransmitStart is blocked while the link is
 *    down (queued packets drain into the dead channel and are lost; FLUSH
 *    additionally purges the endpoint queues at detection time).
 *  - DOWN + DELIVER_THEN_DROP: packets already on the wire complete, and
 *    exactly the packets queued at detection time (per direction) are still
 *    transmitted (drain allowance); all further transmissions are blocked.
 *
 * No core patch is involved: this is a subclass overriding the virtual
 * PointToPointChannel::TransmitStart. The stock device tolerates a `false`
 * return (fires MacTxDrop/PhyTxDrop traces, state machine proceeds).
 */

#ifndef NDM_POINT_TO_POINT_CHANNEL_H
#define NDM_POINT_TO_POINT_CHANNEL_H

#include "ns3/point-to-point-channel.h"

namespace ns3
{

class NdmLink;

class NdmPointToPointChannel : public PointToPointChannel
{
  public:
    static TypeId GetTypeId();

    NdmPointToPointChannel();
    ~NdmPointToPointChannel() override;

    /// Back-pointer set by NdmTopology before the link is used.
    void SetLink(Ptr<NdmLink> link);
    Ptr<NdmLink> GetLink() const;

    /// Block transmissions that the link's failure state forbids.
    bool TransmitStart(Ptr<const Packet> p, Ptr<PointToPointNetDevice> src, Time txTime) override;

  private:
    Ptr<NdmLink> m_link;
};

} // namespace ns3

#endif // NDM_POINT_TO_POINT_CHANNEL_H
