/*
 * Copyright (c) 2005,2006 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "channel-access-manager.h"

#include "txop.h"
#include "wifi-mac-queue.h"
#include "wifi-phy-listener.h"
#include "wifi-phy.h"

#include "ns3/eht-frame-exchange-manager.h"
#include "ns3/log.h"
#include "ns3/simulator.h"

#include <cmath>
#include <sstream>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "] "

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ChannelAccessManager");

NS_OBJECT_ENSURE_REGISTERED(ChannelAccessManager);

const Time ChannelAccessManager::DEFAULT_N_SLOTS_LEFT_MIN_DELAY = MicroSeconds(25);

/**
 * Listener for PHY events. Forwards to ChannelAccessManager.
 * The ChannelAccessManager may handle multiple PHY listeners connected to distinct PHYs,
 * but only one listener at a time can be active. Notifications from inactive listeners are
 * ignored by the ChannelAccessManager, except for the channel switch notification.
 * Inactive PHY listeners are typically configured by 11be EMLSR clients.
 */
class PhyListener : public ns3::WifiPhyListener
{
  public:
    /**
     * Create a PhyListener for the given ChannelAccessManager.
     *
     * @param cam the ChannelAccessManager
     */
    PhyListener(ns3::ChannelAccessManager* cam)
        : m_cam(cam),
          m_active(true)
    {
    }

    ~PhyListener() override
    {
    }

    /**
     * Set this listener to be active or not.
     *
     * @param active whether this listener is active or not
     */
    void SetActive(bool active)
    {
        m_active = active;
    }

    /**
     * @return whether this listener is active or not
     */
    bool IsActive() const
    {
        return m_active;
    }

    void NotifyRxStart(Time duration) override
    {
        if (m_active)
        {
            m_cam->NotifyRxStartNow(duration);
        }
    }

    void NotifyRxEndOk() override
    {
        if (m_active)
        {
            m_cam->NotifyRxEndOkNow();
        }
    }

    void NotifyRxEndError(const WifiTxVector& txVector) override
    {
        if (m_active)
        {
            m_cam->NotifyRxEndErrorNow(txVector);
        }
    }

    void NotifyTxStart(Time duration, dBm_u txPower) override
    {
        if (m_active)
        {
            m_cam->NotifyTxStartNow(duration);
        }
    }

    void NotifyCcaBusyStart(Time duration,
                            WifiChannelListType channelType,
                            const std::vector<Time>& per20MhzDurations) override
    {
        if (m_active)
        {
            m_cam->NotifyCcaBusyStartNow(duration, channelType, per20MhzDurations);
        }
    }

    void NotifySwitchingStart(Time duration) override
    {
        m_cam->NotifySwitchingStartNow(this, duration);
    }

    void NotifySleep() override
    {
        if (m_active)
        {
            m_cam->NotifySleepNow();
        }
    }

    void NotifyOff() override
    {
        if (m_active)
        {
            m_cam->NotifyOffNow();
        }
    }

    void NotifyWakeup() override
    {
        if (m_active)
        {
            m_cam->NotifyWakeupNow();
        }
    }

    void NotifyOn() override
    {
        if (m_active)
        {
            m_cam->NotifyOnNow();
        }
    }

  private:
    ns3::ChannelAccessManager* m_cam; //!< ChannelAccessManager to forward events to
    bool m_active;                    //!< whether this PHY listener is active
};

/****************************************************************
 *      Implement the channel access manager of all Txop holders
 ****************************************************************/

TypeId
ChannelAccessManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::ChannelAccessManager")
            .SetParent<ns3::Object>()
            .SetGroupName("Wifi")
            .AddConstructor<ChannelAccessManager>()
            .AddAttribute("GenerateBackoffIfTxopWithoutTx",
                          "Specify whether the backoff should be invoked when the AC gains the "
                          "right to start a TXOP but it does not transmit any frame "
                          "(e.g., due to constraints associated with EMLSR operations), "
                          "provided that the queue is not actually empty.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&ChannelAccessManager::SetGenerateBackoffOnNoTx,
                                              &ChannelAccessManager::GetGenerateBackoffOnNoTx),
                          MakeBooleanChecker())
            .AddAttribute("ProactiveBackoff",
                          "Specify whether a new backoff value is generated when a CCA busy "
                          "period starts, the backoff counter is zero and the station is not a "
                          "TXOP holder. This is useful to generate a new backoff value when, "
                          "e.g., the backoff counter reaches zero, the station does not transmit "
                          "and subsequently the medium becomes busy.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&ChannelAccessManager::m_proactiveBackoff),
                          MakeBooleanChecker())
            .AddAttribute("ResetBackoffThreshold",
                          "If no PHY operates on this link, or the PHY operating on this link "
                          "stays in sleep mode or off mode, for a period greater than this "
                          "threshold, all the backoffs are reset.",
                          TimeValue(Time{0}),
                          MakeTimeAccessor(&ChannelAccessManager::m_resetBackoffThreshold),
                          MakeTimeChecker())
            .AddAttribute("NSlotsLeft",
                          "The NSlotsLeftAlert trace source is fired when the number of remaining "
                          "backoff slots for any AC is equal to or less than the value of this "
                          "attribute. Note that the trace source is fired only if the AC for which "
                          "the previous condition is met has requested channel access. Also, if "
                          "the value of this attribute is zero, the trace source is never fired.",
                          UintegerValue(0),
                          MakeUintegerAccessor(&ChannelAccessManager::m_nSlotsLeft),
                          MakeUintegerChecker<uint8_t>())
            .AddAttribute("NSlotsLeftMinDelay",
                          "The minimum gap between the end of a medium busy event and the time "
                          "the NSlotsLeftAlert trace source can be fired.",
                          TimeValue(ChannelAccessManager::DEFAULT_N_SLOTS_LEFT_MIN_DELAY),
                          MakeTimeAccessor(&ChannelAccessManager::m_nSlotsLeftMinDelay),
                          MakeTimeChecker())
            .AddTraceSource("NSlotsLeftAlert",
                            "The number of remaining backoff slots for the AC with the given index "
                            "reached the threshold set through the NSlotsLeft attribute.",
                            MakeTraceSourceAccessor(&ChannelAccessManager::m_nSlotsLeftCallback),
                            "ns3::ChannelAccessManager::NSlotsLeftCallback");
    return tid;
}

