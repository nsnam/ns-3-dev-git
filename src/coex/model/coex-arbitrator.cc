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
            m_coexEvents.insert(std::move(nh));
            it = next;
            continue;
        }
        ++it;
    }
}

} // namespace coex
} // namespace ns3
