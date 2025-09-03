/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef BACKOFF_MONITOR_H
#define BACKOFF_MONITOR_H

#include "qos-utils.h"
#include "wifi-utils.h"

#include "ns3/callback.h"
#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include <functional>
#include <list>
#include <memory>
#include <optional>

namespace ns3
{

class BackoffMonPhyListener;
class ChannelAccessManager;
class WifiMac;
class Txop;

/**
 * Enumeration for backoff status
 */
enum class BackoffStatus : uint8_t
{
    ONGOING = 0, // backoff counter is counting down
    PAUSED,      // backoff counter is paused due to medium busy
    ZERO,        // backoff counter reached zero
    RESET,       // backoff counter is reset
    UNKNOWN      // waiting for the first backoff value to be generated
};

/**
 * Object to monitor changes in the status of a backoff counter (@see BackoffStatus). It is
 * enabled by setting the WifiMac attribute EnableBackoffMon to true. Updates in the backoff status
 * can be traced via the BackoffStatus trace source of class Txop.
 * Once the BackoffMonitor is enabled, we start tracking the status of the backoff counter the first
 * time a new backoff value is generated.
 */
class BackoffMonitor
{
  public:
    ~BackoffMonitor();

    /// Information fed to the Txop::BackoffStatus trace
    struct StatusTrace
    {
        using enum BackoffStatus;

        linkId_t linkId{};                 ///< link ID
        BackoffStatus prevStatus{UNKNOWN}; ///< previous backoff status
        BackoffStatus currStatus{UNKNOWN}; ///< current backoff status
        uint32_t counter{};                ///< backoff counter value (remaining slots count)
    };

    /**
     * Status change callback typedef. Parameters are the link ID, the previous status, the new
     * status and the current backoff counter value.
     */
    using StatusChangeCb = Callback<void, const StatusTrace&>;

    /**
     * Enable backoff monitoring by setting up PHY listeners for all PHY objects connected to the
     * given MAC and setting the given status change callback.
     *
     * @param mac the given MAC
     * @param txop the Txop monitored by this object
     * @param cb the status change callback
     */
    void Enable(Ptr<WifiMac> mac, Ptr<Txop> txop, StatusChangeCb cb);

    /**
     * Disable backoff monitoring by removing all PHY listeners and nullifying the status change
     * callback.
     */
    void Disable();

    /**
     * Swap the links based on the information included in the given map. This method is normally
     * called on a non-AP MLD upon completing ML setup to have its link IDs match AP MLD's link IDs.
     *
     * @param links a set of pairs (from, to) each mapping a current link ID to the
     *              link ID it has to become (i.e., link 'from' becomes link 'to')
     */
    void SwapLinks(const std::map<linkId_t, linkId_t>& links);

    /**
     * Get the backoff status on the given link.
     *
     * @param linkId the ID of the given link
     * @return the backoff status on the given link
     */
    BackoffStatus GetBackoffStatus(linkId_t linkId) const;

    /**
     * Notify that a new backoff value has been generated for the given link.
     *
     * @param linkId the ID of the given link
     */
    void NotifyBackoffGenerated(linkId_t linkId);

    /**
     * Notify that channel access has been granted on the given link.
     *
     * @param linkId the ID of the given link
     */
    void NotifyChannelAccessed(linkId_t linkId);

    /**
     * Notify that the backoff has been reset for the given link.
     *
     * @param linkId the ID of the given link
     */
    void NotifyBackoffReset(linkId_t linkId);

    /**
     * Update backoff slots for the given link.
     *
     * @param linkId the ID of the given link
     * @param nSlots the updated number of remaining backoff slots
     */
    void NotifyBackoffUpdated(linkId_t linkId, uint32_t nSlots);

    /**
     * Used by the PHY listener to notify that medium is busy for the given period.
     *
     * @param phyId the ID of the PHY notifying the PHY state change
     * @param duration the duration of the medium busy period
     */
    void NotifyMediumBusy(uint8_t phyId, Time duration);

    /**
     * Callback to be connected to the NavEnd trace source of the ChannelAccessManager.
     *
     * @param cam pointer to the ChannelAccessManager object
     * @param oldNav the previous NAV end
     * @param newNav the updated NAV end
     */
    void NotifyNavUpdated(Ptr<ChannelAccessManager> cam, Time oldNav, Time newNav);

  private:
    /**
     * Get the time during which the medium has to be idle since the last busy event to consider the
     * ongoing TXOP as ended.
     *
     * @param linkId the ID of the link on which the TXOP is ongoing
     * @return the TXOP end timeout
     */
    Time GetTxopEndTimeout(linkId_t linkId) const;

    /**
     * Handle notifications of medium busy due to physical or virtual carries sense by updating the
     * backoff status accordingly.
     *
     * @param linkId the ID of the link involved in the notification
     * @param physicalCsEnd the new medium busy end due to physical carrier sense
     * @param virtualCsEnd the new medium busy end due to virtual carrier sense
     */
    void HandleMediumBusyUpdates(linkId_t linkId,
                                 std::optional<Time> physicalCsEnd,
                                 std::optional<Time> virtualCsEnd);

    /// Backoff counter state information
    struct State
    {
        linkId_t linkId; ///< ID of the link this information refers to
        BackoffStatus backoffStatus{BackoffStatus::UNKNOWN}; //!< backoff status
        Time physicalCsEnd; ///< last time medium is busy based on physical carrier sense
        Time virtualCsEnd;  ///< last time medium is busy based on virtual carrier sense (NAV)
        StatusChangeCb statusChangeCb;                  ///< status change callback
        EventId statusChangeEvent;                      ///< scheduled status change event
        Callback<uint32_t, linkId_t> getBackoffSlotsCb; ///< callback to get remaining backoff slots

        ~State()
        {
            statusChangeCb.Nullify();
            statusChangeEvent.Cancel();
            getBackoffSlotsCb.Nullify();
        }

        /**
         * Set the given backoff status.
         *
         * @param status the given backoff status
         */
        void SetStatus(BackoffStatus status);
    };

    /**
     * Get the backoff counter state information for the given link.
     *
     * @param linkId the ID of the given link
     * @return the backoff counter state information
     */
    State& GetState(linkId_t linkId);

    AcIndex m_aci{AC_UNDEF}; ///< AC monitored by this object
    bool m_enabled{false};   ///< whether this backoff monitor is enabled
    Ptr<WifiMac> m_mac;      ///< the MAC
    std::map<uint8_t, std::shared_ptr<BackoffMonPhyListener>>
        m_listeners;           ///< Per-PHY ID PHY listeners map
    std::list<State> m_states; ///< backoff state information for every link
};

/**
 * Stream insertion operator.
 *
 * @param os the stream
 * @param status the backoff status
 * @returns a reference to the stream
 */
std::ostream& operator<<(std::ostream& os, BackoffStatus status);

} // namespace ns3

#endif /* BACKOFF_MONITOR_H */
