/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009, 2010 MIRKO BANCHI
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "block-ack-manager.h"
#include "wifi-remote-station-manager.h"
#include "ctrl-headers.h"
#include "mgt-headers.h"
#include "wifi-mac-queue.h"
#include "mac-tx-middle.h"
#include "qos-utils.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("BlockAckManager");

Bar::Bar ()
{
  NS_LOG_FUNCTION (this);
}

Bar::Bar (Ptr<const Packet> bar, Mac48Address recipient, uint8_t tid, bool immediate)
  : bar (bar),
    recipient (recipient),
    tid (tid),
    immediate (immediate)
{
  NS_LOG_FUNCTION (this << bar << recipient << +tid << immediate);
}

NS_OBJECT_ENSURE_REGISTERED (BlockAckManager);

TypeId
BlockAckManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::BlockAckManager")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<BlockAckManager> ()
    .AddTraceSource ("AgreementState",
                     "The state of the ADDBA handshake",
                     MakeTraceSourceAccessor (&BlockAckManager::m_agreementState),
                     "ns3::BlockAckManager::AgreementStateTracedCallback")
  ;
  return tid;
}

BlockAckManager::BlockAckManager ()
{
  NS_LOG_FUNCTION (this);
  m_retryPackets = CreateObject<WifiMacQueue> ();
  m_retryPackets->TraceConnectWithoutContext ("Expired", MakeCallback (&BlockAckManager::NotifyDiscardedMpdu, this));
}

BlockAckManager::~BlockAckManager ()
{
  NS_LOG_FUNCTION (this);
  m_queue = 0;
  m_agreements.clear ();
  m_retryPackets = 0;
}

bool
BlockAckManager::ExistsAgreement (Mac48Address recipient, uint8_t tid) const
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  return (m_agreements.find (std::make_pair (recipient, tid)) != m_agreements.end ());
}

bool
BlockAckManager::ExistsAgreementInState (Mac48Address recipient, uint8_t tid,
                                         OriginatorBlockAckAgreement::State state) const
{
  AgreementsCI it;
  it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      switch (state)
        {
        case OriginatorBlockAckAgreement::ESTABLISHED:
          return it->second.first.IsEstablished ();
        case OriginatorBlockAckAgreement::PENDING:
          return it->second.first.IsPending ();
        case OriginatorBlockAckAgreement::REJECTED:
          return it->second.first.IsRejected ();
        case OriginatorBlockAckAgreement::NO_REPLY:
          return it->second.first.IsNoReply ();
        case OriginatorBlockAckAgreement::RESET:
          return it->second.first.IsReset ();
        default:
          NS_FATAL_ERROR ("Invalid state for block ack agreement");
        }
    }
  return false;
}

void
BlockAckManager::CreateAgreement (const MgtAddBaRequestHeader *reqHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << reqHdr << recipient);
  std::pair<Mac48Address, uint8_t> key (recipient, reqHdr->GetTid ());
  OriginatorBlockAckAgreement agreement (recipient, reqHdr->GetTid ());
  agreement.SetStartingSequence (reqHdr->GetStartingSequence ());
  /* For now we assume that originator doesn't use this field. Use of this field
     is mandatory only for recipient */
  agreement.SetBufferSize (reqHdr->GetBufferSize());
  agreement.SetWinEnd ((agreement.GetStartingSequence () + agreement.GetBufferSize () - 1) % SEQNO_SPACE_SIZE);
  agreement.SetTimeout (reqHdr->GetTimeout ());
  agreement.SetAmsduSupport (reqHdr->IsAmsduSupported ());
  agreement.SetHtSupported (m_stationManager->GetHtSupported ());
  if (reqHdr->IsImmediateBlockAck ())
    {
      agreement.SetImmediateBlockAck ();
    }
  else
    {
      agreement.SetDelayedBlockAck ();
    }
  uint8_t tid = reqHdr->GetTid ();
  m_agreementState (Simulator::Now (), recipient, tid, OriginatorBlockAckAgreement::PENDING);
  agreement.SetState (OriginatorBlockAckAgreement::PENDING);
  PacketQueue queue;
  std::pair<OriginatorBlockAckAgreement, PacketQueue> value (agreement, queue);
  if (ExistsAgreement (recipient, tid))
    {
      // Delete agreement if it exists and in RESET state
      NS_ASSERT (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::RESET));
      m_agreements.erase (key);
    }
  m_agreements.insert (std::make_pair (key, value));
  m_blockPackets (recipient, reqHdr->GetTid ());
}

