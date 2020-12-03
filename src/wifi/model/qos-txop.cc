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
#include "qos-blocked-destinations.h"
#include "wifi-remote-station-manager.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "ctrl-headers.h"
#include "wifi-phy.h"
#include "wifi-psdu.h"
#include "ht-frame-exchange-manager.h"
#include "wifi-tx-parameters.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT if (m_stationManager != 0 && m_stationManager->GetMac () != 0) { std::clog << "[mac=" << m_stationManager->GetMac ()->GetAddress () << "] "; }

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
    m_blockAckType (BlockAckType::COMPRESSED),
    m_startTxop (Seconds (0)),
    m_txopDuration (Seconds (0)),
    m_isAccessRequestedForRts (false),
    m_currentIsFragmented (false)
{
  NS_LOG_FUNCTION (this);
  m_qosBlockedDestinations = Create<QosBlockedDestinations> ();
  m_baManager = CreateObject<BlockAckManager> ();
  m_baManager->SetQueue (m_queue);
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
  m_baManager = 0;
  m_qosBlockedDestinations = 0;
  m_qosFem = 0;
  Txop::DoDispose ();
}

void
QosTxop::SetQosQueueSize (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  WifiMacHeader& hdr = mpdu->GetHeader ();
  NS_ASSERT (hdr.IsQosData ());

  uint32_t bufferSize = m_queue->GetNBytes (hdr.GetQosTid (), hdr.GetAddr1 ())
                        + m_baManager->GetRetransmitQueue ()->GetNBytes (hdr.GetQosTid (), hdr.GetAddr1 ());
  // A queue size value of 254 is used for all sizes greater than 64 768 octets.
  uint8_t queueSize = static_cast<uint8_t> (std::ceil (std::min (bufferSize, 64769u) / 256.0));
  NS_LOG_DEBUG ("Buffer size=" << bufferSize << " Queue Size=" << +queueSize);
  hdr.SetQosQueueSize (queueSize);
}

void
QosTxop::SetQosFrameExchangeManager (const Ptr<QosFrameExchangeManager> qosFem)
{
  NS_LOG_FUNCTION (this << qosFem);
  m_qosFem = qosFem;
}

Ptr<BlockAckManager>
QosTxop::GetBaManager (void)
{
  return m_baManager;
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
  NS_ASSERT (QosUtilsMapTidToAc (tid) == m_ac);

  CtrlBAckRequestHeader reqHdr = m_baManager->GetBlockAckReqHeader (recipient, tid);
  Ptr<Packet> bar = Create<Packet> ();
  bar->AddHeader (reqHdr);

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_BACKREQ);
  hdr.SetAddr1 (recipient);
  hdr.SetAddr2 (m_stationManager->GetMac ()->GetAddress ());
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
QosTxop::UseExplicitBarAfterMissedBlockAck (void) const
{
  return m_useExplicitBarAfterMissedBlockAck;
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
QosTxop::PeekNextMpdu (uint8_t tid, Mac48Address recipient)
{
  return PeekNextMpdu ({nullptr, WifiMacQueue::EMPTY}, tid, recipient);
}

Ptr<const WifiMacQueueItem>
QosTxop::PeekNextMpdu (WifiMacQueueItem::QueueIteratorPair queueIt, uint8_t tid, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << +tid << recipient);

  // lambda to peek the next frame
  auto peek = [this, &tid, &recipient, &queueIt] () -> WifiMacQueue::ConstIterator
    {
      if (tid == 8 && recipient.IsBroadcast ())  // undefined TID and recipient
        {
          return queueIt.queue->PeekFirstAvailable (m_qosBlockedDestinations, queueIt.it);
        }
      if (m_qosBlockedDestinations->IsBlocked (recipient, tid))
        {
          return queueIt.queue->end ();
        }
      return queueIt.queue->PeekByTidAndAddress (tid, recipient, queueIt.it);
    };

  if (queueIt.queue == nullptr && queueIt.it == WifiMacQueue::EMPTY)
    {
      // check if there is a packet in the BlockAckManager retransmit queue
      queueIt.queue = PeekPointer (m_baManager->GetRetransmitQueue ());
    }

  if (queueIt.queue == PeekPointer (m_baManager->GetRetransmitQueue ()))
    {
      queueIt.it = peek ();
      // remove old packets
      while (queueIt.it != m_baManager->GetRetransmitQueue ()->end () && IsQosOldPacket (*queueIt.it))
        {
          NS_LOG_DEBUG ("Removing an old packet from BlockAckManager retransmit queue: " << **queueIt.it);
          queueIt.it = m_baManager->GetRetransmitQueue ()->Remove (queueIt.it);
          queueIt.it = peek ();
        }
      if (queueIt.it != m_baManager->GetRetransmitQueue ()->end ())
        {
          NS_LOG_DEBUG ("Packet peeked from BlockAckManager retransmit queue: " << **queueIt.it);
          return *queueIt.it;
        }
      // otherwise, check if there is a packet in the EDCA queue
      queueIt = {PeekPointer (m_queue), WifiMacQueue::EMPTY};
    }

  queueIt.it = peek ();
  if (queueIt.it != m_queue->end ())
    {
      // peek the next sequence number and check if it is within the transmit window
      // in case of QoS data frame
      uint16_t sequence = m_txMiddle->PeekNextSequenceNumberFor (&(*queueIt.it)->GetHeader ());
      if ((*queueIt.it)->GetHeader ().IsQosData ())
        {
          Mac48Address recipient = (*queueIt.it)->GetHeader ().GetAddr1 ();
          uint8_t tid = (*queueIt.it)->GetHeader ().GetQosTid ();

          if (GetBaAgreementEstablished (recipient, tid)
              && !IsInWindow (sequence, GetBaStartingSequence (recipient, tid), GetBaBufferSize (recipient, tid)))
            {
              NS_LOG_DEBUG ("Packet beyond the end of the current transmit window");
              return 0;
            }
        }

      WifiMacHeader& hdr = (*queueIt.it)->GetHeader ();
      // Assign a sequence number if this is not a fragment nor a retransmission
      if (!(*queueIt.it)->IsFragment () && !hdr.IsRetry ())
        {
          hdr.SetSequenceNumber (sequence);
        }
      NS_LOG_DEBUG ("Packet peeked from EDCA queue: " << **queueIt.it);
      return *queueIt.it;
    }

  return 0;
}

