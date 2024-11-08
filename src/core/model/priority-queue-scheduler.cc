/*
 * Copyright (c) 2016 IITP
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Alexander Krotov <krotov@iitp.ru>
 */

#include "priority-queue-scheduler.h"

#include "assert.h"
#include "event-impl.h"
#include "log-macros-disabled.h"
#include "log.h"
#include "scheduler.h"

#include <string>

/**
 * @file
 * @ingroup scheduler
 * Implementation of ns3::PriorityQueueScheduler class.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("PriorityQueueScheduler");

NS_OBJECT_ENSURE_REGISTERED(PriorityQueueScheduler);

TypeId
PriorityQueueScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::PriorityQueueScheduler")
                            .SetParent<Scheduler>()
                            .SetGroupName("Core")
                            .AddConstructor<PriorityQueueScheduler>();
    return tid;
}

PriorityQueueScheduler::PriorityQueueScheduler()
{
    NS_LOG_FUNCTION(this);
}

PriorityQueueScheduler::~PriorityQueueScheduler()
{
    NS_LOG_FUNCTION(this);
}

void
PriorityQueueScheduler::Insert(const Event& ev)
{
    NS_LOG_FUNCTION(this << ev.impl << ev.key.m_ts << ev.key.m_uid);
    m_queue.push(ev);
}

bool
PriorityQueueScheduler::IsEmpty() const
{
    NS_LOG_FUNCTION(this);
    return m_queue.empty();
}

Scheduler::Event
PriorityQueueScheduler::PeekNext() const
{
    NS_LOG_FUNCTION(this);
    return m_queue.top();
}

Scheduler::Event
PriorityQueueScheduler::RemoveNext()
{
    NS_LOG_FUNCTION(this);
    Scheduler::Event ev = m_queue.top();
    m_queue.pop();
    return ev;
}

bool
PriorityQueueScheduler::EventPriorityQueue::remove(const Scheduler::Event& ev)
{
    auto it = std::find(this->c.begin(), this->c.end(), ev);
    if (it != this->c.end())
    {
        this->c.erase(it);
        std::make_heap(this->c.begin(), this->c.end(), this->comp);
        return true;
    }
    else
    {
        return false;
    }
}

void
PriorityQueueScheduler::Remove(const Scheduler::Event& ev)
{
    NS_LOG_FUNCTION(this);
    m_queue.remove(ev);
}

} // namespace ns3