void
BlockAckManager::DestroyAgreement (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      for (WifiMacQueue::ConstIterator i = m_retryPackets->begin (); i != m_retryPackets->end (); )
        {
          if ((*i)->GetHeader ().GetAddr1 () == recipient && (*i)->GetHeader ().GetQosTid () == tid)
            {
              i = m_retryPackets->Remove (i);
            }
          else
            {
              i++;
            }
        }
      m_agreements.erase (it);
      //remove scheduled bar
      for (std::list<Bar>::const_iterator i = m_bars.begin (); i != m_bars.end (); )
        {
          if (i->recipient == recipient && i->tid == tid)
            {
              i = m_bars.erase (i);
            }
          else
            {
              i++;
            }
        }
    }
}

void
BlockAckManager::UpdateAgreement (const MgtAddBaResponseHeader *respHdr, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this << respHdr << recipient);
  uint8_t tid = respHdr->GetTid ();
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      OriginatorBlockAckAgreement& agreement = it->second.first;
      agreement.SetBufferSize (respHdr->GetBufferSize () + 1);
      agreement.SetTimeout (respHdr->GetTimeout ());
      agreement.SetAmsduSupport (respHdr->IsAmsduSupported ());
      // update the starting sequence number because some frames may have been sent
      // under Normal Ack policy after the transmission of the ADDBA Request frame
      agreement.SetStartingSequence (m_txMiddle->GetNextSeqNumberByTidAndAddress (tid, recipient));
      if (respHdr->IsImmediateBlockAck ())
        {
          agreement.SetImmediateBlockAck ();
        }
      else
        {
          agreement.SetDelayedBlockAck ();
        }
      if (!it->second.first.IsEstablished ())
      {
        m_agreementState (Simulator::Now (), recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED);
      }
      agreement.SetState (OriginatorBlockAckAgreement::ESTABLISHED);
      if (agreement.GetTimeout () != 0)
        {
          Time timeout = MicroSeconds (1024 * agreement.GetTimeout ());
          agreement.m_inactivityEvent = Simulator::Schedule (timeout,
                                                             &BlockAckManager::InactivityTimeout,
                                                             this,
                                                             recipient, tid);
        }
    }
  m_unblockPackets (recipient, tid);
}

Ptr<WifiMacQueue>
BlockAckManager::GetRetransmitQueue (void)
{
  return m_retryPackets;
}

