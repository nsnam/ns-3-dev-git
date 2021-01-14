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
#include "dsss-phy.h"
#include "dsss-ppdu.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("DsssPpdu");

DsssPpdu::DsssPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration, uint64_t uid)
  : WifiPpdu (psdu, txVector, uid)
{
  NS_LOG_FUNCTION (this << psdu << txVector << ppduDuration << uid);
  m_dsssSig.SetRate (txVector.GetMode ().GetDataRate (22));
  Time psduDuration = ppduDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
  m_dsssSig.SetLength (psduDuration.GetMicroSeconds ());
}

DsssPpdu::~DsssPpdu ()
{
}

WifiTxVector
DsssPpdu::DoGetTxVector (void) const
{
  WifiTxVector txVector;
  txVector.SetPreambleType (m_preamble);
  txVector.SetMode (DsssPhy::GetDsssRate (m_dsssSig.GetRate ()));
  txVector.SetChannelWidth (22);
  return txVector;
}

Time
DsssPpdu::GetTxDuration (void) const
{
  Time ppduDuration = Seconds (0);
  WifiTxVector txVector = GetTxVector ();
  ppduDuration = MicroSeconds (m_dsssSig.GetLength ()) + WifiPhy::CalculatePhyPreambleAndHeaderDuration (txVector);
  return ppduDuration;
}

Ptr<WifiPpdu>
DsssPpdu::Copy (void) const
{
  return Create<DsssPpdu> (GetPsdu (), GetTxVector (), GetTxDuration (), m_uid);
}

} //namespace ns3
