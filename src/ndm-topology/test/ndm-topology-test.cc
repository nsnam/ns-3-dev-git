/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * G-topo gate tests (PLAN.md Phase 1): generator unit tests — degrees,
 * link counts, diameters, and parallel-link preservation.
 */

#include "ns3/ndm-topology.h"
#include "ns3/ndm-topology-helper.h"

#include "ns3/core-module.h"
#include "ns3/node.h"
#include "ns3/test.h"

using namespace ns3;

// gtest-style convenience wrappers over the ns-3 test assertions
// (test-only; the project test files are the only users).
#define EXPECT_EQ(a, b) NS_TEST_ASSERT_MSG_EQ(((a) == (b)), true, "EXPECT_EQ(" #a ", " #b ") failed")
#define EXPECT_TRUE(c) NS_TEST_ASSERT_MSG_EQ(((c)), true, "EXPECT_TRUE(" #c ") failed")

class NdmFatTreeTestCase : public TestCase
{
  public:
    NdmFatTreeTestCase()
        : TestCase("ndm-topology/fat-tree")
    {
    }

  protected:
    /// Check the fat-tree invariants for a given (k, d) construction.
    void CheckFatTree(uint32_t k, uint32_t d)
    {
        NdmTopologyHelper::Opts opts;
        auto topo = NdmTopologyHelper::CreateFatTree(k, d, opts);

        const uint32_t L = d * (k / 2); // ToRs
        const uint32_t H = d * (k / 2) * (k / 2); // hosts
        const uint32_t S = d * k / 4; // spines
        const uint32_t nNodes = H + L + S;
        const uint32_t nLinks = H + L * (k / 2); // host links + uplinks

        EXPECT_EQ(topo->GetNodeCount(), nNodes);
        EXPECT_EQ(topo->GetLinkCount(), nLinks);

        // Hosts: degree 1. ToRs and spines: degree k.
        for (uint32_t i = 0; i < H; i++)
        {
            EXPECT_EQ(topo->GetDegree(i), 1u);
        }
        for (uint32_t i = 0; i < L + S; i++)
        {
            EXPECT_EQ(topo->GetDegree(H + i), k);
        }
        EXPECT_EQ(topo->GetMaxDegree(), k);

        // No parallel links anywhere (in particular no duplicate ToR-spine pair).
        EXPECT_EQ(topo->GetParallelLinkCount(), 0u);

        // Diameter: host-ToR-spine-ToR-host = 4.
        EXPECT_EQ(topo->GetDiameter(), 4u);

        // Every ToR connects to exactly k/2 distinct spines (non-blocking 2-tier).
        for (uint32_t i = 0; i < L; i++)
        {
            uint32_t spineLinks = 0;
            for (const auto& l : topo->GetNodeLinks(H + i))
            {
                const auto pA = l->GetPortA();
                const auto pB = l->GetPortB();
                const uint32_t other = (pA.node == H + i) ? pB.node : pA.node;
                if (other >= H + L)
                {
                    spineLinks++;
                }
            }
            EXPECT_EQ(spineLinks, k / 2);
        }
    }

    void DoRun() override
    {
        CheckFatTree(8, 2);
        CheckFatTree(8, 4);
        CheckFatTree(12, 4);
        CheckFatTree(4, 2);
    }
};

class NdmParallelLinkTestCase : public TestCase
{
  public:
    NdmParallelLinkTestCase()
        : TestCase("ndm-topology/parallel-links")
    {
    }
  protected:
    void DoRun() override
    {
        NdmTopologyHelper::Opts opts;
        auto topo = CreateObject<NdmTopology>();
        // Relative node indices (0-based creation order).
        const uint32_t a = 0;
        const uint32_t b = 1;
        topo->AddNode(NdmNodeKind::HOST, 0);
        topo->AddNode(NdmNodeKind::HOST, 1);
        auto l0 = topo->AddLink(a, b, opts.bps, opts.delay, 0, -1, false, 100, nullptr);
        auto l1 = topo->AddLink(a, b, opts.bps, opts.delay, 1, -1, false, 100, nullptr);

        // Adjacency keyed by (node, port): both links survive, distinct ports.
        EXPECT_EQ(topo->GetLinkCount(), 2u);
        EXPECT_EQ(topo->GetParallelLinkCount(), 1u);
        EXPECT_EQ(topo->GetDegree(a), 2u);
        EXPECT_EQ(topo->GetDegree(b), 2u);

        EXPECT_EQ(l0->GetPortA(), NdmPortIdentity{a, 0});
        EXPECT_EQ(l1->GetPortA(), NdmPortIdentity{a, 1});
        EXPECT_EQ(l0->GetPortB(), NdmPortIdentity{b, 0});
        EXPECT_EQ(l1->GetPortB(), NdmPortIdentity{b, 1});

        // Both links individually reachable via distinct paths.
        auto paths = topo->FindPaths(a, b);
        EXPECT_EQ(paths.size(), 2u);
        EXPECT_EQ(paths[0].m_links.front(), l0->GetId());
        EXPECT_EQ(paths[1].m_links.front(), l1->GetId());
    }
};

