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

#include "wifi-phy-state-helper.h"

#include "wifi-phy-listener.h"
#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-tx-vector.h"

#include "ns3/log.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <functional>
#include <iterator>

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiPhyStateHelper");

NS_OBJECT_ENSURE_REGISTERED(WifiPhyStateHelper);

TypeId
WifiPhyStateHelper::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiPhyStateHelper")
            .SetParent<Object>()
            .SetGroupName("Wifi")
            .AddConstructor<WifiPhyStateHelper>()
            .AddTraceSource("State",
                            "The state of the PHY layer",
                            MakeTraceSourceAccessor(&WifiPhyStateHelper::m_stateLogger),
                            "ns3::WifiPhyStateHelper::StateTracedCallback")
            .AddTraceSource("RxOk",
                            "A packet has been received successfully.",
                            MakeTraceSourceAccessor(&WifiPhyStateHelper::m_rxOkTrace),
                            "ns3::WifiPhyStateHelper::RxOkTracedCallback")
            .AddTraceSource(
                "RxOutcome",
                "The outcome of the decoding of the PPDU, including MPDU decoding status",
                MakeTraceSourceAccessor(&WifiPhyStateHelper::m_rxOutcomeTrace),
                "ns3::WifiPhyStateHelper::RxOutcomeTracedCallback")
            .AddTraceSource("RxError",
                            "A packet has been received unsuccessfuly.",
                            MakeTraceSourceAccessor(&WifiPhyStateHelper::m_rxErrorTrace),
                            "ns3::WifiPhyStateHelper::RxEndErrorTracedCallback")
            .AddTraceSource("Tx",
                            "Packet transmission is starting.",
                            MakeTraceSourceAccessor(&WifiPhyStateHelper::m_txTrace),
                            "ns3::WifiPhyStateHelper::TxTracedCallback");
    return tid;
}

WifiPhyStateHelper::WifiPhyStateHelper()
    : NS_LOG_TEMPLATE_DEFINE("WifiPhyStateHelper"),
      m_sleeping(false),
      m_isOff(false),
      m_endTx(Time{0}),
      m_endRx(Time{0}),
      m_endCcaBusy(Time{0}),
      m_endSwitching(Time{0}),
      m_endSleep(Time{0}),
      m_endOff(Time{0}),
      m_endIdle(Time{0}),
      m_startTx(Time{0}),
      m_startRx(Time{0}),
      m_startCcaBusy(Time{0}),
      m_startSwitching(Time{0}),
      m_startSleep(Time{0}),
      m_startOff(Time{0}),
      m_previousStateChangeTime(Time{0})
{
    NS_LOG_FUNCTION(this);
}

void
WifiPhyStateHelper::SetReceiveOkCallback(RxOkCallback callback)
{
    m_rxOkCallback = callback;
}

void
WifiPhyStateHelper::SetReceiveErrorCallback(RxErrorCallback callback)
{
    m_rxErrorCallback = callback;
}

void
WifiPhyStateHelper::RegisterListener(const std::shared_ptr<WifiPhyListener>& listener)
{
    m_listeners.emplace_back(listener);
}

void
WifiPhyStateHelper::UnregisterListener(const std::shared_ptr<WifiPhyListener>& listener)
{
    m_listeners.remove_if([&listener](auto&& weakPtr) { return weakPtr.lock() == listener; });
}

bool
WifiPhyStateHelper::IsStateIdle() const
{
    return (GetState() == WifiPhyState::IDLE);
}

bool
WifiPhyStateHelper::IsStateCcaBusy() const
{
    return (GetState() == WifiPhyState::CCA_BUSY);
}

bool
WifiPhyStateHelper::IsStateRx() const
{
    return (GetState() == WifiPhyState::RX);
}

bool
WifiPhyStateHelper::IsStateTx() const
{
    return (GetState() == WifiPhyState::TX);
}

bool
WifiPhyStateHelper::IsStateSwitching() const
{
    return (GetState() == WifiPhyState::SWITCHING);
}

