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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/simulator.h"
#include "ns3/log.h"
#include "ns3/packet.h"
#include "interference-helper.h"
#include "wifi-phy.h"
#include "error-rate-model.h"
#include "wifi-utils.h"
#include "wifi-ppdu.h"
#include "wifi-psdu.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("InterferenceHelper");

/****************************************************************
 *       PHY event class
 ****************************************************************/

Event::Event (Ptr<const WifiPpdu> ppdu, WifiTxVector txVector, Time duration, double rxPower)
  : m_ppdu (ppdu),
    m_txVector (txVector),
    m_startTime (Simulator::Now ()),
    m_endTime (m_startTime + duration),
    m_rxPowerW (rxPower)
{
}

Event::~Event ()
{
}

Ptr<const WifiPsdu>
Event::GetPsdu (void) const
{
  return m_ppdu->GetPsdu ();
}

Ptr<const WifiPpdu>
Event::GetPpdu (void) const
{
  return m_ppdu;
}

Time
Event::GetStartTime (void) const
{
  return m_startTime;
}

Time
Event::GetEndTime (void) const
{
  return m_endTime;
}

Time
Event::GetDuration (void) const
{
  return m_endTime - m_startTime;
}

double
Event::GetRxPowerW (void) const
{
  return m_rxPowerW;
}

WifiTxVector
Event::GetTxVector (void) const
{
  return m_txVector;
}

std::ostream & operator << (std::ostream &os, const Event &event)
{
  os << "start=" << event.GetStartTime () << ", end=" << event.GetEndTime ()
     << ", TXVECTOR=" << event.GetTxVector ()
     << ", power=" << event.GetRxPowerW () << "W"
     << ", PPDU=" << event.GetPpdu ();
  return os;
}

/****************************************************************
 *       Class which records SNIR change events for a
 *       short period of time.
 ****************************************************************/

InterferenceHelper::NiChange::NiChange (double power, Ptr<Event> event)
  : m_power (power),
    m_event (event)
{
}

double
InterferenceHelper::NiChange::GetPower (void) const
{
  return m_power;
}

void
InterferenceHelper::NiChange::AddPower (double power)
{
  m_power += power;
}

Ptr<Event>
InterferenceHelper::NiChange::GetEvent (void) const
{
  return m_event;
}


/****************************************************************
 *       The actual InterferenceHelper
 ****************************************************************/

InterferenceHelper::InterferenceHelper ()
  : m_errorRateModel (0),
    m_numRxAntennas (1),
    m_firstPower (0),
    m_rxing (false)
{
  // Always have a zero power noise event in the list
  AddNiChangeEvent (Time (0), NiChange (0.0, 0));
}

InterferenceHelper::~InterferenceHelper ()
{
  EraseEvents ();
  m_errorRateModel = 0;
}

Ptr<Event>
InterferenceHelper::Add (Ptr<const WifiPpdu> ppdu, WifiTxVector txVector, Time duration, double rxPowerW)
{
  Ptr<Event> event = Create<Event> (ppdu, txVector, duration, rxPowerW);
  AppendEvent (event);
  return event;
}

void
InterferenceHelper::AddForeignSignal (Time duration, double rxPowerW)
{
  // Parameters other than duration and rxPowerW are unused for this type
  // of signal, so we provide dummy versions
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_QOSDATA);
  Ptr<WifiPpdu> fakePpdu = Create<WifiPpdu> (Create<WifiPsdu> (Create<Packet> (0), hdr),
                                             WifiTxVector (), duration, WIFI_PHY_BAND_UNSPECIFIED);
  Add (fakePpdu, WifiTxVector (), duration, rxPowerW);
}

void
InterferenceHelper::SetNoiseFigure (double value)
{
  m_noiseFigure = value;
}

void
InterferenceHelper::SetErrorRateModel (const Ptr<ErrorRateModel> rate)
{
  m_errorRateModel = rate;
}

Ptr<ErrorRateModel>
InterferenceHelper::GetErrorRateModel (void) const
{
  return m_errorRateModel;
}

void
InterferenceHelper::SetNumberOfReceiveAntennas (uint8_t rx)
{
  m_numRxAntennas = rx;
}

