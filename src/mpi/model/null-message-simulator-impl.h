/*
 *  Copyright 2013. Lawrence Livermore National Security, LLC.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Steven Smith <smith84@llnl.gov>
 */

/**
 * \file
 * \ingroup mpi
 * Declaration of class ns3::NullMessageSimulatorImpl.
 */

#ifndef NULLMESSAGE_SIMULATOR_IMPL_H
#define NULLMESSAGE_SIMULATOR_IMPL_H

#include <ns3/event-impl.h>
#include <ns3/ptr.h>
#include <ns3/scheduler.h>
#include <ns3/simulator-impl.h>

#include <fstream>
#include <iostream>
#include <list>

namespace ns3
{

class NullMessageEvent;
class NullMessageMpiInterface;
class RemoteChannelBundle;

/**
 * \ingroup mpi
 *
 * \brief Simulator implementation using MPI and a Null Message algorithm.
 */
class NullMessageSimulatorImpl : public SimulatorImpl
{
  public:
    /**
     *  Register this type.
     *  \return The object TypeId.
     */
    static TypeId GetTypeId();

    /** Default constructor. */
    NullMessageSimulatorImpl();

    /** Destructor. */
    ~NullMessageSimulatorImpl() override;

    // virtual from SimulatorImpl
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

    /**
     * \return singleton instance
     *
     * Singleton accessor.
     */
    static NullMessageSimulatorImpl* GetInstance();

  private:
    friend class NullMessageEvent;
    friend class NullMessageMpiInterface;
    friend class RemoteChannelBundleManager;

    /**
     * Non blocking receive of pending messages.
     */
    void HandleArrivingMessagesNonBlocking();

    /**
     * Blocking receive of arriving messages.
     */
    void HandleArrivingMessagesBlocking();

    void DoDispose() override;

    /**
     * Calculate the lookahead allowable for this MPI task.   Basically
     * the minimum latency on links to neighbor MPI tasks.
     */
    void CalculateLookAhead();

    /**
     * Process the next event on the queue.
     */
    void ProcessOneEvent();

    /**
     * \return next local event time.
     */
    Time Next() const;

    /**
     * Calculate the SafeTime. Should be called after message receives.
     */
    void CalculateSafeTime();

    /**
     * Get the current SafeTime; the maximum time that events can
     * be processed based on information received from neighboring
     * MPI tasks.
     * \return the current SafeTime
     */
    Time GetSafeTime();

    /**
     * \param bundle Bundle to schedule Null Message event for
     *
     * Schedule Null Message event for the specified RemoteChannelBundle.
     */
    void ScheduleNullMessageEvent(Ptr<RemoteChannelBundle> bundle);

    /**
     * \param bundle Bundle to reschedule Null Message event for
     *
     * Reschedule Null Message event for the specified
     * RemoteChannelBundle.  Existing event will be canceled.
     */
    void RescheduleNullMessageEvent(Ptr<RemoteChannelBundle> bundle);

    /**
     * \param nodeSysId SystemID to reschedule null event for
     *
     * Reschedule Null Message event for the RemoteChannelBundle to the
     * task nodeSysId.  Existing event will be canceled.
     */
    void RescheduleNullMessageEvent(uint32_t nodeSysId);

    /**
     * \param systemId SystemID to compute guarantee time for
     *
     * \return Guarantee time
     *
     * Calculate the guarantee time for incoming RemoteChannelBundle
     * from task nodeSysId.  No message should arrive from task
     * nodeSysId with a receive time less than the guarantee time.
     */
    Time CalculateGuaranteeTime(uint32_t systemId);

    /**
     * \param bundle remote channel bundle to schedule an event for.
     *
     * Null message event handler.   Scheduled to send a null message
     * for the specified bundle at regular intervals.   Will canceled
     * and rescheduled when packets are sent.
     */
    void NullMessageEventHandler(RemoteChannelBundle* bundle);

    /** Container type for the events to run at Simulator::Destroy(). */
    typedef std::list<EventId> DestroyEvents;

    /** The container of events to run at Destroy() */
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
     * not counting the "destroy" events; this is used for validation.
     */
    int m_unscheduledEvents;

    uint32_t m_myId;        /**< MPI rank. */
    uint32_t m_systemCount; /**< MPI communicator size. */

    /**
     * The time for which it is safe for this task to execute events
     * without danger of out-of-order events.
     */
    Time m_safeTime;

    /**
     * Null Message performance tuning parameter.  Controls when Null
     * messages are sent.  When value is 1 the minimum number of Null
     * messages are sent conserving bandwidth.  The delay in arrival of
     * lookahead information is the greatest resulting in maximum
     * unnecessary blocking of the receiver.  When the value is near 0
     * Null Messages are sent with high frequency, costing more
     * bandwidth and Null Message processing time, but there is minimum
     * unnecessary block of the receiver.
     */
    double m_schedulerTune;

    /** Singleton instance. */
    static NullMessageSimulatorImpl* g_instance;
};

} // namespace ns3

#endif /* NULLMESSAGE_SIMULATOR_IMPL_H */
