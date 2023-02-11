/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MAC_QUEUE_CONTAINER_H
#define WIFI_MAC_QUEUE_CONTAINER_H

#include "wifi-mac-queue-elem.h"

#include "ns3/mac48-address.h"

#include <list>
#include <optional>
#include <tuple>
#include <unordered_map>

namespace ns3
{

/// enumeration of container queue types
enum WifiContainerQueueType
{
    WIFI_CTL_QUEUE = 0,
    WIFI_MGT_QUEUE = 1,
    WIFI_QOSDATA_QUEUE = 2,
    WIFI_DATA_QUEUE = 3
};

/// enumeration of frame directions
enum WifiReceiverAddressType : uint8_t
{
    WIFI_UNICAST = 0,
    WIFI_BROADCAST
};

/**
 * Tuple (queue type, receiver address type, Address, TID) identifying a container queue.
 *
 * \note that Address has a different meaning depending on container queue type:
 *
 * - for container queue types holding unicast frames, Address is the Receiver Address (RA)
 *   of the frames stored in the queue. For 11be MLDs, it is expected that:
 *   + the RA of unicast management frames are link addresses (indicating the link on which
 *     they must be sent)
 *   + the RA of unicast QoS data frames are MLD addresses (indicating that they can be sent
 *     on any link)
 *   + if the RA of a unicast control frame is a link address, that control frame can only be
 *     sent on the corresponding link; if the RA is an MLD address, that control frame can be
 *     sent on any link
 *
 * - for container queue types holding broadcast frames, Address is the Transmitter Address (TA)
 *   of the frames stored in the queue. For 11be MLDs, it is expected that:
 *   + the TA of broadcast management frames are link addresses (indicating the link on which
 *     they must be sent)
 *   + the TA of broadcast QoS data frames are MLD addresses (indicating that they can be sent
 *     on any link)
 *   + if the TA of a broadcast control frame is a link address, that control frame can only be
 *     sent on the corresponding link; if the TA is an MLD address, that control frame can be
 *     sent on any link
 *
 * The TID is only specified for container queue types holding QoS data frames.
 */
using WifiContainerQueueId = std::
    tuple<WifiContainerQueueType, WifiReceiverAddressType, Mac48Address, std::optional<uint8_t>>;

} // namespace ns3

/****************************************************
 *      Global Functions (outside namespace ns3)
 ***************************************************/

/**
 * \ingroup wifi
 * Hashing functor taking a QueueId and returning a @c std::size_t.
 * For use with `unordered_map` and `unordered_set`.
 */
template <>
struct std::hash<ns3::WifiContainerQueueId>
{
    /**
     * The functor.
     * \param queueId The QueueId value to hash.
     * \return the hash
     */
    std::size_t operator()(ns3::WifiContainerQueueId queueId) const;
};

namespace ns3
{

/**
 * \ingroup wifi
 * Class for the container used by WifiMacQueue
 *
 * This container holds multiple container queues organized in an hash table
 * whose keys are WifiContainerQueueId tuples identifying the container queues.
 */
class WifiMacQueueContainer
{
  public:
    /// Type of a queue held by the container
    using ContainerQueue = std::list<WifiMacQueueElem>;
    /// iterator over elements in a container queue
    using iterator = ContainerQueue::iterator;
    /// const iterator over elements in a container queue
    using const_iterator = ContainerQueue::const_iterator;

    /**
     * Erase all elements from the container.
     */
    void clear();

    /**
     * Insert the given item at the specified location in the container.
     *
     * \param pos iterator before which the item will be inserted
     * \param item the item to insert in the container
     * \return iterator pointing to the inserted item
     */
    iterator insert(const_iterator pos, Ptr<WifiMpdu> item);

    /**
     * Erase the specified elements from the container.
     *
     * \param pos iterator to the element to remove
     * \return iterator following the removed element
     */
    iterator erase(const_iterator pos);

    /**
     * Return the WifiMpdu included in the element pointed to by the given iterator.
     *
     * \param it the given iterator
     * \return the item included in the element pointed to by the given iterator
     */
    Ptr<WifiMpdu> GetItem(const const_iterator it) const;

    /**
     * Return the QueueId identifying the container queue in which the given MPDU is
     * (or is to be) enqueued. Note that the given MPDU must not contain a control frame.
     *
     * \param mpdu the given MPDU
     * \return the QueueId identifying the container queue in which the given MPDU
     *         is (or is to be) enqueued
     */
    static WifiContainerQueueId GetQueueId(Ptr<const WifiMpdu> mpdu);

    /**
     * Get a const reference to the container queue identified by the given QueueId.
     * The container queue is created if it does not exist.
     *
     * \param queueId the given QueueId
     * \return a const reference to the container queue identified by the given QueueId
     */
    const ContainerQueue& GetQueue(const WifiContainerQueueId& queueId) const;

    /**
     * Get the total size of the MPDUs stored in the queue identified by the given QueueId.
     *
     * \param queueId the given queue ID
     * \return true if the given queue does not exist in the container or is empty,
     *         false otherwise
     */
    uint32_t GetNBytes(const WifiContainerQueueId& queueId) const;

    /**
     * Transfer non-inflight MPDUs with expired lifetime in the container queue identified by
     * the given QueueId to the container queue storing MPDUs with expired lifetime.
     *
     * \param queueId the QueueId identifying the container queue
     * \return the range [first, last) of iterators pointing to the MPDUs transferred
     *         to the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> ExtractExpiredMpdus(const WifiContainerQueueId& queueId) const;
    /**
     * Transfer non-inflight MPDUs with expired lifetime in all the container queues to the
     * container queue storing MPDUs with expired lifetime.
     *
     * \return the range [first, last) of iterators pointing to the MPDUs transferred
     *         to the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> ExtractAllExpiredMpdus() const;
    /**
     * Get the range [first, last) of iterators pointing to all the MPDUs queued
     * in the container queue storing MPDUs with expired lifetime.
     *
     * \return the range [first, last) of iterators pointing to all the MPDUs queued
     *         in the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> GetAllExpiredMpdus() const;

  private:
    /**
     * Transfer non-inflight MPDUs with expired lifetime in the given container queue to the
     * container queue storing MPDUs with expired lifetime.
     *
     * \param queue the given container queue
     * \return the range [first, last) of iterators pointing to the MPDUs transferred
     *         to the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> DoExtractExpiredMpdus(ContainerQueue& queue) const;

    mutable std::unordered_map<WifiContainerQueueId, ContainerQueue>
        m_queues;                          //!< the container queues
    mutable ContainerQueue m_expiredQueue; //!< queue storing MPDUs with expired lifetime
    mutable std::unordered_map<WifiContainerQueueId, uint32_t>
        m_nBytesPerQueue; //!< size in bytes of the container queues
};

} // namespace ns3

#endif /* WIFI_MAC_QUEUE_CONTAINER_H */
