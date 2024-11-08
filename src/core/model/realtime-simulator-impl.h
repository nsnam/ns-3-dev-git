/*
 * Copyright (c) 2008 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef REALTIME_SIMULATOR_IMPL_H
#define REALTIME_SIMULATOR_IMPL_H

#include "assert.h"
#include "event-impl.h"
#include "log.h"
#include "ptr.h"
#include "scheduler.h"
#include "simulator-impl.h"
#include "synchronizer.h"

#include <list>
#include <mutex>
#include <thread>

/**
 * @file
 * @ingroup realtime
 * ns3::RealtimeSimulatorImpl declaration.
 */

namespace ns3
{

/**
 * @ingroup simulator
 * @defgroup realtime Realtime Simulator
 *
 * Realtime simulator implementation.
 */

/**
 * @ingroup realtime
 *
 * Realtime version of SimulatorImpl.
 */
class RealtimeSimulatorImpl : public SimulatorImpl
{
  public:
    /**
     * Get the registered TypeId for this class.
     * @returns The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * What to do when we can't maintain real time synchrony.
     */
    enum SynchronizationMode
    {
        /**
         * Make a best effort to keep synced to real-time.
         *
         * If we fall behind, keep going.
         */
        SYNC_BEST_EFFORT,
        /**
         * Keep to real time within the hard limit tolerance configured
         * with SetHardLimit, or die trying.
         *
         * Falling behind by more than the hard limit tolerance triggers
         * a fatal error.
         * @see SetHardLimit
         */
        SYNC_HARD_LIMIT,
    };

    /** Constructor. */
    RealtimeSimulatorImpl();
    /** Destructor. */
    ~RealtimeSimulatorImpl() override;

    // Inherited from SimulatorImpl
    void Destroy() override;
    bool IsFinished() const override;
    void Stop() override;
    EventId Stop(const Time& delay) override;
    EventId Schedule(const Time& delay, EventImpl* event) override;
    void ScheduleWithContext(uint32_t context, const Time& delay, EventImpl* event) override;
    EventId ScheduleNow(EventImpl* event) override;
    EventId ScheduleDestroy(EventImpl* event) override;
    void Remove(const EventId& ev) override;
    void Cancel(const EventId& ev) override;
    bool IsExpired(const EventId& ev) const override;
    void Run() override;
    Time Now() const override;
    Time GetDelayLeft(const EventId& id) const override;
    Time GetMaximumSimulationTime() const override;
    void SetScheduler(ObjectFactory schedulerFactory) override;
    uint32_t GetSystemId() const override;
    uint32_t GetContext() const override;
    uint64_t GetEventCount() const override;

    /** @copydoc ScheduleWithContext(uint32_t,const Time&,EventImpl*) */
    void ScheduleRealtimeWithContext(uint32_t context, const Time& delay, EventImpl* event);
    /**
     * Schedule a future event execution (in the same context).
     *
     * @param [in] delay Delay until the event expires.
     * @param [in] event The event to schedule.
     */
    void ScheduleRealtime(const Time& delay, EventImpl* event);
    /**
     * Schedule an event to run at the current virtual time.
     *
     * @param [in] context Event context.
     * @param [in] event The event to schedule.
     */
    void ScheduleRealtimeNowWithContext(uint32_t context, EventImpl* event);
    /**
     * Schedule an event to run at the current virtual time.
     *
     * @param [in] event The event to schedule.
     */
    void ScheduleRealtimeNow(EventImpl* event);
    /**
     * Get the current real time from the synchronizer.
     * @returns The current real time.
     */
    Time RealtimeNow() const;

    /**
     * Set the SynchronizationMode.
     *
     * @param [in] mode The new SynchronizationMode.
     */
    void SetSynchronizationMode(RealtimeSimulatorImpl::SynchronizationMode mode);
    /**
     * Get the SynchronizationMode.
     * @returns The current SynchronizationMode.
     */
    RealtimeSimulatorImpl::SynchronizationMode GetSynchronizationMode() const;

    /**
     * Set the fatal error threshold for SynchronizationMode SYNC_HARD_LIMIT.
     *
     * @param [in] limit The maximum amount of real time we are allowed to fall
     *     behind before we trigger a fatal error.
     */
    void SetHardLimit(Time limit);
    /**
     * Get the current fatal error threshold for SynchronizationMode
     * SYNC_HARD_LIMIT.
     *
     * @returns The hard limit threshold.
     */
    Time GetHardLimit() const;

  private:
    /**
     * Is the simulator running?
     * @returns \c true if we are running.
     */
    bool Running() const;
    /**
     * Check that the Synchronizer is locked to the real time clock.
     * @return \c true if the Synchronizer is locked.
     */
    bool Realtime() const;
    /**
     * Get the timestep of the next event.
     * @returns The timestep of the next event.
     */
    uint64_t NextTs() const;
    /** Process the next event. */
    void ProcessOneEvent();
    /** Destructor implementation. */
    void DoDispose() override;

    /** Container type for events to be run at destroy time. */
    typedef std::list<EventId> DestroyEvents;
    /** Container for events to be run at destroy time. */
    DestroyEvents m_destroyEvents;
    /** Has the stopping condition been reached? */
    bool m_stop;
    /** Is the simulator currently running. */
    bool m_running;

    /**
     * @name Mutex-protected variables.
     *
     * These variables are protected by #m_mutex.
     */
    /**@{*/
    /** The event list. */
    Ptr<Scheduler> m_events;
    /**< Number of events in the event list. */
    int m_unscheduledEvents;
    /**< Unique id for the next event to be scheduled. */
    uint32_t m_uid;
    /**< Unique id of the current event. */
    uint32_t m_currentUid;
    /**< Timestep of the current event. */
    uint64_t m_currentTs;
    /**< Execution context. */
    uint32_t m_currentContext;
    /** The event count. */
    uint64_t m_eventCount;
    /**@}*/

    /** Mutex to control access to key state. */
    mutable std::mutex m_mutex;

    /** The synchronizer in use to track real time. */
    Ptr<Synchronizer> m_synchronizer;

    /** SynchronizationMode policy. */
    SynchronizationMode m_synchronizationMode;

    /** The maximum allowable drift from real-time in SYNC_HARD_LIMIT mode. */
    Time m_hardLimit;

    /** Main thread. */
    std::thread::id m_main;
};

} // namespace ns3

#endif /* REALTIME_SIMULATOR_IMPL_H */
