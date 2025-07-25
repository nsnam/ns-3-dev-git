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
        // Do nothing, the backoff is reset
    }

    void NotifyOff() override
    {
        // Do nothing, the backoff is reset
    }

    void NotifyWakeup() override
    {
        // Do nothing, the backoff is reset
    }

    void NotifyOn() override
    {
        // Do nothing, the backoff is reset
    }

  private:
    BackoffMonitor& m_backoffMon; //!< BackoffMonitor to forward events to
    uint8_t m_phyId;              //!< the ID of the PHY listening to
};

/****************************************************************
 *      Implement the backoff monitor
 ****************************************************************/

void
BackoffMonitor::Enable(Ptr<WifiMac> mac, AcIndex aci, StatusChangeCb cb)
{
    NS_LOG_FUNCTION(this << mac << aci);
    m_mac = mac;
    NS_ASSERT_MSG(!cb.IsNull(), "Cannot connect a null callback");
    m_enabled = true;
    m_aci = aci;
    Callback<uint32_t, linkId_t> getSlotsCb = [=](linkId_t linkId) {
        auto txop = mac->GetTxopFor(aci);
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
}

void
BackoffMonitor::Disable()
{
    NS_LOG_FUNCTION(this);
    for (const auto& [phyId, listener] : m_listeners)
    {
        m_mac->GetDevice()->GetPhy(phyId)->UnregisterListener(listener);
    }
    m_listeners.clear();
    m_states.clear();
    m_enabled = false;
    m_aci = AC_UNDEF;
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

void
BackoffMonitor::NotifyBackoffGenerated(linkId_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    if (m_enabled)
    {
        auto& state = GetState(linkId);
        state.SetStatus(BackoffStatus::ONGOING);
    }
}

void
BackoffMonitor::NotifyChannelAccessed(linkId_t linkId)
{
    NS_LOG_FUNCTION(this << linkId);

    // set status to ZERO (unless it is already ZERO)
    using enum BackoffStatus;
    if (const auto status = GetBackoffStatus(linkId); status != ZERO && status != UNKNOWN)
    {
        GetState(linkId).SetStatus(BackoffStatus::ZERO);
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
BackoffMonitor::NotifyBackoffUpdated(linkId_t linkId, uint32_t nSlots)
{
    NS_LOG_FUNCTION(this << linkId << nSlots);

    // if the backoff counter has reached zero, set status to ZERO (unless it is already ZERO)
    using enum BackoffStatus;
    if (const auto status = GetBackoffStatus(linkId);
        nSlots == 0 && status != ZERO && status != UNKNOWN)
    {
        GetState(linkId).SetStatus(BackoffStatus::ZERO);
    }
}

void
BackoffMonitor::NotifyMediumBusy(uint8_t phyId, Time duration)
{
    NS_LOG_FUNCTION(this << phyId << duration);

    if (const auto linkId = m_mac->GetLinkForPhy(phyId))
    {
        HandleMediumBusyUpdates(*linkId, Simulator::Now() + duration, std::nullopt);
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

    if (physicalCsEnd.has_value())
    {
        state.physicalCsEnd = Max(state.physicalCsEnd, *physicalCsEnd);
    }
    if (virtualCsEnd.has_value())
    {
        // NAV may be reset
        state.virtualCsEnd = *virtualCsEnd;
    }

    const auto now = Simulator::Now();
    const auto timeout =
        Max(state.physicalCsEnd, state.virtualCsEnd) + GetTxopEndTimeout(linkId) - now;

    using enum BackoffStatus;
    switch (state.backoffStatus)
    {
    case PAUSED:
        // if this busy notification modifies the current medium busy period, reschedule the
        // resume event
        NS_ASSERT_MSG(state.statusChangeEvent.IsPending(),
                      "Resume event should be pending in PAUSE status");
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
