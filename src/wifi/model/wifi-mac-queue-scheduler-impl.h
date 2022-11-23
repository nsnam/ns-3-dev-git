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
    /** \copydoc ns3::WifiMacQueueScheduler::GetNext(AcIndex,uint8_t) */
    std::optional<WifiContainerQueueId> GetNext(AcIndex ac, uint8_t linkId) final;
    /** \copydoc ns3::WifiMacQueueScheduler::GetNext(AcIndex,uint8_t,const WifiContainerQueueId&) */
    std::optional<WifiContainerQueueId> GetNext(AcIndex ac,
                                                uint8_t linkId,
                                                const WifiContainerQueueId& prevQueueId) final;
    /** \copydoc ns3::WifiMacQueueScheduler::GetLinkIds */
    std::list<uint8_t> GetLinkIds(AcIndex ac, const WifiContainerQueueId& queueId) final;
    /** \copydoc ns3::WifiMacQueueScheduler::SetLinkIds */
    void SetLinkIds(AcIndex ac,
                    const WifiContainerQueueId& queueId,
                    const std::list<uint8_t>& linkIds) final;
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
            priorityIt;             /**< iterator pointing to the entry
                                         for this queue in the sorted list */
        std::list<uint8_t> linkIds; /**< IDs of the links over which packets contained in this
                                         queue can be sent over. Empty means that packets in this
                                         queue can be sent over any link */
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
     * Add the information associated with the given container queue (if not already
     * present) to the map corresponding to the given Access Category and Initialize
     * the list of the IDs of the links over which packets contained in the given
     * container queue can be sent over.
     *
     * \param ac the given Access Category
     * \param queueId the ID of the given container queue
     * \return an iterator to the information associated with the given container queue
     */
    typename QueueInfoMap::iterator InitQueueInfo(AcIndex ac, const WifiContainerQueueId& queueId);

    /**
     * Get the next queue to serve. The search starts from the given one. The returned
     * queue is guaranteed to contain at least an MPDU whose lifetime has not expired.
     * Queues containing MPDUs that cannot be sent over the given link are ignored.
     *
     * \param ac the Access Category that we want to serve
     * \param linkId the ID of the link on which we got channel access
     * \param sortedQueuesIt iterator pointing to the queue we start the search from
     * \return the ID of the selected container queue (if any)
     */
    std::optional<WifiContainerQueueId> DoGetNext(AcIndex ac,
                                                  uint8_t linkId,
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
WifiMacQueueSchedulerImpl<Priority, Compare>::InitQueueInfo(AcIndex ac,
                                                            const WifiContainerQueueId& queueId)
{
    NS_LOG_FUNCTION(this);

    // insert queueId in the queue info map if not present yet
    auto [queueInfoIt, ret] = m_perAcInfo[ac].queueInfoMap.insert({queueId, QueueInfo()});

    if (ret)
    {
        // The given queueid has just been inserted in the queue info map.
        // Initialize the set of link IDs depending on the container queue type
        auto queueType = std::get<WifiContainerQueueType>(queueId);
        auto address = std::get<Mac48Address>(queueId);

        if (queueType == WIFI_MGT_QUEUE ||
            (queueType == WIFI_CTL_QUEUE && GetMac() && address != GetMac()->GetAddress()) ||
            queueType == WIFI_QOSDATA_BROADCAST_QUEUE)
        {
            // these queue types are associated with just one link
            NS_ASSERT(GetMac());
            auto linkId = GetMac()->GetLinkIdByAddress(address);
            NS_ASSERT(linkId.has_value());
            queueInfoIt->second.linkIds = {*linkId};
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

    // insert queueId in the queue info map if not present yet
    auto queueInfoIt = InitQueueInfo(ac, queueId);
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
WifiMacQueueSchedulerImpl<Priority, Compare>::GetLinkIds(AcIndex ac,
                                                         const WifiContainerQueueId& queueId)
{
    auto queueInfoIt = InitQueueInfo(ac, queueId);

    if (queueInfoIt->second.linkIds.empty())
    {
        // return the IDs of all available links
        NS_ASSERT(GetMac() != nullptr);
        std::list<uint8_t> linkIds(GetMac()->GetNLinks());
        std::iota(linkIds.begin(), linkIds.end(), 0);
        return linkIds;
    }
    return queueInfoIt->second.linkIds;
}

template <class Priority, class Compare>
void
WifiMacQueueSchedulerImpl<Priority, Compare>::SetLinkIds(AcIndex ac,
                                                         const WifiContainerQueueId& queueId,
                                                         const std::list<uint8_t>& linkIds)
{
    NS_LOG_FUNCTION(this << +ac);
    auto [queueInfoIt, ret] = m_perAcInfo[ac].queueInfoMap.insert({queueId, QueueInfo()});
    queueInfoIt->second.linkIds = linkIds;
}

template <class Priority, class Compare>
std::optional<WifiContainerQueueId>
WifiMacQueueSchedulerImpl<Priority, Compare>::GetNext(AcIndex ac, uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +ac << +linkId);
    return DoGetNext(ac, linkId, m_perAcInfo[ac].sortedQueues.begin());
}

template <class Priority, class Compare>
std::optional<WifiContainerQueueId>
WifiMacQueueSchedulerImpl<Priority, Compare>::GetNext(AcIndex ac,
                                                      uint8_t linkId,
                                                      const WifiContainerQueueId& prevQueueId)
{
    NS_LOG_FUNCTION(this << +ac << +linkId);

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
    uint8_t linkId,
    typename SortedQueues::iterator sortedQueuesIt)
{
    NS_LOG_FUNCTION(this << +ac << +linkId);
    NS_ASSERT(static_cast<uint8_t>(ac) < AC_UNDEF);

    while (sortedQueuesIt != m_perAcInfo[ac].sortedQueues.end())
    {
        const auto& queueInfoPair = sortedQueuesIt->second.get();
        const auto& linkIds = queueInfoPair.second.linkIds;

        if (linkIds.empty() || std::find(linkIds.begin(), linkIds.end(), linkId) != linkIds.end())
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

    DoNotifyEnqueue(ac, mpdu);

    if (auto queueInfoIt =
            m_perAcInfo[ac].queueInfoMap.find(WifiMacQueueContainer::GetQueueId(mpdu));
        queueInfoIt == m_perAcInfo[ac].queueInfoMap.end() ||
        !queueInfoIt->second.priorityIt.has_value())
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
