/*
 * Copyright (c) 2026 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef RR_WIFI_QUEUE_SCHEDULER_H
#define RR_WIFI_QUEUE_SCHEDULER_H

#include "wifi-mac-queue-scheduler-impl.h"
#include "wifi-psdu.h"

#include "ns3/nstime.h"

#include <unordered_map>
#include <vector>

namespace ns3
{

class WifiMpdu;

/**
 * @ingroup wifi
 *
 * RrWifiQueueScheduler is a wifi queue scheduler that serves container queues in a round robin
 * fashion. The priority of a container queue is defined as the last time a frame contained in that
 * container queue was transmitted. Specifically, every time a frame is transmitted (regardless of
 * the link used) within a TXOP of which this device is the holder, the priority of the container
 * queue containing the frame is set to the current time. Container queues of a same AC are
 * therefore sorted in increasing order of last serving time and, every time an AC of this device
 * can transmit, the container queue at the front of the list for that AC is served.
 */
class RrWifiQueueScheduler : public WifiMacQueueSchedulerImpl<WifiSchedPrecedence<Time>>
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    RrWifiQueueScheduler();

    void SetWifiMac(Ptr<WifiMac> mac) override;

  private:
    void DoNotifyEnqueue(AcIndex ac, Ptr<WifiMpdu> mpdu) override;
    void DoNotifyDequeue(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) override;
    void DoNotifyRemove(AcIndex ac, const std::list<Ptr<WifiMpdu>>& mpdus) override;

    /**
     * Callback connected to the PHY trace that is fired every time a PSDU (map) is transmitted.
     * If the PSDU is transmitted in a TXOP of which this device is the holder, the priority of the
     * queue(s) containing the transmitted MPDUs is updated.
     *
     * @param phyId the ID of the PHY transmitting the PSDUs
     * @param psduMap the PSDU map
     * @param txVector the TX vector
     * @param txPower the tx power
     */
    void PsduTransmitted(uint8_t phyId,
                         WifiConstPsduMap psduMap,
                         WifiTxVector txVector,
                         Watt_u txPower);

    /// Maps the queue Id of containers to the last time a frame from each of them was transmitted
    using QueueIdLastTimeMap = std::unordered_map<WifiContainerQueueId, Time>;

    /// per-AC map of last time a frame in each container queue was transmitted
    std::vector<QueueIdLastTimeMap> m_perAcQueueIdLastTxTime{AC_UNDEF};

    NS_LOG_TEMPLATE_DECLARE; //!< redefinition of the log component
};

} // namespace ns3

#endif /* RR_WIFI_QUEUE_SCHEDULER_H */
