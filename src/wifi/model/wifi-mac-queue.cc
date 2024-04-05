/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-mac-queue.h"

#include "wifi-mac-queue-scheduler.h"

#include "ns3/simulator.h"

#include <functional>
#include <optional>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiMacQueue");

NS_OBJECT_ENSURE_REGISTERED(WifiMacQueue);
NS_OBJECT_TEMPLATE_CLASS_TWO_DEFINE(Queue, WifiMpdu, WifiMacQueueContainer);

TypeId
WifiMacQueue::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiMacQueue")
            .SetParent<Queue<WifiMpdu, WifiMacQueueContainer>>()
            .SetGroupName("Wifi")
            .AddConstructor<WifiMacQueue>()
            .AddAttribute("MaxSize",
                          "The max queue size",
                          QueueSizeValue(QueueSize("500p")),
                          MakeQueueSizeAccessor(&QueueBase::SetMaxSize, &QueueBase::GetMaxSize),
                          MakeQueueSizeChecker())
            .AddAttribute("MaxDelay",
                          "If a packet stays longer than this delay in the queue, it is dropped.",
                          TimeValue(MilliSeconds(500)),
                          MakeTimeAccessor(&WifiMacQueue::SetMaxDelay),
                          MakeTimeChecker())
            .AddTraceSource("Expired",
                            "MPDU dropped because its lifetime expired.",
                            MakeTraceSourceAccessor(&WifiMacQueue::m_traceExpired),
                            "ns3::WifiMpdu::TracedCallback");
    return tid;
}

WifiMacQueue::WifiMacQueue(AcIndex ac)
    : m_ac(ac),
      NS_LOG_TEMPLATE_DEFINE("WifiMacQueue")
{
}

WifiMacQueue::~WifiMacQueue()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
WifiMacQueue::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_scheduler = nullptr;
    Queue<WifiMpdu, WifiMacQueueContainer>::DoDispose();
}

AcIndex
WifiMacQueue::GetAc() const
{
    return m_ac;
}

WifiMacQueue::Iterator
WifiMacQueue::GetIt(Ptr<const WifiMpdu> mpdu) const
{
    NS_ASSERT(mpdu->IsQueued());
    return mpdu->GetQueueIt(WmqIteratorTag());
}

Ptr<WifiMpdu>
WifiMacQueue::GetOriginal(Ptr<WifiMpdu> mpdu)
{
    return GetIt(mpdu)->mpdu;
}

Ptr<WifiMpdu>
WifiMacQueue::GetAlias(Ptr<const WifiMpdu> mpdu, uint8_t linkId)
{
    if (!mpdu->IsQueued())
    {
        return nullptr;
    }
    if (auto aliasIt = GetIt(mpdu)->inflights.find(linkId);
        aliasIt != GetIt(mpdu)->inflights.cend())
    {
        return aliasIt->second;
    }
    return nullptr;
}

void
WifiMacQueue::ExtractExpiredMpdus(const WifiContainerQueueId& queueId) const
{
    NS_LOG_FUNCTION(this);

    std::list<Ptr<WifiMpdu>> mpdus;
    auto [first, last] = GetContainer().ExtractExpiredMpdus(queueId);

    for (auto it = first; it != last; it++)
    {
        mpdus.push_back(it->mpdu);
    }
    for (const auto& mpdu : mpdus)
    {
        // fire the Expired trace
        auto fire = [this, mpdu]() -> void { this->m_traceExpired(mpdu); };
        Simulator::ScheduleNow(fire);
    }
    // notify the scheduler
    if (!mpdus.empty())
    {
        m_scheduler->NotifyRemove(m_ac, mpdus);
    }
}

void
WifiMacQueue::ExtractAllExpiredMpdus() const
{
    NS_LOG_FUNCTION(this);

    std::list<Ptr<WifiMpdu>> mpdus;
    auto [first, last] = GetContainer().ExtractAllExpiredMpdus();

    for (auto it = first; it != last; it++)
    {
        mpdus.push_back(it->mpdu);
    }
    for (const auto& mpdu : mpdus)
    {
        // fire the Expired trace
        auto fire = [this, mpdu]() -> void { this->m_traceExpired(mpdu); };
        Simulator::ScheduleNow(fire);
    }
    // notify the scheduler
    if (!mpdus.empty())
    {
        m_scheduler->NotifyRemove(m_ac, mpdus);
    }
}