Time
InterferenceHelper::GetEnergyDuration (double energyW) const
{
  Time now = Simulator::Now ();
  auto i = GetPreviousPosition (now);
  Time end = i->first;
  for (; i != m_niChanges.end (); ++i)
    {
      double noiseInterferenceW = i->second.GetPower ();
      end = i->first;
      if (noiseInterferenceW < energyW)
        {
          break;
        }
    }
  return end > now ? end - now : MicroSeconds (0);
}

void
InterferenceHelper::AppendEvent (Ptr<Event> event)
{
  NS_LOG_FUNCTION (this);
  double previousPowerStart = 0;
  double previousPowerEnd = 0;
  previousPowerStart = GetPreviousPosition (event->GetStartTime ())->second.GetPower ();
  previousPowerEnd = GetPreviousPosition (event->GetEndTime ())->second.GetPower ();

  if (!m_rxing)
    {
      m_firstPower = previousPowerStart;
      // Always leave the first zero power noise event in the list
      m_niChanges.erase (++(m_niChanges.begin ()),
                         GetNextPosition (event->GetStartTime ()));
    }
  auto first = AddNiChangeEvent (event->GetStartTime (), NiChange (previousPowerStart, event));
  auto last = AddNiChangeEvent (event->GetEndTime (), NiChange (previousPowerEnd, event));
  for (auto i = first; i != last; ++i)
    {
      i->second.AddPower (event->GetRxPowerW ());
    }
}

double
InterferenceHelper::CalculateSnr (double signal, double noiseInterference, WifiTxVector txVector) const
{
  //thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.3803e-23;
  uint16_t channelWidth = txVector.GetChannelWidth ();
  //Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290 * channelWidth * 1e6;
  //receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noiseFloor = m_noiseFigure * Nt;
  double noise = noiseFloor + noiseInterference;
  double snr = signal / noise; //linear scale
  NS_LOG_DEBUG ("bandwidth(MHz)=" << channelWidth << ", signal(W)= " << signal << ", noise(W)=" << noiseFloor << ", interference(W)=" << noiseInterference << ", snr=" << RatioToDb(snr) << "dB");
  double gain = 1;
  if (m_numRxAntennas > txVector.GetNss ())
    {
      gain = static_cast<double>(m_numRxAntennas) / txVector.GetNss (); //compute gain offered by diversity for AWGN
    }
  NS_LOG_DEBUG ("SNR improvement thanks to diversity: " << 10 * std::log10 (gain) << "dB");
  snr *= gain;
  return snr;
}

double
InterferenceHelper::CalculateNoiseInterferenceW (Ptr<Event> event, NiChanges *ni) const
{
  double noiseInterferenceW = m_firstPower;
  auto it = m_niChanges.find (event->GetStartTime ());
  for (; it != m_niChanges.end () && it->first < Simulator::Now (); ++it)
    {
      noiseInterferenceW = it->second.GetPower () - event->GetRxPowerW ();
    }
  it = m_niChanges.find (event->GetStartTime ());
  for (; it != m_niChanges.end () && it->second.GetEvent () != event; ++it);
  ni->emplace (event->GetStartTime (), NiChange (0, event));
  while (++it != m_niChanges.end () && it->second.GetEvent () != event)
    {
      ni->insert (*it);
    }
  ni->emplace (event->GetEndTime (), NiChange (0, event));
  NS_ASSERT_MSG (noiseInterferenceW >= 0, "CalculateNoiseInterferenceW returns negative value " << noiseInterferenceW);
  return noiseInterferenceW;
}

double
InterferenceHelper::CalculateChunkSuccessRate (double snir, Time duration, WifiMode mode, WifiTxVector txVector) const
{
  if (duration.IsZero ())
    {
      return 1.0;
    }
  uint64_t rate = mode.GetDataRate (txVector.GetChannelWidth ());
  uint64_t nbits = static_cast<uint64_t> (rate * duration.GetSeconds ());
  double csr = m_errorRateModel->GetChunkSuccessRate (mode, txVector, snir, nbits);
  return csr;
}

