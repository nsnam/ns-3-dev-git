/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * G-fail gate tests (PLAN.md Phase 1):
 *  - scheduled link-down => no delivery across the link after T+detection;
 *  - rerouting only after the convergence delay;
 *  - in-flight policy (DROP / FLUSH / DELIVER_THEN_DROP) honored exactly;
 *  - planned switch failure isolates and re-plans;
 *  - deterministic under fixed seed (two runs identical);
 *  - loss models (Bernoulli/burst) deterministic per seed;
 *  - lossy mode: queue drops (D5).
 *
 * All scenarios are built from the public module APIs only (topology,
 * failure scheduler, path forwarder) with explicit times chosen to avoid
 * same-instant ties, so every expectation below is an exact value.
 */

#include "ndm-failure-scheduler.h"
#include "ndm-link.h"
#include "ndm-path-forwarder.h"
#include "ndm-topology.h"
#include "ndm-topology-helper.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/test.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <vector>

using namespace ns3;

namespace
{

constexpr uint32_t kPayload = 1200; // bytes
constexpr uint64_t kNone = UINT32_MAX;

/// Interval sender: every `interval` picks a path via `plan()` and sends,
/// counting skips when no valid path exists.
struct TestSender
{
    Ptr<NdmPathForwarder> fwd;
    Ptr<NdmTopology> topo;
    uint32_t src{0};
    uint32_t dst{0};
    Time interval{Seconds(0)};
    uint32_t total{0};
    uint32_t done{0};
    uint32_t skipped{0};
    uint32_t sent{0};
    std::function<uint32_t()> plan;

    void Replan()
    {
        // plan() is evaluated lazily at each tick; nothing to cache.
    }

    void Tick()
    {
        if (done >= total)
        {
            return;
        }
        const uint32_t pid = plan();
        if (pid == kNone || !fwd->GetPath(pid).IsValid(*topo))
        {
            skipped++;
        }
        else
        {
            if (fwd->Send(pid, kPayload))
            {
                sent++;
            }
        }
        done++;
        Simulator::Schedule(interval, [this] { Tick(); });
    }

    void StartAt(Time t)
    {
        Simulator::Schedule(t, [this] { Tick(); });
    }
};

/// Metrics snapshot used for exact assertions and the determinism pair.
struct Metrics
{
    uint32_t delivered{0};
    uint32_t sent{0};
    uint32_t skipped{0};
    uint64_t txBlocked{0};
    uint64_t rxLost{0};
    uint64_t flushed{0};
    std::vector<Time> deliveryTimes;
    std::vector<uint32_t> deliveryPathIds;
    std::map<NdmLinkId, uint64_t> linkRxCount;
    std::map<NdmLinkId, Time> linkRxFirst;
    std::map<NdmLinkId, Time> linkRxLast;

    bool operator==(const Metrics& o) const
    {
        return delivered == o.delivered && sent == o.sent && skipped == o.skipped &&
               txBlocked == o.txBlocked && rxLost == o.rxLost && flushed == o.flushed &&
               deliveryTimes == o.deliveryTimes && deliveryPathIds == o.deliveryPathIds &&
               linkRxCount == o.linkRxCount && linkRxFirst == o.linkRxFirst &&
               linkRxLast == o.linkRxLast;
    }
};

Metrics
Collect(Ptr<NdmPathForwarder> fwd, Ptr<NdmLink> l0, Ptr<NdmLink> l1)
{
    Metrics m;
    m.delivered = fwd->GetDeliveredCount();
    m.deliveryTimes = fwd->GetDeliveryTimes();
    m.deliveryPathIds = fwd->GetDeliveryPathIds();
    for (const auto& [id, rx] : fwd->GetLinkRx())
    {
        m.linkRxCount[id] = rx.count;
        m.linkRxFirst[id] = rx.firstTime;
        m.linkRxLast[id] = rx.lastTime;
    }
    m.txBlocked = l0->GetTxBlockedCount() + l1->GetTxBlockedCount();
    m.rxLost = l0->GetRxLostInFlightCount() + l1->GetRxLostInFlightCount();
    m.flushed = l0->GetQueueFlushedCount() + l1->GetQueueFlushedCount();
    return m;
}

} // namespace

