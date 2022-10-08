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

    // Management frames should be prioritized
    if (m_dropPolicy == DROP_OLDEST || mpdu->GetHeader().IsMgt())
    {
        auto sortedQueuesIt = GetSortedQueues(ac).begin();

        while (sortedQueuesIt != GetSortedQueues(ac).end() &&
               std::get<WifiContainerQueueType>(sortedQueuesIt->second.get().first) ==
                   WIFI_MGT_QUEUE)
        {
            sortedQueuesIt++;
        }

        if (sortedQueuesIt != GetSortedQueues(ac).end())
        {
            return queue->PeekByQueueId(sortedQueuesIt->second.get().first);
        }
    }
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

    auto priority =
        (mpdu->GetHeader().IsMgt() ? Seconds(0) // Highest priority for management frames
                                   : mpdu->GetExpiryTime());
    SetPriority(ac, queueId, priority);
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
        if (std::get<WifiContainerQueueType>(queueId) == WIFI_MGT_QUEUE)
        {
            // the priority of management queues does not change
            continue;
        }

        if (auto item = GetWifiMacQueue(ac)->PeekByQueueId(queueId); item != nullptr)
        {
            SetPriority(ac, queueId, item->GetExpiryTime());
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
        if (std::get<0>(queueId) == WIFI_MGT_QUEUE)
        {
            // the priority of management queues does not change
            continue;
        }

        if (auto item = GetWifiMacQueue(ac)->PeekByQueueId(queueId); item != nullptr)
        {
            SetPriority(ac, queueId, item->GetExpiryTime());
        }
    }
}

} // namespace ns3
