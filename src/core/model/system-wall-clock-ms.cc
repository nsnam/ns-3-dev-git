/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage.inria.fr>
 */

#include "system-wall-clock-ms.h"

#include "log.h"

#include <chrono>

/**
 * @file
 * @ingroup system
 * ns3::SystemWallClockMs and ns3::SystemWallClockMsPrivate implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("SystemWallClockMs");

/**
 * @ingroup system
 * @brief System-dependent implementation for SystemWallClockMs
 */
class SystemWallClockMsPrivate
{
  public:
    /** @copydoc SystemWallClockMs::Start() */
    void Start();
    /** @copydoc SystemWallClockMs::End() */
    int64_t End();
    /** @copydoc SystemWallClockMs::GetElapsedReal() */
    int64_t GetElapsedReal() const;
    /** @copydoc SystemWallClockMs::GetElapsedUser() */
    int64_t GetElapsedUser() const;
    /** @copydoc SystemWallClockMs::GetElapsedSystem() */
    int64_t GetElapsedSystem() const;

  private:
    std::chrono::system_clock::time_point m_startTime; //!< The wall clock start time.
    int64_t m_elapsedReal;                             //!< Elapsed real time, in ms.
    int64_t m_elapsedUser;                             //!< Elapsed user time, in ms.
    int64_t m_elapsedSystem;                           //!< Elapsed system time, in ms.
};

void
SystemWallClockMsPrivate::Start()
{
    NS_LOG_FUNCTION(this);
    m_startTime = std::chrono::system_clock::now();
}

int64_t
SystemWallClockMsPrivate::End()
{
    //
    // We need to return the number of milliseconds that have elapsed in some
    // reasonably portable way.  The underlying function that we will use returns
    // a number of elapsed ticks.  We can look up the number of ticks per second
    // from the system configuration.
    //
    // Conceptually, we need to find the number of elapsed clock ticks and then
    // multiply the result by the milliseconds per clock tick (or just as easily
    // divide by clock ticks per millisecond).  Integer dividing by clock ticks
    // per millisecond is bad since this number is fractional on most machines
    // and would result in divide by zero errors due to integer rounding.
    //
    // Multiplying by milliseconds per clock tick works up to a clock resolution
    // of 1000 ticks per second.  If we go  past this point, we begin to get zero
    // elapsed times when millisecondsPerTick becomes fractional and another
    // rounding error appears.
    //
    // So rounding errors using integers can bite you from two direction.  Since
    // all of our targets have math coprocessors, why not just use doubles
    // internally?  Works fine, lasts a long time.
    //
    // If millisecondsPerTick becomes fractional, and an elapsed time greater than
    // a millisecond is measured, the function will work as expected.  If an elapsed
    // time is measured that turns out to be less than a millisecond, we'll just
    // return zero which would, I think, also will be expected.
    //
    NS_LOG_FUNCTION(this);

    auto endTime = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = endTime - m_startTime;
    m_elapsedReal = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_seconds).count();

    //
    // Nothing like this in MinGW, for example.
    //
    m_elapsedUser = 0;
    m_elapsedSystem = 0;

    return m_elapsedReal;
}

int64_t
SystemWallClockMsPrivate::GetElapsedReal() const
{
    NS_LOG_FUNCTION(this);
    return m_elapsedReal;
}

int64_t
SystemWallClockMsPrivate::GetElapsedUser() const
{
    NS_LOG_FUNCTION(this);
    return m_elapsedUser;
}

int64_t
SystemWallClockMsPrivate::GetElapsedSystem() const
{
    NS_LOG_FUNCTION(this);
    return m_elapsedSystem;
}

SystemWallClockMs::SystemWallClockMs()
    : m_priv(new SystemWallClockMsPrivate())
{
    NS_LOG_FUNCTION(this);
}

SystemWallClockMs::~SystemWallClockMs()
{
    NS_LOG_FUNCTION(this);
    delete m_priv;
    m_priv = nullptr;
}

void
SystemWallClockMs::Start()
{
    NS_LOG_FUNCTION(this);
    m_priv->Start();
}

int64_t
SystemWallClockMs::End()
{
    NS_LOG_FUNCTION(this);
    return m_priv->End();
}

int64_t
SystemWallClockMs::GetElapsedReal() const
{
    NS_LOG_FUNCTION(this);
    return m_priv->GetElapsedReal();
}

int64_t
SystemWallClockMs::GetElapsedUser() const
{
    NS_LOG_FUNCTION(this);
    return m_priv->GetElapsedUser();
}

int64_t
SystemWallClockMs::GetElapsedSystem() const
{
    NS_LOG_FUNCTION(this);
    return m_priv->GetElapsedSystem();
}

} // namespace ns3
