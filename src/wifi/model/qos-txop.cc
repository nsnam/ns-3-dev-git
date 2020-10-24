/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 *          Stefano Avallone <stavalli@unina.it>
 */

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/random-variable-stream.h"
#include "qos-txop.h"
#include "channel-access-manager.h"
#include "mac-tx-middle.h"
#include "mgt-headers.h"
#include "wifi-mac-trailer.h"
#include "wifi-mac-queue.h"
#include "mac-low.h"
#include "qos-blocked-destinations.h"
#include "wifi-remote-station-manager.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "ctrl-headers.h"
#include "wifi-phy.h"
#include "wifi-ack-policy-selector.h"
#include "wifi-psdu.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_low != 0) { std::clog << "[mac=" << m_low->GetAddress () << "] "; }

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("QosTxop");

NS_OBJECT_ENSURE_REGISTERED (QosTxop);

TypeId
QosTxop::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::QosTxop")
    .SetParent<ns3::Txop> ()
    .SetGroupName ("Wifi")
    .AddConstructor<QosTxop> ()
    .AddAttribute ("UseExplicitBarAfterMissedBlockAck",
                   "Specify whether explicit BlockAckRequest should be sent upon missed BlockAck Response.",
                   BooleanValue (true),
                   MakeBooleanAccessor (&QosTxop::m_useExplicitBarAfterMissedBlockAck),
                   MakeBooleanChecker ())
    .AddAttribute ("AddBaResponseTimeout",
                   "The timeout to wait for ADDBA response after the Ack to "
                   "ADDBA request is received.",
                   TimeValue (MilliSeconds (1)),
                   MakeTimeAccessor (&QosTxop::SetAddBaResponseTimeout,
                                     &QosTxop::GetAddBaResponseTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("FailedAddBaTimeout",
                   "The timeout after a failed BA agreement. During this "
                   "timeout, the originator resumes sending packets using normal "
                   "MPDU. After that, BA agreement is reset and the originator "
                   "will retry BA negotiation.",
                   TimeValue (MilliSeconds (200)),
                   MakeTimeAccessor (&QosTxop::SetFailedAddBaTimeout,
                                     &QosTxop::GetFailedAddBaTimeout),
                   MakeTimeChecker ())
    .AddAttribute ("BlockAckManager",
                   "The BlockAckManager object.",
                   PointerValue (),
                   MakePointerAccessor (&QosTxop::m_baManager),
                   MakePointerChecker<BlockAckManager> ())
    .AddTraceSource ("TxopTrace",
                     "Trace source for TXOP start and duration times",
                     MakeTraceSourceAccessor (&QosTxop::m_txopTrace),
                     "ns3::TracedValueCallback::Time")
  ;
  return tid;
}

QosTxop::QosTxop ()
  : m_typeOfStation (STA),
    m_blockAckType (COMPRESSED_BLOCK_ACK),
    m_startTxop (Seconds (0)),
    m_isAccessRequestedForRts (false),
    m_currentIsFragmented (false)
{
  NS_LOG_FUNCTION (this);
  m_qosBlockedDestinations = Create<QosBlockedDestinations> ();
  m_baManager = CreateObject<BlockAckManager> ();
  m_baManager->SetQueue (m_queue);
  m_baManager->SetBlockAckType (m_blockAckType);
  m_baManager->SetBlockDestinationCallback (MakeCallback (&QosBlockedDestinations::Block, m_qosBlockedDestinations));
  m_baManager->SetUnblockDestinationCallback (MakeCallback (&QosBlockedDestinations::Unblock, m_qosBlockedDestinations));
  m_baManager->SetTxOkCallback (MakeCallback (&QosTxop::BaTxOk, this));
  m_baManager->SetTxFailedCallback (MakeCallback (&QosTxop::BaTxFailed, this));
}

QosTxop::~QosTxop ()
{
  NS_LOG_FUNCTION (this);
}

void
QosTxop::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_ackPolicySelector = 0;
  m_baManager = 0;
  m_qosBlockedDestinations = 0;
  Txop::DoDispose ();
}

bool
QosTxop::GetBaAgreementEstablished (Mac48Address address, uint8_t tid) const
{
  return m_baManager->ExistsAgreementInState (address, tid, OriginatorBlockAckAgreement::ESTABLISHED);
}

uint16_t
QosTxop::GetBaBufferSize (Mac48Address address, uint8_t tid) const
{
  return m_baManager->GetRecipientBufferSize (address, tid);
}

uint16_t
QosTxop::GetBaStartingSequence (Mac48Address address, uint8_t tid) const
{
  return m_baManager->GetOriginatorStartingSequence (address, tid);
}

Ptr<const WifiMacQueueItem>
QosTxop::PrepareBlockAckRequest (Mac48Address recipient, uint8_t tid) const
{
  NS_LOG_FUNCTION (this << recipient << +tid);

  CtrlBAckRequestHeader reqHdr = m_low->GetEdca (tid)->m_baManager->GetBlockAckReqHeader (recipient, tid);
  Ptr<Packet> bar = Create<Packet> ();
  bar->AddHeader (reqHdr);

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKREQ);
  hdr.SetAddr1 (recipient);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();

  return Create<const WifiMacQueueItem> (bar, hdr);
}

void
QosTxop::ScheduleBar (Ptr<const WifiMacQueueItem> bar, bool skipIfNoDataQueued)
{
  m_baManager->ScheduleBar (bar, skipIfNoDataQueued);
}

void
QosTxop::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> remoteManager)
{
  Txop::SetWifiRemoteStationManager (remoteManager);
  NS_LOG_FUNCTION (this << remoteManager);
  m_baManager->SetWifiRemoteStationManager (m_stationManager);
}

void
QosTxop::SetAckPolicySelector (Ptr<WifiAckPolicySelector> ackSelector)
{
  NS_LOG_FUNCTION (this << ackSelector);
  m_ackPolicySelector = ackSelector;
}

Ptr<WifiAckPolicySelector>
QosTxop::GetAckPolicySelector (void) const
{
  return m_ackPolicySelector;
}

void
QosTxop::SetTypeOfStation (TypeOfStation type)
{
  NS_LOG_FUNCTION (this << +type);
  m_typeOfStation = type;
}

TypeOfStation
QosTxop::GetTypeOfStation (void) const
{
  return m_typeOfStation;
}

bool
QosTxop::HasFramesToTransmit (void)
{
  bool ret = (m_currentPacket != 0 || m_baManager->HasPackets () || !m_queue->IsEmpty ());
  NS_LOG_FUNCTION (this << ret);
  return ret;
}

uint16_t
QosTxop::GetNextSequenceNumberFor (const WifiMacHeader *hdr)
{
  return m_txMiddle->GetNextSequenceNumberFor (hdr);
}

uint16_t
QosTxop::PeekNextSequenceNumberFor (const WifiMacHeader *hdr)
{
  return m_txMiddle->PeekNextSequenceNumberFor (hdr);
}

bool
QosTxop::IsQosOldPacket (Ptr<const WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  if (!mpdu->GetHeader ().IsQosData ())
    {
      return false;
    }

  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();
  uint8_t tid = mpdu->GetHeader ().GetQosTid ();

  if (!GetBaAgreementEstablished (recipient, tid))
    {
      return false;
    }

  if (QosUtilsIsOldPacket (GetBaStartingSequence (recipient, tid),
                           mpdu->GetHeader ().GetSequenceNumber ()))
    {
      return true;
    }
  return false;
}

