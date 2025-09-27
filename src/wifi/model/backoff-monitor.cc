/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "backoff-monitor.h"

#include "channel-access-manager.h"
#include "txop.h"
#include "wifi-mac-queue.h"
#include "wifi-mac.h"
#include "wifi-net-device.h"
#include "wifi-phy-listener.h"
#include "wifi-phy.h"

#include "ns3/log.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[" << m_aci << "] "

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("BackoffMonitor");

/**
 * Listener for PHY events. Forwards to BackoffMonitor.
 */
class BackoffMonPhyListener : public ns3::WifiPhyListener
{
  public:
    /**
     * Create a PhyListener for the given BackoffMonitor and the given PHY.
     *
     * @param backoffMon the BackoffMonitor
     * @param phyId the ID of the given PHY
     */
    BackoffMonPhyListener(BackoffMonitor& backoffMon, uint8_t phyId)
        : m_backoffMon(backoffMon),
          m_phyId(phyId)
    {
    }

    void NotifyRxStart(Time duration) override
    {
        m_backoffMon.NotifyMediumBusy(m_phyId, duration);
    }

    void NotifyRxEndOk() override
    {
        m_backoffMon.NotifyMediumBusy(m_phyId, Time{0});
    }

    void NotifyRxEndError(const WifiTxVector& txVector) override
    {
        m_backoffMon.NotifyMediumBusy(m_phyId, Time{0});
    }

    void NotifyTxStart(Time duration, dBm_t txPower) override
    {
        m_backoffMon.NotifyMediumBusy(m_phyId, duration);
    }

    void NotifyCcaBusyStart(Time duration,
                            WifiChannelListType channelType,
                            const std::vector<Time>& per20MhzDurations) override
    {
        m_backoffMon.NotifyMediumBusy(m_phyId, duration);
    }

    void NotifySwitchingStart(Time duration) override
    {
        // Do nothing:
        // - in case of EMLSR link switch, the backoff is reset if no PHY is on the link for more
        //   than a given time, otherwise the backoff keeps counting down
        // - otherwise, the backoff is reset
    }

    void NotifySleep() override
    {
        // schedule now to decrease the priority of this listener
        Simulator::ScheduleNow(&BackoffMonitor::NotifyPhySleepOrOff, &m_backoffMon, m_phyId);
    }

    void NotifyOff() override
    {
        // schedule now to decrease the priority of this listener
        Simulator::ScheduleNow(&BackoffMonitor::NotifyPhySleepOrOff, &m_backoffMon, m_phyId);
    }

    void NotifyWakeup() override
    {
        // schedule now to decrease the priority of this listener
        Simulator::ScheduleNow(&BackoffMonitor::NotifyPhyResume, &m_backoffMon, m_phyId);
    }

    void NotifyOn() override
    {
        // schedule now to decrease the priority of this listener
        Simulator::ScheduleNow(&BackoffMonitor::NotifyPhyResume, &m_backoffMon, m_phyId);
    }

  private:
    BackoffMonitor& m_backoffMon; //!< BackoffMonitor to forward events to
    uint8_t m_phyId;              //!< the ID of the PHY listening to
};

/****************************************************************
 *      Implement the backoff monitor
 ****************************************************************/

BackoffMonitor::~BackoffMonitor()
{
    NS_LOG_FUNCTION(this);
}

void
BackoffMonitor::Enable(Ptr<WifiMac> mac, Ptr<Txop> txop, StatusChangeCb cb)
{
    if (const auto queue = txop->GetWifiMacQueue())
    {
        m_aci = queue->GetAc();
    }
    NS_LOG_FUNCTION(this << mac << m_aci);
    m_mac = mac;
    NS_ASSERT_MSG(!cb.IsNull(), "Cannot connect a null callback");
    m_enabled = true;
    Callback<uint32_t, linkId_t> getSlotsCb = [=](linkId_t linkId) {
        // trigger a backoff update
        mac->GetChannelAccessManager(linkId)->NeedBackoffUponAccess(txop, true, true);
        return txop->GetBackoffSlots(linkId);
    };

    // create an entry in m_states for every link
    for (const auto linkId : m_mac->GetLinkIds())
    {
        auto state = State{.linkId = linkId, .statusChangeCb = cb, .getBackoffSlotsCb = getSlotsCb};
        m_states.emplace_back(state);
    }

    // connect a listener to every PHY
    for (uint8_t phyId = 0; phyId < mac->GetDevice()->GetNPhys(); ++phyId)
    {
        auto listener = std::make_shared<BackoffMonPhyListener>(*this, phyId);
        if (const auto [it, inserted] = m_listeners.emplace(phyId, listener); inserted)
        {
            mac->GetDevice()->GetPhy(phyId)->RegisterListener(listener);
        }
    }

    // connect NAV end callback to all channel access managers
    for (const auto linkId : m_mac->GetLinkIds())
    {
        auto cam = m_mac->GetChannelAccessManager(linkId);
        cam->TraceConnectWithoutContext(
            "NavEnd",
            MakeCallback(&BackoffMonitor::NotifyNavUpdated, this).Bind(cam));
    }

    // connect a callback to the channel access status trace
    txop->TraceConnectWithoutContext(
        "ChannelAccessStatus",
        MakeCallback(&BackoffMonitor::NotifyChannelAccessStatusChange, this));
}

