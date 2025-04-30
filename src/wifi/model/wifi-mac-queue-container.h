/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef WIFI_MAC_QUEUE_CONTAINER_H
#define WIFI_MAC_QUEUE_CONTAINER_H

#include "wifi-mac-queue-elem.h"
#include "wifi-utils.h"

#include "ns3/deprecated.h"
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

/// enumeration of frame types based on receiver address
enum class WifiRcvAddr : uint8_t
{
    UNICAST = 0,
    BROADCAST,
    GROUPCAST,
    COUNT
};

/**
 * Deprecated frame types enums.
 *
 * Use `WifiRcvAddr` class enum values instead.
 * @{
 */
NS_DEPRECATED_3_46("Use WifiRcvAddr::UNICAST instead")
static constexpr auto WIFI_UNICAST = WifiRcvAddr::UNICAST;
NS_DEPRECATED_3_46("Use WifiRcvAddr::BROADCAST instead")
static constexpr auto WIFI_BROADCAST = WifiRcvAddr::BROADCAST;
NS_DEPRECATED_3_46("Use WifiRcvAddr::GROUPCAST instead")
static constexpr auto WIFI_GROUPCAST = WifiRcvAddr::GROUPCAST;

/**@}*/

/**
 * Structure identifying a container queue.
 *
 * - for container queue types holding unicast frames, the Receiver Address (RA) of the frames
 *   stored in the queue needs to be specified. For 11be MLDs, it is expected that:
 *   + the RA of unicast management frames are link addresses (indicating the link on which
 *     they must be sent)
 *   + the RA of unicast QoS data frames are MLD addresses (indicating that they can be sent
 *     on any link)
 *   + if the RA of a unicast control frame is a link address, that control frame can only be
 *     sent on the corresponding link; if the RA is an MLD address, that control frame can be
 *     sent on any link
 *
 * - for container queue types holding broadcast frames, the Transmitter Address (TA) of the frames
 *   stored in the queue needs to be specified. For 11be MLDs, it is expected that:
 *   + the TA of broadcast management frames are link addresses (indicating the link on which
 *     they must be sent)
 *   + the TA of broadcast QoS data frames are MLD addresses (indicating that they can be sent
 *     on any link)
 *   + if the TA of a broadcast control frame is a link address, that control frame can only be
 *     sent on the corresponding link; if the TA is an MLD address, that control frame can be
 *     sent on any link
 *
 * - for container queue types holding groupcast frames, both the RA and TA of the frames
 *   stored in the queue need to be specified. The RA is used to identify the group address, while
 *   the TA is used to identify the link on which the frames must be sent (and is always a link
 *   address).
 *
 * The TID is only specified for container queue types holding QoS data frames.
 */
struct WifiContainerQueueId
{
    /**
     * Constructor.
     * @param type the container queue type
     * @param addrType the type of receiver address (unicast, broadcast, or groupcast)
     * @param addr1 the RA for unicast and groupcast queues, or nullopt otherwise
     * @param addr2 the TA for broadcast and groupcast queues, or nullopt otherwise
     * @param tid the TID for QoS data queues, or nullopt otherwise
     */
    WifiContainerQueueId(WifiContainerQueueType type,
                         WifiRcvAddr addrType,
                         std::optional<Mac48Address> addr1,
                         std::optional<Mac48Address> addr2,
                         std::optional<tid_t> tid)
        : type(type),
          addrType(addrType),
          addr1(addr1),
          addr2(addr2),
          tid(tid)
    {
    }

    /**
     * Three-way comparison operator
     *
     * @param rhs right hand side
     * @return deduced comparison type
     */
    auto operator<=>(const WifiContainerQueueId& rhs) const = default;

    WifiContainerQueueType type;       ///< the container queue type
    WifiRcvAddr addrType;              ///< the type of receiver address
    std::optional<Mac48Address> addr1; ///< the receiver address for unicast and groupcast queues,
                                       ///< or nullopt otherwise
    std::optional<Mac48Address> addr2; ///< the transmitter address for broadcast and groupcast
                                       ///< queues, or nullopt otherwise
    std::optional<tid_t> tid;          ///< the TID for QoS data queues, or nullopt otherwise
};

/**
 * @ingroup wifi
 * Helper function to create WifiContainerQueueId for unicast queues.
 *
 * @param type the container queue type
 * @param addr1 the receiver address for unicast queues
 * @param tid the TID for QoS data queues, or nullopt otherwise
 * @return the created WifiContainerQueueId
 */
inline WifiContainerQueueId
MakeWifiUnicastQueueId(WifiContainerQueueType type,
                       Mac48Address addr1,
                       std::optional<tid_t> tid = std::nullopt)
{
    return WifiContainerQueueId(type, WifiRcvAddr::UNICAST, addr1, std::nullopt, tid);
}

/**
 * @ingroup wifi
 * Helper function to create WifiContainerQueueId for broadcast queues.
 *
 * @param type the container queue type
 * @param addr2 the transmitter address for broadcast queues
 * @param tid the TID for QoS data queues, or nullopt otherwise
 * @return the created WifiContainerQueueId
 */
inline WifiContainerQueueId
MakeWifiBroadcastQueueId(WifiContainerQueueType type,
                         Mac48Address addr2,
                         std::optional<tid_t> tid = std::nullopt)
{
    return WifiContainerQueueId(type, WifiRcvAddr::BROADCAST, std::nullopt, addr2, tid);
}

