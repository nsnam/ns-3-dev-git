/*
 * Copyright (c) 2007 INESC Porto
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt>
 */
#include "event-garbage-collector.h"

/**
 * @file
 * @ingroup core-helpers
 * @ingroup events
 * ns3::EventGarbageCollector implementation.
 */

namespace ns3
{

EventGarbageCollector::EventGarbageCollector()
    : m_nextCleanupSize(CHUNK_INIT_SIZE),
      m_events()
{
}

void
EventGarbageCollector::Track(EventId event)
{
    m_events.insert(event);
    if (m_events.size() >= m_nextCleanupSize)
    {
        Cleanup();
    }
}

void
EventGarbageCollector::Grow()
{
    m_nextCleanupSize += (m_nextCleanupSize < CHUNK_MAX_SIZE ? m_nextCleanupSize : CHUNK_MAX_SIZE);
}

void
EventGarbageCollector::Shrink()
{
    while (m_nextCleanupSize > m_events.size())
    {
        m_nextCleanupSize >>= 1;
    }
    Grow();
}

// Called when a new event was added and
// the cleanup limit was exceeded in consequence.
void
EventGarbageCollector::Cleanup()
{
    for (auto iter = m_events.begin(); iter != m_events.end();)
    {
        if ((*iter).IsExpired())
        {
            m_events.erase(iter++);
        }
        else
        {
            break; // EventIds are sorted by timestamp => further events are not expired for sure
        }
    }

    // If after cleanup we are still over the limit, increase the limit.
    if (m_events.size() >= m_nextCleanupSize)
    {
        Grow();
    }
    else
    {
        Shrink();
    }
}

EventGarbageCollector::~EventGarbageCollector()
{
    for (const auto& event : m_events)
    {
        Simulator::Cancel(event);
    }
}

} // namespace ns3