ChannelAccessManager::ChannelAccessManager()
    : m_lastAckTimeoutEnd(0),
      m_lastCtsTimeoutEnd(0),
      m_lastNavEnd(0),
      m_lastRx({MicroSeconds(0), MicroSeconds(0)}),
      m_lastRxReceivedOk(true),
      m_lastTxEnd(0),
      m_lastSwitchingEnd(0),
      m_linkId(0)
{
    NS_LOG_FUNCTION(this);
    InitLastBusyStructs();
}

ChannelAccessManager::~ChannelAccessManager()
{
    NS_LOG_FUNCTION(this);
}

void
ChannelAccessManager::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    InitLastBusyStructs();
}

void
ChannelAccessManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_txops.clear();
    m_phy = nullptr;
    m_feManager = nullptr;
    m_phyListeners.clear();
}

std::shared_ptr<PhyListener>
ChannelAccessManager::GetPhyListener(Ptr<WifiPhy> phy) const
{
    if (auto listenerIt = m_phyListeners.find(phy); listenerIt != m_phyListeners.end())
    {
        return listenerIt->second;
    }
    return nullptr;
}

void
ChannelAccessManager::SetupPhyListener(Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);

    const auto now = Simulator::Now();
    auto phyListener = GetPhyListener(phy);

    if (phyListener)
    {
        // a PHY listener for the given PHY already exists, it must be inactive
        NS_ASSERT_MSG(!phyListener->IsActive(),
                      "There is already an active listener registered for given PHY");
        NS_ASSERT_MSG(!m_phy, "Cannot reactivate a listener if another PHY is active");
        phyListener->SetActive(true);
        // if a PHY listener already exists, the PHY was disconnected and now reconnected to the
        // channel access manager; unregister the listener and register again (below) to get
        // updated CCA busy information
        phy->UnregisterListener(phyListener);
        // we expect that the PHY is reconnected immediately after the other PHY left the link:
        // reset the start of m_lastNoPhy so as to ignore this event
        NS_ASSERT(m_lastNoPhy.start == now);
        NS_ASSERT(m_lastNoPhy.end <= m_lastNoPhy.start);
        m_lastNoPhy.start = m_lastNoPhy.end;
    }
    else
    {
        phyListener = std::make_shared<PhyListener>(this);
        m_phyListeners.emplace(phy, phyListener);
        if (m_phy)
        {
            DeactivatePhyListener(m_phy);
        }
        else
        {
            // no PHY operating on this link and no previous PHY listener to reactivate
            m_lastSwitchingEnd = now;
            m_lastNoPhy.end = now;
            if (now - m_lastNoPhy.start > m_resetBackoffThreshold)
            {
                ResetAllBackoffs();
            }
        }
    }

    m_phy = phy; // this is the new active PHY
    ResizeLastBusyStructs();
    phy->RegisterListener(phyListener);
}

void
ChannelAccessManager::RemovePhyListener(Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    if (auto phyListener = GetPhyListener(phy))
    {
        phy->UnregisterListener(phyListener);
        m_phyListeners.erase(phy);
        // reset m_phy if we are removing listener registered for the active PHY
        if (m_phy == phy)
        {
            UpdateBackoff();
            UpdateLastIdlePeriod();
            m_phy = nullptr;
            m_lastNoPhy.start = Simulator::Now();
        }
    }
}

void
ChannelAccessManager::DeactivatePhyListener(Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    if (auto listener = GetPhyListener(phy))
    {
        listener->SetActive(false);
    }
}

void
ChannelAccessManager::NotifySwitchingEmlsrLink(Ptr<WifiPhy> phy,
                                               const WifiPhyOperatingChannel& channel,
                                               uint8_t linkId)
{
    NS_LOG_FUNCTION(this << phy << channel << linkId);
    NS_ASSERT_MSG(!m_switchingEmlsrLinks.contains(phy),
                  "The given PHY is already expected to switch channel");
    m_switchingEmlsrLinks.emplace(phy, EmlsrLinkSwitchInfo{channel, linkId});
}

void
ChannelAccessManager::SetLinkId(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    m_linkId = linkId;
}

void
ChannelAccessManager::SetupFrameExchangeManager(Ptr<FrameExchangeManager> feManager)
{
    NS_LOG_FUNCTION(this << feManager);
    m_feManager = feManager;
    m_feManager->SetChannelAccessManager(this);
}

Time
ChannelAccessManager::GetSlot() const
{
    if (m_phy)
    {
        m_cachedSlot = m_phy->GetSlot();
    }
    return m_cachedSlot;
}

Time
ChannelAccessManager::GetSifs() const
{
    if (m_phy)
    {
        m_cachedSifs = m_phy->GetSifs();
    }
    return m_cachedSifs;
}

Time
ChannelAccessManager::GetEifsNoDifs() const
{
    return m_eifsNoDifs;
}

void
ChannelAccessManager::Add(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);
    m_txops.push_back(txop);
}

