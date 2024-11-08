/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * The idea to use a std c++ map came from GTNetS
 */

#include "map-scheduler.h"

#include "assert.h"
#include "event-impl.h"
#include "log.h"

#include <string>

/**
 * @file
 * @ingroup scheduler
 * ns3::MapScheduler implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MapScheduler");

NS_OBJECT_ENSURE_REGISTERED(MapScheduler);

TypeId
MapScheduler::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MapScheduler")
                            .SetParent<Scheduler>()
                            .SetGroupName("Core")
                            .AddConstructor<MapScheduler>();
    return tid;
}

MapScheduler::MapScheduler()
{
    NS_LOG_FUNCTION(this);
}

MapScheduler::~MapScheduler()
{
    NS_LOG_FUNCTION(this);
}

void
MapScheduler::Insert(const Event& ev)
{
    NS_LOG_FUNCTION(this << ev.impl << ev.key.m_ts << ev.key.m_uid);
    std::pair<EventMapI, bool> result;
    result = m_list.insert(std::make_pair(ev.key, ev.impl));
    NS_ASSERT(result.second);
}

bool
MapScheduler::IsEmpty() const
{
    NS_LOG_FUNCTION(this);
    return m_list.empty();
}

Scheduler::Event
MapScheduler::PeekNext() const
{
    NS_LOG_FUNCTION(this);
    auto i = m_list.begin();
    NS_ASSERT(i != m_list.end());

    Event ev;
    ev.impl = i->second;
    ev.key = i->first;
    NS_LOG_DEBUG(this << ": " << ev.impl << ", " << ev.key.m_ts << ", " << ev.key.m_uid);
    return ev;
}

Scheduler::Event
MapScheduler::RemoveNext()
{
    NS_LOG_FUNCTION(this);
    auto i = m_list.begin();
    NS_ASSERT(i != m_list.end());
    Event ev;
    ev.impl = i->second;
    ev.key = i->first;
    m_list.erase(i);
    NS_LOG_DEBUG("@" << this << ": " << ev.impl << ", " << ev.key.m_ts << ", " << ev.key.m_uid);
    return ev;
}

void
MapScheduler::Remove(const Event& ev)
{
    NS_LOG_FUNCTION(this << ev.impl << ev.key.m_ts << ev.key.m_uid);
    auto i = m_list.find(ev.key);
    NS_ASSERT(i->second == ev.impl);
    m_list.erase(i);
}

} // namespace ns3