void
BlockAckManager::StorePacket (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  NS_ASSERT (mpdu->GetHeader ().IsQosData ());

  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();

  AgreementsI agreementIt = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (agreementIt != m_agreements.end ());

  uint16_t startingSeq = agreementIt->second.first.GetStartingSequence ();
  uint16_t mpduDist = (mpdu->GetHeader ().GetSequenceNumber () - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE;

  if (mpduDist >= SEQNO_SPACE_HALF_SIZE)
    {
      NS_LOG_DEBUG ("Got an old packet. Do nothing");
      return;
    }

  // store the packet and keep the list sorted in increasing order of sequence number
  // with respect to the starting sequence number
  PacketQueueI it = agreementIt->second.second.begin ();
  while (it != agreementIt->second.second.end ())
    {
      if (mpdu->GetHeader ().GetSequenceControl () == (*it)->GetHeader ().GetSequenceControl ())
        {
          NS_LOG_DEBUG ("Packet already in the queue of the BA agreement");
          return;
        }

      uint16_t dist = ((*it)->GetHeader ().GetSequenceNumber () - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE;

      if (mpduDist < dist ||
          (mpduDist == dist && mpdu->GetHeader ().GetFragmentNumber () < (*it)->GetHeader ().GetFragmentNumber ()))
        {
          break;
        }

      it++;
    }
  agreementIt->second.second.insert (it, mpdu);
}

bool
BlockAckManager::HasBar (Bar &bar, bool remove)
{
  if (m_bars.size () > 0)
    {
      bar = m_bars.front ();
      if (remove)
        {
          m_bars.pop_front ();
        }
      return true;
    }
  return false;
}

bool
BlockAckManager::HasPackets (void) const
{
  NS_LOG_FUNCTION (this);
  return (!m_retryPackets->IsEmpty () || m_bars.size () > 0);
}

uint32_t
BlockAckManager::GetNBufferedPackets (Mac48Address recipient, uint8_t tid) const
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  AgreementsCI it = m_agreements.find (std::make_pair (recipient, tid));
  if (it == m_agreements.end ())
    {
      return 0;
    }
  uint32_t nPackets = 0;
  PacketQueueCI queueIt = (*it).second.second.begin ();
  while (queueIt != (*it).second.second.end ())
    {
      uint16_t currentSeq = (*queueIt)->GetHeader ().GetSequenceNumber ();
      nPackets++;
      /* a fragmented packet must be counted as one packet */
      while (queueIt != (*it).second.second.end () && (*queueIt)->GetHeader ().GetSequenceNumber () == currentSeq)
        {
          queueIt++;
        }
    }
  return nPackets;
}

void
BlockAckManager::SetBlockAckThreshold (uint8_t nPackets)
{
  NS_LOG_FUNCTION (this << +nPackets);
  m_blockAckThreshold = nPackets;
}

void
BlockAckManager::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> manager)
{
  NS_LOG_FUNCTION (this << manager);
  m_stationManager = manager;
}

void
BlockAckManager::NotifyGotAck (Ptr<const WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  NS_ASSERT (mpdu->GetHeader ().IsQosData ());

  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();
  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
  NS_ASSERT (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED));

  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());

  // remove the acknowledged frame from the queue of outstanding packets
  PacketQueueI queueIt = it->second.second.begin ();
  while (queueIt != it->second.second.end ())
    {
      if ((*queueIt)->GetHeader ().GetSequenceNumber () == mpdu->GetHeader ().GetSequenceNumber ())
        {
          queueIt = it->second.second.erase (queueIt);
        }
      else
      {
        queueIt++;
      }
    }

  uint16_t startingSeq = it->second.first.GetStartingSequence ();
  if (mpdu->GetHeader ().GetSequenceNumber () == startingSeq)
    {
      // make the transmit window advance
      it->second.first.SetStartingSequence ((startingSeq + 1) % SEQNO_SPACE_SIZE);
    }
}

void
BlockAckManager::NotifyMissedAck (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);
  NS_ASSERT (mpdu->GetHeader ().IsQosData ());

  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();
  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
  NS_ASSERT (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED));

  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());

  // remove the frame from the queue of outstanding packets (it will be re-inserted
  // if retransmitted)
  PacketQueueI queueIt = it->second.second.begin ();
  while (queueIt != it->second.second.end ())
    {
      if ((*queueIt)->GetHeader ().GetSequenceNumber () == mpdu->GetHeader ().GetSequenceNumber ())
        {
          queueIt = it->second.second.erase (queueIt);
        }
      else
      {
        queueIt++;
      }
    }

  // insert in the retransmission queue
  InsertInRetryQueue (mpdu);
}