void
WifiMacQueue::WipeAllExpiredMpdus()
{
    NS_LOG_FUNCTION(this);

    ExtractAllExpiredMpdus();

    auto [first, last] = GetContainer().GetAllExpiredMpdus();

    for (auto it = first; it != last;)
    {
        // the scheduler has been notified and the Expired trace has been fired
        // when the MPDU was extracted from its queue. The only thing left to do
        // is to update the Queue base class statistics by calling Queue::DoRemove
        auto curr = it++;
        Queue<WifiMpdu, WifiMacQueueContainer>::DoRemove(curr);
    }
}

bool
WifiMacQueue::TtlExceeded(Ptr<const WifiMpdu> item, const Time& now)
{
    NS_ASSERT(item && item->IsQueued());
    auto it = GetIt(item);
    if (now > it->expiryTime)
    {
        NS_LOG_DEBUG("Removing packet that stayed in the queue for too long (queuing time="
                     << now - it->expiryTime + m_maxDelay << ")");
        // Trace the expired MPDU first and then remove it from the queue (if still in the queue).
        // Indeed, the Expired traced source is connected to BlockAckManager::NotifyDiscardedMpdu,
        // which checks if the expired MPDU is in-flight or is a retransmission to determine
        // whether a BlockAckReq frame must be sent to advance the recipient window. If the
        // expired MPDU is removed from the queue before tracing the expiration, it is no longer
        // in-flight and NotifyDiscardedMpdu wrongfully assumes that a BlockAckReq is not needed.
        m_traceExpired(item);
        if (item->IsQueued())
        {
            DoRemove(it);
        }
        return true;
    }
    return false;
}

void
WifiMacQueue::SetScheduler(Ptr<WifiMacQueueScheduler> scheduler)
{
    NS_LOG_FUNCTION(this << scheduler);
    m_scheduler = scheduler;
}

void
WifiMacQueue::SetMaxDelay(Time delay)
{
    NS_LOG_FUNCTION(this << delay);
    m_maxDelay = delay;
}

Time
WifiMacQueue::GetMaxDelay() const
{
    return m_maxDelay;
}

bool
WifiMacQueue::Enqueue(Ptr<WifiMpdu> item)
{
    NS_LOG_FUNCTION(this << *item);

    auto queueId = WifiMacQueueContainer::GetQueueId(item);
    return Insert(GetContainer().GetQueue(queueId).cend(), item);
}

bool
WifiMacQueue::Insert(ConstIterator pos, Ptr<WifiMpdu> item)
{
    NS_LOG_FUNCTION(this << *item);
    NS_ASSERT_MSG(GetMaxSize().GetUnit() == QueueSizeUnit::PACKETS,
                  "WifiMacQueues must be in packet mode");

    // insert the item if the queue is not full
    if (QueueBase::GetNPackets() < GetMaxSize().GetValue())
    {
        return DoEnqueue(pos, item);
    }

    // the queue is full; try to make some room by removing stale packets
    auto queueId = WifiMacQueueContainer::GetQueueId(item);

    if (pos != GetContainer().GetQueue(queueId).cend())
    {
        NS_ABORT_MSG_IF(WifiMacQueueContainer::GetQueueId(pos->mpdu) != queueId,
                        "pos must point to an element in the same container queue as item");
        if (pos->expiryTime <= Simulator::Now())
        {
            // the element pointed to by pos is stale and will be removed along with all of
            // its predecessors; the new item will be enqueued at the front of the queue
            pos = GetContainer().GetQueue(queueId).cbegin();
        }
    }

    WipeAllExpiredMpdus();

    return DoEnqueue(pos, item);
}

Ptr<WifiMpdu>
WifiMacQueue::Dequeue()
{
    // An MPDU is dequeued when either is acknowledged or is dropped, hence a Dequeue
    // method without an argument makes no sense.
    NS_ABORT_MSG("Not implemented by WifiMacQueue");
    return nullptr;
}

