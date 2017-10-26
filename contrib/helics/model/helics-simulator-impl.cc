/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "helics-simulator-impl.h"

#include "ns3/simulator.h"
#include "ns3/scheduler.h"
#include "ns3/event-impl.h"

#include "ns3/ptr.h"
#include "ns3/pointer.h"
#include "ns3/assert.h"
#include "ns3/log.h"

#include <cmath>

// HELICS model and helpers
#include "helics.h"
#include "ns3/helics-helper.h"

/**
 * \file
 * \ingroup simulator
 * ns3::HelicsSimulatorImpl implementation.
 */

namespace ns3 {

// Note:  Logging in this file is largely avoided due to the
// number of calls that are made to these functions and the possibility
// of causing recursions leading to stack overflow
NS_LOG_COMPONENT_DEFINE ("HelicsSimulatorImpl");

NS_OBJECT_ENSURE_REGISTERED (HelicsSimulatorImpl);

TypeId
HelicsSimulatorImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HelicsSimulatorImpl")
    .SetParent<SimulatorImpl> ()
    .SetGroupName ("Core")
    .AddConstructor<HelicsSimulatorImpl> ()
  ;
  return tid;
}

HelicsSimulatorImpl::HelicsSimulatorImpl ()
{
  NS_LOG_FUNCTION (this);

  // Check if HELICS federate has already been setup
  if (federate == nullptr)
  {
    // Create helics federate with connection to broker
    NS_LOG_INFO ("Creating federate");
    HelicsHelper helics;
    helics.SetupFederate ();
  }

  m_stop = false;
  // uids are allocated from 4.
  // uid 0 is "invalid" events
  // uid 1 is "now" events
  // uid 2 is "destroy" events
  m_uid = 4;
  // before ::Run is entered, the m_currentUid will be zero
  m_currentUid = 0;
  m_currentTs = 0;
  m_currentContext = Simulator::NO_CONTEXT;
  m_unscheduledEvents = 0;
  m_eventsWithContextEmpty = true;
  m_main = SystemThread::Self();
}

HelicsSimulatorImpl::~HelicsSimulatorImpl ()
{
  NS_LOG_FUNCTION (this);
}

void
HelicsSimulatorImpl::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  ProcessEventsWithContext ();

  while (!m_events->IsEmpty ())
    {
      Scheduler::Event next = m_events->RemoveNext ();
      next.impl->Unref ();
    }
  m_events = 0;

  SimulatorImpl::DoDispose ();
}
void
HelicsSimulatorImpl::Destroy ()
{
  NS_LOG_FUNCTION (this);
  while (!m_destroyEvents.empty ()) 
    {
      Ptr<EventImpl> ev = m_destroyEvents.front ().PeekEventImpl ();
      m_destroyEvents.pop_front ();
      NS_LOG_LOGIC ("handle destroy " << ev);
      if (!ev->IsCancelled ())
        {
          ev->Invoke ();
        }
    }
}

void
HelicsSimulatorImpl::SetScheduler (ObjectFactory schedulerFactory)
{
  NS_LOG_FUNCTION (this << schedulerFactory);
  Ptr<Scheduler> scheduler = schedulerFactory.Create<Scheduler> ();

  if (m_events != 0)
    {
      while (!m_events->IsEmpty ())
        {
          Scheduler::Event next = m_events->RemoveNext ();
          scheduler->Insert (next);
        }
    }
  m_events = scheduler;
}

// System ID for non-distributed simulation is always zero
uint32_t 
HelicsSimulatorImpl::GetSystemId (void) const
{
  // Return HELICS federate ID
  return federate->getID ();
}

void
HelicsSimulatorImpl::ProcessOneEvent (void)
{
  NS_LOG_FUNCTION (this);
  Scheduler::Event next = m_events->RemoveNext ();

  NS_ASSERT (next.key.m_ts >= m_currentTs);
  m_unscheduledEvents--;

  NS_LOG_LOGIC ("handle " << next.key.m_ts);
  m_currentTs = next.key.m_ts;
  m_currentContext = next.key.m_context;
  m_currentUid = next.key.m_uid;
  next.impl->Invoke ();
  next.impl->Unref ();

  ProcessEventsWithContext ();
}

bool 
HelicsSimulatorImpl::IsFinished (void) const
{
  return m_events->IsEmpty () || m_stop;
}

