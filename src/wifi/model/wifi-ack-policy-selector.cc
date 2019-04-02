/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/log.h"
#include "wifi-ack-policy-selector.h"
#include <set>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiAckPolicySelector");

NS_OBJECT_ENSURE_REGISTERED (WifiAckPolicySelector);

TypeId
WifiAckPolicySelector::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiAckPolicySelector")
    .SetParent<Object> ()
    .SetGroupName("Wifi")
  ;
  return tid;
}

WifiAckPolicySelector::~WifiAckPolicySelector ()
{
  NS_LOG_FUNCTION (this);
}

void
WifiAckPolicySelector::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  m_qosTxop = 0;
}

void
WifiAckPolicySelector::SetQosTxop (Ptr<QosTxop> qosTxop)
{
  NS_LOG_FUNCTION (this << qosTxop);
  m_qosTxop = qosTxop;
}

Ptr<QosTxop>
WifiAckPolicySelector::GetQosTxop (void) const
{
  return m_qosTxop;
}

void
WifiAckPolicySelector::SetAckPolicy (Ptr<WifiPsdu> psdu, const MacLowTransmissionParameters & params)
{
  NS_LOG_FUNCTION (psdu << params);

  std::set<uint8_t> tids = psdu->GetTids ();
  NS_ASSERT (tids.size () == 1);
  uint8_t tid = *tids.begin ();

  if (params.MustWaitNormalAck () || params.MustWaitBlockAck ())
    {
      // Normal Ack or Implicit Block Ack Request policy
      psdu->SetAckPolicyForTid (tid, WifiMacHeader::NORMAL_ACK);
    }
  else
    {
      // Block Ack policy
      psdu->SetAckPolicyForTid (tid, WifiMacHeader::BLOCK_ACK);
    }
}

} //namespace ns3
