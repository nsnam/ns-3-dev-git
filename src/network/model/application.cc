/*
 * Copyright (c) 2006 Georgia Tech Research Corporation
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: George F. Riley<riley@ece.gatech.edu>
 */

// Implementation for ns3 Application base class.
// George F. Riley, Georgia Tech, Fall 2006

#include "application.h"

#include "node.h"

#include "ns3/log.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Application");

NS_OBJECT_ENSURE_REGISTERED(Application);

// Application Methods

TypeId
Application::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Application")
                            .SetParent<Object>()
                            .SetGroupName("Network")
                            .AddAttribute("StartTime",
                                          "Time at which the application will start",
                                          TimeValue(Seconds(0)),
                                          MakeTimeAccessor(&Application::m_startTime),
                                          MakeTimeChecker())
                            .AddAttribute("StopTime",
                                          "Time at which the application will stop",
                                          TimeValue(TimeStep(0)),
                                          MakeTimeAccessor(&Application::m_stopTime),
                                          MakeTimeChecker());
    return tid;
}

// \brief Application Constructor
Application::Application()
{
    NS_LOG_FUNCTION(this);
}

// \brief Application Destructor
Application::~Application()
{
    NS_LOG_FUNCTION(this);
}

void
Application::SetStartTime(Time start)
{
    NS_LOG_FUNCTION(this << start);
    m_startTime = start;
}

void
Application::SetStopTime(Time stop)
{
    NS_LOG_FUNCTION(this << stop);
    m_stopTime = stop;
}

void
Application::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_node = nullptr;
    m_startEvent.Cancel();
    m_stopEvent.Cancel();
    Object::DoDispose();
}

void
Application::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    m_startEvent = Simulator::Schedule(m_startTime, &Application::StartApplication, this);
    if (m_stopTime != TimeStep(0))
    {
        m_stopEvent = Simulator::Schedule(m_stopTime, &Application::StopApplication, this);
    }
    Object::DoInitialize();
}

Ptr<Node>
Application::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
Application::SetNode(Ptr<Node> node)
{
    NS_LOG_FUNCTION(this);
    m_node = node;
}

// Protected methods
// StartApp and StopApp will likely be overridden by application subclasses
void
Application::StartApplication()
{ // Provide null functionality in case subclass is not interested
    NS_LOG_FUNCTION(this);
}

void
Application::StopApplication()
{ // Provide null functionality in case subclass is not interested
    NS_LOG_FUNCTION(this);
}

int64_t
Application::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    return 0;
}

} // namespace ns3
