
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

#include "ns3/mptcp-fullmesh.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/ipv4-header.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MpTcpFullMesh");

NS_OBJECT_ENSURE_REGISTERED (MpTcpFullMesh);

TypeId
MpTcpFullMesh::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns::MpTcpFullMesh")
    .SetParent<Object> ()
    .AddConstructor<MpTcpFullMesh> ()
    .SetGroupName ("Internet")

  ;
  return tid;
}

MpTcpFullMesh::MpTcpFullMesh (void)
  : Object()
{
  NS_LOG_FUNCTION (this);
}

MpTcpFullMesh::~MpTcpFullMesh (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId
MpTcpFullMesh::GetInstanceTypeId () const
{
  return MpTcpFullMesh::GetTypeId ();
}

void
MpTcpFullMesh::CreateMesh(Ptr<MpTcpSocketBase> meta)
{
  NS_LOG_FUNCTION(this<<meta);

  meta->AddLocalAddresses(); // Add local addresses to vector LocalAddressInfo;
  meta->CreateSubflowsForMesh();
}

} //namespace ns3
