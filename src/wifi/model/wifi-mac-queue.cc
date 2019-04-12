/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/simulator.h"
#include "wifi-mac-queue.h"
#include "qos-blocked-destinations.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiMacQueue");

NS_OBJECT_ENSURE_REGISTERED (WifiMacQueue);
NS_OBJECT_TEMPLATE_CLASS_DEFINE (Queue, WifiMacQueueItem);

TypeId
WifiMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacQueue")
    .SetParent<Queue<WifiMacQueueItem> > ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiMacQueue> ()
    .AddAttribute ("MaxQueueSize",
                   "The max queue size",
                   QueueSizeValue (QueueSize ("500p")),
                   MakeQueueSizeAccessor (&WifiMacQueue::SetMaxQueueSize),
                   MakeQueueSizeChecker ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (MilliSeconds (500)),
                   MakeTimeAccessor (&WifiMacQueue::SetMaxDelay),
                   MakeTimeChecker ())
    .AddAttribute ("DropPolicy", "Upon enqueue with full queue, drop oldest (DropOldest) or newest (DropNewest) packet",
                   EnumValue (DROP_NEWEST),
                   MakeEnumAccessor (&WifiMacQueue::m_dropPolicy),
                   MakeEnumChecker (WifiMacQueue::DROP_OLDEST, "DropOldest",
                                    WifiMacQueue::DROP_NEWEST, "DropNewest"))
    .AddTraceSource ("Expired", "MPDU dropped because its lifetime expired.",
                     MakeTraceSourceAccessor (&WifiMacQueue::m_traceExpired),
                     "ns3::WifiMacQueueItem::TracedCallback")
  ;
  return tid;
}

WifiMacQueue::WifiMacQueue ()
  : m_expiredPacketsPresent (false),
    NS_LOG_TEMPLATE_DEFINE ("WifiMacQueue")
{
}

WifiMacQueue::~WifiMacQueue ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

const WifiMacQueue::ConstIterator WifiMacQueue::EMPTY = std::list<Ptr<WifiMacQueueItem>> ().end ();

void
WifiMacQueue::SetMaxQueueSize (QueueSize size)
{
  NS_LOG_FUNCTION (this << size);
  m_maxSize = size;
}

QueueSize
WifiMacQueue::GetMaxQueueSize (void) const
{
  return m_maxSize;
}

void
WifiMacQueue::SetMaxDelay (Time delay)
{
  NS_LOG_FUNCTION (this << delay);
  m_maxDelay = delay;
}

Time
WifiMacQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}

bool
WifiMacQueue::TtlExceeded (ConstIterator &it)
{
  NS_LOG_FUNCTION (this);

  if (Simulator::Now () > (*it)->GetTimeStamp () + m_maxDelay)
    {
      NS_LOG_DEBUG ("Removing packet that stayed in the queue for too long (" <<
                    Simulator::Now () - (*it)->GetTimeStamp () << ")");
      m_traceExpired (*it);
      auto curr = it++;
      DoRemove (curr);
      return true;
    }
  return false;
}

bool
WifiMacQueue::Enqueue (Ptr<WifiMacQueueItem> item)
{
  NS_LOG_FUNCTION (this << *item);

  return Insert (end (), item);
}

bool
WifiMacQueue::PushFront (Ptr<WifiMacQueueItem> item)
{
  NS_LOG_FUNCTION (this << *item);

  return Insert (begin (), item);
}

bool
WifiMacQueue::Insert (ConstIterator pos, Ptr<WifiMacQueueItem> item)
{
  NS_LOG_FUNCTION (this << *item);
  NS_ASSERT_MSG (GetMaxSize ().GetUnit () == QueueSizeUnit::PACKETS,
                 "WifiMacQueues must be in packet mode");

  QueueBase::SetMaxSize (GetMaxQueueSize ()); //Make sure QueueBase has the same maximum queue size

  // insert the item if the queue is not full
  if (QueueBase::GetNPackets () < GetMaxSize ().GetValue ())
    {
      return DoEnqueue (pos, item);
    }

  // the queue is full; scan the list in the attempt to remove stale packets
  ConstIterator it = begin ();
  while (it != end ())
    {
      if (it == pos && TtlExceeded (it))
        {
          return DoEnqueue (it, item);
        }
      if (TtlExceeded (it))
        {
          return DoEnqueue (pos, item);
        }
      it++;
    }

  // the queue is still full, remove the oldest item if the policy is drop oldest
  if (m_dropPolicy == DROP_OLDEST)
    {
      NS_LOG_DEBUG ("Remove the oldest item in the queue");
      DoRemove (begin ());
    }

  return DoEnqueue (pos, item);
}

Ptr<WifiMacQueueItem>
WifiMacQueue::Dequeue (void)
{
  NS_LOG_FUNCTION (this);
  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          return DoDequeue (it);
        }
    }
  NS_LOG_DEBUG ("The queue is empty");
  return 0;
}

