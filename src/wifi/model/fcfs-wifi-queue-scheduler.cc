/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "fcfs-wifi-queue-scheduler.h"

#include "wifi-mac-queue.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("FcfsWifiQueueScheduler");

bool
operator==(const FcfsPrio& lhs, const FcfsPrio& rhs)
{
    return lhs.priority == rhs.priority && lhs.type == rhs.type;
}

bool
operator<(const FcfsPrio& lhs, const FcfsPrio& rhs)
{
    // Control queues have the highest priority
    if (lhs.type == WIFI_CTL_QUEUE && rhs.type != WIFI_CTL_QUEUE)
    {
        return true;
    }
    if (lhs.type != WIFI_CTL_QUEUE && rhs.type == WIFI_CTL_QUEUE)
    {
        return false;
    }
    // Management queues have the second highest priority
    if (lhs.type == WIFI_MGT_QUEUE && rhs.type != WIFI_MGT_QUEUE)
    {
        return true;
    }
    if (lhs.type != WIFI_MGT_QUEUE && rhs.type == WIFI_MGT_QUEUE)
    {
        return false;
    }
    // we get here if both priority values refer to container queues of the same type,
    // hence we can compare the time values.
    return lhs.priority < rhs.priority;
}

NS_OBJECT_ENSURE_REGISTERED(FcfsWifiQueueScheduler);

TypeId
FcfsWifiQueueScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FcfsWifiQueueScheduler")
                            .SetParent<WifiMacQueueSchedulerImpl<FcfsPrio>>()
                            .SetGroupName("Wifi")
                            .AddConstructor<FcfsWifiQueueScheduler>();
    return tid;
}

FcfsWifiQueueScheduler::FcfsWifiQueueScheduler()
    : NS_LOG_TEMPLATE_DEFINE("FcfsWifiQueueScheduler")
{
}

void
FcfsWifiQueueScheduler::DoNotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << +ac << *mpdu);

    const auto queueId = WifiMacQueueContainer::GetQueueId(mpdu);

    // priority is determined by the head of the queue
    auto item = GetWifiMacQueue(ac)->PeekByQueueId(queueId);
    NS_ASSERT(item);

    SetPriority(ac, queueId, {item->GetTimestamp(), queueId.type});
}

void
FcfsWifiQueueScheduler::DoNotifyDequeue(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus)
{
    NS_LOG_FUNCTION(this << +ac << mpdus.size());

    std::set<WifiContainerQueueId> queueIds;

    for (const auto& mpdu : mpdus)
    {
        queueIds.insert(WifiMacQueueContainer::GetQueueId(mpdu));
    }

    for (const auto& queueId : queueIds)
    {
        if (auto item = GetWifiMacQueue(ac)->PeekByQueueId(queueId))
        {
            SetPriority(ac, queueId, {item->GetTimestamp(), queueId.type});
        }
    }
}

void
FcfsWifiQueueScheduler::DoNotifyRemove(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus)
{
    NS_LOG_FUNCTION(this << +ac << mpdus.size());

    std::set<WifiContainerQueueId> queueIds;

    for (const auto& mpdu : mpdus)
    {
        queueIds.insert(WifiMacQueueContainer::GetQueueId(mpdu));
    }

    for (const auto& queueId : queueIds)
    {
        if (auto item = GetWifiMacQueue(ac)->PeekByQueueId(queueId))
        {
            SetPriority(ac, queueId, {item->GetTimestamp(), queueId.type});
        }
    }
}

} // namespace ns3