void
BlockAckManager::NotifyGotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, WifiMode txMode, double dataSnr)
{
  NS_LOG_FUNCTION (this << blockAck << recipient << rxSnr << txMode.GetUniqueName () << dataSnr);
  if (!blockAck->IsMultiTid ())
    {
      uint8_t tid = blockAck->GetTidInfo ();
      if (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED))
        {
          bool foundFirstLost = false;
          uint8_t nSuccessfulMpdus = 0;
          uint8_t nFailedMpdus = 0;
          AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
          PacketQueueI queueEnd = it->second.second.end ();

          if (it->second.first.m_inactivityEvent.IsRunning ())
            {
              /* Upon reception of a block ack frame, the inactivity timer at the
                 originator must be reset.
                 For more details see section 11.5.3 in IEEE802.11e standard */
              it->second.first.m_inactivityEvent.Cancel ();
              Time timeout = MicroSeconds (1024 * it->second.first.GetTimeout ());
              it->second.first.m_inactivityEvent = Simulator::Schedule (timeout,
                                                                        &BlockAckManager::InactivityTimeout,
                                                                        this,
                                                                        recipient, tid);
            }

          uint16_t currentStartingSeq = it->second.first.GetStartingSequence ();
          uint16_t currentSeq = SEQNO_SPACE_SIZE;   // invalid value

          if (blockAck->IsBasic ())
            {
              for (PacketQueueI queueIt = it->second.second.begin (); queueIt != queueEnd; )
                {
                  currentSeq = (*queueIt)->GetHeader ().GetSequenceNumber ();
                  if (blockAck->IsFragmentReceived (currentSeq,
                                                    (*queueIt)->GetHeader ().GetFragmentNumber ()))
                    {
                      nSuccessfulMpdus++;
                    }
                  else if (!QosUtilsIsOldPacket (currentStartingSeq, currentSeq))
                    {
                      if (!foundFirstLost)
                        {
                          foundFirstLost = true;
                          SetStartingSequence (recipient, tid, currentSeq);
                        }
                      nFailedMpdus++;
                      InsertInRetryQueue (*queueIt);
                    }
                  // in any case, this packet is no longer outstanding
                  queueIt = it->second.second.erase (queueIt);
                }
              // If all frames were acknowledged, move the transmit window past the last one
              if (!foundFirstLost && currentSeq != SEQNO_SPACE_SIZE)
                {
                  SetStartingSequence (recipient, tid, (currentSeq + 1) % SEQNO_SPACE_SIZE);
                }
            }
          else if (blockAck->IsCompressed () || blockAck->IsExtendedCompressed ())
            {
              for (PacketQueueI queueIt = it->second.second.begin (); queueIt != queueEnd; )
                {
                  currentSeq = (*queueIt)->GetHeader ().GetSequenceNumber ();
                  if (blockAck->IsPacketReceived (currentSeq))
                    {
                      nSuccessfulMpdus++;
                      if (!m_txOkCallback.IsNull ())
                        {
                          m_txOkCallback ((*queueIt)->GetHeader ());
                        }
                    }
                  else if (!QosUtilsIsOldPacket (currentStartingSeq, currentSeq))
                    {
                      if (!foundFirstLost)
                        {
                          foundFirstLost = true;
                          SetStartingSequence (recipient, tid, currentSeq);
                        }
                      nFailedMpdus++;
                      if (!m_txFailedCallback.IsNull ())
                        {
                          m_txFailedCallback ((*queueIt)->GetHeader ());
                        }
                      InsertInRetryQueue (*queueIt);
                    }
                  // in any case, this packet is no longer outstanding
                  queueIt = it->second.second.erase (queueIt);
                }
              // If all frames were acknowledged, move the transmit window past the last one
              if (!foundFirstLost && currentSeq != SEQNO_SPACE_SIZE)
                {
                  SetStartingSequence (recipient, tid, (currentSeq + 1) % SEQNO_SPACE_SIZE);
                }
            }
          m_stationManager->ReportAmpduTxStatus (recipient, tid, nSuccessfulMpdus, nFailedMpdus, rxSnr, dataSnr);
        }
    }
  else
    {
      //NOT SUPPORTED FOR NOW
      NS_FATAL_ERROR ("Multi-tid block ack is not supported.");
    }
}
  
void
BlockAckManager::NotifyMissedBlockAck (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  if (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED))
    {
      AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
      for (auto& item : it->second.second)
        {
          // Queue previously transmitted packets that do not already exist in the retry queue.
          InsertInRetryQueue (item);
        }
      // remove all packets from the queue of outstanding packets (they will be
      // re-inserted if retransmitted)
      it->second.second.clear ();
    }
}

void
BlockAckManager::DiscardOutstandingMpdus (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  if (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED))
    {
      AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
      it->second.second.clear ();
    }
}