void
ChannelAccessManager::ResizeLastBusyStructs()
{
    NS_LOG_FUNCTION(this);
    const auto now = Simulator::Now();

    m_lastBusyEnd.emplace(WIFI_CHANLIST_PRIMARY, now);
    m_lastIdle.emplace(WIFI_CHANLIST_PRIMARY, Timespan{now, now});

    const auto width = m_phy ? m_phy->GetChannelWidth() : MHz_u{0};
    std::size_t size = (width > MHz_u{20} && m_phy->GetStandard() >= WIFI_STANDARD_80211ax)
                           ? Count20MHzSubchannels(width)
                           : 0;
    m_lastPer20MHzBusyEnd.resize(size, now);

    if (!m_phy || !m_phy->GetOperatingChannel().IsOfdm())
    {
        return;
    }

    if (width >= MHz_u{40})
    {
        m_lastBusyEnd.emplace(WIFI_CHANLIST_SECONDARY, now);
        m_lastIdle.emplace(WIFI_CHANLIST_SECONDARY, Timespan{now, now});
    }
    else
    {
        m_lastBusyEnd.erase(WIFI_CHANLIST_SECONDARY);
        m_lastIdle.erase(WIFI_CHANLIST_SECONDARY);
    }

    if (width >= MHz_u{80})
    {
        m_lastBusyEnd.emplace(WIFI_CHANLIST_SECONDARY40, now);
        m_lastIdle.emplace(WIFI_CHANLIST_SECONDARY40, Timespan{now, now});
    }
    else
    {
        m_lastBusyEnd.erase(WIFI_CHANLIST_SECONDARY40);
        m_lastIdle.erase(WIFI_CHANLIST_SECONDARY40);
    }

    if (width >= MHz_u{160})
    {
        m_lastBusyEnd.emplace(WIFI_CHANLIST_SECONDARY80, now);
        m_lastIdle.emplace(WIFI_CHANLIST_SECONDARY80, Timespan{now, now});
    }
    else
    {
        m_lastBusyEnd.erase(WIFI_CHANLIST_SECONDARY80);
        m_lastIdle.erase(WIFI_CHANLIST_SECONDARY80);
    }

    if (width >= MHz_u{320})
    {
        m_lastBusyEnd.emplace(WIFI_CHANLIST_SECONDARY160, now);
        m_lastIdle.emplace(WIFI_CHANLIST_SECONDARY160, Timespan{now, now});
    }
    else
    {
        m_lastBusyEnd.erase(WIFI_CHANLIST_SECONDARY160);
        m_lastIdle.erase(WIFI_CHANLIST_SECONDARY160);
    }

    // TODO Add conditions for new channel widths as they get supported
}

void
ChannelAccessManager::InitLastBusyStructs()
{
    NS_LOG_FUNCTION(this);
    Time now = Simulator::Now();

    ResizeLastBusyStructs();

    // reset all values
    for (auto& [chType, time] : m_lastBusyEnd)
    {
        time = now;
    }

    for (auto& [chType, timeSpan] : m_lastIdle)
    {
        timeSpan = Timespan{now, now};
    }

    for (auto& time : m_lastPer20MHzBusyEnd)
    {
        time = now;
    }
}

bool
ChannelAccessManager::IsBusy() const
{
    NS_LOG_FUNCTION(this);
    Time now = Simulator::Now();
    return (m_lastRx.end > now)    // RX
           || (m_lastTxEnd > now)  // TX
           || (m_lastNavEnd > now) // NAV busy
           // an EDCA TXOP is obtained based solely on activity of the primary channel
           // (Sec. 10.23.2.5 of IEEE 802.11-2020)
           || (m_lastBusyEnd.at(WIFI_CHANLIST_PRIMARY) > now); // CCA busy
}

bool
ChannelAccessManager::NeedBackoffUponAccess(Ptr<Txop> txop,
                                            bool hadFramesToTransmit,
                                            bool checkMediumBusy)
{
    NS_LOG_FUNCTION(this << txop << hadFramesToTransmit << checkMediumBusy);

    // No backoff needed if in sleep mode or off. Checking if m_phy is nullptr is a workaround
    // needed for EMLSR and may be removed in the future
    if (!m_phy || m_phy->IsStateSleep() || m_phy->IsStateOff())
    {
        return false;
    }

    // the Txop might have a stale value of remaining backoff slots
    UpdateBackoff();

    /*
     * From section 10.3.4.2 "Basic access" of IEEE 802.11-2016:
     *
     * A STA may transmit an MPDU when it is operating under the DCF access
     * method, either in the absence of a PC, or in the CP of the PCF access
     * method, when the STA determines that the medium is idle when a frame is
     * queued for transmission, and remains idle for a period of a DIFS, or an
     * EIFS (10.3.2.3.7) from the end of the immediately preceding medium-busy
     * event, whichever is the greater, and the backoff timer is zero. Otherwise
     * the random backoff procedure described in 10.3.4.3 shall be followed.
     *
     * From section 10.22.2.2 "EDCA backoff procedure" of IEEE 802.11-2016:
     *
     * The backoff procedure shall be invoked by an EDCAF when any of the following
     * events occurs:
     * a) An MA-UNITDATA.request primitive is received that causes a frame with that AC
     *    to be queued for transmission such that one of the transmit queues associated
     *    with that AC has now become non-empty and any other transmit queues
     *    associated with that AC are empty; the medium is busy on the primary channel
     */
    if (!hadFramesToTransmit && txop->HasFramesToTransmit(m_linkId) &&
        txop->GetAccessStatus(m_linkId) != Txop::GRANTED && txop->GetBackoffSlots(m_linkId) == 0)
    {
        if (checkMediumBusy && !IsBusy())
        {
            // medium idle. If this is a DCF, use immediate access (we can transmit
            // in a DIFS if the medium remains idle). If this is an EDCAF, update
            // the backoff start time kept by the EDCAF to the current time in order
            // to correctly align the backoff start time at the next slot boundary
            // (performed by the next call to ChannelAccessManager::RequestAccess())
            Time delay =
                (txop->IsQosTxop() ? Seconds(0) : GetSifs() + txop->GetAifsn(m_linkId) * GetSlot());
            txop->UpdateBackoffSlotsNow(0, Simulator::Now() + delay, m_linkId);
        }
        else
        {
            // medium busy, backoff is needed
            return true;
        }
    }
    return false;
}

void
ChannelAccessManager::RequestAccess(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);
    if (m_phy && txop->HasFramesToTransmit(m_linkId))
    {
        m_phy->NotifyChannelAccessRequested();
    }
    // Deny access if in sleep mode or off. Checking if m_phy is nullptr is a workaround
    // needed for EMLSR and may be removed in the future
    if (!m_phy || m_phy->IsStateSleep() || m_phy->IsStateOff())
    {
        return;
    }
    /*
     * EDCAF operations shall be performed at slot boundaries (Sec. 10.22.2.4 of 802.11-2016)
     */
    Time accessGrantStart = GetAccessGrantStart() + (txop->GetAifsn(m_linkId) * GetSlot());

    if (const auto diff = txop->GetBackoffStart(m_linkId) - accessGrantStart;
        txop->IsQosTxop() && diff.IsStrictlyPositive())
    {
        // The backoff start time reported by the EDCAF is more recent than the last time the medium
        // was busy plus an AIFS, hence we need to align it to the next slot boundary.
        const auto div = diff / GetSlot();
        const uint32_t nIntSlots = div.GetHigh() + (div.GetLow() > 0 ? 1 : 0);
        txop->UpdateBackoffSlotsNow(0, accessGrantStart + (nIntSlots * GetSlot()), m_linkId);
    }

    UpdateBackoff();
    NS_ASSERT(txop->GetAccessStatus(m_linkId) != Txop::REQUESTED);
    txop->NotifyAccessRequested(m_linkId);
    DoGrantDcfAccess();
    DoRestartAccessTimeoutIfNeeded();
}