// ===========================================================================
// Scenario builder: 2 nodes, 2 parallel links (rails 0/1), link-down at 50ms
// (detection 2ms, convergence 6ms). Optionally: link-up at 100ms (conv 10ms),
// and N sends on the odd-millisecond grid.
// ===========================================================================
namespace
{

struct TwoRail
{
    Ptr<NdmTopology> topo;
    Ptr<NdmPathForwarder> fwdA;
    Ptr<NdmPathForwarder> fwdB;
    Ptr<NdmLink> l0;
    Ptr<NdmLink> l1;
    uint32_t A; // relative node ids
    uint32_t B;
};

TwoRail
BuildTwoRail(bool withUp)
{
    TwoRail r;
    r.topo = CreateObject<NdmTopology>();
    r.A = r.topo->AddNode(NdmNodeKind::HOST, 0)->GetId();
    r.B = r.topo->AddNode(NdmNodeKind::HOST, 1)->GetId();
    NdmTopologyHelper::Opts opts;
    opts.bps = DataRate("1 Gbps");
    opts.delay = MilliSeconds(1.5);
    r.l0 = r.topo->AddLink(r.A, r.B, opts.bps, opts.delay, 0, -1, false, 1000, nullptr);
    r.l1 = r.topo->AddLink(r.A, r.B, opts.bps, opts.delay, 1, -1, false, 1000, nullptr);

    r.fwdA = CreateObject<NdmPathForwarder>();
    r.fwdB = CreateObject<NdmPathForwarder>();
    r.fwdA->Attach(r.topo, r.topo->GetNode(0));
    r.fwdB->Attach(r.topo, r.topo->GetNode(1));

    auto paths = r.topo->FindPaths(r.A, r.B);
    NS_ASSERT_MSG(paths.size() == 2, "two-rail: expected 2 paths");
    // pathId 0 = rail 0 (L0 first, deterministic ordering), 1 = rail 1.
    r.fwdA->RegisterPath(paths[0]);
    r.fwdB->RegisterPath(paths[0]);
    r.fwdA->RegisterPath(paths[1]);
    r.fwdB->RegisterPath(paths[1]);

    if (withUp)
    {
        // (events added by the caller via the scheduler)
    }
    return r;
}

struct TwoRailResult
{
    Metrics m;
    uint32_t sent{0};
    uint32_t skipped{0};
};

TwoRailResult
RunTwoRail(bool withUp, uint32_t nSends)
{
    TwoRail r = BuildTwoRail(withUp);

    NdmTopology::NdmPath p0 = r.fwdA->GetPath(0);
    NdmTopology::NdmPath p1 = r.fwdA->GetPath(1);

    TestSender sender;
    sender.fwd = r.fwdA;
    sender.topo = r.topo;
    sender.src = r.A;
    sender.dst = r.B;
    sender.interval = MilliSeconds(2);
    sender.total = nSends;
    sender.plan = [&]() -> uint32_t {
        auto paths = r.topo->FindPaths(r.A, r.B);
        if (paths.empty())
        {
            return kNone;
        }
        const auto& best = paths.front();
        if (best.m_links == p0.m_links)
        {
            return 0;
        }
        if (best.m_links == p1.m_links)
        {
            return 1;
        }
        return kNone;
    };

    auto sched = CreateObject<NdmFailureScheduler>();
    sched->SetTopology(r.topo);
    NdmFailureScheduler::Event down;
    down.t = MilliSeconds(50);
    down.action = NdmFailureScheduler::Action::LINK_DOWN;
    down.link = r.l0->GetId();
    down.detectionDelay = MilliSeconds(2); // tDetect = 52ms
    down.convergenceDelay = MilliSeconds(6); // reroute = 58ms
    down.inFlight = InFlightPolicy::DROP;
    sched->AddEvent(down);
    if (withUp)
    {
        NdmFailureScheduler::Event up;
        up.t = MilliSeconds(100);
        up.action = NdmFailureScheduler::Action::LINK_UP;
        up.link = r.l0->GetId();
        up.convergenceDelay = MilliSeconds(10); // ready = 110ms
        sched->AddEvent(up);
    }
    sched->SetRerouteHandler([s = &sender](const NdmLinkId&, Time) { s->Replan(); });
    sched->Start();

    sender.StartAt(MilliSeconds(1)); // odd-millisecond grid: 1,3,5,...
    Simulator::StopAt(MilliSeconds(nSends * 2 + 20));
    Simulator::Run();

    TwoRailResult out;
    out.m = Collect(r.fwdB, r.l0, r.l1);
    out.sent = sender.sent;
    out.skipped = sender.skipped;
    return out;
}

} // namespace

