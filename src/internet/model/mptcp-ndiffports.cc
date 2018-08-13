
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

#include "ns3/mptcp-ndiffports.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/tcp-l4-protocol.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/ipv4-header.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MpTcpNdiffPorts");

NS_OBJECT_ENSURE_REGISTERED (MpTcpNdiffPorts);

TypeId
MpTcpNdiffPorts::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns::MpTcpNdiffPorts")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddConstructor<MpTcpNdiffPorts> ()
    .AddAttribute ("MaxSubflows", "Maximum number of sub-flows per each mptcp connection",
                  UintegerValue (4),
                   MakeUintegerAccessor (&MpTcpNdiffPorts::m_maxSubflows),
                   MakeUintegerChecker<uint8_t> ())

  ;
  return tid;
}

MpTcpNdiffPorts::MpTcpNdiffPorts (void)
  : Object()
{
  NS_LOG_FUNCTION (this);
}

MpTcpNdiffPorts::~MpTcpNdiffPorts (void)
{
  NS_LOG_FUNCTION (this);
}

TypeId
MpTcpNdiffPorts::GetInstanceTypeId () const
{
  return MpTcpNdiffPorts::GetTypeId ();
}

void
MpTcpNdiffPorts::CreateSubflows (Ptr<MpTcpSocketBase> meta, uint16_t localport, uint16_t remoteport)
{ 
  NS_LOG_FUNCTION(this<< meta << localport<< remoteport);

  uint16_t port1 = meta->m_endPoint->GetLocalPort();
  uint16_t port2 = meta->m_endPoint->GetPeerPort ();
  std::cout<<"port1 and port2 "<<port1<<":"<<port2<<" localport and remoteport "<<localport<<":"<<remoteport<<std::endl;
  for (int i = 0; i < m_maxSubflows; i++)
    {
      uint16_t randomSourcePort = rand() % 65000;
      uint16_t randomDestinationPort = rand() % 65000;

      if (randomSourcePort == localport)
      {
        NS_LOG_UNCOND("Generated random port is the same as meta subflow's local port, increment by +1");
        randomSourcePort++;
      }
   meta->CreateSingleSubflow(randomSourcePort, randomDestinationPort); 
    }
}

} //namespace ns3
