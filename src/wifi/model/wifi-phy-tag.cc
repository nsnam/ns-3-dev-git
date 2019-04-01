/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 CTTC
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
 * Author: Nicola Baldo <nbaldo@cttc.es>
 */

#include "wifi-phy-tag.h"

namespace ns3 {

TypeId
WifiPhyTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiPhyTag")
    .SetParent<Tag> ()
  ;
  return tid;
}

TypeId
WifiPhyTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
WifiPhyTag::GetSerializedSize (void) const
{
  return (sizeof (WifiTxVector) + 1);
}

void
WifiPhyTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_wifiTxVector, sizeof (WifiTxVector));
  i.WriteU8 (m_frameComplete);
}

void
WifiPhyTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_wifiTxVector, sizeof (WifiTxVector));
  m_frameComplete = i.ReadU8 ();
}

void
WifiPhyTag::Print (std::ostream &os) const
{
  os << m_wifiTxVector << " " << m_frameComplete;
}

WifiPhyTag::WifiPhyTag ()
{
}

WifiPhyTag::WifiPhyTag (WifiTxVector txVector, uint8_t frameComplete)
  : m_wifiTxVector (txVector),
    m_frameComplete (frameComplete)
{
}

WifiTxVector
WifiPhyTag::GetWifiTxVector (void) const
{
  return m_wifiTxVector;
}

uint8_t
WifiPhyTag::GetFrameComplete (void) const
{
  return m_frameComplete;
}

} // namespace ns3
