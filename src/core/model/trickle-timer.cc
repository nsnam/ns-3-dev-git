/*
 * Copyright (c) 2020 Universita' di Firenze, Italy
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#include "trickle-timer.h"

#include "log.h"

#include <bit>
#include <limits>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TrickleTimer");

TrickleTimer::TrickleTimer()
    : m_impl(nullptr),
      m_timerExpiration(),
      m_intervalExpiration(),
      m_currentInterval(Time(0)),
      m_counter(0),
      m_uniRand(CreateObject<UniformRandomVariable>())
{
    NS_LOG_FUNCTION(this);

    m_minInterval = Time(0);
    m_ticks = 0;
    m_maxInterval = Time(0);
    m_redundancy = 0;
}

TrickleTimer::TrickleTimer(Time minInterval, uint8_t doublings, uint16_t redundancy)
    : m_impl(nullptr),
      m_timerExpiration(),
      m_intervalExpiration(),
      m_currentInterval(Time(0)),
      m_counter(0),
      m_uniRand(CreateObject<UniformRandomVariable>())
{
    NS_LOG_FUNCTION(this << minInterval << doublings << redundancy);
    NS_ASSERT_MSG(doublings < std::numeric_limits<decltype(m_ticks)>::digits,
                  "Doublings value is too large");

    m_minInterval = minInterval;
    m_ticks = 1;
    m_ticks <<= doublings;
    m_maxInterval = m_ticks * minInterval;
    m_redundancy = redundancy;
}

TrickleTimer::~TrickleTimer()
{
    NS_LOG_FUNCTION(this);
    m_timerExpiration.Cancel();
    m_intervalExpiration.Cancel();
    delete m_impl;
}

int64_t
TrickleTimer::AssignStreams(int64_t streamNum)
{
    m_uniRand->SetStream(streamNum);
    return 1;
}

void
TrickleTimer::SetParameters(Time minInterval, uint8_t doublings, uint16_t redundancy)
{
    NS_LOG_FUNCTION(this << minInterval << doublings << redundancy);
    NS_ASSERT_MSG(doublings < std::numeric_limits<decltype(m_ticks)>::digits,
                  "Doublings value is too large");

    m_minInterval = minInterval;
    m_ticks = 1;
    m_ticks <<= doublings;
    m_maxInterval = m_ticks * minInterval;
    m_redundancy = redundancy;
}

Time
TrickleTimer::GetMinInterval() const
{
    NS_LOG_FUNCTION(this);
    return m_minInterval;
}

Time
TrickleTimer::GetMaxInterval() const
{
    NS_LOG_FUNCTION(this);
    return m_maxInterval;
}

uint8_t
TrickleTimer::GetDoublings() const
{
    NS_LOG_FUNCTION(this);

    if (m_ticks == 0)
    {
        return 0;
    }

    return std::countr_zero(m_ticks);
}

uint16_t
TrickleTimer::GetRedundancy() const
{
    NS_LOG_FUNCTION(this);
    return m_redundancy;
}

Time
TrickleTimer::GetDelayLeft() const
{
    NS_LOG_FUNCTION(this);

    if (m_timerExpiration.IsPending())
    {
        return Simulator::GetDelayLeft(m_timerExpiration);
    }

    return TimeStep(0);
}

Time
TrickleTimer::GetIntervalLeft() const
{
    NS_LOG_FUNCTION(this);

    if (m_intervalExpiration.IsPending())
    {
        return Simulator::GetDelayLeft(m_intervalExpiration);
    }

    return TimeStep(0);
}

void
TrickleTimer::Enable()
{
    NS_LOG_FUNCTION(this);

    uint64_t randomInt;
    double random;

    NS_ASSERT_MSG(!m_minInterval.IsZero(), "Timer not initialized");

    randomInt = m_uniRand->GetInteger(1, m_ticks);
    random = randomInt;
    if (randomInt < m_ticks)
    {
        random += m_uniRand->GetValue(0, 1);
    }

    m_currentInterval = m_minInterval * random;
    m_intervalExpiration =
        Simulator::Schedule(m_currentInterval, &TrickleTimer::IntervalExpire, this);

    m_counter = 0;

    Time timerExpitation = m_uniRand->GetValue(0.5, 1) * m_currentInterval;
    m_timerExpiration = Simulator::Schedule(timerExpitation, &TrickleTimer::TimerExpire, this);
}

void
TrickleTimer::ConsistentEvent()
{
    NS_LOG_FUNCTION(this);
    m_counter++;
}

void
TrickleTimer::InconsistentEvent()
{
    NS_LOG_FUNCTION(this);
    if (m_currentInterval > m_minInterval)
    {
        Reset();
    }
}

void
TrickleTimer::Reset()
{
    NS_LOG_FUNCTION(this);

    m_currentInterval = m_minInterval;
    m_intervalExpiration.Cancel();
    m_timerExpiration.Cancel();

    m_intervalExpiration =
        Simulator::Schedule(m_currentInterval, &TrickleTimer::IntervalExpire, this);

    m_counter = 0;

    Time timerExpitation = m_uniRand->GetValue(0.5, 1) * m_currentInterval;
    m_timerExpiration = Simulator::Schedule(timerExpitation, &TrickleTimer::TimerExpire, this);
}

void
TrickleTimer::Stop()
{
    NS_LOG_FUNCTION(this);

    m_currentInterval = m_minInterval;
    m_intervalExpiration.Cancel();
    m_timerExpiration.Cancel();
    m_counter = 0;
}

void
TrickleTimer::TimerExpire()
{
    NS_LOG_FUNCTION(this);

    if (m_counter < m_redundancy || m_redundancy == 0)
    {
        m_impl->Invoke();
    }
}

void
TrickleTimer::IntervalExpire()
{
    NS_LOG_FUNCTION(this);

    m_currentInterval = m_currentInterval * 2;
    if (m_currentInterval > m_maxInterval)
    {
        m_currentInterval = m_maxInterval;
    }

    m_intervalExpiration =
        Simulator::Schedule(m_currentInterval, &TrickleTimer::IntervalExpire, this);

    m_counter = 0;

    Time timerExpitation = m_uniRand->GetValue(0.5, 1) * m_currentInterval;
    m_timerExpiration = Simulator::Schedule(timerExpitation, &TrickleTimer::TimerExpire, this);
}

} // namespace ns3
