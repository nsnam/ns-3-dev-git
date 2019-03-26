/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013
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
 * Author: Ghada Badawy <gbadawy@gmail.com>
 */

#include "ns3/log.h"
#include "ns3/packet.h"
#include "mpdu-aggregator.h"
#include "ampdu-subframe-header.h"
#include "wifi-phy.h"
#include "wifi-tx-vector.h"
#include "wifi-remote-station-manager.h"
#include "mac-low.h"
#include "wifi-mac-queue-item.h"
#include "wifi-mac-queue.h"
#include "msdu-aggregator.h"
#include "wifi-net-device.h"
#include "ht-capabilities.h"
#include "vht-capabilities.h"
#include "he-capabilities.h"
#include "wifi-mac.h"
#include "ctrl-headers.h"
#include "wifi-mac-trailer.h"

NS_LOG_COMPONENT_DEFINE ("MpduAggregator");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (MpduAggregator);

TypeId
MpduAggregator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MpduAggregator")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<MpduAggregator> ()
  ;
  return tid;
}

MpduAggregator::MpduAggregator ()
{
}

MpduAggregator::~MpduAggregator ()
{
}

void
MpduAggregator::SetEdcaQueues (EdcaQueues edcaQueues)
{
    m_edca = edcaQueues;
}

void
MpduAggregator::Aggregate (Ptr<const WifiMacQueueItem> mpdu, Ptr<Packet> ampdu, bool isSingle)
{
  NS_LOG_FUNCTION (mpdu << ampdu << isSingle);
  NS_ASSERT (ampdu);
  // if isSingle is true, then ampdu must be empty
  NS_ASSERT (!isSingle || ampdu->GetSize () == 0);

  // pad the previous A-MPDU subframe if the A-MPDU is not empty
  if (ampdu->GetSize () > 0)
    {
      uint8_t padding = CalculatePadding (ampdu->GetSize ());

      if (padding)
        {
          Ptr<Packet> pad = Create<Packet> (padding);
          ampdu->AddAtEnd (pad);
        }
    }

  // add MPDU header and trailer
  Ptr<Packet> tmp = mpdu->GetPacket ()->Copy ();
  tmp->AddHeader (mpdu->GetHeader ());
  AddWifiMacTrailer (tmp);

  // add A-MPDU subframe header and MPDU to the A-MPDU
  AmpduSubframeHeader hdr;
  hdr.SetLength (static_cast<uint16_t> (tmp->GetSize ()));
  if (isSingle)
    {
      hdr.SetEof (1);
    }

  tmp->AddHeader (hdr);
  ampdu->AddAtEnd (tmp);
}

void
MpduAggregator::AddHeaderAndPad (Ptr<Packet> mpdu, bool last, bool isSingleMpdu)
{
  NS_LOG_FUNCTION (mpdu << last << isSingleMpdu);
  AmpduSubframeHeader currentHdr;

  //This is called to prepare packets from the aggregate queue to be sent so no need to check total size since it has already been
  //done before when deciding how many packets to add to the queue
  currentHdr.SetLength (static_cast<uint16_t> (mpdu->GetSize ()));
  if (isSingleMpdu)
    {
      currentHdr.SetEof (1);
    }

  mpdu->AddHeader (currentHdr);
  uint32_t padding = CalculatePadding (mpdu->GetSize ());

  if (padding && !last)
    {
      Ptr<Packet> pad = Create<Packet> (padding);
      mpdu->AddAtEnd (pad);
    }
}

uint32_t
MpduAggregator::GetSizeIfAggregated (uint32_t mpduSize, uint32_t ampduSize)
{
  NS_LOG_FUNCTION (mpduSize << ampduSize);

  return ampduSize + CalculatePadding (ampduSize) + 4 + mpduSize;
}