class NdmLinkDownRerouteTestCase : public TestCase
{
  public:
    NdmLinkDownRerouteTestCase()
        : TestCase("ndm-failure/link-down-reroute")
    {
    }
  protected:
    void DoRun() override
    {
        // 40 sends at t=1,3,...,79 ms. tDetect=52, reroute=58.
        auto r = RunTwoRail(/*withUp=*/false, 40);

        // L0: sends 1..49 delivered (arrivals 2.5..50.5); send 51 in flight at
        // 52 (rx at 52.5) lost; sends 53,55,57 skipped (no valid path).
        // L1: sends 59..79 delivered (first arrival 60.5).
        EXPECT_EQ(r.m.delivered, 36u);
        EXPECT_EQ(r.sent, 37u);
        EXPECT_EQ(r.skipped, 3u);
        EXPECT_EQ(r.m.rxLost, 1u);
        EXPECT_EQ(r.m.txBlocked, 0u);

        // No delivery across L0 after tDetect (52ms).
        const auto l0It = r.m.linkRxLast.find(NdmLinkId{0, 0, -1});
        EXPECT_TRUE(l0It != r.m.linkRxLast.end());
        EXPECT_TRUE(l0It->second < MilliSeconds(52));
        EXPECT_EQ(r.m.linkRxCount.at(NdmLinkId{0, 0, -1}), 25u);

        // Rerouting only after the convergence delay: first L1 delivery at
        // earliest 58 + 1.5 = 59.5ms.
        EXPECT_TRUE(r.m.linkRxFirst.at(NdmLinkId{1, 1, -1}) >= MilliSeconds(59.5));
        EXPECT_EQ(r.m.linkRxCount.at(NdmLinkId{1, 1, -1}), 11u);
    }
};

class NdmLinkUpConvergenceTestCase : public TestCase
{
  public:
    NdmLinkUpConvergenceTestCase()
        : TestCase("ndm-failure/link-up-convergence")
    {
    }
  protected:
    void DoRun() override
    {
        // 140 sends at t=1,3,...,279 ms. DOWN L0 (tDetect 52, reroute 58);
        // UP L0 (ready 110, reroute 110).
        auto r = RunTwoRail(/*withUp=*/true, 140);

        // L0: 25 (sends 1..49) + 85 (sends 111..279); L1: 26 (sends 59..109).
        EXPECT_EQ(r.m.delivered, 136u);
        EXPECT_EQ(r.skipped, 3u);
        EXPECT_EQ(r.m.rxLost, 1u);
        EXPECT_EQ(r.m.linkRxCount.at(NdmLinkId{0, 0, -1}), 110u);
        EXPECT_EQ(r.m.linkRxCount.at(NdmLinkId{1, 1, -1}), 26u);

        // No rail-0 delivery between the failure (52ms) and recovery: the
        // last pre-failure arrival is 50.5ms; the first post-recovery
        // arrival is >= 110 + 1.5 = 111.5ms.
        bool gapOk = true;
        for (size_t i = 0; i < r.m.deliveryTimes.size(); i++)
        {
            if (r.m.deliveryPathIds[i] == 0)
            {
                const Time t = r.m.deliveryTimes[i];
                if (t > MilliSeconds(52) && t < MilliSeconds(111.5))
                {
                    gapOk = false;
                }
            }
        }
        EXPECT_TRUE(gapOk);

        // L1 last delivery (send 109) at 110.5ms.
        EXPECT_EQ(r.m.linkRxLast.at(NdmLinkId{1, 1, -1}), MilliSeconds(110.5));
    }
};

