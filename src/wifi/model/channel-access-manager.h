/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef CHANNEL_ACCESS_MANAGER_H
#define CHANNEL_ACCESS_MANAGER_H

#include "wifi-phy-common.h"

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/object.h"

#include <algorithm>
#include <map>
#include <vector>

namespace ns3
{

class WifiPhy;
class PhyListener;
class Txop;
class FrameExchangeManager;

/**
 * \brief Manage a set of ns3::Txop
 * \ingroup wifi
 *
 * Handle a set of independent ns3::Txop, each of which represents
 * a single DCF within a MAC stack. Each ns3::Txop has a priority
 * implicitly associated with it (the priority is determined when the
 * ns3::Txop is added to the ChannelAccessManager: the first Txop to be
 * added gets the highest priority, the second, the second highest
 * priority, and so on.) which is used to handle "internal" collisions.
 * i.e., when two local Txop are expected to get access to the
 * medium at the same time, the highest priority local Txop wins
 * access to the medium and the other Txop suffers a "internal"
 * collision.
 */
class ChannelAccessManager : public Object
{
  public:
    ChannelAccessManager();
    ~ChannelAccessManager() override;

    /**
     * Set up listener for PHY events.
     *
     * \param phy the WifiPhy to listen to
     */
    void SetupPhyListener(Ptr<WifiPhy> phy);
    /**
     * Remove current registered listener for PHY events.
     *
     * \param phy the WifiPhy to listen to
     */
    void RemovePhyListener(Ptr<WifiPhy> phy);
    /**
     * Set the ID of the link this Channel Access Manager is associated with.
     *
     * \param linkId the ID of the link this Channel Access Manager is associated with
     */
    void SetLinkId(uint8_t linkId);
    /**
     * Set up the Frame Exchange Manager.
     *
     * \param feManager the Frame Exchange Manager
     */
    void SetupFrameExchangeManager(Ptr<FrameExchangeManager> feManager);

    /**
     * \param txop a new Txop.
     *
     * The ChannelAccessManager does not take ownership of this pointer so, the callee
     * must make sure that the Txop pointer will stay valid as long
     * as the ChannelAccessManager is valid. Note that the order in which Txop
     * objects are added to a ChannelAccessManager matters: the first Txop added
     * has the highest priority, the second Txop added, has the second
     * highest priority, etc.
     */
    void Add(Ptr<Txop> txop);

    /**
     * Determine if a new backoff needs to be generated when a packet is queued
     * for transmission.
     *
     * \param txop the Txop requesting to generate a backoff
     * \return true if backoff needs to be generated, false otherwise
     */
    bool NeedBackoffUponAccess(Ptr<Txop> txop);

    /**
     * \param txop a Txop
     *
     * Notify the ChannelAccessManager that a specific Txop needs access to the
     * medium. The ChannelAccessManager is then responsible for starting an access
     * timer and, invoking FrameExchangeManager::StartTransmission when the access
     * is granted if it ever gets granted.
     */
    void RequestAccess(Ptr<Txop> txop);

    /**
     * Access will never be granted to the medium _before_
     * the time returned by this method.
     *
     * \param ignoreNav flag whether NAV should be ignored
     *
     * \returns the absolute time at which access could start to be granted
     */
    Time GetAccessGrantStart(bool ignoreNav = false) const;

    /**
     * \param qosTxop a QosTxop that needs to be disabled
     * \param duration the amount of time during which the QosTxop is disabled
     *
     * Disable the given EDCA for the given amount of time. This EDCA will not be
     * granted channel access during this period and the backoff timer will be frozen.
     * After this period, the EDCA will start normal operations again by resuming
     * the backoff timer.
     */
    void DisableEdcaFor(Ptr<Txop> qosTxop, Time duration);

    /**
     * Return the width of the largest primary channel that has been idle for the
     * given time interval before the given time, if any primary channel has been
     * idle, or zero, otherwise.
     *
     * \param interval the given time interval
     * \param end the given end time
     * \return the width of the largest primary channel that has been idle for the
     *         given time interval before the given time, if any primary channel has
     *         been idle, or zero, otherwise
     */
    uint16_t GetLargestIdlePrimaryChannel(Time interval, Time end);

    /**
     * \param indices a set of indices (starting at 0) specifying the 20 MHz channels to test
     * \return true if per-20 MHz CCA indicates busy for at least one of the
     *         specified 20 MHz channels, false otherwise
     */
    bool GetPer20MHzBusy(const std::set<uint8_t>& indices) const;