Ptr<const WifiMacQueueItem>
QosTxop::PeekNextFrame (uint8_t tid, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this);
  WifiMacQueue::ConstIterator it = WifiMacQueue::EMPTY;

  // lambda to peek the next frame
  auto peek = [this, &tid, &recipient, &it] (Ptr<WifiMacQueue> queue)
    {
      if (tid == 8 && recipient.IsGroup ())  // undefined TID and recipient
        {
          return queue->PeekFirstAvailable (m_qosBlockedDestinations, it);
        }
      return queue->PeekByTidAndAddress (tid, recipient, it);
    };

  // check if there is a packet in the BlockAckManager retransmit queue
  it = peek (m_baManager->GetRetransmitQueue ());
  // remove old packets
  while (it != m_baManager->GetRetransmitQueue ()->end () && IsQosOldPacket (*it))
    {
      NS_LOG_DEBUG ("removing an old packet from BlockAckManager retransmit queue: " << **it);
      it = m_baManager->GetRetransmitQueue ()->Remove (it);
      it = peek (m_baManager->GetRetransmitQueue ());
    }
  if (it != m_baManager->GetRetransmitQueue ()->end ())
    {
      NS_LOG_DEBUG ("packet peeked from BlockAckManager retransmit queue: " << **it);
      return *it;
    }

  // otherwise, check if there is a packet in the EDCA queue
  it = WifiMacQueue::EMPTY;
  it = peek (m_queue);
  if (it != m_queue->end ())
    {
      // peek the next sequence number and check if it is within the transmit window
      // in case of QoS data frame
      uint16_t sequence = m_txMiddle->PeekNextSequenceNumberFor (&(*it)->GetHeader ());
      if ((*it)->GetHeader ().IsQosData ())
        {
          Mac48Address recipient = (*it)->GetHeader ().GetAddr1 ();
          uint8_t tid = (*it)->GetHeader ().GetQosTid ();

          if (GetBaAgreementEstablished (recipient, tid)
              && !IsInWindow (sequence, GetBaStartingSequence (recipient, tid), GetBaBufferSize (recipient, tid)))
            {
              NS_LOG_DEBUG ("packet beyond the end of the current transmit window");
              return 0;
            }
        }

      WifiMacHeader hdr = (*it)->GetHeader ();
      hdr.SetSequenceNumber (sequence);
      hdr.SetFragmentNumber (0);
      hdr.SetNoMoreFragments ();
      hdr.SetNoRetry ();
      Ptr<const WifiMacQueueItem> item = Create<const WifiMacQueueItem> ((*it)->GetPacket (), hdr, (*it)->GetTimeStamp ());
      NS_LOG_DEBUG ("packet peeked from EDCA queue: " << *item);
      return item;
    }

  return 0;
}

Ptr<WifiMacQueueItem>
QosTxop::DequeuePeekedFrame (Ptr<const WifiMacQueueItem> peekedItem, WifiTxVector txVector,
                             bool aggregate, uint32_t ampduSize, Time ppduDurationLimit)
{
  NS_LOG_FUNCTION (this << peekedItem << txVector << ampduSize << ppduDurationLimit);
  NS_ASSERT (peekedItem != 0);

  // do not dequeue the frame if it is a QoS data frame that does not meet the
  // max A-MPDU size limit (if applicable) or the duration limit (if applicable)
  if (peekedItem->GetHeader ().IsQosData () &&
      !m_low->IsWithinSizeAndTimeLimits (peekedItem, txVector, ampduSize, ppduDurationLimit))
    {
      return 0;
    }

  Mac48Address recipient = peekedItem->GetHeader ().GetAddr1 ();
  Ptr<WifiMacQueueItem> item;
  Ptr<const WifiMacQueueItem> testItem;
  WifiMacQueue::ConstIterator testIt;

  // the packet can only have been peeked from the block ack manager retransmit
  // queue if:
  // - the peeked packet is a QoS Data frame AND
  // - the peeked packet is not a group addressed frame AND
  // - an agreement has been established
  if (peekedItem->GetHeader ().IsQosData () && !recipient.IsGroup ()
      && GetBaAgreementEstablished (recipient, peekedItem->GetHeader ().GetQosTid ()))
    {
      uint8_t tid = peekedItem->GetHeader ().GetQosTid ();
      testIt = m_baManager->GetRetransmitQueue ()->PeekByTidAndAddress (tid, recipient);

      if (testIt != m_baManager->GetRetransmitQueue ()->end ())
        {
          testItem = *testIt;
          // if not null, the test packet must equal the peeked packet
          NS_ASSERT (testItem->GetPacket () == peekedItem->GetPacket ());
          // we should not be asked to dequeue an old packet
          NS_ASSERT (!QosUtilsIsOldPacket (GetBaStartingSequence (recipient, tid),
                                           testItem->GetHeader ().GetSequenceNumber ()));
          item = m_baManager->GetRetransmitQueue ()->Dequeue (testIt);
          NS_LOG_DEBUG ("dequeued from BA manager queue: " << *item);
          return item;
        }
    }

  // The packet has been peeked from the EDCA queue.
  // If it is a QoS Data frame and it is not a broadcast frame, attempt A-MSDU
  // aggregation if aggregate is true
  if (peekedItem->GetHeader ().IsQosData ())
    {
      uint8_t tid = peekedItem->GetHeader ().GetQosTid ();
      testIt = m_queue->PeekByTidAndAddress (tid, recipient);

      NS_ASSERT (testIt != m_queue->end () && (*testIt)->GetPacket () == peekedItem->GetPacket ());

      uint16_t sequence = m_txMiddle->PeekNextSequenceNumberFor (&peekedItem->GetHeader ());

      // check if the peeked packet is within the transmit window
      if (GetBaAgreementEstablished (recipient, tid)
          && !IsInWindow (sequence, GetBaStartingSequence (recipient, tid), GetBaBufferSize (recipient, tid)))
        {
          NS_LOG_DEBUG ("packet beyond the end of the current transmit window");
          return 0;
        }

      // try A-MSDU aggregation
      if (m_low->GetMsduAggregator () != 0 && !recipient.IsGroup () && aggregate)
        {
          item = m_low->GetMsduAggregator ()->GetNextAmsdu (recipient, tid, txVector, ampduSize, ppduDurationLimit);
        }

      if (item != 0)
        {
          NS_LOG_DEBUG ("tx unicast A-MSDU");
        }
      else  // aggregation was not attempted or failed
        {
          item = m_queue->Dequeue (testIt);
        }
    }
  else
    {
      // the peeked packet is a non-QoS Data frame (e.g., a DELBA Request), hence
      // it was not peeked by TID, hence it must be the head of the queue
      item = m_queue->DequeueFirstAvailable (m_qosBlockedDestinations);
      NS_ASSERT (item != 0 && item->GetPacket () == peekedItem->GetPacket ());
    }

  NS_ASSERT (item != 0);

  // Assign a sequence number to the MSDU or A-MSDU dequeued from the EDCA queue
  uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor (&item->GetHeader ());
  item->GetHeader ().SetSequenceNumber (sequence);
  item->GetHeader ().SetFragmentNumber (0);
  item->GetHeader ().SetNoMoreFragments ();
  item->GetHeader ().SetNoRetry ();
  NS_LOG_DEBUG ("dequeued from EDCA queue: " << *item);

  return item;
}

