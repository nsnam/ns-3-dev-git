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
 */

#include "ns3/packet.h"
#include "ns3/log.h"
#include "wifi-ppdu.h"
#include "wifi-psdu.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiPpdu");

WifiPpdu::WifiPpdu (Ptr<const WifiPsdu> psdu, WifiTxVector txVector, Time ppduDuration)
  : m_txVector (txVector),
    m_psdu (psdu),
    m_txDuration (ppduDuration),
    m_truncatedTx (false)
{
}

WifiPpdu::~WifiPpdu ()
{
}

WifiTxVector
WifiPpdu::GetTxVector (void) const
{
  return m_txVector;
}

Ptr<const WifiPsdu>
WifiPpdu::GetPsdu (void) const
{
  return m_psdu;
}

bool
WifiPpdu::IsTruncatedTx (void) const
{
  return m_truncatedTx;
}

void
WifiPpdu::SetTruncatedTx (void)
{
  NS_LOG_FUNCTION (this);
  m_truncatedTx = true;
}

Time
WifiPpdu::GetTxDuration (void) const
{
  return m_txDuration;
}

void
WifiPpdu::Print (std::ostream& os) const
{
  os << "txVector=" << m_txVector
     << ", duration=" << m_txDuration.GetMicroSeconds () << "us"
     << ", truncatedTx=" << (m_truncatedTx ? "Y" : "N")
     << ", PSDU=" << m_psdu;
}

std::ostream & operator << (std::ostream &os, const WifiPpdu &ppdu)
{
  ppdu.Print (os);
  return os;
}

} //namespace ns3
