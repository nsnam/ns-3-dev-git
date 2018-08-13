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
#ifndef MPTCP_FULLMESH_H
#define MPTCP_FULLMESH_H

#include "ns3/mptcp-socket-base.h"
#include "ns3/ipv4-header.h"
#include "ns3/object.h"

namespace ns3 {

/**
 * \ingroup socket
 * \ingroup mptcp
 *
 * \brief A base class for implementation of a MPTCP Fullmesh Path Manager.
 *
 * This class contains the functionality to create mesh of subflows between
 * sender and receiver. This class makes calls to add addresses of the local 
 * host to the container. Makes call to the function CreateSubflowsForMesh
 * from MPTCP socket base to create subflows
 */
class MpTcpFullMesh : public Object
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

  MpTcpFullMesh (void);
  virtual ~MpTcpFullMesh (void);
  /**
    * Creates the mesh of subflows between the sender and receiver
    * based on the available IP addresses
    * \param meta the pointer to the MpTcpSocketBase
    */

  virtual void CreateMesh(Ptr<MpTcpSocketBase> meta);

};

} //namespace ns3
#endif //MPTCP_FULLMESH_H