double
InterferenceHelper::CalculatePayloadChunkSuccessRate (double snir, Time duration, WifiTxVector txVector) const
{
  if (duration.IsZero ())
    {
      return 1.0;
    }
  WifiMode mode = txVector.GetMode ();
  uint64_t rate = mode.GetDataRate (txVector);
  uint64_t nbits = static_cast<uint64_t> (rate * duration.GetSeconds ());
  nbits /= txVector.GetNss (); //divide effective number of bits by NSS to achieve same chunk error rate as SISO for AWGN
  double csr = m_errorRateModel->GetChunkSuccessRate (mode, txVector, snir, nbits);
  return csr;
}

double
InterferenceHelper::CalculatePayloadPer (Ptr<const Event> event, NiChanges *ni, std::pair<Time, Time> window) const
{
  NS_LOG_FUNCTION (this << window.first << window.second);
  const WifiTxVector txVector = event->GetTxVector ();
  double psr = 1.0; /* Packet Success Rate */
  auto j = ni->begin ();
  Time previous = j->first;
  WifiMode payloadMode = event->GetTxVector ().GetMode ();
  WifiPreamble preamble = txVector.GetPreambleType ();
  Time phyHeaderStart = j->first + WifiPhy::GetPhyPreambleDuration (txVector); //PPDU start time + preamble
  Time phyLSigHeaderEnd = phyHeaderStart + WifiPhy::GetPhyHeaderDuration (txVector); //PPDU start time + preamble + L-SIG
  Time phyTrainingSymbolsStart = phyLSigHeaderEnd + WifiPhy::GetPhyHtSigHeaderDuration (preamble) + WifiPhy::GetPhySigA1Duration (preamble) + WifiPhy::GetPhySigA2Duration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A
  Time phyPayloadStart = phyTrainingSymbolsStart + WifiPhy::GetPhyTrainingSymbolDuration (txVector) + WifiPhy::GetPhySigBDuration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
  Time windowStart = phyPayloadStart + window.first;
  Time windowEnd = phyPayloadStart + window.second;
  double noiseInterferenceW = m_firstPower;
  double powerW = event->GetRxPowerW ();
  while (++j != ni->end ())
    {
      Time current = j->first;
      NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
      NS_ASSERT (current >= previous);
      double snr = CalculateSnr (powerW, noiseInterferenceW, txVector);
      //Case 1: Both previous and current point to the windowed payload
      if (previous >= windowStart)
        {
          psr *= CalculatePayloadChunkSuccessRate (snr, Min (windowEnd, current) - previous, txVector);
          NS_LOG_DEBUG ("Both previous and current point to the windowed payload: mode=" << payloadMode << ", psr=" << psr);
        }
      //Case 2: previous is before windowed payload and current is in the windowed payload
      else if (current >= windowStart)
        {
          psr *= CalculatePayloadChunkSuccessRate (snr, Min (windowEnd, current) - windowStart, txVector);
          NS_LOG_DEBUG ("previous is before windowed payload and current is in the windowed payload: mode=" << payloadMode << ", psr=" << psr);
        }
      noiseInterferenceW = j->second.GetPower () - powerW;
      previous = j->first;
      if (previous > windowEnd)
        {
          NS_LOG_DEBUG ("Stop: new previous=" << previous << " after time window end=" << windowEnd);
          break;
        }
    }
  double per = 1 - psr;
  return per;
}

