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
#include "ns3/abort.h"
#include "he-frame-exchange-manager.h"
#include "he-configuration.h"
#include "ns3/wifi-protection-manager.h"
#include "ns3/wifi-ack-manager.h"
#include "ns3/recipient-block-ack-agreement.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/sta-wifi-mac.h"
#include "multi-user-scheduler.h"
#include "ns3/channel-access-manager.h"
#include "ns3/snr-tag.h"
#include "he-phy.h"
#include <algorithm>
#include <functional>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[mac=" << m_self << "] "

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HeFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED (HeFrameExchangeManager);

TypeId
HeFrameExchangeManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HeFrameExchangeManager")
    .SetParent<VhtFrameExchangeManager> ()
    .AddConstructor<HeFrameExchangeManager> ()
    .SetGroupName ("Wifi")
  ;
  return tid;
}

HeFrameExchangeManager::HeFrameExchangeManager ()
  : m_triggerFrameInAmpdu (false)
{
  NS_LOG_FUNCTION (this);
}

HeFrameExchangeManager::~HeFrameExchangeManager ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

uint16_t
HeFrameExchangeManager::GetSupportedBaBufferSize (void) const
{
  NS_ASSERT (m_mac->GetHeConfiguration () != 0);
  if (m_mac->GetHeConfiguration ()->GetMpduBufferSize () > 64)
    {
      return 256;
    }
  return 64;
}

void
HeFrameExchangeManager::SetWifiMac (const Ptr<RegularWifiMac> mac)
{
  m_apMac = DynamicCast<ApWifiMac> (mac);
  m_staMac = DynamicCast<StaWifiMac> (mac);
  VhtFrameExchangeManager::SetWifiMac (mac);
}

void
HeFrameExchangeManager::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  m_apMac = 0;
  m_staMac = 0;
  m_psduMap.clear ();
  m_txParams.Clear ();
  m_muScheduler = 0;
  VhtFrameExchangeManager::DoDispose ();
}

void
HeFrameExchangeManager::SetMultiUserScheduler (const Ptr<MultiUserScheduler> muScheduler)
{
  NS_ASSERT (m_mac);
  NS_ABORT_MSG_IF (m_apMac == 0,
                   "A Multi-User Scheduler can only be aggregated to an AP");
  NS_ABORT_MSG_IF (m_apMac->GetHeConfiguration () == 0,
                   "A Multi-User Scheduler can only be aggregated to an HE AP");
  m_muScheduler = muScheduler;
}

bool
HeFrameExchangeManager::StartFrameExchange (Ptr<QosTxop> edca, Time availableTime, bool initialFrame)
{
  NS_LOG_FUNCTION (this << edca << availableTime << initialFrame);

  MultiUserScheduler::TxFormat txFormat = MultiUserScheduler::SU_TX;
  Ptr<const WifiMacQueueItem> mpdu = edca->PeekNextMpdu ();

  /*
   * We consult the Multi-user Scheduler (if available) to know the type of transmission to make if:
   * - there is no pending BlockAckReq to transmit
   * - either the AC queue is empty (the scheduler might select an UL MU transmission)
   *   or the next frame in the AC queue is a non-broadcast QoS data frame addressed to
   *   a receiver with which a BA agreement has been already established
   */
  if (m_muScheduler != 0
      && edca->GetBaManager ()->GetBar (false) == nullptr
      && (mpdu == 0
          || (mpdu->GetHeader ().IsQosData ()
              && !mpdu->GetHeader ().GetAddr1 ().IsGroup ()
              && edca->GetBaAgreementEstablished (mpdu->GetHeader ().GetAddr1 (), mpdu->GetHeader ().GetQosTid ()))))
    {
      txFormat = m_muScheduler->NotifyAccessGranted (edca, availableTime, initialFrame);
    }

  if (txFormat == MultiUserScheduler::SU_TX)
    {
      return VhtFrameExchangeManager::StartFrameExchange (edca, availableTime, initialFrame);
    }

  if (txFormat == MultiUserScheduler::DL_MU_TX)
    {
      if (m_muScheduler->GetDlMuInfo ().psduMap.empty ())
        {
          NS_LOG_DEBUG ("The Multi-user Scheduler returned DL_MU_TX with empty psduMap, do not transmit");
          return false;
        }

      SendPsduMapWithProtection (m_muScheduler->GetDlMuInfo ().psduMap,
                                 m_muScheduler->GetDlMuInfo ().txParams);
      return true;
    }

  if (txFormat == MultiUserScheduler::UL_MU_TX)
    {
      if (m_muScheduler->GetUlMuInfo ().trigger == nullptr)
        {
          NS_LOG_DEBUG ("The Multi-user Scheduler returned UL_MU_TX with empty Trigger Frame, do not transmit");
          return false;
        }

      NS_ASSERT (m_muScheduler->GetUlMuInfo ().trigger->GetHeader ().IsTrigger ());
      SendPsduMapWithProtection (WifiPsduMap {{SU_STA_ID, GetWifiPsdu (m_muScheduler->GetUlMuInfo ().trigger,
                                                                       m_muScheduler->GetUlMuInfo ().txParams.m_txVector)}},
                                 m_muScheduler->GetUlMuInfo ().txParams);
      return true;
    }

  return false;
}

