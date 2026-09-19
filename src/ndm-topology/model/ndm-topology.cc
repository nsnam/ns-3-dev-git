/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/log.h"
#include "ns3/ndm-topology.h"

#include "ns3/ndm-link-loss-model.h"
#include "ns3/ndm-point-to-point-channel.h"
#include "ns3/ndm-rx-guard.h"

#include "ns3/pointer.h"
#include "ns3/uinteger.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/packet.h"
#include "ns3/point-to-point-net-device.h"
#include "ns3/queue.h"
#include "ns3/node.h"

#include <algorithm>
#include <queue>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NdmTopology");

NS_OBJECT_ENSURE_REGISTERED(NdmTopology);

TypeId
NdmTopology::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NdmTopology").SetParent<Object>().SetGroupName("NdmTopology").AddConstructor<NdmTopology>();
    return tid;
}

NdmTopology::NdmTopology() = default;

NdmTopology::~NdmTopology() = default;

bool
NdmTopology::NdmPath::IsValid(const NdmTopology& topo) const
{
    for (const auto& id : m_links)
    {
        if (topo.GetLink(id)->IsDown())
        {
            return false;
        }
    }
    return true;
}

Ptr<Node>
NdmTopology::AddNode(NdmNodeKind kind, uint32_t semantic)
{
    Ptr<Node> node = CreateObject<Node>();
    // Note: ns-3 node ids are process-global and do not restart between
    // simulations; NdmTopology indexes are relative to this object's first
    // node. Tests must use captured node ids, not absolute values.
    const uint32_t rel = m_nodes.size();
    m_nodes.push_back(node);
    m_identities[rel] = NdmNodeIdentity{node->GetId(), kind, semantic};
    return node;
}

Ptr<NdmLink>
NdmTopology::AddLink(uint32_t nodeA, uint32_t nodeB,
                     DataRate bps, Time delay,
                     int32_t rail, int32_t plane, bool crossPlane,
                     uint32_t queueMaxPackets,
                     Ptr<NdmLinkLossModel> loss)
{
    NS_LOG_FUNCTION(this << nodeA << nodeB);
    NS_ASSERT_MSG(nodeA != nodeB, "NdmTopology: self-loop link");
    NS_ASSERT_MSG(nodeA < m_nodes.size() && nodeB < m_nodes.size(),
                  "NdmTopology: unknown node");

    Ptr<Node> nA = m_nodes[nodeA];
    Ptr<Node> nB = m_nodes[nodeB];

    // Ports are the next free device index on each node (creation order).
    const uint32_t portA = nA->GetNDevices();
    const uint32_t portB = nB->GetNDevices();
    const NdmPortIdentity pA{nodeA, portA};
    const NdmPortIdentity pB{nodeB, portB};
    NS_ASSERT_MSG(m_portToLink.find(pA) == m_portToLink.end() &&
                      m_portToLink.find(pB) == m_portToLink.end(),
                  "NdmTopology: port already in use");

    // Channel + stock devices (no core patch; NdmPointToPointChannel only
    // overrides the virtual TransmitStart for failure enforcement).
    Ptr<NdmPointToPointChannel> channel = CreateObject<NdmPointToPointChannel>();
    Ptr<PointToPointNetDevice> devA = CreateObject<PointToPointNetDevice>();
    Ptr<PointToPointNetDevice> devB = CreateObject<PointToPointNetDevice>();

    auto makeQueue = [queueMaxPackets]() {
        Ptr<DropTailQueue<Packet>> q = CreateObject<DropTailQueue<Packet>>();
        q->SetAttribute("MaxPackets", UintegerValue(queueMaxPackets));
        return q;
    };
    devA->SetAttribute("TxQueue", PointerValue(makeQueue()));
    devB->SetAttribute("TxQueue", PointerValue(makeQueue()));
    devA->SetAttribute("InterframeGap", TimeValue(Seconds(0)));
    devB->SetAttribute("InterframeGap", TimeValue(Seconds(0)));

    Ptr<NdmLink> link = CreateObject<NdmLink>();
    NdmLinkId id{m_nextLinkId++, rail, plane, crossPlane};
    link->SetId(id);
    link->SetEnds(pA, pB);

    channel->SetLink(link);
    channel->Attach(devA);
    channel->Attach(devB);
    devA->SetAttribute("DataRate", DataRateValue(bps));
    devB->SetAttribute("DataRate", DataRateValue(bps));
    channel->SetAttribute("Delay", TimeValue(delay));
    link->SetDelay(delay);
    link->SetChannel(channel);
    link->SetDevices(devA, devB);

    // Receive guard: failure state (and chained stochastic loss).
    if (loss != nullptr)
    {
        link->SetLossModel(loss);
    }
    auto makeGuard = [this, link, loss]() {
        Ptr<NdmRxGuard> guard = CreateObject<NdmRxGuard>();
        guard->SetLink(link);
        guard->SetLossModel(loss);
        return guard;
    };
    devA->SetAttribute("ReceiveErrorModel", PointerValue(makeGuard()));
    devB->SetAttribute("ReceiveErrorModel", PointerValue(makeGuard()));

    NS_ASSERT(nA->AddDevice(devA) == portA);
    NS_ASSERT(nB->AddDevice(devB) == portB);

    m_links[id] = link;
    m_portToLink[pA] = id;
    m_portToLink[pB] = id;
    return link;
}

Ptr<Node>
NdmTopology::GetNode(uint32_t node) const
{
    return m_nodes[node];
}

