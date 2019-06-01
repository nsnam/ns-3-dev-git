/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 *         Stefano Avallone <stavallo@unina.it>
 */

#include "ns3/log.h"
#include "ns3/packet.h"
#include "msdu-aggregator.h"
#include "amsdu-subframe-header.h"
#include "qos-txop.h"
#include "mpdu-aggregator.h"
#include "wifi-remote-station-manager.h"
#include "mac-low.h"
#include "wifi-phy.h"
#include "wifi-net-device.h"
#include "ht-capabilities.h"
#include "wifi-mac.h"
#include "wifi-mac-queue.h"
#include "wifi-mac-trailer.h"
#include <algorithm>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("MsduAggregator");

NS_OBJECT_ENSURE_REGISTERED (MsduAggregator);

TypeId
MsduAggregator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::MsduAggregator")
    .SetParent<Object> ()
    .SetGroupName ("Wifi")
    .AddConstructor<MsduAggregator> ()
  ;
  return tid;
}

MsduAggregator::MsduAggregator ()
{
}

MsduAggregator::~MsduAggregator ()
{
}

void
MsduAggregator::SetEdcaQueues (EdcaQueues map)
{
    m_edca = map;
}

uint16_t
MsduAggregator::GetSizeIfAggregated (uint16_t msduSize, uint16_t amsduSize)
{
  NS_LOG_FUNCTION (msduSize << amsduSize);

  // the size of the A-MSDU subframe header is 14 bytes: DA (6), SA (6) and Length (2)
  return amsduSize + CalculatePadding (amsduSize) + 14 + msduSize;
}

void
MsduAggregator::Aggregate (Ptr<const Packet> msdu, Ptr<Packet> amsdu,
                           Mac48Address src, Mac48Address dest) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (amsdu);

  // pad the previous A-MSDU subframe if the A-MSDU is not empty
  if (amsdu->GetSize () > 0)
    {
      uint8_t padding = CalculatePadding (amsdu->GetSize ());

      if (padding)
        {
          Ptr<Packet> pad = Create<Packet> (padding);
          amsdu->AddAtEnd (pad);
        }
    }

  // add A-MSDU subframe header and MSDU
  AmsduSubframeHeader hdr;
  hdr.SetDestinationAddr (dest);
  hdr.SetSourceAddr (src);
  hdr.SetLength (static_cast<uint16_t> (msdu->GetSize ()));

  Ptr<Packet> tmp = msdu->Copy ();
  tmp->AddHeader (hdr);
  amsdu->AddAtEnd (tmp);
}

