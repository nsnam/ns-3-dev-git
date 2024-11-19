//
// Copyright (c) 2009 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
//

#include "flow-monitor.h"

#include "ns3/double.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <fstream>
#include <limits>
#include <sstream>

#define PERIODIC_CHECK_INTERVAL (Seconds(1))

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FlowMonitor");

NS_OBJECT_ENSURE_REGISTERED(FlowMonitor);

TypeId
FlowMonitor::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::FlowMonitor")
            .SetParent<Object>()
            .SetGroupName("FlowMonitor")
            .AddConstructor<FlowMonitor>()
            .AddAttribute(
                "MaxPerHopDelay",
                ("The maximum per-hop delay that should be considered.  "
                 "Packets still not received after this delay are to be considered lost."),
                TimeValue(Seconds(10)),
                MakeTimeAccessor(&FlowMonitor::m_maxPerHopDelay),
                MakeTimeChecker())
            .AddAttribute("StartTime",
                          ("The time when the monitoring starts."),
                          TimeValue(Seconds(0)),
                          MakeTimeAccessor(&FlowMonitor::Start),
                          MakeTimeChecker())
            .AddAttribute("DelayBinWidth",
                          ("The width used in the delay histogram."),
                          DoubleValue(0.001),
                          MakeDoubleAccessor(&FlowMonitor::m_delayBinWidth),
                          MakeDoubleChecker<double>())
            .AddAttribute("JitterBinWidth",
                          ("The width used in the jitter histogram."),
                          DoubleValue(0.001),
                          MakeDoubleAccessor(&FlowMonitor::m_jitterBinWidth),
                          MakeDoubleChecker<double>())
            .AddAttribute("PacketSizeBinWidth",
                          ("The width used in the packetSize histogram."),
                          DoubleValue(20),
                          MakeDoubleAccessor(&FlowMonitor::m_packetSizeBinWidth),
                          MakeDoubleChecker<double>())
            .AddAttribute("FlowInterruptionsBinWidth",
                          ("The width used in the flowInterruptions histogram."),
                          DoubleValue(0.250),
                          MakeDoubleAccessor(&FlowMonitor::m_flowInterruptionsBinWidth),
                          MakeDoubleChecker<double>())
            .AddAttribute(
                "FlowInterruptionsMinTime",
                ("The minimum inter-arrival time that is considered a flow interruption."),
                TimeValue(Seconds(0.5)),
                MakeTimeAccessor(&FlowMonitor::m_flowInterruptionsMinTime),
                MakeTimeChecker());
    return tid;
}

TypeId
FlowMonitor::GetInstanceTypeId() const
{
    return GetTypeId();
}

FlowMonitor::FlowMonitor()
    : m_enabled(false)
{
    NS_LOG_FUNCTION(this);
}

void
FlowMonitor::DoDispose()
{
    NS_LOG_FUNCTION(this);
    Simulator::Cancel(m_startEvent);
    Simulator::Cancel(m_stopEvent);
    for (auto iter = m_classifiers.begin(); iter != m_classifiers.end(); iter++)
    {
        *iter = nullptr;
    }
    for (uint32_t i = 0; i < m_flowProbes.size(); i++)
    {
        m_flowProbes[i]->Dispose();
        m_flowProbes[i] = nullptr;
    }
    Object::DoDispose();
}