MacLowTransmissionParameters
QosTxop::GetTransmissionParameters (Ptr<const WifiMacQueueItem> frame) const
{
  NS_LOG_FUNCTION (this << *frame);

  MacLowTransmissionParameters params;
  Mac48Address recipient = frame->GetHeader ().GetAddr1 ();

  params.DisableNextData ();

  // group addressed frames
  if (recipient.IsGroup ())
    {
      params.DisableRts ();
      params.DisableAck ();
      return params;
    }
  if (frame->GetHeader ().IsMgt ())
    {
      params.DisableRts ();
      params.EnableAck ();
      return params;
    }

  // Enable/disable RTS
  if (!frame->GetHeader ().IsBlockAckReq ()
      && m_stationManager->NeedRts (frame->GetHeader (), frame->GetSize ())
      && !m_low->IsCfPeriod ())
    {
      params.EnableRts ();
    }
  else
    {
      params.DisableRts ();
    }

  // Select ack technique.
  if (frame->GetHeader ().IsQosData ())
    {
      // Assume normal Ack by default
      params.EnableAck ();
    }
  else if (frame->GetHeader ().IsBlockAckReq ())
    {
      // assume a BlockAck variant. Later, if this frame is not aggregated,
      // the acknowledgment type will be switched to normal Ack
      if (m_blockAckType == BASIC_BLOCK_ACK)
        {
          params.EnableBlockAck (BlockAckType::BASIC_BLOCK_ACK);
        }
      else if (m_blockAckType == COMPRESSED_BLOCK_ACK)
        {
          CtrlBAckRequestHeader baReqHdr;
          frame->GetPacket ()->PeekHeader (baReqHdr);
          uint8_t tid = baReqHdr.GetTidInfo ();

          if (GetBaBufferSize (recipient, tid) > 64)
            {
              params.EnableBlockAck (BlockAckType::EXTENDED_COMPRESSED_BLOCK_ACK);
            }
          else
            {
              params.EnableBlockAck (BlockAckType::COMPRESSED_BLOCK_ACK);
            }
        }
      else if (m_blockAckType == MULTI_TID_BLOCK_ACK)
        {
          NS_FATAL_ERROR ("Multi-tid block ack is not supported");
        }
    }

  return params;
}

void
QosTxop::UpdateCurrentPacket (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  m_currentPacket = mpdu->GetPacket ();
  m_currentHdr = mpdu->GetHeader ();
  m_currentPacketTimestamp = mpdu->GetTimeStamp ();
}

void
QosTxop::NotifyAccessGranted (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_accessRequested);
  m_accessRequested = false;
  m_isAccessRequestedForRts = false;
  m_startTxop = Simulator::Now ();
  // discard the current packet if it is a QoS Data frame with expired lifetime
  if (m_currentPacket != 0 && m_currentHdr.IsQosData ()
      && (m_currentPacketTimestamp + m_queue->GetMaxDelay () < Simulator::Now ()))
    {
      NS_LOG_DEBUG ("the lifetime of current packet expired");
      m_currentPacket = 0;
    }
  // If the current packet is a QoS Data frame, then there must be no block ack agreement
  // established with the receiver for the TID of the packet. Indeed, retransmission
  // of MPDUs sent under a block ack agreement is handled through the retransmit queue.
  NS_ASSERT (m_currentPacket == 0 || !m_currentHdr.IsQosData ()
             || !GetBaAgreementEstablished (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ()));

  if (m_currentPacket == 0)
    {
      Ptr<const WifiMacQueueItem> peekedItem = m_baManager->GetBar ();
      if (peekedItem != 0)
        {
          m_currentHdr = peekedItem->GetHeader ();
          m_currentPacket = peekedItem->GetPacket ();
          m_currentPacketTimestamp = Simulator::Now ();
        }
      else
        {
          peekedItem = PeekNextFrame ();
          if (peekedItem == 0)
            {
              NS_LOG_DEBUG ("no packets available for transmission");
              return;
            }
          // check if a block ack agreement needs to be established
          m_currentHdr = peekedItem->GetHeader ();
          m_currentPacket = peekedItem->GetPacket ();
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsGroup ()
              && m_stationManager->GetQosSupported (m_currentHdr.GetAddr1 ())
              && (!m_baManager->ExistsAgreement (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ())
                  || m_baManager->ExistsAgreementInState (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid (), OriginatorBlockAckAgreement::RESET))
              && SetupBlockAckIfNeeded ())
            {
              return;
            }

          m_stationManager->UpdateFragmentationThreshold ();
          Ptr<WifiMacQueueItem> item;
          // non-group addressed QoS Data frames may be sent in MU PPDUs. Given that at this stage
          // we do not know the bandwidth it would be given nor the selected acknowledgment
          // sequence, we cannot determine the constraints on size and duration limit. Hence,
          // we only peek the non-group addressed QoS Data frame. MacLow will be in charge of
          // computing the correct limits and dequeue the frame.
          if (peekedItem->GetHeader ().IsQosData () && !peekedItem->GetHeader ().GetAddr1 ().IsGroup ()
              && !NeedFragmentation ())
            {
              item = Copy (peekedItem);
            }
          else
            {
              // compute the limit on the PPDU duration due to the TXOP duration, if any
              Time ppduDurationLimit = Time::Min ();
              if (peekedItem->GetHeader ().IsQosData () && GetTxopLimit ().IsStrictlyPositive ())
                {
                  MacLowTransmissionParameters params = GetTransmissionParameters (peekedItem);
                  ppduDurationLimit = GetTxopRemaining () - m_low->CalculateOverheadTxTime (peekedItem, params);
                }

                // dequeue the peeked item if it fits within the TXOP duration, if any
                item = DequeuePeekedFrame (peekedItem, m_low->GetDataTxVector (peekedItem),
                                          !NeedFragmentation (), 0, ppduDurationLimit);
            }

          if (item == 0)
            {
              NS_LOG_DEBUG ("Not enough time in the current TXOP");
              return;
            }
          m_currentPacket = item->GetPacket ();
          m_currentHdr = item->GetHeader ();
          m_currentPacketTimestamp = item->GetTimeStamp ();

          m_fragmentNumber = 0;
        }
      NS_ASSERT (m_currentPacket != 0);
    }
  Ptr<WifiMacQueueItem> mpdu = Create <WifiMacQueueItem> (m_currentPacket, m_currentHdr,
                                                          m_currentPacketTimestamp);
  m_currentParams = GetTransmissionParameters (mpdu);

  if (m_currentHdr.GetAddr1 ().IsGroup ())
    {
      NS_LOG_DEBUG ("tx broadcast");
      m_low->StartTransmission (mpdu, m_currentParams, this);
    }
  //With COMPRESSED_BLOCK_ACK fragmentation must be avoided.
  else if (((m_currentHdr.IsQosData () && !m_currentHdr.IsQosAmsdu ())
            || (m_currentHdr.IsData () && !m_currentHdr.IsQosData ()))
           && (GetBlockAckThreshold () == 0 || m_blockAckType == BASIC_BLOCK_ACK)
           && NeedFragmentation ())
    {
      m_currentIsFragmented = true;
      m_currentParams.DisableRts ();
      WifiMacHeader hdr;
      Ptr<Packet> fragment = GetFragmentPacket (&hdr);
      if (IsLastFragment ())
        {
          NS_LOG_DEBUG ("fragmenting last fragment size=" << fragment->GetSize ());
          m_currentParams.DisableNextData ();
        }
      else
        {
          NS_LOG_DEBUG ("fragmenting size=" << fragment->GetSize ());
          m_currentParams.EnableNextData (GetNextFragmentSize ());
        }
      m_low->StartTransmission (Create<WifiMacQueueItem> (fragment, hdr),
                                m_currentParams, this);
    }
  else
    {
      m_currentIsFragmented = false;
      m_low->StartTransmission (mpdu, m_currentParams, this);
    }
}