bool
HeFrameExchangeManager::SendMpduFromBaManager (Ptr<QosTxop> edca, Time availableTime, bool initialFrame)
{
  NS_LOG_FUNCTION (this << edca << availableTime << initialFrame);

  // First, check if there is a BAR to be transmitted
  Ptr<const WifiMacQueueItem> peekedItem = edca->GetBaManager ()->GetBar (false);

  if (peekedItem == 0)
    {
      NS_LOG_DEBUG ("Block Ack Manager returned no frame to send");
      return false;
    }

  if (peekedItem->GetHeader ().IsBlockAckReq ())
    {
      // BlockAckReq are handled by the HT FEM
      return HtFrameExchangeManager::SendMpduFromBaManager (edca, availableTime, initialFrame);
    }

  NS_ASSERT (peekedItem->GetHeader ().IsTrigger ());
  m_triggerFrame = Copy (edca->GetBaManager ()->GetBar ());

  SendPsduMap ();
  return true;
}

void
HeFrameExchangeManager::SendPsduMapWithProtection (WifiPsduMap psduMap, WifiTxParameters& txParams)
{
  NS_LOG_FUNCTION (this << &txParams);

  m_psduMap = std::move (psduMap);
  m_txParams = std::move (txParams);

#ifdef NS3_BUILD_PROFILE_DEBUG
  // If protection is required, the MPDUs must be stored in some queue because
  // they are not put back in a queue if the MU-RTS/CTS exchange fails
  if (m_txParams.m_protection->method != WifiProtection::NONE)
    {
      for (const auto& psdu : psduMap)
        {
          for (const auto& mpdu : *PeekPointer (psdu.second))
            {
              NS_ASSERT (mpdu->GetHeader ().IsCtl () || !mpdu->GetHeader ().HasData () || mpdu->IsQueued ());
            }
        }
    }
#endif

  // Make sure that the acknowledgment time has been computed, so that SendMuRts()
  // can reuse this value.
  NS_ASSERT (m_txParams.m_acknowledgment);

  if (m_txParams.m_acknowledgment->acknowledgmentTime == Time::Min ())
    {
      CalculateAcknowledgmentTime (m_txParams.m_acknowledgment.get ());
    }

  // Set QoS Ack policy
  for (auto& psdu : m_psduMap)
    {
      WifiAckManager::SetQosAckPolicy (psdu.second, m_txParams.m_acknowledgment.get ());
    }

  if (m_txParams.m_protection->method == WifiProtection::NONE)
    {
      SendPsduMap ();
    }
  else
    {
      NS_ABORT_MSG ("Unknown or prohibited protection type: " << m_txParams.m_protection.get ());
    }
}

Ptr<WifiPsdu>
HeFrameExchangeManager::GetPsduTo (Mac48Address to, const WifiPsduMap& psduMap)
{
  auto it = std::find_if (psduMap.begin (), psduMap.end (),
                          [&to] (std::pair<uint16_t, Ptr<WifiPsdu>> psdu)
                            { return psdu.second->GetAddr1 () == to; });
  if (it != psduMap.end ())
    {
      return it->second;
    }
  return nullptr;
}

