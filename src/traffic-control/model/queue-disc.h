/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007, 2014 University of Washington
 *               2015 Universita' degli Studi di Napoli Federico II
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
 */

#ifndef QUEUE_DISC_H
#define QUEUE_DISC_H

#include "ns3/packet.h"
#include "ns3/object.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv6-header.h"
#include "ns3/net-device.h"
#include "ns3/traced-value.h"
#include <ns3/queue.h>

namespace ns3 {

/**
 * \ingroup internet
 *
 * QueueDiscItem represents the kind of items that are stored in a queue
 * disc. It is derived from QueueItem (which only consists of a Ptr<Packet>)
 * to additionally store the destination MAC address, the
 * protocol number and the transmission queue index,
 */
class QueueDiscItem : public QueueItem {
public:
  /**
   * \brief Create a queue disc item containing an IPv4 packet.
   * \param p the packet included in the created item.
   * \param addr the destination MAC address
   * \param protocol the protocol number
   * \param txq the transmission queue index
   */
  QueueDiscItem (Ptr<Packet> p, const Address & addr, uint16_t protocol, uint8_t txq);

  virtual ~QueueDiscItem ();

  /**
   * \return the MAC address included in this item.
   */
  Address GetAddress (void) const;

  /**
   * \return the protocol included in this item.
   */
  uint16_t GetProtocol (void) const;

  /**
   * \return the transmission queue index included in this item.
   */
  uint16_t GetTxq (void) const;

  /**
   * \brief Print the item contents.
   * \param os output stream in which the data should be printed.
   */
  virtual void Print (std::ostream &os) const;

private:
  /**
   * \brief Default constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  QueueDiscItem ();
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  QueueDiscItem (const QueueDiscItem &);
  /**
   * \brief Assignment operator
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  QueueDiscItem &operator = (const QueueDiscItem &);

  Address m_address;
  uint16_t m_protocol;
  uint8_t m_txq;
};

/**
 * \ingroup internet
 *
 * QueueDisc is a base class providing the interface and implementing
 * the operations common to all the queueing disciplines. Child classes
 * need to implement the methods used to enqueue a packet (DoEnqueue),
 * dequeue a single packet (DoDequeue), get a copy of the next packet
 * to extract (DoPeek), plus methods to classify enqueued packets in
 * case they manage multiple queues internally.
 *
 * The root queue disc must be aggregated to the NetDevice it is attached to.
 * A multi-queue aware (root) queue disc, i.e., a queue disc that is aware
 * of the number of (hardware) transmission queues used by the device, can
 * aggregate a child queue disc to each of the NetDeviceQueues.
 * Thus, in case of a root queue disc, the m_devQueue member points to one
 * of the transmission queues of the device (typically the first one).
 * In case of a queue disc that is a child of a multi-queue aware queue
 * disc, the m_devQueue member points to the transmission queue of the device
 * it is aggregated to.
 *
 * When setting up a root qdisc, a wake callback must be set on each of the
 * netdevice queues pointing to the Run method of the root queue disc. This is
 * done by calling the SetWakeCbOnAllTxQueues method. Multi-queue aware queue
 * discs can then override such a configuration by setting a wake callback on
 * each netdevice queue pointing to the Run method of the child queue disc
 * created to handle that netdevice queue. To this end, the SetWakeCbOnTxQueue
 * method can be exploited.
 *
 * When the Traffic Control layer receives a packet from the upper layers, it
 * enqueues such packet into the root queue disc, by calling Enqueue, and
 * requests the root queue disc to extract a train of packets by calling Run.
 * If the queue disc contains child queue discs (it might be multi-queue aware
 * or not), the root queue qdisc calls the Enqueue and Dequeue methods,
 * respectively, of the selected child classes to enqueue and dequeue packets.
 * Hence, Run is only called by the Traffic Control layer on the root queue disc.
 *
 * The design and implementation of this class is heavily inspired by Linux.
 */
class QueueDisc : public Object {
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  QueueDisc ();

  uint32_t GetNPackets (void) const;

  uint32_t GetNBytes (void) const;

  uint32_t GetTotalReceivedPackets (void) const;

  uint32_t GetTotalReceivedBytes (void) const;

  uint32_t GetTotalDroppedPackets (void) const;

  uint32_t GetTotalDroppedBytes (void) const;

  uint32_t GetTotalRequeuedPackets (void) const;

  uint32_t GetTotalRequeuedBytes (void) const;

  /**
   * Set the NetDeviceQueue associated with this queue discipline.
   * \param devQueue the NetDeviceQueue associated with this queue discipline.
   */
  void SetNetDeviceQueue (Ptr<NetDeviceQueue> devQueue);

  /**
   * \return the NetDeviceQueue associated with this queue discipline.
   */
  Ptr<NetDeviceQueue> GetNetDeviceQueue (void) const;

  /**
   * Set the wake callback on the NetDeviceQueue associated with this queue
   * discipline, if any, and do nothing otherwise.
   */
  void SetWakeCbOnTxQueue (void);

  /**
   * Set the wake callback on all the transmission queues of the device associated
   * with this queue discipline, if any, and do nothing otherwise.
   */
  void SetWakeCbOnAllTxQueues (void);

  virtual void SetQuota (const uint32_t quota);
  virtual uint32_t GetQuota (void) const;