class NdmInFlightPolicyTestCase : public TestCase
{
  public:
    NdmInFlightPolicyTestCase(std::string name, InFlightPolicy policy)
        : TestCase(std::move(name)),
          m_policy(policy)
    {
    }
  protected:
    void DoRun() override;
    InFlightPolicy m_policy;
};

void
NdmInFlightPolicyTestCase::DoRun()
{
    // One link, 10 Mbps (1000 B => 0.8 ms serialization), 1 ms delay.
    // 100 packets sent at t=0. DOWN at t=10 (detection 0), no reroute.
    auto topo = CreateObject<NdmTopology>();
    const uint32_t A = topo->AddNode(NdmNodeKind::HOST, 0)->GetId();
    const uint32_t B = topo->AddNode(NdmNodeKind::HOST, 1)->GetId();
    auto link = topo->AddLink(A, B, DataRate("10 Mbps"), MilliSeconds(1), -1, -1, false, 1000,
                              nullptr);

    auto fwdA = CreateObject<NdmPathForwarder>();
    auto fwdB = CreateObject<NdmPathForwarder>();
    fwdA->Attach(topo, 0);
    fwdB->Attach(topo, 1);
    auto paths = topo->FindPaths(A, B);
    fwdA->RegisterPath(paths[0]);
    fwdB->RegisterPath(paths[0]);

    auto sched = CreateObject<NdmFailureScheduler>();
    sched->SetTopology(topo);
    NdmFailureScheduler::Event down;
    down.t = MilliSeconds(10);
    down.action = NdmFailureScheduler::Action::LINK_DOWN;
    down.link = link->GetId();
    down.detectionDelay = Seconds(0); // tDetect = 10ms
    down.convergenceDelay = Seconds(0);
    down.inFlight = m_policy;
    sched->AddEvent(down);
    sched->Start();

    Simulator::Schedule(Seconds(0), [f = fwdA]() {
        for (uint32_t i = 0; i < 100; i++)
        {
            (void)f->Send(0, 1000);
        }
    });
    Simulator::StopAt(MilliSeconds(200));
    Simulator::Run();

    // TX grid: 0, 0.8, ..., 79.2 ms. Arrivals = TX + 1 ms.
    // Before tDetect: 12 delivered (TX 0..8.8); TX 9.6 in flight at 10.
    switch (m_policy)
    {
        case InFlightPolicy::DROP:
            EXPECT_EQ(fwdB->GetDeliveredCount(), 12u);
            EXPECT_EQ(link->GetRxLostInFlightCount(), 1u);
            EXPECT_EQ(link->GetTxBlockedCount(), 87u); // TX 10.4..79.2
            EXPECT_EQ(link->GetQueueFlushedCount(), 0u);
            break;
        case InFlightPolicy::FLUSH:
            EXPECT_EQ(fwdB->GetDeliveredCount(), 12u);
            EXPECT_EQ(link->GetRxLostInFlightCount(), 1u);
            EXPECT_EQ(link->GetQueueFlushedCount(), 87u);
            EXPECT_EQ(link->GetTxBlockedCount(), 0u);
            break;
        case InFlightPolicy::DELIVER_THEN_DROP:
            // On-wire (1) + queued-at-detection (87) all complete.
            EXPECT_EQ(fwdB->GetDeliveredCount(), 100u);
            EXPECT_EQ(link->GetRxLostInFlightCount(), 0u);
            EXPECT_EQ(link->GetTxBlockedCount(), 0u);
            EXPECT_EQ(link->GetQueueFlushedCount(), 0u);
            break;
    }
}

