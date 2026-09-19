/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/ndm-failure-scheduler.h"

#include "ns3/ndm-link.h"
#include "ns3/ndm-topology.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("NdmFailureScheduler");

NS_OBJECT_ENSURE_REGISTERED(NdmFailureScheduler);

TypeId
NdmFailureScheduler::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::NdmFailureScheduler")
            .SetParent<Object>()
            .SetGroupName("NdmFailure")
            .AddConstructor<NdmFailureScheduler>()
            .AddTraceSource("LinkFailed",
                            "Link failure state applied: (linkId, tDetect)",
                            MakeTraceSourceAccessor(&NdmFailureScheduler::m_linkFailedTraced),
                            "ns3::TracedCallback<const NdmLinkId&, Time>")
            .AddTraceSource("LinkRestored",
                            "Link usable again: (linkId, tReady)",
                            MakeTraceSourceAccessor(&NdmFailureScheduler::m_linkRestoredTraced),
                            "ns3::TracedCallback<const NdmLinkId&, Time>")
            .AddTraceSource("SwitchFailed",
                            "Planned switch failure applied: (switchNode, tDetect)",
                            MakeTraceSourceAccessor(&NdmFailureScheduler::m_switchFailedTraced),
                            "ns3::TracedCallback<uint32_t, Time>")
            .AddTraceSource("SwitchRestored",
                            "Planned switch failure cleared: (switchNode, tReady)",
                            MakeTraceSourceAccessor(&NdmFailureScheduler::m_switchRestoredTraced),
                            "ns3::TracedCallback<uint32_t, Time>");
    return tid;
}

NdmFailureScheduler::NdmFailureScheduler() = default;

NdmFailureScheduler::~NdmFailureScheduler() = default;

void
NdmFailureScheduler::SetTopology(Ptr<NdmTopology> topo)
{
    NS_ASSERT_MSG(!m_topo, "NdmFailureScheduler: topology already set");
    m_topo = topo;
}

void
NdmFailureScheduler::AddEvent(const Event& ev)
{
    NS_ASSERT_MSG(!m_started, "NdmFailureScheduler: events frozen after Start()");
    m_events.push_back(ev);
}

void
NdmFailureScheduler::SetRerouteHandler(std::function<void(const NdmLinkId&, Time)> handler)
{
    m_reroute = std::move(handler);
}

uint32_t
NdmFailureScheduler::GetEventCount() const
{
    return m_events.size();
}

std::vector<NdmLinkId>
NdmFailureScheduler::IncidentLinks(uint32_t switchNode) const
{
    return m_topo->GetNodeLinkIds(switchNode); // sorted by link index (deterministic)
}

void
NdmFailureScheduler::ApplyLinkDown(const NdmLinkId& id, Time tDetect, InFlightPolicy policy)
{
    NS_LOG_INFO("NdmFailureScheduler: link " << id.index << " DOWN at " << tDetect
                                             << " policy=" << uint8_t(policy));
    Ptr<NdmLink> link = m_topo->GetLink(id);
    NS_ASSERT_MSG(!link->IsDown(), "NdmFailureScheduler: link already down");
    link->SetDownAt(tDetect, policy);
    m_linkFailedTraced(id, tDetect);
}

void
NdmFailureScheduler::ApplyLinkUp(const NdmLinkId& id, Time tReady)
{
    NS_LOG_INFO("NdmFailureScheduler: link " << id.index << " UP at " << tReady);
    Ptr<NdmLink> link = m_topo->GetLink(id);
    NS_ASSERT_MSG(link->IsDown(), "NdmFailureScheduler: link not down");
    link->SetReadyAt(tReady);
    m_linkRestoredTraced(id, tReady);
}

void
NdmFailureScheduler::Start()
{
    NS_ASSERT_MSG(!m_started, "NdmFailureScheduler: already started");
    NS_ASSERT_MSG(m_topo != nullptr, "NdmFailureScheduler: no topology");
    m_started = true;

    for (const Event& ev : m_events)
    {
        switch (ev.action)
        {
            case Action::LINK_DOWN:
            {
                const Time tDetect = ev.t + ev.detectionDelay;
                const Time tReroute = tDetect + ev.convergenceDelay;
                Simulator::Schedule(ev.t + ev.detectionDelay,
                                    &NdmFailureScheduler::ApplyLinkDown,
                                    this,
                                    ev.link,
                                    tDetect,
                                    ev.inFlight);
                Simulator::Schedule(tReroute,
                                    [this, link = ev.link, t = tReroute]() {
                                        if (m_reroute)
                                        {
                                            m_reroute(link, t);
                                        }
                                    });
                break;
            }
            case Action::LINK_UP:
            {
                const Time tReady = ev.t + ev.convergenceDelay;
                Simulator::Schedule(tReady, &NdmFailureScheduler::ApplyLinkUp, this, ev.link, tReady);
                Simulator::Schedule(tReady,
                                    [this, link = ev.link, t = tReady]() {
                                        if (m_reroute)
                                        {
                                            m_reroute(link, t);
                                        }
                                    });
                break;
            }
            case Action::SWITCH_DOWN:
            {
                const Time tDetect = ev.t + ev.detectionDelay;
                const Time tReroute = tDetect + ev.convergenceDelay;
                const std::vector<NdmLinkId> incident = IncidentLinks(ev.switchNode);
                for (const auto& id : incident)
                {
                    Simulator::Schedule(tDetect,
                                        &NdmFailureScheduler::ApplyLinkDown,
                                        this,
                                        id,
                                        tDetect,
                                        ev.inFlight);
                    Simulator::Schedule(tReroute,
                                        [this, l = id, t = tReroute]() {
                                            if (m_reroute)
                                            {
                                                m_reroute(l, t);
                                            }
                                        });
                }
                Simulator::Schedule(tDetect,
                                    [this, node = ev.switchNode, t = tDetect]() {
                                        m_switchFailedTraced(node, t);
                                    });
                break;
            }
            case Action::SWITCH_UP:
            {
                const Time tReady = ev.t + ev.convergenceDelay;
                const std::vector<NdmLinkId> incident = IncidentLinks(ev.switchNode);
                for (const auto& id : incident)
                {
                    Simulator::Schedule(tReady, &NdmFailureScheduler::ApplyLinkUp, this, id, tReady);
                    Simulator::Schedule(tReady,
                                        [this, l = id, t = tReady]() {
                                            if (m_reroute)
                                            {
                                                m_reroute(l, t);
                                            }
                                        });
                }
                Simulator::Schedule(tReady,
                                    [this, node = ev.switchNode, t = tReady]() {
                                        m_switchRestoredTraced(node, t);
                                    });
                break;
            }
        }
    }
}

} // namespace ns3
