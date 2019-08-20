/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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

#include <algorithm>
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "wifi-phy-state-helper.h"
#include "wifi-tx-vector.h"
#include "wifi-phy-listener.h"
#include "wifi-psdu.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPhyStateHelper");

NS_OBJECT_ENSURE_REGISTERED (WifiPhyStateHelper);

TypeId
WifiPhyStateHelper::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhyStateHelper")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiPhyStateHelper> ()
    .AddTraceSource ("State",
                     "The state of the PHY layer",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_stateLogger),
                     "ns3::WifiPhyStateHelper::StateTracedCallback")
    .AddTraceSource ("RxOk",
                     "A packet has been received successfully.",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_rxOkTrace),
                     "ns3::WifiPhyStateHelper::RxOkTracedCallback")
    .AddTraceSource ("RxError",
                     "A packet has been received unsuccessfully.",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_rxErrorTrace),
                     "ns3::WifiPhyStateHelper::RxEndErrorTracedCallback")
    .AddTraceSource ("Tx", "Packet transmission is starting.",
                     MakeTraceSourceAccessor (&WifiPhyStateHelper::m_txTrace),
                     "ns3::WifiPhyStateHelper::TxTracedCallback")
  ;
  return tid;
}

WifiPhyStateHelper::WifiPhyStateHelper ()
  : m_sleeping (false),
    m_isOff (false),
    m_endTx (Seconds (0)),
    m_endRx (Seconds (0)),
    m_endCcaBusy (Seconds (0)),
    m_endSwitching (Seconds (0)),
    m_startTx (Seconds (0)),
    m_startRx (Seconds (0)),
    m_startCcaBusy (Seconds (0)),
    m_startSwitching (Seconds (0)),
    m_startSleep (Seconds (0)),
    m_previousStateChangeTime (Seconds (0))
{
  NS_LOG_FUNCTION (this);
}

void
WifiPhyStateHelper::SetReceiveOkCallback (RxOkCallback callback)
{
  m_rxOkCallback = callback;
}

void
WifiPhyStateHelper::SetReceiveErrorCallback (RxErrorCallback callback)
{
  m_rxErrorCallback = callback;
}

void
WifiPhyStateHelper::RegisterListener (WifiPhyListener *listener)
{
  m_listeners.push_back (listener);
}

void
WifiPhyStateHelper::UnregisterListener (WifiPhyListener *listener)
{
  ListenersI i = find (m_listeners.begin (), m_listeners.end (), listener);
  if (i != m_listeners.end ())
    {
      m_listeners.erase (i);
    }
}

bool
WifiPhyStateHelper::IsStateIdle (void) const
{
  return (GetState () == WifiPhyState::IDLE);
}

bool
WifiPhyStateHelper::IsStateCcaBusy (void) const
{
  return (GetState () == WifiPhyState::CCA_BUSY);
}

bool
WifiPhyStateHelper::IsStateRx (void) const
{
  return (GetState () == WifiPhyState::RX);
}

bool
WifiPhyStateHelper::IsStateTx (void) const
{
  return (GetState () == WifiPhyState::TX);
}

bool
WifiPhyStateHelper::IsStateSwitching (void) const
{
  return (GetState () == WifiPhyState::SWITCHING);
}

bool
WifiPhyStateHelper::IsStateSleep (void) const
{
  return (GetState () == WifiPhyState::SLEEP);
}

bool
WifiPhyStateHelper::IsStateOff (void) const
{
  return (GetState () == WifiPhyState::OFF);
}

Time
WifiPhyStateHelper::GetDelayUntilIdle (void) const
{
  Time retval;

  switch (GetState ())
    {
    case WifiPhyState::RX:
      retval = m_endRx - Simulator::Now ();
      break;
    case WifiPhyState::TX:
      retval = m_endTx - Simulator::Now ();
      break;
    case WifiPhyState::CCA_BUSY:
      retval = m_endCcaBusy - Simulator::Now ();
      break;
    case WifiPhyState::SWITCHING:
      retval = m_endSwitching - Simulator::Now ();
      break;
    case WifiPhyState::IDLE:
    case WifiPhyState::SLEEP:
    case WifiPhyState::OFF:
      retval = Seconds (0);
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      retval = Seconds (0);
      break;
    }
  retval = Max (retval, Seconds (0));
  return retval;
}

