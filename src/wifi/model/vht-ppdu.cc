/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Orange Labs
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
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 */

#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "vht-phy.h"
#include "vht-ppdu.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("VhtPpdu");

VhtPpdu::VhtPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration,
                  WifiPhyBand band, uint64_t uid)
  : OfdmPpdu (psdu, txVector, band, uid, false) //don't instantiate LSigHeader of OfdmPpdu
{
  NS_LOG_FUNCTION (this << psdu << txVector << ppduDuration << band << uid);
  uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000)) / 1000) / 4.0) * 3) - 3);
  m_lSig.SetLength (length);
  m_vhtSig.SetMuFlag (m_preamble == WIFI_PREAMBLE_VHT_MU);
  m_vhtSig.SetChannelWidth (m_channelWidth);
  m_vhtSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
  uint32_t nSymbols = (static_cast<double> ((ppduDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector)).GetNanoSeconds ()) / (3200 + txVector.GetGuardInterval ()));
  if (txVector.GetGuardInterval () == 400)
    {
      m_vhtSig.SetShortGuardIntervalDisambiguation ((nSymbols % 10) == 9);
    }
  m_vhtSig.SetSuMcs (txVector.GetMode ().GetMcsValue ());
  m_vhtSig.SetNStreams (txVector.GetNss ());
}

VhtPpdu::~VhtPpdu ()
{
}

WifiTxVector
VhtPpdu::DoGetTxVector (void) const
{
  WifiTxVector txVector;
  txVector.SetPreambleType (m_preamble);
  txVector.SetMode (VhtPhy::GetVhtMcs (m_vhtSig.GetSuMcs ()));
  txVector.SetChannelWidth (m_vhtSig.GetChannelWidth ());
  txVector.SetNss (m_vhtSig.GetNStreams ());
  txVector.SetGuardInterval (m_vhtSig.GetShortGuardInterval () ? 400 : 800);
  txVector.SetAggregation (GetPsdu ()->IsAggregate ());
  return txVector;
}

Time
VhtPpdu::GetTxDuration (void) const
{
  Time ppduDuration = Seconds (0);
  WifiTxVector txVector = GetTxVector ();
  Time tSymbol = NanoSeconds (3200 + txVector.GetGuardInterval ());
  Time preambleDuration = WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
  Time calculatedDuration = MicroSeconds (((ceil (static_cast<double> (m_lSig.GetLength () + 3) / 3)) * 4) + 20);
  uint32_t nSymbols = floor (static_cast<double> ((calculatedDuration - preambleDuration).GetNanoSeconds ()) / tSymbol.GetNanoSeconds ());
  if (m_vhtSig.GetShortGuardInterval () && m_vhtSig.GetShortGuardIntervalDisambiguation ())
    {
      nSymbols--;
    }
  ppduDuration = preambleDuration + (nSymbols * tSymbol);
  return ppduDuration;
}

Ptr<WifiPpdu>
VhtPpdu::Copy (void) const
{
  return Create<VhtPpdu> (GetPsdu (), GetTxVector (), GetTxDuration (), m_band, m_uid);
}

WifiPpduType
VhtPpdu::GetType (void) const
{
  return (m_preamble == WIFI_PREAMBLE_VHT_MU) ? WIFI_PPDU_TYPE_DL_MU : WIFI_PPDU_TYPE_SU;
}

} //namespace ns3
