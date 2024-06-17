/*
 * Copyright (c) 2011 Bucknell University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: L. Felipe Perrone (perrone@bucknell.edu)
 *          Tiago G. Rodrigues (tgr002@bucknell.edu)
 *
 * Modified by: Mitch Watrous (watrous@u.washington.edu)
 */

#include "double-probe.h"

#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/object.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("DoubleProbe");

NS_OBJECT_ENSURE_REGISTERED(DoubleProbe);

TypeId
DoubleProbe::GetTypeId()
{
    static TypeId tid = TypeId("ns3::DoubleProbe")
                            .SetParent<Probe>()
                            .SetGroupName("Stats")
                            .AddConstructor<DoubleProbe>()
                            .AddTraceSource("Output",
                                            "The double that serves as output for this probe",
                                            MakeTraceSourceAccessor(&DoubleProbe::m_output),
                                            "ns3::TracedValueCallback::Double");
    return tid;
}

DoubleProbe::DoubleProbe()
{
    NS_LOG_FUNCTION(this);
    m_output = 0;
}

DoubleProbe::~DoubleProbe()
{
    NS_LOG_FUNCTION(this);
}

double
DoubleProbe::GetValue() const
{
    NS_LOG_FUNCTION(this);
    return m_output;
}

void
DoubleProbe::SetValue(double newVal)
{
    NS_LOG_FUNCTION(this << newVal);
    m_output = newVal;
}

void
DoubleProbe::SetValueByPath(std::string path, double newVal)
{
    NS_LOG_FUNCTION(path << newVal);
    Ptr<DoubleProbe> probe = Names::Find<DoubleProbe>(path);
    NS_ASSERT_MSG(probe, "Error:  Can't find probe for path " << path);
    probe->SetValue(newVal);
}

bool
DoubleProbe::ConnectByObject(std::string traceSource, Ptr<Object> obj)
{
    NS_LOG_FUNCTION(this << traceSource << obj);
    NS_LOG_DEBUG("Name of trace source (if any) in names database: " << Names::FindPath(obj));
    bool connected =
        obj->TraceConnectWithoutContext(traceSource,
                                        MakeCallback(&ns3::DoubleProbe::TraceSink, this));
    return connected;
}

void
DoubleProbe::ConnectByPath(std::string path)
{
    NS_LOG_FUNCTION(this << path);
    NS_LOG_DEBUG("Name of trace source to search for in config database: " << path);
    Config::ConnectWithoutContext(path, MakeCallback(&ns3::DoubleProbe::TraceSink, this));
}

void
DoubleProbe::TraceSink(double oldData, double newData)
{
    NS_LOG_FUNCTION(this << oldData << newData);
    if (IsEnabled())
    {
        m_output = newData;
    }
}

} // namespace ns3
