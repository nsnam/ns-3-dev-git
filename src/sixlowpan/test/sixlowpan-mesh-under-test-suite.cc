/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "mock-net-device.h"

#include "ns3/boolean.h"
#include "ns3/iana-ieee802-numbers.h"
#include "ns3/mac16-address.h"
#include "ns3/mac48-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/sixlowpan-adaptive-flooding.h"
#include "ns3/sixlowpan-header.h"
#include "ns3/sixlowpan-mesh-under-routing.h"
#include "ns3/sixlowpan-net-device.h"
#include "ns3/sixlowpan-simple-flooding.h"
#include "ns3/test.h"
#include "ns3/uinteger.h"

#include <algorithm>
#include <vector>

using namespace ns3;

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify the FIFO drop policy of the duplicate-detection cache.
 *
 * The base-class cache keeps at most MeshCacheLength entries per
 * originator. Once that capacity is reached, the oldest entry is
 * dropped on each new insertion (FIFO drop policy).
 */
class DuplicateCacheFifoDropTestCase : public TestCase
{
  public:
    DuplicateCacheFifoDropTestCase()
        : TestCase("Duplicate cache drops oldest entry when full")
    {
    }

  private:
    void DoRun() override
    {
        Ptr<SixLowPanSimpleFlooding> policy = CreateObject<SixLowPanSimpleFlooding>();
        policy->SetAttribute("MeshCacheLength", UintegerValue(3));

        Mac16Address orig("00:01");

        policy->RecordPacket(orig, 1);
        policy->RecordPacket(orig, 2);
        policy->RecordPacket(orig, 3);

        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 1), true, "seq 1 should be cached");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 2), true, "seq 2 should be cached");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 3), true, "seq 3 should be cached");

        // Insert seq 4 -- the oldest entry (seq 1) is dropped.
        policy->RecordPacket(orig, 4);

        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 1),
                              false,
                              "seq 1 should have been dropped (FIFO)");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(orig, 4), true, "seq 4 should be cached");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify per-originator cache partitioning under overflow.
 *
 * Each originator has its own FIFO. Overflowing one originator's cache
 * must drop only that originator's oldest entry; it must never drop an
 * entry belonging to a different originator.
 */
class DuplicateCachePerOriginatorTestCase : public TestCase
{
  public:
    DuplicateCachePerOriginatorTestCase()
        : TestCase("Overflowing one originator does not drop another's entries")
    {
    }

  private:
    void DoRun() override
    {
        Ptr<SixLowPanSimpleFlooding> policy = CreateObject<SixLowPanSimpleFlooding>();
        policy->SetAttribute("MeshCacheLength", UintegerValue(2));

        Mac16Address origA("00:01");
        Mac16Address origB("00:02");

        // Fill origB to capacity, then push origA one past capacity.
        policy->RecordPacket(origB, 1);
        policy->RecordPacket(origB, 2);
        policy->RecordPacket(origA, 1);
        policy->RecordPacket(origA, 2);
        policy->RecordPacket(origA, 3); // origA overflows: its oldest (seq 1) is dropped

        // The overflow dropped only origA's oldest entry.
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origA, 1), false, "origA seq 1 dropped (FIFO)");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origA, 2), true, "origA seq 2 still cached");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origA, 3), true, "origA seq 3 cached");

        // origB's entries are untouched by origA's overflow.
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origB, 1), true, "origB seq 1 untouched");
        NS_TEST_ASSERT_MSG_EQ(policy->IsDuplicate(origB, 2), true, "origB seq 2 untouched");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify the default policy schedules and invokes the forward callback.
 */
class SimpleFloodingForwardsTestCase : public TestCase
{
  public:
    SimpleFloodingForwardsTestCase()
        : TestCase("SimpleFlooding invokes the forward callback once")
    {
    }

  private:
    /**
     * @brief Callback used to capture forward invocations.
     * @param packet The forwarded packet.
     */
    void RecordForward(Ptr<Packet> packet)
    {
        m_forwardedPacket = packet;
        m_forwardCount++;
    }

    void DoRun() override
    {
        Ptr<SixLowPanSimpleFlooding> policy = CreateObject<SixLowPanSimpleFlooding>();
        policy->AssignStreams(1);

        Ptr<Packet> packet = Create<Packet>(64);
        Mac16Address orig("00:01");

        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&SimpleFloodingForwardsTestCase::RecordForward, this);

        policy->OnPacketForward(packet, orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_forwardCount, 1, "Forward callback should fire exactly once");
        NS_TEST_ASSERT_MSG_EQ(m_forwardedPacket, packet, "Same packet pointer should be forwarded");
    }

    void DoTeardown() override
    {
        Simulator::Destroy();
    }

    Ptr<Packet> m_forwardedPacket; ///< Packet captured at the forward callback.
    int m_forwardCount{0};         ///< Number of times the forward callback was invoked.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify adaptive flooding forwards a packet on a quiet channel.
 *
 * With c staying below k, the timer fires once, forwards the packet, and
 * then resets (stops), so exactly one forward is expected.
 */
class AdaptiveFloodingForwardsWhenQuietTestCase : public TestCase
{
  public:
    AdaptiveFloodingForwardsWhenQuietTestCase()
        : TestCase("Adaptive flooding forwards when no duplicates are heard")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet (unused).
     */
    void RecordForward(Ptr<Packet> packet [[maybe_unused]])
    {
        m_forwardCount++;
        m_forwardTime = Simulator::Now();
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(1));
        trickle->AssignStreams(1);

