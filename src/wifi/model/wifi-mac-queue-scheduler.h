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

#ifndef WIFI_MAC_QUEUE_SCHEDULER_H
#define WIFI_MAC_QUEUE_SCHEDULER_H

#include "qos-utils.h"
#include "wifi-mac-queue-container.h"

#include "ns3/object.h"

#include <bitset>
#include <optional>

namespace ns3
{

class WifiMpdu;
class WifiMac;

/**
 * \ingroup wifi
 *
 * Enumeration of the reasons to block container queues.
 */
enum class WifiQueueBlockedReason : uint8_t
{
    WAITING_ADDBA_RESP = 0,
    POWER_SAVE_MODE,
    USING_OTHER_EMLSR_LINK,
    WAITING_EMLSR_TRANSITION_DELAY,
    TID_NOT_MAPPED,
    REASONS_COUNT
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the stream
 * \param reason the reason to block container queues
 * \returns a reference to the stream
 */
inline std::ostream&
operator<<(std::ostream& os, WifiQueueBlockedReason reason)
{
    switch (reason)
    {
    case WifiQueueBlockedReason::WAITING_ADDBA_RESP:
        return (os << "WAITING_ADDBA_RESP");
    case WifiQueueBlockedReason::POWER_SAVE_MODE:
        return (os << "POWER_SAVE_MODE");
    case WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK:
        return (os << "USING_OTHER_EMLSR_LINK");
    case WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY:
        return (os << "WAITING_EMLSR_TRANSITION_DELAY");
    case WifiQueueBlockedReason::TID_NOT_MAPPED:
        return (os << "TID_NOT_MAPPED");
    case WifiQueueBlockedReason::REASONS_COUNT:
        return (os << "REASONS_COUNT");
    default:
        NS_ABORT_MSG("Unknown queue blocked reason");
        return (os << "unknown");
    }
}

/**
 * \ingroup wifi
 *
 * WifiMacQueueScheduler is an abstract base class defining the public interface
 * for a wifi MAC queue scheduler.
 */
class WifiMacQueueScheduler : public Object
{
  public:
    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * Set the wifi MAC.
     *
     * \param mac the wifi MAC
     */
    virtual void SetWifiMac(Ptr<WifiMac> mac);

    /**
     * Get the next queue to serve, which is guaranteed to contain at least an MPDU
     * whose lifetime has not expired. Queues containing MPDUs that cannot be sent
     * over the given link (if any) are ignored.
     *
     * \param ac the Access Category that we want to serve
     * \param linkId the ID of the link on which MPDUs contained in the returned queue must be
     *               allowed to be sent
     * \return the ID of the selected container queue (if any)
     */
    virtual std::optional<WifiContainerQueueId> GetNext(AcIndex ac,
                                                        std::optional<uint8_t> linkId) = 0;
    /**
     * Get the next queue to serve after the given one. The returned queue is
     * guaranteed to contain at least an MPDU whose lifetime has not expired.
     * Queues containing MPDUs that cannot be sent over the given link (if any) are ignored.
     *
     * \param ac the Access Category that we want to serve
     * \param linkId the ID of the link on which MPDUs contained in the returned queue must be
     *               allowed to be sent
     * \param prevQueueId the ID of the container queue served previously
     * \return the ID of the selected container queue (if any)
     */
    virtual std::optional<WifiContainerQueueId> GetNext(
        AcIndex ac,
        std::optional<uint8_t> linkId,
        const WifiContainerQueueId& prevQueueId) = 0;

    /**
     * Get the list of the IDs of the links the given MPDU (belonging to the given
     * Access Category) can be sent over.
     *
     * \param ac the given Access Category
     * \param mpdu the given MPDU
     * \param ignoredReasons list of reasons for blocking a link that are ignored
     * \return the list of the IDs of the links the given MPDU can be sent over
     */
    virtual std::list<uint8_t> GetLinkIds(
        AcIndex ac,
        Ptr<const WifiMpdu> mpdu,
        const std::list<WifiQueueBlockedReason>& ignoredReasons = {}) = 0;

