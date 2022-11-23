/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "fcfs-wifi-queue-scheduler.h"

#include "wifi-mac-queue.h"

#include "ns3/enum.h"
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
                            .SetParent<WifiMacQueueSchedulerImpl<Time>>()
                            .SetGroupName("Wifi")
                            .AddConstructor<FcfsWifiQueueScheduler>()
                            .AddAttribute("DropPolicy",
                                          "Upon enqueue with full queue, drop oldest (DropOldest) "
                                          "or newest (DropNewest) packet",
                                          EnumValue(DROP_NEWEST),
                                          MakeEnumAccessor(&FcfsWifiQueueScheduler::m_dropPolicy),
                                          MakeEnumChecker(FcfsWifiQueueScheduler::DROP_OLDEST,
                                                          "DropOldest",
                                                          FcfsWifiQueueScheduler::DROP_NEWEST,
                                                          "DropNewest"));
    return tid;
}

FcfsWifiQueueScheduler::FcfsWifiQueueScheduler()
    : NS_LOG_TEMPLATE_DEFINE("FcfsWifiQueueScheduler")
{
}

Ptr<WifiMpdu>
FcfsWifiQueueScheduler::HasToDropBeforeEnqueuePriv(AcIndex ac, Ptr<WifiMpdu> mpdu)
{
    auto queue = GetWifiMacQueue(ac);
    if (queue->QueueBase::GetNPackets() < queue->GetMaxSize().GetValue())
    {
        // the queue is not full, do not drop anything
        return nullptr;
    }

    // Control and management frames should be prioritized
    if (m_dropPolicy == DROP_OLDEST || mpdu->GetHeader().IsCtl() || mpdu->GetHeader().IsMgt())
    {
        for (const auto& [priority, queueInfo] : GetSortedQueues(ac))
        {
            if (std::get<WifiContainerQueueType>(queueInfo.get().first) == WIFI_MGT_QUEUE ||
                std::get<WifiContainerQueueType>(queueInfo.get().first) == WIFI_CTL_QUEUE)
            {
                // do not drop control or management frames
                continue;
            }

            // do not drop frames that are inflight or to be retransmitted
            Ptr<WifiMpdu> item;
            while ((item = queue->PeekByQueueId(queueInfo.get().first, item)))
            {
                if (!item->IsInFlight() && !item->GetHeader().IsRetry())
                {
                    NS_LOG_DEBUG("Dropping " << *item);
                    return item;
                }
            }
        }
    }
    NS_LOG_DEBUG("Dropping received MPDU: " << *mpdu);
    return mpdu;
}

void
FcfsWifiQueueScheduler::DoNotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << +ac << *mpdu);

    const auto queueId = WifiMacQueueContainer::GetQueueId(mpdu);

    if (GetWifiMacQueue(ac)->GetNPackets(queueId) > 1)
    {
        // Enqueue takes place at the tail, while the priority is determined by the
        // head of the queue. Therefore, if the queue was not empty before inserting
        // this MPDU, priority does not change
        return;
    }

    SetPriority(ac, queueId, {mpdu->GetExpiryTime(), std::get<WifiContainerQueueType>(queueId)});
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
            SetPriority(ac,
                        queueId,
                        {item->GetExpiryTime(), std::get<WifiContainerQueueType>(queueId)});
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
            SetPriority(ac,
                        queueId,
                        {item->GetExpiryTime(), std::get<WifiContainerQueueType>(queueId)});
        }
    }
}

} // namespace ns3
