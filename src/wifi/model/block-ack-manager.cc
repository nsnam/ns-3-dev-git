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
  agreement.SetWinEnd ((agreement.GetStartingSequence () + agreement.GetBufferSize () - 1) % 4096);
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

void
BlockAckManager::StorePacket (Ptr<WifiMacQueueItem> mpdu)
{
  NS_LOG_FUNCTION (this << mpdu);
  NS_ASSERT (mpdu->GetHeader ().IsQosData ());

  uint8_t tid = mpdu->GetHeader ().GetQosTid ();
  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();

  AgreementsI it = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (it != m_agreements.end ());
  PacketQueueI queueIt = it->second.second.begin ();
  for (; queueIt != it->second.second.end (); )
    {
      if (((mpdu->GetHeader ().GetSequenceNumber () - (*queueIt)->GetHeader ().GetSequenceNumber () + 4096) % 4096) > 2047)
        {
          queueIt = it->second.second.insert (queueIt, mpdu);
          break;
        }
      else
        {
          queueIt++;
        }
    }
  if (queueIt == it->second.second.end ())
    {
      it->second.second.push_back (mpdu);
    }
}

Ptr<WifiMacQueueItem>
BlockAckManager::GetNextPacket (bool removePacket)
{
  NS_LOG_FUNCTION (this << removePacket);
  Ptr<WifiMacQueueItem> item;
  uint8_t tid;
  Mac48Address recipient;
  CleanupBuffers ();
  if (!m_retryPackets->IsEmpty ())
    {
      NS_LOG_DEBUG ("Retry buffer size is " << m_retryPackets->GetNPackets ());
      WifiMacQueue::ConstIterator it = m_retryPackets->begin ();
      while (it != m_retryPackets->end ())
        {
          if ((*it)->GetHeader ().IsQosData ())
            {
              tid = (*it)->GetHeader ().GetQosTid ();
            }
          else
            {
              NS_FATAL_ERROR ("Packet in blockAck manager retry queue is not Qos Data");
            }
          recipient = (*it)->GetHeader ().GetAddr1 ();
          AgreementsI agreement = m_agreements.find (std::make_pair (recipient, tid));
          NS_ASSERT (agreement != m_agreements.end ());
          if (removePacket)
            {
              if (QosUtilsIsOldPacket (agreement->second.first.GetStartingSequence (), (*it)->GetHeader ().GetSequenceNumber ()))
                {
                  //Standard says the originator should not send a packet with seqnum < winstart
                  NS_LOG_DEBUG ("The Retry packet have sequence number < WinStartO --> Discard "
                                << (*it)->GetHeader ().GetSequenceNumber () << " "
                                << agreement->second.first.GetStartingSequence ());
                  agreement->second.second.remove ((*it));
                  it = m_retryPackets->Remove (it);
                  continue;
                }
              else if (((((*it)->GetHeader ().GetSequenceNumber () - agreement->second.first.GetStartingSequence ()) + 4096) % 4096) > (agreement->second.first.GetBufferSize () - 1))
                {
                  agreement->second.first.SetStartingSequence ((*it)->GetHeader ().GetSequenceNumber ());
                }
            }
          item = *it;
          item->GetHeader ().SetRetry ();
          if (item->GetHeader ().IsQosData ())
            {
              tid = item->GetHeader ().GetQosTid ();
            }
          else
            {
              NS_FATAL_ERROR ("Packet in blockAck manager retry queue is not Qos Data");
            }
          recipient = item->GetHeader ().GetAddr1 ();
          if (!agreement->second.first.IsHtSupported ()
              && (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED)
                  || SwitchToBlockAckIfNeeded (recipient, tid, item->GetHeader ().GetSequenceNumber ())))
            {
              item->GetHeader ().SetQosAckPolicy (WifiMacHeader::BLOCK_ACK);
            }
          else
            {
              /* From section 9.10.3 in IEEE802.11e standard:
               * In order to improve efficiency, originators using the Block Ack facility
               * may send MPDU frames with the Ack Policy subfield in QoS control frames
               * set to Normal Ack if only a few MPDUs are available for transmission.[...]
               * When there are sufficient number of MPDUs, the originator may switch back to
               * the use of Block Ack.
               */
              item->GetHeader ().SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
              if (removePacket)
                {
                  AgreementsI i = m_agreements.find (std::make_pair (recipient, tid));
                  i->second.second.remove (*it);
                }
            }
          if (removePacket)
            {
              NS_LOG_INFO ("Retry packet seq = " << item->GetHeader ().GetSequenceNumber ());
              it = m_retryPackets->Remove (it);
              NS_LOG_DEBUG ("Removed one packet, retry buffer size = " << m_retryPackets->GetNPackets ());
            }
          break;
        }
    }
  return item;
}