Ptr<WifiMacQueueItem>
MsduAggregator::GetNextAmsdu (Mac48Address recipient, uint8_t tid,
                              WifiTxVector txVector, uint32_t ampduSize,
                              Time ppduDurationLimit) const
{
  NS_LOG_FUNCTION (recipient << +tid << txVector << ampduSize << ppduDurationLimit);

  /* "The Address 1 field of an MPDU carrying an A-MSDU shall be set to an
   * individual address or to the GCR concealment address" (Section 10.12
   * of 802.11-2016)
   */
  NS_ABORT_MSG_IF (recipient.IsBroadcast (), "Recipient address is broadcast");

  Ptr<QosTxop> qosTxop = m_edca.find (QosUtilsMapTidToAc (tid))->second;
  Ptr<WifiMacQueue> queue = qosTxop->GetWifiMacQueue ();
  WifiMacQueue::ConstIterator peekedIt = queue->PeekByTidAndAddress (tid, recipient);

  if (peekedIt == queue->end ())
    {
      NS_LOG_DEBUG ("No packet with the given TID and address in the queue");
      return 0;
    }

  /* "A STA shall not transmit an A-MSDU within a QoS Data frame under a block
   * ack agreement unless the recipient indicates support for A-MSDU by setting
   * the A-MSDU Supported field to 1 in its BlockAck Parameter Set field of the
   * ADDBA Response frame" (Section 10.12 of 802.11-2016)
   */
  // No check required for now, as we always set the A-MSDU Supported field to 1

  WifiModulationClass modulation = txVector.GetMode ().GetModulationClass ();

  // Get the maximum size of the A-MSDU we can send to the recipient
  uint16_t maxAmsduSize = GetMaxAmsduSize (recipient, tid, modulation);

  if (maxAmsduSize == 0)
    {
      return 0;
    }

  Ptr<Packet> amsdu = Create<Packet> ();
  uint8_t nMsdu = 0;
  WifiMacHeader header = (*peekedIt)->GetHeader ();
  Time tstamp = (*peekedIt)->GetTimeStamp ();
  // We need to keep track of the first MSDU. When it is processed, it is not known
  // if aggregation will succeed or not.
  WifiMacQueue::ConstIterator first = peekedIt;

  // TODO Add support for the Max Number Of MSDUs In A-MSDU field in the Extended
  // Capabilities element sent by the recipient

  while (peekedIt != queue->end ())
    {
      // check if aggregating the peeked MSDU violates the A-MSDU size limit
      uint16_t newAmsduSize = GetSizeIfAggregated ((*peekedIt)->GetPacket ()->GetSize (),
                                                   amsdu->GetSize ());

      if (newAmsduSize > maxAmsduSize)
        {
          NS_LOG_DEBUG ("No other MSDU can be aggregated: maximum A-MSDU size reached");
          break;
        }

      // check if the A-MSDU obtained by aggregating the peeked MSDU violates
      // the A-MPDU size limit or the PPDU duration limit
      if (!qosTxop->GetLow ()->IsWithinSizeAndTimeLimits (header.GetSize () + newAmsduSize + WIFI_MAC_FCS_LENGTH,
                                                          recipient, tid, txVector, ampduSize, ppduDurationLimit))
        {
          NS_LOG_DEBUG ("No other MSDU can be aggregated");
          break;
        }

      // We can now safely aggregate the MSDU to the A-MSDU
      Aggregate ((*peekedIt)->GetPacket (), amsdu,
                 qosTxop->MapSrcAddressForAggregation (header),
                 qosTxop->MapDestAddressForAggregation (header));

      /* "The expiration of the A-MSDU lifetime timer occurs only when the lifetime
       * timer of all of the constituent MSDUs of the A-MSDU have expired" (Section
       * 10.12 of 802.11-2016)
       */
      // The timestamp of the A-MSDU is the most recent among those of the MSDUs
      tstamp = Max (tstamp, (*peekedIt)->GetTimeStamp ());

      // If it is the first MSDU, move to the next one
      if (nMsdu == 0)
        {
          peekedIt++;
        }
      // otherwise, remove it from the queue
      else
        {
          peekedIt = queue->Remove (peekedIt);
        }

      nMsdu++;

      peekedIt = queue->PeekByTidAndAddress (tid, recipient, peekedIt);
    }

  if (nMsdu < 2)
    {
      NS_LOG_DEBUG ("Aggregation failed (could not aggregate at least two MSDUs)");
      return 0;
    }

  // Aggregation succeeded, we have to remove the first MSDU
  queue->Remove (first);

  header.SetQosAmsdu ();
  header.SetAddr3 (qosTxop->GetLow ()->GetBssid ());

  return Create<WifiMacQueueItem> (amsdu, header, tstamp);
}

uint8_t
MsduAggregator::CalculatePadding (uint16_t amsduSize)
{
  return (4 - (amsduSize % 4 )) % 4;
}