bool
WifiPhyStateHelper::IsStateSleep() const
{
    return (GetState() == WifiPhyState::SLEEP);
}

bool
WifiPhyStateHelper::IsStateOff() const
{
    return (GetState() == WifiPhyState::OFF);
}

Time
WifiPhyStateHelper::GetDelayUntilIdle() const
{
    Time retval;

    switch (GetState())
    {
    case WifiPhyState::RX:
        retval = m_endRx - Simulator::Now();
        break;
    case WifiPhyState::TX:
        retval = m_endTx - Simulator::Now();
        break;
    case WifiPhyState::CCA_BUSY:
        retval = m_endCcaBusy - Simulator::Now();
        break;
    case WifiPhyState::SWITCHING:
        retval = m_endSwitching - Simulator::Now();
        break;
    case WifiPhyState::IDLE:
    case WifiPhyState::SLEEP:
    case WifiPhyState::OFF:
        retval = Seconds(0);
        break;
    default:
        NS_FATAL_ERROR("Invalid WifiPhy state.");
        retval = Seconds(0);
        break;
    }
    retval = Max(retval, Seconds(0));
    return retval;
}

Time
WifiPhyStateHelper::GetLastRxStartTime() const
{
    return m_startRx;
}

Time
WifiPhyStateHelper::GetLastRxEndTime() const
{
    return m_endRx;
}

Time
WifiPhyStateHelper::GetLastTime(std::initializer_list<WifiPhyState> states) const
{
    Time last{0};
    auto currentState = GetState();

    for (auto state : states)
    {
        if (state == currentState)
        {
            return Simulator::Now();
        }

        switch (state)
        {
        case WifiPhyState::RX:
            last = std::max(last, m_endRx);
            break;
        case WifiPhyState::TX:
            last = std::max(last, m_endTx);
            break;
        case WifiPhyState::CCA_BUSY:
            last = std::max(last, m_endCcaBusy);
            break;
        case WifiPhyState::SWITCHING:
            last = std::max(last, m_endSwitching);
            break;
        case WifiPhyState::SLEEP:
            last = std::max(last, m_endSleep);
            break;
        case WifiPhyState::OFF:
            last = std::max(last, m_endOff);
            break;
        case WifiPhyState::IDLE:
            last = std::max(last, m_endIdle);
            break;
        default:
            NS_FATAL_ERROR("Invalid WifiPhy state " << state);
        }
    }
    NS_ASSERT(last <= Simulator::Now());
    return last;
}

WifiPhyState
WifiPhyStateHelper::GetState() const
{
    const auto now = Simulator::Now();
    if (m_isOff)
    {
        return WifiPhyState::OFF;
    }
    if (m_sleeping)
    {
        return WifiPhyState::SLEEP;
    }
    else if (m_endTx > now)
    {
        return WifiPhyState::TX;
    }
    else if (m_endRx > now)
    {
        return WifiPhyState::RX;
    }
    else if (m_endSwitching > now)
    {
        return WifiPhyState::SWITCHING;
    }
    else if (m_endCcaBusy > now)
    {
        return WifiPhyState::CCA_BUSY;
    }
    else
    {
        return WifiPhyState::IDLE;
    }
}

