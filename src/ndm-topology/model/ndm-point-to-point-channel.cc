/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-point-to-point-channel.h"

#include "ns3/ndm-link.h"

#include "ns3/log.h"
#include "ns3/packet.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NdmPointToPointChannel");

NS_OBJECT_ENSURE_REGISTERED(NdmPointToPointChannel);

TypeId
NdmPointToPointChannel::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NdmPointToPointChannel")
                            .SetParent<PointToPointChannel>()
                            .SetGroupName("NdmTopology")
                            .AddConstructor<NdmPointToPointChannel>();
    return tid;
}

NdmPointToPointChannel::NdmPointToPointChannel() = default;

NdmPointToPointChannel::~NdmPointToPointChannel() = default;

void
NdmPointToPointChannel::SetLink(Ptr<NdmLink> link)
{
    m_link = link;
}

Ptr<NdmLink>
NdmPointToPointChannel::GetLink() const
{
    return m_link;
}

bool
NdmPointToPointChannel::TransmitStart(Ptr<const Packet> p, Ptr<PointToPointNetDevice> src, Time txTime)
{
    NS_LOG_FUNCTION(this << p << src << txTime);

    if (m_link != nullptr && m_link->IsDown())
    {
        const InFlightPolicy policy = m_link->GetPolicy();
        bool forbidden;
        if (policy == InFlightPolicy::DELIVER_THEN_DROP)
        {
            // Queued-at-detection packets per direction may still go out.
            forbidden = m_link->ConsumeDtdAllowance(src) == 0;
        }
        else
        {
            forbidden = true;
        }
        if (forbidden)
        {
            NS_LOG_INFO("NdmPointToPointChannel: tx blocked (link " << m_link->GetId().index
                                                                    << " down, policy="
                                                                    << uint8_t(policy)
                                                                    << ", txTime=" << txTime << ")");
            m_link->CountTxBlocked();
            return false;
        }
        // DELIVER_THEN_DROP within the drain allowance: let it complete.
    }

    return PointToPointChannel::TransmitStart(p, src, txTime);
}

} // namespace ns3
