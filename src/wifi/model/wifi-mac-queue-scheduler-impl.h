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

#ifndef WIFI_MAC_QUEUE_SCHEDULER_IMPL_H
#define WIFI_MAC_QUEUE_SCHEDULER_IMPL_H

#include "wifi-mac-queue-scheduler.h"
#include "wifi-mac-queue.h"
#include "wifi-mac.h"

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <numeric>
#include <unordered_map>
#include <vector>

class WifiMacQueueDropOldestTest;

namespace ns3
{

class WifiMpdu;
class WifiMacQueue;

/**
 * \ingroup wifi
 *
 * WifiMacQueueSchedulerImpl is a template class enabling the definition of
 * different types of priority values for the container queues. The function to
 * compare priority values can be customized as well.
 */
template <class Priority, class Compare = std::less<Priority>>
class WifiMacQueueSchedulerImpl : public WifiMacQueueScheduler
{
  public:
    /// allow WifiMacQueueDropOldestTest class access
    friend class ::WifiMacQueueDropOldestTest;

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Constructor
     */
    WifiMacQueueSchedulerImpl();

    /** \copydoc ns3::WifiMacQueueScheduler::SetWifiMac */
    void SetWifiMac(Ptr<WifiMac> mac) final;
    /** \copydoc ns3::WifiMacQueueScheduler::GetNext(AcIndex,std::optional<uint8_t>) */
    std::optional<WifiContainerQueueId> GetNext(AcIndex ac, std::optional<uint8_t> linkId) final;
    /**
     *  \copydoc ns3::WifiMacQueueScheduler::GetNext(AcIndex,std::optional<uint8_t>,
     *           const WifiContainerQueueId&)
     */
    std::optional<WifiContainerQueueId> GetNext(AcIndex ac,
                                                std::optional<uint8_t> linkId,
                                                const WifiContainerQueueId& prevQueueId) final;
    /** \copydoc ns3::WifiMacQueueScheduler::GetLinkIds */
    std::list<uint8_t> GetLinkIds(AcIndex ac, Ptr<const WifiMpdu> mpdu) final;
    /** \copydoc ns3::WifiMacQueueScheduler::BlockQueues */
    void BlockQueues(WifiQueueBlockedReason reason,
                     AcIndex ac,
                     const std::list<WifiContainerQueueType>& types,
                     const Mac48Address& rxAddress,
                     const Mac48Address& txAddress,
                     const std::set<uint8_t>& tids,
                     const std::set<uint8_t>& linkIds) final;
    /** \copydoc ns3::WifiMacQueueScheduler::UnblockQueues */
    void UnblockQueues(WifiQueueBlockedReason reason,
                       AcIndex ac,
                       const std::list<WifiContainerQueueType>& types,
                       const Mac48Address& rxAddress,
                       const Mac48Address& txAddress,
                       const std::set<uint8_t>& tids,
                       const std::set<uint8_t>& linkIds) final;
    /** \copydoc ns3::WifiMacQueueScheduler::GetQueueLinkMask */
    std::optional<Mask> GetQueueLinkMask(AcIndex ac,
                                         const WifiContainerQueueId& queueId,
                                         uint8_t linkId) final;
    /** \copydoc ns3::WifiMacQueueScheduler::HasToDropBeforeEnqueue */
    Ptr<WifiMpdu> HasToDropBeforeEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu) final;
    /** \copydoc ns3::WifiMacQueueScheduler::NotifyEnqueue */
    void NotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu) final;
    /** \copydoc ns3::WifiMacQueueScheduler::NotifyDequeue */
    void NotifyDequeue(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) final;
    /** \copydoc ns3::WifiMacQueueScheduler::NotifyRemove */
    void NotifyRemove(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) final;

  protected:
    /** \copydoc ns3::Object::DoDispose */
    void DoDispose() override;

    /**
     * Set the priority for the given container queue belonging to the given Access Category.
     *
     * \param ac the Access Category of the container queue
     * \param queueId the ID of the given container queue
     * \param priority the priority value
     */
    void SetPriority(AcIndex ac, const WifiContainerQueueId& queueId, const Priority& priority);

    struct QueueInfo;

    /**
     * Map identifiers (QueueIds) to information associated with container queues.
     *
     * Empty queues shall be kept in this data structure because queue information
     * (such as the set of link IDs) may be configured just once.
     */
    using QueueInfoMap = std::unordered_map<WifiContainerQueueId, QueueInfo>;

    /// typedef for a QueueInfoMap element
    using QueueInfoPair = std::pair<const WifiContainerQueueId, QueueInfo>;

    /**
     * List of container queues sorted in decreasing order of priority.
     *
     * Empty queues shall not be kept in this data structure.
     *
     * \note We cannot store iterators to QueueInfoMap because if rehashing occurs due
     *       to an insertion, all iterators are invalidated. References are not invalidated
     *       instead. Therefore, we store reference wrappers (which can be reassigned).
     */
    using SortedQueues = std::multimap<Priority, std::reference_wrapper<QueueInfoPair>, Compare>;

    /**
     * Information associated with a container queue.
     */
    struct QueueInfo
    {
        std::optional<typename SortedQueues::iterator>
            priorityIt;                  /**< iterator pointing to the entry
                                              for this queue in the sorted list */
        std::map<uint8_t, Mask> linkIds; /**< Maps ID of each link on which packets contained
                                              in this queue can be sent to a bitset indicating
                                              whether the link is blocked (at least one bit is
                                              non-zero) and for which reason */
    };

    /**
     * Information specific to a wifi MAC queue
     */
    struct PerAcInfo
    {
        SortedQueues sortedQueues;      //!< sorted list of container queues
        QueueInfoMap queueInfoMap;      //!< information associated with container queues
        Ptr<WifiMacQueue> wifiMacQueue; //!< pointer to the WifiMacQueue object
    };

    /**
     * Get a const reference to the sorted list of container queues for the given
     * Access Category.
     *
     * \param ac the given Access Category
     * \return a const reference to the sorted list of container queues for the given Access
     * Category
     */
    const SortedQueues& GetSortedQueues(AcIndex ac) const;

    /**
     * Get the wifi MAC queue associated with the given Access Category.
     *
     * \param ac the given Access Category
     * \return the wifi MAC queue associated with the given Access Category
     */
    Ptr<WifiMacQueue> GetWifiMacQueue(AcIndex ac) const;

  private:
    /**
     * If no information for the container queue used to store the given MPDU of the given
     * Access Category is present in the queue info map, add the information for such a
     * container queue and initialize the list of the IDs of the links over which packets
     * contained in that container queue can be sent.
     *
     * \param ac the given Access Category
     * \param mpdu the given MPDU
     * \return an iterator to the information associated with the container queue used to
     *         store the given MPDU of the given Access Category
     */
    typename QueueInfoMap::iterator InitQueueInfo(AcIndex ac, Ptr<const WifiMpdu> mpdu);

    /**
     * Get the next queue to serve. The search starts from the given one. The returned
     * queue is guaranteed to contain at least an MPDU whose lifetime has not expired.
     * Queues containing MPDUs that cannot be sent over the given link are ignored.
     *
     * \param ac the Access Category that we want to serve
     * \param linkId the ID of the link on which MPDUs contained in the returned queue must be
     *               allowed to be sent
     * \param sortedQueuesIt iterator pointing to the queue we start the search from
     * \return the ID of the selected container queue (if any)
     */
    std::optional<WifiContainerQueueId> DoGetNext(AcIndex ac,
                                                  std::optional<uint8_t> linkId,
                                                  typename SortedQueues::iterator sortedQueuesIt);

    /**
     * Check whether an MPDU has to be dropped before enqueuing the given MPDU.
     *
     * \param ac the Access Category of the MPDU being enqueued
     * \param mpdu the MPDU to enqueue
     * \return a pointer to the MPDU to drop, if any, or a null pointer, otherwise
     */
    virtual Ptr<WifiMpdu> HasToDropBeforeEnqueuePriv(AcIndex ac, Ptr<WifiMpdu> mpdu) = 0;
    /**
     * Notify the scheduler that the given MPDU has been enqueued by the given Access
     * Category. The container queue in which the MPDU has been enqueued must be
     * assigned a priority value.
     *
     * \param ac the Access Category of the enqueued MPDU
     * \param mpdu the enqueued MPDU
     */
    virtual void DoNotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu) = 0;
    /**
     * Notify the scheduler that the given list of MPDUs have been dequeued by the
     * given Access Category. The container queues which became empty after dequeuing
     * the MPDUs are removed from the sorted list of queues.
     *
     * \param ac the Access Category of the dequeued MPDUs
     * \param mpdus the list of dequeued MPDUs
     */
    virtual void DoNotifyDequeue(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) = 0;
    /**
     * Notify the scheduler that the given list of MPDUs have been removed by the
     * given Access Category. The container queues which became empty after removing
     * the MPDUs are removed from the sorted list of queues.
     *
     * \param ac the Access Category of the removed MPDUs
     * \param mpdus the list of removed MPDUs
     */
    virtual void DoNotifyRemove(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) = 0;

    /**
     * Block or unblock the given set of links for the container queues of the given types and
     * Access Category that hold frames having the given Receiver Address (RA),
     * Transmitter Address (TA) and TID (if needed) for the given reason.
     *
     * \param block true to block the queues, false to unblock
     * \param reason the reason for blocking the queues
     * \param ac the given Access Category
     * \param types the types of the queues to block
     * \param rxAddress the Receiver Address (RA) of the frames
     * \param txAddress the Transmitter Address (TA) of the frames
     * \param tids the TIDs optionally identifying the queues to block
     * \param linkIds set of links to block (empty to block all setup links)
     */
    void DoBlockQueues(bool block,
                       WifiQueueBlockedReason reason,
                       AcIndex ac,
                       const std::list<WifiContainerQueueType>& types,
                       const Mac48Address& rxAddress,
                       const Mac48Address& txAddress,
                       const std::set<uint8_t>& tids,
                       const std::set<uint8_t>& linkIds);

    std::vector<PerAcInfo> m_perAcInfo{AC_UNDEF}; //!< vector of per-AC information
    NS_LOG_TEMPLATE_DECLARE;                      //!< the log component
};