void
WifiPhyStateHelper::LogPreviousIdleAndCcaBusyStates()
{
    NS_LOG_FUNCTION(this);
    const auto now = Simulator::Now();
    WifiPhyState state = GetState();
    if (state == WifiPhyState::CCA_BUSY)
    {
        m_endCcaBusy = now;
        const auto ccaStart =
            std::max({m_endRx, m_endTx, m_startCcaBusy, m_endSwitching, m_endSleep, m_endOff});
        m_stateLogger(ccaStart, now - ccaStart, WifiPhyState::CCA_BUSY);
    }
    else if (state == WifiPhyState::IDLE)
    {
        m_endIdle = now;
        const auto endAllButCcaBusy =
            std::max({m_endRx, m_endTx, m_endSwitching, m_endSleep, m_endOff});
        const auto idleStart = std::max(m_endCcaBusy, endAllButCcaBusy);
        NS_ASSERT(idleStart <= now);
        if (m_endCcaBusy > endAllButCcaBusy)
        {
            const auto ccaBusyStart = std::max(m_startCcaBusy, endAllButCcaBusy);
            if (const auto ccaBusyDuration = idleStart - ccaBusyStart;
                ccaBusyDuration.IsStrictlyPositive())
            {
                m_stateLogger(ccaBusyStart, ccaBusyDuration, WifiPhyState::CCA_BUSY);
            }
        }
        if (const auto idleDuration = now - idleStart; idleDuration.IsStrictlyPositive())
        {
            m_stateLogger(idleStart, idleDuration, WifiPhyState::IDLE);
        }
    }
}

void
WifiPhyStateHelper::SwitchToTx(Time txDuration,
                               WifiConstPsduMap psdus,
                               double txPowerDbm,
                               const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << txDuration << psdus << txPowerDbm << txVector);
    if (!m_txTrace.IsEmpty())
    {
        for (const auto& psdu : psdus)
        {
            m_txTrace(psdu.second->GetPacket(),
                      txVector.GetMode(psdu.first),
                      txVector.GetPreambleType(),
                      txVector.GetTxPowerLevel());
        }
    }
    Time now = Simulator::Now();
    switch (GetState())
    {
    case WifiPhyState::RX:
        /* The packet which is being received as well
         * as its endRx event are cancelled by the caller.
         */
        m_stateLogger(m_startRx, now - m_startRx, WifiPhyState::RX);
        m_endRx = now;
        break;
    case WifiPhyState::CCA_BUSY:
        [[fallthrough]];
    case WifiPhyState::IDLE:
        LogPreviousIdleAndCcaBusyStates();
        break;
    default:
        NS_FATAL_ERROR("Invalid WifiPhy state.");
        break;
    }
    m_stateLogger(now, txDuration, WifiPhyState::TX);
    m_previousStateChangeTime = now;
    m_endTx = now + txDuration;
    m_startTx = now;
    NotifyListeners(&WifiPhyListener::NotifyTxStart, txDuration, txPowerDbm);
}

void
WifiPhyStateHelper::SwitchToRx(Time rxDuration)
{
    NS_LOG_FUNCTION(this << rxDuration);
    NS_ASSERT(IsStateIdle() || IsStateCcaBusy());
    Time now = Simulator::Now();
    switch (GetState())
    {
    case WifiPhyState::IDLE:
        [[fallthrough]];
    case WifiPhyState::CCA_BUSY:
        LogPreviousIdleAndCcaBusyStates();
        break;
    default:
        NS_FATAL_ERROR("Invalid WifiPhy state " << GetState());
        break;
    }
    m_previousStateChangeTime = now;
    m_startRx = now;
    m_endRx = now + rxDuration;
    NotifyListeners(&WifiPhyListener::NotifyRxStart, rxDuration);
    NS_ASSERT(IsStateRx());
}

void
WifiPhyStateHelper::SwitchToChannelSwitching(Time switchingDuration)
{
    NS_LOG_FUNCTION(this << switchingDuration);
    Time now = Simulator::Now();
    switch (GetState())
    {
    case WifiPhyState::RX:
        /* The packet which is being received as well
         * as its endRx event are cancelled by the caller.
         */
        m_stateLogger(m_startRx, now - m_startRx, WifiPhyState::RX);
        m_endRx = now;
        break;
    case WifiPhyState::CCA_BUSY:
        [[fallthrough]];
    case WifiPhyState::IDLE:
        LogPreviousIdleAndCcaBusyStates();
        break;
    default:
        NS_FATAL_ERROR("Invalid WifiPhy state.");
        break;
    }

    m_endCcaBusy = std::min(now, m_endCcaBusy);
    m_stateLogger(now, switchingDuration, WifiPhyState::SWITCHING);
    m_previousStateChangeTime = now;
    m_startSwitching = now;
    m_endSwitching = now + switchingDuration;
    NotifyListeners(&WifiPhyListener::NotifySwitchingStart, switchingDuration);
    NS_ASSERT(switchingDuration.IsZero() || IsStateSwitching());
}

