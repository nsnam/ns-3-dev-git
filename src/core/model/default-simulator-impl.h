/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef DEFAULT_SIMULATOR_IMPL_H
#define DEFAULT_SIMULATOR_IMPL_H

#include "simulator-impl.h"

#include <list>
#include <mutex>
#include <thread>

/**
 * @file
 * @ingroup simulator
 * ns3::DefaultSimulatorImpl declaration.
 */

namespace ns3
{

// Forward
class Scheduler;

/**
 * @ingroup simulator
 *
 * The default single process simulator implementation.
 */
class DefaultSimulatorImpl : public SimulatorImpl
{
  public:
    /**
     *  Register this type.
     *  @return The object TypeId.
     */
    static TypeId GetTypeId();

    /** Constructor. */
    DefaultSimulatorImpl();
    /** Destructor. */
    ~DefaultSimulatorImpl() override;

    // Inherited
    void Destroy() override;
    bool IsFinished() const override;
    void Stop() override;
    EventId Stop(const Time& delay) override;
    EventId Schedule(const Time& delay, EventImpl* event) override;
    void ScheduleWithContext(uint32_t context, const Time& delay, EventImpl* event) override;
    EventId ScheduleNow(EventImpl* event) override;
    EventId ScheduleDestroy(EventImpl* event) override;
    void Remove(const EventId& id) override;
    void Cancel(const EventId& id) override;
    bool IsExpired(const EventId& id) const override;
    void Run() override;
    Time Now() const override;
    Time GetDelayLeft(const EventId& id) const override;
    Time GetMaximumSimulationTime() const override;
    void SetScheduler(ObjectFactory schedulerFactory) override;
    uint32_t GetSystemId() const override;
    uint32_t GetContext() const override;
    uint64_t GetEventCount() const override;

  private:
    void DoDispose() override;

    /** Process the next event. */
    void ProcessOneEvent();
    /** Move events from a different context into the main event queue. */
    void ProcessEventsWithContext();

    /** Wrap an event with its execution context. */
    struct EventWithContext
    {
        /** The event context. */
        uint32_t context;
        /** Event timestamp. */
        uint64_t timestamp;
        /** The event implementation. */
        EventImpl* event;
    };

    /** Container type for the events from a different context. */
    typedef std::list<EventWithContext> EventsWithContext;
    /** The container of events from a different context. */
    EventsWithContext m_eventsWithContext;
    /**
     * Flag \c true if all events with context have been moved to the
     * primary event queue.
     */
    bool m_eventsWithContextEmpty;
    /** Mutex to control access to the list of events with context. */
    std::mutex m_eventsWithContextMutex;

    /** Container type for the events to run at Simulator::Destroy() */
    typedef std::list<EventId> DestroyEvents;
    /** The container of events to run at Destroy. */
    DestroyEvents m_destroyEvents;
    /** Flag calling for the end of the simulation. */
    bool m_stop;
    /** The event priority queue. */
    Ptr<Scheduler> m_events;

    /** Next event unique id. */
    uint32_t m_uid;
    /** Unique id of the current event. */
    uint32_t m_currentUid;
    /** Timestamp of the current event. */
    uint64_t m_currentTs;
    /** Execution context of the current event. */
    uint32_t m_currentContext;
    /** The event count. */
    uint64_t m_eventCount;
    /**
     * Number of events that have been inserted but not yet scheduled,
     *  not counting the Destroy events; this is used for validation
     */
    int m_unscheduledEvents;

    /** Main execution thread. */
    std::thread::id m_mainThreadId;
};

} // namespace ns3

#endif /* DEFAULT_SIMULATOR_IMPL_H */
