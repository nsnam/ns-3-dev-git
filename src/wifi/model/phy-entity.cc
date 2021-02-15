/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Orange Labs
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
 * Authors: Rediet <getachew.redieteab@orange.com>
 *          Sébastien Deronne <sebastien.deronne@gmail.com> (for logic ported from wifi-phy)
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr> (for logic ported from wifi-phy)
 */

#include "phy-entity.h"
#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "preamble-detection-model.h"
#include "wifi-utils.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/assert.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PhyEntity");

std::ostream & operator << (std::ostream &os, const PhyEntity::PhyRxFailureAction &action)
{
  switch (action)
    {
      case PhyEntity::DROP:
        return (os << "DROP");
      case PhyEntity::ABORT:
        return (os << "ABORT");
      case PhyEntity::IGNORE:
        return (os << "IGNORE");
      default:
        NS_FATAL_ERROR ("Unknown action");
        return (os << "unknown");
    }
}

std::ostream & operator << (std::ostream &os, const PhyEntity::PhyFieldRxStatus &status)
{
  if (status.isSuccess)
    {
      return os << "success";
    }
  else
    {
      return os << "failure (" << status.reason << "/" << status.actionIfFailure << ")";
    }
}

/*******************************************************
 *       Abstract base class for PHY entities
 *******************************************************/

PhyEntity::~PhyEntity ()
{
  m_modeList.clear ();
}

void
PhyEntity::SetOwner (Ptr<WifiPhy> wifiPhy)
{
  NS_LOG_FUNCTION (this << wifiPhy);
  m_wifiPhy = wifiPhy;
  m_state = m_wifiPhy->m_state;
}

bool
PhyEntity::IsModeSupported (WifiMode mode) const
{
  for (const auto & m : m_modeList)
    {
      if (m == mode)
        {
          return true;
        }
    }
  return false;
}

uint8_t
PhyEntity::GetNumModes (void) const
{
  return m_modeList.size ();
}

WifiMode
PhyEntity::GetMcs (uint8_t /* index */) const
{
  NS_ABORT_MSG ("This method should be used only for HtPhy and child classes. Use GetMode instead.");
  return WifiMode ();
}

bool
PhyEntity::IsMcsSupported (uint8_t /* index */) const
{
  NS_ABORT_MSG ("This method should be used only for HtPhy and child classes. Use IsModeSupported instead.");
  return false;
}

bool
PhyEntity::HandlesMcsModes (void) const
{
  return false;
}

std::list<WifiMode>::const_iterator
PhyEntity::begin (void) const
{
  return m_modeList.begin ();
}

std::list<WifiMode>::const_iterator
PhyEntity::end (void) const
{
  return m_modeList.end ();
}

WifiMode
PhyEntity::GetSigMode (WifiPpduField field, WifiTxVector txVector) const
{
  NS_FATAL_ERROR ("PPDU field is not a SIG field (no sense in retrieving the signaled mode) or is unsupported: " << field);
  return WifiMode (); //should be overloaded
}

WifiPpduField
PhyEntity::GetNextField (WifiPpduField currentField, WifiPreamble preamble) const
{
  auto ppduFormats = GetPpduFormats ();
  const auto itPpdu = ppduFormats.find (preamble);
  if (itPpdu != ppduFormats.end ())
    {
      const auto itField = std::find (itPpdu->second.begin (), itPpdu->second.end (), currentField);
      if (itField != itPpdu->second.end ())
        {
          const auto itNextField = std::next (itField, 1);
          if (itNextField != itPpdu->second.end ())
            {
              return *(itNextField);
            }
          NS_FATAL_ERROR ("No field after " << currentField << " for " << preamble << " for the provided PPDU formats");
        }
      else
        {
          NS_FATAL_ERROR ("Unsupported PPDU field " << currentField << " for " << preamble << " for the provided PPDU formats");
        }
    }
  else
    {
      NS_FATAL_ERROR ("Unsupported preamble " << preamble << " for the provided PPDU formats");
    }
}

Time
PhyEntity::GetDuration (WifiPpduField field, WifiTxVector txVector) const
{
  if (field > WIFI_PPDU_FIELD_SIG_B)
    {
      NS_FATAL_ERROR ("Unsupported PPDU field");
    }
  return MicroSeconds (0); //should be overloaded
}

