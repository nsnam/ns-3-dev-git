/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#include "time-data-calculators.h"

#include "ns3/log.h"
#include "ns3/nstime.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("TimeDataCalculators");

//--------------------------------------------------------------
//----------------------------------------------
TimeMinMaxAvgTotalCalculator::TimeMinMaxAvgTotalCalculator()
{
    NS_LOG_FUNCTION(this);

    m_count = 0;
}

TimeMinMaxAvgTotalCalculator::~TimeMinMaxAvgTotalCalculator()
{
    NS_LOG_FUNCTION(this);
}

/* static */
TypeId
TimeMinMaxAvgTotalCalculator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TimeMinMaxAvgTotalCalculator")
                            .SetParent<DataCalculator>()
                            .SetGroupName("Stats")
                            .AddConstructor<TimeMinMaxAvgTotalCalculator>();
    return tid;
}

void
TimeMinMaxAvgTotalCalculator::DoDispose()
{
    NS_LOG_FUNCTION(this);

    DataCalculator::DoDispose();
    // TimeMinMaxAvgTotalCalculator::DoDispose
}

void
TimeMinMaxAvgTotalCalculator::Update(const Time i)
{
    NS_LOG_FUNCTION(this << i);

    if (m_enabled)
    {
        if (m_count)
        {
            m_total += i;

            if (i < m_min)
            {
                m_min = i;
            }

            if (i > m_max)
            {
                m_max = i;
            }
        }
        else
        {
            m_min = i;
            m_max = i;
            m_total = i;
        }
        m_count++;
    }
    // end TimeMinMaxAvgTotalCalculator::Update
}

void
TimeMinMaxAvgTotalCalculator::Output(DataOutputCallback& callback) const
{
    NS_LOG_FUNCTION(this << &callback);

    callback.OutputSingleton(m_context, m_key + "-count", m_count);
    if (m_count > 0)
    {
        callback.OutputSingleton(m_context, m_key + "-total", m_total);
        callback.OutputSingleton(m_context, m_key + "-average", Time(m_total / m_count));
        callback.OutputSingleton(m_context, m_key + "-max", m_max);
        callback.OutputSingleton(m_context, m_key + "-min", m_min);
    }
    // end TimeMinMaxAvgTotalCalculator::Output
}