uint32_t
MpduAggregator::GetMaxAmpduSize (Mac48Address recipient, uint8_t tid,
                                 WifiModulationClass modulation) const
{
  NS_LOG_FUNCTION (this << recipient << +tid << modulation);

  AcIndex ac = QosUtilsMapTidToAc (tid);
  Ptr<QosTxop> qosTxop = m_edca.find (ac)->second;
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (qosTxop->GetLow ()->GetPhy ()->GetDevice ());
  NS_ASSERT (device);
  Ptr<WifiRemoteStationManager> stationManager = device->GetRemoteStationManager ();
  NS_ASSERT (stationManager);

  // Find the A-MPDU max size configured on this device
  UintegerValue size;

  switch (ac)
    {
      case AC_BE:
        device->GetMac ()->GetAttribute ("BE_MaxAmpduSize", size);
        break;
      case AC_BK:
        device->GetMac ()->GetAttribute ("BK_MaxAmpduSize", size);
        break;
      case AC_VI:
        device->GetMac ()->GetAttribute ("VI_MaxAmpduSize", size);
        break;
      case AC_VO:
        device->GetMac ()->GetAttribute ("VO_MaxAmpduSize", size);
        break;
      default:
        NS_ABORT_MSG ("Unknown AC " << ac);
        return 0;
    }

  uint32_t maxAmpduSize = size.Get ();

  if (maxAmpduSize == 0)
    {
      NS_LOG_DEBUG ("A-MPDU Aggregation is disabled on this station for AC " << ac);
      return 0;
    }

  // Retrieve the Capabilities elements advertised by the recipient
  Ptr<const HeCapabilities> heCapabilities = stationManager->GetStationHeCapabilities (recipient);
  Ptr<const VhtCapabilities> vhtCapabilities = stationManager->GetStationVhtCapabilities (recipient);
  Ptr<const HtCapabilities> htCapabilities = stationManager->GetStationHtCapabilities (recipient);

  // Determine the constraint imposed by the recipient based on the PPDU
  // format used to transmit the A-MPDU
  if (modulation == WIFI_MOD_CLASS_HE)
    {
      NS_ABORT_MSG_IF (!heCapabilities, "HE Capabilities element not received");

      maxAmpduSize = std::min (maxAmpduSize, heCapabilities->GetMaxAmpduLength ());
    }
  else if (modulation == WIFI_MOD_CLASS_VHT)
    {
      NS_ABORT_MSG_IF (!vhtCapabilities, "VHT Capabilities element not received");

      maxAmpduSize = std::min (maxAmpduSize, vhtCapabilities->GetMaxAmpduLength ());
    }
  else if (modulation == WIFI_MOD_CLASS_HT)
    {
      NS_ABORT_MSG_IF (!htCapabilities, "HT Capabilities element not received");

      maxAmpduSize = std::min (maxAmpduSize, htCapabilities->GetMaxAmpduLength ());
    }
  else  // non-HT PPDU
    {
      NS_LOG_DEBUG ("A-MPDU aggregation is not available for non-HT PHYs");

      maxAmpduSize = 0;
    }

  return maxAmpduSize;
}

uint8_t
MpduAggregator::CalculatePadding (uint32_t ampduSize)
{
  return (4 - (ampduSize % 4 )) % 4;
}

MpduAggregator::DeaggregatedMpdus
MpduAggregator::Deaggregate (Ptr<Packet> aggregatedPacket)
{
  NS_LOG_FUNCTION_NOARGS ();
  DeaggregatedMpdus set;

  AmpduSubframeHeader hdr;
  Ptr<Packet> extractedMpdu = Create<Packet> ();
  uint32_t maxSize = aggregatedPacket->GetSize ();
  uint16_t extractedLength;
  uint32_t padding;
  uint32_t deserialized = 0;

  while (deserialized < maxSize)
    {
      deserialized += aggregatedPacket->RemoveHeader (hdr);
      extractedLength = hdr.GetLength ();
      extractedMpdu = aggregatedPacket->CreateFragment (0, static_cast<uint32_t> (extractedLength));
      aggregatedPacket->RemoveAtStart (extractedLength);
      deserialized += extractedLength;

      padding = (4 - (extractedLength % 4 )) % 4;

      if (padding > 0 && deserialized < maxSize)
        {
          aggregatedPacket->RemoveAtStart (padding);
          deserialized += padding;
        }

      std::pair<Ptr<Packet>, AmpduSubframeHeader> packetHdr (extractedMpdu, hdr);
      set.push_back (packetHdr);
    }
  NS_LOG_INFO ("Deaggreated A-MPDU: extracted " << set.size () << " MPDUs");
  return set;
}