void
HeFrameExchangeManager::SendPsduMap (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_txParams.m_acknowledgment);
  NS_ASSERT (!m_txTimer.IsRunning ());

  WifiTxTimer::Reason timerType = WifiTxTimer::NOT_RUNNING;  // no timer
  WifiTxVector* responseTxVector = nullptr;
  Ptr<WifiMacQueueItem> mpdu = nullptr;
  Ptr<WifiPsdu> psdu = nullptr;

  /*
   * Acknowledgment via a sequence of BlockAckReq and BlockAck frames
   */
  if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE)
    {
      WifiDlMuBarBaSequence* acknowledgment = static_cast<WifiDlMuBarBaSequence*> (m_txParams.m_acknowledgment.get ());

      // schedule the transmission of required BlockAckReq frames
      for (const auto& psdu : m_psduMap)
        {
          if (acknowledgment->stationsSendBlockAckReqTo.find (psdu.second->GetAddr1 ())
              != acknowledgment->stationsSendBlockAckReqTo.end ())
            {
              // the receiver of this PSDU will receive a BlockAckReq
              std::set<uint8_t> tids = psdu.second->GetTids ();
              NS_ABORT_MSG_IF (tids.size () > 1, "Acknowledgment method incompatible with a Multi-TID A-MPDU");
              uint8_t tid = *tids.begin ();

              NS_ASSERT (m_edca != 0);
              m_edca->ScheduleBar (m_edca->PrepareBlockAckRequest (psdu.second->GetAddr1 (), tid));
            }
        }

      if (!acknowledgment->stationsReplyingWithNormalAck.empty ())
        {
          // a station will reply immediately with a Normal Ack
          timerType = WifiTxTimer::WAIT_NORMAL_ACK_AFTER_DL_MU_PPDU;
          responseTxVector = &acknowledgment->stationsReplyingWithNormalAck.begin ()->second.ackTxVector;
          psdu = GetPsduTo (acknowledgment->stationsReplyingWithNormalAck.begin ()->first, m_psduMap);
          NS_ASSERT (psdu->GetNMpdus () == 1);
          mpdu = *psdu->begin ();
        }
      else if (!acknowledgment->stationsReplyingWithBlockAck.empty ())
        {
          // a station will reply immediately with a Block Ack
          timerType = WifiTxTimer::WAIT_BLOCK_ACK;
          responseTxVector = &acknowledgment->stationsReplyingWithBlockAck.begin ()->second.blockAckTxVector;
          psdu = GetPsduTo (acknowledgment->stationsReplyingWithBlockAck.begin ()->first, m_psduMap);
        }
      // else no station will reply immediately
    }
  /*
   * Acknowledgment via a MU-BAR Trigger Frame sent as single user frame
   */
  else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_TF_MU_BAR)
    {
      WifiDlMuTfMuBar* acknowledgment = static_cast<WifiDlMuTfMuBar*> (m_txParams.m_acknowledgment.get ());

      if (m_triggerFrame == nullptr)
        {
          // we are transmitting the DL MU PPDU and have to schedule the
          // transmission of a MU-BAR Trigger Frame.
          // Create a TXVECTOR by "merging" all the BlockAck TXVECTORs
          std::map<uint16_t, CtrlBAckRequestHeader> recipients;

          NS_ASSERT (!acknowledgment->stationsReplyingWithBlockAck.empty ());
          auto staIt = acknowledgment->stationsReplyingWithBlockAck.begin ();
          WifiTxVector txVector = staIt->second.blockAckTxVector;
          while (staIt != acknowledgment->stationsReplyingWithBlockAck.end ())
            {
              NS_ASSERT (m_apMac != 0);
              uint16_t staId = m_apMac->GetAssociationId (staIt->first);

              txVector.SetHeMuUserInfo (staId, staIt->second.blockAckTxVector.GetHeMuUserInfo (staId));
              recipients.emplace (staId, staIt->second.barHeader);

              staIt++;
            }
          // set the Length field of the response TXVECTOR, which is needed to correctly
          // set the UL Length field of the MU-BAR Trigger Frame
          txVector.SetLength (acknowledgment->ulLength);

          NS_ASSERT (m_edca != 0);
          m_edca->ScheduleBar (PrepareMuBar (txVector, recipients));
        }
      else
        {
          // we are transmitting the MU-BAR following the DL MU PPDU after a SIFS.
          // m_psduMap and m_txParams are still the same as when the DL MU PPDU was sent.
          // record the set of stations expected to send a BlockAck frame
          m_staExpectTbPpduFrom.clear ();
          for (auto& station : acknowledgment->stationsReplyingWithBlockAck)
            {
              m_staExpectTbPpduFrom.insert (station.first);
            }

          Time txDuration = m_phy->CalculateTxDuration (m_triggerFrame->GetSize (),
                                                        acknowledgment->muBarTxVector,
                                                        m_phy->GetPhyBand ());
          // update acknowledgmentTime to correctly set the Duration/ID
          acknowledgment->acknowledgmentTime -= (m_phy->GetSifs () + txDuration);
          m_triggerFrame->GetHeader ().SetDuration (GetPsduDurationId (txDuration, m_txParams));

          responseTxVector = &acknowledgment->stationsReplyingWithBlockAck.begin ()->second.blockAckTxVector;
          Time timeout = txDuration + m_phy->GetSifs () + m_phy->GetSlot ()
                         + m_phy->CalculatePhyPreambleAndHeaderDuration (*responseTxVector);

          m_txTimer.Set (WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU, timeout,
                         &HeFrameExchangeManager::BlockAcksInTbPpduTimeout, this, &m_psduMap,
                         &m_staExpectTbPpduFrom, m_staExpectTbPpduFrom.size ());
          m_channelAccessManager->NotifyAckTimeoutStartNow (timeout);

          ForwardMpduDown (m_triggerFrame, acknowledgment->muBarTxVector);
          return;
        }
    }
  /*
   * Acknowledgment requested by MU-BAR TFs aggregated to PSDUs in the DL MU PPDU
   */
  else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
    {
      WifiDlMuAggregateTf* acknowledgment = static_cast<WifiDlMuAggregateTf*> (m_txParams.m_acknowledgment.get ());

      // record the set of stations expected to send a BlockAck frame
      m_staExpectTbPpduFrom.clear ();

      for (auto& station : acknowledgment->stationsReplyingWithBlockAck)
        {
          m_staExpectTbPpduFrom.insert (station.first);
          // check that the station that is expected to send a BlockAck frame is
          // actually the receiver of a PSDU
          auto psduMapIt = std::find_if (m_psduMap.begin (), m_psduMap.end (),
                                         [&station] (std::pair<uint16_t, Ptr<WifiPsdu>> psdu)
                                           { return psdu.second->GetAddr1 () == station.first; });

          NS_ASSERT (psduMapIt != m_psduMap.end ());
          // add a MU-BAR Trigger Frame to the PSDU
          std::vector<Ptr<WifiMacQueueItem>> mpduList (psduMapIt->second->begin (), psduMapIt->second->end ());
          NS_ASSERT (mpduList.size () == psduMapIt->second->GetNMpdus ());
          // set the Length field of the response TXVECTOR, which is needed to correctly
          // set the UL Length field of the MU-BAR Trigger Frame
          station.second.blockAckTxVector.SetLength (acknowledgment->ulLength);
          mpduList.push_back (PrepareMuBar (station.second.blockAckTxVector,
                                            {{psduMapIt->first, station.second.barHeader}}));
          psduMapIt->second = Create<WifiPsdu> (std::move (mpduList));
        }

      timerType = WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU;
      responseTxVector = &acknowledgment->stationsReplyingWithBlockAck.begin ()->second.blockAckTxVector;
    }
  else
    {
      NS_ABORT_MSG ("Unable to handle the selected acknowledgment method ("
                    << m_txParams.m_acknowledgment.get () << ")");
    }

  // create a map of Ptr<const WifiPsdu>, as required by the PHY
  WifiConstPsduMap psduMap;
  for (const auto& psdu : m_psduMap)
    {
      psduMap.emplace (psdu.first, psdu.second);
    }

  Time txDuration;
  if (m_txParams.m_txVector.IsUlMu ())
    {
      txDuration = HePhy::ConvertLSigLengthToHeTbPpduDuration (m_txParams.m_txVector.GetLength (),
                                                               m_txParams.m_txVector,
                                                               m_phy->GetPhyBand ());
    }
  else
    {
      txDuration = m_phy->CalculateTxDuration (psduMap, m_txParams.m_txVector, m_phy->GetPhyBand ());

      // Set Duration/ID
      Time durationId = GetPsduDurationId (txDuration, m_txParams);
      for (auto& psdu : m_psduMap)
        {
          psdu.second->SetDuration (durationId);
        }
    }

  if (timerType == WifiTxTimer::NOT_RUNNING)
    {
      if (!m_txParams.m_txVector.IsUlMu ())
        {
          Simulator::Schedule (txDuration, &HeFrameExchangeManager::TransmissionSucceeded, this);
        }
    }
  else
    {
      Time timeout = txDuration + m_phy->GetSifs () + m_phy->GetSlot ()
                     + m_phy->CalculatePhyPreambleAndHeaderDuration (*responseTxVector);
      m_channelAccessManager->NotifyAckTimeoutStartNow (timeout);

      // start timer
      switch (timerType)
        {
          case WifiTxTimer::WAIT_NORMAL_ACK_AFTER_DL_MU_PPDU:
            NS_ASSERT (mpdu != nullptr);
            m_txTimer.Set (timerType, timeout, &HeFrameExchangeManager::NormalAckTimeout,
                          this, mpdu, m_txParams.m_txVector);
            break;
          case WifiTxTimer::WAIT_BLOCK_ACK:
            NS_ASSERT (psdu != nullptr);
            m_txTimer.Set (timerType, timeout, &HeFrameExchangeManager::BlockAckTimeout,
                          this, psdu, m_txParams.m_txVector);
            break;
          case WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU:
            m_txTimer.Set (timerType, timeout, &HeFrameExchangeManager::BlockAcksInTbPpduTimeout, this,
                          &m_psduMap, &m_staExpectTbPpduFrom, m_staExpectTbPpduFrom.size ());
            break;
          default:
            NS_ABORT_MSG ("Unknown timer type: " << timerType);
            break;
        }
    }

  // transmit the map of PSDUs
  ForwardPsduMapDown (psduMap, m_txParams.m_txVector);
}