        Ptr<Packet> packet = Create<Packet>(64);
        Mac16Address orig("00:01");

        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingForwardsWhenQuietTestCase::RecordForward, this);

        trickle->OnPacketForward(packet, orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);

        Simulator::Stop(MilliSeconds(500));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_forwardCount, 1, "Should forward once when c < k");
        // The timer starts at Imin (Enable + Reset), so the firing lands in
        // [Imin/2, Imin) for every RNG draw, never in a later interval.
        NS_TEST_ASSERT_MSG_LT(m_forwardTime,
                              MilliSeconds(10),
                              "First forward should happen within MinInterval");
    }

    int m_forwardCount{0}; ///< Number of forward-callback invocations.
    Time m_forwardTime;    ///< Time of the (single) forward.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify adaptive flooding suppresses while the packet is covered.
 *
 * A duplicate is heard every few milliseconds for the whole run, keeping
 * c >= k at every interval, so the packet is never forwarded.
 */
class AdaptiveFloodingSuppressesWhileCoveredTestCase : public TestCase
{
  public:
    AdaptiveFloodingSuppressesWhileCoveredTestCase()
        : TestCase("Adaptive flooding suppresses while duplicates keep arriving")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet (unused).
     */
    void RecordForward(Ptr<Packet> packet [[maybe_unused]])
    {
        m_forwardCount++;
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(1));
        trickle->SetAttribute("MaxForwardingDelay", TimeValue(MilliSeconds(1000)));
        trickle->AssignStreams(1);

        Ptr<Packet> packet = Create<Packet>(64);
        Mac16Address orig("00:01");

        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingSuppressesWhileCoveredTestCase::RecordForward, this);

        trickle->OnPacketForward(packet, orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);

        // A neighbour rebroadcasts every 5 ms, keeping c >= k at every interval.
        for (uint32_t ms = 3; ms <= 395; ms += 5)
        {
            Simulator::Schedule(MilliSeconds(ms),
                                &SixLowPanAdaptiveFlooding::OnDuplicateReceived,
                                trickle,
                                Address(orig),
                                uint8_t(1));
        }

        Simulator::Stop(MilliSeconds(400));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_forwardCount, 0, "Should suppress while c >= k throughout");
    }

    int m_forwardCount{0}; ///< Number of forward-callback invocations.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify a zero redundancy constant disables suppression.
 */
class AdaptiveFloodingZeroRedundancyAlwaysForwardsTestCase : public TestCase
{
  public:
    AdaptiveFloodingZeroRedundancyAlwaysForwardsTestCase()
        : TestCase("Adaptive flooding with k = 0 always forwards")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet (unused).
     */
    void RecordForward(Ptr<Packet> packet [[maybe_unused]])
    {
        m_forwardCount++;
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(0));
        trickle->SetAttribute("DuplicateThreshold", UintegerValue(0));
        trickle->AssignStreams(1);

        Ptr<Packet> packet = Create<Packet>(64);
        Mac16Address orig("00:01");

        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingZeroRedundancyAlwaysForwardsTestCase::RecordForward,
                         this);

        trickle->OnPacketForward(packet, orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);

        // Even with duplicates, k = 0 means suppression is disabled.
        Simulator::Schedule(MilliSeconds(1),
                            &SixLowPanAdaptiveFlooding::OnDuplicateReceived,
                            trickle,
                            Address(orig),
                            uint8_t(1));
        Simulator::Schedule(MilliSeconds(2),
                            &SixLowPanAdaptiveFlooding::OnDuplicateReceived,
                            trickle,
                            Address(orig),
                            uint8_t(1));

        Simulator::Stop(MilliSeconds(500));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_forwardCount, 1, "k = 0 should always forward");
    }

    int m_forwardCount{0}; ///< Number of forward-callback invocations.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify pending packets are forwarded on separate firings.
 *
 * With two packets pending, each Trickle
 * firing forwards exactly one packet, in arrival order, at distinct times.
 */
class AdaptiveFloodingOnePerFiringTestCase : public TestCase
{
  public:
    AdaptiveFloodingOnePerFiringTestCase()
        : TestCase("Adaptive flooding forwards one packet per firing, in order")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet.
     */
    void RecordForward(Ptr<Packet> packet)
    {
        m_times.push_back(Simulator::Now());
        m_uids.push_back(packet->GetUid());
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(1));
        trickle->AssignStreams(1);

        Mac16Address orig("00:01");
        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingOnePerFiringTestCase::RecordForward, this);

        Ptr<Packet> first = Create<Packet>(64);
        Ptr<Packet> second = Create<Packet>(64);

        // Two packets arrive before the first firing.
        trickle->OnPacketForward(first, orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);
        trickle->OnPacketForward(second, orig, /*seqNo=*/2, /*hopsLeft=*/5, cb);

        Simulator::Stop(MilliSeconds(500));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_times.size(), 2, "Both pending packets should be forwarded");
        NS_TEST_ASSERT_MSG_EQ(m_uids[0], first->GetUid(), "Head of the queue goes first");
        NS_TEST_ASSERT_MSG_EQ(m_uids[1], second->GetUid(), "Second packet goes second");
        NS_TEST_ASSERT_MSG_GT(m_times[1],
                              m_times[0],
                              "The packets should be forwarded on separate firings");
    }

    std::vector<Time> m_times;    ///< Forward times.
    std::vector<uint64_t> m_uids; ///< Forwarded packet UIDs, in order.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify the default consistency mode counts ANY duplicate.
 *
 * With HeadOfLineConsistency disabled (the default), duplicates of a packet
 * unrelated to the head of the pending queue are still channel-level
 * evidence of coverage, so they must suppress.
 */
