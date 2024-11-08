/*
 * Copyright (c) 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include "synchronizer.h"

#include "log.h"

/**
 * @file
 * @ingroup realtime
 * ns3::Synchronizer implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Synchronizer");

NS_OBJECT_ENSURE_REGISTERED(Synchronizer);

TypeId
Synchronizer::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Synchronizer").SetParent<Object>().SetGroupName("Core");
    return tid;
}

Synchronizer::Synchronizer()
    : m_realtimeOriginNano(0),
      m_simOriginNano(0)
{
    NS_LOG_FUNCTION(this);
}

Synchronizer::~Synchronizer()
{
    NS_LOG_FUNCTION(this);
}

bool
Synchronizer::Realtime()
{
    NS_LOG_FUNCTION(this);
    return DoRealtime();
}

uint64_t
Synchronizer::GetCurrentRealtime()
{
    NS_LOG_FUNCTION(this);
    return NanosecondToTimeStep(DoGetCurrentRealtime());
}

void
Synchronizer::SetOrigin(uint64_t ts)
{
    NS_LOG_FUNCTION(this << ts);
    m_simOriginNano = TimeStepToNanosecond(ts);
    DoSetOrigin(m_simOriginNano);
}

uint64_t
Synchronizer::GetOrigin()
{
    NS_LOG_FUNCTION(this);
    return NanosecondToTimeStep(m_simOriginNano);
}

int64_t
Synchronizer::GetDrift(uint64_t ts)
{
    NS_LOG_FUNCTION(this << ts);
    int64_t tDrift = DoGetDrift(TimeStepToNanosecond(ts));

    if (tDrift < 0)
    {
        return -static_cast<int64_t>(NanosecondToTimeStep(-tDrift));
    }
    else
    {
        return static_cast<int64_t>(NanosecondToTimeStep(tDrift));
    }
}

bool
Synchronizer::Synchronize(uint64_t tsCurrent, uint64_t tsDelay)
{
    NS_LOG_FUNCTION(this << tsCurrent << tsDelay);
    return DoSynchronize(TimeStepToNanosecond(tsCurrent), TimeStepToNanosecond(tsDelay));
}

void
Synchronizer::Signal()
{
    NS_LOG_FUNCTION(this);
    DoSignal();
}

void
Synchronizer::SetCondition(bool cond)
{
    NS_LOG_FUNCTION(this << cond);
    DoSetCondition(cond);
}

void
Synchronizer::EventStart()
{
    NS_LOG_FUNCTION(this);
    DoEventStart();
}

uint64_t
Synchronizer::EventEnd()
{
    NS_LOG_FUNCTION(this);
    return NanosecondToTimeStep(DoEventEnd());
}

uint64_t
Synchronizer::TimeStepToNanosecond(uint64_t ts)
{
    NS_LOG_FUNCTION(this << ts);
    return TimeStep(ts).GetNanoSeconds();
}

uint64_t
Synchronizer::NanosecondToTimeStep(uint64_t ns)
{
    NS_LOG_FUNCTION(this << ns);
    return NanoSeconds(ns).GetTimeStep();
}

} // namespace ns3