Ptr<const WifiMacQueueItem>
BlockAckManager::PeekNextPacketByTidAndAddress (uint8_t tid, Mac48Address recipient)
{
  NS_LOG_FUNCTION (this);
  Ptr<const WifiMacQueueItem> item = 0;
  CleanupBuffers ();
  AgreementsI agreement = m_agreements.find (std::make_pair (recipient, tid));
  NS_ASSERT (agreement != m_agreements.end ());
  WifiMacQueue::ConstIterator it = m_retryPackets->begin ();
  for (; it != m_retryPackets->end (); it++)
    {
      if (!(*it)->GetHeader ().IsQosData ())
        {
          NS_FATAL_ERROR ("Packet in blockAck manager retry queue is not Qos Data");
        }
      if ((*it)->GetHeader ().GetAddr1 () == recipient && (*it)->GetHeader ().GetQosTid () == tid)
        {
          if (QosUtilsIsOldPacket (agreement->second.first.GetStartingSequence (), (*it)->GetHeader ().GetSequenceNumber ()))
            {
              //standard says the originator should not send a packet with seqnum < winstart
              NS_LOG_DEBUG ("The Retry packet have sequence number < WinStartO --> Discard "
                            << (*it)->GetHeader ().GetSequenceNumber () << " "
                            << agreement->second.first.GetStartingSequence ());
              agreement->second.second.remove ((*it));
              it = m_retryPackets->Remove (it);
              it--;
              continue;
            }
          else if (((((*it)->GetHeader ().GetSequenceNumber () - agreement->second.first.GetStartingSequence ()) + 4096) % 4096) > (agreement->second.first.GetBufferSize () - 1))
            {
              agreement->second.first.SetStartingSequence ((*it)->GetHeader ().GetSequenceNumber ());
            }
          WifiMacHeader hdr = (*it)->GetHeader ();
          hdr.SetRetry ();
          item = Create<const WifiMacQueueItem> ((*it)->GetPacket (), hdr, (*it)->GetTimeStamp ());
          NS_LOG_INFO ("Retry packet seq = " << hdr.GetSequenceNumber ());
          if (!agreement->second.first.IsHtSupported ()
              && (ExistsAgreementInState (recipient, tid, OriginatorBlockAckAgreement::ESTABLISHED)
                  || SwitchToBlockAckIfNeeded (recipient, tid, hdr.GetSequenceNumber ())))
            {
              hdr.SetQosAckPolicy (WifiMacHeader::BLOCK_ACK);
            }
          else
            {
              /* From section 9.10.3 in IEEE802.11e standard:
               * In order to improve efficiency, originators using the Block Ack facility
               * may send MPDU frames with the Ack Policy subfield in QoS control frames
               * set to Normal Ack if only a few MPDUs are available for transmission.[...]
               * When there are sufficient number of MPDUs, the originator may switch back to
               * the use of Block Ack.
               */
              hdr.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
            }
          NS_LOG_DEBUG ("Peeked one packet from retry buffer size = " << m_retryPackets->GetNPackets () );
          return item;
        }
    }
  return item;
}

bool
BlockAckManager::RemovePacket (uint8_t tid, Mac48Address recipient, uint16_t seqnumber)
{
  NS_LOG_FUNCTION (this << seqnumber);
  WifiMacQueue::ConstIterator it = m_retryPackets->begin ();
  for (; it != m_retryPackets->end (); it++)
    {
      if (!(*it)->GetHeader ().IsQosData ())
        {
          NS_FATAL_ERROR ("Packet in blockAck manager retry queue is not Qos Data");
        }
      if ((*it)->GetHeader ().GetAddr1 () == recipient && (*it)->GetHeader ().GetQosTid () == tid
          && (*it)->GetHeader ().GetSequenceNumber () == seqnumber)
        {
          WifiMacHeader hdr = (*it)->GetHeader ();
          AgreementsI i = m_agreements.find (std::make_pair (recipient, tid));
          i->second.second.remove ((*it));
          m_retryPackets->Remove (it);
          NS_LOG_DEBUG ("Removed Packet from retry queue = " << hdr.GetSequenceNumber () << " " << +tid << " " << recipient << " Buffer Size = " << m_retryPackets->GetNPackets ());
          return true;
        }
    }
  return false;
}

