/*
 * Copyright (c) 2026 SRM Institute of Science and Technology, India
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Usham Roy <ushamroy80@gmail.com>
 */

#include "sixlowpan-adaptive-flooding.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SixLowPanAdaptiveFlooding");
NS_OBJECT_ENSURE_REGISTERED(SixLowPanAdaptiveFlooding);

TypeId
SixLowPanAdaptiveFlooding::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::SixLowPanAdaptiveFlooding")
            .SetParent<SixLowPanMeshUnderRouting>()
            .SetGroupName("SixLowPan")
            .AddConstructor<SixLowPanAdaptiveFlooding>()
            .AddAttribute("MinInterval",
                          "RFC 6206 Imin: the minimum Trickle interval.",
                          TimeValue(MilliSeconds(10)),
                          MakeTimeAccessor(&SixLowPanAdaptiveFlooding::m_minInterval),
                          MakeTimeChecker(MilliSeconds(1)))
            .AddAttribute("Doublings",
                          "Number of interval doublings; Imax = MinInterval * 2^Doublings.",
                          UintegerValue(4),
                          MakeUintegerAccessor(&SixLowPanAdaptiveFlooding::m_doublings),
                          MakeUintegerChecker<uint8_t>(0, 16))
            .AddAttribute("RedundancyConstant",
                          "RFC 6206 k: forward only if fewer than k copies were heard. "
                          "Zero disables suppression.",
                          UintegerValue(1),
                          MakeUintegerAccessor(&SixLowPanAdaptiveFlooding::m_redundancy),
                          MakeUintegerChecker<uint16_t>())
            .AddAttribute("MaxForwardingDelay",
                          "Per-packet deadline: discard a pending packet if it is not "
                          "forwarded within this time of its arrival (suppression won).",
                          TimeValue(MilliSeconds(500)),
                          MakeTimeAccessor(&SixLowPanAdaptiveFlooding::m_maxForwardingDelay),
                          MakeTimeChecker(MilliSeconds(1)))
            .AddAttribute("DuplicateThreshold",
                          "Discard a pending packet at transmit time once it has been "
                          "received this many times in total (the reception that queued "
                          "it plus the overheard duplicates). Zero disables the check.",
                          UintegerValue(2),
                          MakeUintegerAccessor(&SixLowPanAdaptiveFlooding::m_duplicateThreshold),
                          MakeUintegerChecker<uint16_t>())
            .AddTraceSource("PendingQueueSize",
                            "Number of packets in the pending queue.",
                            MakeTraceSourceAccessor(&SixLowPanAdaptiveFlooding::m_pendingSize),
                            "ns3::TracedValueCallback::Uint32")
            .AddTraceSource("PacketDiscarded",
                            "A pending packet was discarded at its deadline "
                            "without being forwarded.",
                            MakeTraceSourceAccessor(&SixLowPanAdaptiveFlooding::m_discardTrace),
                            "ns3::Packet::TracedCallback")
            .AddTraceSource("PacketSuppressed",
                            "A pending packet was discarded at transmit time because it "
                            "was already received DuplicateThreshold times.",
                            MakeTraceSourceAccessor(&SixLowPanAdaptiveFlooding::m_suppressedTrace),
                            "ns3::Packet::TracedCallback");
    return tid;
}

SixLowPanAdaptiveFlooding::SixLowPanAdaptiveFlooding()
{
    NS_LOG_FUNCTION(this);
    m_timer.SetFunction(&SixLowPanAdaptiveFlooding::Transmit, this);
}

SixLowPanAdaptiveFlooding::~SixLowPanAdaptiveFlooding()
{
    NS_LOG_FUNCTION(this);
}

void
SixLowPanAdaptiveFlooding::DoDispose()
{
    NS_LOG_FUNCTION(this);
    StopTimer();
    m_pending.clear();
    m_pendingSize = 0;
    SixLowPanMeshUnderRouting::DoDispose();
}

void
SixLowPanAdaptiveFlooding::OnPacketForward(Ptr<Packet> packet,
                                           const Address& originator,
                                           uint8_t seqNo,
                                           uint8_t hopsLeft,
                                           ForwardCallback forwardCb)
{
    NS_LOG_FUNCTION(this << packet << originator << +seqNo << +hopsLeft);

    // A new packet joins the pending queue; it does not restart a running
    // timer, so an earlier pending packet is never starved.
    m_pending.push_back(
        {packet, forwardCb, originator, seqNo, Simulator::Now() + m_maxForwardingDelay, 1});
    m_pendingSize = m_pending.size();

    if (!m_timer.IsRunning())
    {
        StartTimer();
    }
}

