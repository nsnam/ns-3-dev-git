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

#ifndef WIFI_MAC_QUEUE_H
#define WIFI_MAC_QUEUE_H

#include "wifi-mac-queue-item.h"
#include "ns3/queue.h"
#include <unordered_map>
#include "qos-utils.h"

namespace ns3 {

class QosBlockedDestinations;

// The following explicit template instantiation declaration prevents modules
// including this header file from implicitly instantiating Queue<WifiMacQueueItem>.
// This would cause python examples using wifi to crash at runtime with the
// following error message: "Trying to allocate twice the same UID:
// ns3::Queue<WifiMacQueueItem>"
extern template class Queue<WifiMacQueueItem>;


/**
 * \ingroup wifi
 *
 * This queue implements the timeout procedure described in
 * (Section 9.19.2.6 "Retransmit procedures" paragraph 6; IEEE 802.11-2012).
 *
 * When a packet is received by the MAC, to be sent to the PHY,
 * it is queued in the internal queue after being tagged by the
 * current time.
 *
 * When a packet is dequeued, the queue checks its timestamp
 * to verify whether or not it should be dropped. If
 * dot11EDCATableMSDULifetime has elapsed, it is dropped.
 * Otherwise, it is returned to the caller.
 */
class WifiMacQueue : public Queue<WifiMacQueueItem>
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  WifiMacQueue ();
  ~WifiMacQueue ();

  /// drop policy
  enum DropPolicy
  {
    DROP_NEWEST,
    DROP_OLDEST
  };

  /// allow the usage of iterators and const iterators
  using Queue<WifiMacQueueItem>::ConstIterator;
  using Queue<WifiMacQueueItem>::Iterator;
  using Queue<WifiMacQueueItem>::begin;
  using Queue<WifiMacQueueItem>::end;

  /**
   * Set the maximum delay before the packet is discarded.
   *
   * \param delay the maximum delay
   */
  void SetMaxDelay (Time delay);
  /**
   * Return the maximum delay before the packet is discarded.
   *
   * \return the maximum delay
   */
  Time GetMaxDelay (void) const;

