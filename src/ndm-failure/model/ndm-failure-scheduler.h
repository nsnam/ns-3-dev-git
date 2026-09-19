/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * NdmFailureScheduler: planned/scripted failure injection (AGENTS.md D6).
 *
 * A scenario defines explicit events (link down/up, planned switch
 * down/up). Each event has an explicit detection delay and convergence
 * delay, and (for down events) an in-flight packet policy.
 *
 * Timing model (documented contract):
 *  - LINK_DOWN  at t: the failure is *detected* at t + detectionDelay; the
 *    link's failure state (and the in-flight policy) applies from that
 *    instant. The reroute window (when a reroute handler may act) opens at
 *    t + detectionDelay + convergenceDelay.
 *  - LINK_UP    at t: the link is usable again from t + convergenceDelay
 *    (recovery detection is folded into the convergence delay); the
 *    reroute handler is invoked at that instant.
 *  - SWITCH_*:  applied to every link incident to the switch node
 *    (isolate = down all incident links; restore = up all of them).
 *
 * Anti-pattern guard (D6): the scheduler never recomputes anything
 * globally or instantly. It only (a) flips link state at the detected
 * time and (b) invokes a per-link reroute callback at the convergence
 * time; *what* to replan (and whether to replan at all) is the handler's
 * decision (the tests/planner do a local replan of the affected flow).
 * The HPCC scratch "BFS + rewrite every table at once" behavior is the
 * reference anti-pattern this design avoids.
 *
 * Determinism (D7): plain scheduled events; no wall clock, no threads.
 */

#ifndef NDM_FAILURE_SCHEDULER_H
#define NDM_FAILURE_SCHEDULER_H

#include "ns3/ndm-identity.h"

#include "ns3/nstime.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

#include <functional>
#include <vector>

namespace ns3
{

class NdmTopology;

class NdmFailureScheduler : public Object
{
  public:
    static TypeId GetTypeId();

    NdmFailureScheduler();
    ~NdmFailureScheduler() override;

    enum class Action : uint8_t
    {
        LINK_DOWN,
        LINK_UP,
        SWITCH_DOWN,
        SWITCH_UP
    };

    struct Event
    {
        Time t{Seconds(0)};                    //!< scheduled time
        Action action{Action::LINK_DOWN};
        NdmLinkId link{};                      //!< for LINK_* actions
        uint32_t switchNode{UINT32_MAX};       //!< for SWITCH_* actions
        Time detectionDelay{Seconds(0)};
        Time convergenceDelay{Seconds(0)};
        InFlightPolicy inFlight{InFlightPolicy::DROP};
    };

    void SetTopology(Ptr<NdmTopology> topo);
    void AddEvent(const Event& ev);
    /// Invoked at each convergence instant (per affected link) — the
    /// scenario's reroute point. The scheduler itself does no replanning.
    void SetRerouteHandler(std::function<void(const NdmLinkId&, Time)> handler);
    /// Schedule all event effects. Call once, before Simulator::Run().
    void Start();

    uint32_t GetEventCount() const;

    // -- traces (gate evidence) ----------------------------------------------
    /// (link, tDetect) — link failure state applied.
    TracedCallback<const NdmLinkId&, Time> m_linkFailedTraced;
    /// (link, tReady) — link usable again.
    TracedCallback<const NdmLinkId&, Time> m_linkRestoredTraced;
    /// (switchNode, tDetect).
    TracedCallback<uint32_t, Time> m_switchFailedTraced;
    /// (switchNode, tReady).
    TracedCallback<uint32_t, Time> m_switchRestoredTraced;

  private:
    void ApplyLinkDown(const NdmLinkId& id, Time tDetect, InFlightPolicy policy);
    void ApplyLinkUp(const NdmLinkId& id, Time tReady);
    std::vector<NdmLinkId> IncidentLinks(uint32_t switchNode) const;

    Ptr<NdmTopology> m_topo;
    std::vector<Event> m_events;
    std::function<void(const NdmLinkId&, Time)> m_reroute;
    bool m_started{false};
};

} // namespace ns3

#endif // NDM_FAILURE_SCHEDULER_H