Ptr<WifiMacQueueItem>
QosTxop::GetNextMpdu (Ptr<const WifiMacQueueItem> peekedItem, WifiTxParameters& txParams,
                      Time availableTime, bool initialFrame, WifiMacQueueItem::QueueIteratorPair& queueIt)
{
  NS_ASSERT (peekedItem != 0);
  NS_ASSERT (m_qosFem != 0);
  NS_LOG_FUNCTION (this << *peekedItem << &txParams << availableTime << initialFrame);

  Mac48Address recipient = peekedItem->GetHeader ().GetAddr1 ();

  // The TXOP limit can be exceeded by the TXOP holder if it does not transmit more
  // than one Data or Management frame in the TXOP and the frame is not in an A-MPDU
  // consisting of more than one MPDU (Sec. 10.22.2.8 of 802.11-2016)
  Time actualAvailableTime = (initialFrame && txParams.GetSize (recipient) == 0
                              ? Time::Min () : availableTime);

  if (!m_qosFem->TryAddMpdu (peekedItem, txParams, actualAvailableTime))
    {
      return nullptr;
    }

  NS_ASSERT (peekedItem->IsQueued ());
  NS_ASSERT_MSG (peekedItem->GetQueueIteratorPairs ().size () == 1,
                 "An item in the MAC queue cannot contain an A-MSDU");
  WifiMacQueueItem::QueueIteratorPair peekedIt = peekedItem->GetQueueIteratorPairs ().front ();
  NS_ASSERT ((*peekedIt.it)->GetPacket () == peekedItem->GetPacket ());

  if (peekedIt.queue == PeekPointer (m_baManager->GetRetransmitQueue ()))
    {
      // the packet can only have been peeked from the block ack manager retransmit
      // queue if:
      // - the peeked packet is a QoS Data frame AND
      // - the peeked packet is not a broadcast frame AND
      // - an agreement has been established
      NS_ASSERT (peekedItem->GetHeader ().IsQosData () && !recipient.IsBroadcast ());
      uint8_t tid = peekedItem->GetHeader ().GetQosTid ();
      NS_ASSERT (GetBaAgreementEstablished (recipient, tid));
      // we should not be asked to dequeue an old packet
      NS_ASSERT (!QosUtilsIsOldPacket (GetBaStartingSequence (recipient, tid),
                                       peekedItem->GetHeader ().GetSequenceNumber ()));
      // we should not be asked to dequeue an MPDU that is beyond the transmit window
      NS_ASSERT (IsInWindow (peekedItem->GetHeader ().GetSequenceNumber (),
                             GetBaStartingSequence (recipient, tid),
                             GetBaBufferSize (recipient, tid)));
      // A-MSDU aggregation cannot be done on a retransmitted MPDU, hence return
      // the peeked MPDU
      NS_LOG_DEBUG ("Got MPDU from BA manager queue: " << *peekedItem);

      queueIt = peekedIt;
      queueIt.it++;

      return *peekedIt.it;
    }

  // The MPDU has been peeked from the EDCA queue.
  NS_ASSERT (peekedIt.queue == PeekPointer (m_queue));
  Ptr<WifiMacQueueItem> mpdu;

  // If it is a non-broadcast QoS Data frame and it is not a retransmission nor a fragment,
  // attempt A-MSDU aggregation
  if (peekedItem->GetHeader ().IsQosData ())
    {
      uint8_t tid = peekedItem->GetHeader ().GetQosTid ();

      // we should not be asked to dequeue an MPDU that is beyond the transmit window.
      // Note that PeekNextMpdu() temporarily assigns the next available sequence number
      // to the peeked frame
      NS_ASSERT (!GetBaAgreementEstablished (recipient, tid)
                 || IsInWindow (peekedItem->GetHeader ().GetSequenceNumber (),
                                GetBaStartingSequence (recipient, tid),
                                GetBaBufferSize (recipient, tid)));

      // try A-MSDU aggregation
      if (m_stationManager->GetHtSupported () && !recipient.IsBroadcast ()
          && !peekedItem->GetHeader ().IsRetry () && !peekedItem->IsFragment ())
        {
          Ptr<HtFrameExchangeManager> htFem = StaticCast<HtFrameExchangeManager> (m_qosFem);
          mpdu = htFem->GetMsduAggregator ()->GetNextAmsdu (peekedItem, txParams, availableTime, peekedIt);
        }

      if (mpdu != 0)
        {
          NS_LOG_DEBUG ("Prepared an MPDU containing an A-MSDU");
        }
      // else aggregation was not attempted or failed
    }

  if (mpdu == 0)
    {
      mpdu = *peekedIt.it;
      peekedIt.it++;
    }

  // Assign a sequence number if this is not a fragment nor a retransmission
  AssignSequenceNumber (mpdu);
  NS_LOG_DEBUG ("Got MPDU from EDCA queue: " << *mpdu);
  queueIt = peekedIt;

  return mpdu;
}

