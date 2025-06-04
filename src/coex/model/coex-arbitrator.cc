/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#include "coex-arbitrator.h"

#include "device-coex-manager.h"

#include "ns3/log.h"
#include "ns3/net-device.h"
#include "ns3/node.h"
#include "ns3/simulator.h"

/**
 * @file
 * @ingroup coex
 * ns3::coex::Arbitrator implementation.
 */

namespace ns3
{
namespace coex
{

NS_LOG_COMPONENT_DEFINE("CoexArbitrator");

NS_OBJECT_ENSURE_REGISTERED(Arbitrator);

TypeId
Arbitrator::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::coex::Arbitrator")
            .SetParent<Object>()
            .SetGroupName("Coex")
            .AddTraceSource(
                "CoexEvent",
                "Trace source providing all the coex events notified to this arbitrator.",
                MakeTraceSourceAccessor(&Arbitrator::m_coexEventTrace),
                "ns3::CoexArbitrator::CoexEventTraceCb");
    return tid;
}

Arbitrator::Arbitrator()
{
    NS_LOG_FUNCTION_NOARGS();
}

Arbitrator::~Arbitrator()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
Arbitrator::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    m_node = nullptr;
    for (const auto& [rat, coexManager] : m_devCoexManagers)
    {
        coexManager->Dispose();
    }
    m_devCoexManagers.clear();
    m_notifyEvent.Cancel();
    Object::DoDispose();
}

void
Arbitrator::NotifyNewAggregate()
{
    NS_LOG_FUNCTION(this);
    if (!m_node)
    {
        // verify that a Node has been aggregated
        if (auto node = this->GetObject<Node>())
        {
            m_node = node;
        }
    }
    Object::NotifyNewAggregate();
}

void
Arbitrator::SetDeviceCoexManager(Rat rat, Ptr<DeviceManager> coexManager)
{
    NS_LOG_FUNCTION(this << rat << coexManager);

    m_devCoexManagers[rat] = coexManager;
    coexManager->SetCoexArbitrator(this);
}

Ptr<DeviceManager>
Arbitrator::GetDeviceCoexManager(Rat rat) const
{
    if (const auto it = m_devCoexManagers.find(rat); it != m_devCoexManagers.cend())
    {
        return it->second;
    }
    return nullptr;
}

std::optional<eventId_t>
Arbitrator::RegisterProtection(const Event& coexEvent)
{
    NS_LOG_FUNCTION(this << coexEvent);

    const auto rat = coexEvent.type.GetRat();
    NS_ASSERT_MSG(m_devCoexManagers.contains(rat), "No registered device for RAT " << rat);

    if (coexEvent.periodicity.IsStrictlyNegative() ||
        (coexEvent.periodicity.IsStrictlyPositive() && coexEvent.duration > coexEvent.periodicity))
    {
        NS_LOG_DEBUG("Invalid combination of period (" << coexEvent.periodicity.As(Time::US)
                                                       << ") and duration ("
                                                       << coexEvent.duration.As(Time::US) << ")");
        return std::nullopt;
    }

    const auto [it, inserted] =
        m_coexEvents.emplace(CoexEventElem{.id = m_nextCoexEvId, .coexEvent = coexEvent});
    NS_ASSERT(inserted);

    UpdateCoexEvents();

    // if the coex event is not valid (i.e., it is already past), it is removed from the list

    for (auto it = m_coexEvents.cbegin(); it != m_coexEvents.cend(); ++it)
    {
        if (it->id == m_nextCoexEvId)
        {
            if (IsRequestAccepted(it->coexEvent))
            {
                ++m_nextCoexEvId;
                m_coexEventTrace(coexEvent);
                return it->id;
            }
            else
            {
                m_coexEvents.erase(it);
                break;
            }
        }
    }

    return std::nullopt;
}

void
Arbitrator::CancelCoexEvent(eventId_t coexEvId)
{
    for (auto it = m_coexEvents.cbegin(); it != m_coexEvents.cend(); ++it)
    {
        if (it->id == coexEvId)
        {
            m_coexEvents.erase(it);
            return;
        }
    }

    UpdateCoexEvents();
}

const Arbitrator::Events&
Arbitrator::GetCoexEvents() const
{
    return m_coexEvents;
}

std::optional<Time>
Arbitrator::GetCurrOrNextEventStart()
{
    UpdateCoexEvents();

    if (m_coexEvents.empty())
    {
        return std::nullopt;
    }

    const auto& head = m_coexEvents.cbegin()->coexEvent;
    NS_ASSERT_MSG(head.start + head.duration > Simulator::Now(),
                  "Returned coex event (" << head << ") has already ended");
    return head.start;
}

void
Arbitrator::UpdateCoexEvents()
{
    NS_LOG_FUNCTION(this);
    const auto now = Simulator::Now();

    for (auto it = m_coexEvents.begin(); it != m_coexEvents.end();)
    {
        if (now >= it->coexEvent.start + it->coexEvent.duration)
        {
            if (it->coexEvent.periodicity.IsZero())
            {
                // past occurrence of a non-periodic event, just remove it
                it = m_coexEvents.erase(it);
                continue;
            }
            // past occurrence of a periodic event, update the start time
            NS_ASSERT_MSG((now - it->coexEvent.start) <= it->coexEvent.periodicity,
                          "More than a period elapsed since the last coex event start");
            auto next = std::next(it);
            auto nh = m_coexEvents.extract(it);
            nh.value().coexEvent.start += nh.value().coexEvent.periodicity;
            nh.value().notified = false;
            m_coexEvents.insert(std::move(nh));
            it = next;
            continue;
        }
        ++it;
    }

    ScheduleNotificationIfNeeded();
}

void
Arbitrator::ScheduleNotificationIfNeeded()
{
    NS_LOG_FUNCTION(this);

    const auto now = Simulator::Now();
    auto coexElemIt = m_coexEvents.cbegin();

    // skip coex events that have been already notified
    while (coexElemIt != m_coexEvents.cend() && coexElemIt->notified)
    {
        ++coexElemIt;
    }

    if (coexElemIt == m_coexEvents.cend())
    {
        m_notifyEvent.Cancel();
        return;
    }

    const auto remTime = coexElemIt->coexEvent.start - now;

    if (m_notifyEvent.IsPending() && Simulator::GetDelayLeft(m_notifyEvent) == remTime)
    {
        return; // nothing to do
    }

    m_notifyEvent.Cancel();
    NS_LOG_DEBUG("Schedule resource busy start at " << coexElemIt->coexEvent.start.As(Time::MS));
    m_notifyEvent = Simulator::Schedule(remTime, &Arbitrator::NotifyResourceBusyStart, this);
}

void
Arbitrator::NotifyResourceBusyStart()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT_MSG(
        !m_coexEvents.empty() && m_coexEvents.cbegin()->coexEvent.start == Simulator::Now(),
        "NotifyResourceBusyStart is expected to be called at the start of a resource busy period");

    // notify all coex managers but the one associated with the device using the resource
    for (const auto& [rat, coexManager] : m_devCoexManagers)
    {
        if (rat != m_coexEvents.cbegin()->coexEvent.type.GetRat())
        {
            coexManager->ResourceBusyStart(m_coexEvents.cbegin()->coexEvent);
        }
    }

    // mark the event as notified
    auto nh = m_coexEvents.extract(m_coexEvents.begin());
    nh.value().notified = true;
    m_coexEvents.insert(std::move(nh));

    // schedule a new notification if needed by calling UpdateCoexEvents
    UpdateCoexEvents();
}

} // namespace coex
} // namespace ns3