double
InterferenceHelper::CalculateNonHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const
{
  NS_LOG_FUNCTION (this);
  const WifiTxVector txVector = event->GetTxVector ();
  double psr = 1.0; /* Packet Success Rate */
  auto j = ni->begin ();
  Time previous = j->first;
  WifiPreamble preamble = txVector.GetPreambleType ();
  WifiMode headerMode = WifiPhy::GetPhyHeaderMode (txVector);
  Time phyHeaderStart = j->first + WifiPhy::GetPhyPreambleDuration (txVector); //PPDU start time + preamble
  Time phyLSigHeaderEnd = phyHeaderStart + WifiPhy::GetPhyHeaderDuration (txVector); //PPDU start time + preamble + L-SIG
  Time phyTrainingSymbolsStart = phyLSigHeaderEnd + WifiPhy::GetPhyHtSigHeaderDuration (preamble) + WifiPhy::GetPhySigA1Duration (preamble) + WifiPhy::GetPhySigA2Duration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A
  Time phyPayloadStart = phyTrainingSymbolsStart + WifiPhy::GetPhyTrainingSymbolDuration (txVector) + WifiPhy::GetPhySigBDuration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
  double noiseInterferenceW = m_firstPower;
  double powerW = event->GetRxPowerW ();
  while (++j != ni->end ())
    {
      Time current = j->first;
      NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
      NS_ASSERT (current >= previous);
      double snr = CalculateSnr (powerW, noiseInterferenceW, txVector);
      //Case 1: previous and current after payload start
      if (previous >= phyPayloadStart)
        {
          psr *= 1;
          NS_LOG_DEBUG ("Case 1 - previous and current after payload start: nothing to do");
        }
      //Case 2: previous is in training or in SIG-B: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
      else if (previous >= phyTrainingSymbolsStart)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          psr *= 1;
          NS_LOG_DEBUG ("Case 2 - previous is in training or in SIG-B: nothing to do");
        }
      //Case 3: previous is in HT-SIG or SIG-A: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
      else if (previous >= phyLSigHeaderEnd)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          psr *= 1;
          NS_LOG_DEBUG ("Case 3cii - previous is in HT-SIG or SIG-A: nothing to do");
        }
      //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
      else if (previous >= phyHeaderStart)
        {
          NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
          //Case 4a: current after payload start
          if (current >= phyPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - previous, headerMode, txVector);
              NS_LOG_DEBUG ("Case 4a - previous in L-SIG and current after payload start: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 4b: current is in training or in SIG-B. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - previous, headerMode, txVector);
              NS_LOG_DEBUG ("Case 4a - previous in L-SIG and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 4c: current in HT-SIG or in SIG-A. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyLSigHeaderEnd)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - previous, headerMode, txVector);
              NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in HT-SIG or in SIG-A: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 4d: current with previous in L-SIG
          else
            {
              psr *= CalculateChunkSuccessRate (snr, current - previous, headerMode, txVector);
              NS_LOG_DEBUG ("Case 4d - current with previous in L-SIG: mode=" << headerMode << ", psr=" << psr);
            }
        }
      //Case 5: previous is in the preamble works for all cases
      else
        {
          //Case 5a: current after payload start
          if (current >= phyPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - phyHeaderStart, headerMode, txVector);
              NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 5b: current is in training or in SIG-B. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - phyHeaderStart, headerMode, txVector);
              NS_LOG_DEBUG ("Case 5b - previous is in the preamble and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 5c: current in HT-SIG or in SIG-A. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyLSigHeaderEnd)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              psr *= CalculateChunkSuccessRate (snr, phyLSigHeaderEnd - phyHeaderStart, headerMode, txVector);
              NS_LOG_DEBUG ("Case 5b - previous is in the preamble and current in HT-SIG or in SIG-A: mode=" << headerMode << ", psr=" << psr);
            }
          //Case 5d: current is in L-SIG.
          else if (current >= phyHeaderStart)
            {
              NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
              psr *= CalculateChunkSuccessRate (snr, current - phyHeaderStart, headerMode, txVector);
              NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: mode=" << headerMode << ", psr=" << psr);
            }
        }

      noiseInterferenceW = j->second.GetPower () - powerW;
      previous = j->first;
    }

  double per = 1 - psr;
  return per;
}

