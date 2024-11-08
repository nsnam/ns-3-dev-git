/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-tx-timer.h"

#include "wifi-mpdu.h"
#include "wifi-psdu.h"
#include "wifi-tx-vector.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiTxTimer");

WifiTxTimer::WifiTxTimer()
    : m_timeoutEvent(),
      m_reason(NOT_RUNNING),
      m_impl(nullptr),
      m_end()
{
}

WifiTxTimer::~WifiTxTimer()
{
    m_timeoutEvent.Cancel();
    m_impl = nullptr;
}

void
WifiTxTimer::Reschedule(const Time& delay)
{
    NS_LOG_FUNCTION(this << delay);

    if (m_timeoutEvent.IsPending())
    {
        NS_LOG_DEBUG("Rescheduling " << GetReasonString(m_reason) << " timeout in "
                                     << delay.As(Time::US));
        Time end = Simulator::Now() + delay;
        // If timer expiration is postponed, we have to do nothing but updating
        // the timer expiration, because Expire() will reschedule itself to be
        // executed at the correct time. If timer expiration is moved up, we
        // have to reschedule Expire() (which would be executed too late otherwise)
        if (end < m_end)
        {
            // timer expiration is moved up
            m_timeoutEvent.Cancel();
            m_timeoutEvent = Simulator::Schedule(delay, &WifiTxTimer::Expire, this);
        }
        m_end = end;
    }
}

void
WifiTxTimer::Expire()
{
    NS_LOG_FUNCTION(this);
    Time now = Simulator::Now();

    if (m_end == now)
    {
        m_impl->Invoke();
    }
    else
    {
        m_timeoutEvent = Simulator::Schedule(m_end - now, &WifiTxTimer::Expire, this);
    }
}

WifiTxTimer::Reason
WifiTxTimer::GetReason() const
{
    NS_ASSERT(IsRunning());
    return m_reason;
}

std::string
WifiTxTimer::GetReasonString(Reason reason) const
{
#define CASE_REASON(x)                                                                             \
    case WAIT_##x:                                                                                 \
        return #x;

    switch (reason)
    {
    case NOT_RUNNING:
        return "NOT_RUNNING";
        CASE_REASON(CTS);
        CASE_REASON(NORMAL_ACK);
        CASE_REASON(BLOCK_ACK);
        CASE_REASON(CTS_AFTER_MU_RTS);
        CASE_REASON(NORMAL_ACK_AFTER_DL_MU_PPDU);
        CASE_REASON(BLOCK_ACKS_IN_TB_PPDU);
        CASE_REASON(TB_PPDU_AFTER_BASIC_TF);
        CASE_REASON(QOS_NULL_AFTER_BSRP_TF);
        CASE_REASON(BLOCK_ACK_AFTER_TB_PPDU);
    default:
        NS_ABORT_MSG("Unknown reason");
    }
#undef CASE_REASON
}

bool
WifiTxTimer::IsRunning() const
{
    return m_timeoutEvent.IsPending();
}

void
WifiTxTimer::Cancel()
{
    NS_LOG_FUNCTION(this << GetReasonString(m_reason));
    m_timeoutEvent.Cancel();
    m_impl = nullptr;
    m_staExpectResponseFrom.clear();
}

void
WifiTxTimer::GotResponseFrom(const Mac48Address& from)
{
    m_staExpectResponseFrom.erase(from);
}

const std::set<Mac48Address>&
WifiTxTimer::GetStasExpectedToRespond() const
{
    return m_staExpectResponseFrom;
}

Time
WifiTxTimer::GetDelayLeft() const
{
    return m_end - Simulator::Now();
}

void
WifiTxTimer::SetMpduResponseTimeoutCallback(MpduResponseTimeout callback) const
{
    m_mpduResponseTimeoutCallback = callback;
}

void
WifiTxTimer::FeedTraceSource(Ptr<WifiMpdu> item, WifiTxVector txVector)
{
    if (!m_mpduResponseTimeoutCallback.IsNull())
    {
        m_mpduResponseTimeoutCallback(m_reason, item, txVector);
    }
}

void
WifiTxTimer::SetPsduResponseTimeoutCallback(PsduResponseTimeout callback) const
{
    m_psduResponseTimeoutCallback = callback;
}

void
WifiTxTimer::FeedTraceSource(Ptr<WifiPsdu> psdu, WifiTxVector txVector)
{
    if (!m_psduResponseTimeoutCallback.IsNull())
    {
        m_psduResponseTimeoutCallback(m_reason, psdu, txVector);
    }
}

void
WifiTxTimer::SetPsduMapResponseTimeoutCallback(PsduMapResponseTimeout callback) const
{
    m_psduMapResponseTimeoutCallback = callback;
}

void
WifiTxTimer::FeedTraceSource(WifiPsduMap* psduMap, std::size_t nTotalStations)
{
    if (!m_psduMapResponseTimeoutCallback.IsNull())
    {
        m_psduMapResponseTimeoutCallback(m_reason,
                                         psduMap,
                                         &m_staExpectResponseFrom,
                                         nTotalStations);
    }
}

} // namespace ns3
