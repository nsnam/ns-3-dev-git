/*
 * Copyright (c)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:  Juliana Freitag Borin, Flavio Kubota and Nelson L.
 * S. da Fonseca - wimaxgroup@lrc.ic.unicamp.br
 */

#include "ul-job.h"

#include <stdint.h>

namespace ns3
{

UlJob::UlJob()
    : m_deadline(),
      m_size(0)
{
}

UlJob::~UlJob()
{
}

SSRecord*
UlJob::GetSsRecord() const
{
    return m_ssRecord;
}

void
UlJob::SetSsRecord(SSRecord* ssRecord)
{
    m_ssRecord = ssRecord;
}

ServiceFlow::SchedulingType
UlJob::GetSchedulingType() const
{
    return m_schedulingType;
}

void
UlJob::SetSchedulingType(ServiceFlow::SchedulingType schedulingType)
{
    m_schedulingType = schedulingType;
}

ReqType
UlJob::GetType() const
{
    return m_type;
}

void
UlJob::SetType(ReqType type)
{
    m_type = type;
}

ServiceFlow*
UlJob::GetServiceFlow() const
{
    return m_serviceFlow;
}

void
UlJob::SetServiceFlow(ServiceFlow* serviceFlow)
{
    m_serviceFlow = serviceFlow;
}

Time
UlJob::GetReleaseTime() const
{
    return m_releaseTime;
}

void
UlJob::SetReleaseTime(Time releaseTime)
{
    m_releaseTime = releaseTime;
}

Time
UlJob::GetPeriod() const
{
    return m_period;
}

void
UlJob::SetPeriod(Time period)
{
    m_period = period;
}

Time
UlJob::GetDeadline() const
{
    return m_deadline;
}

void
UlJob::SetDeadline(Time deadline)
{
    m_deadline = deadline;
}

uint32_t
UlJob::GetSize() const
{
    return m_size;
}

void
UlJob::SetSize(uint32_t size)
{
    m_size = size;
}

/**
 * @brief equality operator
 * @param a first ULJob
 * @param b second ULJob
 * @returns true if equal
 */
bool
operator==(const UlJob& a, const UlJob& b)
{
    return a.GetServiceFlow() == b.GetServiceFlow() && a.GetSsRecord() == b.GetSsRecord();
}

PriorityUlJob::PriorityUlJob()
{
}

int
PriorityUlJob::GetPriority() const
{
    return m_priority;
}

void
PriorityUlJob::SetPriority(int priority)
{
    m_priority = priority;
}

Ptr<UlJob>
PriorityUlJob::GetUlJob() const
{
    return m_job;
}

void
PriorityUlJob::SetUlJob(Ptr<UlJob> job)
{
    m_job = job;
}

} // namespace ns3