void
QosTxop::AssignSequenceNumber (Ptr<WifiMacQueueItem> mpdu) const
{
  NS_LOG_FUNCTION (this << *mpdu);

  if (!mpdu->IsFragment () && !mpdu->GetHeader ().IsRetry ())
    {
      uint16_t sequence = m_txMiddle->GetNextSequenceNumberFor (&mpdu->GetHeader ());
      mpdu->GetHeader ().SetSequenceNumber (sequence);
    }
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
      && m_stationManager->NeedRts (frame->GetHeader (), frame->GetSize ()))
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
      CtrlBAckRequestHeader baReqHdr;
      frame->GetPacket ()->PeekHeader (baReqHdr);
      uint8_t tid = baReqHdr.GetTidInfo ();
      params.EnableBlockAck (m_baManager->GetBlockAckType (recipient, tid));
    }

  return params;
}

BlockAckReqType
QosTxop::GetBlockAckReqType (Mac48Address recipient, uint8_t tid) const
{
  return m_baManager->GetBlockAckReqType (recipient, tid);
}

BlockAckType
QosTxop::GetBlockAckType (Mac48Address recipient, uint8_t tid) const
{
  return m_baManager->GetBlockAckType (recipient, tid);
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
QosTxop::NotifyInternalCollision (void)
{
  NS_LOG_FUNCTION (this);

  // For internal collisions occurring with the EDCA access method, the appropriate
  // retry counters (short retry counter for MSDU, A-MSDU, or MMPDU and QSRC[AC] or
  // long retry counter for MSDU, A-MSDU, or MMPDU and QLRC[AC]) are incremented
  // (Sec. 10.22.2.11.1 of 802.11-2016).
  // We do not prepare the PSDU that the AC losing the internal collision would have
  // sent. As an approximation, we consider the frame peeked from the queues of the AC.
  Ptr<const WifiMacQueueItem> mpdu = PeekNextMpdu ();

  if (mpdu != nullptr)
    {
      m_stationManager->ReportDataFailed (mpdu);

      if (!mpdu->GetHeader ().GetAddr1 ().IsGroup () && !m_stationManager->NeedRetransmission (mpdu))
        {
          NS_LOG_DEBUG ("reset DCF");
          m_stationManager->ReportFinalDataFailed (mpdu);
          if (!m_txDroppedCallback.IsNull ())
            {
              m_txDroppedCallback (mpdu->GetPacket ());
            }
          ResetCw ();
          // We have to discard mpdu, but first we have to determine whether mpdu
          // is stored in the Block Ack Manager retransmit queue or in the AC queue
          Mac48Address receiver = mpdu->GetHeader ().GetAddr1 ();
          WifiMacQueue::ConstIterator testIt;
          bool found = false;

          if (mpdu->GetHeader ().IsQosData ()
              && GetBaAgreementEstablished (receiver, mpdu->GetHeader ().GetQosTid ()))
            {
              uint8_t tid = mpdu->GetHeader ().GetQosTid ();
              testIt = m_baManager->GetRetransmitQueue ()->PeekByTidAndAddress (tid, receiver);

              if (testIt != m_baManager->GetRetransmitQueue ()->end ())
                {
                  found = true;
                  // if not null, the test packet must equal the peeked packet
                  NS_ASSERT ((*testIt)->GetPacket () == mpdu->GetPacket ());
                  m_baManager->GetRetransmitQueue ()->Remove (testIt);
                  m_baManager->NotifyDiscardedMpdu (mpdu);
                }
            }

          if (!found)
            {
              if (mpdu->GetHeader ().IsQosData ())
                {
                  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
                  testIt = m_queue->PeekByTidAndAddress (tid, receiver);
                  NS_ASSERT (testIt != m_queue->end () && (*testIt)->GetPacket () == mpdu->GetPacket ());
                  m_queue->Remove (testIt);
                }
              else
                {
                  // the peeked packet is a non-QoS Data frame (e.g., a DELBA Request), hence
                  // it was not peeked by TID, hence it must be the head of the queue
                  Ptr<WifiMacQueueItem> item;
                  item = m_queue->DequeueFirstAvailable (m_qosBlockedDestinations);
                  NS_ASSERT (item != 0 && item->GetPacket () == mpdu->GetPacket ());
                }
            }
        }
      else
        {
          NS_LOG_DEBUG ("Update CW");
          UpdateFailedCw ();
        }
    }

  GenerateBackoff ();
  m_access = NOT_REQUESTED;
  if (HasFramesToTransmit ())
    {
      m_channelAccessManager->RequestAccess (this);
    }
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
      m_stationManager->ReportFinalRtsFailed (m_currentHdr);
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
    }
  else
    {
      UpdateFailedCw ();
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
      m_stationManager->ReportFinalDataFailed (Create<const WifiMacQueueItem> (m_currentPacket,
                                               m_currentHdr));
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
      m_stationManager->ReportDataFailed (Create<const WifiMacQueueItem> (m_currentPacket,
                                          m_currentHdr));
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
          m_stationManager->ReportFinalDataFailed (Create<const WifiMacQueueItem> (m_currentPacket,
                                                   m_currentHdr));
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
          m_stationManager->ReportFinalDataFailed (Create<const WifiMacQueueItem> (m_currentPacket,
                                                   m_currentHdr));
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
          m_stationManager->ReportDataFailed (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr));
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
      && m_access == NOT_REQUESTED)
    {
      Ptr<const WifiMacQueueItem> item;
      if (m_currentPacket != 0)
        {
          item = Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr);
        }
      else
        {
          item = PeekNextMpdu ();
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
      && m_access == NOT_REQUESTED)
    {
      Ptr<const WifiMacQueueItem> item = PeekNextMpdu ();
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

void
QosTxop::NotifyChannelAccessed (Time txopDuration)
{
  NS_LOG_FUNCTION (this << txopDuration);

  NS_ASSERT (txopDuration != Time::Min ());
  m_startTxop = Simulator::Now ();
  m_txopDuration = txopDuration;
  Txop::NotifyChannelAccessed ();
}

bool
QosTxop::IsTxopStarted (void) const
{
  NS_LOG_FUNCTION (this << !m_startTxop.IsZero ());
  return (!m_startTxop.IsZero ());
}

void
QosTxop::NotifyChannelReleased (void)
{
  NS_LOG_FUNCTION (this);

  if (m_startTxop.IsStrictlyPositive ())
    {
      NS_LOG_DEBUG ("Terminating TXOP. Duration = " << Simulator::Now () - m_startTxop);
      m_txopTrace (m_startTxop, Simulator::Now () - m_startTxop);
    }
  m_startTxop = Seconds (0);
  Txop::NotifyChannelReleased ();
}

Time
QosTxop::GetRemainingTxop (void) const
{
  NS_ASSERT (m_startTxop.IsStrictlyPositive ());
  Time remainingTxop = m_txopDuration;
  remainingTxop -= (Simulator::Now () - m_startTxop);
  if (remainingTxop.IsStrictlyNegative ())
    {
      remainingTxop = Seconds (0);
    }
  NS_LOG_FUNCTION (this << remainingTxop);
  return remainingTxop;
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
          && GetBaAgreementEstablished (m_currentHdr.GetAddr1 (), GetTid (m_currentPacket, m_currentHdr))))
    {
      //MSDU is not fragmented when it is transmitted using an HT-immediate or
      //HT-delayed block ack agreement or when it is carried in an A-MPDU.
      return false;
    }
  bool needTxopFragmentation = false;
  if (GetTxopLimit ().IsStrictlyPositive () && m_currentHdr.IsData ())
    {
    }
  return (needTxopFragmentation || m_stationManager->NeedFragmentation (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr)));
}

