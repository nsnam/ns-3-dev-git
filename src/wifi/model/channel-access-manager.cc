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

#include "channel-access-manager.h"

#include "frame-exchange-manager.h"
#include "txop.h"
#include "wifi-phy-listener.h"
#include "wifi-phy.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "] "

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ChannelAccessManager");

/**
 * Listener for PHY events. Forwards to ChannelAccessManager
 */
class PhyListener : public ns3::WifiPhyListener
{
  public:
    /**
     * Create a PhyListener for the given ChannelAccessManager.
     *
     * \param cam the ChannelAccessManager
     */
    PhyListener(ns3::ChannelAccessManager* cam)
        : m_cam(cam)
    {
    }

    ~PhyListener() override
    {
    }

    void NotifyRxStart(Time duration) override
    {
        m_cam->NotifyRxStartNow(duration);
    }

    void NotifyRxEndOk() override
    {
        m_cam->NotifyRxEndOkNow();
    }

    void NotifyRxEndError() override
    {
        m_cam->NotifyRxEndErrorNow();
    }

    void NotifyTxStart(Time duration, double txPowerDbm) override
    {
        m_cam->NotifyTxStartNow(duration);
    }

    void NotifyCcaBusyStart(Time duration,
                            WifiChannelListType channelType,
                            const std::vector<Time>& per20MhzDurations) override
    {
        m_cam->NotifyCcaBusyStartNow(duration, channelType, per20MhzDurations);
    }

    void NotifySwitchingStart(Time duration) override
    {
        m_cam->NotifySwitchingStartNow(duration);
    }

    void NotifySleep() override
    {
        m_cam->NotifySleepNow();
    }

    void NotifyOff() override
    {
        m_cam->NotifyOffNow();
    }

    void NotifyWakeup() override
    {
        m_cam->NotifyWakeupNow();
    }

    void NotifyOn() override
    {
        m_cam->NotifyOnNow();
    }

  private:
    ns3::ChannelAccessManager* m_cam; //!< ChannelAccessManager to forward events to
};

/****************************************************************
 *      Implement the channel access manager of all Txop holders
 ****************************************************************/

ChannelAccessManager::ChannelAccessManager()
    : m_lastAckTimeoutEnd(MicroSeconds(0)),
      m_lastCtsTimeoutEnd(MicroSeconds(0)),
      m_lastNavEnd(MicroSeconds(0)),
      m_lastRx({MicroSeconds(0), MicroSeconds(0)}),
      m_lastRxReceivedOk(true),
      m_lastTxEnd(MicroSeconds(0)),
      m_lastSwitchingEnd(MicroSeconds(0)),
      m_sleeping(false),
      m_off(false),
      m_phyListener(nullptr),
      m_linkId(0)
{
    NS_LOG_FUNCTION(this);
    InitLastBusyStructs();
}

ChannelAccessManager::~ChannelAccessManager()
{
    NS_LOG_FUNCTION(this);
    delete m_phyListener;
    m_phyListener = nullptr;
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
    for (Ptr<Txop> i : m_txops)
    {
        i->Dispose();
        i = nullptr;
    }
    m_phy = nullptr;
    m_feManager = nullptr;
}

void
ChannelAccessManager::SetupPhyListener(Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    NS_ASSERT(m_phyListener == nullptr);
    m_phyListener = new PhyListener(this);
    phy->RegisterListener(m_phyListener);
    m_phy = phy;
    InitLastBusyStructs();
}

void
ChannelAccessManager::RemovePhyListener(Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    if (m_phyListener != nullptr)
    {
        phy->UnregisterListener(m_phyListener);
        delete m_phyListener;
        m_phyListener = nullptr;
        m_phy = nullptr;
    }
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
    return m_phy->GetSlot();
}

Time
ChannelAccessManager::GetSifs() const
{
    return m_phy->GetSifs();
}

Time
ChannelAccessManager::GetEifsNoDifs() const
{
    return m_phy->GetSifs() + m_phy->GetAckTxTime();
}

void
ChannelAccessManager::Add(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);
    m_txops.push_back(txop);
}