uint16_t
MsduAggregator::GetMaxAmsduSize (Mac48Address recipient, uint8_t tid,
                                 WifiModulationClass modulation) const
{
  NS_LOG_FUNCTION (this << recipient << +tid << modulation);

  AcIndex ac = QosUtilsMapTidToAc (tid);
  Ptr<QosTxop> qosTxop = m_edca.find (ac)->second;
  Ptr<WifiNetDevice> device = DynamicCast<WifiNetDevice> (qosTxop->GetLow ()->GetPhy ()->GetDevice ());
  NS_ASSERT (device);
  Ptr<WifiRemoteStationManager> stationManager = device->GetRemoteStationManager ();
  NS_ASSERT (stationManager);

  // Find the A-MSDU max size configured on this device
  UintegerValue size;

  switch (ac)
    {
      case AC_BE:
        device->GetMac ()->GetAttribute ("BE_MaxAmsduSize", size);
        break;
      case AC_BK:
        device->GetMac ()->GetAttribute ("BK_MaxAmsduSize", size);
        break;
      case AC_VI:
        device->GetMac ()->GetAttribute ("VI_MaxAmsduSize", size);
        break;
      case AC_VO:
        device->GetMac ()->GetAttribute ("VO_MaxAmsduSize", size);
        break;
      default:
        NS_ABORT_MSG ("Unknown AC " << ac);
        return 0;
    }

  uint16_t maxAmsduSize = size.Get ();

  if (maxAmsduSize == 0)
    {
      NS_LOG_DEBUG ("A-MSDU Aggregation is disabled on this station for AC " << ac);
      return 0;
    }

  // Retrieve the Capabilities elements advertised by the recipient
  Ptr<const VhtCapabilities> vhtCapabilities = stationManager->GetStationVhtCapabilities (recipient);
  Ptr<const HtCapabilities> htCapabilities = stationManager->GetStationHtCapabilities (recipient);

  if (!htCapabilities)
    {
      /* "A non-DMG STA shall not transmit an A-MSDU to a STA from which it has
       * not received a frame containing an HT Capabilities element" (Section
       * 10.12 of 802.11-2016)
       */
      NS_LOG_DEBUG ("A-MSDU Aggregation disabled because the recipient did not"
                    " send an HT Capabilities element");
      return 0;
    }

  // Determine the constraint imposed by the recipient based on the PPDU
  // format used to transmit the A-MSDU
  if (modulation == WIFI_MOD_CLASS_HE || modulation == WIFI_MOD_CLASS_VHT)
    {
      // the maximum A-MSDU size is indirectly constrained by the maximum MPDU
      // size supported by the recipient and advertised in the VHT Capabilities
      // element (see Table 9-19 of 802.11-2016 as amended by 802.11ax)
      NS_ABORT_MSG_IF (!vhtCapabilities, "VHT Capabilities element not received");

      maxAmsduSize = std::min (maxAmsduSize, static_cast<uint16_t>(vhtCapabilities->GetMaxMpduLength () - 56));
    }
  else if (modulation == WIFI_MOD_CLASS_HT)
    {
      // the maximum A-MSDU size is constrained by the maximum A-MSDU size
      // supported by the recipient and advertised in the HT Capabilities
      // element (see Table 9-19 of 802.11-2016)

      maxAmsduSize = std::min (maxAmsduSize, htCapabilities->GetMaxAmsduLength ());
    }
  else  // non-HT PPDU
    {
      // the maximum A-MSDU size is indirectly constrained by the maximum PSDU size
      // supported by the recipient (see Table 9-19 of 802.11-2016)

      maxAmsduSize = std::min (maxAmsduSize, static_cast<uint16_t>(3839));
    }

  return maxAmsduSize;
}

MsduAggregator::DeaggregatedMsdus
MsduAggregator::Deaggregate (Ptr<Packet> aggregatedPacket)
{
  NS_LOG_FUNCTION_NOARGS ();
  DeaggregatedMsdus set;

  AmsduSubframeHeader hdr;
  Ptr<Packet> extractedMsdu = Create<Packet> ();
  uint32_t maxSize = aggregatedPacket->GetSize ();
  uint16_t extractedLength;
  uint8_t padding;
  uint32_t deserialized = 0;

  while (deserialized < maxSize)
    {
      deserialized += aggregatedPacket->RemoveHeader (hdr);
      extractedLength = hdr.GetLength ();
      extractedMsdu = aggregatedPacket->CreateFragment (0, static_cast<uint32_t> (extractedLength));
      aggregatedPacket->RemoveAtStart (extractedLength);
      deserialized += extractedLength;

      padding = (4 - ((extractedLength + 14) % 4 )) % 4;

      if (padding > 0 && deserialized < maxSize)
        {
          aggregatedPacket->RemoveAtStart (padding);
          deserialized += padding;
        }

      std::pair<Ptr<Packet>, AmsduSubframeHeader> packetHdr (extractedMsdu, hdr);
      set.push_back (packetHdr);
    }
  NS_LOG_INFO ("Deaggreated A-MSDU: extracted " << set.size () << " MSDUs");
  return set;
}

} //namespace ns3