void
BackoffMonitor::Disable()
{
    NS_LOG_FUNCTION(this);

    if (!m_enabled)
    {
        return;
    }

    for (const auto linkId : m_mac->GetLinkIds())
    {
        if (auto cam = m_mac->GetChannelAccessManager(linkId))
        {
            cam->TraceDisconnectWithoutContext(
                "NavEnd",
                MakeCallback(&BackoffMonitor::NotifyNavUpdated, this).Bind(cam));
        }
    }
    for (const auto& [phyId, listener] : m_listeners)
    {
        if (auto phy = m_mac->GetDevice()->GetPhy(phyId); phy && phy->GetState())
        {
            phy->UnregisterListener(listener);
        }
    }
    m_listeners.clear();
    m_states.clear();
    m_enabled = false;
    m_aci = AC_UNDEF;
    m_mac = nullptr;
}

void
BackoffMonitor::SwapLinks(const std::map<linkId_t, linkId_t>& links)
{
    NS_LOG_FUNCTION(this);

    for (auto& state : m_states)
    {
        if (links.contains(state.linkId))
        {
            state.linkId = links.at(state.linkId);
        }
    }
}

BackoffMonitor::State&
BackoffMonitor::GetState(linkId_t linkId)
{
    const auto it = std::find_if(m_states.begin(), m_states.end(), [=](auto&& state) {
        return state.linkId == linkId;
    });
    NS_ASSERT_MSG(it != m_states.end(), "State for link " << +linkId << " not found");
    return *it;
}

BackoffStatus
BackoffMonitor::GetBackoffStatus(linkId_t linkId) const
{
    const auto it = std::find_if(m_states.cbegin(), m_states.cend(), [=](auto&& state) {
        return state.linkId == linkId;
    });
    return (it != m_states.cend() ? it->backoffStatus : BackoffStatus::UNKNOWN);
}

void
BackoffMonitor::State::SetStatus(BackoffStatus status)
{
    const auto prevStatus = backoffStatus;
    backoffStatus = status;
    auto slots = getBackoffSlotsCb(linkId);
    statusChangeCb(
        {.linkId = linkId, .prevStatus = prevStatus, .currStatus = status, .counter = slots});
}

bool
BackoffMonitor::SwitchToPausedIfBusy(linkId_t linkId)
{
    NS_LOG_FUNCTION(this);

    auto& state = GetState(linkId);
    auto phy = m_mac->GetWifiPhy(linkId);
    NS_ASSERT(phy);
    const auto accessGrantStart =
        m_mac->GetChannelAccessManager(linkId)->GetAccessGrantStart() - phy->GetSifs();
    if (const auto now = Simulator::Now(); accessGrantStart > now)
    {
        // medium is busy
        if (state.backoffStatus != BackoffStatus::PAUSED)
        {
            state.SetStatus(BackoffStatus::PAUSED);
        }
        const auto timeout = accessGrantStart + GetTxopEndTimeout(linkId) - now;
        NS_LOG_DEBUG("Medium busy, resume event set to expire in " << timeout.As(Time::US)
                                                                   << " at time " << now + timeout);
        state.statusChangeEvent.Cancel();
        state.statusChangeEvent = Simulator::Schedule(timeout,
                                                      &BackoffMonitor::State::SetStatus,
                                                      &state,
                                                      BackoffStatus::ONGOING);

        return true;
    }
    return false;
}