void
ChannelAccessManager::DoGrantDcfAccess()
{
    NS_LOG_FUNCTION(this);
    uint32_t k = 0;
    const auto now = Simulator::Now();
    const auto accessGrantStart = GetAccessGrantStart();
    if (accessGrantStart > now)
    {
        NS_LOG_DEBUG("access cannot be granted yet");
        return;
    }

    for (auto i = m_txops.begin(); i != m_txops.end(); k++)
    {
        Ptr<Txop> txop = *i;
        if (txop->GetAccessStatus(m_linkId) == Txop::REQUESTED &&
            (!txop->IsQosTxop() || !StaticCast<QosTxop>(txop)->EdcaDisabled(m_linkId)) &&
            GetBackoffEndFor(txop, accessGrantStart) <= now)
        {
            /**
             * This is the first Txop we find with an expired backoff and which
             * needs access to the medium. i.e., it has data to send.
             */
            NS_LOG_DEBUG("dcf " << k << " needs access. backoff expired. access granted. slots="
                                << txop->GetBackoffSlots(m_linkId));
            i++; // go to the next item in the list.
            k++;
            std::vector<Ptr<Txop>> internalCollisionTxops;
            for (auto j = i; j != m_txops.end(); j++, k++)
            {
                Ptr<Txop> otherTxop = *j;
                if (otherTxop->GetAccessStatus(m_linkId) == Txop::REQUESTED &&
                    GetBackoffEndFor(otherTxop, accessGrantStart) <= now)
                {
                    NS_LOG_DEBUG(
                        "dcf " << k << " needs access. backoff expired. internal collision. slots="
                               << otherTxop->GetBackoffSlots(m_linkId));
                    /**
                     * all other Txops with a lower priority whose backoff
                     * has expired and which needed access to the medium
                     * must be notified that we did get an internal collision.
                     */
                    internalCollisionTxops.push_back(otherTxop);
                }
            }

            /**
             * Now, we notify all of these changes in one go if the EDCAF winning
             * the contention actually transmitted a frame. It is necessary to
             * perform first the calculations of which Txops are colliding and then
             * only apply the changes because applying the changes through notification
             * could change the global state of the manager, and, thus, could change
             * the result of the calculations.
             */
            NS_ASSERT(m_feManager);
            // If we are operating on an OFDM channel wider than 20 MHz, find the largest
            // idle primary channel and pass its width to the FrameExchangeManager, so that
            // the latter can transmit PPDUs of the appropriate width (see Section 10.23.2.5
            // of IEEE 802.11-2020).
            auto interval = (m_phy->GetPhyBand() == WIFI_PHY_BAND_2_4GHZ)
                                ? GetSifs() + 2 * GetSlot()
                                : m_phy->GetPifs();
            auto width =
                (m_phy->GetOperatingChannel().IsOfdm() && m_phy->GetChannelWidth() > MHz_u{20})
                    ? GetLargestIdlePrimaryChannel(interval, now)
                    : m_phy->GetChannelWidth();
            if (m_feManager->StartTransmission(txop, width))
            {
                for (auto& collidingTxop : internalCollisionTxops)
                {
                    m_feManager->NotifyInternalCollision(collidingTxop);
                }
                break;
            }
            else
            {
                // this TXOP did not transmit anything, make sure that backoff counter starts
                // decreasing in a slot again
                txop->UpdateBackoffSlotsNow(0, now, m_linkId);
                // reset the current state to the EDCAF that won the contention
                // but did not transmit anything
                i--;
                k = std::distance(m_txops.begin(), i);
            }
        }
        i++;
    }
}

void
ChannelAccessManager::AccessTimeout()
{
    NS_LOG_FUNCTION(this);

    const auto now = Simulator::Now();
    const auto noPhyForTooLong = (!m_phy && now - m_lastNoPhy.start > m_resetBackoffThreshold);
    const auto sleepForTooLong =
        (m_phy && m_phy->IsStateSleep() && now - m_lastSleep.start > m_resetBackoffThreshold);
    const auto offForTooLong =
        (m_phy && m_phy->IsStateOff() && now - m_lastOff.start > m_resetBackoffThreshold);

    if (noPhyForTooLong || sleepForTooLong || offForTooLong)
    {
        ResetAllBackoffs();
        return;
    }

    UpdateBackoff();
    DoGrantDcfAccess();
    DoRestartAccessTimeoutIfNeeded();
}

std::multimap<Time, WifiExpectedAccessReason>
ChannelAccessManager::DoGetAccessGrantStart(bool ignoreNav) const
{
    NS_LOG_FUNCTION(this << ignoreNav);
    const auto now = Simulator::Now();

    std::multimap<Time, WifiExpectedAccessReason> ret;

    // an EDCA TXOP is obtained based solely on activity of the primary channel
    // (Sec. 10.23.2.5 of IEEE 802.11-2020)
    const auto busyAccessStart = m_lastBusyEnd.at(WIFI_CHANLIST_PRIMARY);
    ret.emplace(busyAccessStart, WifiExpectedAccessReason::BUSY_END);

    auto rxAccessStart = m_lastRx.end;
    if ((m_lastRx.end <= now) && !m_lastRxReceivedOk)
    {
        rxAccessStart += GetEifsNoDifs();
    }
    ret.emplace(rxAccessStart, WifiExpectedAccessReason::RX_END);

    ret.emplace(m_lastTxEnd, WifiExpectedAccessReason::TX_END);

    const auto navAccessStart = ignoreNav ? Time{0} : m_lastNavEnd;
    ret.emplace(navAccessStart, WifiExpectedAccessReason::NAV_END);

    ret.emplace(m_lastAckTimeoutEnd, WifiExpectedAccessReason::ACK_TIMER_END);
    ret.emplace(m_lastCtsTimeoutEnd, WifiExpectedAccessReason::CTS_TIMER_END);
    ret.emplace(m_lastSwitchingEnd, WifiExpectedAccessReason::SWITCHING_END);

    const auto noPhyStart = m_phy ? m_lastNoPhy.end : now;
    ret.emplace(noPhyStart, WifiExpectedAccessReason::NO_PHY_END);

    const auto lastSleepEnd = (m_lastSleep.start > m_lastSleep.end ? now : m_lastSleep.end);
    ret.emplace(lastSleepEnd, WifiExpectedAccessReason::SLEEP_END);

    const auto lastOffEnd = (m_lastOff.start > m_lastOff.end ? now : m_lastOff.end);
    ret.emplace(lastOffEnd, WifiExpectedAccessReason::OFF_END);

    NS_LOG_INFO("rx access start=" << rxAccessStart.As(Time::US)
                                   << ", busy access start=" << busyAccessStart.As(Time::US)
                                   << ", tx access start=" << m_lastTxEnd.As(Time::US)
                                   << ", nav access start=" << navAccessStart.As(Time::US)
                                   << ", switching access start=" << m_lastSwitchingEnd.As(Time::US)
                                   << ", no PHY start=" << noPhyStart.As(Time::US)
                                   << ", sleep access start=" << lastSleepEnd.As(Time::US)
                                   << ", off access start=" << lastOffEnd.As(Time::US));
    return ret;
}