// ===========================================================================
// Planned switch failure: multi-rail (2 pods x 2 gpus x 2 rails x 1 spine).
// GPU 0 -> GPU 3 travels rail 0: [L0, L8, L9, L3]; alternative rail 1:
// [L4, L10, L11, L7]. SWITCH_DOWN ToR_rail0_pod0 (incident L0, L1, L8).
// ===========================================================================
namespace
{

struct SwitchResult
{
    Metrics dst; // at GPU 3
    uint32_t sent{0};
    uint32_t skipped{0};
    std::map<uint32_t, std::map<NdmLinkId, Time>> nodeLinkRxLast; // per intermediate node
};

SwitchResult
RunSwitchFailure()
{
    NdmTopologyHelper::Opts opts;
    opts.bps = DataRate("1 Gbps");
    opts.delay = MilliSeconds(1.5);
    auto topo = NdmTopologyHelper::CreateMultiRail(2, 2, 2, 1, 1, opts);

    // Node layout (relative): 0..3 GPUs (p0g0, p0g1, p1g0, p1g1);
    // 4..7 ToRs (r0p0, r0p1, r1p0, r1p1); 8..9 spines (r0, r1).
    const uint32_t n0 = topo->GetNode(0)->GetId(); // src GPU p0g0
    const uint32_t n3 = topo->GetNode(3)->GetId(); // dst GPU p1g1
    const uint32_t n4 = topo->GetNode(4)->GetId(); // ToR r0 p0 (fails)

    auto fwds = std::vector<Ptr<NdmPathForwarder>>(topo->GetNodeCount());
    for (uint32_t i = 0; i < topo->GetNodeCount(); i++)
    {
        fwds[i] = CreateObject<NdmPathForwarder>();
        fwds[i]->Attach(topo, i);
    }

    auto paths = topo->FindPaths(n0, n3);
    NS_ASSERT_MSG(paths.size() >= 2, "switch: expected >= 2 paths");
    const NdmTopology::NdmPath rail0 = paths[0];
    const NdmTopology::NdmPath rail1 = paths[1];
    for (auto& f : fwds)
    {
        f->RegisterPath(rail0); // pathId 0
        f->RegisterPath(rail1); // pathId 1
    }

    TestSender sender;
    sender.fwd = fwds[0];
    sender.topo = topo;
    sender.src = n0;
    sender.dst = n3;
    sender.interval = MilliSeconds(2);
    sender.total = 40; // t = 1,3,...,79
    sender.plan = [&]() -> uint32_t {
        auto ps = topo->FindPaths(n0, n3);
        if (ps.empty())
        {
            return kNone;
        }
        if (ps.front().m_links == rail0.m_links)
        {
            return 0;
        }
        if (ps.front().m_links == rail1.m_links)
        {
            return 1;
        }
        return kNone;
    };

    auto sched = CreateObject<NdmFailureScheduler>();
    sched->SetTopology(topo);
    NdmFailureScheduler::Event down;
    down.t = MilliSeconds(50);
    down.action = NdmFailureScheduler::Action::SWITCH_DOWN;
    down.switchNode = n4;
    down.detectionDelay = MilliSeconds(2); // tDetect = 52ms
    down.convergenceDelay = MilliSeconds(6); // reroute = 58ms
    down.inFlight = InFlightPolicy::DROP;
    sched->AddEvent(down);
    sched->SetRerouteHandler([s = &sender](const NdmLinkId&, Time) { s->Replan(); });
    sched->Start();

    sender.StartAt(MilliSeconds(1));
    Simulator::StopAt(MilliSeconds(100));
    Simulator::Run();

    SwitchResult out;
    out.dst = Collect(fwds[3], topo->GetLink(NdmLinkId{0, 0, -1}), topo->GetLink(NdmLinkId{1, 0, -1}));
    out.sent = sender.sent;
    out.skipped = sender.skipped;
    for (uint32_t i = 0; i < topo->GetNodeCount(); i++)
    {
        for (const auto& [id, rx] : fwds[i]->GetLinkRx())
        {
            out.nodeLinkRxLast[i][id] = rx.lastTime;
        }
    }
    return out;
}

} // namespace

