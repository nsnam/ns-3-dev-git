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
 */

#include "ns3/mptcp-scheduler-round-robin.h"
#include "ns3/mptcp-subflow.h"
#include "ns3/mptcp-socket-base.h"
#include "ns3/log.h"

NS_LOG_COMPONENT_DEFINE("MpTcpSchedulerRoundRobin");

namespace ns3
{

static inline
SequenceNumber64 SEQ32TO64(SequenceNumber32 seq)
{
  return SequenceNumber64( seq.GetValue());
}

TypeId
MpTcpSchedulerRoundRobin::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpTcpSchedulerRoundRobin")
    .SetParent<MpTcpScheduler> ()
    .AddConstructor<MpTcpSchedulerRoundRobin> ()
  ;
  return tid;
}

MpTcpSchedulerRoundRobin::MpTcpSchedulerRoundRobin() :
  MpTcpScheduler(),
  m_lastUsedFlowId(0),
  m_metaSock(0)
{
  NS_LOG_FUNCTION(this);
}

MpTcpSchedulerRoundRobin::~MpTcpSchedulerRoundRobin (void)
{
  NS_LOG_FUNCTION(this);
}

void
MpTcpSchedulerRoundRobin::SetMeta(Ptr<MpTcpSocketBase> metaSock)
{
  NS_ASSERT(metaSock);
  NS_ASSERT_MSG(m_metaSock == 0, "SetMeta already called");
  m_metaSock = metaSock;
}

Ptr<MpTcpSubflow>
MpTcpSchedulerRoundRobin::GetSubflowToUseForEmptyPacket()
{
  NS_ASSERT(m_metaSock->GetNActiveSubflows() > 0 );
  return  m_metaSock->GetSubflow(0);
}

/* We assume scheduler can't send data on subflows, so it can just generate mappings */
bool
MpTcpSchedulerRoundRobin::GenerateMapping(int& activeSubflowArrayId, SequenceNumber64& dsn, uint16_t& length)
{
  NS_LOG_FUNCTION(this);
  NS_ASSERT(m_metaSock);

  int nbOfSubflows = m_metaSock->GetNActiveSubflows();
  int attempt = 0;
  uint32_t amountOfDataToSend = 0;

  //! Tx data not sent to subflows yet
  SequenceNumber32 metaNextTxSeq = m_metaSock->Getvalue_nextTxSeq(); 
  amountOfDataToSend = m_metaSock->m_txBuffer->SizeFromSequence( metaNextTxSeq );
  uint32_t metaWindow = m_metaSock->AvailableWindow();

  if(amountOfDataToSend <= 0)
   {
     NS_LOG_DEBUG("Nothing to send from meta");
     return false;
   }

  if(metaWindow <= 0)
   {
     NS_LOG_DEBUG("No meta window available (TODO should be in persist state ?)");
     return false;
   }
  while(attempt < nbOfSubflows)
     {
       attempt++;
       m_lastUsedFlowId = (m_lastUsedFlowId + 1) % nbOfSubflows;
       Ptr<MpTcpSubflow> subflow = m_metaSock->GetSubflow(m_lastUsedFlowId);
       uint32_t subflowWindow = subflow->AvailableWindow();
       uint32_t canSend = std::min( subflowWindow, metaWindow);

       //! Can't send more than SegSize
       //metaWindow en fait on s'en fout du SegSize ?
       if(canSend > 0)
        {
          activeSubflowArrayId = m_lastUsedFlowId;
          dsn = SEQ32TO64(metaNextTxSeq);
          canSend = std::min(canSend, amountOfDataToSend);
          // For now we limit ourselves to a per packet basis
          length = std::min(canSend, subflow->GetSegSize());
          return true;
        }
     }
  return false;
}

} // end of 'ns3'