Time
ChannelAccessManager::GetAccessGrantStart(bool ignoreNav) const
{
    NS_LOG_FUNCTION(this << ignoreNav);

    auto timeReasonMap = DoGetAccessGrantStart(ignoreNav);
    NS_ASSERT(!timeReasonMap.empty());
    const auto accessGrantedStart = timeReasonMap.crbegin()->first;
    NS_LOG_INFO("access grant start=" << accessGrantedStart.As(Time::US));

    return accessGrantedStart + GetSifs();
}

Time
ChannelAccessManager::GetBackoffStartFor(Ptr<Txop> txop) const
{
    return GetBackoffStartFor(txop, GetAccessGrantStart());
}

Time
ChannelAccessManager::GetBackoffStartFor(Ptr<Txop> txop, Time accessGrantStart) const
{
    NS_LOG_FUNCTION(this << txop << accessGrantStart.As(Time::S));
    const auto mostRecentEvent =
        std::max({txop->GetBackoffStart(m_linkId),
                  accessGrantStart + (txop->GetAifsn(m_linkId) * GetSlot())});
    NS_LOG_DEBUG("Backoff start for " << txop->GetWifiMacQueue()->GetAc() << ": "
                                      << mostRecentEvent.As(Time::US));

    return mostRecentEvent;
}

Time
ChannelAccessManager::GetBackoffEndFor(Ptr<Txop> txop) const
{
    return GetBackoffEndFor(txop, GetAccessGrantStart());
}

Time
ChannelAccessManager::GetBackoffEndFor(Ptr<Txop> txop, Time accessGrantStart) const
{
    NS_LOG_FUNCTION(this << txop);
    Time backoffEnd =
        GetBackoffStartFor(txop, accessGrantStart) + (txop->GetBackoffSlots(m_linkId) * GetSlot());
    NS_LOG_DEBUG("Backoff end for " << txop->GetWifiMacQueue()->GetAc() << ": "
                                    << backoffEnd.As(Time::US));

    return backoffEnd;
}

WifiExpectedAccessReason
ChannelAccessManager::GetExpectedAccessWithin(const Time& delay) const
{
    NS_LOG_FUNCTION(this << delay.As(Time::US));

    const auto now = Simulator::Now();
    const auto deadline = now + delay;
    const auto timeReasonMap = DoGetAccessGrantStart(false);
    NS_ASSERT(!timeReasonMap.empty());
    auto accessGrantStart = timeReasonMap.crbegin()->first;

    if (accessGrantStart >= deadline)
    {
        // return the earliest reason for which access cannot be granted in time
        for (const auto& [time, reason] : timeReasonMap)
        {
            if (time >= deadline)
            {
                NS_ASSERT(reason != WifiExpectedAccessReason::ACCESS_EXPECTED);
                NS_ASSERT(reason != WifiExpectedAccessReason::NOTHING_TO_TX);
                NS_ASSERT(reason != WifiExpectedAccessReason::NOT_REQUESTED);
                NS_ASSERT(reason != WifiExpectedAccessReason::BACKOFF_END);
                NS_LOG_DEBUG("Access grant start (" << accessGrantStart.As(Time::US)
                                                    << ") too late for reason " << reason);
                return reason;
            }
        }
        NS_ABORT_MSG("No reason found that exceeds the deadline!");
    }

    accessGrantStart += GetSifs();
    auto reason = WifiExpectedAccessReason::NOT_REQUESTED;

    for (auto txop : m_txops)
    {
        if (txop->GetAccessStatus(m_linkId) != Txop::REQUESTED)
        {
            continue;
        }

        if (!txop->HasFramesToTransmit(m_linkId))
        {
            if (reason != WifiExpectedAccessReason::BACKOFF_END)
            {
                reason = WifiExpectedAccessReason::NOTHING_TO_TX;
            }
            continue;
        }

        reason = WifiExpectedAccessReason::BACKOFF_END;
        const auto backoffEnd = GetBackoffEndFor(txop, accessGrantStart);

        if (backoffEnd >= now && backoffEnd <= deadline)
        {
            NS_LOG_DEBUG("Backoff end for " << txop->GetWifiMacQueue()->GetAc() << " on link "
                                            << +m_linkId << ": " << backoffEnd.As(Time::US));
            return WifiExpectedAccessReason::ACCESS_EXPECTED;
        }
    }

    NS_LOG_DEBUG("Access grant not expected for reason: " << reason);
    return reason;
}

Time
ChannelAccessManager::GetNavEnd() const
{
    return m_lastNavEnd;
}

