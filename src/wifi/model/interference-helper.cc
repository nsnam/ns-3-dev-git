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

#include "interference-helper.h"
#include "wifi-phy.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("InterferenceHelper");

/****************************************************************
 *       Phy event class
 ****************************************************************/

InterferenceHelper::Event::Event (Ptr<const Packet> packet, WifiTxVector txVector, Time duration, double rxPower)
  : m_packet (packet),
    m_txVector (txVector),
    m_startTime (Simulator::Now ()),
    m_endTime (m_startTime + duration),
    m_rxPowerW (rxPower)
{
}

InterferenceHelper::Event::~Event ()
{
}

Ptr<const Packet>
InterferenceHelper::Event::GetPacket (void) const
{
  return m_packet;
}

Time
InterferenceHelper::Event::GetDuration (void) const
{
  return m_endTime - m_startTime;
}

Time
InterferenceHelper::Event::GetStartTime (void) const
{
  return m_startTime;
}

Time
InterferenceHelper::Event::GetEndTime (void) const
{
  return m_endTime;
}

double
InterferenceHelper::Event::GetRxPowerW (void) const
{
  return m_rxPowerW;
}

WifiTxVector
InterferenceHelper::Event::GetTxVector (void) const
{
  return m_txVector;
}

WifiMode
InterferenceHelper::Event::GetPayloadMode (void) const
{
  return m_txVector.GetMode ();
}


/****************************************************************
 *       Class which records SNIR change events for a
 *       short period of time.
 ****************************************************************/

InterferenceHelper::NiChange::NiChange (Time time, double delta, Ptr<InterferenceHelper::Event> event)
  : m_time (time),
    m_delta (delta),
    m_event (event)
{
}

Time
InterferenceHelper::NiChange::GetTime (void) const
{
  return m_time;
}

double
InterferenceHelper::NiChange::GetDelta (void) const
{
  return m_delta;
}

Ptr<InterferenceHelper::Event>
InterferenceHelper::NiChange::GetEvent (void) const
{
  return m_event;
}

bool
InterferenceHelper::NiChange::operator < (const InterferenceHelper::NiChange& o) const
{
  return (m_time < o.m_time);
}


/****************************************************************
 *       The actual InterferenceHelper
 ****************************************************************/

InterferenceHelper::InterferenceHelper ()
  : m_errorRateModel (0),
    m_numRxAntennas (1),
    m_firstPower (0.0),
    m_rxing (false)
{
}

InterferenceHelper::~InterferenceHelper ()
{
  EraseEvents ();
  m_errorRateModel = 0;
}

Ptr<InterferenceHelper::Event>
InterferenceHelper::Add (Ptr<const Packet> packet, WifiTxVector txVector, Time duration, double rxPowerW)
{
  Ptr<InterferenceHelper::Event> event;
  event = Create<InterferenceHelper::Event> (packet, txVector, duration, rxPowerW);
  AppendEvent (event);
  return event;
}

void
InterferenceHelper::AddForeignSignal (Time duration, double rxPowerW)
{
  // Parameters other than duration and rxPowerW are unused for this type
  // of signal, so we provide dummy versions
  WifiTxVector fakeTxVector;
  Ptr<const Packet> packet (0);
  Add (packet, fakeTxVector, duration, rxPowerW);
}

void
InterferenceHelper::SetNoiseFigure (double value)
{
  m_noiseFigure = value;
}

double
InterferenceHelper::GetNoiseFigure (void) const
{
  return m_noiseFigure;
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
  double noiseInterferenceW = 0.0;
  Time end = now;
  noiseInterferenceW = m_firstPower;
  for (NiChanges::const_iterator i = m_niChanges.begin (); i != m_niChanges.end (); i++)
    {
      noiseInterferenceW += i->GetDelta ();
      end = i->GetTime ();
      if (end < now)
        {
          continue;
        }
      if (noiseInterferenceW < energyW)
        {
          break;
        }
    }
  return end > now ? end - now : MicroSeconds (0);
}