void QosTxop::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);
  bool resetTxop = false;
  // If an internal collision is experienced, the frame involved may still
  // be sitting in the queue, and m_currentPacket may still be null.
  Ptr<const Packet> packet;
  WifiMacHeader header;
  if (m_currentPacket == 0)
    {
      Ptr<const WifiMacQueueItem> item = PeekNextFrame ();
      if (item)
        {
          packet = item->GetPacket ();
          header = item->GetHeader ();
        }
    }
  else
    {
      packet = m_currentPacket;
      header = m_currentHdr;
    }
  if (packet != 0)
    {
      if (m_isAccessRequestedForRts)
        {
          if (!NeedRtsRetransmission (packet, header))
            {
              resetTxop = true;
              m_stationManager->ReportFinalRtsFailed (header.GetAddr1 (), &header);
            }
          else
            {
              m_stationManager->ReportRtsFailed (header.GetAddr1 (), &header);
            }
        }
      else if (header.GetAddr1 () == Mac48Address::GetBroadcast ())
        {
          resetTxop = false;
        }
      else
        {
          if (!NeedDataRetransmission (packet, header))
            {
              resetTxop = true;
              m_stationManager->ReportFinalDataFailed (header.GetAddr1 (), &header, packet->GetSize ());
            }
          else
            {
              m_stationManager->ReportDataFailed (header.GetAddr1 (), &header, packet->GetSize ());
            }
        }
      if (resetTxop)
        {
          NS_LOG_DEBUG ("reset DCF");
          if (!m_txFailedCallback.IsNull ())
            {
              m_txFailedCallback (header);
            }
          //to reset the Txop.
          if (m_currentPacket)
            {
              NS_LOG_DEBUG ("Discarding m_currentPacket");
              m_currentPacket = 0;
            }
          else
            {
              NS_LOG_DEBUG ("Dequeueing and discarding head of queue");
              m_queue->Remove ();
            }
          ResetCw ();
        }
      else
        {
          UpdateFailedCw ();
        }
    }
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
QosTxop::NotifyMissedCts (std::list<Ptr<WifiMacQueueItem>> mpduList)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed cts");
  NS_ASSERT (!mpduList.empty ());
  if (!NeedRtsRetransmission (m_currentPacket, m_currentHdr))
    {
      NS_LOG_DEBUG ("Cts Fail");
      m_stationManager->ReportFinalRtsFailed (m_currentHdr.GetAddr1 (), &m_currentHdr);
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      for (auto& mpdu : mpduList)
        {
          m_baManager->NotifyDiscardedMpdu (mpdu);
        }
      //to reset the Txop.
      m_currentPacket = 0;
      ResetCw ();
      m_cwTrace = GetCw ();
    }
  else
    {
      UpdateFailedCw ();
      m_cwTrace = GetCw ();
      // if a BA agreement is established, store the MPDUs in the block ack manager
      // retransmission queue. Otherwise, this QosTxop will handle the retransmission
      // of the (single) frame
      if (mpduList.size () > 1 ||
          (mpduList.front ()->GetHeader ().IsQosData ()
           && GetBaAgreementEstablished (mpduList.front ()->GetHeader ().GetAddr1 (),
                                         mpduList.front ()->GetHeader ().GetQosTid ())))
        {
          for (auto it = mpduList.rbegin (); it != mpduList.rend (); it++)
            {
              m_baManager->GetRetransmitQueue ()->PushFront (*it);
            }
          m_currentPacket = 0;
        }
    }
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
QosTxop::GotAck (void)
{
  NS_LOG_FUNCTION (this);
  if (!m_currentIsFragmented
      || !m_currentParams.HasNextPacket ()
      || m_currentHdr.IsQosAmsdu ())
    {
      NS_LOG_DEBUG ("got ack. tx done.");
      if (!m_txOkCallback.IsNull ())
        {
          m_txOkCallback (m_currentHdr);
        }

      if (m_currentHdr.IsAction ())
        {
          WifiActionHeader actionHdr;
          Ptr<Packet> p = m_currentPacket->Copy ();
          p->RemoveHeader (actionHdr);
          if (actionHdr.GetCategory () == WifiActionHeader::BLOCK_ACK)
            {
              if (actionHdr.GetAction ().blockAck == WifiActionHeader::BLOCK_ACK_DELBA)
                {
                  MgtDelBaHeader delBa;
                  p->PeekHeader (delBa);
                  if (delBa.IsByOriginator ())
                    {
                      m_baManager->DestroyAgreement (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                    }
                  else
                    {
                      m_low->DestroyBlockAckAgreement (m_currentHdr.GetAddr1 (), delBa.GetTid ());
                    }
                }
              else if (actionHdr.GetAction ().blockAck == WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST)
                {
                  // Setup ADDBA response timeout
                  MgtAddBaRequestHeader addBa;
                  p->PeekHeader (addBa);
                  Simulator::Schedule (m_addBaResponseTimeout,
                                       &QosTxop::AddBaResponseTimeout, this,
                                       m_currentHdr.GetAddr1 (), addBa.GetTid ());
                }
            }
        }
      if (m_currentHdr.IsQosData () && GetBaAgreementEstablished (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ()))
        {
          // notify the BA manager that the current packet was acknowledged
          m_baManager->NotifyGotAck (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr,
                                                                     m_currentPacketTimestamp));
        }
      m_currentPacket = 0;
      ResetCw ();
    }
  else
    {
      NS_LOG_DEBUG ("got ack. tx not done, size=" << m_currentPacket->GetSize ());
    }
}