void
SixLowPanAdaptiveFlooding::OnDuplicateReceived(const Address& originator, uint8_t seqNo)
{
    NS_LOG_FUNCTION(this << originator << +seqNo);

    // Consistent event (Rule 3): a neighbour is spreading information we
    // have also seen. Count it; sustained consistency grows the interval.
    if (!m_timer.IsRunning())
    {
        return;
    }
    // Track how many times each pending packet has been received: the
    // count feeds the DuplicateThreshold check at transmit time.
    for (auto& entry : m_pending)
    {
        if (entry.originator == originator && entry.seqNo == seqNo)
        {
            entry.seenCount++;
            break;
        }
    }
    m_timer.ConsistentEvent();
}

void
SixLowPanAdaptiveFlooding::StartTimer()
{
    NS_LOG_FUNCTION(this);

    m_timer.SetParameters(m_minInterval, m_doublings, m_redundancy);
    m_timer.Enable();
    // Enable() picks the first interval in [Imin, Imax] (RFC 6206 section
    // 4.2 step 1); the packet that starts the timer is new information, so
    // restart from Imin to react quickly and let consistency back it off.
    m_timer.Reset();
    ScheduleDiscard();
}

void
SixLowPanAdaptiveFlooding::StopTimer()
{
    NS_LOG_FUNCTION(this);

    m_timer.Stop();
    m_discardEvent.Cancel();
}

bool
SixLowPanAdaptiveFlooding::ReachedDuplicateThreshold(const PendingPacket& entry) const
{
    return m_duplicateThreshold > 0 && entry.seenCount >= m_duplicateThreshold;
}

void
SixLowPanAdaptiveFlooding::Transmit()
{
    NS_LOG_FUNCTION(this);

    // Reached only when c < k (the TrickleTimer enforces this).
    // Count-based suppression, checked right before transmission: a
    // packet already received DuplicateThreshold times is covered by
    // the neighbourhood, so kill it and try the next one.
    while (!m_pending.empty() && ReachedDuplicateThreshold(m_pending.front()))
    {
        NS_LOG_LOGIC("Suppressing a packet received " << m_pending.front().seenCount << " times");
        m_suppressedTrace(m_pending.front().packet);
        m_pending.pop_front();
    }
    m_pendingSize = m_pending.size();
    if (m_pending.empty())
    {
        StopTimer();
        return;
    }

    NS_LOG_LOGIC("Forwarding the head of " << m_pending.size() << " pending packet(s)");
    PendingPacket head = m_pending.front();
    m_pending.pop_front();
    m_pendingSize = m_pending.size();
    head.forwardCb(head.packet);

    m_discardEvent.Cancel();
    if (m_pending.empty())
    {
        StopTimer();
    }
    else
    {
        ScheduleDiscard();
    }
}

void
SixLowPanAdaptiveFlooding::DiscardExpired()
{
    NS_LOG_FUNCTION(this);

    // Suppression kept the head of the queue silent past its deadline;
    // give up on the expired packets only, the rest keep their own deadlines.
    while (!m_pending.empty() && m_pending.front().deadline <= Simulator::Now())
    {
        NS_LOG_LOGIC("Discarding a suppressed packet past its deadline");
        m_discardTrace(m_pending.front().packet);
        m_pending.pop_front();
    }
    m_pendingSize = m_pending.size();

    if (m_pending.empty())
    {
        StopTimer();
    }
    else
    {
        ScheduleDiscard();
    }
}

void
SixLowPanAdaptiveFlooding::ScheduleDiscard()
{
    NS_LOG_FUNCTION(this);

    // FIFO queue + constant per-packet delay: deadlines are monotonic, so
    // one event for the head of the queue is enough. The delay is clamped
    // to zero because reducing MaxForwardingDelay at runtime can leave the
    // new head with a deadline that already passed.
    NS_ASSERT_MSG(!m_pending.empty(), "No pending packet to schedule a discard for");
    Time delay = Max(m_pending.front().deadline - Simulator::Now(), Time(0));
    m_discardEvent = Simulator::Schedule(delay, &SixLowPanAdaptiveFlooding::DiscardExpired, this);
}

int64_t
SixLowPanAdaptiveFlooding::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    return m_timer.AssignStreams(stream);
}

} // namespace ns3