void
BlockAckManager::SetBlockAckType (BlockAckType bAckType)
{
  NS_LOG_FUNCTION (this << bAckType);
  m_blockAckType = bAckType;
}

void
BlockAckManager::NotifyDiscardedMpdu (Ptr<const WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << *mpdu);

  if (!mpdu->GetHeader ().IsQosData ())
    {
      NS_LOG_DEBUG ("Not a QoS Data frame");
      return;
    }

  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();
  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
  if (!ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED))
    {
      NS_LOG_DEBUG ("No established Block Ack agreement");
      return;
    }

  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  uint16_t currStartingSeq = it->second.first.GetStartingSequence ();
  if (QosUtilsIsOldPacket (currStartingSeq, mpdu->GetHeader ().GetSequenceNumber ()))
    {
      NS_LOG_DEBUG ("Discarded an old frame");
      return;
    }

  // advance the transmit window past the discarded mpdu
  SetStartingSequence (recipient, tid, (mpdu->GetHeader ().GetSequenceNumber () + 1) % SEQNO_SPACE_SIZE);

  // schedule a block ack request
  NS_LOG_DEBUG ("Schedule a Block Ack Request for agreement (" << recipient << ", " << +tid << ")");
  ScheduleBlockAckReq (recipient, tid);
}

void
BlockAckManager::ScheduleBlockAckReq (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());

  OriginatorBlockAckAgreement &agreement = (*it).second.first;

  CtrlBAckRequestHeader reqHdr;
  reqHdr.SetType (m_blockAckType);
  reqHdr.SetTidInfo (agreement.GetTid ());
  reqHdr.SetStartingSequence (agreement.GetStartingSequence ());

  Ptr<Packet> bar = Create<Packet> ();
  bar->AddHeader (reqHdr);
  Bar request (bar, recipient, tid, it->second.first.IsImmediateBlockAck ());

  // if a BAR for the given agreement is present, replace it with the new one
  for (std::list<Bar>::const_iterator i = m_bars.begin (); i != m_bars.end (); i++)
    {
      if (i->recipient == recipient && i->tid == tid)
        {
          i = m_bars.erase (i);
          m_bars.insert (i, request);
          return;
        }
    }
  m_bars.push_back (request);
}

void
BlockAckManager::InactivityTimeout (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  m_blockAckInactivityTimeout (recipient, tid, true);
}

void
BlockAckManager::NotifyAgreementEstablished (Mac48Address recipient, uint8_t tid, uint16_t startingSeq)
{
  NS_LOG_FUNCTION (this << recipient << +tid << startingSeq);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  if (!it->second.first.IsEstablished ())
  {
    m_agreementState (Simulator::Now (), recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED);
  }
  it->second.first.SetState (OriginatorBlockAckAgreement::ESTABLISHED);
  it->second.first.SetStartingSequence (startingSeq);
}

void
BlockAckManager::NotifyAgreementRejected (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  if (!it->second.first.IsRejected ())
    {
      m_agreementState (Simulator::Now (), recipient, tid, OriginatorBlockAckAgreement::REJECTED);
    }
  it->second.first.SetState (OriginatorBlockAckAgreement::REJECTED);
}

void
BlockAckManager::NotifyAgreementNoReply (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  if (!it->second.first.IsNoReply ())
    {
      m_agreementState (Simulator::Now (), recipient, tid, OriginatorBlockAckAgreement::NO_REPLY);
    }
  it->second.first.SetState (OriginatorBlockAckAgreement::NO_REPLY);
  m_unblockPackets (recipient, tid);
}

void
BlockAckManager::NotifyAgreementReset (Mac48Address recipient, uint8_t tid)
{
  NS_LOG_FUNCTION (this << recipient << +tid);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  if (!it->second.first.IsReset ())
    {
      m_agreementState (Simulator::Now (), recipient, tid, OriginatorBlockAckAgreement::RESET);
    }
  it->second.first.SetState (OriginatorBlockAckAgreement::RESET);
}

