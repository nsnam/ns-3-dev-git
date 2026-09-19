/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 */

#include "ns3/log.h"
#include "ndm-topology-helper.h"

#include "ndm-link-loss-model.h"

#include "ns3/random-variable-stream.h"

namespace ns3
{

Ptr<NdmLinkLossModel>
NdmTopologyHelper::MakeLinkLoss(const Opts& opts, uint64_t linkIndex)
{
    if (opts.lossMode == NdmLinkLossModel::Mode::NONE)
    {
        return nullptr;
    }
    Ptr<NdmLinkLossModel> loss = CreateObject<NdmLinkLossModel>();
    loss->SetAttribute("Mode", EnumValue<NdmLinkLossModel::Mode>(opts.lossMode));
    loss->SetAttribute("LossProbability", DoubleValue(opts.lossProbability));
    loss->SetAttribute("BurstLength", UintegerValue(opts.burstLen));
    Ptr<RngStream> rng = CreateObject<RngStream>();
    rng->SetSeed(opts.seed + linkIndex);
    loss->SetRng(rng);
    return loss;
}

Ptr<NdmTopology>
NdmTopologyHelper::CreateFatTree(uint32_t k, uint32_t d, const Opts& opts)
{
    NS_ABORT_MSG_UNLESS(k >= 4 && k % 2 == 0, "CreateFatTree: k must be even and >= 4");
    NS_ABORT_MSG_UNLESS(d >= 2 && d % 2 == 0, "CreateFatTree: d must be even and >= 2");

    const uint32_t toRsPerPod = k / 2;
    const uint32_t L = d * toRsPerPod; // ToRs
    const uint32_t H = d * toRsPerPod * toRsPerPod; // hosts
    const uint32_t S = d * k / 4; // spines
    NS_ABORT_MSG_UNLESS(d * k % 4 == 0, "CreateFatTree: d*k must be divisible by 4");

    Ptr<NdmTopology> topo = CreateObject<NdmTopology>();

    // Hosts: host(p, j, s) ordinal = p*(k/2)^2 + j*(k/2) + s
    for (uint32_t p = 0; p < d; p++)
        for (uint32_t j = 0; j < toRsPerPod; j++)
            for (uint32_t s = 0; s < toRsPerPod; s++)
            {
                topo->AddNode(NdmNodeKind::HOST, p * toRsPerPod * toRsPerPod + j * toRsPerPod + s);
            }
    // ToRs: ordinal (p, j) -> node id H + p*(k/2) + j
    for (uint32_t p = 0; p < d; p++)
        for (uint32_t j = 0; j < toRsPerPod; j++)
        {
            topo->AddNode(NdmNodeKind::SWITCH, p * toRsPerPod + j);
        }
    // Spines: ordinal s -> node id H + L + s
    for (uint32_t s = 0; s < S; s++)
    {
        topo->AddNode(NdmNodeKind::SWITCH, L + s);
    }

    uint64_t linkIdx = 0;
    auto addLoss = [&]() { return MakeLinkLoss(opts, linkIdx++); };

    // Host links: host(p,j,s) -- ToR(p,j)
    for (uint32_t p = 0; p < d; p++)
        for (uint32_t j = 0; j < toRsPerPod; j++)
            for (uint32_t s = 0; s < toRsPerPod; s++)
            {
                const uint32_t host = p * toRsPerPod * toRsPerPod + j * toRsPerPod + s;
                const uint32_t tor = H + p * toRsPerPod + j;
                topo->AddLink(host, tor, opts.bps, opts.delay, -1, -1, false,
                              opts.queueMaxPackets, addLoss());
            }
    // Spine links: spine s slot t -> ToR (s*k + t) mod L
    for (uint32_t s = 0; s < S; s++)
        for (uint32_t t = 0; t < k; t++)
        {
            const uint32_t spine = H + L + s;
            const uint32_t tor = H + (s * k + t) % L;
            topo->AddLink(spine, tor, opts.bps, opts.delay, -1, -1, false,
                          opts.queueMaxPackets, addLoss());
        }

    return topo;
}

Ptr<NdmTopology>
NdmTopologyHelper::CreateMultiRail(uint32_t pods,
                                   uint32_t gpusPerPod,
                                   uint32_t rails,
                                   uint32_t spinesPerRail,
                                   uint32_t nicPerRail,
                                   const Opts& opts)
{
    NS_ABORT_MSG_UNLESS(pods >= 1 && gpusPerPod >= 1 && rails >= 1 && spinesPerRail >= 1 &&
                            nicPerRail >= 1,
                        "CreateMultiRail: parameters must be >= 1");

    const uint32_t G = pods * gpusPerPod; // GPU nodes
    const uint32_t T = pods * rails; // ToRs: ToR_r(p) node id G + r*pods + p
    const uint32_t Sp = rails * spinesPerRail; // Spine_r(s) node id G + T + r*spinesPerRail + s

    Ptr<NdmTopology> topo = CreateObject<NdmTopology>();

    for (uint32_t p = 0; p < pods; p++)
        for (uint32_t g = 0; g < gpusPerPod; g++)
        {
            topo->AddNode(NdmNodeKind::GPU, p * gpusPerPod + g);
        }
    for (uint32_t r = 0; r < rails; r++)
        for (uint32_t p = 0; p < pods; p++)
        {
            topo->AddNode(NdmNodeKind::SWITCH, r * pods + p);
        }
    for (uint32_t r = 0; r < rails; r++)
        for (uint32_t s = 0; s < spinesPerRail; s++)
        {
            topo->AddNode(NdmNodeKind::SWITCH, T + r * spinesPerRail + s);
        }

    uint64_t linkIdx = 0;
    auto addLoss = [&]() { return MakeLinkLoss(opts, linkIdx++); };

    // GPU NIC links: GPU(p,g) -- ToR_r(p), nicPerRail parallel links per (gpu, rail)
    for (uint32_t p = 0; p < pods; p++)
        for (uint32_t g = 0; g < gpusPerPod; g++)
            for (uint32_t r = 0; r < rails; r++)
                for (uint32_t n = 0; n < nicPerRail; n++)
                {
                    const uint32_t gpu = p * gpusPerPod + g;
                    const uint32_t tor = G + r * pods + p;
                    topo->AddLink(gpu, tor, opts.bps, opts.delay, int32_t(r), -1, false,
                                  opts.queueMaxPackets, addLoss());
                }
    // Rail spine links: ToR_r(p) -- Spine_r(s) (complete bipartite per rail)
    for (uint32_t r = 0; r < rails; r++)
        for (uint32_t p = 0; p < pods; p++)
            for (uint32_t s = 0; s < spinesPerRail; s++)
            {
                const uint32_t tor = G + r * pods + p;
                const uint32_t spine = G + T + r * spinesPerRail + s;
                topo->AddLink(tor, spine, opts.bps, opts.delay, int32_t(r), -1, false,
                              opts.queueMaxPackets, addLoss());
            }

    return topo;
}

Ptr<NdmTopology>
NdmTopologyHelper::CreateMultiPlane(uint32_t planes,
                                    PlaneKind kind,
                                    uint32_t ringHosts,
                                    uint32_t fatTreeK,
                                    uint32_t fatTreeD,
                                    bool crossPlane,
                                    const Opts& opts)
{
    NS_ABORT_MSG_UNLESS(planes >= 1, "CreateMultiPlane: planes must be >= 1");

    const uint32_t H = (kind == PlaneKind::RING) ? ringHosts : fatTreeD * (fatTreeK / 2) * (fatTreeK / 2);
    if (kind == PlaneKind::RING)
    {
        NS_ABORT_MSG_UNLESS(H >= 3, "CreateMultiPlane: ring needs >= 3 hosts");
    }
    else
    {
        NS_ABORT_MSG_UNLESS(fatTreeK >= 4 && fatTreeK % 2 == 0 && fatTreeD >= 2 &&
                                fatTreeD % 2 == 0,
                            "CreateMultiPlane: fat-tree params (k even >=4, d even >=2)");
    }

    const uint32_t toRsPerPod = fatTreeK / 2;
    const uint32_t L = fatTreeD * toRsPerPod; // ToRs per plane (FAT_TREE)
    const uint32_t S = fatTreeD * fatTreeK / 4; // spines per plane (FAT_TREE)
    const uint32_t switchesPerPlane = (kind == PlaneKind::RING) ? H : (L + S);

    Ptr<NdmTopology> topo = CreateObject<NdmTopology>();

    // Shared host set
    for (uint32_t i = 0; i < H; i++)
    {
        topo->AddNode(NdmNodeKind::HOST, i);
    }
    // Plane switches: plane p, ordinal w -> node id H + p*switchesPerPlane + w
    for (uint32_t p = 0; p < planes; p++)
        for (uint32_t w = 0; w < switchesPerPlane; w++)
        {
            topo->AddNode(NdmNodeKind::SWITCH, p * switchesPerPlane + w);
        }

    uint64_t linkIdx = 0;
    auto addLoss = [&]() { return MakeLinkLoss(opts, linkIdx++); };

    // Plane-local links, plane by plane (host ports thus index the planes).
    for (uint32_t p = 0; p < planes; p++)
    {
        const uint32_t swBase = H + p * switchesPerPlane;
        if (kind == PlaneKind::RING)
        {
            for (uint32_t i = 0; i < H; i++)
            {
                topo->AddLink(i, swBase + i, opts.bps, opts.delay, -1, int32_t(p), false,
                              opts.queueMaxPackets, addLoss());
            }
            for (uint32_t i = 0; i < H; i++)
            {
                topo->AddLink(swBase + i, swBase + (i + 1) % H, opts.bps, opts.delay, -1,
                              int32_t(p), false, opts.queueMaxPackets, addLoss());
            }
        }
        else // FAT_TREE
        {
            // host links: host(pod,j,s) -- ToR_p(pod,j)
            for (uint32_t pod = 0; pod < fatTreeD; pod++)
                for (uint32_t j = 0; j < toRsPerPod; j++)
                    for (uint32_t s = 0; s < toRsPerPod; s++)
                    {
                        const uint32_t host = pod * toRsPerPod * toRsPerPod +
                                              j * toRsPerPod + s;
                        const uint32_t tor = swBase + pod * toRsPerPod + j;
                        topo->AddLink(host, tor, opts.bps, opts.delay, -1, int32_t(p), false,
                                      opts.queueMaxPackets, addLoss());
                    }
            // spine links: Spine_p(s) -- ToR_p((s*k + t) mod L)
            for (uint32_t s = 0; s < S; s++)
                for (uint32_t t = 0; t < fatTreeK; t++)
                {
                    const uint32_t spine = swBase + L + s;
                    const uint32_t tor = swBase + (s * fatTreeK + t) % L;
                    topo->AddLink(spine, tor, opts.bps, opts.delay, -1, int32_t(p), false,
                                  opts.queueMaxPackets, addLoss());
                }
        }
    }

    // Optional cross-plane links: equal ordinals of adjacent planes.
    if (crossPlane)
    {
        for (uint32_t p = 0; p < planes; p++)
        {
            const uint32_t a = H + p * switchesPerPlane;
            const uint32_t b = H + ((p + 1) % planes) * switchesPerPlane;
            for (uint32_t w = 0; w < switchesPerPlane; w++)
            {
                topo->AddLink(a + w, b + w, opts.bps, opts.delay, -1, -1, true,
                              opts.queueMaxPackets, addLoss());
            }
        }
    }

    return topo;
}

} // namespace ns3