Time
WifiPhyStateHelper::GetLastRxStartTime (void) const
{
  return m_startRx;
}

Time
WifiPhyStateHelper::GetLastRxEndTime (void) const
{
  return m_endRx;
}

WifiPhyState
WifiPhyStateHelper::GetState (void) const
{
  if (m_isOff)
    {
      return WifiPhyState::OFF;
    }
  if (m_sleeping)
    {
      return WifiPhyState::SLEEP;
    }
  else if (m_endTx > Simulator::Now ())
    {
      return WifiPhyState::TX;
    }
  else if (m_endRx > Simulator::Now ())
    {
      return WifiPhyState::RX;
    }
  else if (m_endSwitching > Simulator::Now ())
    {
      return WifiPhyState::SWITCHING;
    }
  else if (m_endCcaBusy > Simulator::Now ())
    {
      return WifiPhyState::CCA_BUSY;
    }
  else
    {
      return WifiPhyState::IDLE;
    }
}

void
WifiPhyStateHelper::NotifyTxStart (Time duration, double txPowerDbm)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyTxStart (duration, txPowerDbm);
    }
}

void
WifiPhyStateHelper::NotifyRxStart (Time duration)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxStart (duration);
    }
}

void
WifiPhyStateHelper::NotifyRxEndOk (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxEndOk ();
    }
}

void
WifiPhyStateHelper::NotifyRxEndError (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyRxEndError ();
    }
}

void
WifiPhyStateHelper::NotifyMaybeCcaBusyStart (Time duration)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyMaybeCcaBusyStart (duration);
    }
}

void
WifiPhyStateHelper::NotifySwitchingStart (Time duration)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifySwitchingStart (duration);
    }
}

void
WifiPhyStateHelper::NotifySleep (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifySleep ();
    }
}

void
WifiPhyStateHelper::NotifyOff (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyOff ();
    }
}

void
WifiPhyStateHelper::NotifyWakeup (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyWakeup ();
    }
}

void
WifiPhyStateHelper::NotifyOn (void)
{
  NS_LOG_FUNCTION (this);
  for (Listeners::const_iterator i = m_listeners.begin (); i != m_listeners.end (); i++)
    {
      (*i)->NotifyOn ();
    }
}

void
WifiPhyStateHelper::LogPreviousIdleAndCcaBusyStates (void)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  Time idleStart = Max (m_endCcaBusy, m_endRx);
  idleStart = Max (idleStart, m_endTx);
  idleStart = Max (idleStart, m_endSwitching);
  NS_ASSERT (idleStart <= now);
  if (m_endCcaBusy > m_endRx
      && m_endCcaBusy > m_endSwitching
      && m_endCcaBusy > m_endTx)
    {
      Time ccaBusyStart = Max (m_endTx, m_endRx);
      ccaBusyStart = Max (ccaBusyStart, m_startCcaBusy);
      ccaBusyStart = Max (ccaBusyStart, m_endSwitching);
      Time ccaBusyDuration = idleStart - ccaBusyStart;
      if (ccaBusyDuration.IsStrictlyPositive ())
        {
          m_stateLogger (ccaBusyStart, ccaBusyDuration, WifiPhyState::CCA_BUSY);
        }
    }
  Time idleDuration = now - idleStart;
  if (idleDuration.IsStrictlyPositive ())
    {
      m_stateLogger (idleStart, idleDuration, WifiPhyState::IDLE);
    }
}