void
HeFrameExchangeManager::ForwardPsduMapDown (WifiConstPsduMap psduMap, WifiTxVector& txVector)
{
  NS_LOG_FUNCTION (this << psduMap << txVector);

  for (const auto& psdu : psduMap)
    {
      NS_LOG_DEBUG ("Transmitting: [STAID=" << psdu.first << ", " << *psdu.second << "]"); 
    }
  NS_LOG_DEBUG ("TXVECTOR: " << txVector);
  for (const auto& psdu : psduMap)
    {
      NotifyTxToEdca (psdu.second);

      // the PSDU is about to be transmitted, we can now dequeue the MPDUs
      DequeuePsdu (psdu.second);
    }
  if (psduMap.size () > 1 || psduMap.begin ()->second->IsAggregate () || psduMap.begin ()->second->IsSingle ())
    {
      txVector.SetAggregation (true);
    }

  m_phy->Send (psduMap, txVector);
}

Ptr<WifiMacQueueItem>
HeFrameExchangeManager::PrepareMuBar (const WifiTxVector& responseTxVector,
                                      std::map<uint16_t, CtrlBAckRequestHeader> recipients) const
{
  NS_LOG_FUNCTION (this << responseTxVector);
  NS_ASSERT (responseTxVector.GetHeMuUserInfoMap ().size () == recipients.size ());
  NS_ASSERT (!recipients.empty ());

  CtrlTriggerHeader muBar (TriggerFrameType::MU_BAR_TRIGGER, responseTxVector);
  SetTargetRssi (muBar);
  Mac48Address rxAddress;

  // Add the Trigger Dependent User Info subfield to every User Info field
  for (auto& userInfo : muBar)
    {
      auto recipientIt = recipients.find (userInfo.GetAid12 ());
      NS_ASSERT (recipientIt != recipients.end ());

      // Store the BAR in the Trigger Dependent User Info subfield
      userInfo.SetMuBarTriggerDepUserInfo (recipientIt->second);
    }

  Ptr<Packet> bar = Create<Packet> ();
  bar->AddHeader (muBar);
  // "If the Trigger frame has one User Info field and the AID12 subfield of the
  // User Info contains the AID of a STA, then the RA field is set to the address
  // of that STA". Otherwise, it is set to the broadcast address (Sec. 9.3.1.23 -
  // 802.11ax amendment draft 3.0)
  if (muBar.GetNUserInfoFields () > 1)
    {
      rxAddress = Mac48Address::GetBroadcast ();
    }
  else
    {
      NS_ASSERT (m_apMac != 0);
      rxAddress = m_apMac->GetStaList ().at (recipients.begin ()->first);
    }

  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_CTL_TRIGGER);
  hdr.SetAddr1 (rxAddress);
  hdr.SetAddr2 (m_self);
  hdr.SetDsNotTo ();
  hdr.SetDsNotFrom ();
  hdr.SetNoRetry ();
  hdr.SetNoMoreFragments ();

  return Create<WifiMacQueueItem> (bar, hdr);
}

