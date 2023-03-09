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
    WIFI_QOSDATA_UNICAST_QUEUE = 2,
    WIFI_QOSDATA_BROADCAST_QUEUE = 3,
    WIFI_DATA_QUEUE = 4
};

/**
 * Tuple (queue type, Address, TID) identifying a container queue.
 *
 * \note that Address has a different meaning depending on container queue type:
 * - if container queue type is WIFI_CTL_QUEUE, Address is the Transmitter Address
 * (TA) of the frames stored in the queue. We have distinct control queues
 * depending on TA to distinguish among control frames that need to be sent
 * over different links by 11be MLDs. MLD address as TA indicates that the frames
 * can be sent on any link. TID is ignored.
 * - if container queue type is WIFI_MGT_QUEUE, Address is the Transmitter Address
 * (TA) of the frames stored in the queue. We have distinct management queues
 * depending on TA to distinguish among management frames that need to be sent
 * over different links by 11be MLDs. TID is ignored.
 * - if container queue type is WIFI_QOSDATA_UNICAST_QUEUE, Address is the Receiver
 * Address (RA) of the frames stored in the queue.
 * - if container queue type is WIFI_QOSDATA_BROADCAST_QUEUE, Address is the
 * Transmitter Address (TA) of the frames stored in the queue. We have distinct
 * broadcast QoS queues depending on TA to distinguish among broadcast QoS Data
 * frames that need to be sent over different links by 11be MLDs.
 * - if container queue type is WIFI_DATA_QUEUE, Address is the Receiver Address
 * (RA) of the frames stored in the queue. We do not need to consider the
 * Transmitter Address (TA) because 11be stations are QoS stations and hence do
 * not send non-QoS Data frames. TID is ignored.
 */
using WifiContainerQueueId =
    std::tuple<WifiContainerQueueType, Mac48Address, std::optional<uint8_t>>;

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
