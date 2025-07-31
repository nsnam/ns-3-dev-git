/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#include "mock-event-generator.h"

#include "coex-arbitrator.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/string.h"

/**
 * @file
 * @ingroup coex
 * ns3::MockEventGenerator implementation.
 */

namespace ns3
{
namespace coex
{

NS_LOG_COMPONENT_DEFINE("MockEventGenerator");

NS_OBJECT_ENSURE_REGISTERED(MockEventGenerator);

TypeId
MockEventGenerator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::MockEventGenerator")
            .SetParent<coex::DeviceManager>()
            .SetGroupName("Coex")
            .AddConstructor<MockEventGenerator>()
            .AddAttribute(
                "InterEventTime",
                "Probability distribution of the time interval (in microseconds) between the "
                "generation of two consecutive coex events.",
                StringValue("ns3::ConstantRandomVariable[Constant=5000.0]"),
                MakePointerAccessor(&MockEventGenerator::SetInterEventTimeRv,
                                    &MockEventGenerator::GetInterEventTimeRv),
                MakePointerChecker<RandomVariableStream>())
            .AddAttribute("EventStartDelay",
                          "Probability distribution of the delay (in microseconds) between the "
                          "time the coex event is notified and the time the coex event starts.",
                          StringValue("ns3::ConstantRandomVariable[Constant=100.0]"),
                          MakePointerAccessor(&MockEventGenerator::m_eventStartDelay),
                          MakePointerChecker<RandomVariableStream>())
            .AddAttribute(
                "EventDuration",
                "Probability distribution of the duration (in microseconds) of a coex event.",
                StringValue("ns3::UniformRandomVariable[Min=500.0|Max=1500.0]"),
                MakePointerAccessor(&MockEventGenerator::m_eventDuration),
                MakePointerChecker<RandomVariableStream>());
    return tid;
}

MockEventGenerator::MockEventGenerator()
{
    NS_LOG_FUNCTION(this);
}

MockEventGenerator::~MockEventGenerator()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
MockEventGenerator::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    m_nextCoexEvent.Cancel();
    coex::DeviceManager::DoDispose();
}

void
MockEventGenerator::ResourceBusyStart(const Event& coexEvent)
{
    NS_LOG_FUNCTION(this << coexEvent);
    // do nothing
}

int64_t
MockEventGenerator::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(this << stream);
    const auto initStream = stream;
    m_interEventTime->SetStream(stream);
    m_eventStartDelay->SetStream(++stream);
    m_eventDuration->SetStream(++stream);
    return (++stream - initStream);
}

Ptr<RandomVariableStream>
MockEventGenerator::GetInterEventTimeRv() const
{
    return m_interEventTime;
}

void
MockEventGenerator::SetInterEventTimeRv(Ptr<RandomVariableStream> rv)
{
    NS_LOG_FUNCTION(this << rv);
    NS_ASSERT(rv);
    m_interEventTime = rv;

    // if an event is scheduled, we let it be executed; the next event will be scheduled by using
    // the updated inter event time.
    if (!m_nextCoexEvent.IsPending())
    {
        m_nextCoexEvent = Simulator::Schedule(MicroSeconds(m_interEventTime->GetValue()),
                                              &MockEventGenerator::GenerateCoexEvent,
                                              this);
    }
}

void
MockEventGenerator::GenerateCoexEvent()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_coexArbitrator);

    const auto event =
        Event{.type = EventType{}, // UNDEFINED
              .start = Simulator::Now() + MicroSeconds(m_eventStartDelay->GetValue()),
              .duration = MicroSeconds(m_eventDuration->GetValue()),
              .periodicity = Time{0}};

    m_coexArbitrator->RegisterProtection(event);
    m_nextCoexEvent = Simulator::Schedule(MicroSeconds(m_interEventTime->GetValue()),
                                          &MockEventGenerator::GenerateCoexEvent,
                                          this);
}

} // namespace coex
} // namespace ns3