class NdmMultiRailTestCase : public TestCase
{
  public:
    NdmMultiRailTestCase()
        : TestCase("ndm-topology/multi-rail")
    {
    }
  protected:
    void DoRun() override
    {
        const uint32_t pods = 2, gpus = 4, rails = 3, spines = 2;
        NdmTopologyHelper::Opts opts;
        auto topo = NdmTopologyHelper::CreateMultiRail(pods, gpus, rails, spines, 1, opts);

        const uint32_t G = pods * gpus;
        EXPECT_EQ(topo->GetNodeCount(), G + pods * rails + rails * spines);
        EXPECT_EQ(topo->GetLinkCount(), G * rails + rails * pods * spines);

        // GPU degree = rails (one NIC per rail); ToR degree = gpus + spines;
        // Spine degree = pods.
        for (uint32_t i = 0; i < G; i++)
        {
            EXPECT_EQ(topo->GetDegree(i), rails);
        }
        for (uint32_t r = 0; r < rails; r++)
            for (uint32_t p = 0; p < pods; p++)
            {
                EXPECT_EQ(topo->GetDegree(G + r * pods + p), gpus + spines);
            }
        for (uint32_t r = 0; r < rails; r++)
            for (uint32_t s = 0; s < spines; s++)
            {
                EXPECT_EQ(topo->GetDegree(G + pods * rails + r * spines + s), pods);
            }

        // Rail identity on every link; no parallel links at nic=1.
        for (uint32_t i = 0; i < topo->GetLinkCount(); i++)
        {
            const NdmLinkId id{i, -1, -1};
            const auto& lid = topo->GetLink(id)->GetId();
            EXPECT_TRUE(lid.rail >= 0 && lid.rail < int32_t(rails));
            EXPECT_EQ(lid.plane, -1);
        }
        EXPECT_EQ(topo->GetParallelLinkCount(), 0u);
        EXPECT_EQ(topo->GetDiameter(), 4u);

        // nicPerRail=2 => parallel links, one extra per (gpu, rail) pair.
        auto topo2 = NdmTopologyHelper::CreateMultiRail(1, 2, 2, 1, 2, opts);
        EXPECT_EQ(topo2->GetParallelLinkCount(), 2u * 2u); // 2 gpus x 2 rails
        EXPECT_EQ(topo2->GetDiameter(), 2u); // single pod: gpu-ToR-gpu
    }
};