Time
PhyEntity::CalculatePhyPreambleAndHeaderDuration (WifiTxVector txVector) const
{
  Time duration = MicroSeconds (0);
  for (uint8_t field = WIFI_PPDU_FIELD_PREAMBLE; field < WIFI_PPDU_FIELD_DATA; ++field)
    {
      duration += GetDuration (static_cast<WifiPpduField> (field), txVector);
    }
  return duration;
}

Ptr<const WifiPsdu>
PhyEntity::GetAddressedPsduInPpdu (Ptr<const WifiPpdu> ppdu) const
{
  return ppdu->GetPsdu ();
}

PhyEntity::PhyHeaderSections
PhyEntity::GetPhyHeaderSections (WifiTxVector txVector, Time ppduStart) const
{
  PhyHeaderSections map;
  WifiPpduField field = WIFI_PPDU_FIELD_PREAMBLE; //preamble always present
  Time start = ppduStart;

  while (field != WIFI_PPDU_FIELD_DATA)
    {
      Time duration = GetDuration (field, txVector);
      map[field] = std::make_pair (std::make_pair (start, start + duration),
                                   GetSigMode (field, txVector));
      //Move to next field
      start += duration;
      field = GetNextField (field, txVector.GetPreambleType ());
    }
  return map;
}

Ptr<WifiPpdu>
PhyEntity::BuildPpdu (const WifiConstPsduMap & psdus, WifiTxVector txVector,
                      Time /* ppduDuration */, WifiPhyBand /* band */, uint64_t /* uid */) const
{
  NS_FATAL_ERROR ("This method is unsupported for the base PhyEntity class. Use the overloaded version in the amendment-specific subclasses instead!");
  return Create<WifiPpdu> (psdus.begin ()->second, txVector); //should be overloaded
}

Time
PhyEntity::GetDurationUpToField (WifiPpduField field, WifiTxVector txVector) const
{
  if (field == WIFI_PPDU_FIELD_DATA) //this field is not in the map returned by GetPhyHeaderSections
    {
      return CalculatePhyPreambleAndHeaderDuration (txVector);
    }
  const auto & sections = GetPhyHeaderSections (txVector, NanoSeconds (0));
  auto it = sections.find (field);
  NS_ASSERT (it != sections.end ());
  const auto & startStopTimes = it->second.first;
  return startStopTimes.first; //return the start time of field relatively to the beginning of the PPDU
}

PhyEntity::SnrPer
PhyEntity::GetPhyHeaderSnrPer (WifiPpduField field, Ptr<Event> event) const
{
  uint16_t measurementChannelWidth = m_wifiPhy->GetMeasurementChannelWidth (event->GetPpdu ());
  return m_wifiPhy->m_interference.CalculatePhyHeaderSnrPer (event, measurementChannelWidth, m_wifiPhy->GetBand (measurementChannelWidth),
                                                             field);
}

void
PhyEntity::StartReceiveField (WifiPpduField field, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << field << *event);
  NS_ASSERT (m_wifiPhy); //no sense if no owner WifiPhy instance
  NS_ASSERT (m_wifiPhy->m_endPhyRxEvent.IsExpired ()
             || m_wifiPhy->m_currentPreambleEvents.size () > 1); //TODO find a better way of handling multiple preambles until synching on one after detection period

  //Handle special cases of preamble and data reception (TODO improve this logic later on)
  if (field == WIFI_PPDU_FIELD_PREAMBLE)
    {
      DoStartReceivePreamble (event);
      return;
    }
  if (field == WIFI_PPDU_FIELD_DATA)
    {
      //TODO improve this logic later on
      //Hand over to WifiPhy for data processing
      m_wifiPhy->StartReceivePayload (event);
      return;
    }

  bool supported = DoStartReceiveField (field, event);
  NS_ABORT_MSG_IF (!supported, "Unknown field " << field << " for this PHY entity"); //TODO see what to do if not supported
  Time duration = GetDuration (field, event->GetTxVector ());
  m_wifiPhy->m_endPhyRxEvent = Simulator::Schedule (duration, &PhyEntity::EndReceiveField, this, field, event);
  m_state->SwitchMaybeToCcaBusy (duration); //keep in CCA busy state up to reception of Data (will then switch to RX)
}