class AdaptiveFloodingAnyDuplicateSuppressesTestCase : public TestCase
{
  public:
    AdaptiveFloodingAnyDuplicateSuppressesTestCase()
        : TestCase("Default consistency mode suppresses on any duplicate")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet (unused).
     */
    void RecordForward(Ptr<Packet> packet [[maybe_unused]])
    {
        m_forwardCount++;
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(1));
        trickle->SetAttribute("MaxForwardingDelay", TimeValue(MilliSeconds(1000)));
        trickle->AssignStreams(1);

        Mac16Address pendingOrig("00:01");
        Mac16Address otherOrig("00:02");

        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingAnyDuplicateSuppressesTestCase::RecordForward, this);

        trickle->OnPacketForward(Create<Packet>(64), pendingOrig, /*seqNo=*/1, /*hopsLeft=*/5, cb);

        // Duplicates of a DIFFERENT packet arrive every 5 ms: in the default
        // (channel-level) mode they count as consistent events all the same.
        for (uint32_t ms = 3; ms <= 395; ms += 5)
        {
            Simulator::Schedule(MilliSeconds(ms),
                                &SixLowPanAdaptiveFlooding::OnDuplicateReceived,
                                trickle,
                                Address(otherOrig),
                                uint8_t(9));
        }

        Simulator::Stop(MilliSeconds(400));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_forwardCount, 0, "Any duplicate must suppress in the default mode");
    }

    int m_forwardCount{0}; ///< Number of forward-callback invocations.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify the timer stops when the queue drains and restarts at Imin.
 *
 * A first packet is forwarded and the queue drains (the timer must stop);
 * a second packet arriving much later must restart the timer from the
 * minimum interval and be forwarded within MinInterval of ITS arrival.
 */
class AdaptiveFloodingRestartAfterDrainTestCase : public TestCase
{
  public:
    AdaptiveFloodingRestartAfterDrainTestCase()
        : TestCase("Timer stops on drain and restarts at Imin for a later packet")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet (unused).
     */
    void RecordForward(Ptr<Packet> packet [[maybe_unused]])
    {
        m_times.push_back(Simulator::Now());
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(1));
        trickle->AssignStreams(1);

        Mac16Address orig("00:01");
        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingRestartAfterDrainTestCase::RecordForward, this);

        // First packet at t = 0 drains the queue; second arrives at 300 ms,
        // long after every interval a still-running timer could be in.
        trickle->OnPacketForward(Create<Packet>(64), orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);
        Simulator::Schedule(MilliSeconds(300),
                            &SixLowPanAdaptiveFlooding::OnPacketForward,
                            trickle,
                            Create<Packet>(64),
                            Address(orig),
                            uint8_t(2),
                            uint8_t(5),
                            cb);

        Simulator::Stop(MilliSeconds(500));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_times.size(), 2, "Both packets should be forwarded");
        NS_TEST_ASSERT_MSG_LT(m_times[0], MilliSeconds(10), "First forward within MinInterval");
        NS_TEST_ASSERT_MSG_GT(m_times[1], MilliSeconds(300), "Second forward after its arrival");
        NS_TEST_ASSERT_MSG_LT(m_times[1],
                              MilliSeconds(310),
                              "Restarted timer must fire within MinInterval of the arrival");
    }

    std::vector<Time> m_times; ///< Forward times.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify the duplicate threshold kills a covered packet at transmit.
 *
 * With DuplicateThreshold = 2, a pending packet
 * whose duplicate was overheard is suppressed at the firing and the NEXT
 * pending packet is forwarded in its place.
 */
class AdaptiveFloodingDuplicateThresholdKillsHeadTestCase : public TestCase
{
  public:
    AdaptiveFloodingDuplicateThresholdKillsHeadTestCase()
        : TestCase("Duplicate threshold kills the covered head, next packet goes out")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet.
     */
    void RecordForward(Ptr<Packet> packet)
    {
        m_forwardedUids.push_back(packet->GetUid());
    }

    /**
     * @brief Suppressed-trace callback.
     * @param packet The suppressed packet.
     */
    void RecordSuppressed(Ptr<const Packet> packet)
    {
        m_suppressedUids.push_back(packet->GetUid());
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(0));
        trickle->SetAttribute("DuplicateThreshold", UintegerValue(2));
        trickle->AssignStreams(1);
        trickle->TraceConnectWithoutContext(
            "PacketSuppressed",
            MakeCallback(&AdaptiveFloodingDuplicateThresholdKillsHeadTestCase::RecordSuppressed,
                         this));

