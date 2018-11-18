/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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

#include "tx-vector-tag.h"

/***************************************************************
 *           Packet Mode Tagger
 ***************************************************************/

namespace ns3 {

HighLatencyDataTxVectorTag::HighLatencyDataTxVectorTag ()
{
}

HighLatencyDataTxVectorTag::HighLatencyDataTxVectorTag (WifiTxVector dataTxVector)
  : m_dataTxVector (dataTxVector)
{
}

WifiTxVector
HighLatencyDataTxVectorTag::GetDataTxVector (void) const
{
  return m_dataTxVector;
}

TypeId
HighLatencyDataTxVectorTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HighLatencyDataTxVectorTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HighLatencyDataTxVectorTag> ()
  ;
  return tid;
}

TypeId
HighLatencyDataTxVectorTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
HighLatencyDataTxVectorTag::GetSerializedSize (void) const
{
  return sizeof (WifiTxVector);
}

void
HighLatencyDataTxVectorTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_dataTxVector, sizeof (WifiTxVector));
}

void
HighLatencyDataTxVectorTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_dataTxVector, sizeof (WifiTxVector));
}

void
HighLatencyDataTxVectorTag::Print (std::ostream &os) const
{
  os << "Data=" << m_dataTxVector;
}

HighLatencyRtsTxVectorTag::HighLatencyRtsTxVectorTag ()
{
}

HighLatencyRtsTxVectorTag::HighLatencyRtsTxVectorTag (WifiTxVector rtsTxVector)
  : m_rtsTxVector (rtsTxVector)
{
}

WifiTxVector
HighLatencyRtsTxVectorTag::GetRtsTxVector (void) const
{
  return m_rtsTxVector;
}

TypeId
HighLatencyRtsTxVectorTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HighLatencyRtsTxVectorTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HighLatencyRtsTxVectorTag> ()
  ;
  return tid;
}

TypeId
HighLatencyRtsTxVectorTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
HighLatencyRtsTxVectorTag::GetSerializedSize (void) const
{
  return sizeof (WifiTxVector);
}

void
HighLatencyRtsTxVectorTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_rtsTxVector, sizeof (WifiTxVector));
}

void
HighLatencyRtsTxVectorTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_rtsTxVector, sizeof (WifiTxVector));
}

void
HighLatencyRtsTxVectorTag::Print (std::ostream &os) const
{
  os << "Rts=" << m_rtsTxVector;
}

HighLatencyCtsToSelfTxVectorTag::HighLatencyCtsToSelfTxVectorTag ()
{
}

HighLatencyCtsToSelfTxVectorTag::HighLatencyCtsToSelfTxVectorTag (WifiTxVector ctsToSelfTxVector)
  : m_ctsToSelfTxVector (ctsToSelfTxVector)
{
}

WifiTxVector
HighLatencyCtsToSelfTxVectorTag::GetCtsToSelfTxVector (void) const
{
  return m_ctsToSelfTxVector;
}

TypeId
HighLatencyCtsToSelfTxVectorTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HighLatencyCtsToSelfTxVectorTag")
    .SetParent<Tag> ()
    .SetGroupName ("Wifi")
    .AddConstructor<HighLatencyCtsToSelfTxVectorTag> ()
  ;
  return tid;
}

TypeId
HighLatencyCtsToSelfTxVectorTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
HighLatencyCtsToSelfTxVectorTag::GetSerializedSize (void) const
{
  return sizeof (WifiTxVector);
}

void
HighLatencyCtsToSelfTxVectorTag::Serialize (TagBuffer i) const
{
  i.Write ((uint8_t *)&m_ctsToSelfTxVector, sizeof (WifiTxVector));
}

void
HighLatencyCtsToSelfTxVectorTag::Deserialize (TagBuffer i)
{
  i.Read ((uint8_t *)&m_ctsToSelfTxVector, sizeof (WifiTxVector));
}

void
HighLatencyCtsToSelfTxVectorTag::Print (std::ostream &os) const
{
  os << "Cts To Self=" << m_ctsToSelfTxVector;
}

} //namespace ns3