void
HeFrameExchangeManager::CalculateAcknowledgmentTime (WifiAcknowledgment* acknowledgment) const
{
  NS_LOG_FUNCTION (this << acknowledgment);
  NS_ASSERT (acknowledgment != nullptr);

  /*
   * Acknowledgment via a sequence of BlockAckReq and BlockAck frames
   */
  if (acknowledgment->method == WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE)
    {
      WifiDlMuBarBaSequence* dlMuBarBaAcknowledgment = static_cast<WifiDlMuBarBaSequence*> (acknowledgment);

      Time duration = Seconds (0);

      // normal ack or implicit BAR policy can be used for (no more than) one receiver
      NS_ABORT_IF (dlMuBarBaAcknowledgment->stationsReplyingWithNormalAck.size ()
                   + dlMuBarBaAcknowledgment->stationsReplyingWithBlockAck.size () > 1);

      if (!dlMuBarBaAcknowledgment->stationsReplyingWithNormalAck.empty ())
        {
          const auto& info = dlMuBarBaAcknowledgment->stationsReplyingWithNormalAck.begin ()->second;
          duration += m_phy->GetSifs ()
                      + m_phy->CalculateTxDuration (GetAckSize (), info.ackTxVector, m_phy->GetPhyBand ());
        }

      if (!dlMuBarBaAcknowledgment->stationsReplyingWithBlockAck.empty ())
        {
          const auto& info = dlMuBarBaAcknowledgment->stationsReplyingWithBlockAck.begin ()->second;
          duration += m_phy->GetSifs ()
                      + m_phy->CalculateTxDuration (GetBlockAckSize (info.baType),
                                                    info.blockAckTxVector, m_phy->GetPhyBand ());
        }

      for (const auto& stations : dlMuBarBaAcknowledgment->stationsSendBlockAckReqTo)
        {
          const auto& info = stations.second;
          duration += m_phy->GetSifs ()
                      + m_phy->CalculateTxDuration (GetBlockAckRequestSize (info.barType),
                                                    info.blockAckReqTxVector, m_phy->GetPhyBand ())
                      + m_phy->GetSifs ()
                      + m_phy->CalculateTxDuration (GetBlockAckSize (info.baType),
                                                    info.blockAckTxVector, m_phy->GetPhyBand ());
        }

      dlMuBarBaAcknowledgment->acknowledgmentTime = duration;
    }
  /*
   * Acknowledgment via a MU-BAR Trigger Frame sent as single user frame
   */
  else if (acknowledgment->method == WifiAcknowledgment::DL_MU_TF_MU_BAR)
    {
      WifiDlMuTfMuBar* dlMuTfMuBarAcknowledgment = static_cast<WifiDlMuTfMuBar*> (acknowledgment);

      Time duration = Seconds (0);

      for (const auto& stations : dlMuTfMuBarAcknowledgment->stationsReplyingWithBlockAck)
        {
          // compute the TX duration of the BlockAck response from this receiver.
          const auto& info = stations.second;
          NS_ASSERT (info.blockAckTxVector.GetHeMuUserInfoMap ().size () == 1);
          uint16_t staId = info.blockAckTxVector.GetHeMuUserInfoMap ().begin ()->first;
          Time currBlockAckDuration = m_phy->CalculateTxDuration (GetBlockAckSize (info.baType),
                                                                  info.blockAckTxVector,
                                                                  m_phy->GetPhyBand (),
                                                                  staId);
          // update the max duration among all the Block Ack responses
          if (currBlockAckDuration > duration)
            {
              duration = currBlockAckDuration;
            }
        }

      // The computed duration may not be coded exactly in the L-SIG length, hence determine
      // the exact duration corresponding to the value that will be coded in this field.
      dlMuTfMuBarAcknowledgment->ulLength = HePhy::ConvertHeTbPpduDurationToLSigLength (duration, m_phy->GetPhyBand ());
      WifiTxVector& txVector = dlMuTfMuBarAcknowledgment->stationsReplyingWithBlockAck.begin ()->second.blockAckTxVector;
      duration = HePhy::ConvertLSigLengthToHeTbPpduDuration (dlMuTfMuBarAcknowledgment->ulLength,
                                                             txVector, m_phy->GetPhyBand ());

      uint32_t muBarSize = GetMuBarSize (dlMuTfMuBarAcknowledgment->barTypes);
      dlMuTfMuBarAcknowledgment->acknowledgmentTime = m_phy->GetSifs ()
                                                      + m_phy->CalculateTxDuration (muBarSize,
                                                                                    dlMuTfMuBarAcknowledgment->muBarTxVector,
                                                                                    m_phy->GetPhyBand ())
                                                      + m_phy->GetSifs () + duration;
    }
  /*
   * Acknowledgment requested by MU-BAR TFs aggregated to PSDUs in the DL MU PPDU
   */
  else if (acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
    {
      WifiDlMuAggregateTf* dlMuAggrTfAcknowledgment = static_cast<WifiDlMuAggregateTf*> (acknowledgment);

      Time duration = Seconds (0);

      for (const auto& stations : dlMuAggrTfAcknowledgment->stationsReplyingWithBlockAck)
        {
          // compute the TX duration of the BlockAck response from this receiver.
          const auto& info = stations.second;
          NS_ASSERT (info.blockAckTxVector.GetHeMuUserInfoMap ().size () == 1);
          uint16_t staId = info.blockAckTxVector.GetHeMuUserInfoMap ().begin ()->first;
          Time currBlockAckDuration = m_phy->CalculateTxDuration (GetBlockAckSize (info.baType),
                                                                  info.blockAckTxVector,
                                                                  m_phy->GetPhyBand (),
                                                                  staId);
          // update the max duration among all the Block Ack responses
          if (currBlockAckDuration > duration)
            {
              duration = currBlockAckDuration;
            }
        }

      // The computed duration may not be coded exactly in the L-SIG length, hence determine
      // the exact duration corresponding to the value that will be coded in this field.
      dlMuAggrTfAcknowledgment->ulLength = HePhy::ConvertHeTbPpduDurationToLSigLength (duration, m_phy->GetPhyBand ());
      WifiTxVector& txVector = dlMuAggrTfAcknowledgment->stationsReplyingWithBlockAck.begin ()->second.blockAckTxVector;
      duration = HePhy::ConvertLSigLengthToHeTbPpduDuration (dlMuAggrTfAcknowledgment->ulLength,
                                                             txVector, m_phy->GetPhyBand ());

      dlMuAggrTfAcknowledgment->acknowledgmentTime = m_phy->GetSifs () + duration;
    }
  else
    {
      VhtFrameExchangeManager::CalculateAcknowledgmentTime (acknowledgment);
    }
}

Time
HeFrameExchangeManager::GetTxDuration (uint32_t ppduPayloadSize, Mac48Address receiver,
                                       const WifiTxParameters& txParams) const
{
  if (!txParams.m_txVector.IsMu ())
    {
      return VhtFrameExchangeManager::GetTxDuration (ppduPayloadSize, receiver, txParams);
    }

  NS_ASSERT_MSG (!txParams.m_txVector.IsDlMu () || m_apMac != 0, "DL MU can be done by an AP");
  NS_ASSERT_MSG (!txParams.m_txVector.IsUlMu () || m_staMac != 0, "UL MU can be done by a STA");

  if (txParams.m_acknowledgment
      && txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
    {
      // we need to account for the size of the aggregated MU-BAR Trigger Frame
      WifiDlMuAggregateTf* acknowledgment = static_cast<WifiDlMuAggregateTf*> (txParams.m_acknowledgment.get ());

      const auto& info = acknowledgment->stationsReplyingWithBlockAck.find (receiver);
      NS_ASSERT (info != acknowledgment->stationsReplyingWithBlockAck.end ());

      ppduPayloadSize = MpduAggregator::GetSizeIfAggregated (info->second.muBarSize, ppduPayloadSize);
    }

  uint16_t staId = (txParams.m_txVector.IsDlMu () ? m_apMac->GetAssociationId (receiver)
                                                  : m_staMac->GetAssociationId ());
  Time psduDuration = m_phy->CalculateTxDuration (ppduPayloadSize, txParams.m_txVector,
                                                  m_phy->GetPhyBand (), staId);

  return std::max (psduDuration, txParams.m_txDuration);
}

void
HeFrameExchangeManager::BlockAcksInTbPpduTimeout (WifiPsduMap* psduMap,
                                                  const std::set<Mac48Address>* staMissedBlockAckFrom,
                                                  std::size_t nSolicitedStations)
{
  NS_LOG_FUNCTION (this << psduMap << nSolicitedStations);

  NS_ASSERT (psduMap != nullptr);
  NS_ASSERT (m_txParams.m_acknowledgment
             && (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF
                 || m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_TF_MU_BAR));

  // This method is called if some station(s) did not send a BlockAck frame in a TB PPDU
  NS_ASSERT (!staMissedBlockAckFrom->empty ());

  bool resetCw;

  if (staMissedBlockAckFrom->size () == nSolicitedStations)
    {
      // no station replied, the transmission failed
      // call ReportDataFailed to increase SRC/LRC
      m_mac->GetWifiRemoteStationManager ()->ReportDataFailed (*psduMap->begin ()->second->begin ());
      resetCw = false;
    }
  else
    {
      // the transmission succeeded
      resetCw = true;
    }

  m_triggerFrame = nullptr;  // this is strictly needed for DL_MU_TF_MU_BAR only

  for (const auto& sta : *staMissedBlockAckFrom)
    {
      Ptr<WifiPsdu> psdu = GetPsduTo (sta, *psduMap);
      NS_ASSERT (psdu != nullptr);
      // If the QSRC[AC] or the QLRC[AC] has reached dot11ShortRetryLimit or dot11LongRetryLimit
      // respectively, CW[AC] shall be reset to CWmin[AC] (sec. 10.22.2.2 of 802.11-2016).
      // We should get that psduResetCw is the same for all PSDUs, but the handling of QSRC/QLRC
      // needs to be aligned to the specifications.
      bool psduResetCw;
      MissedBlockAck (psdu, m_txParams.m_txVector, psduResetCw);
      resetCw = resetCw || psduResetCw;
    }

  NS_ASSERT (m_edca != 0);

  if (resetCw)
    {
      m_edca->ResetCw ();
    }
  else
    {
      m_edca->UpdateFailedCw ();
    }

  if (staMissedBlockAckFrom->size () == nSolicitedStations)
    {
      // no station replied, the transmission failed
      TransmissionFailed ();
    }
  else
    {
      TransmissionSucceeded ();
    }
  m_psduMap.clear ();
}

WifiTxVector
HeFrameExchangeManager::GetHeTbTxVector (CtrlTriggerHeader trigger, Mac48Address triggerSender) const
{
  NS_ASSERT (triggerSender != m_self); //TxPower information is used only by STAs, it is useless for the sending AP (which can directly use CtrlTriggerHeader::GetHeTbTxVector)
  NS_ASSERT (m_staMac != nullptr);
  uint16_t staId = m_staMac->GetAssociationId ();
  auto userInfoIt = trigger.FindUserInfoWithAid (staId);
  NS_ASSERT (userInfoIt != trigger.end ());

  WifiTxVector v = trigger.GetHeTbTxVector (staId);

  Ptr<HeConfiguration> heConfiguration = m_mac->GetHeConfiguration ();
  NS_ASSERT_MSG (heConfiguration != 0, "This STA has to be an HE station to send an HE TB PPDU");
  v.SetBssColor (heConfiguration->GetBssColor ());

  uint8_t powerLevel = m_mac->GetWifiRemoteStationManager ()->GetDefaultTxPowerLevel ();
  /**
   * Get the transmit power to use for an HE TB PPDU
   * considering:
   * - the transmit power used by the AP to send the Trigger Frame (TF),
   *   obtained from the AP TX Power subfield of the Common Info field
   *   of the TF.
   * - the target uplink RSSI expected by the AP for the triggered HE TB PPDU,
   *   obtained from the UL Target RSSI subfield of the User Info field
   *   of the TF.
   * - the RSSI of the PPDU containing the TF, typically logged by the
   *   WifiRemoteStationManager upon reception of the TF from the AP.
   *
   * It is assumed that path loss is symmetric (i.e. uplink path loss is
   * equivalent to the measured downlink path loss);
   *
   * Refer to section 27.3.14.2 (Power pre-correction) of 802.11ax Draft 4.0 for more details.
   */
  int8_t pathLossDb = trigger.GetApTxPower () - static_cast<int8_t> (m_mac->GetWifiRemoteStationManager ()->GetMostRecentRssi (triggerSender)); //cast RSSI to be on equal footing with AP Tx power information
  double reqTxPowerDbm = static_cast<double> (userInfoIt->GetUlTargetRssi () + pathLossDb);

  //Convert the transmit power to a power level
  uint8_t numPowerLevels = m_phy->GetNTxPower ();
  if (numPowerLevels > 1)
    {
      double stepDbm = (m_phy->GetTxPowerEnd () - m_phy->GetTxPowerStart ()) / (numPowerLevels - 1);
      powerLevel = static_cast<uint8_t> (ceil ((reqTxPowerDbm - m_phy->GetTxPowerStart ()) / stepDbm)); //better be slightly above so as to satisfy target UL RSSI
      if (powerLevel > numPowerLevels)
        {
          powerLevel = numPowerLevels; //capping will trigger warning below
        }
    }
  if (reqTxPowerDbm > m_phy->GetPowerDbm (powerLevel))
    {
      NS_LOG_WARN ("The requested power level (" << reqTxPowerDbm << "dBm) cannot be satisfied (max: " << m_phy->GetTxPowerEnd () << "dBm)");
    }
  v.SetTxPowerLevel (powerLevel);
  NS_LOG_LOGIC ("UL power control: "
                << "input {pathLoss=" << pathLossDb << "dB, reqTxPower=" << reqTxPowerDbm << "dBm}"
                << " output {powerLevel=" << +powerLevel << " -> " << m_phy->GetPowerDbm (powerLevel) << "dBm}"
                << " PHY power capa {min=" << m_phy->GetTxPowerStart () << "dBm, max=" << m_phy->GetTxPowerEnd () << "dBm, levels:" << +numPowerLevels << "}");

  return v;
}

void
HeFrameExchangeManager::SetTargetRssi (CtrlTriggerHeader& trigger) const
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_apMac != 0);

  trigger.SetApTxPower (static_cast<int8_t> (m_phy->GetPowerDbm (m_mac->GetWifiRemoteStationManager ()->GetDefaultTxPowerLevel ())));
  for (auto& userInfo : trigger)
    {
      const auto staList = m_apMac->GetStaList ();
      auto itAidAddr = staList.find (userInfo.GetAid12 ());
      NS_ASSERT (itAidAddr != staList.end ());
      int8_t rssi = static_cast<int8_t> (m_mac->GetWifiRemoteStationManager ()->GetMostRecentRssi (itAidAddr->second));
      rssi = (rssi >= -20) ? -20 : ((rssi <= -110) ? -110 : rssi); //cap so as to keep within [-110; -20] dBm
      userInfo.SetUlTargetRssi (rssi);
    }
}