void
WifiPhyStateHelper::SwitchToTx (Time txDuration, Ptr<const Packet> packet, double txPowerDbm,
                                WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << txDuration << packet << txPowerDbm << txVector);
  m_txTrace (packet, txVector.GetMode (), txVector.GetPreambleType (), txVector.GetTxPowerLevel ());
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhyState::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_stateLogger (m_startRx, now - m_startRx, WifiPhyState::RX);
      m_endRx = now;
      break;
    case WifiPhyState::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhyState::CCA_BUSY);
      } break;
    case WifiPhyState::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_stateLogger (now, txDuration, WifiPhyState::TX);
  m_previousStateChangeTime = now;
  m_endTx = now + txDuration;
  m_startTx = now;
  NotifyTxStart (txDuration, txPowerDbm);
}

void
WifiPhyStateHelper::SwitchToRx (Time rxDuration)
{
  NS_LOG_FUNCTION (this << rxDuration);
  NS_ASSERT (IsStateIdle () || IsStateCcaBusy ());
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhyState::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhyState::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhyState::CCA_BUSY);
      } break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state " << GetState ());
      break;
    }
  m_previousStateChangeTime = now;
  m_startRx = now;
  m_endRx = now + rxDuration;
  NotifyRxStart (rxDuration);
  NS_ASSERT (IsStateRx ());
}

void
WifiPhyStateHelper::SwitchToChannelSwitching (Time switchingDuration)
{
  NS_LOG_FUNCTION (this << switchingDuration);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhyState::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_stateLogger (m_startRx, now - m_startRx, WifiPhyState::RX);
      m_endRx = now;
      break;
    case WifiPhyState::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhyState::CCA_BUSY);
      } break;
    case WifiPhyState::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }

  if (now < m_endCcaBusy)
    {
      m_endCcaBusy = now;
    }

  m_stateLogger (now, switchingDuration, WifiPhyState::SWITCHING);
  m_previousStateChangeTime = now;
  m_startSwitching = now;
  m_endSwitching = now + switchingDuration;
  NotifySwitchingStart (switchingDuration);
  NS_ASSERT (IsStateSwitching ());
}

void
WifiPhyStateHelper::ContinueRxNextMpdu (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector);
  std::vector<bool> statusPerMpdu;
  if (!m_rxOkCallback.IsNull ())
    {
      m_rxOkCallback (psdu, snr, txVector, statusPerMpdu);
    }
}

void
WifiPhyStateHelper::SwitchFromRxEndOk (Ptr<WifiPsdu> psdu, double snr, WifiTxVector txVector, std::vector<bool> statusPerMpdu)
{
  NS_LOG_FUNCTION (this << *psdu << snr << txVector << statusPerMpdu.size () <<
                   std::all_of(statusPerMpdu.begin(), statusPerMpdu.end(), [](bool v) { return v; })); //returns true if all true
  NS_ASSERT (statusPerMpdu.size () != 0);
  NS_ASSERT (m_endRx == Simulator::Now ());
  m_rxOkTrace (psdu->GetPacket (), snr, txVector.GetMode (), txVector.GetPreambleType ());
  NotifyRxEndOk ();
  DoSwitchFromRx ();
  if (!m_rxOkCallback.IsNull ())
    {
      m_rxOkCallback (psdu, snr, txVector, statusPerMpdu);
    }
}

void
WifiPhyStateHelper::SwitchFromRxEndError (Ptr<WifiPsdu> psdu, double snr)
{
  NS_LOG_FUNCTION (this << *psdu << snr);
  NS_ASSERT (m_endRx == Simulator::Now ());
  m_rxErrorTrace (psdu->GetPacket (), snr);
  NotifyRxEndError ();
  DoSwitchFromRx ();
  if (!m_rxErrorCallback.IsNull ())
    {
      m_rxErrorCallback (psdu);
    }
}

