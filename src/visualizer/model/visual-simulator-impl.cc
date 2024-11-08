/*
 * Copyright (c) 2010 Gustavo Carneiro
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Gustavo Carneiro <gjcarneiro@gmail.com> <gjc@inescporto.pt>
 */
#include <Python.h>
#undef HAVE_SYS_STAT_H
#include "visual-simulator-impl.h"

#include "ns3/default-simulator-impl.h"
#include "ns3/log.h"
#include "ns3/packet-metadata.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("VisualSimulatorImpl");

NS_OBJECT_ENSURE_REGISTERED(VisualSimulatorImpl);

namespace
{
/**
 * Get an object factory configured to the default simulator implementation
 * @return an object factory.
 */
ObjectFactory
GetDefaultSimulatorImplFactory()
{
    ObjectFactory factory;
    factory.SetTypeId(DefaultSimulatorImpl::GetTypeId());
    return factory;
}
} // namespace

TypeId
VisualSimulatorImpl::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::VisualSimulatorImpl")
            .SetParent<SimulatorImpl>()
            .SetGroupName("Visualizer")
            .AddConstructor<VisualSimulatorImpl>()
            .AddAttribute(
                "SimulatorImplFactory",
                "Factory for the underlying simulator implementation used by the visualizer.",
                ObjectFactoryValue(GetDefaultSimulatorImplFactory()),
                MakeObjectFactoryAccessor(&VisualSimulatorImpl::m_simulatorImplFactory),
                MakeObjectFactoryChecker());
    return tid;
}

VisualSimulatorImpl::VisualSimulatorImpl()
{
    PacketMetadata::Enable();
}

VisualSimulatorImpl::~VisualSimulatorImpl()
{
}

void
VisualSimulatorImpl::DoDispose()
{
    if (m_simulator)
    {
        m_simulator->Dispose();
        m_simulator = nullptr;
    }
    SimulatorImpl::DoDispose();
}

void
VisualSimulatorImpl::NotifyConstructionCompleted()
{
    m_simulator = m_simulatorImplFactory.Create<SimulatorImpl>();
}

void
VisualSimulatorImpl::Destroy()
{
    m_simulator->Destroy();
}

void
VisualSimulatorImpl::SetScheduler(ObjectFactory schedulerFactory)
{
    m_simulator->SetScheduler(schedulerFactory);
}

// System ID for non-distributed simulation is always zero
uint32_t
VisualSimulatorImpl::GetSystemId() const
{
    return m_simulator->GetSystemId();
}

bool
VisualSimulatorImpl::IsFinished() const
{
    return m_simulator->IsFinished();
}

void
VisualSimulatorImpl::Run()
{
    if (!Py_IsInitialized())
    {
        Py_Initialize();
        PyRun_SimpleString("import visualizer\n"
                           "visualizer.start();\n");
    }
    else
    {
        PyGILState_STATE __py_gil_state = PyGILState_Ensure();

        PyRun_SimpleString("import visualizer\n"
                           "visualizer.start();\n");

        PyGILState_Release(__py_gil_state);
    }
}

void
VisualSimulatorImpl::Stop()
{
    m_simulator->Stop();
}

EventId
VisualSimulatorImpl::Stop(const Time& delay)
{
    return m_simulator->Stop(delay);
}

//
// Schedule an event for a _relative_ time in the future.
//
EventId
VisualSimulatorImpl::Schedule(const Time& delay, EventImpl* event)
{
    return m_simulator->Schedule(delay, event);
}

void
VisualSimulatorImpl::ScheduleWithContext(uint32_t context, const Time& delay, EventImpl* event)
{
    m_simulator->ScheduleWithContext(context, delay, event);
}

EventId
VisualSimulatorImpl::ScheduleNow(EventImpl* event)
{
    return m_simulator->ScheduleNow(event);
}

EventId
VisualSimulatorImpl::ScheduleDestroy(EventImpl* event)
{
    return m_simulator->ScheduleDestroy(event);
}

Time
VisualSimulatorImpl::Now() const
{
    return m_simulator->Now();
}

Time
VisualSimulatorImpl::GetDelayLeft(const EventId& id) const
{
    return m_simulator->GetDelayLeft(id);
}

void
VisualSimulatorImpl::Remove(const EventId& id)
{
    m_simulator->Remove(id);
}

void
VisualSimulatorImpl::Cancel(const EventId& id)
{
    m_simulator->Cancel(id);
}

bool
VisualSimulatorImpl::IsExpired(const EventId& id) const
{
    return m_simulator->IsExpired(id);
}

Time
VisualSimulatorImpl::GetMaximumSimulationTime() const
{
    return m_simulator->GetMaximumSimulationTime();
}

uint32_t
VisualSimulatorImpl::GetContext() const
{
    return m_simulator->GetContext();
}

uint64_t
VisualSimulatorImpl::GetEventCount() const
{
    return m_simulator->GetEventCount();
}

void
VisualSimulatorImpl::RunRealSimulator()
{
    m_simulator->Run();
}

} // namespace ns3