void
QosTxop::MissedAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("missed ack");
  if (!NeedDataRetransmission (m_currentPacket, m_currentHdr))
    {
      NS_LOG_DEBUG ("Ack Fail");
      m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                               m_currentPacket->GetSize ());
      if (!m_txFailedCallback.IsNull ())
        {
          m_txFailedCallback (m_currentHdr);
        }
      if (m_currentHdr.IsAction ())
        {
          WifiActionHeader actionHdr;
          m_currentPacket->PeekHeader (actionHdr);
          if (actionHdr.GetCategory () == WifiActionHeader::BLOCK_ACK)
            {
              uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
              if (m_baManager->ExistsAgreementInState (m_currentHdr.GetAddr1 (), tid, OriginatorBlockAckAgreement::PENDING))
                {
                  NS_LOG_DEBUG ("No ACK after ADDBA request");
                  m_baManager->NotifyAgreementNoReply (m_currentHdr.GetAddr1 (), tid);
                  Simulator::Schedule (m_failedAddBaTimeout, &QosTxop::ResetBa, this, m_currentHdr.GetAddr1 (), tid);
                }
            }
        }
      if (GetAmpduExist (m_currentHdr.GetAddr1 ()) || m_currentHdr.IsQosData ())
        {
          m_baManager->NotifyDiscardedMpdu (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr));
        }
      m_currentPacket = 0;
      ResetCw ();
      m_cwTrace = GetCw ();
    }
  else
    {
      NS_LOG_DEBUG ("Retransmit");
      m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                          m_currentPacket->GetSize ());
      m_currentHdr.SetRetry ();
      if (m_currentHdr.IsQosData () && GetBaAgreementEstablished (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ()))
        {
          // notify the BA manager that the current packet was not acknowledged
          m_baManager->NotifyMissedAck (Create<WifiMacQueueItem> (m_currentPacket, m_currentHdr,
                                                                  m_currentPacketTimestamp));
          // let the BA manager handle its retransmission
          m_currentPacket = 0;
        }
      UpdateFailedCw ();
      m_cwTrace = GetCw ();
    }
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
QosTxop::MissedBlockAck (uint8_t nMpdus)
{
  NS_LOG_FUNCTION (this << +nMpdus);
  /*
   * If the BlockAck frame is lost, the originator may transmit a BlockAckReq
   * frame to solicit an immediate BlockAck frame or it may retransmit the Data
   * frames. (IEEE std 802.11-2016 sec. 10.24.7.7
   */
  uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
  if (m_useExplicitBarAfterMissedBlockAck || m_currentHdr.IsBlockAckReq ())
    {
      if (NeedBarRetransmission ())
        {
          NS_LOG_DEBUG ("Retransmit block ack request");
          if (m_currentHdr.IsBlockAckReq ())
            {
              m_currentHdr.SetRetry ();
              UpdateFailedCw ();
              m_cwTrace = GetCw ();
            }
          else // missed BlockAck after data frame with Implicit BAR Ack Policy
            {
              Ptr<const WifiMacQueueItem> bar = PrepareBlockAckRequest (m_currentHdr.GetAddr1 (), tid);
              ScheduleBar (bar);
              m_currentPacket = 0;
            }
        }
      else
        {
          NS_LOG_DEBUG ("Block Ack Request Fail");
          // if a BA agreement exists, we can get here if there is no outstanding
          // MPDU whose lifetime has not expired yet.
          m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                   m_currentPacket->GetSize ());
          if (m_baManager->ExistsAgreementInState (m_currentHdr.GetAddr1 (), tid,
                                                   OriginatorBlockAckAgreement::ESTABLISHED))
            {
              // If there is any (expired) outstanding MPDU, request the BA manager to discard
              // it, which involves the scheduling of a BAR to advance the recipient's window
              if (m_baManager->GetNBufferedPackets (m_currentHdr.GetAddr1 (), tid) > 0)
                {
                  m_baManager->DiscardOutstandingMpdus (m_currentHdr.GetAddr1 (), tid);
                }
              // otherwise, it means that we have not received a BlockAck in response to a
              // BlockAckRequest sent while no frame was outstanding, whose purpose was therefore
              // to advance the recipient's window. Schedule a BlockAckRequest with
              // skipIfNoDataQueued set to true, so that the BlockAckRequest is only sent
              // if there are data frames queued for this recipient.
              else
                {
                  ScheduleBar (PrepareBlockAckRequest (m_currentHdr.GetAddr1 (), tid), true);
                }
            }
          //to reset the Txop.
          m_currentPacket = 0;
          ResetCw ();
          m_cwTrace = GetCw ();
        }
    }
  else
    {
      if (GetAmpduExist (m_currentHdr.GetAddr1 ()))
        {
          m_stationManager->ReportAmpduTxStatus (m_currentHdr.GetAddr1 (), 0, nMpdus, 0, 0, WifiTxVector ());
        }
      // implicit BAR and do not use BAR after missed BlockAck, hence try to retransmit data frames
      if (!NeedDataRetransmission (m_currentPacket, m_currentHdr))
        {
          NS_LOG_DEBUG ("Block Ack Fail");
          m_stationManager->ReportFinalDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                   m_currentPacket->GetSize ());
          if (!m_txFailedCallback.IsNull ())
            {
              m_txFailedCallback (m_currentHdr);
            }
          if (m_currentHdr.IsAction ())
            {
              WifiActionHeader actionHdr;
              m_currentPacket->PeekHeader (actionHdr);
              if (actionHdr.GetCategory () == WifiActionHeader::BLOCK_ACK)
                {
                  uint8_t tid = GetTid (m_currentPacket, m_currentHdr);
                  if (m_baManager->ExistsAgreementInState (m_currentHdr.GetAddr1 (), tid, OriginatorBlockAckAgreement::PENDING))
                    {
                      NS_LOG_DEBUG ("No ACK after ADDBA request");
                      m_baManager->NotifyAgreementNoReply (m_currentHdr.GetAddr1 (), tid);
                      Simulator::Schedule (m_failedAddBaTimeout, &QosTxop::ResetBa, this, m_currentHdr.GetAddr1 (), tid);
                    }
                }
            }
          //to reset the Txop.
          m_baManager->DiscardOutstandingMpdus (m_currentHdr.GetAddr1 (), GetTid (m_currentPacket, m_currentHdr));
          m_currentPacket = 0;
          ResetCw ();
          m_cwTrace = GetCw ();
        }
      else
        {
          NS_LOG_DEBUG ("Retransmit");
          m_stationManager->ReportDataFailed (m_currentHdr.GetAddr1 (), &m_currentHdr, m_currentPacket->GetSize ());
          m_baManager->NotifyMissedBlockAck (m_currentHdr.GetAddr1 (), tid);
          m_currentPacket = 0;
          UpdateFailedCw ();
          m_cwTrace = GetCw ();
        }
    }
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

void
QosTxop::RestartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);

  // check if the BA manager retransmit queue is empty, so that expired
  // frames (if any) are removed and a BlockAckRequest is scheduled to advance
  // the starting sequence number of the transmit (and receiver) window
  bool baManagerHasPackets = m_baManager->HasPackets ();
  // remove MSDUs with expired lifetime starting from the head of the queue
  bool queueIsNotEmpty = !m_queue->IsEmpty ();

  if ((m_currentPacket != 0 || baManagerHasPackets || queueIsNotEmpty)
      && !IsAccessRequested ())
    {
      Ptr<const WifiMacQueueItem> item;
      if (m_currentPacket != 0)
        {
          item = Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr);
        }
      else
        {
          item = PeekNextFrame ();
        }
      if (item != 0)
        {
          m_isAccessRequestedForRts = m_stationManager->NeedRts (item->GetHeader (), item->GetSize ());
        }
      else
        {
          m_isAccessRequestedForRts = false;
        }
      m_channelAccessManager->RequestAccess (this);
    }
}

void
QosTxop::StartAccessIfNeeded (void)
{
  NS_LOG_FUNCTION (this);

  // check if the BA manager retransmit queue is empty, so that expired
  // frames (if any) are removed and a BlockAckRequest is scheduled to advance
  // the starting sequence number of the transmit (and receiver) window
  bool baManagerHasPackets = m_baManager->HasPackets ();
  // remove MSDUs with expired lifetime starting from the head of the queue
  bool queueIsNotEmpty = !m_queue->IsEmpty ();

  if (m_currentPacket == 0
      && (baManagerHasPackets || queueIsNotEmpty)
      && !IsAccessRequested ())
    {
      Ptr<const WifiMacQueueItem> item = PeekNextFrame ();
      if (item != 0)
        {
          m_isAccessRequestedForRts = m_stationManager->NeedRts (item->GetHeader (), item->GetSize ());
        }
      else
        {
          m_isAccessRequestedForRts = false;
        }
      m_channelAccessManager->RequestAccess (this);
    }
}