        Mac16Address orig("00:01");
        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingDuplicateThresholdKillsHeadTestCase::RecordForward, this);

        Ptr<Packet> covered = Create<Packet>(64);
        Ptr<Packet> fresh = Create<Packet>(64);
        trickle->OnPacketForward(covered, orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);
        trickle->OnPacketForward(fresh, orig, /*seqNo=*/2, /*hopsLeft=*/5, cb);

        // One overheard duplicate takes the first packet to the threshold
        // before the first firing (which happens no earlier than 5 ms).
        Simulator::Schedule(MilliSeconds(1),
                            &SixLowPanAdaptiveFlooding::OnDuplicateReceived,
                            trickle,
                            Address(orig),
                            uint8_t(1));

        Simulator::Stop(MilliSeconds(500));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_suppressedUids.size(), 1, "The covered packet is suppressed");
        NS_TEST_ASSERT_MSG_EQ(m_suppressedUids[0], covered->GetUid(), "Suppressed = the covered");
        NS_TEST_ASSERT_MSG_EQ(m_forwardedUids.size(), 1, "Only the fresh packet is forwarded");
        NS_TEST_ASSERT_MSG_EQ(m_forwardedUids[0], fresh->GetUid(), "Forwarded = the fresh one");
    }

    std::vector<uint64_t> m_forwardedUids;  ///< UIDs handed to the forward callback.
    std::vector<uint64_t> m_suppressedUids; ///< UIDs reported by the suppressed trace.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Verify each packet is discarded at its own deadline.
 *
 * Two packets arrive 50 ms apart while suppression holds. The first must
 * be discarded exactly MaxForwardingDelay after ITS arrival, the second
 * exactly MaxForwardingDelay after its own, later arrival.
 */
class AdaptiveFloodingPerPacketDeadlineTestCase : public TestCase
{
  public:
    AdaptiveFloodingPerPacketDeadlineTestCase()
        : TestCase("Suppressed packets are discarded at per-packet deadlines")
    {
    }

  private:
    /**
     * @brief Forward callback.
     * @param packet The forwarded packet (unused).
     */
    void RecordForward(Ptr<Packet> packet [[maybe_unused]])
    {
        m_forwardCount++;
    }

    /**
     * @brief Discard-trace callback.
     * @param packet The discarded packet (unused).
     */
    void RecordDiscard(Ptr<const Packet> packet [[maybe_unused]])
    {
        m_discardTimes.push_back(Simulator::Now());
    }

    /**
     * @brief Pending-queue-size trace callback.
     * @param oldSize The previous queue size (unused).
     * @param newSize The new queue size.
     */
    void RecordQueueSize(uint32_t oldSize [[maybe_unused]], uint32_t newSize)
    {
        m_maxQueue = std::max(m_maxQueue, newSize);
    }

    void DoRun() override
    {
        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(1));
        trickle->SetAttribute("MaxForwardingDelay", TimeValue(MilliSeconds(100)));
        trickle->AssignStreams(1);
        trickle->TraceConnectWithoutContext(
            "PacketDiscarded",
            MakeCallback(&AdaptiveFloodingPerPacketDeadlineTestCase::RecordDiscard, this));
        trickle->TraceConnectWithoutContext(
            "PendingQueueSize",
            MakeCallback(&AdaptiveFloodingPerPacketDeadlineTestCase::RecordQueueSize, this));

        Mac16Address orig("00:01");
        SixLowPanMeshUnderRouting::ForwardCallback cb =
            MakeCallback(&AdaptiveFloodingPerPacketDeadlineTestCase::RecordForward, this);

        // First packet at t = 0, second at t = 50 ms.
        trickle->OnPacketForward(Create<Packet>(64), orig, /*seqNo=*/1, /*hopsLeft=*/5, cb);
        Simulator::Schedule(MilliSeconds(50),
                            &SixLowPanAdaptiveFlooding::OnPacketForward,
                            trickle,
                            Create<Packet>(64),
                            Address(orig),
                            uint8_t(2),
                            uint8_t(5),
                            cb);

        // Duplicates every 5 ms keep c >= k, so nothing is ever forwarded.
        for (uint32_t ms = 3; ms <= 395; ms += 5)
        {
            Simulator::Schedule(MilliSeconds(ms),
                                &SixLowPanAdaptiveFlooding::OnDuplicateReceived,
                                trickle,
                                Address(orig),
                                uint8_t(1));
        }

        Simulator::Stop(MilliSeconds(400));
        Simulator::Run();
        Simulator::Destroy();

        NS_TEST_ASSERT_MSG_EQ(m_forwardCount, 0, "Suppression should hold for the whole run");
        NS_TEST_ASSERT_MSG_EQ(m_discardTimes.size(), 2, "Both packets should be discarded");
        NS_TEST_ASSERT_MSG_EQ(m_discardTimes[0],
                              MilliSeconds(100),
                              "First packet discarded at its own deadline");
        NS_TEST_ASSERT_MSG_EQ(m_discardTimes[1],
                              MilliSeconds(150),
                              "Second packet discarded at its own, later deadline");
        NS_TEST_ASSERT_MSG_EQ(m_maxQueue, 2, "Both packets were pending simultaneously");
    }

    int m_forwardCount{0};            ///< Number of forward-callback invocations.
    std::vector<Time> m_discardTimes; ///< Times of PacketDiscarded events.
    uint32_t m_maxQueue{0};           ///< Maximum observed pending-queue size.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Base for device-level mesh-under tests.
 *
 * Builds a SixLowPanNetDevice over a MockNetDevice, injects mesh frames
 * through the underlying device and captures both the frames the device
 * re-broadcasts and the packets it delivers locally. Subclasses exercise
 * the device public API end to end, verifying that the policy
 * delegation preserves the historical device behavior.
 */
