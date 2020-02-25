/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006 INRIA
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

#ifndef PRIORITY_QUEUE_SCHEDULER_H
#define PRIORITY_QUEUE_SCHEDULER_H

#include "scheduler.h"
#include <stdint.h>
#include <queue>
#include <utility>

/**
 * \file
 * \ingroup scheduler
 * Declaration of ns3::PriorityQueueScheduler class.
 */

namespace ns3 {

/**
 * \ingroup scheduler
 * \brief a std::map event scheduler
 *
 * This class implements the an event scheduler using an std::map
 * data structure.
 */
class PriorityQueueScheduler : public Scheduler
{
public:
  /**
   *  Register this type.
   *  \return The object TypeId.
   */
  static TypeId GetTypeId (void);

  /** Constructor. */
  PriorityQueueScheduler ();
  /** Destructor. */
  virtual ~PriorityQueueScheduler ();

  // Inherited
  virtual void Insert (const Scheduler::Event &ev);
  virtual bool IsEmpty (void) const;
  virtual Scheduler::Event PeekNext (void) const;
  virtual Scheduler::Event RemoveNext (void);
  virtual void Remove (const Event &ev);

private:

  /** The event list. */
  std::priority_queue<Scheduler::Event,
                      std::vector<Scheduler::Event>,
                      std::greater<Scheduler::Event>> m_list;
};

} // namespace ns3

#endif /* PRIORITY_QUEUE_SCHEDULER_H */
