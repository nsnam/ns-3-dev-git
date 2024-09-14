/*
 * Copyright (c) 2024 Huazhong University of Science and Technology
 * Copyright (c) 2024 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:  Muyuan Shen <muyuan@uw.edu>
 *           Hao Yin <haoyin@uw.edu>
 */

#include "wifi-tx-stats-helper.h"

#include "ns3/frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/net-device-container.h"
#include "ns3/node-container.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"

#include <functional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiTxStatsHelper");

WifiTxStatsHelper::WifiTxStatsHelper()
{
    NS_LOG_FUNCTION(this);
}

WifiTxStatsHelper::WifiTxStatsHelper(Time startTime, Time stopTime)
    : m_startTime(startTime),
      m_stopTime(stopTime)
{
    NS_LOG_FUNCTION(this << startTime.As(Time::S) << stopTime.As(Time::S));
    NS_ASSERT_MSG(startTime <= stopTime,
                  "Invalid Start: " << startTime << " and Stop: " << stopTime << " Time");
}

void
WifiTxStatsHelper::Enable(const NodeContainer& nodes)
{
    NS_LOG_FUNCTION(this << nodes.GetN());
    NetDeviceContainer netDevices;
    for (uint32_t i = 0; i < nodes.GetN(); ++i)
    {
        for (uint32_t j = 0; j < nodes.Get(i)->GetNDevices(); ++j)
        {
            netDevices.Add(nodes.Get(i)->GetDevice(j));
        }
    }
    Enable(netDevices);
}

void
WifiTxStatsHelper::Enable(const NetDeviceContainer& devices)
{
    NS_LOG_FUNCTION(this << devices.GetN());

    for (uint32_t j = 0; j < devices.GetN(); ++j)
    {
        auto device = DynamicCast<WifiNetDevice>(devices.Get(j));
        if (!device)
        {
            NS_LOG_DEBUG("Ignoring deviceId: " << devices.Get(j)->GetIfIndex() << " on nodeId: "
                                               << devices.Get(j)->GetNode()->GetId()
                                               << " because it is not of type WifiNetDevice");
            continue;
        }
        NS_LOG_INFO("Adding deviceId: " << devices.Get(j)->GetIfIndex()
                                        << " on nodeId: " << devices.Get(j)->GetNode()->GetId());
        const auto nodeId = devices.Get(j)->GetNode()->GetId();
        const auto deviceId = devices.Get(j)->GetIfIndex();
        if (device->GetMac()->GetQosSupported())
        {
            for (const auto& [ac, wifiAc] : wifiAcList)
            {
                // Trace enqueue for available ACs
                device->GetMac()->GetTxopQueue(ac)->TraceConnectWithoutContext(
                    "Enqueue",
                    MakeCallback(&WifiTxStatsHelper::NotifyMacEnqueue, this)
                        .Bind(nodeId, deviceId));
            }
        }
        else
        {
            // Trace enqueue for Non-QoS AC
            device->GetMac()
                ->GetTxopQueue(AC_BE_NQOS)
                ->TraceConnectWithoutContext(
                    "Enqueue",
                    MakeCallback(&WifiTxStatsHelper::NotifyMacEnqueue, this)
                        .Bind(nodeId, deviceId));
        }
        device->GetMac()->TraceConnectWithoutContext(
            "AckedMpdu",
            MakeCallback(&WifiTxStatsHelper::NotifyAcked, this).Bind(nodeId, deviceId));
        device->GetMac()->TraceConnectWithoutContext(
            "DroppedMpdu",
            MakeCallback(&WifiTxStatsHelper::NotifyMacDropped, this).Bind(nodeId, deviceId));
        for (int i = 0; i < device->GetNPhys(); ++i)
        {
            device->GetPhy(i)->TraceConnectWithoutContext(
                "PhyTxBegin",
                MakeCallback(&WifiTxStatsHelper::NotifyTxStart, this).Bind(nodeId, deviceId));
        }
    }
}

void
WifiTxStatsHelper::Start(Time startTime)
{
    NS_LOG_FUNCTION(this << startTime.As(Time::S));
    NS_ASSERT_MSG(startTime >= Now(), "Invalid Start: " << startTime << " less than current time");
    NS_ASSERT_MSG(startTime <= m_stopTime,
                  "Invalid Start: " << startTime << " and Stop: " << m_stopTime << " Time");
    m_startTime = startTime;
}