void
HelicsSimulatorImpl::ProcessEventsWithContext (void)
{
  if (m_eventsWithContextEmpty)
    {
      return;
    }

  // swap queues
  EventsWithContext eventsWithContext;
  {
    CriticalSection cs (m_eventsWithContextMutex);
    m_eventsWithContext.swap(eventsWithContext);
    m_eventsWithContextEmpty = true;
  }
  while (!eventsWithContext.empty ())
    {
       EventWithContext event = eventsWithContext.front ();
       eventsWithContext.pop_front ();
       Scheduler::Event ev;
       ev.impl = event.event;
       ev.key.m_ts = m_currentTs + event.timestamp;
       ev.key.m_context = event.context;
       ev.key.m_uid = m_uid;
       m_uid++;
       m_unscheduledEvents++;
       m_events->Insert (ev);
    }
}

Time
HelicsSimulatorImpl::Next (void) const
{
  if (m_events->IsEmpty ())
    {
      return GetMaximumSimulationTime ();
    }
  else
    {
      Scheduler::Event ev = m_events->PeekNext ();
      return TimeStep (ev.key.m_ts);
    }
}

void
HelicsSimulatorImpl::Run (void)
{
  NS_LOG_FUNCTION (this);
  // Set the current threadId as the main threadId
  m_main = SystemThread::Self();
  ProcessEventsWithContext ();
  m_stop = false;

  // Begin HELICS simulation
  NS_LOG_INFO ("Entering execution state");
  federate->enterExecutionState ();

  // Requests time of next event, or max simulation time if nothing in the queue
  auto requested = Next ().GetSeconds ();
  NS_LOG_INFO ("    Requesting time: " << requested);
  auto granted = federate->requestTime (requested);
  NS_LOG_INFO ("Granted time helics: " << granted);
  Time grantedTime = Time::FromDouble (granted, Time::S);
  NS_LOG_INFO ("  Granted time ns-3: " << grantedTime);
  Time nextTime = Next ();
  NS_LOG_INFO ("     Next time ns-3: " << grantedTime);

  // Keep processing events until stop time is reached
  while (!m_stop) 
    {
      // Only process events up until the granted time
      NS_LOG_INFO ("    m_events->IsEmpty(): " << m_events->IsEmpty());
      NS_LOG_INFO ("                 m_stop: " << m_stop);
      NS_LOG_INFO ("nextTime <= grantedTime: " << (nextTime<=grantedTime));
      while (!m_events->IsEmpty () && !m_stop && nextTime <= grantedTime) 
        {
          ProcessOneEvent ();
          nextTime = Next ();
          NS_LOG_INFO ("Next time ns-3: " << nextTime);
          NS_LOG_INFO ("    m_events->IsEmpty(): " << m_events->IsEmpty());
          NS_LOG_INFO ("                 m_stop: " << m_stop);
          NS_LOG_INFO ("nextTime <= grantedTime: " << (nextTime<=grantedTime));
        }
  

      if (!m_stop)
        {
          m_currentTs = grantedTime.GetTimeStep ();
          requested = Next ().GetSeconds ();
          NS_LOG_INFO ("    Requesting time: " << requested);
          granted = federate->requestTime (requested);
          NS_LOG_INFO ("Granted time helics: " << granted);
          grantedTime = Time::FromDouble (granted, Time::S);
          NS_LOG_INFO ("  Granted time ns-3: " << grantedTime);
        }
   }

  // End HELICS simulation
  federate->finalize ();

  // If the simulator stopped naturally by lack of events, make a
  // consistency test to check that we didn't lose any events along the way.
  NS_ASSERT (!m_events->IsEmpty () || m_unscheduledEvents == 0);
}

void 
HelicsSimulatorImpl::Stop (void)
{
  NS_LOG_FUNCTION (this);

  m_stop = true;
}

void 
HelicsSimulatorImpl::Stop (Time const &delay)
{
  NS_LOG_FUNCTION (this << delay.GetTimeStep ());
  Simulator::Schedule (delay, &Simulator::Stop);
}

//
// Schedule an event for a _relative_ time in the future.
//
EventId
HelicsSimulatorImpl::Schedule (Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << delay.GetTimeStep () << event);
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::Schedule Thread-unsafe invocation!");

  Time tAbsolute = delay + TimeStep (m_currentTs);

  NS_ASSERT (tAbsolute.IsPositive ());
  NS_ASSERT (tAbsolute >= TimeStep (m_currentTs));
  Scheduler::Event ev;
  ev.impl = event;
  ev.key.m_ts = (uint64_t) tAbsolute.GetTimeStep ();
  ev.key.m_context = GetContext ();
  ev.key.m_uid = m_uid;
  m_uid++;
  m_unscheduledEvents++;
  m_events->Insert (ev);
  return EventId (event, ev.key.m_ts, ev.key.m_context, ev.key.m_uid);
}