  /**
   * Pass a packet to store to the queue discipline. This function only updates
   * the statistics and calls the (private) DoEnqueue function, which must be
   * implemented by derived classes.
   * \param item item to enqueue
   * \return True if the operation was successful; false otherwise
   */
  bool Enqueue (Ptr<QueueDiscItem> item);

  /**
   * Request the queue discipline to extract a packet. This function only updates
   * the statistics and calls the (private) DoDequeue function, which must be
   * implemented by derived classes.
   * \return 0 if the operation was not successful; the item otherwise.
   */
  Ptr<QueueDiscItem> Dequeue (void);

  /**
   * Get a copy of the next packet the queue discipline will extract, without
   * actually extracting the packet. This function only calls the (private)
   * DoPeek function, which must be implemented by derived classes.
   * \return 0 if the operation was not successful; the item otherwise.
   */
  Ptr<const QueueDiscItem> Peek (void) const;

  /**
   * Modelled after the Linux function __qdisc_run (net/sched/sch_generic.c)
   * Dequeues multiple packets, until a quota is exceeded or sending a packet
   * to the device failed.
   */
  void Run (void);

protected:
  /**
   *  \brief Drop a packet
   *  \param packet packet that was dropped
   *  This method is called by subclasses to notify parent (this class) of packet drops.
   */
  void Drop (Ptr<Packet> packet);

private:

  /**
   * This function actually enqueues a packet into the queue disc.
   * \param item item to enqueue
   * \return True if the operation was successful; false otherwise
   */
  virtual bool DoEnqueue (Ptr<QueueDiscItem> item) = 0;

  /**
   * This function actually extracts a packet from the queue disc.
   * \return 0 if the operation was not successful; the item otherwise.
   */
  virtual Ptr<QueueDiscItem> DoDequeue (void) = 0;

  /**
   * This function returns a copy of the next packet the queue disc will extract.
   * \return 0 if the operation was not successful; the packet otherwise.
   */
  virtual Ptr<const QueueDiscItem> DoPeek (void) const = 0;

  /**
   * Modelled after the Linux function qdisc_run_begin (include/net/sch_generic.h).
   * \return false if the qdisc is already running; otherwise, set the qdisc as running and return true.
   */
  bool RunBegin (void);

  /**
   * Modelled after the Linux function qdisc_run_end (include/net/sch_generic.h).
   * Set the qdisc as not running.
   */
  void RunEnd (void);

  /**
   * Modelled after the Linux function qdisc_restart (net/sched/sch_generic.c)
   * Dequeue a packet (by calling DequeuePacket) and send it to the device (by calling Transmit).
   * \return true if a packet is successfully sent to the device.
   */
  bool Restart (void);

  /**
   * Modelled after the Linux function dequeue_skb (net/sched/sch_generic.c)
   * \return the requeued packet, if any, or the packet dequeued by the queue disc, otherwise.
   */
  Ptr<QueueDiscItem> DequeuePacket (void);

  /**
   * Modelled after the Linux function dev_requeue_skb (net/sched/sch_generic.c)
   * Requeues a packet whose transmission failed.
   * \param p the packet to requeue
   */
  void Requeue (Ptr<QueueDiscItem> p);

  /**
   * Modelled after the Linux function sch_direct_xmit (net/sched/sch_generic.c)
   * Sends a packet to the device and requeues it in case transmission fails.
   * \param p the packet to transmit
   * \return true if the transmission succeeded and the queue is not stopped
   */
  bool Transmit (Ptr<QueueDiscItem> p);

  static const uint32_t DEFAULT_QUOTA = 64; //!< Default quota (as in /proc/sys/net/core/dev_weight)

  TracedValue<uint32_t> m_nPackets; //!< Number of packets in the queue
  TracedValue<uint32_t> m_nBytes;   //!< Number of bytes in the queue

  uint32_t m_nTotalReceivedPackets; //!< Total received packets
  uint32_t m_nTotalReceivedBytes;   //!< Total received bytes
  uint32_t m_nTotalDroppedPackets;  //!< Total dropped packets
  uint32_t m_nTotalDroppedBytes;    //!< Total dropped bytes
  uint32_t m_nTotalRequeuedPackets; //!< Total requeued packets
  uint32_t m_nTotalRequeuedBytes;   //!< Total requeued bytes
  uint32_t m_quota;                 //!< Maximum number of packets dequeued in a qdisc run
  Ptr<NetDeviceQueue> m_devQueue;   //!< The NetDeviceQueue associated with this queue discipline
  bool m_running;                   //!< The queue disc is performing multiple dequeue operations
  Ptr<QueueDiscItem> m_requeued;           //!< The last packet that failed to be transmitted

  /// Traced callback: fired when a packet is enqueued
  TracedCallback<Ptr<const Packet> > m_traceEnqueue;
    /// Traced callback: fired when a packet is dequeued
  TracedCallback<Ptr<const Packet> > m_traceDequeue;
    /// Traced callback: fired when a packet is requeued
  TracedCallback<Ptr<const Packet> > m_traceRequeue;
  /// Traced callback: fired when a packet is dropped
  TracedCallback<Ptr<const Packet> > m_traceDrop;
};

} // namespace ns3

#endif /* QueueDisc */