void
WifiTxStatsHelper::Stop(Time stopTime)
{
    NS_LOG_FUNCTION(this << stopTime.As(Time::S));
    NS_ASSERT_MSG(stopTime >= Now(), "Invalid Stop: " << stopTime << " less than current time");
    NS_ASSERT_MSG(m_startTime <= stopTime,
                  "Invalid Start: " << m_startTime << " and Stop: " << stopTime << " Time");
    m_stopTime = stopTime;
}

void
WifiTxStatsHelper::Reset()
{
    NS_LOG_FUNCTION(this);
    m_successMap.clear();
    m_failureMap.clear();
    m_startTime = Now();
}

WifiTxStatsHelper::CountPerNodeDevice_t
WifiTxStatsHelper::GetSuccessesByNodeDevice() const
{
    WifiTxStatsHelper::CountPerNodeDevice_t results;
    for (const auto& [nodeDevTuple, records] : m_successMap)
    {
        results[{std::get<0>(nodeDevTuple), std::get<1>(nodeDevTuple)}] = records.size();
    }
    return results;
}

WifiTxStatsHelper::CountPerNodeDeviceLink_t
WifiTxStatsHelper::GetSuccessesByNodeDeviceLink(WifiTxStatsHelper::MultiLinkSuccessType type) const
{
    WifiTxStatsHelper::CountPerNodeDeviceLink_t results;
    for (const auto& [nodeDevTuple, records] : m_successMap)
    {
        for (const auto& record : records)
        {
            NS_ASSERT_MSG(!record.m_successLinkIdSet.empty(), "No LinkId set on MPDU");
            if (type == FIRST_LINK_IN_SET)
            {
                const auto firstLinkId = *record.m_successLinkIdSet.begin();
                const auto nodeDevLinkTuple =
                    std::tuple<uint32_t, uint32_t, uint8_t>(std::get<0>(nodeDevTuple),
                                                            std::get<1>(nodeDevTuple),
                                                            firstLinkId);
                if (auto countIt = results.find(nodeDevLinkTuple); countIt != results.end())
                {
                    countIt->second++;
                }
                else
                {
                    results[nodeDevLinkTuple] = 1;
                }
            }
            else
            {
                for (const auto linkId : record.m_successLinkIdSet)
                {
                    const auto nodeDevLinkTuple =
                        std::tuple<uint32_t, uint32_t, uint8_t>(std::get<0>(nodeDevTuple),
                                                                std::get<1>(nodeDevTuple),
                                                                linkId);
                    if (auto countIt = results.find(nodeDevLinkTuple); countIt != results.end())
                    {
                        countIt->second++;
                    }
                    else
                    {
                        results[nodeDevLinkTuple] = 1;
                    }
                }
            }
        }
    }
    return results;
}

WifiTxStatsHelper::CountPerNodeDevice_t
WifiTxStatsHelper::GetFailuresByNodeDevice() const
{
    WifiTxStatsHelper::CountPerNodeDevice_t results;
    for (const auto& [nodeDevTuple, records] : m_failureMap)
    {
        results[{std::get<0>(nodeDevTuple), std::get<1>(nodeDevTuple)}] = records.size();
    }
    return results;
}

WifiTxStatsHelper::CountPerNodeDevice_t
WifiTxStatsHelper::GetFailuresByNodeDevice(WifiMacDropReason reason) const
{
    WifiTxStatsHelper::CountPerNodeDevice_t results;
    for (const auto& [nodeDevTuple, records] : m_failureMap)
    {
        for (const auto& record : records)
        {
            NS_ASSERT_MSG(record.m_dropTime.has_value() && record.m_dropReason.has_value(),
                          "Incomplete dropped MPDU record");
            if (record.m_dropReason == reason)
            {
                if (auto countIt =
                        results.find({std::get<0>(nodeDevTuple), std::get<1>(nodeDevTuple)});
                    countIt != results.end())
                {
                    countIt->second++;
                }
                else
                {
                    results[{std::get<0>(nodeDevTuple), std::get<1>(nodeDevTuple)}] = 1;
                }
            }
        }
    }
    return results;
}

