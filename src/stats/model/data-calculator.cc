/*
 * Copyright (c) 2008 Drexel University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Joe Kopena (tjkopena@cs.drexel.edu)
 */

#include "data-calculator.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("DataCalculator");

// NS_DEPRECATED_3_44
const double ns3::NaN = std::nan("");

//--------------------------------------------------------------
//----------------------------------------------
DataCalculator::DataCalculator()
    : m_enabled(true)
{
    NS_LOG_FUNCTION(this);
}

DataCalculator::~DataCalculator()
{
    NS_LOG_FUNCTION(this);
}

/* static */
TypeId
DataCalculator::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DataCalculator").SetParent<Object>().SetGroupName("Stats")
        // No AddConstructor because this is an abstract class.
        ;
    return tid;
}

void
DataCalculator::DoDispose()
{
    NS_LOG_FUNCTION(this);

    Simulator::Cancel(m_startEvent);
    Simulator::Cancel(m_stopEvent);

    Object::DoDispose();
    // DataCalculator::DoDispose
}

//----------------------------------------------
void
DataCalculator::SetKey(const std::string key)
{
    NS_LOG_FUNCTION(this << key);

    m_key = key;
    // end DataCalculator::SetKey
}

std::string
DataCalculator::GetKey() const
{
    NS_LOG_FUNCTION(this);

    return m_key;
    // end DataCalculator::GetKey
}

//----------------------------------------------
void
DataCalculator::SetContext(const std::string context)
{
    NS_LOG_FUNCTION(this << context);

    m_context = context;
    // end DataCalculator::SetContext
}

std::string
DataCalculator::GetContext() const
{
    NS_LOG_FUNCTION(this);

    return m_context;
    // end DataCalculator::GetContext
}

//----------------------------------------------
void
DataCalculator::Enable()
{
    NS_LOG_FUNCTION(this);

    m_enabled = true;
    // end DataCalculator::Enable
}

void
DataCalculator::Disable()
{
    NS_LOG_FUNCTION(this);

    m_enabled = false;
    // end DataCalculator::Disable
}

bool
DataCalculator::GetEnabled() const
{
    NS_LOG_FUNCTION(this);

    return m_enabled;
    // end DataCalculator::GetEnabled
}

//----------------------------------------------
void
DataCalculator::Start(const Time& startTime)
{
    NS_LOG_FUNCTION(this << startTime);

    m_startEvent = Simulator::Schedule(startTime, &DataCalculator::Enable, this);

    // end DataCalculator::Start
}

void
DataCalculator::Stop(const Time& stopTime)
{
    NS_LOG_FUNCTION(this << stopTime);

    m_stopEvent = Simulator::Schedule(stopTime, &DataCalculator::Disable, this);
    // end DataCalculator::Stop
}
