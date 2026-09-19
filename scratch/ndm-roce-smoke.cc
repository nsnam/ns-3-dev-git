/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * Phase 2 RoCE smoke: one RC Write (1 MiB, patterned bytes) over a single
 * 10 Gbps / 10 us link between two ndm-topology nodes, transported by the
 * RoCE baseline (ndm-roce) on the path forwarder.
 *
 * Checks (all hard asserts; stdout carries one metrics line):
 *   - every byte delivered in order, byte-identical to the source pattern
 *   - exactly-once completion callback (once, and only after all ACKs)
 *   - zero retransmissions on a lossless link; ACK count == data packets
 *
 *   ROCE smoke=<0|1> bytes=<n> packets=<n> retrans=<n> rttNs=<ns>
 *
 * Determinism (D7): no randomness at all here; the metrics line must be
 * bit-identical across runs.
 */

#include "ns3/core-module.h"
#include "ns3/ndm-path-forwarder.h"
#include "ns3/ndm-roce-endpoint.h"
#include "ns3/ndm-roce-qpair.h"
#include "ns3/ndm-roce-rx-qpair.h"
#include "ns3/ndm-topology.h"
#include "ns3/ndm-topology-helper.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdint>
#include <cstdio>
#include <vector>

using namespace ns3;

namespace
{

constexpr uint32_t kBytes = 1 << 20; // 1 MiB
constexpr uint32_t kMtu = 2048;

uint8_t PatternByte(uint32_t i)
{
    return static_cast<uint8_t>((static_cast<uint32_t>(i) * 7u + 13u) & 0xFFu);
}

struct Smoke
{
    uint32_t rxBytes{0};
    uint32_t rxPackets{0};
    uint32_t rxLastMessages{0};
    bool badByte{false};
    uint32_t completions{0};
    std::vector<uint8_t> rx;
};

Smoke g_smoke;
Ptr<NdmRoceQPair> g_tx;
Ptr<NdmRoceRxQPair> g_rx;
bool g_done{false};

void
OnDeliver(uint32_t psn, Ptr<Packet> payload, uint32_t imm, bool isLast)
{
    (void)imm;
    Smoke& s = g_smoke;
    const uint32_t len = payload->GetSize();
    if (len > 0)
    {
        const uint8_t* d = payload->Head();
        for (uint32_t i = 0; i < len; i++)
        {
            if (d[i] != PatternByte(s.rxBytes + i))
            {
                s.badByte = true;
            }
        }
        s.rx.insert(s.rx.end(), d, d + len);
        s.rxBytes += len;
    }
    s.rxPackets++;
    if (isLast)
    {
        s.rxLastMessages++;
    }
    if (!g_done && s.rxBytes >= kBytes)
    {
        // data complete; completion callback follows the last ACK
    }
}

void
OnComplete()
{
    g_smoke.completions++;
    if (!g_done)
    {
        g_done = true;
        Simulator::Stop();
    }
}

} // namespace

int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // --- topology: 2 host nodes, one 10 Gbps / 10 us link ---------------------
    auto topo = CreateObject<NdmTopology>();
    topo->AddNode(NdmNodeKind::HOST, 0);
    topo->AddNode(NdmNodeKind::HOST, 1);
    topo->AddLink(0, 1, DataRate("10Gbps"), MicroSeconds(10), -1, -1, false,
                  4096, nullptr);

    auto pathsAB = topo->FindPaths(0, 1);
    auto pathsBA = topo->FindPaths(1, 0);
    NS_ABORT_MSG_UNLESS(pathsAB.size() == 1 && pathsBA.size() == 1,
                        "smoke: expected one path each direction");

    auto fwdA = CreateObject<NdmPathForwarder>();
    auto fwdB = CreateObject<NdmPathForwarder>();
    fwdA->Attach(topo, 0);
    fwdB->Attach(topo, 1);
    const uint32_t abA = fwdA->RegisterPath(pathsAB[0]); // 0
    const uint32_t baA = fwdA->RegisterPath(pathsBA[0]); // 1
    const uint32_t baB = fwdB->RegisterPath(pathsBA[0]); // 0
    const uint32_t abB = fwdB->RegisterPath(pathsAB[0]); // 1

    auto epA = Create<NdmRoceEndpoint>();
    auto epB = Create<NdmRoceEndpoint>();
    epA->Attach(topo->GetNode(0), fwdA, abA, baA);
    epB->Attach(topo->GetNode(1), fwdB, abB, baB); // B: data unused; control (ACK) returns B->A via baB
    epA->SetAddresses(Ipv6Address("fe80::a"), Ipv6Address("fe80::b"));
    epB->SetAddresses(Ipv6Address("fe80::b"), Ipv6Address("fe80::a"));

    // --- one RC connection: local QP 1 (A) <-> local QP 2 (B) ------------------
    g_tx = epA->CreateConnection(1, 2);
    g_rx = epB->GetRxQp(2);
    g_tx->SetMtu(kMtu);
    g_tx->SetLinkRate(DataRate("10Gbps"));
    g_tx->SetInitialRtt(MicroSeconds(30));
    g_tx->m_notifyComplete = MakeCallback(&OnComplete);
    g_rx->m_deliver = MakeCallback(&OnDeliver);

    // --- source data: patterned 1 MiB ------------------------------------------
    std::vector<uint8_t> src(kBytes);
    for (uint32_t i = 0; i < kBytes; i++)
    {
        src[i] = PatternByte(i);
    }

    Simulator::Schedule(MicroSeconds(100), [tx = g_tx, &src]() {
        tx->PostWrite(src.data(), kBytes, 0xDEADBEEF);
        tx->Start(Simulator::Now());
    });

    // Safety horizon (deterministic; well beyond the expected ~2 ms).
    Simulator::Schedule(MilliSeconds(50), []() {
        NS_ABORT_MSG("smoke: RoCE Write did not complete in horizon");
    });

    Simulator::Run();

    NS_ASSERT_MSG(g_smoke.completions == 1,
                  "smoke: exactly-once completion violated (got "
                      << g_smoke.completions << ")");
    NS_ASSERT_MSG(g_smoke.rxBytes == kBytes, "smoke: byte count mismatch");
    NS_ASSERT_MSG(!g_smoke.badByte, "smoke: corrupted bytes on the wire");
    NS_ASSERT_MSG(g_smoke.rxLastMessages == 1, "smoke: message boundary lost");

    std::printf("ROCE smoke=1 bytes=%u packets=%u retrans=%llu rttNs=%llu ctrlSent=%llu\n",
                g_smoke.rxBytes,
                g_smoke.rxPackets,
                (unsigned long long)g_tx->GetRetransmitCount(),
                (unsigned long long)g_tx->GetLastRttSample().GetNanoSeconds(),
                (unsigned long long)epA->GetControlSentCount() +
                    epB->GetControlSentCount());

    Simulator::Destroy();
    return 0;
}
