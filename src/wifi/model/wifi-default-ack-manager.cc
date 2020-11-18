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

std::unique_ptr<WifiAcknowledgment>
WifiDefaultAckManager::TryAddMpdu (Ptr<const WifiMacQueueItem> mpdu,
                                   const WifiTxParameters& txParams)
{
  NS_LOG_FUNCTION (this << *mpdu << &txParams);

  const WifiMacHeader& hdr = mpdu->GetHeader ();
  Mac48Address receiver = hdr.GetAddr1 ();

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

  WifiNormalAck* acknowledgment = new WifiNormalAck;
  WifiMode mode = txParams.m_txVector.GetMode ();
  acknowledgment->ackTxVector = m_mac->GetWifiRemoteStationManager ()->GetAckTxVector (receiver, mode);
  if (hdr.IsQosData ())
    {
      acknowledgment->SetQosAckPolicy (receiver, hdr.GetQosTid (), WifiMacHeader::NORMAL_ACK);
    }
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