double
InterferenceHelper::CalculateHtPhyHeaderPer (Ptr<const Event> event, NiChanges *ni) const
{
  NS_LOG_FUNCTION (this);
  const WifiTxVector txVector = event->GetTxVector ();
  double psr = 1.0; /* Packet Success Rate */
  auto j = ni->begin ();
  Time previous = j->first;
  WifiPreamble preamble = txVector.GetPreambleType ();
  WifiMode mcsHeaderMode;
  if (preamble == WIFI_PREAMBLE_HT_MF || preamble == WIFI_PREAMBLE_HT_GF)
    {
      //mode for PHY header fields sent with HT modulation
      mcsHeaderMode = WifiPhy::GetHtPhyHeaderMode ();
    }
  else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_VHT_MU)
    {
      //mode for PHY header fields sent with VHT modulation
      mcsHeaderMode = WifiPhy::GetVhtPhyHeaderMode ();
    }
  else if (preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_HE_MU)
    {
      //mode for PHY header fields sent with HE modulation
      mcsHeaderMode = WifiPhy::GetHePhyHeaderMode ();
    }
  WifiMode headerMode = WifiPhy::GetPhyHeaderMode (txVector);
  Time phyHeaderStart = j->first + WifiPhy::GetPhyPreambleDuration (txVector); //PPDU start time + preamble
  Time phyLSigHeaderEnd = phyHeaderStart + WifiPhy::GetPhyHeaderDuration (txVector); //PPDU start time + preamble + L-SIG
  Time phyTrainingSymbolsStart = phyLSigHeaderEnd + WifiPhy::GetPhyHtSigHeaderDuration (preamble) + WifiPhy::GetPhySigA1Duration (preamble) + WifiPhy::GetPhySigA2Duration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A
  Time phyPayloadStart = phyTrainingSymbolsStart + WifiPhy::GetPhyTrainingSymbolDuration (txVector) + WifiPhy::GetPhySigBDuration (preamble); //PPDU start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
  double noiseInterferenceW = m_firstPower;
  double powerW = event->GetRxPowerW ();
  while (++j != ni->end ())
    {
      Time current = j->first;
      NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
      NS_ASSERT (current >= previous);
      double snr = CalculateSnr (powerW, noiseInterferenceW, txVector);
      //Case 1: previous and current after payload start: nothing to do
      if (previous >= phyPayloadStart)
        {
          psr *= 1;
          NS_LOG_DEBUG ("Case 1 - previous and current after payload start: nothing to do");
        }
      //Case 2: previous is in training or in SIG-B: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
      else if (previous >= phyTrainingSymbolsStart)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          //Case 2a: current after payload start
          if (current >= phyPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - previous, mcsHeaderMode, txVector);
              NS_LOG_DEBUG ("Case 2a - previous is in training or in SIG-B and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
            }
          //Case 2b: current is in training or in SIG-B
          else
            {
              psr *= CalculateChunkSuccessRate (snr, current - previous, mcsHeaderMode, txVector);
              NS_LOG_DEBUG ("Case 2b - previous is in training or in SIG-B and current is in training or in SIG-B: mode=" << mcsHeaderMode << ", psr=" << psr);
            }
        }
      //Case 3: previous is in HT-SIG or SIG-A: non-HT will not enter here since it didn't enter in the last two and they are all the same for non-HT
      else if (previous >= phyLSigHeaderEnd)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          //Case 3a: current after payload start
          if (current >= phyPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
              //Case 3ai: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  //SIG-A is sent using non-HT OFDM modulation
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 3ai - previous is in SIG-A and current after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3aii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 3aii - previous is in HT-SIG and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 3b: current is in training or in SIG-B
          else if (current >= phyTrainingSymbolsStart)
            {
              psr *= CalculateChunkSuccessRate (snr, current - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
              //Case 3bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  //SIG-A is sent using non-HT OFDM modulation
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 3bi - previous is in SIG-A and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3bii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - previous, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 3bii - previous is in HT-SIG and current is in HT training: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 3c: current with previous in HT-SIG or SIG-A
          else
            {
              //Case 3ci: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  //SIG-A is sent using non-HT OFDM modulation
                  psr *= CalculateChunkSuccessRate (snr, current - previous, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 3ci - previous with current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3cii: HT mixed format or HT greenfield
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, current - previous, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 3cii - previous with current in HT-SIG: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
        }
      //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
      else if (previous >= phyHeaderStart)
        {
          NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
          //Case 4a: current after payload start
          if (current >= phyPayloadStart)
            {
              //Case 4ai: non-HT format
              if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                {
                  psr *= 1;
                  NS_LOG_DEBUG ("Case 4ai - previous in L-SIG and current after payload start: nothing to do");
                }
              //Case 4aii: VHT or HE format
              else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 4aii - previous is in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4aiii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 4aiii - previous in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 4b: current is in training or in SIG-B. non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 4bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 4bi - previous is in L-SIG and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4bii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 4bii - previous in L-SIG and current in HT training: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 4c: current in HT-SIG or in SIG-A. Non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyLSigHeaderEnd)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 4ci: VHT format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4cii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 4cii - previous in L-SIG and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 4d: current with previous in L-SIG
          else
            {
              psr *= 1;
              NS_LOG_DEBUG ("Case 4d - current with previous in L-SIG: nothing to do");
            }
        }
      //Case 5: previous is in the preamble works for all cases
      else
        {
          //Case 5a: current after payload start
          if (current >= phyPayloadStart)
            {
              //Case 5ai: non-HT format (No HT-SIG or Training Symbols)
              if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                {
                  psr *= 1;
                  NS_LOG_DEBUG ("Case 5ai - previous is in the preamble and current is after payload start: nothing to do");
                }
              //Case 5aii: VHT or HE format
              else if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5aiii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, phyPayloadStart - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 5aiii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 5b: current is in training or in SIG-B. Non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 5bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyTrainingSymbolsStart, mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (snr, phyTrainingSymbolsStart - phyLSigHeaderEnd, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 5bi - previous is in the preamble and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", non-HT mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5bii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 5bii - previous is in the preamble and current in HT training: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 5c: current in HT-SIG or in SIG-A. Non-HT will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= phyLSigHeaderEnd)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
              //Case 5ci: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT_SU || preamble == WIFI_PREAMBLE_HE_SU || preamble == WIFI_PREAMBLE_VHT_MU || preamble == WIFI_PREAMBLE_HE_MU)
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, headerMode, txVector);
                  NS_LOG_DEBUG ("Case 5ci - previous is in preamble and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5cii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (snr, current - phyLSigHeaderEnd, mcsHeaderMode, txVector);
                  NS_LOG_DEBUG ("Case 5cii - previous in preamble and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 5d: current is in L-SIG. HT-GF will not come here
          else if (current >= phyHeaderStart)
            {
              NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
              psr *= 1;
              NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: nothing to do");
            }
        }

      noiseInterferenceW = j->second.GetPower () - powerW;
      previous = j->first;
    }

  double per = 1 - psr;
  return per;
}

struct InterferenceHelper::SnrPer
InterferenceHelper::CalculatePayloadSnrPer (Ptr<Event> event, std::pair<Time, Time> relativeMpduStartStop) const
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector ());

  /* calculate the SNIR at the start of the MPDU (located through windowing) and accumulate
   * all SNIR changes in the SNIR vector.
   */
  double per = CalculatePayloadPer (event, &ni, relativeMpduStartStop);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

double
InterferenceHelper::CalculateSnr (Ptr<Event> event) const
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector ());
 return snr;
}

struct InterferenceHelper::SnrPer
InterferenceHelper::CalculateNonHtPhyHeaderSnrPer (Ptr<Event> event) const
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector ());

  /* calculate the SNIR at the start of the PHY header and accumulate
   * all SNIR changes in the SNIR vector.
   */
  double per = CalculateNonHtPhyHeaderPer (event, &ni);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

struct InterferenceHelper::SnrPer
InterferenceHelper::CalculateHtPhyHeaderSnrPer (Ptr<Event> event) const
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector ());
  
  /* calculate the SNIR at the start of the PHY header and accumulate
   * all SNIR changes in the SNIR vector.
   */
  double per = CalculateHtPhyHeaderPer (event, &ni);
  
  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