void
PhyEntity::EndReceiveField (WifiPpduField field, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << field << *event);
  NS_ASSERT (m_wifiPhy); //no sense if no owner WifiPhy instance
  NS_ASSERT (m_wifiPhy->m_endPhyRxEvent.IsExpired ());
  PhyFieldRxStatus status = DoEndReceiveField (field, event);
  WifiTxVector txVector = event->GetTxVector ();
  if (status.isSuccess) //move to next field if reception succeeded
    {
      StartReceiveField (GetNextField (field, txVector.GetPreambleType ()), event);
    }
  else
    {
      Ptr<const WifiPpdu> ppdu = event->GetPpdu ();
      switch (status.actionIfFailure)
        {
          case ABORT:
            //Abort reception, but consider medium as busy
            if (status.reason == FILTERED)
              {
                //PHY-RXSTART is immediately followed by PHY-RXEND (Filtered)
                m_wifiPhy->m_phyRxPayloadBeginTrace (txVector, NanoSeconds (0)); //this callback (equivalent to PHY-RXSTART primitive) is also triggered for filtered PPDUs
              }
            AbortCurrentReception (status.reason);
            if (event->GetEndTime () > (Simulator::Now () + m_state->GetDelayUntilIdle ()))
              {
                m_wifiPhy->MaybeCcaBusyDuration (m_wifiPhy->GetMeasurementChannelWidth (ppdu));
              }
            break;
          case DROP:
            //Notify drop, keep in CCA busy, and perform same processing as IGNORE case
            m_wifiPhy->NotifyRxDrop (GetAddressedPsduInPpdu (ppdu), status.reason);
            m_state->SwitchMaybeToCcaBusy (GetRemainingDurationAfterField (ppdu, field)); //keep in CCA busy state till the end
          case IGNORE:
            //Keep in Rx state and reset at end
            m_wifiPhy->m_endRxEvents.push_back (Simulator::Schedule (GetRemainingDurationAfterField (ppdu, field),
                                                                     &PhyEntity::ResetReceive, this, event));
            break;
          default:
            NS_FATAL_ERROR ("Unknown action in case of failure");
        }
    }
}

Time
PhyEntity::GetRemainingDurationAfterField (Ptr<const WifiPpdu> ppdu, WifiPpduField field) const
{
  WifiTxVector txVector = ppdu->GetTxVector ();
  return ppdu->GetTxDuration () - (GetDurationUpToField (field, txVector) + GetDuration (field, txVector));
}

bool
PhyEntity::DoStartReceiveField (WifiPpduField field, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << field << *event);
  NS_ASSERT (field != WIFI_PPDU_FIELD_PREAMBLE && field != WIFI_PPDU_FIELD_DATA); //handled apart for the time being
  auto ppduFormats = GetPpduFormats ();
  auto itFormat = ppduFormats.find (event->GetPpdu ()->GetPreamble ());
  if (itFormat != ppduFormats.end ())
    {
      auto itField = std::find (itFormat->second.begin (), itFormat->second.end (), field);
      if (itField != itFormat->second.end ())
        {
          return true; //supported field so we can start receiving
        }
    }
  return false; //unsupported otherwise
}

PhyEntity::PhyFieldRxStatus
PhyEntity::DoEndReceiveField (WifiPpduField field, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << field << *event);
  NS_ASSERT (field != WIFI_PPDU_FIELD_DATA); //handled apart for the time being
  if (field == WIFI_PPDU_FIELD_PREAMBLE)
    {
      return DoEndReceivePreamble (event);
    }
  return PhyFieldRxStatus (false); //failed reception by default
}

bool
PhyEntity::DoStartReceivePreamble (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  //TODO handle special cases in WifiPhy::StartReceivePreamble
  StartPreambleDetectionPeriod (event);
  return true;
}

PhyEntity::PhyFieldRxStatus
PhyEntity::DoEndReceivePreamble (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (m_wifiPhy->m_currentPreambleEvents.size () == 1); //Synched on one after detection period
  return PhyFieldRxStatus (true); //always consider that preamble has been correctly received if preamble detection was OK
}