void
WifiPhyStateHelper::NotifyRxMpdu(Ptr<const WifiPsdu> psdu,
                                 RxSignalInfo rxSignalInfo,
                                 const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector);
    if (!m_rxOkCallback.IsNull())
    {
        m_rxOkCallback(psdu, rxSignalInfo, txVector, {});
    }
}

void
WifiPhyStateHelper::NotifyRxPsduSucceeded(Ptr<const WifiPsdu> psdu,
                                          RxSignalInfo rxSignalInfo,
                                          const WifiTxVector& txVector,
                                          uint16_t staId,
                                          const std::vector<bool>& statusPerMpdu)
{
    NS_LOG_FUNCTION(this << *psdu << rxSignalInfo << txVector << staId << statusPerMpdu.size()
                         << std::all_of(statusPerMpdu.begin(), statusPerMpdu.end(), [](bool v) {
                                return v;
                            })); // returns true if all true
    NS_ASSERT(!statusPerMpdu.empty());
    if (!m_rxOkTrace.IsEmpty())
    {
        m_rxOkTrace(psdu->GetPacket(),
                    rxSignalInfo.snr,
                    txVector.GetMode(staId),
                    txVector.GetPreambleType());
    }
    if (!m_rxOkCallback.IsNull())
    {
        m_rxOkCallback(psdu, rxSignalInfo, txVector, statusPerMpdu);
    }
}

void
WifiPhyStateHelper::NotifyRxPsduFailed(Ptr<const WifiPsdu> psdu, double snr)
{
    NS_LOG_FUNCTION(this << *psdu << snr);
    if (!m_rxErrorTrace.IsEmpty())
    {
        m_rxErrorTrace(psdu->GetPacket(), snr);
    }
    if (!m_rxErrorCallback.IsNull())
    {
        m_rxErrorCallback(psdu);
    }
}

void
WifiPhyStateHelper::NotifyRxPpduOutcome(Ptr<const WifiPpdu> ppdu,
                                        RxSignalInfo rxSignalInfo,
                                        const WifiTxVector& txVector,
                                        uint16_t staId,
                                        const std::vector<bool>& statusPerMpdu)
{
    m_rxOutcomeTrace(ppdu, rxSignalInfo, txVector, statusPerMpdu);
}

void
WifiPhyStateHelper::SwitchFromRxEndOk()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_endRx == Simulator::Now());
    NotifyListeners(&WifiPhyListener::NotifyRxEndOk);
    DoSwitchFromRx();
}

void
WifiPhyStateHelper::SwitchFromRxEndError()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_endRx == Simulator::Now());
    NotifyListeners(&WifiPhyListener::NotifyRxEndError);
    DoSwitchFromRx();
}

void
WifiPhyStateHelper::DoSwitchFromRx()
{
    NS_LOG_FUNCTION(this);
    Time now = Simulator::Now();
    m_stateLogger(m_startRx, now - m_startRx, WifiPhyState::RX);
    m_previousStateChangeTime = now;
    m_endRx = Simulator::Now();
    NS_ASSERT(IsStateIdle() || IsStateCcaBusy());
}

void
WifiPhyStateHelper::SwitchMaybeToCcaBusy(Time duration,
                                         WifiChannelListType channelType,
                                         const std::vector<Time>& per20MhzDurations)
{
    NS_LOG_FUNCTION(this << duration << channelType);
    if (GetState() == WifiPhyState::RX)
    {
        return;
    }
    NotifyListeners(&WifiPhyListener::NotifyCcaBusyStart, duration, channelType, per20MhzDurations);
    if (channelType != WIFI_CHANLIST_PRIMARY)
    {
        // WifiPhyStateHelper only updates CCA start and end durations for the primary channel
        return;
    }
    Time now = Simulator::Now();
    if (GetState() == WifiPhyState::IDLE)
    {
        LogPreviousIdleAndCcaBusyStates();
    }
    if (GetState() != WifiPhyState::CCA_BUSY)
    {
        m_startCcaBusy = now;
    }
    m_endCcaBusy = std::max(m_endCcaBusy, now + duration);
}

