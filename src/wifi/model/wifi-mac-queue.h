/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MAC_QUEUE_H
#define WIFI_MAC_QUEUE_H

#include "qos-utils.h"
#include "wifi-mac-queue-container.h"
#include "wifi-mpdu.h"

#include "ns3/queue.h"

#include <functional>
#include <optional>
#include <unordered_map>

namespace ns3
{

class WifiMacQueueScheduler;

// The following explicit template instantiation declaration prevents modules
// including this header file from implicitly instantiating Queue<WifiMpdu>.
// This would cause python examples using wifi to crash at runtime with the
// following error message: "Trying to allocate twice the same UID:
// ns3::Queue<WifiMpdu>"
extern template class Queue<WifiMpdu, ns3::WifiMacQueueContainer>;

/**
 * @ingroup wifi
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
 *
 * Compiling python bindings fails if the namespace (ns3) is not
 * specified for WifiMacQueueContainerT.
 */
class WifiMacQueue : public Queue<WifiMpdu, ns3::WifiMacQueueContainer>
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     *
     * @param ac the Access Category of the packets stored in this queue
     */
    WifiMacQueue(AcIndex ac = AC_UNDEF);

    ~WifiMacQueue() override;

    /// allow the usage of iterators and const iterators
    using Queue<WifiMpdu, WifiMacQueueContainer>::ConstIterator;
    using Queue<WifiMpdu, WifiMacQueueContainer>::Iterator;
    using Queue<WifiMpdu, WifiMacQueueContainer>::IsEmpty;
    using Queue<WifiMpdu, WifiMacQueueContainer>::GetNPackets;
    using Queue<WifiMpdu, WifiMacQueueContainer>::GetNBytes;

    /**
     * Get the Access Category of the packets stored in this queue
     *
     * @return the Access Category of the packets stored in this queue
     */
    AcIndex GetAc() const;

    /**
     * Set the wifi MAC queue scheduler
     *
     * @param scheduler the wifi MAC queue scheduler
     */
    void SetScheduler(Ptr<WifiMacQueueScheduler> scheduler);

    /**
     * Set the maximum delay before the packet is discarded.
     *
     * @param delay the maximum delay
     */
    void SetMaxDelay(Time delay);
    /**
     * Return the maximum delay before the packet is discarded.
     *
     * @return the maximum delay
     */
    Time GetMaxDelay() const;

    /**
     * Enqueue the given Wifi MAC queue item at the <i>end</i> of the queue.
     *
     * @param item the Wifi MAC queue item to be enqueued at the end
     * @return true if success, false if the packet has been dropped
     */
    bool Enqueue(Ptr<WifiMpdu> item) override;
    /**
     * Dequeue the packet in the front of the queue.
     *
     * @return the packet
     */
    Ptr<WifiMpdu> Dequeue() override;
    /**
     * Dequeue the given MPDUs if they are stored in this queue.
     *
     * @param mpdus the given MPDUs
     */
    void DequeueIfQueued(const std::list<Ptr<const WifiMpdu>>& mpdus);
    /**
     * Peek the packet in the front of the queue. The packet is not removed.
     *
     * @return the packet
     */
    Ptr<const WifiMpdu> Peek() const override;
    /**
     * Peek the packet in the front of the queue for transmission on the given
     * link (if any). The packet is not removed.
     *
     * @param linkId the ID of the link onto which we can transmit a packet
     * @return the packet
     */
    Ptr<WifiMpdu> Peek(std::optional<uint8_t> linkId) const;
    /**
     * Search and return, if present in the queue, the first packet having the
     * receiver address equal to <i>dest</i>, and TID equal to <i>tid</i>.
     * If <i>item</i> is not a null pointer, the search starts from the packet
     * following <i>item</i> in the queue; otherwise, the search starts from the
     * head of the queue. This method does not remove the packet from the queue.
     * It is typically used by ns3::QosTxop in order to perform correct MSDU aggregation
     * (A-MSDU).
     *
     * @param tid the given TID
     * @param dest the given destination
     * @param item the item after which the search starts from
     *
     * @return the peeked packet or nullptr if no packet was found
     */
    Ptr<WifiMpdu> PeekByTidAndAddress(uint8_t tid,
                                      Mac48Address dest,
                                      Ptr<const WifiMpdu> item = nullptr) const;
    /**
     * Search and return the first packet present in the container queue identified
     * by the given queue ID. If <i>item</i> is a null pointer, the search starts from
     * the head of the container queue; MPDUs with expired lifetime at the head of the
     * container queue are ignored (and moved to the container queue storing MPDUs with
     * expired lifetime). If <i>item</i> is not a null pointer, the search starts from
     * the packet following <i>item</i> in the container queue (and we do not check for
     * expired lifetime because we assume that a previous call was made with a null
     * pointer as argument, which removed the MPDUs with expired lifetime).
     *
     * @param queueId the given queue ID
     * @param item the item after which the search starts from
     * @return the peeked packet or nullptr if no packet was found
     */
    Ptr<WifiMpdu> PeekByQueueId(const WifiContainerQueueId& queueId,
                                Ptr<const WifiMpdu> item = nullptr) const;