bool
QosTxop::NeedBarRetransmission (void)
{
  uint8_t tid = 0;
  if (m_currentHdr.IsQosData ())
    {
      tid = m_currentHdr.GetQosTid ();
    }
  else if (m_currentHdr.IsBlockAckReq ())
    {
      CtrlBAckRequestHeader baReqHdr;
      m_currentPacket->PeekHeader (baReqHdr);
      tid = baReqHdr.GetTidInfo ();
    }
  else if (m_currentHdr.IsBlockAck ())
    {
      CtrlBAckResponseHeader baRespHdr;
      m_currentPacket->PeekHeader (baRespHdr);
      tid = baRespHdr.GetTidInfo ();
    }
  return m_baManager->NeedBarRetransmission (tid, m_currentHdr.GetAddr1 ());
}

void
QosTxop::StartNextPacket (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (GetTxopLimit ().IsStrictlyPositive () && GetTxopRemaining ().IsStrictlyPositive ());

  m_currentPacket = 0;
  // peek the next BlockAckReq, if any
  Ptr<const WifiMacQueueItem> nextFrame = m_baManager->GetBar (false);

  if (nextFrame == 0)
    {
      nextFrame = PeekNextFrame ();
    }

  if (nextFrame != 0)
    {
      MacLowTransmissionParameters params = GetTransmissionParameters (nextFrame);

      if (GetTxopRemaining () >= m_low->CalculateOverallTxTime (nextFrame->GetPacket (), &nextFrame->GetHeader (), params))
        {
          // check if a block ack agreement needs to be established
          m_currentHdr = nextFrame->GetHeader ();
          m_currentPacket = nextFrame->GetPacket ();
          if (m_currentHdr.IsQosData () && !m_currentHdr.GetAddr1 ().IsGroup ()
              && m_stationManager->GetQosSupported (m_currentHdr.GetAddr1 ())
              && (!m_baManager->ExistsAgreement (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid ())
                  || m_baManager->ExistsAgreementInState (m_currentHdr.GetAddr1 (), m_currentHdr.GetQosTid (), OriginatorBlockAckAgreement::RESET))
              && SetupBlockAckIfNeeded ())
            {
              return;
            }

          Ptr<WifiMacQueueItem> item;
          if (nextFrame->GetHeader ().IsBlockAckReq ())
            {
              item = Copy (m_baManager->GetBar ());
            }
          // non-group addressed QoS Data frames may be sent in MU PPDUs. Given that at this stage
          // we do not know the bandwidth it would be given nor the selected acknowledgment
          // sequence, we cannot determine the constraints on size and duration limit. Hence,
          // we only peek the non-group addressed QoS Data frame. MacLow will be in charge of
          // computing the correct limits and dequeue the frame.
          else if (nextFrame->GetHeader ().IsQosData () && !nextFrame->GetHeader ().GetAddr1 ().IsGroup ())
            {
              item = Copy (nextFrame);
            }
          else
            {
              // dequeue the peeked frame
              item = DequeuePeekedFrame (nextFrame, m_low->GetDataTxVector (nextFrame));
            }

          NS_ASSERT (item != 0);
          NS_LOG_DEBUG ("start next packet " << *item << " within the current TXOP");
          m_currentPacket = item->GetPacket ();
          m_currentHdr = item->GetHeader ();
          m_currentPacketTimestamp = item->GetTimeStamp ();
          NS_ASSERT (m_currentPacket != 0);

          m_currentParams = params;
          m_stationManager->UpdateFragmentationThreshold ();
          m_fragmentNumber = 0;
          GetLow ()->StartTransmission (item, m_currentParams, this);
          return;
        }
    }

  // terminate TXOP because no (suitable) frame was found
  TerminateTxop ();
}

void
QosTxop::TerminateTxop (void)
{
  NS_LOG_FUNCTION (this);
  if (GetTxopLimit ().IsStrictlyPositive ())
    {
      NS_LOG_DEBUG ("Terminating TXOP. Duration = " << Simulator::Now () - m_startTxop);
    }
  m_txopTrace (m_startTxop, Simulator::Now () - m_startTxop);
  GenerateBackoff ();
  RestartAccessIfNeeded ();
}

Time
QosTxop::GetTxopRemaining (void) const
{
  Time remainingTxop = GetTxopLimit ();
  remainingTxop -= (Simulator::Now () - m_startTxop);
  if (remainingTxop.IsStrictlyNegative ())
    {
      remainingTxop = Seconds (0);
    }
  NS_LOG_FUNCTION (this << remainingTxop);
  return remainingTxop;
}

void
QosTxop::EndTxNoAck (void)
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("a transmission that did not require an ACK just finished");
  m_currentPacket = 0;
  ResetCw ();
  TerminateTxop ();
}

bool
QosTxop::NeedFragmentation (void) const
{
  NS_LOG_FUNCTION (this);
  if (m_stationManager->GetVhtSupported ()
      || m_stationManager->GetHeSupported ()
      || GetAmpduExist (m_currentHdr.GetAddr1 ())
      || (m_stationManager->GetHtSupported ()
          && m_currentHdr.IsQosData ()
          && GetBaAgreementEstablished (m_currentHdr.GetAddr1 (), GetTid (m_currentPacket, m_currentHdr))
          && GetLow ()->GetMpduAggregator () != 0
          && GetLow ()->GetMpduAggregator ()->GetMaxAmpduSize (m_currentHdr.GetAddr1 (), GetTid (m_currentPacket, m_currentHdr),
                                                               WIFI_MOD_CLASS_HT) >= m_currentPacket->GetSize ()))
    {
      //MSDU is not fragmented when it is transmitted using an HT-immediate or
      //HT-delayed block ack agreement or when it is carried in an A-MPDU.
      return false;
    }
  bool needTxopFragmentation = false;
  if (GetTxopLimit ().IsStrictlyPositive () && m_currentHdr.IsData ())
    {
      needTxopFragmentation = (GetLow ()->CalculateOverallTxTime (m_currentPacket, &m_currentHdr, m_currentParams) > GetTxopLimit ());
    }
  return (needTxopFragmentation || m_stationManager->NeedFragmentation (m_currentHdr.GetAddr1 (), &m_currentHdr, m_currentPacket));
}

bool
QosTxop::IsTxopFragmentation (void) const
{
  if (GetTxopLimit ().IsZero ())
    {
      return false;
    }
  if (!m_stationManager->NeedFragmentation (m_currentHdr.GetAddr1 (), &m_currentHdr, m_currentPacket)
      || (GetTxopFragmentSize () < m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,m_currentPacket, 0)))
    {
      return true;
    }
  return false;
}

uint32_t
QosTxop::GetTxopFragmentSize (void) const
{
  Time txopDuration = GetTxopLimit ();
  if (txopDuration.IsZero ())
    {
      return 0;
    }
  uint32_t maxSize = m_currentPacket->GetSize ();
  uint32_t minSize = 0;
  uint32_t size = 0;
  bool found = false;
  while (!found)
    {
      size = (minSize + ((maxSize - minSize) / 2));
      if (GetLow ()->CalculateOverallTxTime (m_currentPacket, &m_currentHdr, m_currentParams, size) > txopDuration)
        {
          maxSize = size;
        }
      else
        {
          minSize = size;
        }
      if (GetLow ()->CalculateOverallTxTime (m_currentPacket, &m_currentHdr, m_currentParams, size) <= txopDuration
          && GetLow ()->CalculateOverallTxTime (m_currentPacket, &m_currentHdr, m_currentParams, size + 1) > txopDuration)
        {
          found = true;
        }
    }
  NS_ASSERT (size != 0);
  return size;
}