WifiTxStatsHelper::CountPerNodeDevice_t
WifiTxStatsHelper::GetRetransmissionsByNodeDevice() const
{
    WifiTxStatsHelper::CountPerNodeDevice_t results;
    for (const auto& [nodeDevLinkTuple, records] : m_successMap)
    {
        for (const auto& record : records)
        {
            if (auto countIt =
                    results.find({std::get<0>(nodeDevLinkTuple), std::get<1>(nodeDevLinkTuple)});
                countIt != results.end())
            {
                countIt->second += record.m_retransmissions;
            }
            else
            {
                results[{std::get<0>(nodeDevLinkTuple), std::get<1>(nodeDevLinkTuple)}] =
                    record.m_retransmissions;
            }
        }
    }
    return results;
}

uint64_t
WifiTxStatsHelper::GetSuccesses() const
{
    uint64_t count{0};
    for (const auto& [nodeDevLinkTuple, records] : m_successMap)
    {
        count += records.size();
    }
    return count;
}

uint64_t
WifiTxStatsHelper::GetFailures() const
{
    uint64_t count{0};
    for (const auto& [nodeDevTuple, records] : m_failureMap)
    {
        count += records.size();
    }
    return count;
}

uint64_t
WifiTxStatsHelper::GetFailures(WifiMacDropReason reason) const
{
    uint64_t count{0};
    for (const auto& [nodeDevTuple, records] : m_failureMap)
    {
        for (const auto& record : records)
        {
            NS_ASSERT_MSG(record.m_dropTime.has_value() && record.m_dropReason.has_value(),
                          "Incomplete dropped MPDU record");
            if (record.m_dropReason == reason)
            {
                count++;
            }
        }
    }
    return count;
}

uint64_t
WifiTxStatsHelper::GetRetransmissions() const
{
    uint64_t count{0};
    for (const auto& [nodeDevTuple, records] : m_successMap)
    {
        for (const auto& record : records)
        {
            count += record.m_retransmissions;
        }
    }
    return count;
}

Time
WifiTxStatsHelper::GetDuration() const
{
    return Now() - m_startTime;
}

const WifiTxStatsHelper::MpduRecordsPerNodeDeviceLink_t
WifiTxStatsHelper::GetSuccessRecords(WifiTxStatsHelper::MultiLinkSuccessType type) const
{
    WifiTxStatsHelper::MpduRecordsPerNodeDeviceLink_t results;
    for (const auto& [nodeDevTuple, records] : m_successMap)
    {
        for (const auto& record : records)
        {
            NS_ASSERT_MSG(!record.m_successLinkIdSet.empty(), "No LinkId set on MPDU");
            if (type == FIRST_LINK_IN_SET)
            {
                const auto firstLinkId = *record.m_successLinkIdSet.begin();
                const auto nodeDevLinkTuple =
                    std::tuple<uint32_t, uint32_t, uint8_t>(std::get<0>(nodeDevTuple),
                                                            std::get<1>(nodeDevTuple),
                                                            firstLinkId);
                results[nodeDevLinkTuple].emplace_back(record);
            }
            else
            {
                for (const auto linkId : record.m_successLinkIdSet)
                {
                    const auto nodeDevLinkTuple =
                        std::tuple<uint32_t, uint32_t, uint8_t>(std::get<0>(nodeDevTuple),
                                                                std::get<1>(nodeDevTuple),
                                                                linkId);
                    results[nodeDevLinkTuple].emplace_back(record);
                }
            }
        }
    }
    return results;
}

const WifiTxStatsHelper::MpduRecordsPerNodeDevice_t&
WifiTxStatsHelper::GetFailureRecords() const
{
    return m_failureMap;
}

void
WifiTxStatsHelper::NotifyMacEnqueue(uint32_t nodeId, uint32_t deviceId, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << nodeId << deviceId << mpdu);
    const auto now = Simulator::Now();
    if (now > m_stopTime)
    {
        NS_LOG_DEBUG("Ignoring enqueue because helper is stopped");
        return;
    }
    if (mpdu->GetHeader().IsData())
    {
        if (!mpdu->GetHeader().HasData())
        {
            // exclude Null frames
            return;
        }
        MpduRecord record{.m_nodeId = nodeId, .m_deviceId = deviceId, .m_enqueueTime = now};
        if (mpdu->GetHeader().IsQosData())
        {
            record.m_tid = mpdu->GetHeader().GetQosTid();
        }
        NS_LOG_INFO("Creating inflight record for packet UID "
                    << mpdu->GetPacket()->GetUid() << " node " << nodeId << " device " << deviceId
                    << " tid " << +record.m_tid);
        m_inflightMap[mpdu->GetPacket()->GetUid()] = record;
    }
}