void
BackoffMonitor::NotifyBackoffGenerated(linkId_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    if (m_enabled)
    {
        auto& state = GetState(linkId);

        if (state.backoffStatus == BackoffStatus::UNKNOWN)
        {
            // initialize physical and virtual CS end
            state.physicalCsEnd = Simulator::Now() + m_mac->GetWifiPhy(linkId)->GetDelayUntilIdle();
            state.virtualCsEnd = m_mac->GetChannelAccessManager(linkId)->GetNavEnd();
        }

        state.SetStatus(BackoffStatus::ONGOING);
        SwitchToPausedIfBusy(linkId);
    }
}

void
BackoffMonitor::NotifyChannelAccessStatusChange(const ChannelAccessStatusTrace& info)
{
    NS_LOG_FUNCTION(this << info.linkId << info.prevStatus << info.currStatus);

    // if a backoff has been generated, notify it first, because the backoff status may be UNKNOWN
    // and notifying the backoff generation creates the state information that is used below
    if (info.currStatus == WifiChannelAccessStatus::NOT_REQUESTED_WITH_BACKOFF)
    {
        NotifyBackoffGenerated(info.linkId);
    }

    const auto status = GetBackoffStatus(info.linkId);

    if (status == BackoffStatus::UNKNOWN)
    {
        return;
    }

    auto& state = GetState(info.linkId);
    state.accessStatus = info.currStatus;

    switch (state.accessStatus)
    {
    case WifiChannelAccessStatus::GRANTED:
        // channel access granted, backoff counter has reached zero
        state.SetStatus(BackoffStatus::ZERO);
        break;
    case WifiChannelAccessStatus::NOT_REQUESTED_NO_BACKOFF:
        // channel access status transitions from GRANTED to NOT_REQUESTED_NO_BACKOFF when the
        // channel is released after being accessed; in this case, the backoff status has been
        // already set to ZERO and nothing needs to be done; channel access status transitions from
        // BACKOFF_GENERATED or REQUESTED to NOT_REQUESTED_NO_BACKOFF when the backoff is reset, or
        // the backoff counter reaches zero without channel access having been requested, or channel
        // access is granted but nothing is transmitted, or in case of internal collision: in all
        // such cases, the backoff status must be ZERO
        if (info.prevStatus != WifiChannelAccessStatus::NOT_REQUESTED_NO_BACKOFF &&
            info.prevStatus != WifiChannelAccessStatus::GRANTED)
        {
            state.SetStatus(BackoffStatus::ZERO);
        }
        break;
    default:
        break;
    }
}

void
BackoffMonitor::NotifyBackoffReset(linkId_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    if (GetBackoffStatus(linkId) != BackoffStatus::UNKNOWN)
    {
        GetState(linkId).SetStatus(BackoffStatus::RESET);
    }
}

void
BackoffMonitor::NotifyMediumBusy(uint8_t phyId, Time duration)
{
    NS_LOG_FUNCTION(this << phyId << duration);

    if (const auto linkId = m_mac->GetLinkForPhy(phyId))
    {
        // schedule now to give lower priority to PHY listener connected to backoff monitor
        Simulator::ScheduleNow(&BackoffMonitor::HandleMediumBusyUpdates,
                               this,
                               *linkId,
                               Simulator::Now() + duration,
                               std::nullopt);
    }
    // else, PHY is not operating on any link
}

void
BackoffMonitor::NotifyNavUpdated(Ptr<ChannelAccessManager> cam, Time oldNav, Time newNav)
{
    const auto linkId = cam->GetLinkId();
    NS_LOG_FUNCTION(this << linkId << oldNav << newNav);

    HandleMediumBusyUpdates(linkId, std::nullopt, newNav);
}