void
BlockAckManager::NotifyMpduTransmission (Mac48Address recipient, uint8_t tid, uint16_t nextSeqNumber, WifiMacHeader::QosAckPolicy policy)
{
  NS_LOG_FUNCTION (this << recipient << +tid << nextSeqNumber);
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  if (policy == WifiMacHeader::BLOCK_ACK)
    {
      ScheduleBlockAckReq (recipient, tid);
    }
}

void
BlockAckManager::SetQueue (const Ptr<WifiMacQueue> queue)
{
  NS_LOG_FUNCTION (this << queue);
  m_queue = queue;
}

bool
BlockAckManager::SwitchToBlockAckIfNeeded (Mac48Address recipient, uint8_t tid, uint16_t startingSeq)
{
  NS_LOG_FUNCTION (this << recipient << +tid << startingSeq);
  NS_ASSERT (!ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::PENDING));
  if (!ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::REJECTED) && ExistsAgreement (recipient, tid))
    {
      uint32_t packets = m_queue->GetNPacketsByTidAndAddress (tid, recipient) +
        GetNBufferedPackets (recipient, tid);
      if (packets >= m_blockAckThreshold)
        {
          NotifyAgreementEstablished (recipient, tid, startingSeq);
          return true;
        }
    }
  return false;
}

bool BlockAckManager::NeedBarRetransmission (uint8_t tid, uint16_t seqNumber, Mac48Address recipient)
{
  //The standard says the BAR gets discarded when all MSDUs lifetime expires
  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  if (QosUtilsIsOldPacket (it->second.first.GetStartingSequence (), seqNumber))
    {
      return false;
    }
  else if (it->second.first.GetTimeout () > 0 && it->second.first.m_inactivityEvent.IsExpired ())
    {
      /*
       * According to "11.5.4 Error recovery upon a peer failure",
       * DELBA should be issued after inactivity timeout,
       * so block ack request should not be retransmitted anymore.
       *
       * Otherwise we risk retransmitting BAR forever if condition
       * above is never met and the STA is not available.
       *
       * See https://www.nsnam.org/bugzilla/show_bug.cgi?id=2928 for details.
       */
      return false;
    }
  else
    {
      return true;
    }
}

void
BlockAckManager::RemoveFromRetryQueue (Mac48Address address, uint8_t tid, uint16_t seq)
{
  RemoveFromRetryQueue (address, tid, seq, seq);
}