void
InterferenceHelper::AppendEvent (Ptr<InterferenceHelper::Event> event)
{
  Time now = Simulator::Now ();
  if (!m_rxing)
    {
      NiChanges::const_iterator nowIterator = GetPosition (now);
      for (NiChanges::const_iterator i = m_niChanges.begin (); i != nowIterator; i++)
        {
          m_firstPower += i->GetDelta ();
        }
      m_niChanges.erase (m_niChanges.begin (), nowIterator);
      m_niChanges.insert (m_niChanges.begin (), NiChange (event->GetStartTime (), event->GetRxPowerW (), event));
    }
  else
    {
      AddNiChangeEvent (NiChange (event->GetStartTime (), event->GetRxPowerW (), event));
    }
  AddNiChangeEvent (NiChange (event->GetEndTime (), -event->GetRxPowerW (), event));

}


double
InterferenceHelper::CalculateSnr (double signal, double noiseInterference, uint8_t channelWidth) const
{
  //thermal noise at 290K in J/s = W
  static const double BOLTZMANN = 1.3803e-23;
  //Nt is the power of thermal noise in W
  double Nt = BOLTZMANN * 290.0 * channelWidth * 1000000;
  //receiver noise Floor (W) which accounts for thermal noise and non-idealities of the receiver
  double noiseFloor = m_noiseFigure * Nt;
  double noise = noiseFloor + noiseInterference;
  double snr = signal / noise; //linear scale
  NS_LOG_DEBUG ("bandwidth(MHz)=" << (uint16_t)channelWidth << ", signal(W)= " << signal << ", noise(W)=" << noiseFloor << ", interference(W)=" << noiseInterference << ", snr(linear)=" << snr);
  return snr;
}

double
InterferenceHelper::CalculateNoiseInterferenceW (Ptr<InterferenceHelper::Event> event, NiChanges *ni) const
{
  double noiseInterference = m_firstPower;
  NiChanges::const_iterator eventIterator = m_niChanges.begin ();
  while (eventIterator != m_niChanges.end ())
    {
      // Iterate the NI change list from the beginning to the end
      // until find the position of the event in the NI change list
      // The reason of using the event that causes the NI change to identify
      // different NI changes is because in some special cases
      // different NI changes happen at the same time with the same delta
      // value. Therefore, it may be impossible to identify a NI change that belongs
      // to which event based on just the NI time and NI delta value
      if (eventIterator->GetEvent () != event)
        {
          // The NI changes which happen before the event should be considered
          // as the interference. This considers the case that the receiving event
          // arrives while another receiving event is going on. The SINR of
          // the newly arrived event is calculated for checking the possibility of frame capture
          noiseInterference += eventIterator->GetDelta ();
        }
      else
        {
          break;
        }
      ++eventIterator;
    }

  for (NiChanges::const_iterator i = eventIterator + 1; i != m_niChanges.end (); ++i)
    {
      if (event->GetEndTime () == i->GetTime () && event == i->GetEvent ())
        {
          break;
        }
      ni->push_back (*i);
    }
  ni->insert (ni->begin (), NiChange (event->GetStartTime (), noiseInterference, event));
  ni->push_back (NiChange (event->GetEndTime (), 0, event));
  return noiseInterference;
}

double
InterferenceHelper::CalculateChunkSuccessRate (double snir, Time duration, WifiMode mode, WifiTxVector txVector) const
{
  if (duration.IsZero ())
    {
      return 1.0;
    }
  uint64_t rate = mode.GetPhyRate (txVector);
  uint64_t nbits = (uint64_t)(rate * duration.GetSeconds ());
  if (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HT || txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_VHT || txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_HE)
    {
      nbits /= txVector.GetNss (); //divide effective number of bits by NSS to achieve same chunk error rate as SISO for AWGN
      double gain = (txVector.GetNTx () * m_numRxAntennas); //compute gain offered by MIMO, SIMO or MISO compared to SISO for AWGN
      NS_LOG_DEBUG ("TX=" << (uint16_t)txVector.GetNTx () << ", RX=" << (uint16_t)m_numRxAntennas << ", SNIR improvement=+" << 10.0 * std::log10 (gain) << "dB");
      snir *= gain;
    }
  double csr = m_errorRateModel->GetChunkSuccessRate (mode, txVector, snir, (uint32_t)nbits);
  return csr;
}