/**
 * Implementation of the templates declared above.
 */

template <class Priority, class Compare>
WifiMacQueueSchedulerImpl<Priority, Compare>::WifiMacQueueSchedulerImpl()
    : NS_LOG_TEMPLATE_DEFINE("WifiMacQueueScheduler")
{
}

template <class Priority, class Compare>
TypeId
WifiMacQueueSchedulerImpl<Priority, Compare>::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiMacQueueSchedulerImpl")
                            .SetParent<WifiMacQueueScheduler>()
                            .SetGroupName("Wifi");
    return tid;
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::DoDispose()
{
    m_perAcInfo.clear();
    WifiMacQueueScheduler::DoDispose();
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::SetWifiMac(Ptr<WifiMac> mac)
{
    for (auto ac : {AC_BE, AC_BK, AC_VI, AC_VO, AC_BE_NQOS, AC_BEACON})
    {
        if (auto queue = mac->GetTxopQueue(ac); queue != nullptr)
        {
            m_perAcInfo.at(ac).wifiMacQueue = queue;
            queue->SetScheduler(this);
        }
    }
    WifiMacQueueScheduler::SetWifiMac(mac);
}

template <class Priority, class Compare>
Ptr<WifiMacQueue>
WifiMacQueueSchedulerImpl<Priority, Compare>::GetWifiMacQueue(AcIndex ac) const
{
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);
    return m_perAcInfo.at(ac).wifiMacQueue;
}