void
WifiTxStatsHelper::NotifyTxStart(uint32_t nodeId, uint32_t deviceId, Ptr<const Packet> pkt, Watt_u)
{
    NS_LOG_FUNCTION(this << nodeId << deviceId << pkt);
    const auto now = Now();
    if (now > m_stopTime)
    {
        NS_LOG_DEBUG("Ignoring TxStart because helper is stopped");
        return;
    }
    if (auto pktRecord = m_inflightMap.find(pkt->GetUid()); pktRecord != m_inflightMap.end())
    {
        auto& [uid, record] = *pktRecord;
        NS_LOG_INFO("Packet UID " << uid << " started");
        if (record.m_txStartTime.IsZero())
        {
            NS_LOG_INFO("TxStart called (first transmission) for inflight packet UID "
                        << pkt->GetUid());
            record.m_txStartTime = now;
        }
        else
        {
            NS_LOG_INFO("TxStart called (retransmission) for inflight packet UID "
                        << pkt->GetUid());
            record.m_retransmissions++;
        }
    }
}

void
WifiTxStatsHelper::NotifyAcked(uint32_t nodeId, uint32_t deviceId, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << nodeId << deviceId << mpdu);
    NS_ASSERT_MSG(!mpdu->GetInFlightLinkIds().empty(), "No LinkId set on MPDU");
    const auto now = Now();
    if (now <= m_startTime || now > m_stopTime)
    {
        if (auto pktRecord = m_inflightMap.find(mpdu->GetPacket()->GetUid());
            pktRecord != m_inflightMap.end())
        {
            m_inflightMap.erase(pktRecord);
        }
        NS_LOG_DEBUG("Ignoring acknowledgement because the time is out of range");
        return;
    }
    // Get the set of in-flight link IDs
    const auto linkIds = mpdu->GetInFlightLinkIds();
    if (auto pktRecord = m_inflightMap.find(mpdu->GetPacket()->GetUid());
        pktRecord != m_inflightMap.end())
    {
        auto& [uid, record] = *pktRecord;
        record.m_ackTime = now;
        record.m_successLinkIdSet = linkIds;
        record.m_mpduSeqNum = mpdu->GetHeader().GetSequenceNumber();
        // Store record in success map and remove it below from inflight map
        NS_LOG_INFO("Successful transmission logged of packet UID " << uid);
        m_successMap[{nodeId, deviceId}].push_back(std::move(record));
        NS_LOG_INFO("Erasing packet UID " << uid << " from inflight map due to success");
        m_inflightMap.erase(pktRecord);
    }
}

void
WifiTxStatsHelper::NotifyMacDropped(uint32_t nodeId,
                                    uint32_t deviceId,
                                    WifiMacDropReason reason,
                                    Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << nodeId << deviceId << +reason << mpdu);
    const auto now = Now();
    if (now <= m_startTime || now > m_stopTime)
    {
        if (auto pktRecord = m_inflightMap.find(mpdu->GetPacket()->GetUid());
            pktRecord != m_inflightMap.end())
        {
            m_inflightMap.erase(pktRecord);
        }
        NS_LOG_DEBUG("Ignoring drop because the time is out of range");
        return;
    }
    if (auto pktRecord = m_inflightMap.find(mpdu->GetPacket()->GetUid());
        pktRecord != m_inflightMap.end())
    {
        auto& [uid, record] = *pktRecord;
        NS_LOG_INFO("Packet UID " << uid << " dropped");
        record.m_dropTime = now;
        record.m_dropReason = reason;
        record.m_mpduSeqNum = mpdu->GetHeader().GetSequenceNumber();
        NS_LOG_INFO("Failed transmission logged of packet UID " << uid);
        // Store record in failure map and remove it below from inflight map
        m_failureMap[{nodeId, deviceId}].push_back(std::move(record));
        NS_LOG_INFO("Erasing packet UID " << uid << " from inflight map due to failure");
        m_inflightMap.erase(pktRecord);
    }
}

} // namespace ns3