void
HelicsSimulatorImpl::ScheduleWithContext (uint32_t context, Time const &delay, EventImpl *event)
{
  NS_LOG_FUNCTION (this << context << delay.GetTimeStep () << event);

  if (SystemThread::Equals (m_main))
    {
      Time tAbsolute = delay + TimeStep (m_currentTs);
      Scheduler::Event ev;
      ev.impl = event;
      ev.key.m_ts = (uint64_t) tAbsolute.GetTimeStep ();
      ev.key.m_context = context;
      ev.key.m_uid = m_uid;
      m_uid++;
      m_unscheduledEvents++;
      m_events->Insert (ev);
    }
  else
    {
      EventWithContext ev;
      ev.context = context;
      // Current time added in ProcessEventsWithContext()
      ev.timestamp = delay.GetTimeStep ();
      ev.event = event;
      {
        CriticalSection cs (m_eventsWithContextMutex);
        m_eventsWithContext.push_back(ev);
        m_eventsWithContextEmpty = false;
      }
    }
}

EventId
HelicsSimulatorImpl::ScheduleNow (EventImpl *event)
{
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::ScheduleNow Thread-unsafe invocation!");

  Scheduler::Event ev;
  ev.impl = event;
  ev.key.m_ts = m_currentTs;
  ev.key.m_context = GetContext ();
  ev.key.m_uid = m_uid;
  m_uid++;
  m_unscheduledEvents++;
  m_events->Insert (ev);
  return EventId (event, ev.key.m_ts, ev.key.m_context, ev.key.m_uid);
}

EventId
HelicsSimulatorImpl::ScheduleDestroy (EventImpl *event)
{
  NS_ASSERT_MSG (SystemThread::Equals (m_main), "Simulator::ScheduleDestroy Thread-unsafe invocation!");

  EventId id (Ptr<EventImpl> (event, false), m_currentTs, 0xffffffff, 2);
  m_destroyEvents.push_back (id);
  m_uid++;
  return id;
}

Time
HelicsSimulatorImpl::Now (void) const
{
  // Do not add function logging here, to avoid stack overflow
  return TimeStep (m_currentTs);
}

Time 
HelicsSimulatorImpl::GetDelayLeft (const EventId &id) const
{
  if (IsExpired (id))
    {
      return TimeStep (0);
    }
  else
    {
      return TimeStep (id.GetTs () - m_currentTs);
    }
}

void
HelicsSimulatorImpl::Remove (const EventId &id)
{
  if (id.GetUid () == 2)
    {
      // destroy events.
      for (DestroyEvents::iterator i = m_destroyEvents.begin (); i != m_destroyEvents.end (); i++)
        {
          if (*i == id)
            {
              m_destroyEvents.erase (i);
              break;
            }
        }
      return;
    }
  if (IsExpired (id))
    {
      return;
    }
  Scheduler::Event event;
  event.impl = id.PeekEventImpl ();
  event.key.m_ts = id.GetTs ();
  event.key.m_context = id.GetContext ();
  event.key.m_uid = id.GetUid ();
  m_events->Remove (event);
  event.impl->Cancel ();
  // whenever we remove an event from the event list, we have to unref it.
  event.impl->Unref ();

  m_unscheduledEvents--;
}

void
HelicsSimulatorImpl::Cancel (const EventId &id)
{
  if (!IsExpired (id))
    {
      id.PeekEventImpl ()->Cancel ();
    }
}

bool
HelicsSimulatorImpl::IsExpired (const EventId &id) const
{
  if (id.GetUid () == 2)
    {
      if (id.PeekEventImpl () == 0 ||
          id.PeekEventImpl ()->IsCancelled ())
        {
          return true;
        }
      // destroy events.
      for (DestroyEvents::const_iterator i = m_destroyEvents.begin (); i != m_destroyEvents.end (); i++)
        {
          if (*i == id)
            {
              return false;
            }
        }
      return true;
    }
  if (id.PeekEventImpl () == 0 ||
      id.GetTs () < m_currentTs ||
      (id.GetTs () == m_currentTs &&
       id.GetUid () <= m_currentUid) ||
      id.PeekEventImpl ()->IsCancelled ()) 
    {
      return true;
    }
  else
    {
      return false;
    }
}

Time 
HelicsSimulatorImpl::GetMaximumSimulationTime (void) const
{
  return TimeStep (0x7fffffffffffffffLL);
}

uint32_t
HelicsSimulatorImpl::GetContext (void) const
{
  return m_currentContext;
}

} // namespace ns3