uint32_t
QosTxop::GetNTxopFragment (void) const
{
  uint32_t fragmentSize = GetTxopFragmentSize ();
  uint32_t nFragments = (m_currentPacket->GetSize () / fragmentSize);
  if ((m_currentPacket->GetSize () % fragmentSize) > 0)
    {
      nFragments++;
    }
  NS_LOG_DEBUG ("GetNTxopFragment returning " << nFragments);
  return nFragments;
}

uint32_t
QosTxop::GetTxopFragmentOffset (uint32_t fragmentNumber) const
{
  if (fragmentNumber == 0)
    {
      return 0;
    }
  uint32_t offset = 0;
  uint32_t fragmentSize = GetTxopFragmentSize ();
  uint32_t nFragments = (m_currentPacket->GetSize () / fragmentSize);
  if ((m_currentPacket->GetSize () % fragmentSize) > 0)
    {
      nFragments++;
    }
  if (fragmentNumber < nFragments)
    {
      offset = (fragmentNumber * fragmentSize);
    }
  else
    {
      NS_ASSERT (false);
    }
  NS_LOG_DEBUG ("GetTxopFragmentOffset returning " << offset);
  return offset;
}

uint32_t
QosTxop::GetNextTxopFragmentSize (uint32_t fragmentNumber) const
{
  NS_LOG_FUNCTION (this << fragmentNumber);
  uint32_t fragmentSize = GetTxopFragmentSize ();
  uint32_t nFragments = GetNTxopFragment ();
  if (fragmentNumber >= nFragments)
    {
      NS_LOG_DEBUG ("GetNextTxopFragmentSize returning 0");
      return 0;
    }
  if (fragmentNumber == nFragments - 1)
    {
      fragmentSize = (m_currentPacket->GetSize () - ((nFragments - 1) * fragmentSize));
    }
  NS_LOG_DEBUG ("GetNextTxopFragmentSize returning " << fragmentSize);
  return fragmentSize;
}

uint32_t
QosTxop::GetFragmentSize (void) const
{
  uint32_t size;
  if (IsTxopFragmentation ())
    {
      size = GetNextTxopFragmentSize (m_fragmentNumber);
    }
  else
    {
      size = m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,m_currentPacket, m_fragmentNumber);
    }
  return size;
}

uint32_t
QosTxop::GetNextFragmentSize (void) const
{
  uint32_t size;
  if (IsTxopFragmentation ())
    {
      size = GetNextTxopFragmentSize (m_fragmentNumber + 1);
    }
  else
    {
      size = m_stationManager->GetFragmentSize (m_currentHdr.GetAddr1 (), &m_currentHdr,m_currentPacket, m_fragmentNumber + 1);
    }
  return size;
}

uint32_t
QosTxop::GetFragmentOffset (void) const
{
  uint32_t offset;
  if (IsTxopFragmentation ())
    {
      offset = GetTxopFragmentOffset (m_fragmentNumber);
    }
  else
    {
      offset = m_stationManager->GetFragmentOffset (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                    m_currentPacket, m_fragmentNumber);
    }
  return offset;
}

bool
QosTxop::IsLastFragment (void) const
{
  bool isLastFragment;
  if (IsTxopFragmentation ())
    {
      isLastFragment = (m_fragmentNumber == GetNTxopFragment () - 1);
    }
  else
    {
      isLastFragment = m_stationManager->IsLastFragment (m_currentHdr.GetAddr1 (), &m_currentHdr,
                                                         m_currentPacket, m_fragmentNumber);
    }
  return isLastFragment;
}

Ptr<Packet>
QosTxop::GetFragmentPacket (WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  *hdr = m_currentHdr;
  hdr->SetFragmentNumber (m_fragmentNumber);
  uint32_t startOffset = GetFragmentOffset ();
  Ptr<Packet> fragment;
  if (IsLastFragment ())
    {
      hdr->SetNoMoreFragments ();
    }
  else
    {
      hdr->SetMoreFragments ();
    }
  fragment = m_currentPacket->CreateFragment (startOffset,
                                              GetFragmentSize ());
  return fragment;
}

void
QosTxop::SetAccessCategory (AcIndex ac)
{
  NS_LOG_FUNCTION (this << +ac);
  m_ac = ac;
}

Mac48Address
QosTxop::MapSrcAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  Mac48Address retval;
  if (GetTypeOfStation () == STA || GetTypeOfStation () == ADHOC_STA)
    {
      retval = hdr.GetAddr2 ();
    }
  else
    {
      retval = hdr.GetAddr3 ();
    }
  return retval;
}

Mac48Address
QosTxop::MapDestAddressForAggregation (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << &hdr);
  Mac48Address retval;
  if (GetTypeOfStation () == AP || GetTypeOfStation () == ADHOC_STA)
    {
      retval = hdr.GetAddr1 ();
    }
  else
    {
      retval = hdr.GetAddr3 ();
    }
  return retval;
}

void
QosTxop::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << packet << &hdr);
  WifiMacTrailer fcs;
  m_queue->PushFront (Create<WifiMacQueueItem> (packet, hdr));
  StartAccessIfNeeded ();
}

void
QosTxop::GotAddBaResponse (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << respHdr << recipient);
  uint8_t tid = respHdr->GetTid ();
  if (respHdr->GetStatusCode ().IsSuccess ())
    {
      NS_LOG_DEBUG ("block ack agreement established with " << recipient);
      // Even though a (destination, TID) pair is "blocked" (i.e., no more packets
      // are sent) when an Add BA Request is sent to the destination,
      // the current packet may still be non-null when the Add BA Response is received.
      // In fact, if the Add BA Request timer expires, the (destination, TID) pair is
      // "unblocked" and packets to the destination are sent again (under normal
      // ack policy). Thus, there may be a packet needing to be retransmitted
      // when the Add BA Response is received. If this is the case, let the block
      // ack manager handle its retransmission.
      if (m_currentPacket != 0 && m_currentHdr.IsQosData ()
          && m_currentHdr.GetAddr1 () == recipient && m_currentHdr.GetQosTid () == tid)
        {
          Ptr<WifiMacQueueItem> mpdu = Create<WifiMacQueueItem> (m_currentPacket, m_currentHdr,
                                                                 m_currentPacketTimestamp);
          m_baManager->GetRetransmitQueue ()->Enqueue (mpdu);
          m_currentPacket = 0;
        }
      m_baManager->UpdateAgreement (respHdr, recipient);
    }
  else
    {
      NS_LOG_DEBUG ("discard ADDBA response" << recipient);
      m_baManager->NotifyAgreementRejected (recipient, tid);
    }
  RestartAccessIfNeeded ();
}

void
QosTxop::GotDelBaFrame (const MgtDelBaHeader *delBaHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << delBaHdr << recipient);
  NS_LOG_DEBUG ("received DELBA frame from=" << recipient);
  m_baManager->DestroyAgreement (recipient, delBaHdr->GetTid ());
}

void
QosTxop::GotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, double dataSnr, WifiTxVector dataTxVector)
{
  NS_LOG_FUNCTION (this << blockAck << recipient << rxSnr << dataSnr << dataTxVector);
  NS_LOG_DEBUG ("got block ack from=" << recipient);
  m_baManager->NotifyGotBlockAck (blockAck, recipient, rxSnr, dataSnr, dataTxVector);
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (m_currentHdr);
    }
  m_currentPacket = 0;
  ResetCw ();
}