class MeshUnderDeviceTestCase : public TestCase
{
  public:
    /**
     * @brief Constructor.
     * @param name The test case name.
     */
    MeshUnderDeviceTestCase(std::string name)
        : TestCase(name)
    {
    }

  protected:
    /**
     * @brief Build the node, the underlying MockNetDevice and the 6LoWPAN device.
     *
     * The underlying device gets the MAC address 00:00:00:00:00:01, so the
     * node's 16-bit mesh address is 00:01.
     *
     * @param useMesh The device's UseMeshUnder attribute (mesh-under membership).
     * @param forwardMesh The device's ForwardMesh attribute (relaying).
     */
    void SetupDevice(bool useMesh = true, bool forwardMesh = true)
    {
        Ptr<Node> node = CreateObject<Node>();

        m_mock = CreateObject<MockNetDevice>();
        node->AddDevice(m_mock);
        m_mock->SetNode(node);
        m_mock->SetAddress(Mac48Address("00:00:00:00:00:01"));
        m_mock->SetMtu(150);
        m_mock->SetSendCallback(MakeCallback(&MeshUnderDeviceTestCase::CaptureSend, this));

        m_dev = CreateObject<SixLowPanNetDevice>();
        node->AddDevice(m_dev);
        m_dev->SetNetDevice(m_mock);
        m_dev->SetAttribute("UseMeshUnder", BooleanValue(useMesh));
        m_dev->SetAttribute("ForwardMesh", BooleanValue(forwardMesh));
        m_dev->SetReceiveCallback(MakeCallback(&MeshUnderDeviceTestCase::CaptureUp, this));
    }

    /**
     * @brief Build a mesh frame (MESH + BC0 + uncompressed IPv6 payload).
     *
     * @param originator The MESH originator address.
     * @param finalDst The MESH final destination address.
     * @param hopsLeft The MESH hops-left value.
     * @param seqNo The BC0 sequence number.
     * @return The assembled frame.
     */
    static Ptr<Packet> BuildMeshFrame(Mac16Address originator,
                                      Mac16Address finalDst,
                                      uint8_t hopsLeft,
                                      uint8_t seqNo)
    {
        Ptr<Packet> packet = Create<Packet>(40);

        SixLowPanIpv6 uncompressedHdr;
        packet->AddHeader(uncompressedHdr);

        SixLowPanBc0 bc0Hdr;
        bc0Hdr.SetSequenceNumber(seqNo);
        packet->AddHeader(bc0Hdr);

        SixLowPanMesh meshHdr;
        meshHdr.SetOriginator(originator);
        meshHdr.SetFinalDst(finalDst);
        meshHdr.SetHopsLeft(hopsLeft);
        packet->AddHeader(meshHdr);

        return packet;
    }

    /**
     * @brief Schedule the injection of a frame, as if received from the channel.
     *
     * The event is scheduled with the node's context, as a channel would.
     *
     * @param when The injection time.
     * @param packet The frame to inject.
     */
    void ScheduleInject(Time when, Ptr<Packet> packet)
    {
        Simulator::ScheduleWithContext(m_mock->GetNode()->GetId(),
                                       when,
                                       &MockNetDevice::Receive,
                                       m_mock,
                                       packet,
                                       uint16_t{iana::ieee802numbers::LoWPAN},
                                       m_mock->GetBroadcast(),
                                       Address(Mac48Address("00:00:00:00:00:02")),
                                       NetDevice::PACKET_BROADCAST);
    }

    /**
     * @brief Capture a frame the device re-broadcasts.
     * @param device The sending device.
     * @param packet The transmitted frame.
     * @param protocol The protocol number.
     * @param source The source address.
     * @param destination The destination address.
     * @param packetType The packet type.
     * @return true
     */
    bool CaptureSend(Ptr<NetDevice> device,
                     Ptr<const Packet> packet,
                     uint16_t protocol,
                     const Address& source,
                     const Address& destination,
                     NetDevice::PacketType packetType)
    {
        m_sentFrames.push_back(packet->Copy());
        return true;
    }

    /**
     * @brief Capture a locally delivered packet.
     * @param device The receiving device.
     * @param packet The delivered packet.
     * @param protocol The protocol number.
     * @param source The source address.
     * @return true
     */
    bool CaptureUp(Ptr<NetDevice> device,
                   Ptr<const Packet> packet,
                   uint16_t protocol,
                   const Address& source)
    {
        m_deliveredCount++;
        return true;
    }

    void DoTeardown() override
    {
        m_dev = nullptr;
        m_mock = nullptr;
        Simulator::Destroy();
    }

    Ptr<MockNetDevice> m_mock;             ///< Underlying (mock) device.
    Ptr<SixLowPanNetDevice> m_dev;         ///< Device under test.
    std::vector<Ptr<Packet>> m_sentFrames; ///< Frames re-broadcast by the device.
    uint32_t m_deliveredCount{0};          ///< Packets delivered locally.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief The device forwards a new mesh frame once and drops duplicates.
 */
class MeshUnderDeviceForwardTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceForwardTestCase()
        : MeshUnderDeviceTestCase("Device forwards new mesh frames once, with decremented hops")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice();