    /**
     * Return first available packet for transmission on the given link. If <i>item</i>
     * is not a null pointer, the search starts from the packet following <i>item</i>
     * in the queue; otherwise, the search starts from the head of the queue.
     * The packet is not removed from queue.
     *
     * @param linkId the ID of the given link
     * @param item the item after which the search starts from
     *
     * @return the peeked packet or nullptr if no packet was found
     */
    Ptr<WifiMpdu> PeekFirstAvailable(uint8_t linkId, Ptr<const WifiMpdu> item = nullptr) const;
    /**
     * Remove the packet in the front of the queue.
     *
     * @return the packet
     */
    Ptr<WifiMpdu> Remove() override;
    /**
     * Remove the given item from the queue and return the item following the
     * removed one, if any, or a null pointer otherwise. If <i>removeExpired</i> is
     * true, all the items in the queue from the head to the given position are
     * removed if their lifetime expired.
     *
     * @param item the item to be removed
     * @return the removed item
     */
    Ptr<WifiMpdu> Remove(Ptr<const WifiMpdu> item);

    /**
     * Flush the queue.
     */
    void Flush();

    /**
     * Replace the given current item with the given new item. Actually, the current
     * item is dequeued and the new item is enqueued in its place. In this way,
     * statistics about queue size (in terms of bytes) are correctly updated.
     *
     * @param currentItem the given current item
     * @param newItem the given new item
     */
    void Replace(Ptr<const WifiMpdu> currentItem, Ptr<WifiMpdu> newItem);

    /**
     * Get the number of MPDUs currently stored in the container queue identified
     * by the given queue ID.
     *
     * @param queueId the given queue ID
     * @return the number of MPDUs currently stored in the container queue
     */
    uint32_t GetNPackets(const WifiContainerQueueId& queueId) const;

    /**
     * Get the amount of bytes currently stored in the container queue identified
     * by the given queue ID.
     *
     * @param queueId the given queue ID
     * @return the amount of bytes currently stored in the container queue
     */
    uint32_t GetNBytes(const WifiContainerQueueId& queueId) const;

    /**
     * Remove the given item if it has been in the queue for too long. Return true
     * if the item is removed, false otherwise.
     *
     * @param item the item whose lifetime is checked
     * @param now a copy of Simulator::Now()
     * @return true if the item is removed, false otherwise
     */
    bool TtlExceeded(Ptr<const WifiMpdu> item, const Time& now);