inline FlowMonitor::FlowStats&
FlowMonitor::GetStatsForFlow(FlowId flowId)
{
    NS_LOG_FUNCTION(this);
    auto iter = m_flowStats.find(flowId);
    if (iter == m_flowStats.end())
    {
        FlowMonitor::FlowStats& ref = m_flowStats[flowId];
        ref.delaySum = Seconds(0);
        ref.jitterSum = Seconds(0);
        ref.lastDelay = Seconds(0);
        ref.maxDelay = Seconds(0);
        ref.minDelay = Time::Max();
        ref.txBytes = 0;
        ref.rxBytes = 0;
        ref.txPackets = 0;
        ref.rxPackets = 0;
        ref.lostPackets = 0;
        ref.timesForwarded = 0;
        ref.delayHistogram.SetDefaultBinWidth(m_delayBinWidth);
        ref.jitterHistogram.SetDefaultBinWidth(m_jitterBinWidth);
        ref.packetSizeHistogram.SetDefaultBinWidth(m_packetSizeBinWidth);
        ref.flowInterruptionsHistogram.SetDefaultBinWidth(m_flowInterruptionsBinWidth);
        return ref;
    }
    else
    {
        return iter->second;
    }
}

void
FlowMonitor::ReportFirstTx(Ptr<FlowProbe> probe,
                           uint32_t flowId,
                           uint32_t packetId,
                           uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << probe << flowId << packetId << packetSize);
    if (!m_enabled)
    {
        NS_LOG_DEBUG("FlowMonitor not enabled; returning");
        return;
    }
    Time now = Simulator::Now();
    TrackedPacket& tracked = m_trackedPackets[std::make_pair(flowId, packetId)];
    tracked.firstSeenTime = now;
    tracked.lastSeenTime = tracked.firstSeenTime;
    tracked.timesForwarded = 0;
    NS_LOG_DEBUG("ReportFirstTx: adding tracked packet (flowId=" << flowId << ", packetId="
                                                                 << packetId << ").");

    probe->AddPacketStats(flowId, packetSize, Seconds(0));

    FlowStats& stats = GetStatsForFlow(flowId);
    stats.txBytes += packetSize;
    stats.txPackets++;
    if (stats.txPackets == 1)
    {
        stats.timeFirstTxPacket = now;
    }
    stats.timeLastTxPacket = now;
}

void
FlowMonitor::ReportForwarding(Ptr<FlowProbe> probe,
                              uint32_t flowId,
                              uint32_t packetId,
                              uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << probe << flowId << packetId << packetSize);
    if (!m_enabled)
    {
        NS_LOG_DEBUG("FlowMonitor not enabled; returning");
        return;
    }
    std::pair<FlowId, FlowPacketId> key(flowId, packetId);
    auto tracked = m_trackedPackets.find(key);
    if (tracked == m_trackedPackets.end())
    {
        NS_LOG_WARN("Received packet forward report (flowId="
                    << flowId << ", packetId=" << packetId << ") but not known to be transmitted.");
        return;
    }

    tracked->second.timesForwarded++;
    tracked->second.lastSeenTime = Simulator::Now();

    Time delay = (Simulator::Now() - tracked->second.firstSeenTime);
    probe->AddPacketStats(flowId, packetSize, delay);
}