void
PhyEntity::StartPreambleDetectionPeriod (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_LOG_DEBUG ("Sync to signal (power=" << GetRxPowerWForPpdu (event) << "W)");
  m_wifiPhy->m_interference.NotifyRxStart (); //We need to notify it now so that it starts recording events
  //TODO see if preamble detection events cannot be ported here
  m_wifiPhy->m_endPreambleDetectionEvents.push_back (Simulator::Schedule (m_wifiPhy->GetPreambleDetectionDuration (), &PhyEntity::EndPreambleDetectionPeriod, this, event));
}

void
PhyEntity::EndPreambleDetectionPeriod (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (!m_wifiPhy->IsStateRx ());
  NS_ASSERT (m_wifiPhy->m_endPhyRxEvent.IsExpired ()); //since end of preamble reception is scheduled by this method upon success

  //calculate PER on the measurement channel for PHY headers
  uint16_t measurementChannelWidth = m_wifiPhy->GetMeasurementChannelWidth (event->GetPpdu ());
  auto measurementBand = m_wifiPhy->GetBand (measurementChannelWidth);
  double maxRxPowerW = -1; //in case current event may not be sent on measurement channel (rxPowerW would be equal to 0)
  Ptr<Event> maxEvent;
  NS_ASSERT (!m_wifiPhy->m_currentPreambleEvents.empty ());
  for (auto preambleEvent : m_wifiPhy->m_currentPreambleEvents)
    {
      double rxPowerW = preambleEvent.second->GetRxPowerW (measurementBand);
      if (rxPowerW > maxRxPowerW)
        {
          maxRxPowerW = rxPowerW;
          maxEvent = preambleEvent.second;
        }
    }

  NS_ASSERT (maxEvent != 0);
  if (maxEvent != event)
    {
      NS_LOG_DEBUG ("Receiver got a stronger packet with UID " << maxEvent->GetPpdu ()->GetUid () << " during preamble detection: drop packet with UID " << event->GetPpdu ()->GetUid ());
      m_wifiPhy->NotifyRxDrop (GetAddressedPsduInPpdu (event->GetPpdu ()), BUSY_DECODING_PREAMBLE);
      auto it = m_wifiPhy->m_currentPreambleEvents.find (std::make_pair (event->GetPpdu ()->GetUid (), event->GetPpdu ()->GetPreamble ()));
      m_wifiPhy->m_currentPreambleEvents.erase (it);
      //This is needed to cleanup the m_firstPowerPerBand so that the first power corresponds to the power at the start of the PPDU
      m_wifiPhy->m_interference.NotifyRxEnd (maxEvent->GetStartTime ());
      //Make sure InterferenceHelper keeps recording events
      m_wifiPhy->m_interference.NotifyRxStart ();
      return;
    }

  m_wifiPhy->m_currentEvent = event;

  double snr = m_wifiPhy->m_interference.CalculateSnr (m_wifiPhy->m_currentEvent, measurementChannelWidth, 1, measurementBand);
  NS_LOG_DEBUG ("SNR(dB)=" << RatioToDb (snr) << " at end of preamble detection period");

  if ((!m_wifiPhy->m_preambleDetectionModel && maxRxPowerW > 0.0)
      || (m_wifiPhy->m_preambleDetectionModel && m_wifiPhy->m_preambleDetectionModel->IsPreambleDetected (m_wifiPhy->m_currentEvent->GetRxPowerW (measurementBand), snr, measurementChannelWidth)))
    {
      for (auto & endPreambleDetectionEvent : m_wifiPhy->m_endPreambleDetectionEvents)
        {
          endPreambleDetectionEvent.Cancel ();
        }
      m_wifiPhy->m_endPreambleDetectionEvents.clear ();

      for (auto it = m_wifiPhy->m_currentPreambleEvents.begin (); it != m_wifiPhy->m_currentPreambleEvents.end (); )
        {
          if (it->second != m_wifiPhy->m_currentEvent)
            {
              NS_LOG_DEBUG ("Drop packet with UID " << it->first.first << " and preamble " << it->first.second << " arrived at time " << it->second->GetStartTime ());
              WifiPhyRxfailureReason reason;
              if (m_wifiPhy->m_currentEvent->GetPpdu ()->GetUid () > it->first.first)
                {
                  reason = PREAMBLE_DETECTION_PACKET_SWITCH;
                  //This is needed to cleanup the m_firstPowerPerBand so that the first power corresponds to the power at the start of the PPDU
                  m_wifiPhy->m_interference.NotifyRxEnd (m_wifiPhy->m_currentEvent->GetStartTime ());
                }
              else
                {
                  reason = BUSY_DECODING_PREAMBLE;
                }
              m_wifiPhy->NotifyRxDrop (GetAddressedPsduInPpdu (it->second->GetPpdu ()), reason);
              it = m_wifiPhy->m_currentPreambleEvents.erase (it);
            }
          else
            {
              ++it;
            }
        }

      //Make sure InterferenceHelper keeps recording events
      m_wifiPhy->m_interference.NotifyRxStart ();

      m_wifiPhy->NotifyRxBegin (GetAddressedPsduInPpdu (m_wifiPhy->m_currentEvent->GetPpdu ()), m_wifiPhy->m_currentEvent->GetRxPowerWPerBand ());
      m_wifiPhy->m_timeLastPreambleDetected = Simulator::Now ();

      //Continue receiving preamble
      Time durationTillEnd = GetDuration (WIFI_PPDU_FIELD_PREAMBLE, event->GetTxVector ()) - m_wifiPhy->GetPreambleDetectionDuration ();
      m_state->SwitchMaybeToCcaBusy (durationTillEnd); //will be prolonged by next field
      m_wifiPhy->m_endPhyRxEvent = Simulator::Schedule (durationTillEnd, &PhyEntity::EndReceiveField, this, WIFI_PPDU_FIELD_PREAMBLE, event);
    }
  else
    {
      NS_LOG_DEBUG ("Drop packet because PHY preamble detection failed");
      // Like CCA-SD, CCA-ED is governed by the 4 us CCA window to flag CCA-BUSY
      // for any received signal greater than the CCA-ED threshold.
      m_wifiPhy->DropPreambleEvent (m_wifiPhy->m_currentEvent->GetPpdu (), PREAMBLE_DETECT_FAILURE, m_wifiPhy->m_currentEvent->GetEndTime (), m_wifiPhy->GetMeasurementChannelWidth (m_wifiPhy->m_currentEvent->GetPpdu ()));
      if (m_wifiPhy->m_currentPreambleEvents.empty ())
        {
          //Do not erase events if there are still pending preamble events to be processed
          m_wifiPhy->m_interference.NotifyRxEnd (Simulator::Now ());
        }
      m_wifiPhy->m_currentEvent = 0;
      //Cancel preamble reception
      m_wifiPhy->m_endPhyRxEvent.Cancel ();
    }
}

bool
PhyEntity::IsConfigSupported (Ptr<const WifiPpdu> ppdu) const
{
  WifiMode txMode = ppdu->GetTxVector ().GetMode ();
  if (!IsModeSupported (txMode))
    {
      NS_LOG_DEBUG ("Drop packet because it was sent using an unsupported mode (" << txMode << ")");
      return false;
    }
  return true;
}

void
PhyEntity::AbortCurrentReception (WifiPhyRxfailureReason reason)
{
  NS_LOG_FUNCTION (this << reason);
  //TODO cancel some events here later on
  m_wifiPhy->AbortCurrentReception (reason);
}

void
PhyEntity::ResetReceive (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << *event);
  NS_ASSERT (event->GetEndTime () == Simulator::Now ()); //TODO: overload for UL MU
  m_wifiPhy->ResetReceive (event);
}

double
PhyEntity::GetRandomValue (void) const
{
  return m_wifiPhy->m_random->GetValue ();
}

double
PhyEntity::GetRxPowerWForPpdu (Ptr<Event> event) const
{
  return event->GetRxPowerW (m_wifiPhy->GetBand (m_wifiPhy->GetMeasurementChannelWidth (event->GetPpdu ())));
}

} //namespace ns3
