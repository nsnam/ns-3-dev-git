/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
 * Copyright (c) 2010 Adrian Sai-wah Tam
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
 * Author: Kashif Nadeem <kshfnadeem@gmail.com>
 *
 */

#ifndef MPTCP_NDIFFPORTS_H
#define MPTCP_NDIFFPORTS_H

#include "ns3/mptcp-socket-base.h"
#include "ns3/ipv4-header.h"
#include "ns3/object.h"

namespace ns3 {

class RttEstimator;
class MpTcpSocketBase;
/**
 * \ingroup socket
 * \ingroup mptcp
 *
 * \brief A base class for implementation of a MPTCP ndiffports Path Manager.
 *
 * This class contains the functionality to create subflows between the same
 * pair of IP between sender and receiver. This class makes to CreateSingSubglows
 * from MpTcpSocketBase to create subflows with different source and destination
 * port pairs
 */
class MpTcpNdiffPorts : public Object
{
public:
  /**
   * Get the type ID.
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * \brief Get the instance TypeId
   * \return the instance TypeId
   */
  virtual TypeId GetInstanceTypeId () const;

  MpTcpNdiffPorts (void);

  virtual ~MpTcpNdiffPorts (void);
  /**
   * \brief create subflows between source and destination IP
   * \param meta   MpTcpSocketBase pointer
   * \param localport meta subflow source port
   * \param remoteport meta subflow destination port
   * \param
   */

  void CreateSubflows (Ptr<MpTcpSocketBase> meta, uint16_t localport, uint16_t remoteport);

  uint8_t m_maxSubflows {3} ;  //!< Maximum number of subflows per mptcp connection

};

} //namespace ns3
#endif //MPTCP_NDIFFPORTS_H
