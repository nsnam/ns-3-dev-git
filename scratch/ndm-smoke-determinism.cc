/*
 * ndm-sys project file — Apache License 2.0 (see LICENSE-PROJECT).
 *
 * Phase 0.6 deterministic smoke scenario (the CI determinism pair input).
 *
 * What it does:
 *   2 nodes, one P2P link (1 Gbps, 1 ms), one sender emitting `nPackets`
 *   1000-byte packets. Inter-packet gap = 1 ms + U(0, 500 us) drawn from an
 *   ns3::RngStream seeded by --seed (tools/seed.py is the canonical cell seed
 *   source; any integer works here).
 *
 * Determinism contract (PPS-safety rules, AGENTS.md D7):
 *   - all randomness through the seeded RngStream; no wall-clock reads;
 *   - stdout carries exactly one metrics line, comparable bit-for-bit:
 *       SMOKE seed=<seed> sent=<n> delivered=<n> lastRxNs=<ns> sumFlightNs=<ns>
 *   - same seed => identical line; different seed => different line
 *     (asserted by tools/check.sh).
 */

#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"

#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>

using namespace ns3;

namespace
{

/// Carries the send timestamp across the link so the receiver can measure flight time.
class SmokeTimeHdr
    : public Header
{
  public:
    static TypeId GetTypeId()
    {
        static TypeId tid =
            TypeId("ndm::SmokeTimeHdr").SetParent<Header>().AddConstructor<SmokeTimeHdr>();
        return tid;
    }
    SmokeTimeHdr() = default;
    explicit SmokeTimeHdr(uint64_t sendNs) : m_sendNs(sendNs) {}
    uint64_t GetSendNs() const
    {
        return m_sendNs;
    }
    uint32_t GetSerializedSize() const override
    {
        return 8;
    }
    void Serialize(Buffer::Iterator start) const override
    {
        start.WriteHtonU64(m_sendNs);
    }
    uint32_t Deserialize(Buffer::Iterator start) override
    {
        m_sendNs = start.ReadNtohU64();
        return 8;
    }
    void Print(std::ostream& os) const override
    {
        os << "SmokeTimeHdr(sendNs=" << m_sendNs << ")";
    }
    TypeId GetInstanceTypeId() const override
    {
        return GetTypeId();
    }

  private:
    uint64_t m_sendNs{0};
};

struct SmokeStats
{
    uint32_t delivered{0};
    uint64_t lastRxNs{0};
    uint64_t sumFlightNs{0};
};

SmokeStats g_stats;

bool
RxCallback(Ptr<const NetDevice>, Ptr<const Packet> p, uint16_t /*protocol*/, const Address&)
{
    SmokeTimeHdr hdr;
    p->PeekHeader(hdr);
    SmokeStats& s = g_stats;
    s.delivered++;
    uint64_t nowNs = uint64_t(Simulator::Now().GetPicoSeconds()) / 1000;
    s.lastRxNs = nowNs;
    s.sumFlightNs += nowNs - hdr.GetSendNs();
    return true;
}

} // namespace

int
main(int argc, char* argv[])
{
    uint64_t seed = 42;
    uint32_t nPackets = 50;
    CommandLine cmd;
    cmd.AddValue("seed", "RngStream seed (see tools/seed.py for cell seeds)", seed);
    cmd.AddValue("nPackets", "number of packets to send", nPackets);
    cmd.Parse(argc, argv);

    // --- topology: 2 nodes, 1 link (3.42 has no NodeContainer; the P2P
    // helper installs directly on the two nodes) -----------------------------
    Ptr<Node> n0 = CreateObject<Node>();
    Ptr<Node> n1 = CreateObject<Node>();
    PointToPointHelper pp;
    pp.SetDeviceAttribute("DataRate", DataRateValue(DataRate("1Gbps")));
    pp.SetChannelAttribute("Delay", TimeValue(MilliSeconds(1)));
    NetDeviceContainer devices = pp.Install(n0, n1);

    Ptr<NetDevice> txDev = devices.Get(0);
    devices.Get(1)->SetReceiveCallback(&RxCallback);

    // --- traffic: seeded inter-packet jitter ---------------------------------
    // 3.42: RngStream is a plain class (seeded in the constructor) and the
    // RandomVariable models are gone — draw jitter directly. MilliSeconds()
    // takes an integer, so the jitter is drawn in whole us (a fractional
    // double would truncate and the "different seed" gate would be vacuous).
    auto rng = std::make_unique<RngStream>(static_cast<uint32_t>(seed), 0, 0);

    Mac48Address dst = Mac48Address("00:00:00:00:00:02");
    uint32_t sent = 0;
    std::function<void()> sendNext = [&, dst]() {
        if (sent >= nPackets)
        {
            return;
        }
        Ptr<Packet> p = Create<Packet>(1000);
        SmokeTimeHdr hdr(uint64_t(Simulator::Now().GetPicoSeconds()) / 1000);
        p->AddHeader(hdr);
        bool ok = txDev->Send(p, dst, 0x86DD);
        (void)ok;
        NS_ABORT_MSG_UNLESS(ok, "smoke: send failed");
        sent++;
        if (sent < nPackets)
        {
            const uint32_t jitterUs = static_cast<uint32_t>(rng->RandU01() * 500.0);
            Time gap = MilliSeconds(1) + MicroSeconds(jitterUs);
            Simulator::Schedule(gap, sendNext);
        }
    };
    Simulator::Schedule(MilliSeconds(10), sendNext);

    // Stop once every packet has been received (deterministic horizon).
    std::function<void()> checkDone = [&]() {
        if (g_stats.delivered >= nPackets)
        {
            Simulator::Stop();
        }
        else
        {
            Simulator::Schedule(MilliSeconds(1), checkDone);
        }
    };
    // Start the poller after the last possible send + slack.
    Simulator::Schedule(Seconds(1), checkDone);

    Simulator::Run();

    std::printf("SMOKE seed=%llu sent=%u delivered=%u lastRxNs=%llu sumFlightNs=%llu\n",
                (unsigned long long)seed,
                sent,
                g_stats.delivered,
                (unsigned long long)g_stats.lastRxNs,
                (unsigned long long)g_stats.sumFlightNs);

    Simulator::Destroy();
    return 0;
}
