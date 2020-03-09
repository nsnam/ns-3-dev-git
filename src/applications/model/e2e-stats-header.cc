/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 Natale Patriciello <natale.patriciello@gmail.com>
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
 */

#include "ns3/log.h"
#include "e2e-stats-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SizeHeader");

NS_OBJECT_ENSURE_REGISTERED (E2eStatsHeader);

E2eStatsHeader::E2eStatsHeader ()
  : SeqTsHeader ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
E2eStatsHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SizeHeader")
    .SetParent<SeqTsHeader> ()
    .SetGroupName ("Applications")
    .AddConstructor<E2eStatsHeader> ()
  ;
  return tid;
}

TypeId
E2eStatsHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void
E2eStatsHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this << &os);
  os << "(size=" << m_size << ") AND ";
  SeqTsHeader::Print (os);
}

uint32_t
E2eStatsHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);
  return SeqTsHeader::GetSerializedSize () + 8;
}

void
E2eStatsHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  i.WriteHtonU64 (m_size);
  SeqTsHeader::Serialize (i);
}

uint32_t
E2eStatsHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION (this << &start);
  Buffer::Iterator i = start;
  m_size = i.ReadNtohU64 ();
  SeqTsHeader::Deserialize (i);
  return GetSerializedSize ();
}

} // namespace ns3
