/*
 * Copyright (c) 2011 Bucknell University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: L. Felipe Perrone (perrone@bucknell.edu)
 *          Tiago G. Rodrigues (tgr002@bucknell.edu)
 *
 * Modified by: Mitch Watrous (watrous@u.washington.edu)
 *
 */

#include "time-probe.h"

#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/object.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("TimeProbe");

NS_OBJECT_ENSURE_REGISTERED(TimeProbe);

TypeId
TimeProbe::GetTypeId()
{
    static TypeId tid = TypeId("ns3::TimeProbe")
                            .SetParent<Probe>()
                            .SetGroupName("Stats")
                            .AddConstructor<TimeProbe>()
                            .AddTraceSource("Output",
                                            "The double valued (units of seconds) probe output",
                                            MakeTraceSourceAccessor(&TimeProbe::m_output),
                                            "ns3::TracedValueCallback::Double");
    return tid;
}

TimeProbe::TimeProbe()
{
    NS_LOG_FUNCTION(this);
    m_output = 0;
}

TimeProbe::~TimeProbe()
{
    NS_LOG_FUNCTION(this);
}

double
TimeProbe::GetValue() const
{
    NS_LOG_FUNCTION(this);
    return m_output;
}

void
TimeProbe::SetValue(Time newVal)
{
    NS_LOG_FUNCTION(this << newVal.As(Time::S));
    m_output = newVal.GetSeconds();
}

void
TimeProbe::SetValueByPath(std::string path, Time newVal)
{
    NS_LOG_FUNCTION(path << newVal.As(Time::S));
    Ptr<TimeProbe> probe = Names::Find<TimeProbe>(path);
    NS_ASSERT_MSG(probe, "Error:  Can't find probe for path " << path);
    probe->SetValue(newVal);
}

bool
TimeProbe::ConnectByObject(std::string traceSource, Ptr<Object> obj)
{
    NS_LOG_FUNCTION(this << traceSource << obj);
    NS_LOG_DEBUG("Name of trace source (if any) in names database: " << Names::FindPath(obj));
    bool connected =
        obj->TraceConnectWithoutContext(traceSource,
                                        MakeCallback(&ns3::TimeProbe::TraceSink, this));
    return connected;
}

void
TimeProbe::ConnectByPath(std::string path)
{
    NS_LOG_FUNCTION(this << path);
    NS_LOG_DEBUG("Name of trace source to search for in config database: " << path);
    Config::ConnectWithoutContext(path, MakeCallback(&ns3::TimeProbe::TraceSink, this));
}

void
TimeProbe::TraceSink(Time oldData, Time newData)
{
    NS_LOG_FUNCTION(this << oldData.As(Time::S) << newData.As(Time::S));
    if (IsEnabled())
    {
        m_output = newData.GetSeconds();
    }
}

} // namespace ns3