double
InterferenceHelper::CalculatePlcpPayloadPer (Ptr<const InterferenceHelper::Event> event, NiChanges *ni) const
{
  NS_LOG_FUNCTION (this);
  const WifiTxVector txVector = event->GetTxVector ();
  double psr = 1.0; /* Packet Success Rate */
  NiChanges::const_iterator j = ni->begin ();
  Time previous = (*j).GetTime ();
  WifiMode payloadMode = event->GetPayloadMode ();
  WifiPreamble preamble = txVector.GetPreambleType ();
  Time plcpHeaderStart = (*j).GetTime () + WifiPhy::GetPlcpPreambleDuration (txVector); //packet start time + preamble
  Time plcpHsigHeaderStart = plcpHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //packet start time + preamble + L-SIG
  Time plcpTrainingSymbolsStart = plcpHsigHeaderStart + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A
  Time plcpPayloadStart = plcpTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
  double noiseInterferenceW = (*j).GetDelta ();
  double powerW = event->GetRxPowerW ();
  j++;
  while (ni->end () != j)
    {
      Time current = (*j).GetTime ();
      NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
      NS_ASSERT (current >= previous);
      //Case 1: Both previous and current point to the payload
      if (previous >= plcpPayloadStart)
        {
          psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                          noiseInterferenceW,
                                                          txVector.GetChannelWidth ()),
                                            current - previous,
                                            payloadMode, txVector);

          NS_LOG_DEBUG ("Both previous and current point to the payload: mode=" << payloadMode << ", psr=" << psr);
        }
      //Case 2: previous is before payload and current is in the payload
      else if (current >= plcpPayloadStart)
        {
          psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                          noiseInterferenceW,
                                                          txVector.GetChannelWidth ()),
                                            current - plcpPayloadStart,
                                            payloadMode, txVector);
          NS_LOG_DEBUG ("previous is before payload and current is in the payload: mode=" << payloadMode << ", psr=" << psr);
        }

      noiseInterferenceW += (*j).GetDelta ();
      previous = (*j).GetTime ();
      j++;
    }

  double per = 1 - psr;
  return per;
}

