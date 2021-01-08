/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
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
#include "wifi-default-ack-manager.h"
#include "wifi-tx-parameters.h"
#include "wifi-mac-queue-item.h"
#include "qos-utils.h"
#include "wifi-mac-queue.h"
#include "wifi-protection.h"
#include "regular-wifi-mac.h"
#include "ctrl-headers.h"
#include "wifi-phy.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("WifiDefaultAckManager");

NS_OBJECT_ENSURE_REGISTERED (WifiDefaultAckManager);

TypeId
WifiDefaultAckManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiDefaultAckManager")
    .SetParent<WifiAckManager> ()
    .SetGroupName ("Wifi")
    .AddConstructor<WifiDefaultAckManager> ()
    .AddAttribute ("UseExplicitBar",
                   "Specify whether to send Block Ack Requests (if true) or use"
                   " Implicit Block Ack Request ack policy (if false).",
                   BooleanValue (false),
                   MakeBooleanAccessor (&WifiDefaultAckManager::m_useExplicitBar),
                   MakeBooleanChecker ())
    .AddAttribute ("BaThreshold",
                   "Immediate acknowledgment is requested upon transmission of a frame "
                   "whose sequence number is distant at least BaThreshold multiplied "
                   "by the transmit window size from the starting sequence number of "
                   "the transmit window. Set to zero to request a response for every "
                   "transmitted frame.",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&WifiDefaultAckManager::m_baThreshold),
                   MakeDoubleChecker<double> (0.0, 1.0))
  ;
  return tid;
}

WifiDefaultAckManager::WifiDefaultAckManager ()
{
  NS_LOG_FUNCTION (this);
}

WifiDefaultAckManager::~WifiDefaultAckManager ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