void
ChannelAccessManager::UpdateBackoff()
{
    NS_LOG_FUNCTION(this);
    uint32_t k = 0;
    const auto accessGrantStart = GetAccessGrantStart();
    for (auto txop : m_txops)
    {
        Time backoffStart = GetBackoffStartFor(txop, accessGrantStart);
        if (backoffStart <= Simulator::Now())
        {
            uint32_t nIntSlots = ((Simulator::Now() - backoffStart) / GetSlot()).GetHigh();
            /*
             * EDCA behaves slightly different to DCA. For EDCA we
             * decrement once at the slot boundary at the end of AIFS as
             * well as once at the end of each clear slot
             * thereafter. For DCA we only decrement at the end of each
             * clear slot after DIFS. We account for the extra backoff
             * by incrementing the slot count here in the case of
             * EDCA. The if statement whose body we are in has confirmed
             * that a minimum of AIFS has elapsed since last busy
             * medium.
             */
            if (txop->IsQosTxop())
            {
                nIntSlots++;
            }
            uint32_t n = std::min(nIntSlots, txop->GetBackoffSlots(m_linkId));
            NS_LOG_DEBUG("dcf " << k << " dec backoff slots=" << n);
            Time backoffUpdateBound = backoffStart + (n * GetSlot());
            txop->UpdateBackoffSlotsNow(n, backoffUpdateBound, m_linkId);
        }
        ++k;
    }
}

void
ChannelAccessManager::DoRestartAccessTimeoutIfNeeded()
{
    NS_LOG_FUNCTION(this);
    /**
     * Is there a Txop which needs to access the medium, and,
     * if there is one, how many slots for AIFS+backoff does it require ?
     */
    Ptr<Txop> nextTxop;
    auto expectedBackoffEnd = Simulator::GetMaximumSimulationTime();
    const auto accessGrantStart = GetAccessGrantStart();
    const auto now = Simulator::Now();
    for (auto txop : m_txops)
    {
        if (txop->GetAccessStatus(m_linkId) == Txop::REQUESTED)
        {
            if (auto backoffEnd = GetBackoffEndFor(txop, accessGrantStart);
                backoffEnd > now && backoffEnd < expectedBackoffEnd)
            {
                expectedBackoffEnd = backoffEnd;
                nextTxop = txop;
            }
        }
    }
    NS_LOG_DEBUG("Access timeout needed: " << (nextTxop != nullptr));
    if (nextTxop)
    {
        const auto aci = nextTxop->GetWifiMacQueue()->GetAc();
        NS_LOG_DEBUG("expected backoff end=" << expectedBackoffEnd << " by " << aci);
        auto expectedBackoffDelay = expectedBackoffEnd - now;

        if (m_nSlotsLeft > 0)
        {
            const auto expectedNotifyTime =
                Max(expectedBackoffEnd - m_nSlotsLeft * GetSlot(),
                    accessGrantStart - GetSifs() + m_nSlotsLeftMinDelay);

            if (expectedNotifyTime > now)
            {
                // make the timer expire when it's time to notify that the given slots are left
                expectedBackoffDelay = expectedNotifyTime - now;
            }
            else
            {
                // notify that a number of slots less than or equal to the specified value are left
                m_nSlotsLeftCallback(m_linkId, aci, expectedBackoffDelay);
            }
        }

        if (m_accessTimeout.IsPending() &&
            Simulator::GetDelayLeft(m_accessTimeout) > expectedBackoffDelay)
        {
            m_accessTimeout.Cancel();
        }
        if (m_accessTimeout.IsExpired())
        {
            m_accessTimeout = Simulator::Schedule(expectedBackoffDelay,
                                                  &ChannelAccessManager::AccessTimeout,
                                                  this);
        }
    }
}

MHz_u
ChannelAccessManager::GetLargestIdlePrimaryChannel(Time interval, Time end)
{
    NS_LOG_FUNCTION(this << interval.As(Time::US) << end.As(Time::S));

    // If the medium is busy or it just became idle, UpdateLastIdlePeriod does
    // nothing. This allows us to call this method, e.g., at the end of a frame
    // reception and check the busy/idle status of the channel before the start
    // of the frame reception (last idle period was last updated at the start of
    // the frame reception).
    // If the medium has been idle for some time, UpdateLastIdlePeriod updates
    // the last idle period. This is normally what we want because this method may
    // also be called before starting a TXOP gained through EDCA.
    UpdateLastIdlePeriod();

    MHz_u width{0};

    // we iterate over the different types of channels in the same order as they
    // are listed in WifiChannelListType
    for (const auto& lastIdle : m_lastIdle)
    {
        if (lastIdle.second.start <= end - interval && lastIdle.second.end >= end)
        {
            // channel idle, update width
            width = (width == MHz_u{0}) ? MHz_u{20} : (2 * width);
        }
        else
        {
            break;
        }
    }
    return width;
}

bool
ChannelAccessManager::GetPer20MHzBusy(const std::set<uint8_t>& indices) const
{
    const auto now = Simulator::Now();

    if (m_phy->GetChannelWidth() < MHz_u{40})
    {
        NS_ASSERT_MSG(indices.size() == 1 && *indices.cbegin() == 0,
                      "Index 0 only can be specified if the channel width is less than 40 MHz");
        return m_lastBusyEnd.at(WIFI_CHANLIST_PRIMARY) > now;
    }

    for (const auto index : indices)
    {
        NS_ASSERT(index < m_lastPer20MHzBusyEnd.size());
        if (m_lastPer20MHzBusyEnd.at(index) > now)
        {
            NS_LOG_DEBUG("20 MHz channel with index " << +index << " is busy");
            return true;
        }
    }
    return false;
}

void
ChannelAccessManager::DisableEdcaFor(Ptr<Txop> qosTxop, Time duration)
{
    NS_LOG_FUNCTION(this << qosTxop << duration);
    NS_ASSERT(qosTxop->IsQosTxop());
    UpdateBackoff();
    Time resume = Simulator::Now() + duration;
    NS_LOG_DEBUG("Backoff will resume at time " << resume << " with "
                                                << qosTxop->GetBackoffSlots(m_linkId)
                                                << " remaining slot(s)");
    qosTxop->UpdateBackoffSlotsNow(0, resume, m_linkId);
    DoRestartAccessTimeoutIfNeeded();
}

void
ChannelAccessManager::SetGenerateBackoffOnNoTx(bool enable)
{
    NS_LOG_FUNCTION(this << enable);
    m_generateBackoffOnNoTx = enable;
}

bool
ChannelAccessManager::GetGenerateBackoffOnNoTx() const
{
    return m_generateBackoffOnNoTx;
}

