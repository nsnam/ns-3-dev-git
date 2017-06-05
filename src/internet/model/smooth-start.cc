/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Asavari Limaye <asavari.limaye@gmail.com>  
 *                    Snigdh Manasvi <snigdhmanas@gmail.com>
 *                    Shrinidhi Talpankar <shrinidhitalpankar@gmail.com>
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  U SA
 *
 */
#include "tcp-congestion-ops.h"
#include "tcp-socket-base.h"
#include "ns3/log.h"
#include "smooth-start.h"

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("SmoothStart");

//NS_OBJECT_ENSURE_REGISTERED (SmoothStart);


//Smooth Start

/**
 * \brief SmoothStart, a variant of SlowStart
 *
 * 
 * \param tcb internal congestion state
 * \param segmentsAcked count of segments acked
 * \return the number of segments not considered for increasing the cWnd
 */

NS_OBJECT_ENSURE_REGISTERED (SmoothStart);

TypeId
SmoothStart::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SmoothStart")
    .SetParent<TcpNewReno> ()
    .SetGroupName ("Internet")
    .AddConstructor<SmoothStart> ()
    .AddAttribute ("GrainNumber",
                   "Grain number used for Smooth Start",
                   UintegerValue (2),
                   MakeUintegerAccessor (&SmoothStart::m_grainNumber),
                   MakeUintegerChecker<uint16_t> ()
                   )
    .AddAttribute ("Depth",
                   "Depth used for Smooth Start",
                   UintegerValue (2),
                   MakeUintegerAccessor (&SmoothStart::m_depth),
                   MakeUintegerChecker<uint16_t> ()
                   );
  return tid;
}


SmoothStart::SmoothStart (void) : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
  m_nAcked = 0;
  m_grainNumber = 0;
  m_depth = 0;
  m_ackThresh = 0;
  m_smsThresh = 0;
  m_firstFlag = 0;
 }

SmoothStart::SmoothStart (const SmoothStart& other)
  : TcpNewReno (other)
{
  NS_LOG_FUNCTION (this);
  m_nAcked = other.m_nAcked;
  m_grainNumber = other.m_grainNumber;
  m_depth = other.m_depth; 
  m_ackThresh = other.m_ackThresh;
  m_smsThresh  = other.m_smsThresh;
  m_firstFlag = other.m_firstFlag;
}

SmoothStart::~SmoothStart (void)
{
} 
 
uint32_t
SmoothStart::SlowStart (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);
  m_smsThresh = tcb->m_ssThresh / pow(2,m_depth);  
       
  NS_LOG_INFO ("Before Decision: smsThresh: "<< m_smsThresh << " ssThresh: " << tcb->m_ssThresh << " nAcked: " << m_nAcked << " Ackthresh: "<<m_ackThresh<< " cwnd: "<< tcb->m_cWnd) ;
  
  if (segmentsAcked >= 1)
    {
    
      if (tcb->m_cWnd <= m_smsThresh)
        {
        NS_LOG_INFO("++ Slow Start ++");
        tcb->m_cWnd += tcb->m_segmentSize;
        NS_LOG_INFO ("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
        return segmentsAcked - 1;
        }
        
    m_nAcked += 1;
    
    if (m_nAcked == m_ackThresh)
        {
        NS_LOG_INFO("++ Smooth Start increasing cwnd++");
        tcb->m_cWnd += tcb->m_segmentSize;
        NS_LOG_INFO ("In SmoothStart, updated to cwnd " << tcb->m_cWnd << " ssthresh " << tcb->m_ssThresh);
        m_ackThresh = m_ackThresh + 1;
        m_nAcked = 0;
        return segmentsAcked - 1;
        }
    else NS_LOG_INFO("++ Smooth Start not cwnd++ m_nAcked!=m_ackThresh"<< m_nAcked <<" " <<m_ackThresh);
    return segmentsAcked - 1;    
    }
  return 0;
}


void
SmoothStart::IncreaseWindow (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked);

  if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        NS_LOG_INFO ("Doing SmoothStart") ;
        if (m_firstFlag == 0)
                {
                  m_firstFlag = 1 ;
                  m_ackThresh = m_grainNumber ;           
                }
        segmentsAcked = SlowStart (tcb, segmentsAcked);
    }

  if (tcb->m_cWnd >= tcb->m_ssThresh)
    {
      m_firstFlag = 0;
      NS_LOG_INFO("SmoothStart: Calling Congestion Avoidance");
      CongestionAvoidance (tcb, segmentsAcked);
    }
}

} // namespace ns3