void
WifiMacQueue::DequeueIfQueued(const std::list<Ptr<const WifiMpdu>>& mpdus)
{
    NS_LOG_FUNCTION(this);

    std::list<ConstIterator> iterators;

    for (const auto& mpdu : mpdus)
    {
        if (mpdu->IsQueued())
        {
            auto it = GetIt(mpdu);
            NS_ASSERT(it->ac == m_ac);
            NS_ASSERT(it->mpdu == mpdu->GetOriginal());
            iterators.emplace_back(it);
        }
    }

    DoDequeue(iterators);
}

Ptr<const WifiMpdu>
WifiMacQueue::Peek() const
{
    return Peek(std::nullopt);
}

Ptr<WifiMpdu>
WifiMacQueue::Peek(std::optional<uint8_t> linkId) const
{
    NS_LOG_FUNCTION(this);

    auto queueId = m_scheduler->GetNext(m_ac, linkId);

    if (!queueId.has_value())
    {
        NS_LOG_DEBUG("The queue is empty");
        return nullptr;
    }

    return GetContainer().GetQueue(queueId.value()).cbegin()->mpdu;
}

Ptr<WifiMpdu>
WifiMacQueue::PeekByTidAndAddress(uint8_t tid, Mac48Address dest, Ptr<const WifiMpdu> item) const
{
    NS_LOG_FUNCTION(this << +tid << dest << item);
    NS_ABORT_IF(dest.IsGroup());
    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, dest, tid);
    return PeekByQueueId(queueId, item);
}

Ptr<WifiMpdu>
WifiMacQueue::PeekByQueueId(const WifiContainerQueueId& queueId, Ptr<const WifiMpdu> item) const
{
    NS_LOG_FUNCTION(this << item);
    NS_ASSERT(!item || (item->IsQueued() && WifiMacQueueContainer::GetQueueId(item) == queueId));

    // Remove MPDUs with expired lifetime if we are looking for the first MPDU in the queue
    if (!item)
    {
        ExtractExpiredMpdus(queueId);
    }

    auto it = (item ? std::next(GetIt(item)) : GetContainer().GetQueue(queueId).cbegin());

    if (it == GetContainer().GetQueue(queueId).cend())
    {
        NS_LOG_DEBUG("The queue is empty");
        return nullptr;
    }

    return it->mpdu;
}

Ptr<WifiMpdu>
WifiMacQueue::PeekFirstAvailable(uint8_t linkId, Ptr<const WifiMpdu> item) const
{
    NS_LOG_FUNCTION(this << +linkId << item);
    NS_ASSERT(!item || item->IsQueued());

    if (item)
    {
        // check if there are other MPDUs in the same container queue as item
        auto mpdu = PeekByQueueId(WifiMacQueueContainer::GetQueueId(item), item);

        if (mpdu)
        {
            return mpdu;
        }
    }

    std::optional<WifiContainerQueueId> queueId;

    if (item)
    {
        queueId = m_scheduler->GetNext(m_ac, linkId, WifiMacQueueContainer::GetQueueId(item));
    }
    else
    {
        queueId = m_scheduler->GetNext(m_ac, linkId);
    }

    if (!queueId.has_value())
    {
        NS_LOG_DEBUG("The queue is empty");
        return nullptr;
    }

    return GetContainer().GetQueue(queueId.value()).cbegin()->mpdu;
}

Ptr<WifiMpdu>
WifiMacQueue::Remove()
{
    return Remove(Peek());
}

Ptr<WifiMpdu>
WifiMacQueue::Remove(Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << mpdu);
    NS_ASSERT(mpdu && mpdu->IsQueued());
    auto it = GetIt(mpdu);
    NS_ASSERT(it->ac == m_ac);
    NS_ASSERT(it->mpdu == mpdu->GetOriginal());

    return DoRemove(it);
}

void
WifiMacQueue::Flush()
{
    NS_LOG_FUNCTION(this);

    // there may be some expired MPDUs in the container queue storing MPDUs with expired lifetime,
    // which will not be flushed by the Flush() method of the base class.
    WipeAllExpiredMpdus();
    Queue<WifiMpdu, WifiMacQueueContainer>::Flush();
}

