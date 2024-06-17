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

#include "uinteger-32-probe.h"

#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/names.h"
#include "ns3/trace-source-accessor.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Uinteger32Probe");

NS_OBJECT_ENSURE_REGISTERED(Uinteger32Probe);

TypeId
Uinteger32Probe::GetTypeId()
{
    static TypeId tid = TypeId("ns3::Uinteger32Probe")
                            .SetParent<Probe>()
                            .SetGroupName("Stats")
                            .AddConstructor<Uinteger32Probe>()
                            .AddTraceSource("Output",
                                            "The uint32_t that serves as output for this probe",
                                            MakeTraceSourceAccessor(&Uinteger32Probe::m_output),
                                            "ns3::TracedValueCallback::Uint32");
    return tid;
}

Uinteger32Probe::Uinteger32Probe()
{
    NS_LOG_FUNCTION(this);
    m_output = 0;
}

Uinteger32Probe::~Uinteger32Probe()
{
    NS_LOG_FUNCTION(this);
}

uint32_t
Uinteger32Probe::GetValue() const
{
    NS_LOG_FUNCTION(this);
    return m_output;
}

void
Uinteger32Probe::SetValue(uint32_t newVal)
{
    NS_LOG_FUNCTION(this << newVal);
    m_output = newVal;
}

void
Uinteger32Probe::SetValueByPath(std::string path, uint32_t newVal)
{
    NS_LOG_FUNCTION(path << newVal);
    Ptr<Uinteger32Probe> probe = Names::Find<Uinteger32Probe>(path);
    NS_ASSERT_MSG(probe, "Error:  Can't find probe for path " << path);
    probe->SetValue(newVal);
}

bool
Uinteger32Probe::ConnectByObject(std::string traceSource, Ptr<Object> obj)
{
    NS_LOG_FUNCTION(this << traceSource << obj);
    NS_LOG_DEBUG("Name of probe (if any) in names database: " << Names::FindPath(obj));
    bool connected =
        obj->TraceConnectWithoutContext(traceSource,
                                        MakeCallback(&ns3::Uinteger32Probe::TraceSink, this));
    return connected;
}

void
Uinteger32Probe::ConnectByPath(std::string path)
{
    NS_LOG_FUNCTION(this << path);
    NS_LOG_DEBUG("Name of probe to search for in config database: " << path);
    Config::ConnectWithoutContext(path, MakeCallback(&ns3::Uinteger32Probe::TraceSink, this));
}

void
Uinteger32Probe::TraceSink(uint32_t oldData, uint32_t newData)
{
    NS_LOG_FUNCTION(this << oldData << newData);
    if (IsEnabled())
    {
        m_output = newData;
    }
}

} // namespace ns3