void
BackoffMonitor::HandleMediumBusyUpdates(linkId_t linkId,
                                        std::optional<Time> physicalCsEnd,
                                        std::optional<Time> virtualCsEnd)
{
    NS_LOG_FUNCTION(this << linkId << physicalCsEnd.has_value() << virtualCsEnd.has_value());

    if (GetBackoffStatus(linkId) == BackoffStatus::UNKNOWN)
    {
        return; // backoff monitor disabled or status not tracked yet
    }

    auto& state = GetState(linkId);
    auto navReset{false};

    if (physicalCsEnd.has_value())
    {
        state.physicalCsEnd = Max(state.physicalCsEnd, *physicalCsEnd);
    }
    if (virtualCsEnd.has_value())
    {
        navReset = (*virtualCsEnd < state.virtualCsEnd);
        // NAV may be reset
        state.virtualCsEnd = *virtualCsEnd;
    }

    const auto now = Simulator::Now();
    const auto timeout =
        (navReset ? state.virtualCsEnd - now // NAV is only reset when medium is idle
                  : Max(state.physicalCsEnd, state.virtualCsEnd) + GetTxopEndTimeout(linkId) - now);

    using enum BackoffStatus;
    switch (state.backoffStatus)
    {
    case PAUSED:
        // if this busy notification modifies the current medium busy period, reschedule the
        // resume event
        if (Simulator::GetDelayLeft(state.statusChangeEvent) != timeout)
        {
            NS_LOG_DEBUG("Resume event set to expire in " << timeout.As(Time::US) << " at time "
                                                          << now + timeout);
            state.statusChangeEvent.Cancel();
            state.statusChangeEvent =
                Simulator::Schedule(timeout, &BackoffMonitor::State::SetStatus, &state, ONGOING);
        }
        break;
    case ONGOING:
        if (state.physicalCsEnd <= now)
        {
            // this may happen, e.g., on Ack RX end if the notification from the PHY listener is
            // processed after the generation of a new backoff value; this is not actually an
            // interruption
            return;
        }
        // set the backoff status to paused and schedule the resume event
        state.SetStatus(PAUSED);
        NS_LOG_DEBUG("Resume event set to expire in " << timeout.As(Time::US) << " at time "
                                                      << now + timeout);
        state.statusChangeEvent.Cancel();
        state.statusChangeEvent =
            Simulator::Schedule(timeout, &BackoffMonitor::State::SetStatus, &state, ONGOING);
        break;
    case ZERO:
    case RESET:
    case UNKNOWN:
        // ignore the medium busy notification
        break;
    }
}

Time
BackoffMonitor::GetTxopEndTimeout(linkId_t linkId) const
{
    auto phy = m_mac->GetWifiPhy(linkId);
    NS_ASSERT(phy);
    // if the medium is idle for more than a SIFS but less than a PIFS, then we consider the TXOP
    // ended
    return phy->GetSifs() + (phy->GetSlot() / 2);
}

void
BackoffMonitor::NotifyPhySleepOrOff(uint8_t phyId)
{
    NS_LOG_FUNCTION(this << phyId);

    const auto linkId = m_mac->GetLinkForPhy(phyId);

    if (!linkId)
    {
        return; // the PHY is not operating on any link
    }

    if (GetBackoffStatus(*linkId) == BackoffStatus::ONGOING)
    {
        GetState(*linkId).SetStatus(BackoffStatus::PAUSED);
    }
    if (GetBackoffStatus(*linkId) == BackoffStatus::PAUSED)
    {
        // cancel the resume event because we don't know when the PHY will resume
        GetState(*linkId).statusChangeEvent.Cancel();
    }
}

void
BackoffMonitor::NotifyPhyResume(uint8_t phyId)
{
    NS_LOG_FUNCTION(this << phyId);

    const auto linkId = m_mac->GetLinkForPhy(phyId);

    if (!linkId)
    {
        return; // the PHY is not operating on any link
    }

    // If the backoff is reset because the time spent by the PHY in sleep/off state exceeds the
    // threshold, the backoff status is set to RESET and then to ZERO; when the PHY is resumed, a
    // backoff value may be generated and the backoff status set to ONGOING. Either way, we have
    // to do nothing.
    // Otherwise, if the backoff status was ZERO when the PHY was put to sleep/off, it is still ZERO
    // and we have to do nothing. Thus, if the backoff status is now PAUSED, it means that it was
    // either PAUSED or ONGOING when the PHY was put to sleep/off and it must be now set to ONGOING,
    // unless the medium is busy (in which case, it is set or stays equal to PAUSED).
    if ((GetBackoffStatus(*linkId) == BackoffStatus::PAUSED) && !SwitchToPausedIfBusy(*linkId))
    {
        GetState(*linkId).SetStatus(BackoffStatus::ONGOING);
    }
}

std::ostream&
operator<<(std::ostream& os, BackoffStatus status)
{
    using enum BackoffStatus;

    switch (status)
    {
    case ONGOING:
        os << "ONGOING";
        break;
    case PAUSED:
        os << "PAUSED";
        break;
    case ZERO:
        os << "ZERO";
        break;
    case RESET:
        os << "RESET";
        break;
    case UNKNOWN:
        os << "UNKNOWN";
        break;
    default:
        NS_ABORT_MSG("Invalid backoff status: " << static_cast<uint16_t>(status));
    }
    return os;
}

} // namespace ns3
