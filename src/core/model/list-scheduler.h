/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef LIST_SCHEDULER_H
#define LIST_SCHEDULER_H

#include "scheduler.h"

#include <list>
#include <stdint.h>
#include <utility>

/**
 * @file
 * @ingroup scheduler
 * ns3::ListScheduler declaration.
 */

namespace ns3
{

class EventImpl;

/**
 * @ingroup scheduler
 * @brief a std::list event scheduler
 *
 * This class implements an event scheduler using an std::list
 * data structure, that is, a double linked-list.
 *
 * @par Time Complexity
 *
 * Operation    | Amortized %Time | Reason
 * :----------- | :-------------- | :-----
 * Insert()     | Linear          | Linear search in `std::list`
 * IsEmpty()    | Constant        | `std::list::size()`
 * PeekNext()   | Constant        | `std::list::front()`
 * Remove()     | Linear          | Linear search in `std::list`
 * RemoveNext() | Constant        | `std::list::pop_front()`
 *
 * @par Memory Complexity
 *
 * Category  | Memory                           | Reason
 * :-------- | :------------------------------- | :-----
 * Overhead  | 2 x `sizeof (*)` + `size_t`<br/>(24 bytes) | `std::list`
 * Per Event | 2 x `sizeof (*)`                 | `std::list`
 *
 */
class ListScheduler : public Scheduler
{
  public:
    /**
     *  Register this type.
     *  @return The object TypeId.
     */
    static TypeId GetTypeId();

    /** Constructor. */
    ListScheduler();
    /** Destructor. */
    ~ListScheduler() override;

    // Inherited
    void Insert(const Scheduler::Event& ev) override;
    bool IsEmpty() const override;
    Scheduler::Event PeekNext() const override;
    Scheduler::Event RemoveNext() override;
    void Remove(const Scheduler::Event& ev) override;

  private:
    /** Event list type: a simple list of Events. */
    typedef std::list<Scheduler::Event> Events;
    /** Events iterator. */
    typedef std::list<Scheduler::Event>::iterator EventsI;

    /** The event list. */
    Events m_events;
};

} // namespace ns3

#endif /* LIST_SCHEDULER_H */
