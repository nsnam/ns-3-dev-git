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

#include "boolean-probe.h"

#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/object.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BooleanProbe");

NS_OBJECT_ENSURE_REGISTERED(BooleanProbe);

TypeId
BooleanProbe::GetTypeId()
{
    static TypeId tid = TypeId("ns3::BooleanProbe")
                            .SetParent<Probe>()
                            .SetGroupName("Stats")
                            .AddConstructor<BooleanProbe>()
                            .AddTraceSource("Output",
                                            "The bool that serves as output for this probe",
                                            MakeTraceSourceAccessor(&BooleanProbe::m_output),
                                            "ns3::TracedValueCallback::Bool");
    return tid;
}

BooleanProbe::BooleanProbe()
{
    NS_LOG_FUNCTION(this);
    m_output = 0;
}

BooleanProbe::~BooleanProbe()
{
    NS_LOG_FUNCTION(this);
}

bool
BooleanProbe::GetValue() const
{
    NS_LOG_FUNCTION(this);
    return m_output;
}

void
BooleanProbe::SetValue(bool newVal)
{
    NS_LOG_FUNCTION(this << newVal);
    m_output = newVal;
}

void
BooleanProbe::SetValueByPath(std::string path, bool newVal)
{
    NS_LOG_FUNCTION(path << newVal);
    Ptr<BooleanProbe> probe = Names::Find<BooleanProbe>(path);
    NS_ASSERT_MSG(probe, "Error:  Can't find probe for path " << path);
    probe->SetValue(newVal);
}

bool
BooleanProbe::ConnectByObject(std::string traceSource, Ptr<Object> obj)
{
    NS_LOG_FUNCTION(this << traceSource << obj);
    NS_LOG_DEBUG("Name of probe (if any) in names database: " << Names::FindPath(obj));
    bool connected =
        obj->TraceConnectWithoutContext(traceSource,
                                        MakeCallback(&ns3::BooleanProbe::TraceSink, this));
    return connected;
}

void
BooleanProbe::ConnectByPath(std::string path)
{
    NS_LOG_FUNCTION(this << path);
    NS_LOG_DEBUG("Name of probe to search for in config database: " << path);
    Config::ConnectWithoutContext(path, MakeCallback(&ns3::BooleanProbe::TraceSink, this));
}

void
BooleanProbe::TraceSink(bool oldData, bool newData)
{
    NS_LOG_FUNCTION(this << oldData << newData);
    if (IsEnabled())
    {
        m_output = newData;
    }
}

} // namespace ns3