void
HeFrameExchangeManager::ReceiveMpdu (Ptr<WifiMacQueueItem> mpdu, RxSignalInfo rxSignalInfo,
                                     const WifiTxVector& txVector, bool inAmpdu)
{
  // The received MPDU is either broadcast or addressed to this station
  NS_ASSERT (mpdu->GetHeader ().GetAddr1 ().IsBroadcast ()
             || mpdu->GetHeader ().GetAddr1 () == m_self);

  const WifiMacHeader& hdr = mpdu->GetHeader ();

  if (hdr.IsCtl ())
    {
      if (hdr.IsAck () && m_txTimer.IsRunning ()
          && m_txTimer.GetReason () == WifiTxTimer::WAIT_NORMAL_ACK_AFTER_DL_MU_PPDU)
        {
          NS_ASSERT (hdr.GetAddr1 () == m_self);
          NS_ASSERT (m_txParams.m_acknowledgment);
          NS_ASSERT (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE);

          WifiDlMuBarBaSequence* acknowledgment = static_cast<WifiDlMuBarBaSequence*> (m_txParams.m_acknowledgment.get ());
          NS_ASSERT (acknowledgment->stationsReplyingWithNormalAck.size () == 1);
          NS_ASSERT (m_apMac != 0);
          uint16_t staId = m_apMac->GetAssociationId (acknowledgment->stationsReplyingWithNormalAck.begin ()->first);
          auto it = m_psduMap.find (staId);
          NS_ASSERT (it != m_psduMap.end ());
          NS_ASSERT (it->second->GetAddr1 () == acknowledgment->stationsReplyingWithNormalAck.begin ()->first);
          SnrTag tag;
          mpdu->GetPacket ()->PeekPacketTag (tag);
          ReceivedNormalAck (*it->second->begin (), m_txParams.m_txVector, txVector, rxSignalInfo, tag.Get ());
        }
      else if (hdr.IsBlockAck () && m_txTimer.IsRunning ()
               && m_txTimer.GetReason () == WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU)
        {
          Mac48Address sender = hdr.GetAddr2 ();
          NS_LOG_DEBUG ("Received BlockAck in TB PPDU from=" << sender);

          SnrTag tag;
          mpdu->GetPacket ()->PeekPacketTag (tag);

          // notify the Block Ack Manager
          CtrlBAckResponseHeader blockAck;
          mpdu->GetPacket ()->PeekHeader (blockAck);
          uint8_t tid = blockAck.GetTidInfo ();
          GetBaManager (tid)->NotifyGotBlockAck (&blockAck, hdr.GetAddr2 (), rxSignalInfo.snr,
                                                 tag.Get (), m_txParams.m_txVector);

          // remove the sender from the set of stations that are expected to send a BlockAck
          if (m_staExpectTbPpduFrom.erase (sender) == 0)
            {
              NS_LOG_WARN ("Received a BlockAck from an unexpected stations: " << sender);
              return;
            }

          if (m_staExpectTbPpduFrom.empty ())
            {
              // we do not expect any other BlockAck frame
              m_txTimer.Cancel ();
              m_channelAccessManager->NotifyAckTimeoutResetNow ();
              m_triggerFrame = nullptr;  // this is strictly needed for DL_MU_TF_MU_BAR only

              m_edca->ResetCw ();
              m_psduMap.clear ();
              TransmissionSucceeded ();
            }
        }
      else if (hdr.IsTrigger ())
        {
          // Trigger Frames are only processed by STAs
          if (m_staMac == nullptr)
            {
              return;
            }

          // A Trigger Frame in an A-MPDU is processed when the A-MPDU is fully received
          if (inAmpdu)
            {
              m_triggerFrameInAmpdu = true;
              return;
            }

          CtrlTriggerHeader trigger;
          mpdu->GetPacket ()->PeekHeader (trigger);

          if (hdr.GetAddr1 () != m_self
              && (!hdr.GetAddr1 ().IsBroadcast ()
                  || !m_staMac->IsAssociated ()
                  || hdr.GetAddr2 () != m_bssid     // not sent by the AP this STA is associated with
                  || trigger.FindUserInfoWithAid (m_staMac->GetAssociationId ()) == trigger.end ()))
            {
              // not addressed to us
              return;
            }

          uint16_t staId = m_staMac->GetAssociationId ();

          if (trigger.IsMuBar ())
            {
              Mac48Address sender = hdr.GetAddr2 ();
              NS_LOG_DEBUG ("Received MU-BAR Trigger Frame from=" << sender);
              m_mac->GetWifiRemoteStationManager ()->ReportRxOk (sender, rxSignalInfo, txVector);

              auto userInfoIt = trigger.FindUserInfoWithAid (staId);
              NS_ASSERT (userInfoIt != trigger.end ());
              CtrlBAckRequestHeader blockAckReq = userInfoIt->GetMuBarTriggerDepUserInfo ();
              NS_ABORT_MSG_IF (blockAckReq.IsMultiTid (), "Multi-TID BlockAckReq not supported");
              uint8_t tid = blockAckReq.GetTidInfo ();

              auto agreementIt = m_agreements.find ({sender, tid});

              if (agreementIt == m_agreements.end ())
                {
                  NS_LOG_DEBUG ("There's not a valid agreement for this BlockAckReq");
                  return;
                }

              agreementIt->second.NotifyReceivedBar (blockAckReq.GetStartingSequence ());

              NS_LOG_DEBUG ("Schedule Block Ack in TB PPDU");
              Simulator::Schedule (m_phy->GetSifs (), &HeFrameExchangeManager::SendBlockAck, this,
                                  agreementIt->second, hdr.GetDuration (),
                                  GetHeTbTxVector (trigger, hdr.GetAddr2 ()), rxSignalInfo.snr);
            }
        }
      else
        {
          // the received control frame cannot be handled here
          VhtFrameExchangeManager::ReceiveMpdu (mpdu, rxSignalInfo, txVector, inAmpdu);
        }

      // the received control frame has been processed
      return;
    }

  // the received frame cannot be handled here
  VhtFrameExchangeManager::ReceiveMpdu (mpdu, rxSignalInfo, txVector, inAmpdu);;
}

void
HeFrameExchangeManager::EndReceiveAmpdu (Ptr<const WifiPsdu> psdu, const RxSignalInfo& rxSignalInfo,
                                         const WifiTxVector& txVector, const std::vector<bool>& perMpduStatus)
{
  std::set<uint8_t> tids = psdu->GetTids ();

  if (m_triggerFrameInAmpdu)
    {
      // the received A-MPDU contains a Trigger Frame. It is now time to handle it.
      auto psduIt = psdu->begin ();
      while (psduIt != psdu->end ())
        {
          if ((*psduIt)->GetHeader ().IsTrigger ())
            {
              ReceiveMpdu (*psduIt, rxSignalInfo, txVector, false);
            }
          psduIt++;
        }

      m_triggerFrameInAmpdu = false;
      return;
    }

  // the received frame cannot be handled here
  VhtFrameExchangeManager::EndReceiveAmpdu (psdu, rxSignalInfo, txVector, perMpduStatus);
}

} //namespace ns3