class NdmSwitchFailureTestCase : public TestCase
{
  public:
    NdmSwitchFailureTestCase()
        : TestCase("ndm-failure/planned-switch-failure")
    {
    }
  protected:
    void DoRun() override
    {
        auto r = RunSwitchFailure();

        // 4-hop flight = 4 * (1.5ms + 9.6us) = 6.0384 ms.
        // Rail-0 deliveries: sends 1..47 (24); lost in flight: 49 (on L8),
        // 51 (on L0); skipped: 53,55,57; rail-1: sends 59..79 (11).
        EXPECT_EQ(r.dst.delivered, 35u);
        EXPECT_EQ(r.skipped, 3u);

        // No rx counted after tDetect on the failed switch's incident links
        // (L0 at node 4, L8 at node 8, L9 at node 5 — relative node idx).
        for (const NdmLinkId bad : {NdmLinkId{0, 0, -1}, NdmLinkId{8, 0, -1}, NdmLinkId{9, 0, -1}})
        {
            for (const uint32_t node : {4u, 8u, 5u})
            {
                auto it = r.nodeLinkRxLast[node].find(bad);
                if (it != r.nodeLinkRxLast[node].end())
                {
                    EXPECT_TRUE(it->second < MilliSeconds(52));
                }
            }
        }

        // Reroute only after convergence: first rail-1 delivery >=
        // 58 + 6.0384 = 64.0384 ms; last rail-0 delivery <= 53.0384 ms.
        for (size_t i = 0; i < r.dst.deliveryTimes.size(); i++)
        {
            if (r.dst.deliveryPathIds[i] == 0)
            {
                EXPECT_TRUE(r.dst.deliveryTimes[i] <= MilliSeconds(53.1));
            }
            else
            {
                EXPECT_TRUE(r.dst.deliveryTimes[i] >= MilliSeconds(64.0));
            }
        }
    }
};

class NdmFailureDeterminismTestCase : public TestCase
{
  public:
    NdmFailureDeterminismTestCase()
        : TestCase("ndm-failure/determinism-pair")
    {
    }
  protected:
    void DoRun() override
    {
        auto r1 = RunTwoRail(/*withUp=*/true, 140);
        auto r2 = RunTwoRail(/*withUp=*/true, 140);
        EXPECT_TRUE(r1.m == r2.m);
        EXPECT_EQ(r1.m.delivered, 136u);
    }
};

class NdmLossModelTestCase : public TestCase
{
  public:
    NdmLossModelTestCase()
        : TestCase("ndm-failure/loss-model")
    {
    }
  protected:
    static uint32_t RunOnce(NdmLinkLossModel::Mode mode, double p, uint32_t burstLen)
    {
        NdmTopologyHelper::Opts opts;
        opts.bps = DataRate("1 Gbps");
        opts.delay = MilliSeconds(1);
        opts.lossMode = mode;
        opts.lossProbability = p;
        opts.burstLen = burstLen;
        opts.seed = 12345;
        auto topo = NdmTopologyHelper::CreateMultiRail(1, 2, 1, 1, 1, opts);
        const uint32_t A = topo->GetNode(0)->GetId();
        const uint32_t B = topo->GetNode(1)->GetId();

        auto fwdA = CreateObject<NdmPathForwarder>();
        auto fwdB = CreateObject<NdmPathForwarder>();
        fwdA->Attach(topo, 0);
        fwdB->Attach(topo, 1);
        auto paths = topo->FindPaths(A, B);
        fwdA->RegisterPath(paths[0]);
        fwdB->RegisterPath(paths[0]);

        Simulator::Schedule(Seconds(0), [f = fwdA]() {
            for (uint32_t i = 0; i < 1000; i++)
            {
                (void)f->Send(0, 1200);
            }
        });
        // All 1000 sends at t=0 serialize at 9.6us each (queue 1000 default
        // in Opts is 1000) => last TX ~9.6ms, delivery ~10.6ms.
        Simulator::StopAt(MilliSeconds(50));
        Simulator::Run();
        return fwdB->GetDeliveredCount();
    }