std::vector<Ptr<WifiMacQueueItem>>
MpduAggregator::GetNextAmpdu (Ptr<const WifiMacQueueItem> mpdu, WifiTxVector txVector,
                              Time ppduDurationLimit) const
{
  std::vector<Ptr<WifiMacQueueItem>> mpduList;
  Mac48Address recipient = mpdu->GetHeader ().GetAddr1 ();

  NS_ASSERT (mpdu->GetHeader ().IsQosData () && !recipient.IsBroadcast ());

  uint8_t tid = GetTid (mpdu->GetPacket (), mpdu->GetHeader ());
  auto edcaIt = m_edca.find (QosUtilsMapTidToAc (tid));
  NS_ASSERT (edcaIt != m_edca.end ());

  WifiModulationClass modulation = txVector.GetMode ().GetModulationClass ();
  uint32_t maxAmpduSize = GetMaxAmpduSize (recipient, tid, modulation);

  if (maxAmpduSize == 0)
    {
      NS_LOG_DEBUG ("A-MPDU aggregation disabled");
      return mpduList;
    }

  //Have to make sure that the block ACK agreement is established before sending an A-MPDU
  if (edcaIt->second->GetBaAgreementEstablished (recipient, tid))
    {
      /* here is performed mpdu aggregation */
      uint16_t startingSequenceNumber = 0;
      uint16_t currentSequenceNumber = 0;
      uint8_t qosPolicy = 0;
      bool retry = false;
      Ptr<const WifiMacQueueItem> nextMpdu = mpdu;
      uint16_t nMpdus = 0;   // number of aggregated MPDUs
      uint16_t maxMpdus = edcaIt->second->GetBaBufferSize (recipient, tid);
      uint32_t currentAmpduSize = 0;
      Ptr<WifiMacQueue> queue = edcaIt->second->GetWifiMacQueue ();
      Ptr<WifiPhy> phy = edcaIt->second->GetLow ()->GetPhy ();

      // Get the maximum PPDU Duration based on the preamble type. It must be a
      // non null value because aggregation is available for HT, VHT and HE, which
      // also provide a limit on the maximum PPDU duration
      Time maxPpduDuration = GetPpduMaxTime (txVector.GetPreambleType ());
      NS_ASSERT (maxPpduDuration.IsStrictlyPositive ());

      // the limit on the PPDU duration is the minimum between the maximum PPDU
      // duration (depending on the PPDU format) and the additional limit provided
      // by the caller (if non-zero)
      if (ppduDurationLimit.IsStrictlyPositive ())
        {
          maxPpduDuration = std::min (maxPpduDuration, ppduDurationLimit);
        }

      while (nextMpdu != 0)
        {
          /* nextMpdu may be any of the following:
            * (a) an A-MSDU (with all the constituent MSDUs dequeued from
            *     the EDCA queue)
            * (b) an MSDU dequeued (1st iteration) or peeked (other iterations)
            *     from the EDCA queue
            * (c) a retransmitted MSDU or A-MSDU dequeued (1st iteration) or
            *     peeked (other iterations) from the BA Manager queue
            * (d) a control or management frame (only 1st iteration, for now)
            */

          // Check if aggregating nextMpdu violates the constraints on the
          // maximum A-MPDU size or on the maximum PPDU duration. This is
          // guaranteed by MsduAggregator::Aggregate in the case of (a)

          uint32_t ampduSize = GetSizeIfAggregated (nextMpdu->GetSize (), currentAmpduSize);

          if (ampduSize > maxAmpduSize ||
              phy->CalculateTxDuration (ampduSize, txVector, phy->GetFrequency ()) > maxPpduDuration)
            {
              NS_LOG_DEBUG ("No other MPDU can be aggregated: " << (ampduSize == 0 ? "size" : "time")
                            << " limit exceeded");
              break;
            }

          // nextMpdu can be aggregated
          nMpdus++;
          currentAmpduSize = ampduSize;

          // Update the header of nextMpdu in case it is not a retransmitted packet
          WifiMacHeader nextHeader = nextMpdu->GetHeader ();

          if (nMpdus == 1)   // first MPDU
            {
              if (!mpdu->GetHeader ().IsBlockAckReq ())
                {
                  if (!mpdu->GetHeader ().IsBlockAck ())
                    {
                      startingSequenceNumber = mpdu->GetHeader ().GetSequenceNumber ();
                      nextHeader.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
                    }
                  else
                    {
                      NS_FATAL_ERROR ("BlockAck is not handled");
                    }

                  currentSequenceNumber = mpdu->GetHeader ().GetSequenceNumber ();
                }
              else
                {
                  qosPolicy = 3; //if the last subframe is block ack req then set ack policy of all frames to blockack
                  CtrlBAckRequestHeader blockAckReq;
                  mpdu->GetPacket ()->PeekHeader (blockAckReq);
                  startingSequenceNumber = blockAckReq.GetStartingSequence ();
                }
              /// \todo We should also handle Ack and BlockAck
            }
          else if (retry == false)
            {
              currentSequenceNumber = edcaIt->second->GetNextSequenceNumberFor (&nextHeader);
              nextHeader.SetSequenceNumber (currentSequenceNumber);
              nextHeader.SetFragmentNumber (0);
              nextHeader.SetNoMoreFragments ();
              nextHeader.SetNoRetry ();
            }

          if (qosPolicy == 0)
            {
              nextHeader.SetQosAckPolicy (WifiMacHeader::NORMAL_ACK);
            }
          else
            {
              nextHeader.SetQosAckPolicy (WifiMacHeader::BLOCK_ACK);
            }

          mpduList.push_back (Create<WifiMacQueueItem> (nextMpdu->GetPacket (), nextHeader,
                                                        nextMpdu->GetTimeStamp ()));

          // Except for the first iteration, complete the processing of the
          // current MPDU, which includes removal from the respective queue
          // (needed for cases (b) and (c) because the packet was just peeked)
          if (nMpdus >= 2 && nextHeader.IsQosData ())
            {
              if (retry)
                {
                  edcaIt->second->RemoveRetransmitPacket (tid, recipient,
                                                          nextHeader.GetSequenceNumber ());
                }
              else if (nextHeader.IsQosData () && !nextHeader.IsQosAmsdu ())
                {
                  queue->Remove (nextMpdu->GetPacket ());
                }
            }

          // If allowed by the BA agreement, get the next MPDU
          nextMpdu = 0;

          if ((nMpdus == 1 || retry)   // check retransmit in the 1st iteration or if retry is true
              && (nextMpdu = edcaIt->second->PeekNextRetransmitPacket (tid, recipient)) != 0)
            {
              retry = true;
              currentSequenceNumber = nextMpdu->GetHeader ().GetSequenceNumber ();

              if (!IsInWindow (currentSequenceNumber, startingSequenceNumber, maxMpdus))
                {
                  break;
                }
            }
          else
            {
              retry = false;
              nextMpdu = queue->PeekByTidAndAddress (tid, recipient);

              if (nextMpdu)
                {
                  currentSequenceNumber = edcaIt->second->PeekNextSequenceNumberFor (&nextMpdu->GetHeader ());

                  if (!IsInWindow (currentSequenceNumber, startingSequenceNumber, maxMpdus))
                    {
                      break;
                    }

                  // Attempt A-MSDU aggregation
                  Ptr<const WifiMacQueueItem> amsdu;
                  if (edcaIt->second->GetLow ()->GetMsduAggregator () != 0)
                    {
                      amsdu = edcaIt->second->GetLow ()->GetMsduAggregator ()->GetNextAmsdu (recipient, tid,
                                                                                              txVector,
                                                                                              currentAmpduSize,
                                                                                              maxPpduDuration);
                      if (amsdu)
                        {
                          nextMpdu = amsdu;
                        }
                    }
                }
            }
        }
      if (mpduList.size () == 1)
        {
          // return an empty vector if it was not possible to aggregate at least two MPDUs
          mpduList.clear ();
        }
    }

  return mpduList;
}

} //namespace ns3