        Mac16Address originator("00:02");
        Mac16Address finalDst("00:03"); // Another node: to be forwarded, not delivered.

        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator, finalDst, /*hopsLeft=*/5, /*seqNo=*/1));
        // The same (originator, sequence) pair again: a duplicate.
        ScheduleInject(MilliSeconds(2),
                       BuildMeshFrame(originator, finalDst, /*hopsLeft=*/5, /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 1, "One forward for two copies (dedup)");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 0, "Frame for another node must not be delivered");

        if (!m_sentFrames.empty())
        {
            SixLowPanMesh meshHdr;
            SixLowPanBc0 bc0Hdr;
            m_sentFrames[0]->RemoveHeader(meshHdr);
            m_sentFrames[0]->RemoveHeader(bc0Hdr);
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetHopsLeft(), 4, "Hops left must be decremented");
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetOriginator(),
                                  Address(originator),
                                  "Originator preserved");
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetFinalDst(), Address(finalDst), "Final dst preserved");
            NS_TEST_ASSERT_MSG_EQ(bc0Hdr.GetSequenceNumber(), 1, "Sequence number preserved");
        }
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief A mesh frame destined to the node is delivered locally.
 *
 * A node in the mesh (UseMeshUnder enabled) must decode and deliver a mesh
 * frame addressed to it, and must not re-broadcast a unicast frame it
 * is the destination of.
 */
class MeshUnderDeviceDeliveryTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceDeliveryTestCase()
        : MeshUnderDeviceTestCase("Mesh frame for this node is delivered")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice();

        Mac16Address originator("00:02");
        Mac16Address finalDst("00:01"); // This node.

        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator, finalDst, /*hopsLeft=*/5, /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 1, "Frame for this node must be delivered");
        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 0, "Unicast frame for this node: no forward");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Hop limit and broadcast handling on the receive path.
 */
class MeshUnderDeviceHopLimitTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceHopLimitTestCase()
        : MeshUnderDeviceTestCase("Hop limit is honored; broadcast is forwarded and delivered")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice();

        Mac16Address originator("00:02");

        // Exhausted hop count: must not be forwarded.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator,
                                      Mac16Address("00:03"),
                                      /*hopsLeft=*/0,
                                      /*seqNo=*/1));
        // Broadcast: must be forwarded and delivered.
        ScheduleInject(MilliSeconds(2),
                       BuildMeshFrame(originator,
                                      Mac16Address("ff:ff"),
                                      /*hopsLeft=*/3,
                                      /*seqNo=*/2));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 1, "Only the broadcast frame is forwarded");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 1, "The broadcast frame is also delivered");

        if (!m_sentFrames.empty())
        {
            SixLowPanMesh meshHdr;
            m_sentFrames[0]->RemoveHeader(meshHdr);
            NS_TEST_ASSERT_MSG_EQ(meshHdr.GetHopsLeft(), 2, "Hops left must be decremented");
        }
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief A node outside the mesh receiving a mesh frame reports and drops it.
 *
 * With UseMeshUnder disabled, a received mesh-under frame is a network
 * misconfiguration: it must be dropped (DROP_MESH_NOT_ENABLED), not
 * delivered and not forwarded.
 */
class MeshUnderDeviceMisconfigTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceMisconfigTestCase()
        : MeshUnderDeviceTestCase("Mesh frame without UseMeshUnder is reported and dropped")
    {
    }

  private:
    /**
     * @brief Capture drop-trace invocations.
     * @param reason The drop reason.
     * @param packet The dropped packet.
     * @param device The dropping device.
     * @param ifindex The interface index.
     */
    void CaptureDrop(SixLowPanNetDevice::DropReason reason,
                     Ptr<const Packet> packet,
                     Ptr<SixLowPanNetDevice> device,
                     uint32_t ifindex)
    {
        m_dropCount++;
        m_lastDropReason = reason;
    }

    void DoRun() override
    {
        SetupDevice(/*useMesh=*/false);
        m_dev->TraceConnectWithoutContext(
            "Drop",
            MakeCallback(&MeshUnderDeviceMisconfigTestCase::CaptureDrop, this));

        // A mesh frame addressed to this very node: it must still be dropped.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(Mac16Address("00:02"),
                                      Mac16Address("00:01"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount,
                              0,
                              "Frame must not be delivered without UseMeshUnder");
        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(),
                              0,
                              "Frame must not be forwarded without UseMeshUnder");
        NS_TEST_ASSERT_MSG_EQ(m_dropCount, 1, "The drop must be reported on the Drop trace");
        NS_TEST_ASSERT_MSG_EQ(m_lastDropReason,
                              SixLowPanNetDevice::DROP_MESH_NOT_ENABLED,
                              "The drop reason must be DROP_MESH_NOT_ENABLED");
    }

    uint32_t m_dropCount{0}; ///< Number of drop-trace invocations.
    SixLowPanNetDevice::DropReason m_lastDropReason{
        SixLowPanNetDevice::DROP_FRAGMENT_TIMEOUT}; ///< Last reported drop reason.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief A node in the mesh with ForwardMesh disabled receives but does not relay.
 */
class MeshUnderDeviceNoRelayTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceNoRelayTestCase()
        : MeshUnderDeviceTestCase("Node with ForwardMesh disabled delivers but does not relay")
    {
    }

  private:
    void DoRun() override
    {
        SetupDevice(/*useMesh=*/true, /*forwardMesh=*/false);

        Mac16Address originator("00:02");

        // A frame for another node: without ForwardMesh it goes nowhere.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(originator,
                                      Mac16Address("00:03"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/1));
        // A broadcast frame: delivered locally, but still not relayed.
        ScheduleInject(MilliSeconds(2),
                       BuildMeshFrame(originator,
                                      Mac16Address("ff:ff"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/2));

        Simulator::Stop(MilliSeconds(50));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 0, "No relaying with ForwardMesh disabled");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 1, "The broadcast frame is still delivered");
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief Three-node chain: the middle node decides whether the far node is reached.
 *
 * A frame from node A (00:02) to node C (00:03) arrives at the middle
 * node B (00:01) only; whatever B transmits reaches C. When B is part
 * of the mesh and forwards, C receives the packet. When B is outside
 * the mesh, C receives nothing.
 */
class MeshUnderChainRelayTestCase : public MeshUnderDeviceTestCase
{
  public:
    /**
     * @brief Constructor.
     * @param middleInMesh Whether the middle node takes part in the mesh.
     */
    MeshUnderChainRelayTestCase(bool middleInMesh)
        : MeshUnderDeviceTestCase(middleInMesh
                                      ? "3-node chain: middle node relays to the far node"
                                      : "3-node chain: middle node outside the mesh, no relay"),
          m_middleInMesh(middleInMesh)
    {
    }

  private:
    /**
     * @brief Deliver whatever the middle node transmits to the far node,
     *        as a channel would.
     * @param device The sending device.
     * @param packet The transmitted frame.
     * @param protocol The protocol number.
     * @param source The source address.
     * @param destination The destination address.
     * @param packetType The packet type.
     * @return true
     */
    bool RelayToFarNode(Ptr<NetDevice> device,
                        Ptr<const Packet> packet,
                        uint16_t protocol,
                        const Address& source,
                        const Address& destination,
                        NetDevice::PacketType packetType)
    {
        Simulator::ScheduleWithContext(m_mockFar->GetNode()->GetId(),
                                       MicroSeconds(100),
                                       &MockNetDevice::Receive,
                                       m_mockFar,
                                       packet->Copy(),
                                       protocol,
                                       m_mockFar->GetBroadcast(),
                                       source,
                                       NetDevice::PACKET_BROADCAST);
        return true;
    }

    /**
     * @brief Count packets delivered on the far node.
     * @param device The receiving device.
     * @param packet The delivered packet.
     * @param protocol The protocol number.
     * @param source The source address.
     * @return true
     */
    bool CaptureUpFar(Ptr<NetDevice> device,
                      Ptr<const Packet> packet,
                      uint16_t protocol,
                      const Address& source)
    {
        m_deliveredFar++;
        return true;
    }

    void DoRun() override
    {
        // Middle node (B, 00:01): the base fixture device.
        SetupDevice(/*useMesh=*/m_middleInMesh, /*forwardMesh=*/true);
        m_mock->SetSendCallback(MakeCallback(&MeshUnderChainRelayTestCase::RelayToFarNode, this));

        // Far node (C, 00:03).
        Ptr<Node> nodeFar = CreateObject<Node>();
        m_mockFar = CreateObject<MockNetDevice>();
        nodeFar->AddDevice(m_mockFar);
        m_mockFar->SetNode(nodeFar);
        m_mockFar->SetAddress(Mac48Address("00:00:00:00:00:03"));
        m_mockFar->SetMtu(150);
        m_devFar = CreateObject<SixLowPanNetDevice>();
        nodeFar->AddDevice(m_devFar);
        m_devFar->SetNetDevice(m_mockFar);
        m_devFar->SetAttribute("UseMeshUnder", BooleanValue(true));
        m_devFar->SetReceiveCallback(
            MakeCallback(&MeshUnderChainRelayTestCase::CaptureUpFar, this));

        // A frame from A (00:02) to C (00:03) arrives at B only.
        ScheduleInject(MilliSeconds(1),
                       BuildMeshFrame(Mac16Address("00:02"),
                                      Mac16Address("00:03"),
                                      /*hopsLeft=*/5,
                                      /*seqNo=*/1));

        Simulator::Stop(MilliSeconds(100));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_deliveredFar,
                              (m_middleInMesh ? 1 : 0),
                              "Far node delivery depends on the middle node relaying");
        NS_TEST_ASSERT_MSG_EQ(m_deliveredCount, 0, "The frame is not for the middle node");
    }

    void DoTeardown() override
    {
        m_devFar = nullptr;
        m_mockFar = nullptr;
        MeshUnderDeviceTestCase::DoTeardown();
    }

    bool m_middleInMesh;              ///< Whether the middle node takes part in the mesh.
    Ptr<MockNetDevice> m_mockFar;     ///< Far node underlying (mock) device.
    Ptr<SixLowPanNetDevice> m_devFar; ///< Far node device under test.
    uint32_t m_deliveredFar{0};       ///< Packets delivered on the far node.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief The device default policy and stream assignment match the
 *        historical behavior.
 */
class MeshUnderDeviceStreamsTestCase : public TestCase
{
  public:
    MeshUnderDeviceStreamsTestCase()
        : TestCase("Default policy exists at construction; AssignStreams returns 2")
    {
    }