  /**
   * Enqueue the given Wifi MAC queue item at the <i>end</i> of the queue.
   *
   * \param item the Wifi MAC queue item to be enqueued at the end
   * \return true if success, false if the packet has been dropped
   */
  bool Enqueue (Ptr<WifiMacQueueItem> item);
  /**
   * Enqueue the given Wifi MAC queue item at the <i>front</i> of the queue.
   *
   * \param item the Wifi MAC queue item to be enqueued at the front
   * \return true if success, false if the packet has been dropped
   */
  bool PushFront (Ptr<WifiMacQueueItem> item);
  /**
   * Enqueue the given Wifi MAC queue item before the given position.
   *
   * \param pos the position before which the item is to be inserted
   * \param item the Wifi MAC queue item to be enqueued
   * \return true if success, false if the packet has been dropped
   */
  bool Insert (ConstIterator pos, Ptr<WifiMacQueueItem> item);
  /**
   * Dequeue the packet in the front of the queue.
   *
   * \return the packet
   */
  Ptr<WifiMacQueueItem> Dequeue (void);
  /**
   * Search and return, if present in the queue, the first packet (either Data
   * frame or QoS Data frame) having the receiver address equal to <i>addr</i>.
   * This method removes the packet from the queue.
   * It is typically used by ns3::Txop during the CF period.
   *
   * \param dest the given destination
   *
   * \return the packet
   */
  Ptr<WifiMacQueueItem> DequeueByAddress (Mac48Address dest);
  /**
   * Search and return, if present in the queue, the first packet having the
   * TID equal to <i>tid</i>.
   * This method removes the packet from the queue.
   *
   * \param tid the given TID
   *
   * \return the packet
   */
  Ptr<WifiMacQueueItem> DequeueByTid (uint8_t tid);
  /**
   * Search and return, if present in the queue, the first packet having the
   * address indicated by <i>type</i> equal to <i>addr</i>, and TID
   * equal to <i>tid</i>. This method removes the packet from the queue.
   * It is typically used by ns3::QosTxop in order to perform correct MSDU
   * aggregation (A-MSDU).
   *
   * \param tid the given TID
   * \param dest the given destination
   *
   * \return the packet
   */
  Ptr<WifiMacQueueItem> DequeueByTidAndAddress (uint8_t tid,
                                                Mac48Address dest);
  /**
   * Return first available packet for transmission. A packet could be no available
   * if it is a QoS packet with a TID and an address1 fields equal to <i>tid</i> and <i>addr</i>
   * respectively that index a pending agreement in the BlockAckManager object.
   * So that packet must not be transmitted until reception of an ADDBA response frame from station
   * addressed by <i>addr</i>. This method removes the packet from queue.
   *
   * \param blockedPackets the destination address & TID pairs that are waiting for a BlockAck response
   *
   * \return the packet
   */
  Ptr<WifiMacQueueItem> DequeueFirstAvailable (const Ptr<QosBlockedDestinations> blockedPackets = nullptr);
  /**
   * Dequeue the item at position <i>pos</i> in the queue. Return a null
   * pointer if the given iterator is invalid, the queue is empty or the
   * lifetime of the item pointed to by the given iterator is expired.
   *
   * \param pos the position of the item to be dequeued
   * \return the dequeued item, if any
   */
  Ptr<WifiMacQueueItem> Dequeue (WifiMacQueue::ConstIterator pos);
  /**
   * Peek the packet in the front of the queue. The packet is not removed.
   *
   * \return the packet
   */
  Ptr<const WifiMacQueueItem> Peek (void) const;
  /**
   * Search and return, if present in the queue, the first packet (either Data
   * frame or QoS Data frame) having the receiver address equal to <i>addr</i>.
   * If <i>pos</i> is a valid iterator, the search starts from the packet pointed
   * to by the given iterator.
   * This method does not remove the packet from the queue.
   *
   * \param dest the given destination
   * \param pos the iterator pointing to the packet the search starts from
   *
   * \return an iterator pointing to the peeked packet
   */
  ConstIterator PeekByAddress (Mac48Address dest, ConstIterator pos = EMPTY) const;
  /**
   * Search and return, if present in the queue, the first packet having the
   * TID equal to <i>tid</i>. If <i>pos</i> is a valid iterator, the search starts
   * from the packet pointed to by the given iterator.
   * This method does not remove the packet from the queue.
   *
   * \param tid the given TID
   * \param pos the iterator pointing to the packet the search starts from
   *
   * \return an iterator pointing to the peeked packet
   */
  ConstIterator PeekByTid (uint8_t tid, ConstIterator pos = EMPTY) const;
  /**
   * Search and return, if present in the queue, the first packet having the
   * receiver address equal to <i>dest</i>, and TID equal to <i>tid</i>.
   * If <i>pos</i> is a valid iterator, the search starts from the packet pointed
   * to by the given iterator. This method does not remove the packet from the queue.
   * It is typically used by ns3::QosTxop in order to perform correct MSDU aggregation
   * (A-MSDU).
   *
   * \param tid the given TID
   * \param dest the given destination
   * \param pos the iterator pointing to the packet the search starts from
   *
   * \return an iterator pointing to the peeked packet
   */
  ConstIterator PeekByTidAndAddress (uint8_t tid, Mac48Address dest, ConstIterator pos = EMPTY) const;
  /**
   * Return first available packet for transmission. The packet is not removed from queue.
   *
   * \param blockedPackets the destination address & TID pairs that are waiting for a BlockAck response
   * \param pos the iterator pointing to the packet the search starts from
   *
   * \return an iterator pointing to the peeked packet
   */
  ConstIterator PeekFirstAvailable (const Ptr<QosBlockedDestinations> blockedPackets = nullptr,
                                    ConstIterator pos = EMPTY) const;
  /**
   * Remove the packet in the front of the queue.
   *
   * \return the packet
   */
  Ptr<WifiMacQueueItem> Remove (void);
  /**
   * If exists, removes <i>packet</i> from queue and returns true. Otherwise it
   * takes no effects and return false. Deletion of the packet is
   * performed in linear time (O(n)).
   *
   * \param packet the packet to be removed
   *
   * \return true if the packet was removed, false otherwise
   */
  bool Remove (Ptr<const Packet> packet);
  /**
   * Remove the item at position <i>pos</i> in the queue and return an iterator
   * pointing to the item following the removed one. If <i>removeExpired</i> is
   * true, all the items in the queue from the head to the given position are
   * removed if their lifetime expired.
   *
   * \param pos the position of the item to be removed
   * \param removeExpired true to remove expired items
   * \return an iterator pointing to the item following the removed one
   */
  ConstIterator Remove (ConstIterator pos, bool removeExpired = false);
  /**
   * Return the number of packets having destination address specified by
   * <i>dest</i>. The complexity is linear in the size of the queue.
   *
   * \param dest the given destination
   *
   * \return the number of packets
   */
  uint32_t GetNPacketsByAddress (Mac48Address dest);
  /**
   * Return the number of QoS packets having TID equal to <i>tid</i> and
   * destination address equal to <i>dest</i>.  The complexity is linear in
   * the size of the queue.
   *
   * \param tid the given TID
   * \param dest the given destination
   *
   * \return the number of QoS packets
   */
  uint32_t GetNPacketsByTidAndAddress (uint8_t tid, Mac48Address dest);