class NdmMultiPlaneTestCase : public TestCase
{
  public:
    NdmMultiPlaneTestCase()
        : TestCase("ndm-topology/multi-plane")
    {
    }
  protected:
    void DoRun() override
    {
        // Two 6-host ring planes.
        NdmTopologyHelper::Opts opts;
        auto topo = NdmTopologyHelper::CreateMultiPlane(2, NdmTopologyHelper::PlaneKind::RING,
                                                        6, 4, 2, false, opts);
        const uint32_t H = 6;
        EXPECT_EQ(topo->GetNodeCount(), H + 2 * H);
        EXPECT_EQ(topo->GetLinkCount(), 2 * (H + H)); // host + ring links per plane
        for (uint32_t i = 0; i < H; i++)
        {
            EXPECT_EQ(topo->GetDegree(i), 2u); // one NIC per plane
        }
        for (uint32_t i = 0; i < 2 * H; i++)
        {
            EXPECT_EQ(topo->GetDegree(H + i), 3u); // host + 2 ring
        }
        EXPECT_EQ(topo->GetDiameter(), 5u); // 1 + ring(3) + 1

        // With cross-plane links (adjacent planes only: H links): switch
        // degree 4, diameter 6 (1 + 1 + ring(3) + 1).
        auto topoX = NdmTopologyHelper::CreateMultiPlane(2, NdmTopologyHelper::PlaneKind::RING,
                                                         6, 4, 2, true, opts);
        EXPECT_EQ(topoX->GetLinkCount(), 2 * (H + H) + H);
        EXPECT_EQ(topoX->GetMaxDegree(), 4u);
        EXPECT_EQ(topoX->GetDiameter(), 6u);

        // Two fat-tree planes (k=4, d=2 => 8 shared hosts).
        auto topoF =
            NdmTopologyHelper::CreateMultiPlane(2, NdmTopologyHelper::PlaneKind::FAT_TREE, 0, 4, 2,
                                                true, opts);
        const uint32_t Hf = 8;
        const uint32_t Lf = 2 * 2; // ToRs per plane
        const uint32_t Sf = 2 * 4 / 4; // spines per plane
        EXPECT_EQ(topoF->GetNodeCount(), Hf + 2 * (Lf + Sf));
        EXPECT_EQ(topoF->GetLinkCount(), 2 * (Hf + Lf * 2) + (Lf + Sf));
        EXPECT_EQ(topoF->GetDiameter(), 4u);

        // Plane identity on plane links.
        for (uint32_t i = 0; i < topo->GetLinkCount(); i++)
        {
            const auto& lid = topo->GetLink(NdmLinkId{i, -1, -1})->GetId();
            EXPECT_TRUE(lid.plane >= 0 && lid.plane < 2);
            EXPECT_EQ(lid.crossPlane, false);
        }
        // Cross-plane links are marked (adjacent planes only).
        uint32_t cross = 0;
        for (uint32_t i = 0; i < topoX->GetLinkCount(); i++)
        {
            if (topoX->GetLink(NdmLinkId{i, -1, -1})->GetId().crossPlane)
            {
                cross++;
            }
        }
        EXPECT_EQ(cross, H);
    }
};

class NdmPathQueryTestCase : public TestCase
{
  public:
    NdmPathQueryTestCase()
        : TestCase("ndm-topology/path-query")
    {
    }
  protected:
    void DoRun() override
    {
        // Diamond: a-b-c and a-d-c.
        NdmTopologyHelper::Opts opts;
        auto topo = CreateObject<NdmTopology>();
        // Relative node indices (0-based creation order).
        const uint32_t a = 0;
        const uint32_t b = 1;
        const uint32_t c = 2;
        const uint32_t d = 3;
        topo->AddNode(NdmNodeKind::HOST, 0);
        topo->AddNode(NdmNodeKind::SWITCH, 0);
        topo->AddNode(NdmNodeKind::HOST, 1);
        topo->AddNode(NdmNodeKind::SWITCH, 1);
        auto ab = topo->AddLink(a, b, opts.bps, opts.delay, -1, -1, false, 100, nullptr);
        auto bc = topo->AddLink(b, c, opts.bps, opts.delay, -1, -1, false, 100, nullptr);
        auto ad = topo->AddLink(a, d, opts.bps, opts.delay, -1, -1, false, 100, nullptr);
        auto dc = topo->AddLink(d, c, opts.bps, opts.delay, -1, -1, false, 100, nullptr);

        auto paths = topo->FindPaths(a, c);
        EXPECT_EQ(paths.size(), 2u);
        EXPECT_EQ(paths[0].m_nodes.size(), 3u);
        EXPECT_EQ(paths[0].m_links.front(), ab->GetId());

        // Take the a-b link down at detection time: only the d path remains.
        ab->SetDownAt(MilliSeconds(100), InFlightPolicy::DROP);
        auto paths2 = topo->FindPaths(a, c);
        EXPECT_EQ(paths2.size(), 1u);
        EXPECT_EQ(paths2[0].m_links.front(), ad->GetId());
        EXPECT_TRUE(paths2[0].IsValid(*topo));
        EXPECT_TRUE(!NdmTopology::NdmPath{paths[0].m_nodes, paths[0].m_links}.IsValid(*topo));
    }
};

class NdmTopologyTestSuite : public TestSuite
{
  public:
    NdmTopologyTestSuite()
        : TestSuite("ndm-topology", Type::UNIT)
    {
        AddTestCase(new NdmFatTreeTestCase(), Duration::QUICK);
        AddTestCase(new NdmParallelLinkTestCase(), Duration::QUICK);
        AddTestCase(new NdmMultiRailTestCase(), Duration::QUICK);
        AddTestCase(new NdmMultiPlaneTestCase(), Duration::QUICK);
        AddTestCase(new NdmPathQueryTestCase(), Duration::QUICK);
    }
};

static NdmTopologyTestSuite g_ndmTopologyTestSuite;

} // namespace