void
FlowMonitor::ReportLastRx(Ptr<FlowProbe> probe,
                          uint32_t flowId,
                          uint32_t packetId,
                          uint32_t packetSize)
{
    NS_LOG_FUNCTION(this << probe << flowId << packetId << packetSize);
    if (!m_enabled)
    {
        NS_LOG_DEBUG("FlowMonitor not enabled; returning");
        return;
    }
    auto tracked = m_trackedPackets.find(std::make_pair(flowId, packetId));
    if (tracked == m_trackedPackets.end())
    {
        NS_LOG_WARN("Received packet last-tx report (flowId="
                    << flowId << ", packetId=" << packetId << ") but not known to be transmitted.");
        return;
    }

    Time now = Simulator::Now();
    Time delay = (now - tracked->second.firstSeenTime);
    probe->AddPacketStats(flowId, packetSize, delay);

    FlowStats& stats = GetStatsForFlow(flowId);
    stats.delaySum += delay;
    stats.delayHistogram.AddValue(delay.GetSeconds());
    if (stats.rxPackets > 0)
    {
        Time jitter = stats.lastDelay - delay;
        if (jitter.IsStrictlyPositive())
        {
            stats.jitterSum += jitter;
            stats.jitterHistogram.AddValue(jitter.GetSeconds());
        }
        else
        {
            stats.jitterSum -= jitter;
            stats.jitterHistogram.AddValue(-jitter.GetSeconds());
        }
    }
    stats.lastDelay = delay;
    if (delay > stats.maxDelay)
    {
        stats.maxDelay = delay;
    }
    if (delay < stats.minDelay)
    {
        stats.minDelay = delay;
    }

    stats.rxBytes += packetSize;
    stats.packetSizeHistogram.AddValue((double)packetSize);
    stats.rxPackets++;
    if (stats.rxPackets == 1)
    {
        stats.timeFirstRxPacket = now;
    }
    else
    {
        // measure possible flow interruptions
        Time interArrivalTime = now - stats.timeLastRxPacket;
        if (interArrivalTime > m_flowInterruptionsMinTime)
        {
            stats.flowInterruptionsHistogram.AddValue(interArrivalTime.GetSeconds());
        }
    }
    stats.timeLastRxPacket = now;
    stats.timesForwarded += tracked->second.timesForwarded;

    NS_LOG_DEBUG("ReportLastTx: removing tracked packet (flowId=" << flowId << ", packetId="
                                                                  << packetId << ").");

    m_trackedPackets.erase(tracked); // we don't need to track this packet anymore
}

void
FlowMonitor::ReportDrop(Ptr<FlowProbe> probe,
                        uint32_t flowId,
                        uint32_t packetId,
                        uint32_t packetSize,
                        uint32_t reasonCode)
{
    NS_LOG_FUNCTION(this << probe << flowId << packetId << packetSize << reasonCode);
    if (!m_enabled)
    {
        NS_LOG_DEBUG("FlowMonitor not enabled; returning");
        return;
    }

    probe->AddPacketDropStats(flowId, packetSize, reasonCode);

    FlowStats& stats = GetStatsForFlow(flowId);
    stats.lostPackets++;
    if (stats.packetsDropped.size() < reasonCode + 1)
    {
        stats.packetsDropped.resize(reasonCode + 1, 0);
        stats.bytesDropped.resize(reasonCode + 1, 0);
    }
    ++stats.packetsDropped[reasonCode];
    stats.bytesDropped[reasonCode] += packetSize;
    NS_LOG_DEBUG("++stats.packetsDropped["
                 << reasonCode << "]; // becomes: " << stats.packetsDropped[reasonCode]);

    auto tracked = m_trackedPackets.find(std::make_pair(flowId, packetId));
    if (tracked != m_trackedPackets.end())
    {
        // we don't need to track this packet anymore
        // FIXME: this will not necessarily be true with broadcast/multicast
        NS_LOG_DEBUG("ReportDrop: removing tracked packet (flowId=" << flowId << ", packetId="
                                                                    << packetId << ").");
        m_trackedPackets.erase(tracked);
    }
}

const FlowMonitor::FlowStatsContainer&
FlowMonitor::GetFlowStats() const
{
    return m_flowStats;
}

void
FlowMonitor::CheckForLostPackets(Time maxDelay)
{
    NS_LOG_FUNCTION(this << maxDelay.As(Time::S));
    Time now = Simulator::Now();

    for (auto iter = m_trackedPackets.begin(); iter != m_trackedPackets.end();)
    {
        if (now - iter->second.lastSeenTime >= maxDelay)
        {
            // packet is considered lost, add it to the loss statistics
            auto flow = m_flowStats.find(iter->first.first);
            NS_ASSERT(flow != m_flowStats.end());
            flow->second.lostPackets++;

            // we won't track it anymore
            m_trackedPackets.erase(iter++);
        }
        else
        {
            iter++;
        }
    }
}

void
FlowMonitor::CheckForLostPackets()
{
    CheckForLostPackets(m_maxPerHopDelay);
}