  /**
   * Return the number of QoS packets in the queue having tid equal to <i>tid</i>
   * and destination address equal to <i>dest</i>. The complexity in the average
   * case is constant. However, packets expired since the last non-const
   * operation on the queue are included in the returned count.
   *
   * \param tid the given TID
   * \param dest the given destination
   *
   * \return the number of QoS packets in the queue
   */
  uint32_t GetNPackets (uint8_t tid, Mac48Address dest) const;
  /**
   * Return the number of bytes in the queue having tid equal to <i>tid</i> and
   * destination address equal to <i>dest</i>. The complexity in the average
   * case is constant. However, packets expired since the last non-const
   * operation on the queue are included in the returned count.
   *
   * \param tid the given TID
   * \param dest the given destination
   *
   * \return the number of bytes in the queue
   */
  uint32_t GetNBytes (uint8_t tid, Mac48Address dest) const;

  /**
   * \return true if the queue is empty; false otherwise
   *
   * Overrides the IsEmpty method provided by QueueBase
   */
  bool IsEmpty (void);

  /**
   * \return The number of packets currently stored in the Queue
   *
   * Overrides the GetNPackets method provided by QueueBase
   */
  uint32_t GetNPackets (void);

  /**
   * \return The number of bytes currently occupied by the packets in the Queue
   *
   * Overrides the GetNBytes method provided by QueueBase
   */
  uint32_t GetNBytes (void);

  static const ConstIterator EMPTY;         //!< Invalid iterator to signal an empty queue


private:
  /**
   * Remove the item pointed to by the iterator <i>it</i> if it has been in the
   * queue for too long. If the item is removed, the iterator is updated to
   * point to the item that followed the erased one.
   *
   * \param it an iterator pointing to the item
   * \return true if the item is removed, false otherwise
   */
  bool TtlExceeded (ConstIterator &it);
  /**
   * Wrapper for the DoEnqueue method provided by the base class that additionally
   * sets the iterator field of the item and updates internal statistics, if
   * insertion succeeded.
   *
   * \param pos the position before where the item will be inserted
   * \param item the item to enqueue
   * \return true if success, false if the packet has been dropped.
   */
  bool DoEnqueue (ConstIterator pos, Ptr<WifiMacQueueItem> item);
  /**
   * Wrapper for the DoDequeue method provided by the base class that additionally
   * resets the iterator field of the item and updates internal statistics, if
   * an item was dequeued.
   *
   * \param pos the position of the item to dequeue
   * \return the item.
   */
  Ptr<WifiMacQueueItem> DoDequeue (ConstIterator pos);
  /**
   * Wrapper for the DoRemove method provided by the base class that additionally
   * resets the iterator field of the item and updates internal statistics, if
   * an item was dropped.
   *
   * \param pos the position of the item to drop
   * \return the item.
   */
  Ptr<WifiMacQueueItem> DoRemove (ConstIterator pos);

  Time m_maxDelay;                          //!< Time to live for packets in the queue
  DropPolicy m_dropPolicy;                  //!< Drop behavior of queue
  mutable bool m_expiredPacketsPresent;     //!< True if expired packets are in the queue

  //!< Per (MAC address, TID) pair queued packets
  std::unordered_map<WifiAddressTidPair, uint32_t, WifiAddressTidHash> m_nQueuedPackets;
  //!< Per (MAC address, TID) pair queued bytes
  std::unordered_map<WifiAddressTidPair, uint32_t, WifiAddressTidHash> m_nQueuedBytes;

  /// Traced callback: fired when a packet is dropped due to lifetime expiration
  TracedCallback<Ptr<const WifiMacQueueItem> > m_traceExpired;

  NS_LOG_TEMPLATE_DECLARE;                  //!< redefinition of the log component
};

} //namespace ns3

#endif /* WIFI_MAC_QUEUE_H */