    /**
     * \param duration expected duration of reception
     *
     * Notify the Txop that a packet reception started
     * for the expected duration.
     */
    void NotifyRxStartNow(Time duration);
    /**
     * Notify the Txop that a packet reception was just
     * completed successfully.
     */
    void NotifyRxEndOkNow();
    /**
     * Notify the Txop that a packet reception was just
     * completed unsuccessfuly.
     */
    void NotifyRxEndErrorNow();
    /**
     * \param duration expected duration of transmission
     *
     * Notify the Txop that a packet transmission was
     * just started and is expected to last for the specified
     * duration.
     */
    void NotifyTxStartNow(Time duration);
    /**
     * \param duration expected duration of CCA busy period
     * \param channelType the channel type for which the CCA busy state is reported.
     * \param per20MhzDurations vector that indicates for how long each 20 MHz subchannel
     *        (corresponding to the index of the element in the vector) is busy and where a zero
     * duration indicates that the subchannel is idle. The vector is non-empty if  the PHY supports
     * 802.11ax or later and if the operational channel width is larger than 20 MHz.
     *
     * Notify the Txop that a CCA busy period has just started.
     */
    void NotifyCcaBusyStartNow(Time duration,
                               WifiChannelListType channelType,
                               const std::vector<Time>& per20MhzDurations);
    /**
     * \param duration expected duration of channel switching period
     *
     * Notify the Txop that a channel switching period has just started.
     * During switching state, new packets can be enqueued in Txop/QosTxop
     * but they won't access to the medium until the end of the channel switching.
     */
    void NotifySwitchingStartNow(Time duration);
    /**
     * Notify the Txop that the device has been put in sleep mode.
     */
    void NotifySleepNow();
    /**
     * Notify the Txop that the device has been put in off mode.
     */
    void NotifyOffNow();
    /**
     * Notify the Txop that the device has been resumed from sleep mode.
     */
    void NotifyWakeupNow();
    /**
     * Notify the Txop that the device has been resumed from off mode.
     */
    void NotifyOnNow();
    /**
     * \param duration the value of the received NAV.
     *
     * Called at end of RX
     */
    void NotifyNavResetNow(Time duration);
    /**
     * \param duration the value of the received NAV.
     *
     * Called at end of RX
     */
    void NotifyNavStartNow(Time duration);
    /**
     * Notify that ack timer has started for the given duration.
     *
     * \param duration the duration of the timer
     */
    void NotifyAckTimeoutStartNow(Time duration);
    /**
     * Notify that ack timer has reset.
     */
    void NotifyAckTimeoutResetNow();
    /**
     * Notify that CTS timer has started for the given duration.
     *
     * \param duration the duration of the timer
     */
    void NotifyCtsTimeoutStartNow(Time duration);
    /**
     * Notify that CTS timer has reset.
     */
    void NotifyCtsTimeoutResetNow();

    /**
     * Check if the device is busy sending or receiving,
     * or NAV or CCA busy.
     *
     * \return true if the device is busy,
     *         false otherwise
     */
    bool IsBusy() const;

  protected:
    void DoDispose() override;

  private:
    /**
     * Initialize the structures holding busy end times per channel type (primary,
     * secondary, etc.) and per 20 MHz channel.
     */
    void InitLastBusyStructs();
    /**
     * Update backoff slots for all Txops.
     */
    void UpdateBackoff();
    /**
     * Return the time when the backoff procedure
     * started for the given Txop.
     *
     * \param txop the Txop
     *
     * \return the time when the backoff procedure started
     */
    Time GetBackoffStartFor(Ptr<Txop> txop);
    /**
     * Return the time when the backoff procedure
     * ended (or will ended) for the given Txop.
     *
     * \param txop the Txop
     *
     * \return the time when the backoff procedure ended (or will ended)
     */
    Time GetBackoffEndFor(Ptr<Txop> txop);
    /**
     * This method determines whether the medium has been idle during a period (of
     * non-null duration) immediately preceding the time this method is called. If
     * so, the last idle start time and end time for each channel type are updated.
     * Otherwise, no change is made by this method.
     * This method is normally called when we are notified of the start of a
     * transmission, reception, CCA Busy or switching to correctly maintain the
     * information about the last idle period.
     */
    void UpdateLastIdlePeriod();

    void DoRestartAccessTimeoutIfNeeded();

    /**
     * Called when access timeout should occur
     * (e.g. backoff procedure expired).
     */
    void AccessTimeout();
    /**
     * Grant access to Txop using DCF/EDCF contention rules
     */
    void DoGrantDcfAccess();

    /**
     * Return the Short Interframe Space (SIFS) for this PHY.
     *
     * \return the SIFS duration
     */
    virtual Time GetSifs() const;
    /**
     * Return the slot duration for this PHY.
     *
     * \return the slot duration
     */
    virtual Time GetSlot() const;
    /**
     * Return the EIFS duration minus a DIFS.
     *
     * \return the EIFS duration minus a DIFS
     */
    virtual Time GetEifsNoDifs() const;

    /**
     * Structure defining start time and end time for a given state.
     */
    struct Timespan
    {
        Time start{0}; //!< start time
        Time end{0};   //!< end time
    };

    /**
     * typedef for a vector of Txops
     */
    typedef std::vector<Ptr<Txop>> Txops;

    Txops m_txops;            //!< the vector of managed Txops
    Time m_lastAckTimeoutEnd; //!< the last Ack timeout end time
    Time m_lastCtsTimeoutEnd; //!< the last CTS timeout end time
    Time m_lastNavEnd;        //!< the last NAV end time
    Timespan m_lastRx;        //!< the last receive start and end time
    bool m_lastRxReceivedOk;  //!< the last receive OK
    Time m_lastTxEnd;         //!< the last transmit end time
    std::map<WifiChannelListType, Time>
        m_lastBusyEnd;                       //!< the last busy end time for each channel type
    std::vector<Time> m_lastPer20MHzBusyEnd; /**< the last busy end time per 20 MHz channel
                                                  (HE stations and channel width > 20 MHz only) */
    std::map<WifiChannelListType, Timespan>
        m_lastIdle;             //!< the last idle start and end time for each channel type
    Time m_lastSwitchingEnd;    //!< the last switching end time
    bool m_sleeping;            //!< flag whether it is in sleeping state
    bool m_off;                 //!< flag whether it is in off state
    Time m_eifsNoDifs;          //!< EIFS no DIFS time
    EventId m_accessTimeout;    //!< the access timeout ID
    PhyListener* m_phyListener; //!< the PHY listener
    Ptr<WifiPhy> m_phy;         //!< pointer to the PHY
    Ptr<FrameExchangeManager> m_feManager; //!< pointer to the Frame Exchange Manager
    uint8_t m_linkId;                      //!< the ID of the link this object is associated with
};

} // namespace ns3

#endif /* CHANNEL_ACCESS_MANAGER_H */