void
ChannelAccessManager::InitLastBusyStructs()
{
    NS_LOG_FUNCTION(this);
    Time now = Simulator::Now();
    m_lastBusyEnd.clear();
    m_lastPer20MHzBusyEnd.clear();
    m_lastIdle.clear();
    m_lastBusyEnd[WIFI_CHANLIST_PRIMARY] = now;
    m_lastIdle[WIFI_CHANLIST_PRIMARY] = {now, now};

    if (!m_phy || !m_phy->GetOperatingChannel().IsOfdm())
    {
        return;
    }

    uint16_t width = m_phy->GetChannelWidth();

    if (width >= 40)
    {
        m_lastBusyEnd[WIFI_CHANLIST_SECONDARY] = now;
        m_lastIdle[WIFI_CHANLIST_SECONDARY] = {now, now};
    }
    if (width >= 80)
    {
        m_lastBusyEnd[WIFI_CHANLIST_SECONDARY40] = now;
        m_lastIdle[WIFI_CHANLIST_SECONDARY40] = {now, now};
    }
    if (width >= 160)
    {
        m_lastBusyEnd[WIFI_CHANLIST_SECONDARY80] = now;
        m_lastIdle[WIFI_CHANLIST_SECONDARY80] = {now, now};
    }
    // TODO Add conditions for new channel widths as they get supported

    if (m_phy->GetStandard() >= WIFI_STANDARD_80211ax && width > 20)
    {
        m_lastPer20MHzBusyEnd.assign(width / 20, now);
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
ChannelAccessManager::NeedBackoffUponAccess(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);

    // No backoff needed if in sleep mode or off
    if (m_sleeping || m_off)
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
    if (!txop->HasFramesToTransmit(m_linkId) && txop->GetAccessStatus(m_linkId) != Txop::GRANTED &&
        txop->GetBackoffSlots(m_linkId) == 0)
    {
        if (!IsBusy())
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
    if (m_phy)
    {
        m_phy->NotifyChannelAccessRequested();
    }
    // Deny access if in sleep mode or off
    if (m_sleeping || m_off)
    {
        return;
    }
    /*
     * EDCAF operations shall be performed at slot boundaries (Sec. 10.22.2.4 of 802.11-2016)
     */
    Time accessGrantStart = GetAccessGrantStart() + (txop->GetAifsn(m_linkId) * GetSlot());

    if (txop->IsQosTxop() && txop->GetBackoffStart(m_linkId) > accessGrantStart)
    {
        // The backoff start time reported by the EDCAF is more recent than the last
        // time the medium was busy plus an AIFS, hence we need to align it to the
        // next slot boundary.
        Time diff = txop->GetBackoffStart(m_linkId) - accessGrantStart;
        uint32_t nIntSlots = (diff / GetSlot()).GetHigh() + 1;
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
    Time now = Simulator::Now();
    for (Txops::iterator i = m_txops.begin(); i != m_txops.end(); k++)
    {
        Ptr<Txop> txop = *i;
        if (txop->GetAccessStatus(m_linkId) == Txop::REQUESTED &&
            (!txop->IsQosTxop() || !StaticCast<QosTxop>(txop)->EdcaDisabled(m_linkId)) &&
            GetBackoffEndFor(txop) <= now)
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
            for (Txops::iterator j = i; j != m_txops.end(); j++, k++)
            {
                Ptr<Txop> otherTxop = *j;
                if (otherTxop->GetAccessStatus(m_linkId) == Txop::REQUESTED &&
                    GetBackoffEndFor(otherTxop) <= now)
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
            auto width = (m_phy->GetOperatingChannel().IsOfdm() && m_phy->GetChannelWidth() > 20)
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
    UpdateBackoff();
    DoGrantDcfAccess();
    DoRestartAccessTimeoutIfNeeded();
}

Time
ChannelAccessManager::GetAccessGrantStart(bool ignoreNav) const
{
    NS_LOG_FUNCTION(this);
    const Time& sifs = GetSifs();
    Time rxAccessStart = m_lastRx.end + sifs;
    if ((m_lastRx.end <= Simulator::Now()) && !m_lastRxReceivedOk)
    {
        rxAccessStart += GetEifsNoDifs();
    }
    // an EDCA TXOP is obtained based solely on activity of the primary channel
    // (Sec. 10.23.2.5 of IEEE 802.11-2020)
    Time busyAccessStart = m_lastBusyEnd.at(WIFI_CHANLIST_PRIMARY) + sifs;
    Time txAccessStart = m_lastTxEnd + sifs;
    Time navAccessStart = m_lastNavEnd + sifs;
    Time ackTimeoutAccessStart = m_lastAckTimeoutEnd + sifs;
    Time ctsTimeoutAccessStart = m_lastCtsTimeoutEnd + sifs;
    Time switchingAccessStart = m_lastSwitchingEnd + sifs;
    Time accessGrantedStart;
    if (ignoreNav)
    {
        accessGrantedStart = std::max({rxAccessStart,
                                       busyAccessStart,
                                       txAccessStart,
                                       ackTimeoutAccessStart,
                                       ctsTimeoutAccessStart,
                                       switchingAccessStart});
    }
    else
    {
        accessGrantedStart = std::max({rxAccessStart,
                                       busyAccessStart,
                                       txAccessStart,
                                       navAccessStart,
                                       ackTimeoutAccessStart,
                                       ctsTimeoutAccessStart,
                                       switchingAccessStart});
    }
    NS_LOG_INFO("access grant start=" << accessGrantedStart << ", rx access start=" << rxAccessStart
                                      << ", busy access start=" << busyAccessStart
                                      << ", tx access start=" << txAccessStart
                                      << ", nav access start=" << navAccessStart);
    return accessGrantedStart;
}

Time
ChannelAccessManager::GetBackoffStartFor(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);
    Time mostRecentEvent =
        std::max({txop->GetBackoffStart(m_linkId),
                  GetAccessGrantStart() + (txop->GetAifsn(m_linkId) * GetSlot())});
    NS_LOG_DEBUG("Backoff start: " << mostRecentEvent.As(Time::US));

    return mostRecentEvent;
}

Time
ChannelAccessManager::GetBackoffEndFor(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);
    Time backoffEnd = GetBackoffStartFor(txop) + (txop->GetBackoffSlots(m_linkId) * GetSlot());
    NS_LOG_DEBUG("Backoff end: " << backoffEnd.As(Time::US));

    return backoffEnd;
}

void
ChannelAccessManager::UpdateBackoff()
{
    NS_LOG_FUNCTION(this);
    uint32_t k = 0;
    for (auto txop : m_txops)
    {
        Time backoffStart = GetBackoffStartFor(txop);
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
    bool accessTimeoutNeeded = false;
    Time expectedBackoffEnd = Simulator::GetMaximumSimulationTime();
    for (auto txop : m_txops)
    {
        if (txop->GetAccessStatus(m_linkId) == Txop::REQUESTED)
        {
            Time tmp = GetBackoffEndFor(txop);
            if (tmp > Simulator::Now())
            {
                accessTimeoutNeeded = true;
                expectedBackoffEnd = std::min(expectedBackoffEnd, tmp);
            }
        }
    }
    NS_LOG_DEBUG("Access timeout needed: " << accessTimeoutNeeded);
    if (accessTimeoutNeeded)
    {
        NS_LOG_DEBUG("expected backoff end=" << expectedBackoffEnd);
        Time expectedBackoffDelay = expectedBackoffEnd - Simulator::Now();
        if (m_accessTimeout.IsRunning() &&
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

uint16_t
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

    uint16_t width = 0;

    // we iterate over the different types of channels in the same order as they
    // are listed in WifiChannelListType
    for (const auto& lastIdle : m_lastIdle)
    {
        if (lastIdle.second.start <= end - interval && lastIdle.second.end >= end)
        {
            // channel idle, update width
            width = (width == 0) ? 20 : (2 * width);
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

    if (m_phy->GetChannelWidth() < 40)
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
ChannelAccessManager::NotifyRxEndErrorNow()
{
    NS_LOG_FUNCTION(this);
    NS_LOG_DEBUG("rx end error");
    // we expect the PHY to notify us of the start of a CCA busy period, if needed
    m_lastRx.end = Simulator::Now();
    m_lastRxReceivedOk = false;
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
}

void
ChannelAccessManager::NotifySwitchingStartNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    Time now = Simulator::Now();
    NS_ASSERT(m_lastTxEnd <= now);
    NS_ASSERT(m_lastSwitchingEnd <= now);

    m_lastRxReceivedOk = true;
    UpdateLastIdlePeriod();
    m_lastRx.end = std::min(m_lastRx.end, now);
    m_lastNavEnd = std::min(m_lastNavEnd, now);
    m_lastAckTimeoutEnd = std::min(m_lastAckTimeoutEnd, now);
    m_lastCtsTimeoutEnd = std::min(m_lastCtsTimeoutEnd, now);

    // the new operating channel may have a different width than the previous one
    InitLastBusyStructs();

    // Cancel timeout
    if (m_accessTimeout.IsRunning())
    {
        m_accessTimeout.Cancel();
    }

    // Notify the FEM, which will in turn notify the MAC
    m_feManager->NotifySwitchingStartNow(duration);

    // Reset backoffs
    for (auto txop : m_txops)
    {
        uint32_t remainingSlots = txop->GetBackoffSlots(m_linkId);
        if (remainingSlots > 0)
        {
            txop->UpdateBackoffSlotsNow(remainingSlots, now, m_linkId);
            NS_ASSERT(txop->GetBackoffSlots(m_linkId) == 0);
        }
        txop->ResetCw(m_linkId);
        txop->GetLink(m_linkId).access = Txop::NOT_REQUESTED;
    }

    NS_LOG_DEBUG("switching start for " << duration);
    m_lastSwitchingEnd = now + duration;
}

void
ChannelAccessManager::NotifySleepNow()
{
    NS_LOG_FUNCTION(this);
    m_sleeping = true;
    // Cancel timeout
    if (m_accessTimeout.IsRunning())
    {
        m_accessTimeout.Cancel();
    }

    // Reset backoffs
    for (auto txop : m_txops)
    {
        txop->NotifySleep(m_linkId);
    }
}

void
ChannelAccessManager::NotifyOffNow()
{
    NS_LOG_FUNCTION(this);
    m_off = true;
    // Cancel timeout
    if (m_accessTimeout.IsRunning())
    {
        m_accessTimeout.Cancel();
    }

    // Reset backoffs
    for (auto txop : m_txops)
    {
        txop->NotifyOff();
    }
}

void
ChannelAccessManager::NotifyWakeupNow()
{
    NS_LOG_FUNCTION(this);
    m_sleeping = false;
    for (auto txop : m_txops)
    {
        uint32_t remainingSlots = txop->GetBackoffSlots(m_linkId);
        if (remainingSlots > 0)
        {
            txop->UpdateBackoffSlotsNow(remainingSlots, Simulator::Now(), m_linkId);
            NS_ASSERT(txop->GetBackoffSlots(m_linkId) == 0);
        }
        txop->ResetCw(m_linkId);
        txop->GetLink(m_linkId).access = Txop::NOT_REQUESTED;
        txop->NotifyWakeUp(m_linkId);
    }
}

void
ChannelAccessManager::NotifyOnNow()
{
    NS_LOG_FUNCTION(this);
    m_off = false;
    for (auto txop : m_txops)
    {
        uint32_t remainingSlots = txop->GetBackoffSlots(m_linkId);
        if (remainingSlots > 0)
        {
            txop->UpdateBackoffSlotsNow(remainingSlots, Simulator::Now(), m_linkId);
            NS_ASSERT(txop->GetBackoffSlots(m_linkId) == 0);
        }
        txop->ResetCw(m_linkId);
        txop->GetLink(m_linkId).access = Txop::NOT_REQUESTED;
        txop->NotifyOn();
    }
}

void
ChannelAccessManager::NotifyNavResetNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
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
    Time idleStart = std::max({m_lastTxEnd, m_lastRx.end, m_lastSwitchingEnd});
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

} // namespace ns3