void
FlowMonitor::PeriodicCheckForLostPackets()
{
    CheckForLostPackets();
    Simulator::Schedule(PERIODIC_CHECK_INTERVAL, &FlowMonitor::PeriodicCheckForLostPackets, this);
}

void
FlowMonitor::NotifyConstructionCompleted()
{
    Object::NotifyConstructionCompleted();
    Simulator::Schedule(PERIODIC_CHECK_INTERVAL, &FlowMonitor::PeriodicCheckForLostPackets, this);
}

void
FlowMonitor::AddProbe(Ptr<FlowProbe> probe)
{
    m_flowProbes.push_back(probe);
}

const FlowMonitor::FlowProbeContainer&
FlowMonitor::GetAllProbes() const
{
    return m_flowProbes;
}

void
FlowMonitor::Start(const Time& time)
{
    NS_LOG_FUNCTION(this << time.As(Time::S));
    if (m_enabled)
    {
        NS_LOG_DEBUG("FlowMonitor already enabled; returning");
        return;
    }
    Simulator::Cancel(m_startEvent);
    NS_LOG_DEBUG("Scheduling start at " << time.As(Time::S));
    m_startEvent = Simulator::Schedule(time, &FlowMonitor::StartRightNow, this);
}

void
FlowMonitor::Stop(const Time& time)
{
    NS_LOG_FUNCTION(this << time.As(Time::S));
    Simulator::Cancel(m_stopEvent);
    NS_LOG_DEBUG("Scheduling stop at " << time.As(Time::S));
    m_stopEvent = Simulator::Schedule(time, &FlowMonitor::StopRightNow, this);
}

void
FlowMonitor::StartRightNow()
{
    NS_LOG_FUNCTION(this);
    if (m_enabled)
    {
        NS_LOG_DEBUG("FlowMonitor already enabled; returning");
        return;
    }
    m_enabled = true;
}

void
FlowMonitor::StopRightNow()
{
    NS_LOG_FUNCTION(this);
    if (!m_enabled)
    {
        NS_LOG_DEBUG("FlowMonitor not enabled; returning");
        return;
    }
    m_enabled = false;
    CheckForLostPackets();
}

void
FlowMonitor::AddFlowClassifier(Ptr<FlowClassifier> classifier)
{
    m_classifiers.push_back(classifier);
}

