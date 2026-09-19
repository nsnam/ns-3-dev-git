/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/log.h"
#include "ns3/ndm-link.h"

#include "ns3/ndm-link-loss-model.h"
#include "ns3/ndm-point-to-point-channel.h"

#include "ns3/drop-tail-queue.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/queue.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NdmLink");

NS_OBJECT_ENSURE_REGISTERED(NdmLink);

TypeId
NdmLink::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NdmLink").SetParent<Object>().SetGroupName("NdmTopology").AddConstructor<NdmLink>();
    return tid;
}

NdmLink::NdmLink() = default;

NdmLink::~NdmLink() = default;

void
NdmLink::SetId(const NdmLinkId& id)
{
    m_id = id;
}

const NdmLinkId&
NdmLink::GetId() const
{
    return m_id;
}

void
NdmLink::SetEnds(const NdmPortIdentity& portA, const NdmPortIdentity& portB)
{
    NS_ASSERT_MSG(portA.node != portB.node || portA.port != portB.port,
                  "NdmLink: self-loop link");
    m_portA = portA;
    m_portB = portB;
}

const NdmPortIdentity&
NdmLink::GetPortA() const
{
    return m_portA;
}

const NdmPortIdentity&
NdmLink::GetPortB() const
{
    return m_portB;
}

NdmPortIdentity
NdmLink::GetOtherPort(const NdmPortIdentity& port) const
{
    if (port == m_portA)
    {
        return m_portB;
    }
    NS_ASSERT(port == m_portB);
    return m_portA;
}

void
NdmLink::SetDevices(Ptr<PointToPointNetDevice> devA, Ptr<PointToPointNetDevice> devB)
{
    m_devA = devA;
    m_devB = devB;
}

void
NdmLink::SetChannel(Ptr<NdmPointToPointChannel> channel)
{
    m_channel = channel;
}

Ptr<PointToPointNetDevice>
NdmLink::GetDevice(const NdmPortIdentity& port) const
{
    if (port == m_portA)
    {
        return m_devA;
    }
    NS_ASSERT(port == m_portB);
    return m_devB;
}

Ptr<Node>
NdmLink::GetNode(const NdmPortIdentity& port) const
{
    return GetDevice(port)->GetNode();
}

Ptr<NdmPointToPointChannel>
NdmLink::GetChannel() const
{
    return m_channel;
}

void
NdmLink::SetDataRate(DataRate bps)
{
    NS_ASSERT(m_devA && m_devB);
    m_devA->SetAttribute("DataRate", DataRateValue(bps));
    m_devB->SetAttribute("DataRate", DataRateValue(bps));
}

void
NdmLink::SetDelay(Time delay)
{
    m_delay = delay;
    NS_ASSERT(m_channel);
    m_channel->SetAttribute("Delay", TimeValue(delay));
}

Time
NdmLink::GetDelay() const
{
    return m_delay;
}

void
NdmLink::SetLossModel(Ptr<NdmLinkLossModel> loss)
{
    m_loss = loss;
}

NdmLink::State
NdmLink::GetState() const
{
    return m_state;
}

bool
NdmLink::IsDown() const
{
    return m_state == State::DOWN;
}

void
NdmLink::SetDownAt(Time tDetect, InFlightPolicy policy)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT_MSG(!IsDown(), "NdmLink: already down");
    m_state = State::DOWN;
    m_policy = policy;
    m_tDown = tDetect;

    if (policy == InFlightPolicy::FLUSH)
    {
        // Explicit active flush: drain both endpoint queues, counting the
        // flushed packets (queue statistics also see the Remove() calls).
        uint32_t flushed = 0;
        flushed += static_cast<uint32_t>(m_devA->GetQueue()->GetNPackets());
        m_devA->GetQueue()->Flush();
        flushed += static_cast<uint32_t>(m_devB->GetQueue()->GetNPackets());
        m_devB->GetQueue()->Flush();
        CountQueueFlushed(flushed);
    }
    else if (policy == InFlightPolicy::DELIVER_THEN_DROP)
    {
        // The queue contents at detection time may still be transmitted
        // (per direction); anything beyond is blocked by the channel.
        m_dtdAllowanceA = static_cast<uint32_t>(m_devA->GetQueue()->GetNPackets());
        m_dtdAllowanceB = static_cast<uint32_t>(m_devB->GetQueue()->GetNPackets());
    }
}

uint32_t
NdmLink::ConsumeDtdAllowance(Ptr<PointToPointNetDevice> endDev)
{
    if (!IsDown() || m_policy != InFlightPolicy::DELIVER_THEN_DROP)
    {
        return 1;
    }
    if (endDev == m_devA)
    {
        return m_dtdAllowanceA > 0 ? m_dtdAllowanceA-- : 0;
    }
    NS_ASSERT(endDev == m_devB);
    return m_dtdAllowanceB > 0 ? m_dtdAllowanceB-- : 0;
}

void
NdmLink::SetReadyAt(Time tReady)
{
    NS_LOG_FUNCTION_NOARGS();
    NS_ASSERT_MSG(IsDown(), "NdmLink: not down");
    (void)tReady;
    m_state = State::UP;
    m_tDown = Time(Seconds(0));
}

InFlightPolicy
NdmLink::GetPolicy() const
{
    return m_policy;
}

Time
NdmLink::GetTDown() const
{
    return m_tDown;
}

uint64_t
NdmLink::GetTxBlockedCount() const
{
    return m_txBlocked;
}

uint64_t
NdmLink::GetRxLostInFlightCount() const
{
    return m_rxLostInFlight;
}

uint64_t
NdmLink::GetQueueFlushedCount() const
{
    return m_queueFlushed;
}

uint64_t
NdmLink::GetQueueDepthSum(uint32_t& portADepth, uint32_t& portBDepth) const
{
    portADepth = m_devA->GetQueue()->GetNPackets();
    portBDepth = m_devB->GetQueue()->GetNPackets();
    return uint64_t(portADepth) + portBDepth;
}

void
NdmLink::CountTxBlocked()
{
    m_txBlocked++;
}

void
NdmLink::CountRxLostInFlight()
{
    m_rxLostInFlight++;
}

void
NdmLink::CountQueueFlushed(uint32_t n)
{
    m_queueFlushed += n;
}

} // namespace ns3