/**
 * @ingroup wifi
 * Helper function to create WifiContainerQueueId for groupcast queues.
 *
 * @param type the container queue type
 * @param addr1 the receiver address for groupcast queues
 * @param addr2 the transmitter address for groupcast queues (always a link address)
 * @param tid the TID for QoS data queues, or nullopt otherwise
 * @return the created WifiContainerQueueId
 */
inline WifiContainerQueueId
MakeWifiGroupcastQueueId(WifiContainerQueueType type,
                         Mac48Address addr1,
                         Mac48Address addr2,
                         std::optional<tid_t> tid = std::nullopt)
{
    return WifiContainerQueueId(type, WifiRcvAddr::GROUPCAST, addr1, addr2, tid);
}

} // namespace ns3

/****************************************************
 *      Global Functions (outside namespace ns3)
 ***************************************************/

/**
 * @ingroup wifi
 * Hashing functor taking a QueueId and returning a @c std::size_t.
 * For use with `unordered_map` and `unordered_set`.
 */
template <>
struct std::hash<ns3::WifiContainerQueueId>
{
    /**
     * The functor.
     * @param queueId The QueueId value to hash.
     * @return the hash
     */
    std::size_t operator()(ns3::WifiContainerQueueId queueId) const;
};

namespace ns3
{

/**
 * @ingroup wifi
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
     * @param pos iterator before which the item will be inserted
     * @param item the item to insert in the container
     * @return iterator pointing to the inserted item
     */
    iterator insert(const_iterator pos, Ptr<WifiMpdu> item);

    /**
     * Erase the specified elements from the container.
     *
     * @param pos iterator to the element to remove
     * @return iterator following the removed element
     */
    iterator erase(const_iterator pos);

    /**
     * Return the WifiMpdu included in the element pointed to by the given iterator.
     *
     * @param it the given iterator
     * @return the item included in the element pointed to by the given iterator
     */
    Ptr<WifiMpdu> GetItem(const const_iterator it) const;

    /**
     * Return the QueueId identifying the container queue in which the given MPDU is
     * (or is to be) enqueued. Note that the given MPDU must not contain a control frame.
     *
     * @param mpdu the given MPDU
     * @return the QueueId identifying the container queue in which the given MPDU
     *         is (or is to be) enqueued
     */
    static WifiContainerQueueId GetQueueId(Ptr<const WifiMpdu> mpdu);

    /**
     * Get a const reference to the container queue identified by the given QueueId.
     * The container queue is created if it does not exist.
     *
     * @param queueId the given QueueId
     * @return a const reference to the container queue identified by the given QueueId
     */
    const ContainerQueue& GetQueue(const WifiContainerQueueId& queueId) const;

    /**
     * Get the total size of the MPDUs stored in the queue identified by the given QueueId.
     *
     * @param queueId the given queue ID
     * @return true if the given queue does not exist in the container or is empty,
     *         false otherwise
     */
    uint32_t GetNBytes(const WifiContainerQueueId& queueId) const;

    /**
     * Transfer non-inflight MPDUs with expired lifetime in the container queue identified by
     * the given QueueId to the container queue storing MPDUs with expired lifetime.
     *
     * @param queueId the QueueId identifying the container queue
     * @return the range [first, last) of iterators pointing to the MPDUs transferred
     *         to the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> ExtractExpiredMpdus(const WifiContainerQueueId& queueId) const;
    /**
     * Transfer non-inflight MPDUs with expired lifetime in all the container queues to the
     * container queue storing MPDUs with expired lifetime.
     *
     * @return the range [first, last) of iterators pointing to the MPDUs transferred
     *         to the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> ExtractAllExpiredMpdus() const;
    /**
     * Get the range [first, last) of iterators pointing to all the MPDUs queued
     * in the container queue storing MPDUs with expired lifetime.
     *
     * @return the range [first, last) of iterators pointing to all the MPDUs queued
     *         in the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> GetAllExpiredMpdus() const;

  private:
    /**
     * Transfer non-inflight MPDUs with expired lifetime in the given container queue to the
     * container queue storing MPDUs with expired lifetime.
     *
     * @param queue the given container queue
     * @return the range [first, last) of iterators pointing to the MPDUs transferred
     *         to the container queue storing MPDUs with expired lifetime
     */
    std::pair<iterator, iterator> DoExtractExpiredMpdus(ContainerQueue& queue) const;

    mutable std::unordered_map<WifiContainerQueueId, ContainerQueue>
        m_queues;                          //!< the container queues
    mutable ContainerQueue m_expiredQueue; //!< queue storing MPDUs with expired lifetime
    mutable std::unordered_map<WifiContainerQueueId, uint32_t>
        m_nBytesPerQueue; //!< size in bytes of the container queues
};

/**
 * @brief Stream insertion operator.
 * @param [in] os the reference to the output stream
 * @param [in] queueType the container queue type
 * @return a reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, WifiContainerQueueType queueType);

/**
 * @brief Stream insertion operator.
 * @param [in] os the reference to the output stream
 * @param [in] rcvAddrType the receiver address type
 * @return a reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, WifiRcvAddr rcvAddrType);

/**
 * @brief Stream insertion operator.
 * @param [in] os the reference to the output stream
 * @param [in] queueId the container queue ID
 * @return a reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, const WifiContainerQueueId& queueId);

} // namespace ns3

#endif /* WIFI_MAC_QUEUE_CONTAINER_H */