void
BlockAckManager::RemoveFromRetryQueue (Mac48Address address, uint8_t tid, uint16_t startSeq, uint16_t endSeq)
{
  NS_LOG_FUNCTION (this << address << +tid << startSeq << endSeq);

  AgreementsI agreementIt = m_agreements.find (std::make_pair (address, tid));
  NS_ASSERT (agreementIt != m_agreements.end ());
  uint16_t startingSeq = agreementIt->second.first.GetStartingSequence ();

  /* remove retry packet iterators if they are present in retry queue */
  WifiMacQueue::ConstIterator it = m_retryPackets->PeekByTidAndAddress (tid, address);

  while (it != m_retryPackets->end ())
    {
      uint16_t itSeq = (*it)->GetHeader ().GetSequenceNumber ();

      if ((itSeq - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE
           >= (startSeq - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE
          && (itSeq - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE
              <= (endSeq - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE)
        {
          NS_LOG_DEBUG ("Removing frame with seqnum = " << itSeq);
          it = m_retryPackets->Remove (it);
          it = m_retryPackets->PeekByTidAndAddress (tid, address, it);
        }
      else
        {
          it = m_retryPackets->PeekByTidAndAddress (tid, address, ++it);
        }
    }
}

void
BlockAckManager::SetStartingSequence (Mac48Address recipient, uint8_t tid, uint16_t startingSeq)
{
  NS_LOG_FUNCTION (this << recipient << +tid << startingSeq);

  AgreementsI agreementIt = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (agreementIt != m_agreements.end ());
  uint16_t currStartingSeq = agreementIt->second.first.GetStartingSequence ();

  NS_ABORT_MSG_IF ((startingSeq - currStartingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE >= SEQNO_SPACE_HALF_SIZE,
                   "The new starting sequence number is an old sequence number");

  if (startingSeq == currStartingSeq)
    {
      return;
    }

  // remove packets that will become old from the retransmission queue
  uint16_t lastRemovedSeq = (startingSeq - 1 + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE;
  RemoveFromRetryQueue (recipient, tid, currStartingSeq, lastRemovedSeq);

  // remove packets that will become old from the queue of outstanding packets
  PacketQueueI it = agreementIt->second.second.begin ();
  while (it != agreementIt->second.second.end ())
    {
      uint16_t itSeq = (*it)->GetHeader ().GetSequenceNumber ();

      if ((itSeq - currStartingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE
           <= (lastRemovedSeq - currStartingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE)
        {
          NS_LOG_DEBUG ("Removing frame with seqnum = " << itSeq);
          it = agreementIt->second.second.erase (it);
        }
      else
        {
          it++;
        }
    }

  // update the starting sequence number
  agreementIt->second.first.SetStartingSequence (startingSeq);
}

void
BlockAckManager::SetBlockAckInactivityCallback (Callback<void, Mac48Address, uint8_t, bool> callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_blockAckInactivityTimeout = callback;
}

void
BlockAckManager::SetBlockDestinationCallback (Callback<void, Mac48Address, uint8_t> callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_blockPackets = callback;
}

void
BlockAckManager::SetUnblockDestinationCallback (Callback<void, Mac48Address, uint8_t> callback)
{
  NS_LOG_FUNCTION (this << &callback);
  m_unblockPackets = callback;
}

void
BlockAckManager::SetTxMiddle (const Ptr<MacTxMiddle> txMiddle)
{
  NS_LOG_FUNCTION (this << txMiddle);
  m_txMiddle = txMiddle;
}

void
BlockAckManager::SetTxOkCallback (TxOk callback)
{
  m_txOkCallback = callback;
}

void
BlockAckManager::SetTxFailedCallback (TxFailed callback)
{
  m_txFailedCallback = callback;
}

void
BlockAckManager::InsertInRetryQueue (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_INFO ("Adding to retry queue " << *mpdu);
  NS_ASSERT (mpdu->GetHeader ().IsQosData ());

  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();

  AgreementsI agreementIt = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (agreementIt != m_agreements.end ());

  uint16_t startingSeq = agreementIt->second.first.GetStartingSequence ();
  uint16_t mpduDist = (mpdu->GetHeader ().GetSequenceNumber () - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE;

  if (mpduDist >= SEQNO_SPACE_HALF_SIZE)
    {
      NS_LOG_DEBUG ("Got an old packet. Do nothing");
      return;
    }

  WifiMacQueue::ConstIterator it = m_retryPackets->PeekByTidAndAddress (tid, recipient);

  while (it != m_retryPackets->end ())
    {
      if (mpdu->GetHeader ().GetSequenceControl () == (*it)->GetHeader ().GetSequenceControl ())
        {
          NS_LOG_DEBUG ("Packet already in the retransmit queue");
          return;
        }

      uint16_t dist = ((*it)->GetHeader ().GetSequenceNumber () - startingSeq + SEQNO_SPACE_SIZE) % SEQNO_SPACE_SIZE;

      if (mpduDist < dist ||
          (mpduDist == dist && mpdu->GetHeader ().GetFragmentNumber () < (*it)->GetHeader ().GetFragmentNumber ()))
        {
          break;
        }

      it = m_retryPackets->PeekByTidAndAddress (tid, recipient, ++it);
    }
  m_retryPackets->Insert (it, mpdu);
}

uint16_t
BlockAckManager::GetRecipientBufferSize (Mac48Address recipient, uint8_t tid) const
{
  uint16_t size = 0;
  AgreementsCI it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      size = it->second.first.GetBufferSize ();
    }
  return size;
}

uint16_t
BlockAckManager::GetOriginatorStartingSequence (Mac48Address recipient, uint8_t tid) const
{
  uint16_t seqNum = 0;
  AgreementsCI it = m_agreements.find (std::make_pair (recipient, tid));
  if (it != m_agreements.end ())
    {
      seqNum = it->second.first.GetStartingSequence ();
    }
  return seqNum;
}

} //namespace ns3