void
WifiPhyStateHelper::DoSwitchFromRx (void)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  m_stateLogger (m_startRx, now - m_startRx, WifiPhyState::RX);
  m_previousStateChangeTime = now;
  m_endRx = Simulator::Now ();
  NS_ASSERT (IsStateIdle () || IsStateCcaBusy ());
}

void
WifiPhyStateHelper::SwitchMaybeToCcaBusy (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  if (GetState () != WifiPhyState::RX)
    {
      NotifyMaybeCcaBusyStart (duration);
    }
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhyState::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhyState::RX:
      return;
    default:
      break;
    }
  if (GetState () != WifiPhyState::CCA_BUSY)
    {
      m_startCcaBusy = now;
    }
  m_endCcaBusy = std::max (m_endCcaBusy, now + duration);
}

void
WifiPhyStateHelper::SwitchToSleep (void)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhyState::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhyState::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhyState::CCA_BUSY);
      } break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_previousStateChangeTime = now;
  m_sleeping = true;
  m_startSleep = now;
  NotifySleep ();
  NS_ASSERT (IsStateSleep ());
}

void
WifiPhyStateHelper::SwitchFromSleep (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  NS_ASSERT (IsStateSleep ());
  Time now = Simulator::Now ();
  m_stateLogger (m_startSleep, now - m_startSleep, WifiPhyState::SLEEP);
  m_previousStateChangeTime = now;
  m_sleeping = false;
  NotifyWakeup ();
  //update m_endCcaBusy after the sleep period
  m_endCcaBusy = std::max (m_endCcaBusy, now + duration);
  if (m_endCcaBusy > now)
    {
      NotifyMaybeCcaBusyStart (m_endCcaBusy - now);
    }
}

void
WifiPhyStateHelper::SwitchFromRxAbort (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (IsStateRx ());
  NotifyRxEndOk ();
  DoSwitchFromRx ();
  m_endCcaBusy = Simulator::Now ();
  NotifyMaybeCcaBusyStart (Seconds (0));
  NS_ASSERT (IsStateIdle ());
}

void
WifiPhyStateHelper::SwitchToOff (void)
{
  NS_LOG_FUNCTION (this);
  Time now = Simulator::Now ();
  switch (GetState ())
    {
    case WifiPhyState::RX:
      /* The packet which is being received as well
       * as its endRx event are cancelled by the caller.
       */
      m_stateLogger (m_startRx, now - m_startRx, WifiPhyState::RX);
      m_endRx = now;
      break;
    case WifiPhyState::TX:
      /* The packet which is being transmitted as well
       * as its endTx event are cancelled by the caller.
       */
      m_stateLogger (m_startTx, now - m_startTx, WifiPhyState::TX);
      m_endTx = now;
      break;
    case WifiPhyState::IDLE:
      LogPreviousIdleAndCcaBusyStates ();
      break;
    case WifiPhyState::CCA_BUSY:
      {
        Time ccaStart = Max (m_endRx, m_endTx);
        ccaStart = Max (ccaStart, m_startCcaBusy);
        ccaStart = Max (ccaStart, m_endSwitching);
        m_stateLogger (ccaStart, now - ccaStart, WifiPhyState::CCA_BUSY);
      } break;
    default:
      NS_FATAL_ERROR ("Invalid WifiPhy state.");
      break;
    }
  m_previousStateChangeTime = now;
  m_isOff = true;
  NotifyOff ();
  NS_ASSERT (IsStateOff ());
}

void
WifiPhyStateHelper::SwitchFromOff (Time duration)
{
  NS_LOG_FUNCTION (this << duration);
  NS_ASSERT (IsStateOff ());
  Time now = Simulator::Now ();
  m_previousStateChangeTime = now;
  m_isOff = false;
  NotifyOn ();
  //update m_endCcaBusy after the off period
  m_endCcaBusy = std::max (m_endCcaBusy, now + duration);
  if (m_endCcaBusy > now)
    {
      NotifyMaybeCcaBusyStart (m_endCcaBusy - now);
    }
}

} //namespace ns3
