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
#ifndef MPTCP_MAPPING_H
#define MPTCP_MAPPING_H

#include <stdint.h>
#include <vector>
#include <queue>
#include <list>
#include <set>
#include <map>
#include "ns3/object.h"
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/sequence-number.h"
#include "ns3/rtt-estimator.h"
#include "ns3/event-id.h"
#include "ns3/packet.h"
#include "ns3/tcp-socket.h"
#include "ns3/ipv4-address.h"
#include "ns3/tcp-tx-buffer.h"
#include "ns3/tcp-rx-buffer.h"

namespace ns3
{
/**
 *\brief A base class for implementation of mapping for mptcp.
 *
 * This class contains the functionality for sequence number mapping.
 *
 * This class maps TCP sequence numbers to subflow level sequnce number (SSN)
 * and connection level sequence numbers (DSN)
 * DSN=Data Sequence Number (mptcp option level)
 * SSN=Subflow Sequence Number (TCP legacy seq nb)
 */

class MpTcpMapping
{
public:
  MpTcpMapping(void);
  virtual ~MpTcpMapping(void);

  /**
   * \brief Set subflow sequence number
   * \param headSSN
   */
  void MapToSSN( SequenceNumber32 const& headSSN);

  /**
   * \return True if mappings share DSN space
   * \param headSSN head SSN
   * \param len 
   */

  virtual bool
  OverlapRangeSSN(const SequenceNumber32& headSSN, const uint16_t& len) const;

  virtual bool
  OverlapRangeDSN(const SequenceNumber64& headDSN, const uint16_t& len) const;
  void SetHeadDSN(SequenceNumber64 const&);

  /**
   * \brief Set mapping length
   */
  virtual void
  SetMappingSize(uint16_t const&);

  /**
   * \param ssn Data seqNb
   */
  bool IsSSNInRange(SequenceNumber32 const& ssn) const;

  /**
   * \param dsn Data seqNb
   */
  bool IsDSNInRange(SequenceNumber64 const& dsn) const;

  /**
   * \param ssn Subflow sequence number
   * \param dsn Data Sequence Number
   * \return True if ssn belonged to this mapping, then a dsn would have been computed
   *
   */
  bool
  TranslateSSNToDSN(const SequenceNumber32& ssn, SequenceNumber64& dsn) const;

  /**
   * \return The last MPTCP sequence number included in the mapping
   */
  SequenceNumber64 TailDSN (void) const;

  /**
   * \return The last subflow sequence number included in the mapping
   */
  SequenceNumber32 TailSSN (void) const;

  /**
   * \brief Necessary for
   * \brief std::set to sort mappings
   * Compares ssn
   * \brief Compares mapping based on their DSN number. It is required when inserting into a set
   */
  bool operator<(MpTcpMapping const& ) const;

  /**
   * \return MPTCP sequence number for the first mapped byte
   */
  virtual SequenceNumber64
  HeadDSN() const;

  /**
   * \return subflow sequence number for the first mapped byte
   */
  virtual SequenceNumber32
  HeadSSN() const;

  /**
   * \return mapping length
   */
  virtual uint16_t
  GetLength() const ;

  /**
   * \brief Mapping are equal if everything concord, SSN/DSN and length
   */
  virtual bool operator==( const MpTcpMapping&) const;

  /**
   * \return Not ==
   */
  virtual bool operator!=( const MpTcpMapping& mapping) const;

protected:
  SequenceNumber64 m_dataSequenceNumber;   //!< MPTCP sequence number
  SequenceNumber32 m_subflowSequenceNumber;  //!< subflow sequence number
  uint16_t m_dataLevelLength;  //!< mapping length / size
};

/**
 * \brief Depending on modifications allowed in upstream ns3, it may some day inherit from TcpTxbuffer etc ...
 * \brief Meanwhile we have a pointer towards the buffers.
 *
 * \class MpTcpMappingContainer
 * Mapping handling
 * Once a mapping has been advertised on a subflow, it must be honored. If the remote host already received the data
 * (because it was sent in parallel over another subflow), then the received data must be discarded.
 * Could be fun implemented as an interval tree
 * http://www.geeksforgeeks.org/interval-tree/
 */

class MpTcpMappingContainer
{
public:
  MpTcpMappingContainer(void);
  virtual ~MpTcpMappingContainer(void);

  /**
   * \brief Discard mappings which TailDSN() < maxDsn and TailSSN() < maxSSN
   *
   * This can be called only when dsn is in the meta socket Rx buffer and in order
   * (since it may renegate some data when out of order).
   * The mapping should also have been thoroughly fulfilled at the subflow level.
   * \return Number of mappings discarded. >= 0
   */

  /**
   * \brief When Buffers work in non renegotiable mode,
   * it should be possible to remove them one by one
   * \param mapping to be discarded
   */
  bool DiscardMapping(const MpTcpMapping& mapping);

  /**
   * \param firstUnmappedSsn last mapped SSN.
   * \return true if non empty
   */
  bool FirstUnmappedSSN(SequenceNumber32& firstUnmappedSsn) const;

  /**
   * \brief For debug purpose. Dump all registered mappings
   */
  virtual void Dump() const;

  /**
   * \brief
   * Should do no check
   * The mapping
   * \note Check for overlap.
   * \return False if the dsn range overlaps with a registered mapping, true otherwise
  **/
  bool AddMapping(const MpTcpMapping& mapping);

  /**
   * \param l list
   * \param m pass on the mapping you want to retrieve
   */
  bool
  GetMappingForSSN(const SequenceNumber32& ssn, MpTcpMapping& m) const;

  /**
   * \param dsn
   */
  virtual bool GetMappingsStartingFromSSN(SequenceNumber32 ssn, std::set<MpTcpMapping>& mappings);

protected:

    std::set<MpTcpMapping> m_mappings;     //!< it is a set ordered by SSN
};

std::ostream& operator<<(std::ostream &os, const MpTcpMapping& mapping);

} //namespace ns3
#endif //MP_TCP_TYPEDEFS_H