bool
QosTxop::IsTxopFragmentation (void) const
{
  if (GetTxopLimit ().IsZero ())
    {
      return false;
    }
  if (!m_stationManager->NeedFragmentation (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr))
      || (GetTxopFragmentSize () < m_stationManager->GetFragmentSize (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr), 0)))
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
      size = m_stationManager->GetFragmentSize (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr), m_fragmentNumber);
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
      size = m_stationManager->GetFragmentSize (Create<const WifiMacQueueItem> (m_currentPacket, m_currentHdr), m_fragmentNumber + 1);
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
      offset = m_stationManager->GetFragmentOffset (Create<const WifiMacQueueItem> (m_currentPacket,
                                                    m_currentHdr), m_fragmentNumber);
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
      isLastFragment = m_stationManager->IsLastFragment (Create<const WifiMacQueueItem> (m_currentPacket,
                                                         m_currentHdr), m_fragmentNumber);
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
      NS_LOG_DEBUG ("block ack agreement established with " << recipient << " tid " << +tid);
      // A (destination, TID) pair is "blocked" (i.e., no more packets are sent)
      // when an Add BA Request is sent to the destination. However, when the
      // Add BA Request timer expires, the (destination, TID) pair is "unblocked"
      // and packets to the destination are sent again (under normal ack policy).
      // Thus, there may be a packet needing to be retransmitted when the
      // Add BA Response is received. In this case, the starting sequence number
      // shall be set equal to the sequence number of such packet.
      uint16_t startingSeq = m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient);
      Ptr<const WifiMacQueueItem> peekedItem = PeekNextMpdu (tid, recipient);
      if (peekedItem != 0 && peekedItem->GetHeader ().IsRetry ())
        {
          startingSeq = peekedItem->GetHeader ().GetSequenceNumber ();
        }
      m_baManager->UpdateAgreement (respHdr, recipient, startingSeq);
    }
  else
    {
      NS_LOG_DEBUG ("discard ADDBA response" << recipient);
      m_baManager->NotifyAgreementRejected (recipient, tid);
    }

  if (HasFramesToTransmit () && m_access == NOT_REQUESTED)
    {
      m_channelAccessManager->RequestAccess (this);
    }
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
      || m_stationManager->GetVhtSupported ()
      || m_stationManager->GetHeSupported ())
    {
      /* Block ack setup */
      return true;
    }
  return false;
}

void
QosTxop::CompleteConfig (void)
{
  NS_LOG_FUNCTION (this);
  m_baManager->SetTxMiddle (m_txMiddle);
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

uint16_t
QosTxop::GetBlockAckInactivityTimeout (void) const
{
  return m_blockAckInactivityTimeout;
}

void
QosTxop::DoInitialize (void)
{
  NS_LOG_FUNCTION (this);
  ResetCw ();
  GenerateBackoff ();
}

void
QosTxop::BaTxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txOkCallback.IsNull ())
    {
      m_txOkCallback (hdr);
    }
}

void
QosTxop::BaTxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  if (!m_txFailedCallback.IsNull ())
    {
      m_txFailedCallback (hdr);
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
