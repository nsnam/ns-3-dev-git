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
  return 3;
}

void
WifiPhyTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (static_cast<uint8_t> (m_preamble));
  i.WriteU8 (static_cast<uint8_t> (m_modulation));
  i.WriteU8 (m_frameComplete);
}

void
WifiPhyTag::Deserialize (TagBuffer i)
{
  m_preamble = static_cast<WifiPreamble> (i.ReadU8 ());
  m_modulation = static_cast<WifiModulationClass> (i.ReadU8 ());
  m_frameComplete = i.ReadU8 ();
}

void
WifiPhyTag::Print (std::ostream &os) const
{
  os << +m_preamble << " " << +m_modulation << " " << m_frameComplete;
}

WifiPhyTag::WifiPhyTag ()
{
}

WifiPhyTag::WifiPhyTag (WifiPreamble preamble, WifiModulationClass modulation, uint8_t frameComplete)
  : m_preamble (preamble),
    m_modulation (modulation),
    m_frameComplete (frameComplete)
{
}

WifiPreamble
WifiPhyTag::GetPreambleType (void) const
{
  return m_preamble;
}

WifiModulationClass
WifiPhyTag::GetModulation (void) const
{
  return m_modulation;
}

uint8_t
WifiPhyTag::GetFrameComplete (void) const
{
  return m_frameComplete;
}

} // namespace ns3