void
WifiMacQueue::Replace(Ptr<const WifiMpdu> currentItem, Ptr<WifiMpdu> newItem)
{
    NS_LOG_FUNCTION(this << *currentItem << *newItem);
    NS_ASSERT(currentItem->IsQueued());
    auto currentIt = GetIt(currentItem);
    NS_ASSERT(currentIt->ac == m_ac);
    NS_ASSERT(currentIt->mpdu == currentItem->GetOriginal());
    NS_ASSERT(!newItem->IsQueued());

    Time expiryTime = currentIt->expiryTime;
    auto pos = std::next(currentIt);
    DoDequeue({currentIt});
    bool ret = Insert(pos, newItem);
    GetIt(newItem)->expiryTime = expiryTime;
    // The size of a WifiMacQueue is measured as number of packets. We dequeued
    // one packet, so there is certainly room for inserting one packet
    NS_ABORT_IF(!ret);
}

uint32_t
WifiMacQueue::GetNPackets(const WifiContainerQueueId& queueId) const
{
    return GetContainer().GetQueue(queueId).size();
}

uint32_t
WifiMacQueue::GetNBytes(const WifiContainerQueueId& queueId) const
{
    return GetContainer().GetNBytes(queueId);
}

bool
WifiMacQueue::DoEnqueue(ConstIterator pos, Ptr<WifiMpdu> item)
{
    NS_LOG_FUNCTION(this << *item);

    auto currSize = GetMaxSize();
    // control frames should not consume room in the MAC queue, so increase queue size
    // if we are trying to enqueue a control frame
    if (item->GetHeader().IsCtl())
    {
        SetMaxSize(currSize + item);
    }
    auto mpdu = m_scheduler->HasToDropBeforeEnqueue(m_ac, item);

    if (mpdu == item)
    {
        // the given item must be dropped
        SetMaxSize(currSize);
        return false;
    }

    auto queueId = WifiMacQueueContainer::GetQueueId(item);
    if (pos != GetContainer().GetQueue(queueId).cend() && mpdu && pos->mpdu == mpdu->GetOriginal())
    {
        // the element pointed to by pos must be dropped; update insert position
        pos = std::next(pos);
    }
    if (mpdu)
    {
        DoRemove(GetIt(mpdu));
    }

    Iterator ret;
    if (Queue<WifiMpdu, WifiMacQueueContainer>::DoEnqueue(pos, item, ret))
    {
        // set item's information about its position in the queue
        item->SetQueueIt(ret, {});
        ret->ac = m_ac;
        ret->expiryTime = item->GetHeader().IsCtl() ? Time::Max() : Simulator::Now() + m_maxDelay;
        WmqIteratorTag tag;
        ret->deleter = [tag](auto mpdu) { mpdu->SetQueueIt(std::nullopt, tag); };

        m_scheduler->NotifyEnqueue(m_ac, item);
        return true;
    }
    SetMaxSize(currSize);
    return false;
}

void
WifiMacQueue::DoDequeue(const std::list<ConstIterator>& iterators)
{
    NS_LOG_FUNCTION(this);

    std::list<Ptr<WifiMpdu>> items;

    // First, dequeue all the items
    for (auto& it : iterators)
    {
        if (auto item = Queue<WifiMpdu, WifiMacQueueContainer>::DoDequeue(it))
        {
            items.push_back(item);
            if (item->GetHeader().IsCtl())
            {
                SetMaxSize(GetMaxSize() - item);
            }
        }
    }

    // Then, notify the scheduler
    if (!items.empty())
    {
        m_scheduler->NotifyDequeue(m_ac, items);
    }
}

Ptr<WifiMpdu>
WifiMacQueue::DoRemove(ConstIterator pos)
{
    NS_LOG_FUNCTION(this);

    auto item = Queue<WifiMpdu, WifiMacQueueContainer>::DoRemove(pos);

    if (item)
    {
        if (item->GetHeader().IsCtl())
        {
            SetMaxSize(GetMaxSize() - item);
        }
        m_scheduler->NotifyRemove(m_ac, {item});
    }

    return item;
}

} // namespace ns3