  private:
    void DoRun() override
    {
        Ptr<SixLowPanNetDevice> dev = CreateObject<SixLowPanNetDevice>();

        // The MeshUnderRouting attribute default provides a policy at
        // construction, so stream assignment is effective during setup.
        Ptr<SixLowPanMeshUnderRouting> policy = dev->GetMeshUnderRouting();
        NS_TEST_ASSERT_MSG_NE(policy, nullptr, "Default policy must exist at construction");
        NS_TEST_ASSERT_MSG_NE(DynamicCast<SixLowPanSimpleFlooding>(policy),
                              nullptr,
                              "Default policy must be SixLowPanSimpleFlooding");

        // The historical implementation assigned two streams (device RNG and
        // forwarding jitter).
        NS_TEST_ASSERT_MSG_EQ(dev->AssignStreams(7), 2, "Two streams, as before the refactoring");
    }

    void DoTeardown() override
    {
        Simulator::Destroy();
    }
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief The duplicate count reaches the Trickle policy through the device.
 *
 * A frame is injected twice (the second reception is a duplicate) followed
 * by a second, distinct frame. With DuplicateThreshold = 2 and k = 0 the
 * covered frame must be killed at the firing and only the fresh frame
 * re-broadcast, proving the device wires the duplicate detections into the
 * policy's per-packet counts.
 */
class MeshUnderDeviceDuplicateCountTestCase : public MeshUnderDeviceTestCase
{
  public:
    MeshUnderDeviceDuplicateCountTestCase()
        : MeshUnderDeviceTestCase("Device-level duplicate count kills the covered frame")
    {
    }

  private:
    /**
     * @brief Suppressed-trace callback.
     * @param packet The suppressed packet (unused).
     */
    void RecordSuppressed(Ptr<const Packet> packet [[maybe_unused]])
    {
        m_suppressedCount++;
    }

    void DoRun() override
    {
        SetupDevice();

        Ptr<SixLowPanAdaptiveFlooding> trickle = CreateObject<SixLowPanAdaptiveFlooding>();
        trickle->SetAttribute("MinInterval", TimeValue(MilliSeconds(10)));
        trickle->SetAttribute("RedundancyConstant", UintegerValue(0));
        trickle->SetAttribute("DuplicateThreshold", UintegerValue(2));
        trickle->TraceConnectWithoutContext(
            "PacketSuppressed",
            MakeCallback(&MeshUnderDeviceDuplicateCountTestCase::RecordSuppressed, this));
        m_dev->SetAttribute("MeshUnderRouting", PointerValue(trickle));
        m_dev->AssignStreams(1);

        Mac16Address orig("00:02");
        Mac16Address dst("00:42"); // neither this node nor broadcast

        // The same frame twice (the second reception is a duplicate), then a
        // fresh frame; all before the first firing, no earlier than 6 ms.
        ScheduleInject(MilliSeconds(1), BuildMeshFrame(orig, dst, 5, /*seqNo=*/1));
        ScheduleInject(MilliSeconds(2), BuildMeshFrame(orig, dst, 5, /*seqNo=*/1));
        ScheduleInject(MilliSeconds(3), BuildMeshFrame(orig, dst, 5, /*seqNo=*/2));

        Simulator::Stop(MilliSeconds(500));
        Simulator::Run();

        NS_TEST_ASSERT_MSG_EQ(m_suppressedCount, 1, "The covered frame is killed at the firing");
        NS_TEST_ASSERT_MSG_EQ(m_sentFrames.size(), 1, "Only the fresh frame is re-broadcast");

        SixLowPanMesh meshHdr;
        SixLowPanBc0 bc0Hdr;
        Ptr<Packet> sent = m_sentFrames[0]->Copy();
        sent->RemoveHeader(meshHdr);
        sent->RemoveHeader(bc0Hdr);
        NS_TEST_ASSERT_MSG_EQ(+bc0Hdr.GetSequenceNumber(), 2, "The re-broadcast frame is seq 2");
    }

    uint32_t m_suppressedCount{0}; ///< PacketSuppressed trace invocations.
};

/**
 * @ingroup sixlowpan-tests
 *
 * @brief 6LoWPAN mesh-under forwarding test suite.
 */
class SixLowPanMeshUnderTestSuite : public TestSuite
{
  public:
    SixLowPanMeshUnderTestSuite()
        : TestSuite("sixlowpan-mesh-under", Type::UNIT)
    {
        AddTestCase(new DuplicateCacheFifoDropTestCase, Duration::QUICK);
        AddTestCase(new DuplicateCachePerOriginatorTestCase, Duration::QUICK);
        AddTestCase(new SimpleFloodingForwardsTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingForwardsWhenQuietTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingSuppressesWhileCoveredTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingZeroRedundancyAlwaysForwardsTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingOnePerFiringTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingAnyDuplicateSuppressesTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingRestartAfterDrainTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingDuplicateThresholdKillsHeadTestCase, Duration::QUICK);
        AddTestCase(new AdaptiveFloodingPerPacketDeadlineTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceForwardTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceDeliveryTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceHopLimitTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceMisconfigTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceNoRelayTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderChainRelayTestCase(true), Duration::QUICK);
        AddTestCase(new MeshUnderChainRelayTestCase(false), Duration::QUICK);
        AddTestCase(new MeshUnderDeviceStreamsTestCase, Duration::QUICK);
        AddTestCase(new MeshUnderDeviceDuplicateCountTestCase, Duration::QUICK);
    }
};

/// Static suite registration.
static SixLowPanMeshUnderTestSuite g_sixLowPanMeshUnderTestSuite;