    /**
     * Move MPDUs with expired lifetime from the container queue identified by the
     * given queue ID to the container queue storing MPDUs with expired lifetime.
     * Each MPDU that is found to have an expired lifetime feeds the "Expired"
     * trace source and is notified to the scheduler.
     * @note that such MPDUs are not removed from the WifiMacQueue (and hence are
     * still accounted for in the overall statistics kept by the Queue base class)
     * in order to make this method const.
     *
     * @param queueId the given queue ID
     */
    void ExtractExpiredMpdus(const WifiContainerQueueId& queueId) const;
    /**
     * Move MPDUs with expired lifetime from all the container queues to the
     * container queue storing MPDUs with expired lifetime. Each MPDU that is found
     * to have an expired lifetime feeds the "Expired" trace source and is notified
     * to the scheduler.
     * @note that such MPDUs are not removed from the WifiMacQueue (and hence are
     * still accounted for in the overall statistics kept by the Queue base class)
     * in order to make this method const.
     */
    void ExtractAllExpiredMpdus() const;
    /**
     * Remove all MPDUs with expired lifetime from this WifiMacQueue object.
     */
    void WipeAllExpiredMpdus();

    /**
     * Unlike the GetOriginal() method of WifiMpdu, this method returns a non-const
     * pointer to the original copy of the given MPDU.
     *
     * @param mpdu the given MPDU
     * @return the original copy of the given MPDU
     */
    Ptr<WifiMpdu> GetOriginal(Ptr<WifiMpdu> mpdu);

    /**
     * @param mpdu the given MPDU
     * @param linkId the ID of the given link
     * @return the alias of the given MPDU that is inflight on the given link, if any, or
     *         a null pointer, otherwise
     */
    Ptr<WifiMpdu> GetAlias(Ptr<const WifiMpdu> mpdu, uint8_t linkId);

  protected:
    using Queue<WifiMpdu, WifiMacQueueContainer>::GetContainer;

    void DoDispose() override;

  private:
    /**
     * @param mpdu the given MPDU
     * @return the queue iterator stored by the given MPDU
     */
    Iterator GetIt(Ptr<const WifiMpdu> mpdu) const;

    /**
     * Enqueue the given Wifi MAC queue item before the given position.
     *
     * @param pos the position before which the item is to be inserted
     * @param item the Wifi MAC queue item to be enqueued
     * @return true if success, false if the packet has been dropped
     */
    bool Insert(ConstIterator pos, Ptr<WifiMpdu> item);
    /**
     * Wrapper for the DoEnqueue method provided by the base class that additionally
     * sets the iterator field of the item and updates internal statistics, if
     * insertion succeeded.
     *
     * @param pos the position before where the item will be inserted
     * @param item the item to enqueue
     * @return true if success, false if the packet has been dropped.
     */
    bool DoEnqueue(ConstIterator pos, Ptr<WifiMpdu> item);
    /**
     * Wrapper for the DoDequeue method provided by the base class that additionally
     * resets the iterator field of the dequeued items and notifies the scheduler, if
     * any item was dequeued.
     *
     * @param iterators the list of iterators pointing to the items to dequeue
     */
    void DoDequeue(const std::list<ConstIterator>& iterators);
    /**
     * Wrapper for the DoRemove method provided by the base class that additionally
     * resets the iterator field of the item and notifies the scheduleer, if an
     * item was dropped.
     *
     * @param pos the position of the item to drop
     * @return the dropped item.
     */
    Ptr<WifiMpdu> DoRemove(ConstIterator pos);

    Time m_maxDelay;                        //!< Time to live for packets in the queue
    AcIndex m_ac;                           //!< the access category
    Ptr<WifiMacQueueScheduler> m_scheduler; //!< the MAC queue scheduler

    /// Traced callback: fired when a packet is dropped due to lifetime expiration
    TracedCallback<Ptr<const WifiMpdu>> m_traceExpired;

    NS_LOG_TEMPLATE_DECLARE; //!< redefinition of the log component
};

} // namespace ns3

#endif /* WIFI_MAC_QUEUE_H */
