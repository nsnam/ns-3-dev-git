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
 * Author: Rediet <getachew.redieteab@orange.com>
 *         Muhammad Iqbal Rochman <muhiqbalcr@uchicago.edu>
 */

#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "wifi-utils.h"
#include "ht-phy.h"
#include "ht-ppdu.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HtPpdu");

HtPpdu::HtPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration,
                WifiPhyBand band, uint64_t uid)
  : OfdmPpdu (psdu, txVector, band, uid, false) //don't instantiate LSigHeader of OfdmPpdu
{
  NS_LOG_FUNCTION (this << psdu << txVector << ppduDuration << band << uid);
  uint8_t sigExtension = 0;
  if (m_band == WIFI_PHY_BAND_2_4GHZ)
    {
      sigExtension = 6;
    }
  uint16_t length = ((ceil ((static_cast<double> (ppduDuration.GetNanoSeconds () - (20 * 1000) - (sigExtension * 1000)) / 1000) / 4.0) * 3) - 3);
  m_lSig.SetLength (length);
  m_htSig.SetMcs (txVector.GetMode ().GetMcsValue ());
  m_htSig.SetChannelWidth (m_channelWidth);
  m_htSig.SetHtLength (psdu->GetSize ());
  m_htSig.SetAggregation (txVector.IsAggregation ());
  m_htSig.SetShortGuardInterval (txVector.GetGuardInterval () == 400);
}

HtPpdu::~HtPpdu ()
{
}

WifiTxVector
HtPpdu::DoGetTxVector (void) const
{
  WifiTxVector txVector;
  txVector.SetPreambleType (m_preamble);
  txVector.SetMode (HtPhy::GetHtMcs (m_htSig.GetMcs ()));
  txVector.SetChannelWidth (m_htSig.GetChannelWidth ());
  txVector.SetNss (1 + (m_htSig.GetMcs () / 8));
  txVector.SetGuardInterval(m_htSig.GetShortGuardInterval () ? 400 : 800);
  txVector.SetAggregation (m_htSig.GetAggregation ());
  return txVector;
}

Time
HtPpdu::GetTxDuration (void) const
{
  Time ppduDuration = Seconds (0);
  WifiTxVector txVector = GetTxVector ();
  ppduDuration = WifiPhy::CalculateTxDuration (m_htSig.GetHtLength (), txVector, m_band);
  return ppduDuration;
}

Ptr<WifiPpdu>
HtPpdu::Copy (void) const
{
  return Create<HtPpdu> (GetPsdu (), GetTxVector (), GetTxDuration (), m_band, m_uid);
}

} //namespace ns3