void
WifiPhyStateHelper::SwitchToSleep()
{
    NS_LOG_FUNCTION(this);
    Time now = Simulator::Now();
    switch (GetState())
    {
    case WifiPhyState::IDLE:
        [[fallthrough]];
    case WifiPhyState::CCA_BUSY:
        LogPreviousIdleAndCcaBusyStates();
        break;
    default:
        NS_FATAL_ERROR("Invalid WifiPhy state.");
        break;
    }
    m_previousStateChangeTime = now;
    m_sleeping = true;
    m_startSleep = now;
    NotifyListeners(&WifiPhyListener::NotifySleep);
    NS_ASSERT(IsStateSleep());
}

void
WifiPhyStateHelper::SwitchFromSleep()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsStateSleep());
    Time now = Simulator::Now();
    m_stateLogger(m_startSleep, now - m_startSleep, WifiPhyState::SLEEP);
    m_previousStateChangeTime = now;
    m_sleeping = false;
    m_endSleep = now;
    NotifyListeners(&WifiPhyListener::NotifyWakeup);
}

void
WifiPhyStateHelper::SwitchFromRxAbort(uint16_t operatingWidth)
{
    NS_LOG_FUNCTION(this << operatingWidth);
    NS_ASSERT(IsStateCcaBusy()); // abort is called (with OBSS_PD_CCA_RESET reason) before RX is set
                                 // by payload start
    NotifyListeners(&WifiPhyListener::NotifyRxEndOk);
    DoSwitchFromRx();
    m_endCcaBusy = Simulator::Now();
    std::vector<Time> per20MhzDurations;
    if (operatingWidth >= 40)
    {
        std::fill_n(std::back_inserter(per20MhzDurations), (operatingWidth / 20), Seconds(0));
    }
    NotifyListeners(&WifiPhyListener::NotifyCcaBusyStart,
                    Seconds(0),
                    WIFI_CHANLIST_PRIMARY,
                    per20MhzDurations);
    NS_ASSERT(IsStateIdle());
}

void
WifiPhyStateHelper::SwitchToOff()
{
    NS_LOG_FUNCTION(this);
    Time now = Simulator::Now();
    switch (GetState())
    {
    case WifiPhyState::RX:
        /* The packet which is being received as well
         * as its endRx event are cancelled by the caller.
         */
        m_stateLogger(m_startRx, now - m_startRx, WifiPhyState::RX);
        m_endRx = now;
        break;
    case WifiPhyState::TX:
        /* The packet which is being transmitted as well
         * as its endTx event are cancelled by the caller.
         */
        m_stateLogger(m_startTx, now - m_startTx, WifiPhyState::TX);
        m_endTx = now;
        break;
    case WifiPhyState::IDLE:
        [[fallthrough]];
    case WifiPhyState::CCA_BUSY:
        LogPreviousIdleAndCcaBusyStates();
        break;
    default:
        NS_FATAL_ERROR("Invalid WifiPhy state.");
        break;
    }
    m_previousStateChangeTime = now;
    m_isOff = true;
    m_startOff = now;
    NotifyListeners(&WifiPhyListener::NotifyOff);
    NS_ASSERT(IsStateOff());
}

void
WifiPhyStateHelper::SwitchFromOff()
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(IsStateOff());
    Time now = Simulator::Now();
    m_previousStateChangeTime = now;
    m_isOff = false;
    m_endOff = now;
    NotifyListeners(&WifiPhyListener::NotifyOn);
}

} // namespace ns3