void
ChannelAccessManager::NotifyRxStartNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    NS_LOG_DEBUG("rx start for=" << duration);
    UpdateBackoff();
    UpdateLastIdlePeriod();
    m_lastRx.start = Simulator::Now();
    m_lastRx.end = m_lastRx.start + duration;
    m_lastRxReceivedOk = true;
}

void
ChannelAccessManager::NotifyRxEndOkNow()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("rx end ok");
    m_lastRx.end = Simulator::Now();
    m_lastRxReceivedOk = true;
}

void
ChannelAccessManager::NotifyRxEndErrorNow(const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("rx end error");
    // we expect the PHY to notify us of the start of a CCA busy period, if needed
    m_lastRx.end = Simulator::Now();
    m_lastRxReceivedOk = false;
    m_eifsNoDifs = m_phy->GetSifs() + GetEstimatedAckTxTime(txVector);
}

void
ChannelAccessManager::NotifyTxStartNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    m_lastRxReceivedOk = true;
    Time now = Simulator::Now();
    if (m_lastRx.end > now)
    {
        // this may be caused only if PHY has started to receive a packet
        // inside SIFS, so, we check that lastRxStart was maximum a SIFS ago
        NS_ASSERT(now - m_lastRx.start <= GetSifs());
        m_lastRx.end = now;
    }
    else
    {
        UpdateLastIdlePeriod();
    }
    NS_LOG_DEBUG("tx start for " << duration);
    UpdateBackoff();
    m_lastTxEnd = now + duration;
}

void
ChannelAccessManager::NotifyCcaBusyStartNow(Time duration,
                                            WifiChannelListType channelType,
                                            const std::vector<Time>& per20MhzDurations)
{
    NS_LOG_FUNCTION(this << duration << channelType);
    UpdateBackoff();
    UpdateLastIdlePeriod();
    auto lastBusyEndIt = m_lastBusyEnd.find(channelType);
    NS_ASSERT(lastBusyEndIt != m_lastBusyEnd.end());
    Time now = Simulator::Now();
    lastBusyEndIt->second = now + duration;
    NS_ASSERT_MSG(per20MhzDurations.size() == m_lastPer20MHzBusyEnd.size(),
                  "Size of received vector (" << per20MhzDurations.size()
                                              << ") differs from the expected size ("
                                              << m_lastPer20MHzBusyEnd.size() << ")");
    for (std::size_t chIdx = 0; chIdx < per20MhzDurations.size(); ++chIdx)
    {
        if (per20MhzDurations[chIdx].IsStrictlyPositive())
        {
            m_lastPer20MHzBusyEnd[chIdx] = now + per20MhzDurations[chIdx];
        }
    }

    if (m_proactiveBackoff)
    {
        // have all EDCAFs that are not carrying out a TXOP and have the backoff counter set to
        // zero proactively generate a new backoff value
        for (auto txop : m_txops)
        {
            if (txop->GetAccessStatus(m_linkId) != Txop::GRANTED &&
                txop->GetBackoffSlots(m_linkId) == 0)
            {
                NS_LOG_DEBUG("Generate backoff for " << txop->GetWifiMacQueue()->GetAc());
                txop->GenerateBackoff(m_linkId);
            }
        }
    }
}

void
ChannelAccessManager::NotifySwitchingStartNow(PhyListener* phyListener, Time duration)
{
    NS_LOG_FUNCTION(this << phyListener << duration);

    Time now = Simulator::Now();
    NS_ASSERT(m_lastTxEnd <= now);

    if (phyListener) // to make tests happy
    {
        // check if the PHY switched channel to operate on another EMLSR link

        for (const auto& [phyRef, listener] : m_phyListeners)
        {
            Ptr<WifiPhy> phy = phyRef;
            auto emlsrInfoIt = m_switchingEmlsrLinks.find(phy);

            if (listener.get() == phyListener && emlsrInfoIt != m_switchingEmlsrLinks.cend() &&
                phy->GetOperatingChannel() == emlsrInfoIt->second.channel)
            {
                // the PHY associated with the given PHY listener switched channel to
                // operate on another EMLSR link as expected. We don't need this listener
                // anymore. The MAC will connect a new listener to the ChannelAccessManager
                // instance associated with the link the PHY is now operating on
                RemovePhyListener(phy);
                auto ehtFem = DynamicCast<EhtFrameExchangeManager>(m_feManager);
                NS_ASSERT(ehtFem);
                ehtFem->NotifySwitchingEmlsrLink(phy, emlsrInfoIt->second.linkId, duration);
                m_switchingEmlsrLinks.erase(emlsrInfoIt);
                return;
            }
        }
    }

    ResetState();

    // Cancel timeout
    if (m_accessTimeout.IsPending())
    {
        m_accessTimeout.Cancel();
    }

    // Reset backoffs
    for (const auto& txop : m_txops)
    {
        ResetBackoff(txop);
    }

    // Notify the FEM, which will in turn notify the MAC
    m_feManager->NotifySwitchingStartNow(duration);

    NS_LOG_DEBUG("switching start for " << duration);
    m_lastSwitchingEnd = now + duration;
}

void
ChannelAccessManager::ResetState()
{
    NS_LOG_FUNCTION(this);

    Time now = Simulator::Now();
    m_lastRxReceivedOk = true;
    UpdateLastIdlePeriod();
    m_lastRx.end = std::min(m_lastRx.end, now);
    m_lastNavEnd = std::min(m_lastNavEnd, now);
    m_lastAckTimeoutEnd = std::min(m_lastAckTimeoutEnd, now);
    m_lastCtsTimeoutEnd = std::min(m_lastCtsTimeoutEnd, now);
    m_lastNoPhy.end = std::min(m_lastNoPhy.end, now);
    m_lastSleep.end = std::min(m_lastSleep.end, now);
    m_lastOff.end = std::min(m_lastOff.end, now);

    InitLastBusyStructs();
}

void
ChannelAccessManager::ResetBackoff(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);

    uint32_t remainingSlots = txop->GetBackoffSlots(m_linkId);
    if (remainingSlots > 0)
    {
        txop->UpdateBackoffSlotsNow(remainingSlots, Simulator::Now(), m_linkId);
        NS_ASSERT(txop->GetBackoffSlots(m_linkId) == 0);
    }
    txop->ResetCw(m_linkId);
    txop->GetLink(m_linkId).access = Txop::NOT_REQUESTED;
}

