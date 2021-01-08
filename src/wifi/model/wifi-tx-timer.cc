/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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

#include "ns3/log.h"
#include "wifi-tx-timer.h"
#include "wifi-mac-queue-item.h"
#include "wifi-tx-vector.h"
#include "wifi-psdu.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiTxTimer");

WifiTxTimer::WifiTxTimer ()
  : m_reason (NOT_RUNNING),
    m_rescheduled (false)
{
}

WifiTxTimer::~WifiTxTimer ()
{
  m_timeoutEvent.Cancel ();
  m_endRxEvent = 0;
}

void
WifiTxTimer::Reschedule (const Time& delay)
{
  NS_LOG_FUNCTION (this << delay);

  if (m_timeoutEvent.IsRunning () && !m_rescheduled)
    {
      NS_LOG_DEBUG ("Rescheduling " << GetReasonString (m_reason) << " timeout in "
                    << delay.As (Time::US));
      m_timeoutEvent.Cancel ();
      m_timeoutEvent = Simulator::Schedule (delay, m_endRxEvent);
      m_rescheduled = true;
    }
}

WifiTxTimer::Reason
WifiTxTimer::GetReason (void) const
{
  NS_ASSERT (IsRunning ());
  return m_reason;
}

std::string
WifiTxTimer::GetReasonString (Reason reason) const
{
#define FOO(x) \
case WAIT_ ## x: \
  return # x; \
  break;

  switch (reason)
    {
    case NOT_RUNNING:
      return "NOT_RUNNING";
      break;
    FOO (CTS);
    FOO (NORMAL_ACK);
    FOO (BLOCK_ACK);
    default:
      NS_ABORT_MSG ("Unknown reason");
    }
#undef FOO
}

bool
WifiTxTimer::IsRunning (void) const
{
  return m_timeoutEvent.IsRunning ();
}

void
WifiTxTimer::Cancel (void)
{
  NS_LOG_FUNCTION (this << GetReasonString (m_reason));
  m_timeoutEvent.Cancel ();
  m_endRxEvent = 0;
}

Time
WifiTxTimer::GetDelayLeft (void) const
{
  return Simulator::GetDelayLeft (m_timeoutEvent);
}

void
WifiTxTimer::SetMpduResponseTimeoutCallback (MpduResponseTimeout callback) const
{
  m_mpduResponseTimeoutCallback = callback;
}

template<>
void
WifiTxTimer::FeedTraceSource<Ptr<WifiMacQueueItem>, WifiTxVector> (Ptr<WifiMacQueueItem> item,
                                                                   WifiTxVector txVector)
{
  if (!m_mpduResponseTimeoutCallback.IsNull ())
    {
      m_mpduResponseTimeoutCallback (m_reason, item, txVector);
    }
}

void
WifiTxTimer::SetPsduResponseTimeoutCallback (PsduResponseTimeout callback) const
{
  m_psduResponseTimeoutCallback = callback;
}

template<>
void
WifiTxTimer::FeedTraceSource<Ptr<WifiPsdu>, WifiTxVector> (Ptr<WifiPsdu> psdu,
                                                           WifiTxVector txVector)
{
  if (!m_psduResponseTimeoutCallback.IsNull ())
    {
      m_psduResponseTimeoutCallback (m_reason, psdu, txVector);
    }
}

void
WifiTxTimer::SetPsduMapResponseTimeoutCallback (PsduMapResponseTimeout callback) const
{
  m_psduMapResponseTimeoutCallback = callback;
}

template<>
void
WifiTxTimer::FeedTraceSource<WifiPsduMap*, std::set<Mac48Address>*, std::size_t> (WifiPsduMap* psduMap,
                                                                                  std::set<Mac48Address>* missingStations,
                                                                                  std::size_t nTotalStations)
{
  if (!m_psduMapResponseTimeoutCallback.IsNull ())
    {
      m_psduMapResponseTimeoutCallback (m_reason, psduMap, missingStations, nTotalStations);
    }
}

} //namespace ns3