bool
BlockAckManager::HasBar (Bar &bar, bool remove)
{
  CleanupBuffers ();
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

bool
BlockAckManager::AlreadyExists (uint16_t currentSeq, Mac48Address recipient, uint8_t tid) const
{
  WifiMacQueue::ConstIterator it = m_retryPackets->begin ();
  while (it != m_retryPackets->end ())
    {
      if (!(*it)->GetHeader ().IsQosData ())
        {
          NS_FATAL_ERROR ("Packet in blockAck manager retry queue is not Qos Data");
        }
      if ((*it)->GetHeader ().GetAddr1 () == recipient && (*it)->GetHeader ().GetQosTid () == tid
          && currentSeq == (*it)->GetHeader ().GetSequenceNumber ())
        {
          return true;
        }
      it++;
    }
  return false;
}

void
BlockAckManager::NotifyGotBlockAck (const CtrlBAckResponseHeader *blockAck, Mac48Address recipient, double rxSnr, WifiMode txMode, double dataSnr)
{
  NS_LOG_FUNCTION (this << blockAck << recipient << rxSnr << txMode.GetUniqueName () << dataSnr);
  uint16_t sequenceFirstLost = 0;
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
          if (blockAck->IsBasic ())
            {
              for (PacketQueueI queueIt = it->second.second.begin (); queueIt != queueEnd; )
                {
                  if (blockAck->IsFragmentReceived ((*queueIt)->GetHeader ().GetSequenceNumber (),
                                                    (*queueIt)->GetHeader ().GetFragmentNumber ()))
                    {
                      nSuccessfulMpdus++;
                      RemoveFromRetryQueue (recipient, tid, (*queueIt)->GetHeader ().GetSequenceNumber ());
                      queueIt = it->second.second.erase (queueIt);
                    }
                  else
                    {
                      if (!foundFirstLost)
                        {
                          foundFirstLost = true;
                          sequenceFirstLost = (*queueIt)->GetHeader ().GetSequenceNumber ();
                          (*it).second.first.SetStartingSequence (sequenceFirstLost);
                        }
                      nFailedMpdus++;
                      if (!AlreadyExists ((*queueIt)->GetHeader ().GetSequenceNumber (), recipient, tid))
                        {
                          InsertInRetryQueue (*queueIt);
                        }
                      queueIt++;
                    }
                }
            }
          else if (blockAck->IsCompressed () || blockAck->IsExtendedCompressed ())
            {
              for (PacketQueueI queueIt = it->second.second.begin (); queueIt != queueEnd; )
                {
                  uint16_t currentSeq = (*queueIt)->GetHeader ().GetSequenceNumber ();
                  if (blockAck->IsPacketReceived (currentSeq))
                    {
                      while (queueIt != queueEnd
                             && (*queueIt)->GetHeader ().GetSequenceNumber () == currentSeq)
                        {
                          nSuccessfulMpdus++;
                          if (!m_txOkCallback.IsNull ())
                            {
                              m_txOkCallback ((*queueIt)->GetHeader ());
                            }
                          RemoveFromRetryQueue (recipient, tid, currentSeq);
                          queueIt = it->second.second.erase (queueIt);
                        }
                    }
                  else
                    {
                      if (!foundFirstLost)
                        {
                          foundFirstLost = true;
                          sequenceFirstLost = (*queueIt)->GetHeader ().GetSequenceNumber ();
                          (*it).second.first.SetStartingSequence (sequenceFirstLost);
                        }
                      nFailedMpdus++;
                      if (!m_txFailedCallback.IsNull ())
                        {
                          m_txFailedCallback ((*queueIt)->GetHeader ());
                        }
                      if (!AlreadyExists ((*queueIt)->GetHeader ().GetSequenceNumber (), recipient, tid))
                        {
                          InsertInRetryQueue (*queueIt);
                        }
                      queueIt++;
                    }
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
          //Queue previously transmitted packets that do not already exist in the retry queue.
          //The first packet is not placed in the retry queue since it should be retransmitted by the invoker.
          if (!AlreadyExists (item->GetHeader ().GetSequenceNumber (), recipient, tid))
            {
              InsertInRetryQueue (item);
            }
        }
    }
}

void
BlockAckManager::SetBlockAckType (BlockAckType bAckType)
{
  NS_LOG_FUNCTION (this << bAckType);
  m_blockAckType = bAckType;
}

Ptr<Packet>
BlockAckManager::ScheduleBlockAckReqIfNeeded (Mac48Address recipient, uint8_t tid)
{
  /* This method checks if a BlockAckRequest frame should be send to the recipient station.
     Number of packets under block ack is specified in OriginatorBlockAckAgreement object but sometimes
     this number could be incorrect. In fact is possible that a block ack agreement exists for n
     packets but some of these packets are dropped due to MSDU lifetime expiration.
   */
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
  return bar;
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
      Ptr<Packet> bar = ScheduleBlockAckReqIfNeeded (recipient, tid);
      Bar request (bar, recipient, tid, it->second.first.IsImmediateBlockAck ());
      m_bars.push_back (request);
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
  CleanupBuffers ();
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
  NS_LOG_FUNCTION (this << address << +tid << seq);
  /* remove retry packet iterator if it's present in retry queue */
  WifiMacQueue::ConstIterator it = m_retryPackets->begin ();
  while (it != m_retryPackets->end ())
    {
      if ((*it)->GetHeader ().GetAddr1 () == address
          && (*it)->GetHeader ().GetQosTid () == tid
          && (*it)->GetHeader ().GetSequenceNumber () == seq)
        {
          it = m_retryPackets->Remove (it);
        }
      else
        {
          it++;
        }
    }
}

void
BlockAckManager::CleanupBuffers (void)
{
  NS_LOG_FUNCTION (this);
  for (AgreementsI j = m_agreements.begin (); j != m_agreements.end (); j++)
    {
      bool removed = false;
      if (j->second.second.empty ())
        {
          continue;
        }
      Time now = Simulator::Now ();
      for (PacketQueueI i = j->second.second.begin (); i != j->second.second.end (); )
        {
          if ((*i)->GetTimeStamp () + m_maxDelay > now)
            {
              break;
            }
          else
            {
              RemoveFromRetryQueue (j->second.first.GetPeer (),
                                    j->second.first.GetTid (),
                                    (*i)->GetHeader ().GetSequenceNumber ());
              j->second.first.SetStartingSequence (((*i)->GetHeader ().GetSequenceNumber () + 1)  % 4096);
              i = j->second.second.erase (i);
              removed = true;
              continue;
            }
          i++;
        }
      if (removed)
        {
          Ptr<Packet> bar = ScheduleBlockAckReqIfNeeded (j->second.first.GetPeer (), j->second.first.GetTid ());
          Bar request (bar, j->second.first.GetPeer (), j->second.first.GetTid (), j->second.first.IsImmediateBlockAck ());
          m_bars.push_back (request);
        }
    }
}

void
BlockAckManager::SetMaxPacketDelay (Time maxDelay)
{
  NS_LOG_FUNCTION (this << maxDelay);
  m_maxDelay = maxDelay;
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
BlockAckManager::InsertInRetryQueue (Ptr<WifiMacQueueItem> item)
{
  NS_LOG_INFO ("Adding to retry queue " << *item);
  if (m_retryPackets->GetNPackets () == 0)
    {
      m_retryPackets->Enqueue (item);
    }
  else
    {
      for (WifiMacQueue::ConstIterator it = m_retryPackets->begin (); it != m_retryPackets->end (); )
        {
          if (((item->GetHeader ().GetSequenceNumber () - (*it)->GetHeader ().GetSequenceNumber () + 4096) % 4096) > 2047)
            {
              m_retryPackets->Insert (it, item);
              break;
            }
          else
            {
              it++;
              if (it == m_retryPackets->end ())
                {
                  m_retryPackets->Enqueue (item);
                }
            }
        }
    }
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