void
FlowMonitor::SerializeToXmlStream(std::ostream& os,
                                  uint16_t indent,
                                  bool enableHistograms,
                                  bool enableProbes)
{
    NS_LOG_FUNCTION(this << indent << enableHistograms << enableProbes);
    CheckForLostPackets();

    os << std::string(indent, ' ') << "<FlowMonitor>\n";
    indent += 2;
    os << std::string(indent, ' ') << "<FlowStats>\n";
    indent += 2;
    for (auto flowI = m_flowStats.begin(); flowI != m_flowStats.end(); flowI++)
    {
        os << std::string(indent, ' ');
#define ATTRIB(name) " " #name "=\"" << flowI->second.name << "\""
#define ATTRIB_TIME(name) " " #name "=\"" << flowI->second.name.As(Time::NS) << "\""
        os << "<Flow flowId=\"" << flowI->first << "\"" << ATTRIB_TIME(timeFirstTxPacket)
           << ATTRIB_TIME(timeFirstRxPacket) << ATTRIB_TIME(timeLastTxPacket)
           << ATTRIB_TIME(timeLastRxPacket) << ATTRIB_TIME(delaySum) << ATTRIB_TIME(jitterSum)
           << ATTRIB_TIME(lastDelay) << ATTRIB_TIME(maxDelay) << ATTRIB_TIME(minDelay)
           << ATTRIB(txBytes) << ATTRIB(rxBytes) << ATTRIB(txPackets) << ATTRIB(rxPackets)
           << ATTRIB(lostPackets) << ATTRIB(timesForwarded) << ">\n";
#undef ATTRIB_TIME
#undef ATTRIB

        indent += 2;
        for (uint32_t reasonCode = 0; reasonCode < flowI->second.packetsDropped.size();
             reasonCode++)
        {
            os << std::string(indent, ' ');
            os << "<packetsDropped reasonCode=\"" << reasonCode << "\""
               << " number=\"" << flowI->second.packetsDropped[reasonCode] << "\" />\n";
        }
        for (uint32_t reasonCode = 0; reasonCode < flowI->second.bytesDropped.size(); reasonCode++)
        {
            os << std::string(indent, ' ');
            os << "<bytesDropped reasonCode=\"" << reasonCode << "\""
               << " bytes=\"" << flowI->second.bytesDropped[reasonCode] << "\" />\n";
        }
        if (enableHistograms)
        {
            flowI->second.delayHistogram.SerializeToXmlStream(os, indent, "delayHistogram");
            flowI->second.jitterHistogram.SerializeToXmlStream(os, indent, "jitterHistogram");
            flowI->second.packetSizeHistogram.SerializeToXmlStream(os,
                                                                   indent,
                                                                   "packetSizeHistogram");
            flowI->second.flowInterruptionsHistogram.SerializeToXmlStream(
                os,
                indent,
                "flowInterruptionsHistogram");
        }
        indent -= 2;

        os << std::string(indent, ' ') << "</Flow>\n";
    }
    indent -= 2;
    os << std::string(indent, ' ') << "</FlowStats>\n";

    for (auto iter = m_classifiers.begin(); iter != m_classifiers.end(); iter++)
    {
        (*iter)->SerializeToXmlStream(os, indent);
    }

    if (enableProbes)
    {
        os << std::string(indent, ' ') << "<FlowProbes>\n";
        indent += 2;
        for (uint32_t i = 0; i < m_flowProbes.size(); i++)
        {
            m_flowProbes[i]->SerializeToXmlStream(os, indent, i);
        }
        indent -= 2;
        os << std::string(indent, ' ') << "</FlowProbes>\n";
    }

    indent -= 2;
    os << std::string(indent, ' ') << "</FlowMonitor>\n";
}

std::string
FlowMonitor::SerializeToXmlString(uint16_t indent, bool enableHistograms, bool enableProbes)
{
    NS_LOG_FUNCTION(this << indent << enableHistograms << enableProbes);
    std::ostringstream os;
    SerializeToXmlStream(os, indent, enableHistograms, enableProbes);
    return os.str();
}

void
FlowMonitor::SerializeToXmlFile(std::string fileName, bool enableHistograms, bool enableProbes)
{
    NS_LOG_FUNCTION(this << fileName << enableHistograms << enableProbes);
    std::ofstream os(fileName, std::ios::out | std::ios::binary);
    os << "<?xml version=\"1.0\" ?>\n";
    SerializeToXmlStream(os, 0, enableHistograms, enableProbes);
    os.close();
}

void
FlowMonitor::ResetAllStats()
{
    NS_LOG_FUNCTION(this);

    for (auto& iter : m_flowStats)
    {
        auto& flowStat = iter.second;
        flowStat.delaySum = Seconds(0);
        flowStat.jitterSum = Seconds(0);
        flowStat.lastDelay = Seconds(0);
        flowStat.maxDelay = Seconds(0);
        flowStat.minDelay = Seconds(std::numeric_limits<double>::max());
        flowStat.txBytes = 0;
        flowStat.rxBytes = 0;
        flowStat.txPackets = 0;
        flowStat.rxPackets = 0;
        flowStat.lostPackets = 0;
        flowStat.timesForwarded = 0;
        flowStat.bytesDropped.clear();
        flowStat.packetsDropped.clear();

        flowStat.delayHistogram.Clear();
        flowStat.jitterHistogram.Clear();
        flowStat.packetSizeHistogram.Clear();
        flowStat.flowInterruptionsHistogram.Clear();
    }
}

} // namespace ns3
