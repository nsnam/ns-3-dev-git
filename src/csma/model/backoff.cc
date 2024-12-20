/*
 * Copyright (c) 2007, Emmanuelle Laprise
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Emmanuelle Laprise <emmanuelle.laprise@bluekazoo.ca>
 */

#include "backoff.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Backoff");

Backoff::Backoff()
{
    m_slotTime = MicroSeconds(1);
    m_minSlots = 1;
    m_maxSlots = 1000;
    m_ceiling = 10;
    m_maxRetries = 1000;
    m_numBackoffRetries = 0;
    m_rng = CreateObject<UniformRandomVariable>();

    ResetBackoffTime();
}

Backoff::Backoff(Time slotTime,
                 uint32_t minSlots,
                 uint32_t maxSlots,
                 uint32_t ceiling,
                 uint32_t maxRetries)
{
    m_slotTime = slotTime;
    m_minSlots = minSlots;
    m_maxSlots = maxSlots;
    m_ceiling = ceiling;
    m_maxRetries = maxRetries;
    m_numBackoffRetries = 0;
    m_rng = CreateObject<UniformRandomVariable>();
}

Time
Backoff::GetBackoffTime()
{
    uint32_t ceiling;

    if ((m_ceiling > 0) && (m_numBackoffRetries > m_ceiling))
    {
        ceiling = m_ceiling;
    }
    else
    {
        ceiling = m_numBackoffRetries;
    }

    uint32_t minSlot = m_minSlots;
    uint32_t maxSlot = (uint32_t)pow(2, ceiling) - 1;
    if (maxSlot > m_maxSlots)
    {
        maxSlot = m_maxSlots;
    }

    auto backoffSlots = (uint32_t)m_rng->GetValue(minSlot, maxSlot);

    Time backoff = Time(backoffSlots * m_slotTime);
    return backoff;
}

void
Backoff::ResetBackoffTime()
{
    m_numBackoffRetries = 0;
}

bool
Backoff::MaxRetriesReached() const
{
    return (m_numBackoffRetries >= m_maxRetries);
}

void
Backoff::IncrNumRetries()
{
    m_numBackoffRetries++;
}

int64_t
Backoff::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    m_rng->SetStream(stream);
    return 1;
}

} // namespace ns3
