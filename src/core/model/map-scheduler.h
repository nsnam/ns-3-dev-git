/*
 * Copyright (c) 2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef MAP_SCHEDULER_H
#define MAP_SCHEDULER_H

#include "scheduler.h"

#include <map>
#include <stdint.h>
#include <utility>

/**
 * @file
 * @ingroup scheduler
 * ns3::MapScheduler declaration.
 */

namespace ns3
{

/**
 * @ingroup scheduler
 * @brief a std::map event scheduler
 *
 * This class implements the an event scheduler using an std::map
 * data structure.
 *
 * @par Time Complexity
 *
 * Operation    | Amortized %Time | Reason
 * :----------- | :-------------- | :-----
 * Insert()     | Logarithmic     | `std::map::insert()`
 * IsEmpty()    | Constant        | `std::map::empty()`
 * PeekNext()   | Constant        | `std::map::begin()`
 * Remove()     | Logarithmic     | `std::map::find()`
 * RemoveNext() | Constant        | `std::map::begin()`
 *
 * @par Memory Complexity
 *
 * Category  | Memory                           | Reason
 * :-------- | :------------------------------- | :-----
 * Overhead  | 3 x `sizeof (*)` + 2 x `size_t`<br/>(40 bytes) | red-black tree
 * Per Event | 3 x `sizeof (*)` + `int`<br/>(32 bytes)        | red-black tree
 *
 */
class MapScheduler : public Scheduler
{
  public:
    /**
     *  Register this type.
     *  @return The object TypeId.
     */
    static TypeId GetTypeId();

    /** Constructor. */
    MapScheduler();
    /** Destructor. */
    ~MapScheduler() override;

    // Inherited
    void Insert(const Scheduler::Event& ev) override;
    bool IsEmpty() const override;
    Scheduler::Event PeekNext() const override;
    Scheduler::Event RemoveNext() override;
    void Remove(const Scheduler::Event& ev) override;

  private:
    /** Event list type: a Map from EventKey to EventImpl. */
    typedef std::map<Scheduler::EventKey, EventImpl*> EventMap;
    /** EventMap iterator. */
    typedef std::map<Scheduler::EventKey, EventImpl*>::iterator EventMapI;
    /** EventMap const iterator. */
    typedef std::map<Scheduler::EventKey, EventImpl*>::const_iterator EventMapCI;

    /** The event list. */
    EventMap m_list;
};

} // namespace ns3

#endif /* MAP_SCHEDULER_H */
