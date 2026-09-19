/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-path-forwarder.h"

#include "ns3/ndm-link.h"

#include "ns3/log.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NdmPathForwarder");

NS_OBJECT_ENSURE_REGISTERED(NdmPathForwarder);

TypeId
NdmPathForwarder::PathHdr::GetTypeId()
{
    static TypeId tid =
        TypeId("ndm::NdmPathForwarder::PathHdr").SetParent<Header>().AddConstructor<PathHdr>();
    return tid;
}

TypeId
NdmPathForwarder::PathHdr::GetInstanceTypeId() const
{
    return GetTypeId();
}

TypeId
NdmPathForwarder::GetTypeId()
{
    static TypeId tid = TypeId("ns3::NdmPathForwarder")
                            .SetParent<Object>()
                            .SetGroupName("NdmTopology")
                            .AddConstructor<NdmPathForwarder>()
                            .AddTraceSource("Delivery",
                                            "Path delivery at the destination node: "
                                            "(pathId, payloadBytes, arrivalTime)",
                                            MakeTraceSourceAccessor(&NdmPathForwarder::m_deliveryTraced),
                                            "ns3::TracedCallback<uint32_t, uint32_t, Time>");
    return tid;
}

NdmPathForwarder::NdmPathForwarder() = default;

NdmPathForwarder::~NdmPathForwarder() = default;

void
NdmPathForwarder::Attach(Ptr<NdmTopology> topo, uint32_t nodeIndex)
{
    NS_LOG_FUNCTION(this << topo << nodeIndex);
    NS_ASSERT_MSG(m_topo == nullptr, "NdmPathForwarder: already attached");
    m_topo = topo;
    m_node = m_topo->GetNode(nodeIndex);
    const uint32_t nodeId = m_node->GetId();
    for (const auto& id : m_topo->GetNodeLinkIds(nodeIndex))
    {
        const Ptr<NdmLink> link = m_topo->GetLink(id);
        // The device on this node is the end whose port.node matches:
        Ptr<NetDevice> dev = (link->GetPortA().node == nodeId) ? link->GetDevice(link->GetPortA())
                                                               : link->GetDevice(link->GetPortB());
        m_linkToDev[id] = dev;
        m_devToLink[dev] = id;
        dev->SetReceiveCallback(MakeCallback(&NdmPathForwarder::HandleRx, this));
    }
}

uint32_t
NdmPathForwarder::RegisterPath(const NdmTopology::NdmPath& path)
{
    m_paths.push_back(path);
    return m_paths.size() - 1;
}

const NdmTopology::NdmPath&
NdmPathForwarder::GetPath(uint32_t pathId) const
{
    return m_paths.at(pathId);
}

uint32_t
NdmPathForwarder::GetPathCount() const
{
    return m_paths.size();
}

bool
NdmPathForwarder::Send(uint32_t pathId, uint32_t payloadBytes)
{
    NS_ASSERT_MSG(pathId < m_paths.size(), "NdmPathForwarder: unknown path");
    const NdmTopology::NdmPath& path = m_paths.at(pathId);
    NS_ASSERT_MSG(path.m_nodes.front() == m_node->GetId(),
                  "NdmPathForwarder: path does not start at this node");
    if (path.m_links.empty())
    {
        // Local "delivery" (same node) — not used by the gates.
        NS_ABORT_MSG("NdmPathForwarder: empty path not supported");
    }

    const NdmLinkId& first = path.m_links.front();
    if (m_topo->GetLink(first)->IsDown())
    {
        return false;
    }

    Ptr<Packet> p = Create<Packet>(payloadBytes);
    PathHdr hdr(pathId, 0);
    p->AddHeader(hdr);
    Ptr<NetDevice> dev = m_linkToDev.at(first);
    const bool ok = dev->Send(p, Mac48Address("00:00:00:00:00:02"), 0x0021);
    if (!ok)
    {
        return false;
    }
    m_sent++;
    return true;
}

uint32_t
NdmPathForwarder::GetSentCount() const
{
    return m_sent;
}

uint32_t
NdmPathForwarder::GetForwardedCount() const
{
    return m_forwarded;
}

uint32_t
NdmPathForwarder::GetDeliveredCount() const
{
    return m_delivered;
}

uint32_t
NdmPathForwarder::GetDroppedMidPathCount() const
{
    return m_droppedMidPath;
}

const std::vector<Time>&
NdmPathForwarder::GetDeliveryTimes() const
{
    return m_deliveryTimes;
}

const std::vector<uint32_t>&
NdmPathForwarder::GetDeliveryPathIds() const
{
    return m_deliveryPathIds;
}

const std::map<NdmLinkId, NdmPathForwarder::LinkRx>&
NdmPathForwarder::GetLinkRx() const
{
    return m_linkRx;
}

bool
NdmPathForwarder::HandleRx(Ptr<NetDevice> dev, Ptr<const Packet> p, uint16_t, const Address&)
{
    auto it = m_devToLink.find(dev);
    NS_ASSERT_MSG(it != m_devToLink.end(), "NdmPathForwarder: rx on unknown device");
    const NdmLinkId rxLink = it->second;

    PathHdr hdr;
    p->PeekHeader(hdr);
    NS_ASSERT_MSG(hdr.m_pathId < m_paths.size(), "NdmPathForwarder: unknown path in packet");
    const NdmTopology::NdmPath& path = m_paths.at(hdr.m_pathId);
    NS_ASSERT_MSG(hdr.m_hop < path.m_links.size(), "NdmPathForwarder: hop out of range");
    NS_ASSERT_MSG(path.m_links.at(hdr.m_hop) == rxLink,
                  "NdmPathForwarder: packet arrived on unexpected link");

    LinkRx& rx = m_linkRx[rxLink];
    rx.count++;
    if (rx.count == 1)
    {
        rx.firstTime = Simulator::Now();
    }
    rx.lastTime = Simulator::Now();

    if (hdr.m_hop + 1 == path.m_links.size())
    {
        // Destination. The 8-byte PathHdr stays in the packet (it is
        // consumed here, not on the wire again); payload = size - header.
        m_delivered++;
        m_deliveryTimes.push_back(Simulator::Now());
        m_deliveryPathIds.push_back(hdr.m_pathId);
        const uint32_t hdrSize = PathHdr().GetSerializedSize();
        NS_ASSERT_MSG(p->GetSize() >= hdrSize, "NdmPathForwarder: short delivery packet");
        m_deliveryTraced(hdr.m_pathId, p->GetSize() - hdrSize, Simulator::Now());
        return true;
    }

    // Forward on the next hop.
    const NdmLinkId next = path.m_links.at(hdr.m_hop + 1);
    if (m_topo->GetLink(next)->IsDown())
    {
        NS_LOG_INFO("NdmPathForwarder: mid-path drop (next link " << next.index << " down)");
        m_droppedMidPath++;
        return false;
    }
    Ptr<Packet> fp = Create<Packet>(p->GetSize() - PathHdr().GetSerializedSize());
    PathHdr fhdr(hdr.m_pathId, hdr.m_hop + 1);
    fp->AddHeader(fhdr);
    Ptr<NetDevice> outDev = m_linkToDev.at(next);
    const bool ok = outDev->Send(fp, Mac48Address("00:00:00:00:00:02"), 0x0021);
    if (!ok)
    {
        m_droppedMidPath++;
        return false;
    }
    m_forwarded++;
    return true;
}

} // namespace ns3
