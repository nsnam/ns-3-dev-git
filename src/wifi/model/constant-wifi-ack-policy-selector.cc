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
#include "constant-wifi-ack-policy-selector.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ConstantWifiAckPolicySelector");

NS_OBJECT_ENSURE_REGISTERED (ConstantWifiAckPolicySelector);

TypeId
ConstantWifiAckPolicySelector::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ConstantWifiAckPolicySelector")
    .SetParent<WifiAckPolicySelector> ()
    .AddConstructor<ConstantWifiAckPolicySelector> ()
    .SetGroupName("Wifi")
    .AddAttribute ("UseExplicitBar",
                   "Specify whether to send Block Ack Requests (if true) or use"
                   " Implicit Block Ack Request ack policy (if false).",
                   BooleanValue (false),
                   MakeBooleanAccessor (&ConstantWifiAckPolicySelector::m_useExplicitBar),
                   MakeBooleanChecker ())
    .AddAttribute ("BaThreshold",
                   "Immediate acknowledgment is requested upon transmission of a frame "
                   "whose sequence number is distant at least BaThreshold multiplied "
                   "by the transmit window size from the starting sequence number of "
                   "the transmit window. Set to zero to request a response for every "
                   "transmitted frame.",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&ConstantWifiAckPolicySelector::m_baThreshold),
                   MakeDoubleChecker<double> (0.0, 1.0))
  ;
  return tid;
}

ConstantWifiAckPolicySelector::ConstantWifiAckPolicySelector ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

ConstantWifiAckPolicySelector::~ConstantWifiAckPolicySelector ()
{
  NS_LOG_FUNCTION (this);
}

void
ConstantWifiAckPolicySelector::UpdateTxParams (Ptr<WifiPsdu> psdu, MacLowTransmissionParameters & params)
{
  NS_LOG_FUNCTION (this << psdu << params);

  std::set<uint8_t> tids = psdu->GetTids ();

  if (tids.empty ())
    {
      NS_LOG_DEBUG ("No QoS Data frame in the PSDU");
      return;
    }

  if (tids.size () > 1)
    {
      NS_LOG_DEBUG ("Multi-TID A-MPDUs not supported");
      return;
    }

  Mac48Address receiver = psdu->GetAddr1 ();
  uint8_t tid = *tids.begin ();

  // Use Normal Ack if a BA agreement has not been established
  if (!m_qosTxop->GetBaAgreementEstablished (receiver, tid))
    {
      params.EnableAck ();
      return;
    }

  // If QosTxop forced the use of Block Ack QoS policy, do not make any change
  if (params.MustSendBlockAckRequest ())
    {
      NS_LOG_DEBUG ("Use Block Ack policy as requested");
      return;
    }

  // find the maximum distance from the sequence number of an MPDU included in the
  // PSDU to the starting sequence number of the transmit window.
  uint16_t maxDistToStartingSeq = psdu->GetMaxDistFromStartingSeq (m_qosTxop->GetBaStartingSequence (receiver, tid));

  // An immediate response (Ack or Block Ack) is needed if any of the following holds:
  // * the maximum distance between the sequence number of an MPDU to transmit
  //   and the starting sequence number of the transmit window is greater than
  //   or equal to the window size multiplied by the BaThreshold
  // * no other frame belonging to this BA agreement is queued (because, in such
  //   a case, a Block Ack is not going to be requested any time soon)
  // * this is the initial frame of a transmission opportunity and it is not
  //   protected by RTS/CTS (see Annex G.3 of IEEE 802.11-2016)
  bool isResponseNeeded = (maxDistToStartingSeq >= m_baThreshold * m_qosTxop->GetBaBufferSize (receiver, tid)
                           || m_qosTxop->PeekNextFrame (tid, receiver) == 0
                           || (m_qosTxop->GetTxopLimit ().IsStrictlyPositive ()
                               && m_qosTxop->GetTxopRemaining () == m_qosTxop->GetTxopLimit ()
                               && !params.MustSendRts ()));

  if (!isResponseNeeded)
    {
      NS_LOG_DEBUG ("A response is not needed: no ack for now, use Block Ack policy");
      params.DisableAck ();
      return;
    }
  // An immediate response is needed
  if (maxDistToStartingSeq == 0)
    {
      NS_LOG_DEBUG ("Sending a single MPDU, no previous frame to ack: use Normal Ack policy");
      NS_ASSERT (psdu->GetNMpdus () == 1);
      params.EnableAck ();
      return;
    }
  // Multiple MPDUs are being/have been sent
  if (psdu->GetNMpdus () == 1 || m_useExplicitBar)
    {
      // in case of single MPDU, there are previous unacknowledged frames, thus
      // we cannot use Implicit Block Ack Request policy, otherwise we get a
      // normal ack as response
      if (m_qosTxop->GetBaBufferSize (receiver, tid) > 64)
        {
          NS_LOG_DEBUG ("Scheduling an Extended Compressed block ack request");
          params.EnableBlockAckRequest (BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK);
        }
      else
        {
          NS_LOG_DEBUG ("Scheduling a Compressed block ack request");
          params.EnableBlockAckRequest (BlockAckType::COMPRESSED_BLOCK_ACK);
        }
      return;
    }
  // Implicit Block Ack Request policy
  if (m_qosTxop->GetBaBufferSize (receiver, tid) > 64)
    {
      NS_LOG_DEBUG ("Implicitly requesting an Extended Compressed block ack");
      params.EnableBlockAck (BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK);
    }
  else
    {
      NS_LOG_DEBUG ("Implicitly requesting a Compressed block ack");
      params.EnableBlockAck (BlockAckType::COMPRESSED_BLOCK_ACK);
    }
}

} //namespace ns3
