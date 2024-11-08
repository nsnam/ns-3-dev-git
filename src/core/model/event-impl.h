/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef EVENT_IMPL_H
#define EVENT_IMPL_H

#include "simple-ref-count.h"

#include <stdint.h>

/**
 * @file
 * @ingroup events
 * ns3::EventImpl declarations.
 */

namespace ns3
{

/**
 * @ingroup events
 * @brief A simulation event.
 *
 * Each subclass of this base class represents a simulation event. The
 * Invoke() method will be called by the simulation engine
 * when it reaches the time associated to this event. Most subclasses
 * are usually created by one of the many Simulator::Schedule
 * methods.
 */
class EventImpl : public SimpleRefCount<EventImpl>
{
  public:
    /** Default constructor. */
    EventImpl();
    /** Destructor. */
    virtual ~EventImpl() = 0;
    /**
     * Called by the simulation engine to notify the event that it is time
     * to execute.
     */
    void Invoke();
    /**
     * Marks the event as 'canceled'. The event is not removed from
     * the event list but the simulation engine will check its canceled status
     * before calling Invoke().
     */
    void Cancel();
    /**
     * @returns true if the event has been canceled.
     *
     * Checked by the simulation engine before calling Invoke().
     */
    bool IsCancelled();

  protected:
    /**
     * Implementation for Invoke().
     *
     * This typically calls a method or function pointer with the
     * arguments bound by a call to one of the MakeEvent() functions.
     */
    virtual void Notify() = 0;

  private:
    bool m_cancel; /**< Has this event been cancelled. */
};

} // namespace ns3

#endif /* EVENT_IMPL_H */