    void DoRun() override
    {
        // Bernoulli p=0.2: deterministic per seed, sane band.
        const uint32_t b1 = RunOnce(NdmLinkLossModel::Mode::BERN, 0.2, 1);
        const uint32_t b2 = RunOnce(NdmLinkLossModel::Mode::BERN, 0.2, 1);
        EXPECT_EQ(b1, b2);
        EXPECT_TRUE(b1 >= 780 && b1 <= 820);

        // Burst p=0.1 len=3: deterministic per seed; more loss than p=0.1
        // Bernoulli expectation on average, but only determinism is the gate.
        const uint32_t u1 = RunOnce(NdmLinkLossModel::Mode::BURST, 0.1, 3);
        const uint32_t u2 = RunOnce(NdmLinkLossModel::Mode::BURST, 0.1, 3);
        EXPECT_EQ(u1, u2);
        EXPECT_TRUE(u1 < 1000);
        EXPECT_TRUE(u1 > 500);
    }
};

class NdmQueueDropTestCase : public TestCase
{
  public:
    NdmQueueDropTestCase()
        : TestCase("ndm-failure/queue-drops")
    {
    }
  protected:
    static uint32_t RunOnce()
    {
        auto topo = CreateObject<NdmTopology>();
        const uint32_t A = topo->AddNode(NdmNodeKind::HOST, 0)->GetId();
        const uint32_t B = topo->AddNode(NdmNodeKind::HOST, 1)->GetId();
        // Tiny queue: with 1000 B and 1 Gbps (9.6us/packet), 6 packets fit
        // (1 in service + 5 queued); the remaining 94 are dropped at enqueue.
        auto link = topo->AddLink(A, B, DataRate("1 Gbps"), MilliSeconds(1), -1, -1, false, 5,
                                  nullptr);

        auto fwdA = CreateObject<NdmPathForwarder>();
        auto fwdB = CreateObject<NdmPathForwarder>();
        fwdA->Attach(topo, 0);
        fwdB->Attach(topo, 1);
        auto paths = topo->FindPaths(A, B);
        fwdA->RegisterPath(paths[0]);
        fwdB->RegisterPath(paths[0]);

        Simulator::Schedule(Seconds(0), [f = fwdA]() {
            for (uint32_t i = 0; i < 100; i++)
            {
                (void)f->Send(0, 1000);
            }
        });
        Simulator::StopAt(MilliSeconds(10));
        Simulator::Run();
        return fwdB->GetDeliveredCount();
    }

    void DoRun() override
    {
        const uint32_t d1 = RunOnce();
        const uint32_t d2 = RunOnce();
        EXPECT_EQ(d1, d2);
        EXPECT_EQ(d1, 6u);
    }
};

class NdmFailureTestSuite : public TestSuite
{
  public:
    NdmFailureTestSuite()
        : TestSuite("ndm-failure", Type::UNIT)
    {
        AddTestCase(new NdmLinkDownRerouteTestCase(), Duration::QUICK);
        AddTestCase(new NdmLinkUpConvergenceTestCase(), Duration::QUICK);
        AddTestCase(new NdmInFlightPolicyTestCase("ndm-failure/inflight-policy-drop",
                                                  InFlightPolicy::DROP),
                    Duration::QUICK);
        AddTestCase(new NdmInFlightPolicyTestCase("ndm-failure/inflight-policy-flush",
                                                  InFlightPolicy::FLUSH),
                    Duration::QUICK);
        AddTestCase(new NdmInFlightPolicyTestCase("ndm-failure/inflight-policy-deliver-then-drop",
                                                  InFlightPolicy::DELIVER_THEN_DROP),
                    Duration::QUICK);
        AddTestCase(new NdmSwitchFailureTestCase(), Duration::QUICK);
        AddTestCase(new NdmFailureDeterminismTestCase(), Duration::QUICK);
        AddTestCase(new NdmLossModelTestCase(), Duration::QUICK);
        AddTestCase(new NdmQueueDropTestCase(), Duration::QUICK);
    }
};

static NdmFailureTestSuite g_ndmFailureTestSuite;