Ptr<WifiMacQueueItem>
WifiMacQueue::DequeueByAddress (Mac48Address dest)
{
  NS_LOG_FUNCTION (this << dest);
  ConstIterator it = PeekByAddress (dest);

  if (it == end ())
    {
      return 0;
    }
  return Dequeue (it);
}

Ptr<WifiMacQueueItem>
WifiMacQueue::DequeueByTid (uint8_t tid)
{
  NS_LOG_FUNCTION (this << +tid);
  ConstIterator it = PeekByTid (tid);

  if (it == end ())
    {
      return 0;
    }
  return Dequeue (it);
}

Ptr<WifiMacQueueItem>
WifiMacQueue::DequeueByTidAndAddress (uint8_t tid, Mac48Address dest)
{
  NS_LOG_FUNCTION (this << +tid << dest);
  ConstIterator it = PeekByTidAndAddress (tid, dest);

  if (it == end ())
    {
      return 0;
    }
  return Dequeue (it);
}

Ptr<WifiMacQueueItem>
WifiMacQueue::DequeueFirstAvailable (const Ptr<QosBlockedDestinations> blockedPackets)
{
  NS_LOG_FUNCTION (this);
  ConstIterator it = PeekFirstAvailable (blockedPackets);

  if (it == end ())
    {
      return 0;
    }
  return Dequeue (it);
}

Ptr<WifiMacQueueItem>
WifiMacQueue::Dequeue (ConstIterator pos)
{
  NS_LOG_FUNCTION (this);

  if (!m_expiredPacketsPresent)
    {
      if (TtlExceeded (pos))
        {
          NS_LOG_DEBUG ("Packet lifetime expired");
          return 0;
        }
      return DoDequeue (pos);
    }

  // remove stale items queued before the given position
  ConstIterator it = begin ();
  while (it != end ())
    {
      if (it == pos)
        {
          // reset the flag signaling the presence of expired packets before returning
          m_expiredPacketsPresent = false;

          if (TtlExceeded (it))
            {
              return 0;
            }
          return DoDequeue (it);
        }
      else if (!TtlExceeded (it))
        {
          it++;
        }
    }
  NS_LOG_DEBUG ("Invalid iterator");
  return 0;
}

Ptr<const WifiMacQueueItem>
WifiMacQueue::Peek (void) const
{
  NS_LOG_FUNCTION (this);
  for (auto it = begin (); it != end (); it++)
    {
      // skip packets that stayed in the queue for too long. They will be
      // actually removed from the queue by the next call to a non-const method
      if (Simulator::Now () <= (*it)->GetTimeStamp () + m_maxDelay)
        {
          return DoPeek (it);
        }
      // signal the presence of expired packets
      m_expiredPacketsPresent = true;
    }
  NS_LOG_DEBUG ("The queue is empty");
  return 0;
}

WifiMacQueue::ConstIterator
WifiMacQueue::PeekByAddress (Mac48Address dest, ConstIterator pos) const
{
  NS_LOG_FUNCTION (this << dest);
  ConstIterator it = (pos != EMPTY ? pos : begin ());
  while (it != end ())
    {
      // skip packets that stayed in the queue for too long. They will be
      // actually removed from the queue by the next call to a non-const method
      if (Simulator::Now () <= (*it)->GetTimeStamp () + m_maxDelay)
        {
          if (((*it)->GetHeader ().IsData () || (*it)->GetHeader ().IsQosData ())
              && (*it)->GetDestinationAddress () == dest)
            {
              return it;
            }
        }
      else
        {
          // signal the presence of expired packets
          m_expiredPacketsPresent = true;
        }
      it++;
    }
  NS_LOG_DEBUG ("The queue is empty");
  return end ();
}

WifiMacQueue::ConstIterator
WifiMacQueue::PeekByTid (uint8_t tid, ConstIterator pos) const
{
  NS_LOG_FUNCTION (this << +tid);
  ConstIterator it = (pos != EMPTY ? pos : begin ());
  while (it != end ())
    {
      // skip packets that stayed in the queue for too long. They will be
      // actually removed from the queue by the next call to a non-const method
      if (Simulator::Now () <= (*it)->GetTimeStamp () + m_maxDelay)
        {
          if ((*it)->GetHeader ().IsQosData () && (*it)->GetHeader ().GetQosTid () == tid)
            {
              return it;
            }
        }
      else
        {
          // signal the presence of expired packets
          m_expiredPacketsPresent = true;
        }
      it++;
    }
  NS_LOG_DEBUG ("The queue is empty");
  return end ();
}