double
InterferenceHelper::CalculatePlcpHeaderPer (Ptr<const InterferenceHelper::Event> event, NiChanges *ni) const
{
  NS_LOG_FUNCTION (this);
  const WifiTxVector txVector = event->GetTxVector ();
  double psr = 1.0; /* Packet Success Rate */
  NiChanges::const_iterator j = ni->begin ();
  Time previous = (*j).GetTime ();
  WifiPreamble preamble = txVector.GetPreambleType ();
  WifiMode mcsHeaderMode;
  if (preamble == WIFI_PREAMBLE_HT_MF || preamble == WIFI_PREAMBLE_HT_GF)
    {
      //mode for PLCP header fields sent with HT modulation
      mcsHeaderMode = WifiPhy::GetHtPlcpHeaderMode ();
    }
  else if (preamble == WIFI_PREAMBLE_VHT)
    {
      //mode for PLCP header fields sent with VHT modulation
      mcsHeaderMode = WifiPhy::GetVhtPlcpHeaderMode ();
    }
  else if (preamble == WIFI_PREAMBLE_HE_SU)
    {
      //mode for PLCP header fields sent with HE modulation
      mcsHeaderMode = WifiPhy::GetHePlcpHeaderMode ();
    }
  WifiMode headerMode = WifiPhy::GetPlcpHeaderMode (txVector);
  Time plcpHeaderStart = (*j).GetTime () + WifiPhy::GetPlcpPreambleDuration (txVector); //packet start time + preamble
  Time plcpHsigHeaderStart = plcpHeaderStart + WifiPhy::GetPlcpHeaderDuration (txVector); //packet start time + preamble + L-SIG
  Time plcpTrainingSymbolsStart = plcpHsigHeaderStart + WifiPhy::GetPlcpHtSigHeaderDuration (preamble) + WifiPhy::GetPlcpSigA1Duration (preamble) + WifiPhy::GetPlcpSigA2Duration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A
  Time plcpPayloadStart = plcpTrainingSymbolsStart + WifiPhy::GetPlcpTrainingSymbolDuration (txVector) + WifiPhy::GetPlcpSigBDuration (preamble); //packet start time + preamble + L-SIG + HT-SIG or SIG-A + Training + SIG-B
  double noiseInterferenceW = (*j).GetDelta ();
  double powerW = event->GetRxPowerW ();
  j++;
  while (ni->end () != j)
    {
      Time current = (*j).GetTime ();
      NS_LOG_DEBUG ("previous= " << previous << ", current=" << current);
      NS_ASSERT (current >= previous);
      //Case 1: previous and current after playload start: nothing to do
      if (previous >= plcpPayloadStart)
        {
          psr *= 1;
          NS_LOG_DEBUG ("Case 1 - previous and current after playload start: nothing to do");
        }
      //Case 2: previous is in training or in SIG-B: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
      else if (previous >= plcpTrainingSymbolsStart)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          //Case 2a: current after payload start
          if (current >= plcpPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpPayloadStart - previous,
                                                mcsHeaderMode, txVector);

              NS_LOG_DEBUG ("Case 2a - previous is in training or in SIG-B and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
            }
          //Case 2b: current is in training or in SIG-B
          else
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - previous,
                                                mcsHeaderMode, txVector);

              NS_LOG_DEBUG ("Case 2b - previous is in training or in SIG-B and current is in training or in SIG-B: mode=" << mcsHeaderMode << ", psr=" << psr);
            }
        }
      //Case 3: previous is in HT-SIG or SIG-A: legacy will not enter here since it didn't enter in the last two and they are all the same for legacy
      else if (previous >= plcpHsigHeaderStart)
        {
          NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));
          //Case 3a: current after payload start
          if (current >= plcpPayloadStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                plcpPayloadStart - plcpTrainingSymbolsStart,
                                                mcsHeaderMode, txVector);

              //Case 3ai: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  //SIG-A is sent using legacy OFDM modulation
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 3ai - previous is in SIG-A and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3aii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    mcsHeaderMode, txVector);

                  NS_LOG_DEBUG ("Case 3aii - previous is in HT-SIG and current after payload start: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 3b: current is in training or in SIG-B
          else if (current >= plcpTrainingSymbolsStart)
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - plcpTrainingSymbolsStart,
                                                mcsHeaderMode, txVector);

              //Case 3bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  //SIG-A is sent using legacy OFDM modulation
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 3bi - previous is in SIG-A and current is in training or in SIG-B: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3bii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    mcsHeaderMode, txVector);

                  NS_LOG_DEBUG ("Case 3bii - previous is in HT-SIG and current is in HT training: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
          //Case 3c: current with previous in HT-SIG or SIG-A
          else
            {
              //Case 3bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  //SIG-A is sent using legacy OFDM modulation
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 3ci - previous with current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 3bii: HT mixed format or HT greenfield
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - previous,
                                                    mcsHeaderMode, txVector);

                  NS_LOG_DEBUG ("Case 3cii - previous with current in HT-SIG: mode=" << mcsHeaderMode << ", psr=" << psr);
                }
            }
        }
      //Case 4: previous in L-SIG: HT GF will not reach here because it will execute the previous if and exit
      else if (previous >= plcpHeaderStart)
        {
          NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);
          //Case 4a: current after payload start
          if (current >= plcpPayloadStart)
            {
              //Case 4ai: legacy format
              if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 4ai - previous in L-SIG and current after payload start: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4aii: VHT or HE format
              else if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 4aii - previous is in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4aiii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpHsigHeaderStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 4aiii - previous in L-SIG and current after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 4b: current is in training or in SIG-B. legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));

              //Case 4bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 4bi - previous is in L-SIG and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4bii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpHsigHeaderStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 4bii - previous in L-SIG and current in HT training: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 4c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpHsigHeaderStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));

              //Case 4ci: VHT format
              if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 4ci - previous is in L-SIG and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 4cii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpHsigHeaderStart - previous,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 4cii - previous in L-SIG and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 4d: current with previous in L-SIG
          else
            {
              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - previous,
                                                headerMode, txVector);

              NS_LOG_DEBUG ("Case 3c - current with previous in L-SIG: mode=" << headerMode << ", psr=" << psr);
            }
        }
      //Case 5: previous is in the preamble works for all cases
      else
        {
          //Case 5a: current after payload start
          if (current >= plcpPayloadStart)
            {
              //Case 5ai: legacy format (No HT-SIG or Training Symbols)
              if (preamble == WIFI_PREAMBLE_LONG || preamble == WIFI_PREAMBLE_SHORT)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpHeaderStart,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 5a - previous is in the preamble and current is after payload start: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5aii: VHT or HE format
              else if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - plcpHeaderStart,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 5aii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }

              //Case 5aiii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpPayloadStart - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpHsigHeaderStart - plcpHeaderStart, //HT GF: plcpHsigHeaderStart - plcpHeaderStart = 0
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 5aiii - previous is in the preamble and current is after payload start: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 5b: current is in training or in SIG-B. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpTrainingSymbolsStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));

              //Case 5bi: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpTrainingSymbolsStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpTrainingSymbolsStart - plcpHeaderStart,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 5bi - previous is in the preamble and current in training or in SIG-B: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5bii: HT mixed format
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpHsigHeaderStart - plcpHeaderStart,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 5bii - previous is in the preamble and current in HT training: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 5c: current in HT-SIG or in SIG-A. Legacy will not come here since it went in previous if or if the previous if is not true this will be not true
          else if (current >= plcpHsigHeaderStart)
            {
              NS_ASSERT ((preamble != WIFI_PREAMBLE_LONG) && (preamble != WIFI_PREAMBLE_SHORT));

              //Case 5ci: VHT or HE format
              if (preamble == WIFI_PREAMBLE_VHT || preamble == WIFI_PREAMBLE_HE_SU)
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHeaderStart,
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 5ci - previous is in preamble and current in SIG-A: mode=" << headerMode << ", psr=" << psr);
                }
              //Case 5cii: HT formats
              else
                {
                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    current - plcpHsigHeaderStart,
                                                    mcsHeaderMode, txVector);

                  psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                                  noiseInterferenceW,
                                                                  txVector.GetChannelWidth ()),
                                                    plcpHsigHeaderStart - plcpHeaderStart, //HT-GF: plcpHsigHeaderStart - plcpHeaderStart = 0
                                                    headerMode, txVector);

                  NS_LOG_DEBUG ("Case 5cii - previous in preamble and current in HT-SIG: mcs mode=" << mcsHeaderMode << ", legacy mode=" << headerMode << ", psr=" << psr);
                }
            }
          //Case 5d: current is in L-SIG. HT-GF will not come here
          else if (current >= plcpHeaderStart)
            {
              NS_ASSERT (preamble != WIFI_PREAMBLE_HT_GF);

              psr *= CalculateChunkSuccessRate (CalculateSnr (powerW,
                                                              noiseInterferenceW,
                                                              txVector.GetChannelWidth ()),
                                                current - plcpHeaderStart,
                                                headerMode, txVector);

              NS_LOG_DEBUG ("Case 5d - previous is in the preamble and current is in L-SIG: mode=" << headerMode << ", psr=" << psr);
            }
        }

      noiseInterferenceW += (*j).GetDelta ();
      previous = (*j).GetTime ();
      j++;
    }

  double per = 1 - psr;
  return per;
}