template <class Priority, class Compare>
const typename WifiMacQueueSchedulerImpl<Priority, Compare>::SortedQueues&
WifiMacQueueSchedulerImpl<Priority, Compare>::GetSortedQueues(AcIndex ac) const
{
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);
    return m_perAcInfo.at(ac).sortedQueues;
}

template <class Priority, class Compare>
typename WifiMacQueueSchedulerImpl<Priority, Compare>::QueueInfoMap::iterator
WifiMacQueueSchedulerImpl<Priority, Compare>::InitQueueInfo(AcIndex ac, Ptr<const WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << ac << *mpdu);

    auto queueId = WifiMacQueueContainer::GetQueueId(mpdu);
    // insert queueId in the queue info map if not present yet
    auto [queueInfoIt, ret] = m_perAcInfo[ac].queueInfoMap.insert({queueId, QueueInfo()});

    // Initialize/update the set of link IDs depending on the container queue type
    if (GetMac() && GetMac()->GetNLinks() > 1 &&
        mpdu->GetHeader().GetAddr2() == GetMac()->GetAddress())
    {
        // this is an MLD and the TA field of the frame contains the MLD address,
        // which means that the frame can be sent on multiple links
        const auto rxAddr = mpdu->GetHeader().GetAddr1();

        // this assert checks that the RA field also contain an MLD address, unless
        // it contains the broadcast address
        NS_ASSERT_MSG(rxAddr.IsGroup() || GetMac()->GetMldAddress(rxAddr) == rxAddr,
                      "Address 1 (" << rxAddr << ") is not an MLD address");

        // this assert checks that association (ML setup) has been established
        // between sender and receiver (unless the receiver is the broadcast address)
        NS_ASSERT_MSG(GetMac()->CanForwardPacketsTo(rxAddr), "Cannot forward frame to " << rxAddr);
        // we have to include all the links in case of broadcast frame (we are an AP)
        // and the links that have been setup with the receiver in case of unicast frame
        for (const auto linkId : GetMac()->GetLinkIds())
        {
            if (rxAddr.IsGroup() ||
                GetMac()->GetWifiRemoteStationManager(linkId)->GetAffiliatedStaAddress(rxAddr))
            {
                // the mask is not modified if linkId is already in the map
                queueInfoIt->second.linkIds.emplace(linkId, Mask{});
            }
            else
            {
                // this link is no (longer) setup
                queueInfoIt->second.linkIds.erase(linkId);
            }
        }
    }
    else
    {
        // the TA field of the frame contains a link address, which means that the
        // frame can only be sent on the corresponding link
        auto linkId = GetMac() ? GetMac()->GetLinkIdByAddress(mpdu->GetHeader().GetAddr2())
                               : SINGLE_LINK_OP_ID; // make unit test happy
        NS_ASSERT(linkId.has_value());
        auto& linkIdsMap = queueInfoIt->second.linkIds;
        NS_ASSERT_MSG(linkIdsMap.size() <= 1,
                      "At most one link can be associated with this container queue");
        // set the link map to contain one entry corresponding to the computed link ID;
        // unless the link map already contained such an entry (in which case the mask
        // is preserved)
        if (linkIdsMap.empty() || linkIdsMap.cbegin()->first != *linkId)
        {
            linkIdsMap = {{*linkId, Mask{}}};
        }
    }

    return queueInfoIt;
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::SetPriority(AcIndex ac,
                                                          const WifiContainerQueueId& queueId,
                                                          const Priority& priority)
{
    NS_LOG_FUNCTION(this << +ac);
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);

    NS_ABORT_MSG_IF(GetWifiMacQueue(ac)->GetNBytes(queueId) == 0,
                    "Cannot set the priority of an empty queue");

    auto queueInfoIt = m_perAcInfo[ac].queueInfoMap.find(queueId);
    NS_ASSERT_MSG(queueInfoIt != m_perAcInfo[ac].queueInfoMap.end(),
                  "No queue info for the given container queue");
    typename SortedQueues::iterator sortedQueuesIt;

    if (queueInfoIt->second.priorityIt.has_value())
    {
        // an element for queueId is present in the set of sorted queues. If the priority
        // has not changed, do nothing. Otherwise, unlink the node containing such element,
        // change the priority and insert it back
        if (queueInfoIt->second.priorityIt.value()->first == priority)
        {
            return;
        }

        auto handle = m_perAcInfo[ac].sortedQueues.extract(queueInfoIt->second.priorityIt.value());
        handle.key() = priority;
        sortedQueuesIt = m_perAcInfo[ac].sortedQueues.insert(std::move(handle));
    }
    else
    {
        // an element for queueId is not present in the set of sorted queues
        sortedQueuesIt = m_perAcInfo[ac].sortedQueues.insert({priority, std::ref(*queueInfoIt)});
    }
    // update the stored iterator
    queueInfoIt->second.priorityIt = sortedQueuesIt;
}