NdmNodeIdentity
NdmTopology::GetNodeIdentity(uint32_t node) const
{
    return m_identities.at(node);
}

uint32_t
NdmTopology::GetNodeCount() const
{
    return m_nodes.size();
}

uint32_t
NdmTopology::GetLinkCount() const
{
    return m_links.size();
}

Ptr<NdmLink>
NdmTopology::GetLink(const NdmLinkId& id) const
{
    return m_links.at(id);
}

std::vector<NdmLinkId>
NdmTopology::GetNodeLinkIds(uint32_t node) const
{
    std::vector<NdmLinkId> out;
    Ptr<Node> n = m_nodes[node];
    for (uint32_t port = 0; port < n->GetNDevices(); port++)
    {
        auto it = m_portToLink.find(NdmPortIdentity{node, port});
        if (it != m_portToLink.end())
        {
            out.push_back(it->second);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<Ptr<NdmLink>>
NdmTopology::GetNodeLinks(uint32_t node) const
{
    std::vector<Ptr<NdmLink>> out;
    for (const auto& id : GetNodeLinkIds(node))
    {
        out.push_back(m_links.at(id));
    }
    return out;
}

NdmPortIdentity
NdmTopology::GetLinkPortA(const NdmLinkId& id) const
{
    return m_links.at(id)->GetPortA();
}

NdmPortIdentity
NdmTopology::GetLinkPortB(const NdmLinkId& id) const
{
    return m_links.at(id)->GetPortB();
}

uint32_t
NdmTopology::GetDegree(uint32_t node) const
{
    return GetNodeLinkIds(node).size();
}

uint32_t
NdmTopology::GetMaxDegree() const
{
    uint32_t mx = 0;
    for (uint32_t n = 0; n < GetNodeCount(); n++)
    {
        mx = std::max(mx, GetDegree(n));
    }
    return mx;
}

uint32_t
NdmTopology::GetDiameter() const
{
    const uint32_t n = GetNodeCount();
    uint32_t diameter = 0;
    for (uint32_t a = 0; a < n; a++)
    {
        std::vector<int32_t> dist(n, -1);
        dist[a] = 0;
        std::queue<uint32_t> q;
        q.push(a);
        while (!q.empty())
        {
            uint32_t u = q.front();
            q.pop();
            for (const auto& id : GetNodeLinkIds(u))
            {
                const Ptr<NdmLink> l = m_links.at(id);
                const auto pA = l->GetPortA();
                const auto pB = l->GetPortB();
                const uint32_t v = (pA.node == u) ? pB.node : pA.node;
                if (dist[v] < 0)
                {
                    dist[v] = dist[u] + 1;
                    diameter = std::max(diameter, uint32_t(dist[v]));
                    q.push(v);
                }
            }
        }
    }
    return diameter;
}

uint32_t
NdmTopology::GetParallelLinkCount() const
{
    std::map<std::pair<uint32_t, uint32_t>, uint32_t> pairs;
    for (const auto& [id, link] : m_links)
    {
        const auto pA = link->GetPortA();
        const auto pB = link->GetPortB();
        const std::pair<uint32_t, uint32_t> key =
            pA.node < pB.node ? std::make_pair(pA.node, pB.node)
                              : std::make_pair(pB.node, pA.node);
        pairs[key]++;
    }
    uint32_t extra = 0;
    for (const auto& [key, count] : pairs)
    {
        if (count > 1)
        {
            extra += count - 1;
        }
    }
    return extra;
}

std::vector<NdmTopology::NdmPath>
NdmTopology::FindPaths(uint32_t src, uint32_t dst) const
{
    std::vector<NdmPath> result;
    if (src == dst)
    {
        result.push_back(NdmPath{{src}, {}});
        return result;
    }

    struct State
    {
        uint32_t node;
        uint64_t lastLink; // NdmLinkId::index of the link used to reach `node`
        std::vector<uint32_t> nodes;
        std::vector<NdmLinkId> links;
    };

    constexpr uint64_t kNoLink = UINT64_MAX;
    std::queue<State> q;
    std::map<std::pair<uint32_t, uint64_t>, bool> visited;
    q.push(State{src, kNoLink, {src}, {}});
    visited.emplace(std::make_pair(src, kNoLink), true);

    constexpr uint32_t kMaxPaths = 1000;
    while (!q.empty() && result.size() < kMaxPaths)
    {
        State s = q.front();
        q.pop();
        for (const auto& id : GetNodeLinkIds(s.node))
        {
            const Ptr<NdmLink> l = m_links.at(id);
            if (l->IsDown())
            {
                continue; // only currently-valid links
            }
            const auto pA = l->GetPortA();
            const auto pB = l->GetPortB();
            const uint32_t v = (pA.node == s.node) ? pB.node : pA.node;

            if (v == dst)
            {
                NdmPath p;
                p.m_nodes = s.nodes;
                p.m_nodes.push_back(dst);
                p.m_links = s.links;
                p.m_links.push_back(id);
                result.push_back(std::move(p));
                continue; // do not extend past the destination
            }
            const auto key = std::make_pair(v, id.index);
            if (!visited.count(key))
            {
                visited.emplace(key, true);
                State next;
                next.node = v;
                next.lastLink = id.index;
                next.nodes = s.nodes;
                next.nodes.push_back(v);
                next.links = s.links;
                next.links.push_back(id);
                q.push(std::move(next));
            }
        }
    }
    return result;
}

} // namespace ns3