struct InterferenceHelper::SnrPer
InterferenceHelper::CalculatePlcpPayloadSnrPer (Ptr<InterferenceHelper::Event> event)
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector ().GetChannelWidth ());

  /* calculate the SNIR at the start of the packet and accumulate
   * all SNIR changes in the snir vector.
   */
  double per = CalculatePlcpPayloadPer (event, &ni);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

struct InterferenceHelper::SnrPer
InterferenceHelper::CalculatePlcpHeaderSnrPer (Ptr<InterferenceHelper::Event> event)
{
  NiChanges ni;
  double noiseInterferenceW = CalculateNoiseInterferenceW (event, &ni);
  double snr = CalculateSnr (event->GetRxPowerW (),
                             noiseInterferenceW,
                             event->GetTxVector ().GetChannelWidth ());

  /* calculate the SNIR at the start of the plcp header and accumulate
   * all SNIR changes in the snir vector.
   */
  double per = CalculatePlcpHeaderPer (event, &ni);

  struct SnrPer snrPer;
  snrPer.snr = snr;
  snrPer.per = per;
  return snrPer;
}

void
InterferenceHelper::EraseEvents (void)
{
  m_niChanges.clear ();
  m_rxing = false;
  m_firstPower = 0.0;
}

InterferenceHelper::NiChanges::const_iterator
InterferenceHelper::GetPosition (Time moment)
{
  return std::upper_bound (m_niChanges.begin (), m_niChanges.end (), NiChange (moment, 0, NULL));
}

void
InterferenceHelper::AddNiChangeEvent (NiChange change)
{
  m_niChanges.insert (GetPosition (change.GetTime ()), change);
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
}

} //namespace ns3