bool QosTxop::GetAmpduExist (Mac48Address dest) const
{
  NS_LOG_FUNCTION (this << dest);
  auto it = m_aMpduEnabled.find (dest);
  if (it != m_aMpduEnabled.end ())
    {
      return it->second;
    }
  return false;
}

void QosTxop::SetAmpduExist (Mac48Address dest, bool enableAmpdu)
{
  NS_LOG_FUNCTION (this << dest << enableAmpdu);
  m_aMpduEnabled[dest] = enableAmpdu;
}

void
QosTxop::CompleteMpduTx (Ptr<WifiMacQueueItem> mpdu)
{
  NS_ASSERT (mpdu->GetHeader ().IsQosData ());
  // If there is an established BA agreement, store the packet in the queue of outstanding packets
  if (GetBaAgreementEstablished (mpdu->GetHeader ().GetAddr1 (), mpdu->GetHeader ().GetQosTid ()))
    {
      m_baManager->StorePacket (mpdu);
    }
}

bool
QosTxop::SetupBlockAckIfNeeded (void)
{
  NS_LOG_FUNCTION (this);
  uint8_t tid = m_currentHdr.GetQosTid ();
  Mac48Address recipient = m_currentHdr.GetAddr1 ();
  uint32_t packets = m_queue->GetNPacketsByTidAndAddress (tid, recipient);
  if ((GetBlockAckThreshold () > 0 && packets >= GetBlockAckThreshold ())
      || (GetLow ()->GetMpduAggregator () != 0 && GetLow ()->GetMpduAggregator ()->GetMaxAmpduSize (recipient, tid, WIFI_MOD_CLASS_HT) > 0 && packets > 1)
      || m_stationManager->GetVhtSupported ()
      || m_stationManager->GetHeSupported ())
    {
      /* Block ack setup */
      uint16_t startingSequence = m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient);
      SendAddBaRequest (recipient, tid, startingSequence, m_blockAckInactivityTimeout, true);
      return true;
    }
  return false;
}

void
QosTxop::CompleteConfig (void)
{
  NS_LOG_FUNCTION (this);
  m_baManager->SetTxMiddle (m_txMiddle);
  m_low->RegisterEdcaForAc (m_ac, this);
  m_baManager->SetBlockAckInactivityCallback (MakeCallback (&QosTxop::SendDelbaFrame, this));
}

void
QosTxop::SetBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  m_blockAckThreshold = threshold;
  m_baManager->SetBlockAckThreshold (threshold);
}

void
QosTxop::SetBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  m_blockAckInactivityTimeout = timeout;
}

uint8_t
QosTxop::GetBlockAckThreshold (void) const
{
  NS_LOG_FUNCTION (this);
  return m_blockAckThreshold;
}

void
QosTxop::SendAddBaRequest (Mac48Address dest, uint8_t tid, uint16_t startSeq,
                           uint16_t timeout, bool immediateBAck)
{
  NS_LOG_FUNCTION (this << dest << +tid << startSeq << timeout << immediateBAck);
  NS_LOG_DEBUG ("sent ADDBA request to " << dest);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (dest);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetBssid ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  /*Setting ADDBARequest header*/
  MgtAddBaRequestHeader reqHdr;
  reqHdr.SetAmsduSupport (true);
  if (immediateBAck)
    {
      reqHdr.SetImmediateBlockAck ();
    }
  else
    {
      reqHdr.SetDelayedBlockAck ();
    }
  reqHdr.SetTid (tid);
  /* For now we don't use buffer size field in the ADDBA request frame. The recipient
   * will choose how many packets it can receive under block ack.
   */
  reqHdr.SetBufferSize (0);
  reqHdr.SetTimeout (timeout);
  reqHdr.SetStartingSequence (startSeq);

  m_baManager->CreateAgreement (&reqHdr, dest);

  packet->AddHeader (reqHdr);
  packet->AddHeader (actionHdr);

  m_currentPacket = packet;
  m_currentHdr = hdr;

  uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor (&m_currentHdr);
  m_currentHdr.SetSequenceNumber (sequence);
  m_stationManager->UpdateFragmentationThreshold ();
  m_currentHdr.SetFragmentNumber (0);
  m_currentHdr.SetNoMoreFragments ();
  m_currentHdr.SetNoRetry ();

  m_currentParams.EnableAck ();
  m_currentParams.DisableRts ();
  m_currentParams.DisableNextData ();

  m_low->StartTransmission (Create<WifiMacQueueItem> (m_currentPacket, m_currentHdr), m_currentParams, this);
}

void
QosTxop::SendDelbaFrame (Mac48Address addr, uint8_t tid, bool byOriginator)
{
  NS_LOG_FUNCTION (this << addr << +tid << byOriginator);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (addr);
  hdr.SetAddr2 (m_low->GetAddress ());
  hdr.SetAddr3 (m_low->GetBssid ());
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();

  MgtDelBaHeader delbaHdr;
  delbaHdr.SetTid (tid);
  if (byOriginator)
    {
      delbaHdr.SetByOriginator ();
      m_baManager->DestroyAgreement (addr, tid);
    }
  else
    {
      delbaHdr.SetByRecipient ();
      m_low->DestroyBlockAckAgreement (addr, tid);
    }

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_DELBA;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (delbaHdr);
  packet->AddHeader (actionHdr);

  PushFront (packet, hdr);
}

void
QosTxop::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  ResetCw ();
  m_cwTrace = GetCw ();
  GenerateBackoff ();
}

void
QosTxop::BaTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (m_currentHdr);
    }
}

void
QosTxop::BaTxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txFailedCallback.IsNull ())
    {
      m_txFailedCallback (m_currentHdr);
    }
}

void
QosTxop::AddBaResponseTimeout (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  // If agreement is still pending, ADDBA response is not received
  if (m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::PENDING))
    {
      m_baManager->NotifyAgreementNoReply (recipient, tid);
      Simulator::Schedule (m_failedAddBaTimeout, &QosTxop::ResetBa, this, recipient, tid);
      GenerateBackoff ();
      RestartAccessIfNeeded ();
    }
}

void
QosTxop::ResetBa (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  // This function is scheduled when waiting for an ADDBA response. However,
  // before this function is called, a DELBA request may arrive, which causes
  // the agreement to be deleted. Hence, check if an agreement exists before
  // notifying that the agreement has to be reset.
  if (m_baManager->ExistsAgreement (recipient, tid)
      && !m_baManager->ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED))
    {
      m_baManager->NotifyAgreementReset (recipient, tid);
    }
}

void
QosTxop::SetAddBaResponseTimeout (Time addBaResponseTimeout)
{
  NS_LOG_FUNCTION (this << addBaResponseTimeout);
  m_addBaResponseTimeout = addBaResponseTimeout;
}

Time
QosTxop::GetAddBaResponseTimeout (void) const
{
  return m_addBaResponseTimeout;
}

void
QosTxop::SetFailedAddBaTimeout (Time failedAddBaTimeout)
{
  NS_LOG_FUNCTION (this << failedAddBaTimeout);
  m_failedAddBaTimeout = failedAddBaTimeout;
}

Time
QosTxop::GetFailedAddBaTimeout (void) const
{
  return m_failedAddBaTimeout;
}

bool
QosTxop::IsQosTxop (void) const
{
  return true;
}

} //namespace ns3