void
ChannelAccessManager::ResetAllBackoffs()
{
    NS_LOG_FUNCTION(this);

    for (const auto& txop : m_txops)
    {
        ResetBackoff(txop);
    }
    m_accessTimeout.Cancel();
}

void
ChannelAccessManager::NotifySleepNow()
{
    NS_LOG_FUNCTION(this);
    UpdateBackoff();
    UpdateLastIdlePeriod();
    m_lastSleep.start = Simulator::Now();
    m_feManager->NotifySleepNow();
    for (auto txop : m_txops)
    {
        txop->NotifySleep(m_linkId);
    }
}

void
ChannelAccessManager::NotifyOffNow()
{
    NS_LOG_FUNCTION(this);
    UpdateBackoff();
    UpdateLastIdlePeriod();
    m_lastOff.start = Simulator::Now();
    m_feManager->NotifyOffNow();
    for (auto txop : m_txops)
    {
        txop->NotifyOff(m_linkId);
    }
}

void
ChannelAccessManager::NotifyWakeupNow()
{
    NS_LOG_FUNCTION(this);
    const auto now = Simulator::Now();
    m_lastSleep.end = now;
    if (now - m_lastSleep.start > m_resetBackoffThreshold)
    {
        ResetAllBackoffs();
    }
    for (auto txop : m_txops)
    {
        txop->NotifyWakeUp(m_linkId);
    }
}

void
ChannelAccessManager::NotifyOnNow()
{
    NS_LOG_FUNCTION(this);
    const auto now = Simulator::Now();
    m_lastOff.end = now;
    if (now - m_lastOff.start > m_resetBackoffThreshold)
    {
        ResetAllBackoffs();
    }
    for (auto txop : m_txops)
    {
        txop->NotifyOn();
    }
}

void
ChannelAccessManager::NotifyNavResetNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);

    if (!m_phy)
    {
        NS_LOG_DEBUG("Do not reset NAV, CTS may have been missed due to the main PHY switching "
                     "to another link to take over a TXOP while receiving the CTS");
        return;
    }

    NS_LOG_DEBUG("nav reset for=" << duration);
    UpdateBackoff();
    m_lastNavEnd = Simulator::Now() + duration;
    /**
     * If the NAV reset indicates an end-of-NAV which is earlier
     * than the previous end-of-NAV, the expected end of backoff
     * might be later than previously thought so, we might need
     * to restart a new access timeout.
     */
    DoRestartAccessTimeoutIfNeeded();
}

void
ChannelAccessManager::NotifyNavStartNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    NS_LOG_DEBUG("nav start for=" << duration);
    UpdateBackoff();
    m_lastNavEnd = std::max(m_lastNavEnd, Simulator::Now() + duration);
}

void
ChannelAccessManager::NotifyAckTimeoutStartNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    NS_ASSERT(m_lastAckTimeoutEnd < Simulator::Now());
    m_lastAckTimeoutEnd = Simulator::Now() + duration;
}

void
ChannelAccessManager::NotifyAckTimeoutResetNow()
{
    NS_LOG_FUNCTION(this);
    m_lastAckTimeoutEnd = Simulator::Now();
    DoRestartAccessTimeoutIfNeeded();
}

void
ChannelAccessManager::NotifyCtsTimeoutStartNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    m_lastCtsTimeoutEnd = Simulator::Now() + duration;
}

void
ChannelAccessManager::NotifyCtsTimeoutResetNow()
{
    NS_LOG_FUNCTION(this);
    m_lastCtsTimeoutEnd = Simulator::Now();
    DoRestartAccessTimeoutIfNeeded();
}

void
ChannelAccessManager::UpdateLastIdlePeriod()
{
    NS_LOG_FUNCTION(this);
    Time idleStart = std::max({m_lastTxEnd,
                               m_lastRx.end,
                               m_lastSwitchingEnd,
                               m_lastNoPhy.end,
                               m_lastSleep.end,
                               m_lastOff.end});
    Time now = Simulator::Now();

    if (idleStart >= now)
    {
        // No new idle period
        return;
    }

    for (const auto& busyEnd : m_lastBusyEnd)
    {
        if (busyEnd.second < now)
        {
            auto lastIdleIt = m_lastIdle.find(busyEnd.first);
            NS_ASSERT(lastIdleIt != m_lastIdle.end());
            lastIdleIt->second = {std::max(idleStart, busyEnd.second), now};
            NS_LOG_DEBUG("New idle period (" << lastIdleIt->second.start.As(Time::S) << ", "
                                             << lastIdleIt->second.end.As(Time::S)
                                             << ") on channel " << lastIdleIt->first);
        }
    }
}

std::ostream&
operator<<(std::ostream& os, const WifiExpectedAccessReason& reason)
{
    switch (reason)
    {
    case WifiExpectedAccessReason::ACCESS_EXPECTED:
        return (os << "ACCESS EXPECTED");
    case WifiExpectedAccessReason::NOT_REQUESTED:
        return (os << "NOT_REQUESTED");
    case WifiExpectedAccessReason::NOTHING_TO_TX:
        return (os << "NOTHING_TO_TX");
    case WifiExpectedAccessReason::RX_END:
        return (os << "RX_END");
    case WifiExpectedAccessReason::BUSY_END:
        return (os << "BUSY_END");
    case WifiExpectedAccessReason::TX_END:
        return (os << "TX_END");
    case WifiExpectedAccessReason::NAV_END:
        return (os << "NAV_END");
    case WifiExpectedAccessReason::ACK_TIMER_END:
        return (os << "ACK_TIMER_END");
    case WifiExpectedAccessReason::CTS_TIMER_END:
        return (os << "CTS_TIMER_END");
    case WifiExpectedAccessReason::SWITCHING_END:
        return (os << "SWITCHING_END");
    case WifiExpectedAccessReason::NO_PHY_END:
        return (os << "NO_PHY_END");
    case WifiExpectedAccessReason::SLEEP_END:
        return (os << "SLEEP_END");
    case WifiExpectedAccessReason::OFF_END:
        return (os << "OFF_END");
    case WifiExpectedAccessReason::BACKOFF_END:
        return (os << "BACKOFF_END");
    default:
        NS_ABORT_MSG("Unknown expected access reason");
        return (os << "Unknown");
    }
}

} // namespace ns3
