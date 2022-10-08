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

#include <optional>

namespace ns3
{

class WifiMpdu;
class WifiMac;

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
     * over the given link are ignored.
     *
     * \param ac the Access Category that we want to serve
     * \param linkId the ID of the link on which we got channel access
     * \return the ID of the selected container queue (if any)
     */
    virtual std::optional<WifiContainerQueueId> GetNext(AcIndex ac, uint8_t linkId) = 0;
    /**
     * Get the next queue to serve after the given one. The returned queue is
     * guaranteed to contain at least an MPDU whose lifetime has not expired.
     * Queues containing MPDUs that cannot be sent over the given link are ignored.
     *
     * \param ac the Access Category that we want to serve
     * \param linkId the ID of the link on which we got channel access
     * \param prevQueueId the ID of the container queue served previously
     * \return the ID of the selected container queue (if any)
     */
    virtual std::optional<WifiContainerQueueId> GetNext(
        AcIndex ac,
        uint8_t linkId,
        const WifiContainerQueueId& prevQueueId) = 0;

    /**
     * Get the list of the IDs of the links the given container queue (belonging to
     * the given Access Category) is associated with.
     *
     * \param ac the given Access Category
     * \param queueId the given container queue
     * \return the list of the IDs of the links the given container queue is associated with
     */
    virtual std::list<uint8_t> GetLinkIds(AcIndex ac, const WifiContainerQueueId& queueId) = 0;
    /**
     * Set the list of the IDs of the links the given container queue (belonging to
     * the given Access Category) is associated with.
     *
     * \param ac the given Access Category
     * \param queueId the given container queue
     * \param linkIds the list of the IDs of the links the given container queue is associated with
     */
    virtual void SetLinkIds(AcIndex ac,
                            const WifiContainerQueueId& queueId,
                            const std::list<uint8_t>& linkIds) = 0;

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