template <class Priority, class Compare>
std::list<uint8_t>
WifiMacQueueSchedulerImpl<Priority, Compare>::GetLinkIds(AcIndex ac, Ptr<const WifiMpdu> mpdu)
{
    auto queueInfoIt = InitQueueInfo(ac, mpdu);
    std::list<uint8_t> linkIds;

    // include only links that are not blocked in the returned list
    for (const auto [linkId, mask] : queueInfoIt->second.linkIds)
    {
        if (mask.none())
        {
            linkIds.emplace_back(linkId);
        }
    }

    return linkIds;
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::DoBlockQueues(
    bool block,
    WifiQueueBlockedReason reason,
    AcIndex ac,
    const std::list<WifiContainerQueueType>& types,
    const Mac48Address& rxAddress,
    const Mac48Address& txAddress,
    const std::set<uint8_t>& tids,
    const std::set<uint8_t>& linkIds)
{
    NS_LOG_FUNCTION(this << block << reason << ac << rxAddress << txAddress);
    std::list<WifiMacHeader> headers;

    for (const auto queueType : types)
    {
        switch (queueType)
        {
        case WIFI_CTL_QUEUE:
            headers.emplace_back(WIFI_MAC_CTL_BACKREQ);
            break;
        case WIFI_MGT_QUEUE:
            headers.emplace_back(WIFI_MAC_MGT_ACTION);
            break;
        case WIFI_QOSDATA_QUEUE:
            NS_ASSERT_MSG(!tids.empty(),
                          "TID must be specified for queues containing QoS data frames");
            for (const auto tid : tids)
            {
                headers.emplace_back(WIFI_MAC_QOSDATA);
                headers.back().SetQosTid(tid);
            }
            break;
        case WIFI_DATA_QUEUE:
            headers.emplace_back(WIFI_MAC_DATA);
            break;
        }
    }
    for (auto& hdr : headers)
    {
        hdr.SetAddr1(rxAddress);
        hdr.SetAddr2(txAddress);

        auto queueInfoIt = InitQueueInfo(ac, Create<WifiMpdu>(Create<Packet>(), hdr));
        for (auto& [linkId, mask] : queueInfoIt->second.linkIds)
        {
            if (linkIds.empty() || linkIds.count(linkId) > 0)
            {
                mask.set(static_cast<std::size_t>(reason), block);
            }
        }
    }
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::BlockQueues(
    WifiQueueBlockedReason reason,
    AcIndex ac,
    const std::list<WifiContainerQueueType>& types,
    const Mac48Address& rxAddress,
    const Mac48Address& txAddress,
    const std::set<uint8_t>& tids,
    const std::set<uint8_t>& linkIds)
{
    DoBlockQueues(true, reason, ac, types, rxAddress, txAddress, tids, linkIds);
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::UnblockQueues(
    WifiQueueBlockedReason reason,
    AcIndex ac,
    const std::list<WifiContainerQueueType>& types,
    const Mac48Address& rxAddress,
    const Mac48Address& txAddress,
    const std::set<uint8_t>& tids,
    const std::set<uint8_t>& linkIds)
{
    DoBlockQueues(false, reason, ac, types, rxAddress, txAddress, tids, linkIds);
}

template <class Priority, class Compare>
std::optional<WifiMacQueueScheduler::Mask>
WifiMacQueueSchedulerImpl<Priority, Compare>::GetQueueLinkMask(AcIndex ac,
                                                               const WifiContainerQueueId& queueId,
                                                               uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +ac << +linkId);

    const auto queueInfoIt = m_perAcInfo[ac].queueInfoMap.find(queueId);

    if (queueInfoIt == m_perAcInfo[ac].queueInfoMap.cend())
    {
        // the given container queue does not exist
        return std::nullopt;
    }

    const auto& linkIds = queueInfoIt->second.linkIds;
    if (const auto linkIt = linkIds.find(linkId); linkIt != linkIds.cend())
    {
        return linkIt->second;
    }

    return std::nullopt;
}

template <class Priority, class Compare>
std::optional<WifiContainerQueueId>
WifiMacQueueSchedulerImpl<Priority, Compare>::GetNext(AcIndex ac, std::optional<uint8_t> linkId)
{
    NS_LOG_FUNCTION(this << +ac << linkId.has_value());
    return DoGetNext(ac, linkId, m_perAcInfo[ac].sortedQueues.begin());
}

template <class Priority, class Compare>
std::optional<WifiContainerQueueId>
WifiMacQueueSchedulerImpl<Priority, Compare>::GetNext(AcIndex ac,
                                                      std::optional<uint8_t> linkId,
                                                      const WifiContainerQueueId& prevQueueId)
{
    NS_LOG_FUNCTION(this << +ac << linkId.has_value());

    auto queueInfoIt = m_perAcInfo[ac].queueInfoMap.find(prevQueueId);
    NS_ABORT_IF(queueInfoIt == m_perAcInfo[ac].queueInfoMap.end() ||
                !queueInfoIt->second.priorityIt.has_value());

    auto sortedQueuesIt = queueInfoIt->second.priorityIt.value();
    NS_ABORT_IF(sortedQueuesIt == m_perAcInfo[ac].sortedQueues.end());

    return DoGetNext(ac, linkId, ++sortedQueuesIt);
}

template <class Priority, class Compare>
std::optional<WifiContainerQueueId>
WifiMacQueueSchedulerImpl<Priority, Compare>::DoGetNext(
    AcIndex ac,
    std::optional<uint8_t> linkId,
    typename SortedQueues::iterator sortedQueuesIt)
{
    NS_LOG_FUNCTION(this << +ac << linkId.has_value());
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);

    while (sortedQueuesIt != m_perAcInfo[ac].sortedQueues.end())
    {
        const auto& queueInfoPair = sortedQueuesIt->second.get();
        const auto& linkIds = queueInfoPair.second.linkIds;
        typename std::decay_t<decltype(linkIds)>::const_iterator linkIt;

        if (!linkId.has_value() ||
            ((linkIt = linkIds.find(*linkId)) != linkIds.cend() && linkIt->second.none()))
        {
            // Packets in this queue can be sent over the link we got channel access on.
            // Now remove packets with expired lifetime from this queue.
            // In case the queue becomes empty, the queue is removed from the sorted
            // list and sortedQueuesIt is invalidated; thus, store an iterator to the
            // previous queue in the sorted list (if any) to resume the search afterwards.
            std::optional<typename SortedQueues::iterator> prevQueueIt;
            if (sortedQueuesIt != m_perAcInfo[ac].sortedQueues.begin())
            {
                prevQueueIt = std::prev(sortedQueuesIt);
            }

            GetWifiMacQueue(ac)->ExtractExpiredMpdus(queueInfoPair.first);

            if (GetWifiMacQueue(ac)->GetNBytes(queueInfoPair.first) == 0)
            {
                sortedQueuesIt = (prevQueueIt.has_value() ? std::next(prevQueueIt.value())
                                                          : m_perAcInfo[ac].sortedQueues.begin());
                continue;
            }
            break;
        }

        sortedQueuesIt++;
    }

    std::optional<WifiContainerQueueId> queueId;

    if (sortedQueuesIt != m_perAcInfo[ac].sortedQueues.end())
    {
        queueId = sortedQueuesIt->second.get().first;
    }
    return queueId;
}

template <class Priority, class Compare>
Ptr<WifiMpdu>
WifiMacQueueSchedulerImpl<Priority, Compare>::HasToDropBeforeEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << +ac << *mpdu);
    return HasToDropBeforeEnqueuePriv(ac, mpdu);
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::NotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu)
{
    NS_LOG_FUNCTION(this << +ac << *mpdu);
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);

    // add information for the queue storing the MPDU to the queue info map, if not present yet
    auto queueInfoIt = InitQueueInfo(ac, mpdu);

    DoNotifyEnqueue(ac, mpdu);

    if (!queueInfoIt->second.priorityIt.has_value())
    {
        NS_ABORT_MSG(
            "No info for the queue the MPDU was stored into (forgot to call SetPriority()?)");
    }
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::NotifyDequeue(AcIndex ac,
                                                            const std::list<Ptr<WifiMpdu>>& mpdus)
{
    NS_LOG_FUNCTION(this << +ac);
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);

    DoNotifyDequeue(ac, mpdus);

    std::list<WifiContainerQueueId> queueIds;

    for (const auto& mpdu : mpdus)
    {
        queueIds.push_back(WifiMacQueueContainer::GetQueueId(mpdu));
    }

    for (const auto& queueId : queueIds)
    {
        if (GetWifiMacQueue(ac)->GetNBytes(queueId) == 0)
        {
            // The queue has now become empty and needs to be removed from the sorted
            // list kept by the scheduler
            auto queueInfoIt = m_perAcInfo[ac].queueInfoMap.find(queueId);
            NS_ASSERT(queueInfoIt != m_perAcInfo[ac].queueInfoMap.end());
            if (queueInfoIt->second.priorityIt.has_value())
            {
                m_perAcInfo[ac].sortedQueues.erase(queueInfoIt->second.priorityIt.value());
                queueInfoIt->second.priorityIt.reset();
            }
        }
    }
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::NotifyRemove(AcIndex ac,
                                                           const std::list<Ptr<WifiMpdu>>& mpdus)
{
    NS_LOG_FUNCTION(this << +ac);
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);

    DoNotifyRemove(ac, mpdus);

    std::list<WifiContainerQueueId> queueIds;

    for (const auto& mpdu : mpdus)
    {
        queueIds.push_back(WifiMacQueueContainer::GetQueueId(mpdu));
    }

    for (const auto& queueId : queueIds)
    {
        if (GetWifiMacQueue(ac)->GetNBytes(queueId) == 0)
        {
            // The queue has now become empty and needs to be removed from the sorted
            // list kept by the scheduler
            auto queueInfoIt = m_perAcInfo[ac].queueInfoMap.find(queueId);
            NS_ASSERT(queueInfoIt != m_perAcInfo[ac].queueInfoMap.end());
            if (queueInfoIt->second.priorityIt.has_value())
            {
                m_perAcInfo[ac].sortedQueues.erase(queueInfoIt->second.priorityIt.value());
                queueInfoIt->second.priorityIt.reset();
            }
        }
    }
}

} // namespace ns3

#endif /* WIFI_MAC_QUEUE_SCHEDULER_IMPL_H */
