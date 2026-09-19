/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * Temporary debug repro for the ndm-failure map::at crash. Runs the
 * two-rail link-down scenario step by step with stderr prints.
 */

#include <iostream>

#include "ns3/ndm-failure-scheduler.h"
#include "ns3/ndm-link.h"
#include "ns3/ndm-path-forwarder.h"
#include "ns3/ndm-topology.h"
#include "ns3/ndm-topology-helper.h"

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    std::cerr << "step: build topo" << std::endl;
    auto topo = CreateObject<NdmTopology>();
    topo->AddNode(NdmNodeKind::HOST, 0);
    topo->AddNode(NdmNodeKind::HOST, 1);
    NdmTopologyHelper::Opts opts;
    opts.bps = DataRate("1Gbps");
    opts.delay = MicroSeconds(1500);
    auto l0 = topo->AddLink(0, 1, opts.bps, opts.delay, 0, -1, false, 1000, nullptr);
    auto l1 = topo->AddLink(0, 1, opts.bps, opts.delay, 1, -1, false, 1000, nullptr);
    std::cerr << "step: attach forwarders" << std::endl;
    auto fwdA = CreateObject<NdmPathForwarder>();
    auto fwdB = CreateObject<NdmPathForwarder>();
    fwdA->Attach(topo, 0);
    fwdB->Attach(topo, 1);
    std::cerr << "step: find paths" << std::endl;
    auto paths = topo->FindPaths(0, 1);
    std::cerr << "  paths=" << paths.size() << std::endl;
    NdmTopology::NdmPath p0 = paths[0];
    NdmTopology::NdmPath p1 = paths[1];
    fwdA->RegisterPath(p0);
    fwdB->RegisterPath(p0);
    fwdA->RegisterPath(p1);
    fwdB->RegisterPath(p1);

    std::cerr << "step: failure scheduler" << std::endl;
    auto sched = CreateObject<NdmFailureScheduler>();
    sched->SetTopology(topo);
    NdmFailureScheduler::Event down;
    down.t = MilliSeconds(50);
    down.action = NdmFailureScheduler::Action::LINK_DOWN;
    down.link = l0->GetId();
    down.detectionDelay = MilliSeconds(2);
    down.convergenceDelay = MilliSeconds(6);
    down.inFlight = InFlightPolicy::DROP;
    sched->AddEvent(down);

    uint32_t sent = 0;
    uint32_t skipped = 0;
    bool rerouted = false;
    sched->SetRerouteHandler([&](const NdmLinkId&, Time) { rerouted = true; });
    sched->Start();

    std::cerr << "step: start sending" << std::endl;
    Simulator::Schedule(MilliSeconds(1), [fwdA, topo, &sent, &skipped, &rerouted, &p0, &p1]() {
        for (uint32_t i = 0; i < 40; i++)
        {
            Time t = MilliSeconds(1) + MilliSeconds(2) * i;
            Simulator::Schedule(t, [fwdA, topo, &sent, &skipped, &rerouted, &p0, &p1, i]() {
                uint32_t pid;
                if (!rerouted)
                {
                    pid = 0;
                }
                else
                {
                    auto ps = topo->FindPaths(0, 1);
                    if (ps.empty())
                    {
                        pid = UINT32_MAX;
                    }
                    else if (ps.front().m_links == p0.m_links)
                    {
                        pid = 0;
                    }
                    else
                    {
                        pid = 1;
                    }
                }
                if (pid == UINT32_MAX || !fwdA->GetPath(pid).IsValid(*topo))
                {
                    skipped++;
                }
                else
                {
                    if (fwdA->Send(pid, 1200))
                    {
                        sent++;
                    }
                }
                (void)i;
            });
        }
    });
    Simulator::Schedule(MilliSeconds(100), &Simulator::Stop);
    std::cerr << "step: run" << std::endl;
    Simulator::Run();
    std::cerr << "step: done. sent=" << sent << " skipped=" << skipped
              << " delivered=" << fwdB->GetDeliveredCount()
              << " txBlocked=" << l0->GetTxBlockedCount() + l1->GetTxBlockedCount()
              << " rxLost=" << l0->GetRxLostInFlightCount() + l1->GetRxLostInFlightCount()
              << " l0down=" << (l0->IsDown() ? 1 : 0)
              << " tdown=" << l0->GetTDown().GetMilliSeconds() << std::endl;
    const auto& dt = fwdB->GetDeliveryTimes();
    std::cerr << "  deliveries: ";
    for (size_t i = 0; i < dt.size(); i++)
    {
        if (dt[i] > MilliSeconds(49) || i < 2)
        {
            std::cerr << dt[i].GetMilliSeconds() << "ms ";
        }
    }
    std::cerr << std::endl;
    return 0;
}