WifiMacQueue::ConstIterator
WifiMacQueue::PeekByTidAndAddress (uint8_t tid, Mac48Address dest, ConstIterator pos) const
{
  NS_LOG_FUNCTION (this << +tid << dest);
  ConstIterator it = (pos != EMPTY ? pos : begin ());
  while (it != end ())
    {
      // skip packets that stayed in the queue for too long. They will be
      // actually removed from the queue by the next call to a non-const method
      if (Simulator::Now () <= (*it)->GetTimeStamp () + m_maxDelay)
        {
          if ((*it)->GetHeader ().IsQosData () && (*it)->GetDestinationAddress () == dest
              && (*it)->GetHeader ().GetQosTid () == tid)
            {
              return it;
            }
        }
      else
        {
          // signal the presence of expired packets
          m_expiredPacketsPresent = true;
        }
      it++;
    }
  NS_LOG_DEBUG ("The queue is empty");
  return end ();
}

WifiMacQueue::ConstIterator
WifiMacQueue::PeekFirstAvailable (const Ptr<QosBlockedDestinations> blockedPackets, ConstIterator pos) const
{
  NS_LOG_FUNCTION (this);
  ConstIterator it = (pos != EMPTY ? pos : begin ());
  while (it != end ())
    {
      // skip packets that stayed in the queue for too long. They will be
      // actually removed from the queue by the next call to a non-const method
      if (Simulator::Now () <= (*it)->GetTimeStamp () + m_maxDelay)
        {
          if (!(*it)->GetHeader ().IsQosData () || !blockedPackets
              || !blockedPackets->IsBlocked ((*it)->GetHeader ().GetAddr1 (), (*it)->GetHeader ().GetQosTid ()))
            {
              return it;
            }
        }
      else
        {
          // signal the presence of expired packets
          m_expiredPacketsPresent = true;
        }
      it++;
    }
  NS_LOG_DEBUG ("The queue is empty");
  return end ();
}

Ptr<WifiMacQueueItem>
WifiMacQueue::Remove (void)
{
  NS_LOG_FUNCTION (this);

  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          return DoRemove (it);
        }
    }
  NS_LOG_DEBUG ("The queue is empty");
  return 0;
}

bool
WifiMacQueue::Remove (Ptr<const Packet> packet)
{
  NS_LOG_FUNCTION (this << packet);
  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          if ((*it)->GetPacket () == packet)
            {
              DoRemove (it);
              return true;
            }

          it++;
        }
    }
  NS_LOG_DEBUG ("Packet " << packet << " not found in the queue");
  return false;
}

WifiMacQueue::ConstIterator
WifiMacQueue::Remove (ConstIterator pos, bool removeExpired)
{
  NS_LOG_FUNCTION (this);

  if (!removeExpired)
    {
      ConstIterator curr = pos++;
      DoRemove (curr);
      return pos;
    }

  // remove stale items queued before the given position
  ConstIterator it = begin ();
  while (it != end ())
    {
      if (it == pos)
        {
          // reset the flag signaling the presence of expired packets before returning
          m_expiredPacketsPresent = false;

          ConstIterator curr = pos++;
          DoRemove (curr);
          return pos;
        }
      else if (!TtlExceeded (it))
        {
          it++;
        }
    }
  NS_LOG_DEBUG ("Invalid iterator");
  return end ();
}

uint32_t
WifiMacQueue::GetNPacketsByAddress (Mac48Address dest)
{
  NS_LOG_FUNCTION (this << dest);

  uint32_t nPackets = 0;

  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          if ((*it)->GetHeader ().IsData () && (*it)->GetDestinationAddress () == dest)
            {
              nPackets++;
            }

          it++;
        }
    }
  NS_LOG_DEBUG ("returns " << nPackets);
  return nPackets;
}

uint32_t
WifiMacQueue::GetNPacketsByTidAndAddress (uint8_t tid, Mac48Address dest)
{
  NS_LOG_FUNCTION (this << dest);
  uint32_t nPackets = 0;
  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          if ((*it)->GetHeader ().IsQosData () && (*it)->GetDestinationAddress () == dest
              && (*it)->GetHeader ().GetQosTid () == tid)
            {
              nPackets++;
            }

          it++;
        }
    }
  NS_LOG_DEBUG ("returns " << nPackets);
  return nPackets;
}

bool
WifiMacQueue::IsEmpty (void)
{
  NS_LOG_FUNCTION (this);
  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          NS_LOG_DEBUG ("returns false");
          return false;
        }
    }
  NS_LOG_DEBUG ("returns true");
  return true;
}

uint32_t
WifiMacQueue::GetNPackets (void)
{
  NS_LOG_FUNCTION (this);
  // remove packets that stayed in the queue for too long
  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          it++;
        }
    }
  return QueueBase::GetNPackets ();
}

uint32_t
WifiMacQueue::GetNBytes (void)
{
  NS_LOG_FUNCTION (this);
  // remove packets that stayed in the queue for too long
  for (ConstIterator it = begin (); it != end (); )
    {
      if (!TtlExceeded (it))
        {
          it++;
        }
    }
  return QueueBase::GetNBytes ();
}

} //namespace ns3
