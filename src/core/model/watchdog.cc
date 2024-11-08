/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "watchdog.h"

#include "log.h"

/**
 * @file
 * @ingroup timer
 * ns3::Watchdog timer class implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Watchdog");

Watchdog::Watchdog()
    : m_impl(nullptr),
      m_event(),
      m_end()
{
    NS_LOG_FUNCTION_NOARGS();
}

Watchdog::~Watchdog()
{
    NS_LOG_FUNCTION(this);
    m_event.Cancel();
    delete m_impl;
}

void
Watchdog::Ping(Time delay)
{
    NS_LOG_FUNCTION(this << delay);
    Time end = Simulator::Now() + delay;
    m_end = std::max(m_end, end);
    if (m_event.IsPending())
    {
        return;
    }
    m_event = Simulator::Schedule(m_end - Now(), &Watchdog::Expire, this);
}

void
Watchdog::Expire()
{
    NS_LOG_FUNCTION(this);
    if (m_end == Simulator::Now())
    {
        m_impl->Invoke();
    }
    else
    {
        m_event = Simulator::Schedule(m_end - Now(), &Watchdog::Expire, this);
    }
}

} // namespace ns3
