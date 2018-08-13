/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015 University of Sussex
 * Copyright (c) 2015 Université Pierre et Marie Curie (UPMC)
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
 * Author:  Kashif Nadeem <kshfnadeem@gmail.com>
 *          Matthieu Coudron <matthieu.coudron@lip6.fr>
 *          Morteza Kheirkhah <m.kheirkhah@sussex.ac.uk>
 *
 */
#include <iostream>
#include <set>
#include <iterator>
#include <algorithm>
#include "ns3/mptcp-mapping.h"
#include "ns3/simulator.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("MpTcpMapping");

namespace ns3
{

// brief sorted on DSNs
typedef std::set<MpTcpMapping> MappingList;

MpTcpMapping::MpTcpMapping()
  : m_dataSequenceNumber(0),
    m_subflowSequenceNumber(0),
    m_dataLevelLength(0)
{
  NS_LOG_FUNCTION(this);
}

MpTcpMapping::~MpTcpMapping(void)
{
  NS_LOG_FUNCTION(this);
};

void
MpTcpMapping::SetMappingSize(uint16_t const& length)
{
  NS_LOG_DEBUG(this << " length=" << length);
  m_dataLevelLength = length;
}

bool
MpTcpMapping::TranslateSSNToDSN(const SequenceNumber32& ssn, SequenceNumber64& dsn) const
{
  if(IsSSNInRange(ssn))
  {
    dsn = SequenceNumber64(ssn - HeadSSN()) + HeadDSN();
    return true;
  }

  return false;
}

std::ostream&
operator<<(std::ostream& os, const MpTcpMapping& mapping)
{
  os << "DSN [" << mapping.HeadDSN() << "-" << mapping.TailDSN ()
    << "] mapped to SSN [" <<  mapping.HeadSSN() << "-" <<  mapping.TailSSN() << "]";
  return os;
}

void
MpTcpMapping::SetHeadDSN(SequenceNumber64 const& dsn)
{
  m_dataSequenceNumber = dsn;
}

void
MpTcpMapping::MapToSSN( SequenceNumber32 const& seq)
{
  m_subflowSequenceNumber = seq;
}

bool
MpTcpMapping::operator==(const MpTcpMapping& mapping) const
{
  return (
  GetLength() == mapping.GetLength()
    && HeadDSN() == mapping.HeadDSN()
    && HeadSSN() == mapping.HeadSSN()
    );
}

bool
MpTcpMapping::operator!=( const MpTcpMapping& mapping) const
{
  return !( *this == mapping);
}

SequenceNumber64
MpTcpMapping::HeadDSN() const
{
  return m_dataSequenceNumber;
}

SequenceNumber32
MpTcpMapping::HeadSSN() const
{
  return m_subflowSequenceNumber;
}

uint16_t
MpTcpMapping::GetLength() const
{
  return m_dataLevelLength;
}

SequenceNumber64
MpTcpMapping::TailDSN(void) const
{
  return(HeadDSN()+GetLength()-1);
}

SequenceNumber32
MpTcpMapping::TailSSN(void) const
{
  return(HeadSSN()+GetLength()-1);
}

bool
MpTcpMapping::operator<(MpTcpMapping const& m) const
{
  return (HeadSSN() < m.HeadSSN());
}

bool
MpTcpMapping::IsSSNInRange(SequenceNumber32 const& ssn) const
{
  return ( (HeadSSN() <= ssn) && (TailSSN() >= ssn) );
}

bool
MpTcpMapping::IsDSNInRange(SequenceNumber64 const& dsn) const
{
  return ( (HeadDSN() <= dsn) && (TailDSN() >= dsn) );
}

bool
MpTcpMapping::OverlapRangeSSN(const SequenceNumber32& headSSN, const uint16_t& len) const
{
  SequenceNumber32 tailSSN = headSSN + len-1;

  if( HeadSSN() >  tailSSN || TailSSN() < headSSN)
  {
    return false;
  }
  NS_LOG_DEBUG("SSN overlap");
  return true;
}

bool
MpTcpMapping::OverlapRangeDSN(const SequenceNumber64& headDSN, const uint16_t& len) const
{
  SequenceNumber64 tailDSN = headDSN + len-1;

  if( HeadDSN() >  tailDSN || TailDSN() < headDSN)
  {
    return false;
  }
  NS_LOG_DEBUG("DSN overlap");
  return true;
}

///////////////////////////////////////////////////////////
///// MpTcpMappingContainer
MpTcpMappingContainer::MpTcpMappingContainer(void)
{
  NS_LOG_LOGIC(this);
}

MpTcpMappingContainer::~MpTcpMappingContainer(void)
{
  NS_LOG_LOGIC(this);
}

void
MpTcpMappingContainer::Dump() const
{
  NS_LOG_UNCOND("\n==== Dumping list of mappings ====");
  for( MappingList::const_iterator it = m_mappings.begin(); it != m_mappings.end(); it++ )
  {
    NS_LOG_UNCOND( *it );
  }
  NS_LOG_UNCOND("==== End of dump ====\n");
}

//! should return a boolean
bool
MpTcpMappingContainer::AddMapping(const MpTcpMapping& mapping)
{
  NS_LOG_LOGIC("Adding mapping " << mapping);

  NS_ASSERT(mapping.GetLength() != 0);
  std::pair<MappingList::iterator,bool> res = m_mappings.insert( mapping);

  return res.second;
}

bool
MpTcpMappingContainer::FirstUnmappedSSN(SequenceNumber32& ssn) const
{
  NS_LOG_FUNCTION_NOARGS();
  if(m_mappings.empty())
  {
      return false;
  }
  ssn = m_mappings.rbegin()->TailSSN() + 1;
  return true;
}

bool
MpTcpMappingContainer::DiscardMapping(const MpTcpMapping& mapping)
{
  NS_LOG_LOGIC("discard mapping "<< mapping);
  return m_mappings.erase(mapping);
}

bool
MpTcpMappingContainer::GetMappingsStartingFromSSN(SequenceNumber32 ssn, std::set<MpTcpMapping>& missing)
{
  NS_LOG_FUNCTION(this << ssn );
  missing.clear();
  //http://www.cplusplus.com/reference/algorithm/equal_range/
  MpTcpMapping temp;
  temp.MapToSSN(ssn);
  MappingList::const_iterator it = std::lower_bound( m_mappings.begin(), m_mappings.end(), temp);

  std::copy(it, m_mappings.end(), std::inserter(missing, missing.begin()));
  return false;
}

bool
MpTcpMappingContainer::GetMappingForSSN(const SequenceNumber32& ssn, MpTcpMapping& mapping) const
{
  NS_LOG_FUNCTION(this << ssn);
  if(m_mappings.empty())
    return false;
  MpTcpMapping temp;
  temp.MapToSSN(ssn);

  // Returns the first that is not less
  // upper_bound returns the greater
  MappingList::const_iterator it = std::upper_bound( m_mappings.begin(), m_mappings.end(), temp);
  it--;
  mapping = *it;
  return mapping.IsSSNInRange( ssn );               // IsSSNInRange() return ( (HeadSSN() <= ssn) && (TailSSN() >= ssn) );
}

} // namespace ns3