uint16_t
WifiDefaultAckManager::GetMaxDistFromStartingSeq (Ptr<const WifiMacQueueItem> mpdu,
                                                  const WifiTxParameters& txParams) const
{
  NS_LOG_FUNCTION (this << *mpdu << &txParams);

  const WifiMacHeader& hdr = mpdu->GetHeader ();
  Mac48Address receiver = hdr.GetAddr1 ();

  uint8_t tid = hdr.GetQosTid ();
  Ptr<QosTxop> edca = m_mac->GetQosTxop (tid);
  NS_ABORT_MSG_IF (!edca->GetBaAgreementEstablished (receiver, tid),
                   "An established Block Ack agreement is required");

  uint16_t startingSeq = edca->GetBaStartingSequence (receiver, tid);
  uint16_t maxDistFromStartingSeq = (mpdu->GetHeader ().GetSequenceNumber () - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE;
  NS_ABORT_MSG_IF (maxDistFromStartingSeq >= SEQNO_SPACE_HALF_SIZE,
                   "The given QoS data frame is too old");

  const WifiTxParameters::PsduInfo* psduInfo = txParams.GetPsduInfo (receiver);

  if (psduInfo == nullptr || psduInfo->seqNumbers.find (tid) == psduInfo->seqNumbers.end ())
    {
      // there are no aggregated MPDUs (so far)
      return maxDistFromStartingSeq;
    }

  for (const auto& seqNumber : psduInfo->seqNumbers.at (tid))
    {
      if (!QosUtilsIsOldPacket (startingSeq, seqNumber))
        {
          uint16_t currDistToStartingSeq = (seqNumber - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE;

          if (currDistToStartingSeq > maxDistFromStartingSeq)
            {
              maxDistFromStartingSeq = currDistToStartingSeq;
            }
        }
    }

  NS_LOG_DEBUG ("Returning " << maxDistFromStartingSeq);
  return maxDistFromStartingSeq;
}

bool
WifiDefaultAckManager::IsResponseNeeded (Ptr<const WifiMacQueueItem> mpdu,
                                         const WifiTxParameters& txParams) const
{
  NS_LOG_FUNCTION (this << *mpdu << &txParams);

  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
  Mac48Address receiver = mpdu->GetHeader ().GetAddr1 ();
  Ptr<QosTxop> edca = m_mac->GetQosTxop (tid);

  // An immediate response (Ack or Block Ack) is needed if any of the following holds:
  // * the maximum distance between the sequence number of an MPDU to transmit
  //   and the starting sequence number of the transmit window is greater than
  //   or equal to the window size multiplied by the BaThreshold
  // * no other frame belonging to this BA agreement is queued (because, in such
  //   a case, a Block Ack is not going to be requested anytime soon)
  // * this is the initial frame of a transmission opportunity and it is not
  //   protected by RTS/CTS (see Annex G.3 of IEEE 802.11-2016)
  if (m_baThreshold > 0
      && GetMaxDistFromStartingSeq (mpdu, txParams) < m_baThreshold * edca->GetBaBufferSize (receiver, tid)
      && (edca->GetWifiMacQueue ()->GetNPackets (tid, receiver)
         + edca->GetBaManager ()->GetRetransmitQueue ()->GetNPackets (tid, receiver) > 1)
      && !(edca->GetTxopLimit ().IsStrictlyPositive ()
           && edca->GetRemainingTxop () == edca->GetTxopLimit ()
           && !(txParams.m_protection && txParams.m_protection->method == WifiProtection::RTS_CTS)))
    {
      return false;
    }

  return true;
}

std::unique_ptr<WifiAcknowledgment>
WifiDefaultAckManager::TryAddMpdu (Ptr<const WifiMacQueueItem> mpdu,
                                   const WifiTxParameters& txParams)
{
  NS_LOG_FUNCTION (this << *mpdu << &txParams);

  const WifiMacHeader& hdr = mpdu->GetHeader ();
  Mac48Address receiver = hdr.GetAddr1 ();

  // if the current protection method (if any) is already BLOCK_ACK or BAR_BLOCK_ACK,
  // it will not change by adding an MPDU
  if (txParams.m_acknowledgment
      && (txParams.m_acknowledgment->method == WifiAcknowledgment::BLOCK_ACK
          || txParams.m_acknowledgment->method == WifiAcknowledgment::BAR_BLOCK_ACK))
    {
      return nullptr;
    }

  if (receiver.IsGroup ())
    {
      NS_ABORT_MSG_IF (txParams.GetSize (receiver) > 0,
                       "Unicast frames only can be aggregated");
      WifiNoAck* acknowledgment = new WifiNoAck;
      if (hdr.IsQosData ())
        {
          acknowledgment->SetQosAckPolicy (receiver, hdr.GetQosTid (),
                                           WifiMacHeader::NO_ACK);
        }
      return std::unique_ptr<WifiAcknowledgment> (acknowledgment);
    }

  if ((!hdr.IsQosData ()
       || !m_mac->GetQosTxop (hdr.GetQosTid ())->GetBaAgreementEstablished (receiver, hdr.GetQosTid ()))
      && !hdr.IsBlockAckReq ())
    {
      NS_LOG_DEBUG ("Non-QoS data frame or Block Ack agreement not established, request Normal Ack");
      WifiNormalAck* acknowledgment = new WifiNormalAck;
      acknowledgment->ackTxVector = m_mac->GetWifiRemoteStationManager ()->GetAckTxVector (receiver, txParams.m_txVector);
      if (hdr.IsQosData ())
        {
          acknowledgment->SetQosAckPolicy (receiver, hdr.GetQosTid (), WifiMacHeader::NORMAL_ACK);
        }
      return std::unique_ptr<WifiAcknowledgment> (acknowledgment);
    }

  // we get here if mpdu is a QoS data frame related to an established Block Ack agreement
  // or mpdu is a BlockAckReq frame
  if (!hdr.IsBlockAckReq () && !IsResponseNeeded (mpdu, txParams))
    {
      NS_LOG_DEBUG ("A response is not needed: no ack for now, use Block Ack policy");
      if (txParams.m_acknowledgment
          && txParams.m_acknowledgment->method == WifiAcknowledgment::NONE)
        {
          // no change if the ack method is already NONE
          return nullptr;
        }

      WifiNoAck* acknowledgment = new WifiNoAck;
      if (hdr.IsQosData ())
        {
          acknowledgment->SetQosAckPolicy (receiver, hdr.GetQosTid (),
                                           WifiMacHeader::BLOCK_ACK);
        }
      return std::unique_ptr<WifiAcknowledgment> (acknowledgment);
    }

  // we get here if a response is needed
  uint8_t tid = GetTid (mpdu->GetPacket (), hdr);
  if (!hdr.IsBlockAckReq ()
      && txParams.GetSize (receiver) == 0
      && hdr.GetSequenceNumber ()
         == m_mac->GetQosTxop (tid)->GetBaStartingSequence (receiver, tid))
    {
      NS_LOG_DEBUG ("Sending a single MPDU, no previous frame to ack: request Normal Ack");
      WifiNormalAck* acknowledgment = new WifiNormalAck;
      acknowledgment->ackTxVector = m_mac->GetWifiRemoteStationManager ()->GetAckTxVector (receiver, txParams.m_txVector);
      acknowledgment->SetQosAckPolicy (receiver, tid, WifiMacHeader::NORMAL_ACK);
      return std::unique_ptr<WifiAcknowledgment> (acknowledgment);
    }

  // we get here if multiple MPDUs are being/have been sent
  if (!hdr.IsBlockAckReq ()
      && (txParams.GetSize (receiver) == 0 || m_useExplicitBar))
    {
      // in case of single MPDU, there are previous unacknowledged frames, thus
      // we cannot use Implicit Block Ack Request policy, otherwise we get a
      // normal ack as response
      NS_LOG_DEBUG ("Request to schedule a Block Ack Request");

      WifiBarBlockAck* acknowledgment = new WifiBarBlockAck;
      acknowledgment->blockAckReqTxVector = m_mac->GetWifiRemoteStationManager ()->GetBlockAckTxVector (receiver, txParams.m_txVector);
      acknowledgment->blockAckTxVector = acknowledgment->blockAckReqTxVector;
      acknowledgment->barType = m_mac->GetQosTxop (tid)->GetBlockAckReqType (receiver, tid);
      acknowledgment->baType = m_mac->GetQosTxop (tid)->GetBlockAckType (receiver, tid);
      acknowledgment->SetQosAckPolicy (receiver, tid, WifiMacHeader::BLOCK_ACK);
      return std::unique_ptr<WifiAcknowledgment> (acknowledgment);
    }

  NS_LOG_DEBUG ("A-MPDU using Implicit Block Ack Request policy or BlockAckReq, request Block Ack");
  WifiBlockAck* acknowledgment = new WifiBlockAck;
  acknowledgment->blockAckTxVector = m_mac->GetWifiRemoteStationManager ()->GetBlockAckTxVector (receiver, txParams.m_txVector);
  acknowledgment->baType = m_mac->GetQosTxop (tid)->GetBlockAckType (receiver, tid);
  acknowledgment->SetQosAckPolicy (receiver, tid, WifiMacHeader::NORMAL_ACK);
  return std::unique_ptr<WifiAcknowledgment> (acknowledgment);
}

std::unique_ptr<WifiAcknowledgment>
WifiDefaultAckManager::TryAggregateMsdu (Ptr<const WifiMacQueueItem> msdu,
                                         const WifiTxParameters& txParams)
{
  NS_LOG_FUNCTION (this << *msdu << &txParams);

  // Aggregating an MSDU does not change the acknowledgment method
  return nullptr;
}

} //namespace ns3