    /**
     * Block the given set of links for the container queues of the given types and
     * Access Category that hold frames having the given Receiver Address (RA),
     * Transmitter Address (TA) and TID (if needed) for the given reason, such that
     * frames in these queues are not transmitted on the given set of links.
     *
     * \param reason the reason for blocking the queues
     * \param ac the given Access Category
     * \param types the types of the queues to block
     * \param rxAddress the Receiver Address (RA) of the frames
     * \param txAddress the Transmitter Address (TA) of the frames
     * \param tids the TIDs optionally identifying the queues to block
     * \param linkIds set of links to block (empty to block all setup links)
     */
    virtual void BlockQueues(WifiQueueBlockedReason reason,
                             AcIndex ac,
                             const std::list<WifiContainerQueueType>& types,
                             const Mac48Address& rxAddress,
                             const Mac48Address& txAddress,
                             const std::set<uint8_t>& tids = {},
                             const std::set<uint8_t>& linkIds = {}) = 0;
    /**
     * Unblock the given set of links for the container queues of the given types and
     * Access Category that hold frames having the given Receiver Address (RA),
     * Transmitter Address (TA) and TID (if needed) for the given reason, such that
     * frames in these queues can be transmitted on the given set of links.
     *
     * \param reason the reason for unblocking the queues
     * \param ac the given Access Category
     * \param types the types of the queues to unblock
     * \param rxAddress the Receiver Address (RA) of the frames
     * \param txAddress the Transmitter Address (TA) of the frames
     * \param tids the TIDs optionally identifying the queues to unblock
     * \param linkIds set of links to unblock (empty to unblock all setup links)
     */
    virtual void UnblockQueues(WifiQueueBlockedReason reason,
                               AcIndex ac,
                               const std::list<WifiContainerQueueType>& types,
                               const Mac48Address& rxAddress,
                               const Mac48Address& txAddress,
                               const std::set<uint8_t>& tids = {},
                               const std::set<uint8_t>& linkIds = {}) = 0;

    /// Bitset identifying the reasons to block individual links for a container queue
    using Mask = std::bitset<static_cast<std::size_t>(WifiQueueBlockedReason::REASONS_COUNT)>;

    /**
     * Get the mask associated with the given container queue indicating whether the given link
     * is blocked and for which reason, provided that the given container queue exists and has
     * a mask for the given link.
     *
     * \param ac the given Access Category
     * \param queueId the ID of the given container queue
     * \param linkId the ID of the given link
     * \return the mask associated with the given container queue for the given link
     */
    virtual std::optional<Mask> GetQueueLinkMask(AcIndex ac,
                                                 const WifiContainerQueueId& queueId,
                                                 uint8_t linkId) = 0;

    /**
     * Check whether an MPDU has to be dropped before enqueuing the given MPDU.
     *
     * \param ac the Access Category of the MPDU being enqueued
     * \param mpdu the MPDU to enqueue
     * \return a pointer to the MPDU to drop, if any, or a null pointer, otherwise
     */
    virtual Ptr<WifiMpdu> HasToDropBeforeEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu) = 0;
    /**
     * Notify the scheduler that the given MPDU has been enqueued by the given Access
     * Category. The container queue in which the MPDU has been enqueued must be
     * assigned a priority value.
     *
     * \param ac the Access Category of the enqueued MPDU
     * \param mpdu the enqueued MPDU
     */
    virtual void NotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu) = 0;
    /**
     * Notify the scheduler that the given list of MPDUs have been dequeued by the
     * given Access Category. The container queues which became empty after dequeuing
     * the MPDUs are removed from the sorted list of queues.
     *
     * \param ac the Access Category of the dequeued MPDUs
     * \param mpdus the list of dequeued MPDUs
     */
    virtual void NotifyDequeue(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) = 0;
    /**
     * Notify the scheduler that the given list of MPDUs have been removed by the
     * given Access Category. The container queues which became empty after removing
     * the MPDUs are removed from the sorted list of queues.
     *
     * \param ac the Access Category of the removed MPDUs
     * \param mpdus the list of removed MPDUs
     */
    virtual void NotifyRemove(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) = 0;

  protected:
    void DoDispose() override;

    /**
     * Get the wifi MAC.
     *
     * \return the wifi MAC
     */
    Ptr<WifiMac> GetMac() const;

  private:
    Ptr<WifiMac> m_mac; //!< MAC layer
};

} // namespace ns3

#endif /* WIFI_MAC_QUEUE_SCHEDULER_H */