void
InterferenceHelper::EraseEvents (void)
{
  m_niChanges.clear ();
  // Always have a zero power noise event in the list
  AddNiChangeEvent (Time (0), NiChange (0.0, 0));
  m_rxing = false;
  m_firstPower = 0;
}

InterferenceHelper::NiChanges::const_iterator
InterferenceHelper::GetNextPosition (Time moment) const
{
  return m_niChanges.upper_bound (moment);
}

InterferenceHelper::NiChanges::const_iterator
InterferenceHelper::GetPreviousPosition (Time moment) const
{
  auto it = GetNextPosition (moment);
  // This is safe since there is always an NiChange at time 0,
  // before moment.
  --it;
  return it;
}

InterferenceHelper::NiChanges::iterator
InterferenceHelper::AddNiChangeEvent (Time moment, NiChange change)
{
  return m_niChanges.insert (GetNextPosition (moment), std::make_pair (moment, change));
}

void
InterferenceHelper::NotifyRxStart ()
{
  NS_LOG_FUNCTION (this);
  m_rxing = true;
}

void
InterferenceHelper::NotifyRxEnd ()
{
  NS_LOG_FUNCTION (this);
  m_rxing = false;
  //Update m_firstPower for frame capture
  auto it = GetPreviousPosition (Simulator::Now ());
  it--;
  m_firstPower = it->second.GetPower ();
}

} //namespace ns3
