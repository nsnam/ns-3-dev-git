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
#include "ofdm-phy.h"
#include "ofdm-ppdu.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("OfdmPpdu");

OfdmPpdu::OfdmPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector,
                    WifiPhyBand band, uint64_t uid,
                    bool instantiateLSig /* = true */)
  : WifiPpdu (psdu, txVector, uid),
    m_band (band),
    m_channelWidth (txVector.GetChannelWidth ())
{
  NS_LOG_FUNCTION (this << psdu << txVector << band << uid);
  if (instantiateLSig)
    {
      m_lSig.SetRate (txVector.GetMode ().GetDataRate (txVector), m_channelWidth);
      m_lSig.SetLength (psdu->GetSize ());
    }
}

OfdmPpdu::~OfdmPpdu ()
{
}

WifiTxVector
OfdmPpdu::DoGetTxVector (void) const
{
  WifiTxVector txVector;
  txVector.SetPreambleType (m_preamble);
  //OFDM uses 20 MHz, unless PHY channel width is 5 MHz or 10 MHz
  uint16_t channelWidth = m_channelWidth < 20 ? m_channelWidth : 20;
  txVector.SetMode (OfdmPhy::GetOfdmRate (m_lSig.GetRate (m_channelWidth), channelWidth));
  txVector.SetChannelWidth (channelWidth);
  return txVector;
}

Time
OfdmPpdu::GetTxDuration (void) const
{
  Time ppduDuration = Seconds (0);
  WifiTxVector txVector = GetTxVector ();
  ppduDuration = WifiPhy::CalculateTxDuration (m_lSig.GetLength (), txVector, m_band);
  return ppduDuration;
}

Ptr<WifiPpdu>
OfdmPpdu::Copy (void) const
{
  return Create<OfdmPpdu> (GetPsdu (), GetTxVector (), m_band, m_uid);
}

} //namespace ns3
