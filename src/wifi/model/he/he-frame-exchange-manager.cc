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

#include "he-frame-exchange-manager.h"

#include "he-configuration.h"
#include "he-phy.h"
#include "multi-user-scheduler.h"

#include "ns3/abort.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/erp-ofdm-phy.h"
#include "ns3/log.h"
#include "ns3/recipient-block-ack-agreement.h"
#include "ns3/snr-tag.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-mac-trailer.h"

#include <algorithm>
#include <functional>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "][mac=" << m_self << "] "

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("HeFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(HeFrameExchangeManager);

bool
IsTrigger(const WifiPsduMap& psduMap)
{
    return psduMap.size() == 1 && psduMap.cbegin()->first == SU_STA_ID &&
           psduMap.cbegin()->second->GetNMpdus() == 1 &&
           psduMap.cbegin()->second->GetHeader(0).IsTrigger();
}

bool
IsTrigger(const WifiConstPsduMap& psduMap)
{
    return psduMap.size() == 1 && psduMap.cbegin()->first == SU_STA_ID &&
           psduMap.cbegin()->second->GetNMpdus() == 1 &&
           psduMap.cbegin()->second->GetHeader(0).IsTrigger();
}

TypeId
HeFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::HeFrameExchangeManager")
                            .SetParent<VhtFrameExchangeManager>()
                            .AddConstructor<HeFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

HeFrameExchangeManager::HeFrameExchangeManager()
    : m_intraBssNavEnd(0),
      m_triggerFrameInAmpdu(false)
{
    NS_LOG_FUNCTION(this);
}

HeFrameExchangeManager::~HeFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
HeFrameExchangeManager::Reset()
{
    NS_LOG_FUNCTION(this);
    if (m_intraBssNavResetEvent.IsRunning())
    {
        m_intraBssNavResetEvent.Cancel();
    }
    m_intraBssNavEnd = Simulator::Now();
    VhtFrameExchangeManager::Reset();
}

uint16_t
HeFrameExchangeManager::GetSupportedBaBufferSize() const
{
    NS_ASSERT(m_mac->GetHeConfiguration());
    if (m_mac->GetHeConfiguration()->GetMpduBufferSize() > 64)
    {
        return 256;
    }
    return 64;
}

void
HeFrameExchangeManager::SetWifiMac(const Ptr<WifiMac> mac)
{
    m_apMac = DynamicCast<ApWifiMac>(mac);
    m_staMac = DynamicCast<StaWifiMac>(mac);
    VhtFrameExchangeManager::SetWifiMac(mac);
}

void
HeFrameExchangeManager::SetWifiPhy(const Ptr<WifiPhy> phy)
{
    NS_LOG_FUNCTION(this << phy);
    VhtFrameExchangeManager::SetWifiPhy(phy);
    // Cancel intra-BSS NAV reset timer when receiving a frame from the PHY
    phy->TraceConnectWithoutContext("PhyRxPayloadBegin",
                                    Callback<void, WifiTxVector, Time>([this](WifiTxVector, Time) {
                                        m_intraBssNavResetEvent.Cancel();
                                    }));
}

void
HeFrameExchangeManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_apMac = nullptr;
    m_staMac = nullptr;
    m_psduMap.clear();
    m_txParams.Clear();
    m_muScheduler = nullptr;
    m_multiStaBaEvent.Cancel();
    VhtFrameExchangeManager::DoDispose();
}

void
HeFrameExchangeManager::SetMultiUserScheduler(const Ptr<MultiUserScheduler> muScheduler)
{
    NS_ASSERT(m_mac);
    NS_ABORT_MSG_IF(!m_apMac, "A Multi-User Scheduler can only be aggregated to an AP");
    NS_ABORT_MSG_IF(!m_apMac->GetHeConfiguration(),
                    "A Multi-User Scheduler can only be aggregated to an HE AP");
    m_muScheduler = muScheduler;
}

bool
HeFrameExchangeManager::StartFrameExchange(Ptr<QosTxop> edca, Time availableTime, bool initialFrame)
{
    NS_LOG_FUNCTION(this << edca << availableTime << initialFrame);

    MultiUserScheduler::TxFormat txFormat = MultiUserScheduler::SU_TX;
    Ptr<const WifiMpdu> mpdu;

    /*
     * We consult the Multi-user Scheduler (if available) to know the type of transmission to make
     * if:
     * - there is no pending BlockAckReq to transmit
     * - either the AC queue is empty (the scheduler might select an UL MU transmission)
     *   or the next frame in the AC queue is a non-broadcast QoS data frame addressed to
     *   a receiver with which a BA agreement has been already established
     */
    if (m_muScheduler && !GetBar(edca->GetAccessCategory()) &&
        (!(mpdu = edca->PeekNextMpdu(m_linkId)) ||
         (mpdu->GetHeader().IsQosData() && !mpdu->GetHeader().GetAddr1().IsGroup() &&
          m_mac->GetBaAgreementEstablishedAsOriginator(mpdu->GetHeader().GetAddr1(),
                                                       mpdu->GetHeader().GetQosTid()))))
    {
        txFormat = m_muScheduler->NotifyAccessGranted(edca,
                                                      availableTime,
                                                      initialFrame,
                                                      m_allowedWidth,
                                                      m_linkId);
    }

    if (txFormat == MultiUserScheduler::SU_TX)
    {
        return VhtFrameExchangeManager::StartFrameExchange(edca, availableTime, initialFrame);
    }

    if (txFormat == MultiUserScheduler::DL_MU_TX)
    {
        if (m_muScheduler->GetDlMuInfo(m_linkId).psduMap.empty())
        {
            NS_LOG_DEBUG(
                "The Multi-user Scheduler returned DL_MU_TX with empty psduMap, do not transmit");
            return false;
        }

        SendPsduMapWithProtection(m_muScheduler->GetDlMuInfo(m_linkId).psduMap,
                                  m_muScheduler->GetDlMuInfo(m_linkId).txParams);
        return true;
    }

    if (txFormat == MultiUserScheduler::UL_MU_TX)
    {
        auto packet = Create<Packet>();
        packet->AddHeader(m_muScheduler->GetUlMuInfo(m_linkId).trigger);
        auto trigger = Create<WifiMpdu>(packet, m_muScheduler->GetUlMuInfo(m_linkId).macHdr);
        SendPsduMapWithProtection(
            WifiPsduMap{
                {SU_STA_ID,
                 GetWifiPsdu(trigger, m_muScheduler->GetUlMuInfo(m_linkId).txParams.m_txVector)}},
            m_muScheduler->GetUlMuInfo(m_linkId).txParams);
        return true;
    }

    return false;
}

bool
HeFrameExchangeManager::SendMpduFromBaManager(Ptr<WifiMpdu> mpdu,
                                              Time availableTime,
                                              bool initialFrame)
{
    NS_LOG_FUNCTION(this << *mpdu << availableTime << initialFrame);

    // First, check if there is a Trigger Frame to be transmitted
    if (!mpdu->GetHeader().IsTrigger())
    {
        // BlockAckReq are handled by the HT FEM
        return HtFrameExchangeManager::SendMpduFromBaManager(mpdu, availableTime, initialFrame);
    }

    m_triggerFrame = mpdu;

    SendPsduMap();
    return true;
}

void
HeFrameExchangeManager::SendPsduMapWithProtection(WifiPsduMap psduMap, WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);

    m_psduMap = std::move(psduMap);
    m_txParams = std::move(txParams);

    // Make sure that the acknowledgment time has been computed, so that SendMuRts()
    // can reuse this value.
    NS_ASSERT(m_txParams.m_acknowledgment);

    if (m_txParams.m_acknowledgment->acknowledgmentTime == Time::Min())
    {
        CalculateAcknowledgmentTime(m_txParams.m_acknowledgment.get());
    }

    // in case we are sending a Trigger Frame, update the acknowledgment time so that
    // the Duration/ID of the MU-RTS is correctly computed
    if (!m_txParams.m_txVector.IsUlMu() && IsTrigger(m_psduMap))
    {
        NS_ASSERT(m_muScheduler);
        const auto& trigger = m_muScheduler->GetUlMuInfo(m_linkId).trigger;
        NS_ASSERT_MSG(!trigger.IsBasic() || m_txParams.m_acknowledgment->method ==
                                                WifiAcknowledgment::UL_MU_MULTI_STA_BA,
                      "Acknowledgment (" << m_txParams.m_acknowledgment.get()
                                         << ") incompatible with Basic Trigger Frame");
        NS_ASSERT_MSG(!trigger.IsBsrp() ||
                          m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE,
                      "Acknowledgment (" << m_txParams.m_acknowledgment.get()
                                         << ") incompatible with BSRP Trigger Frame");
        // Add a SIFS and the TB PPDU duration to the acknowledgment time of the Trigger Frame
        auto txVector = trigger.GetHeTbTxVector(trigger.begin()->GetAid12());
        m_txParams.m_acknowledgment->acknowledgmentTime +=
            m_phy->GetSifs() + HePhy::ConvertLSigLengthToHeTbPpduDuration(trigger.GetUlLength(),
                                                                          txVector,
                                                                          m_phy->GetPhyBand());
    }

    // Set QoS Ack policy
    for (auto& psdu : m_psduMap)
    {
        WifiAckManager::SetQosAckPolicy(psdu.second, m_txParams.m_acknowledgment.get());
    }

    for (const auto& psdu : m_psduMap)
    {
        for (const auto& mpdu : *PeekPointer(psdu.second))
        {
            if (mpdu->IsQueued())
            {
                mpdu->SetInFlight(m_linkId);
            }
        }
    }

    StartProtection(m_txParams);
}

void
HeFrameExchangeManager::StartProtection(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);

    NS_ABORT_MSG_IF(m_psduMap.size() > 1 &&
                        txParams.m_protection->method == WifiProtection::RTS_CTS,
                    "Cannot use RTS/CTS with MU PPDUs");
    if (txParams.m_protection->method == WifiProtection::MU_RTS_CTS)
    {
        RecordSentMuRtsTo(txParams);
        SendMuRts(txParams);
    }
    else
    {
        VhtFrameExchangeManager::StartProtection(txParams);
    }
}

void
HeFrameExchangeManager::RecordSentMuRtsTo(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);

    NS_ASSERT(txParams.m_protection && txParams.m_protection->method == WifiProtection::MU_RTS_CTS);
    WifiMuRtsCtsProtection* protection =
        static_cast<WifiMuRtsCtsProtection*>(txParams.m_protection.get());

    NS_ASSERT(protection->muRts.IsMuRts());
    NS_ASSERT_MSG(m_apMac, "APs only can send MU-RTS TF");
    const auto& aidAddrMap = m_apMac->GetStaList(m_linkId);
    NS_ASSERT(m_sentRtsTo.empty());

    for (const auto& userInfo : protection->muRts)
    {
        const auto addressIt = aidAddrMap.find(userInfo.GetAid12());
        NS_ASSERT_MSG(addressIt != aidAddrMap.end(), "AID not found");
        m_sentRtsTo.insert(addressIt->second);
    }
}

void
HeFrameExchangeManager::ProtectionCompleted()
{
    NS_LOG_FUNCTION(this);
    if (!m_psduMap.empty())
    {
        m_protectedStas.merge(m_sentRtsTo);
        m_sentRtsTo.clear();
        SendPsduMap();
        return;
    }
    VhtFrameExchangeManager::ProtectionCompleted();
}

Time
HeFrameExchangeManager::GetMuRtsDurationId(uint32_t muRtsSize,
                                           const WifiTxVector& muRtsTxVector,
                                           Time txDuration,
                                           Time response) const
{
    NS_LOG_FUNCTION(this << muRtsSize << muRtsTxVector << txDuration << response);

    if (m_edca->GetTxopLimit(m_linkId).IsZero())
    {
        WifiTxVector txVector;
        txVector.SetMode(GetCtsModeAfterMuRts());
        return VhtFrameExchangeManager::GetRtsDurationId(txVector, txDuration, response);
    }

    // under multiple protection settings, if the TXOP limit is not null, Duration/ID
    // is set to cover the remaining TXOP time (Sec. 9.2.5.2 of 802.11-2016).
    // The TXOP holder may exceed the TXOP limit in some situations (Sec. 10.22.2.8
    // of 802.11-2016)
    return std::max(m_edca->GetRemainingTxop(m_linkId) -
                        m_phy->CalculateTxDuration(muRtsSize, muRtsTxVector, m_phy->GetPhyBand()),
                    Seconds(0));
}

void
HeFrameExchangeManager::SendMuRts(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);
    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_TRIGGER);
    hdr.SetAddr1(Mac48Address::GetBroadcast());
    hdr.SetAddr2(m_self);
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();
    hdr.SetNoRetry();
    hdr.SetNoMoreFragments();

    NS_ASSERT(txParams.m_protection && txParams.m_protection->method == WifiProtection::MU_RTS_CTS);
    WifiMuRtsCtsProtection* protection =
        static_cast<WifiMuRtsCtsProtection*>(txParams.m_protection.get());

    NS_ASSERT(protection->muRts.IsMuRts());
    protection->muRts.SetCsRequired(true);
    Ptr<Packet> payload = Create<Packet>();
    payload->AddHeader(protection->muRts);

    auto mpdu = Create<WifiMpdu>(payload, hdr);

    NS_ASSERT(txParams.m_txDuration != Time::Min());
    mpdu->GetHeader().SetDuration(
        GetMuRtsDurationId(mpdu->GetSize(),
                           protection->muRtsTxVector,
                           txParams.m_txDuration,
                           txParams.m_acknowledgment->acknowledgmentTime));

    // Get the TXVECTOR used by one station to send the CTS response. This is used
    // to compute the preamble duration, so it does not matter which station we choose
    WifiTxVector ctsTxVector =
        GetCtsTxVectorAfterMuRts(protection->muRts, protection->muRts.begin()->GetAid12());

    // After transmitting an MU-RTS frame, the STA shall wait for a CTSTimeout interval of
    // aSIFSTime + aSlotTime + aRxPHYStartDelay (Sec. 27.2.5.2 of 802.11ax D3.0).
    // aRxPHYStartDelay equals the time to transmit the PHY header.
    Time timeout = m_phy->CalculateTxDuration(mpdu->GetSize(),
                                              protection->muRtsTxVector,
                                              m_phy->GetPhyBand()) +
                   m_phy->GetSifs() + m_phy->GetSlot() +
                   m_phy->CalculatePhyPreambleAndHeaderDuration(ctsTxVector);

    NS_ASSERT(!m_txTimer.IsRunning());
    m_txTimer.Set(WifiTxTimer::WAIT_CTS_AFTER_MU_RTS,
                  timeout,
                  &HeFrameExchangeManager::CtsAfterMuRtsTimeout,
                  this,
                  mpdu,
                  protection->muRtsTxVector);
    m_channelAccessManager->NotifyCtsTimeoutStartNow(timeout);

    ForwardMpduDown(mpdu, protection->muRtsTxVector);
}

void
HeFrameExchangeManager::CtsAfterMuRtsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *muRts << txVector);

    if (m_psduMap.empty())
    {
        // A CTS Timeout occurred when protecting a single PSDU that is not included
        // in a DL MU PPDU is handled by the parent classes
        VhtFrameExchangeManager::CtsTimeout(muRts, txVector);
        return;
    }

    m_sentRtsTo.clear();
    for (const auto& psdu : m_psduMap)
    {
        for (const auto& mpdu : *PeekPointer(psdu.second))
        {
            if (mpdu->IsQueued())
            {
                mpdu->ResetInFlight(m_linkId);
            }
        }
    }

    // NOTE Implementation of QSRC[AC] and QLRC[AC] should be improved...
    const auto& hdr = m_psduMap.cbegin()->second->GetHeader(0);
    if (!hdr.GetAddr1().IsGroup())
    {
        GetWifiRemoteStationManager()->ReportRtsFailed(hdr);
    }

    if (!hdr.GetAddr1().IsGroup() &&
        !GetWifiRemoteStationManager()->NeedRetransmission(*m_psduMap.cbegin()->second->begin()))
    {
        NS_LOG_DEBUG("Missed CTS, discard MPDUs");
        GetWifiRemoteStationManager()->ReportFinalRtsFailed(hdr);
        for (const auto& psdu : m_psduMap)
        {
            // Dequeue the MPDUs if they are stored in a queue
            DequeuePsdu(psdu.second);
            for (const auto& mpdu : *PeekPointer(psdu.second))
            {
                NotifyPacketDiscarded(mpdu);
            }
        }
        m_edca->ResetCw(m_linkId);
    }
    else
    {
        NS_LOG_DEBUG("Missed CTS, retransmit MPDUs");
        m_edca->UpdateFailedCw(m_linkId);
    }
    // Make the sequence numbers of the MPDUs available again if the MPDUs have never
    // been transmitted, both in case the MPDUs have been discarded and in case the
    // MPDUs have to be transmitted (because a new sequence number is assigned to
    // MPDUs that have never been transmitted and are selected for transmission)
    for (const auto& [staId, psdu] : m_psduMap)
    {
        ReleaseSequenceNumbers(psdu);
    }
    m_psduMap.clear();
    TransmissionFailed();
}

Ptr<WifiPsdu>
HeFrameExchangeManager::GetPsduTo(Mac48Address to, const WifiPsduMap& psduMap)
{
    auto it = std::find_if(
        psduMap.begin(),
        psduMap.end(),
        [&to](std::pair<uint16_t, Ptr<WifiPsdu>> psdu) { return psdu.second->GetAddr1() == to; });
    if (it != psduMap.end())
    {
        return it->second;
    }
    return nullptr;
}

void
HeFrameExchangeManager::CtsTimeout(Ptr<WifiMpdu> rts, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *rts << txVector);

    if (m_psduMap.empty())
    {
        // A CTS Timeout occurred when protecting a single PSDU that is not included
        // in a DL MU PPDU is handled by the parent classes
        VhtFrameExchangeManager::CtsTimeout(rts, txVector);
        return;
    }

    NS_ABORT_MSG_IF(m_psduMap.size() > 1, "RTS/CTS cannot be used to protect an MU PPDU");
    DoCtsTimeout(m_psduMap.begin()->second);
    m_psduMap.clear();
}

void
HeFrameExchangeManager::SendPsduMap()
{
    NS_LOG_FUNCTION(this);

    NS_ASSERT(m_txParams.m_acknowledgment);
    NS_ASSERT(!m_txTimer.IsRunning());

    WifiTxTimer::Reason timerType = WifiTxTimer::NOT_RUNNING; // no timer
    WifiTxVector* responseTxVector = nullptr;
    Ptr<WifiMpdu> mpdu = nullptr;
    Ptr<WifiPsdu> psdu = nullptr;
    WifiTxVector txVector;

    // Compute the type of TX timer to set depending on the acknowledgment method

    /*
     * Acknowledgment via a sequence of BlockAckReq and BlockAck frames
     */
    if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE)
    {
        WifiDlMuBarBaSequence* acknowledgment =
            static_cast<WifiDlMuBarBaSequence*>(m_txParams.m_acknowledgment.get());

        // schedule the transmission of required BlockAckReq frames
        for (const auto& psdu : m_psduMap)
        {
            if (acknowledgment->stationsSendBlockAckReqTo.find(psdu.second->GetAddr1()) !=
                acknowledgment->stationsSendBlockAckReqTo.end())
            {
                // the receiver of this PSDU will receive a BlockAckReq
                std::set<uint8_t> tids = psdu.second->GetTids();
                NS_ABORT_MSG_IF(tids.size() > 1,
                                "Acknowledgment method incompatible with a Multi-TID A-MPDU");
                uint8_t tid = *tids.begin();

                NS_ASSERT(m_edca);
                auto [reqHdr, hdr] =
                    m_mac->GetQosTxop(tid)->PrepareBlockAckRequest(psdu.second->GetAddr1(), tid);
                m_edca->GetBaManager()->ScheduleBar(reqHdr, hdr);
            }
        }

        if (!acknowledgment->stationsReplyingWithNormalAck.empty())
        {
            // a station will reply immediately with a Normal Ack
            timerType = WifiTxTimer::WAIT_NORMAL_ACK_AFTER_DL_MU_PPDU;
            responseTxVector =
                &acknowledgment->stationsReplyingWithNormalAck.begin()->second.ackTxVector;
            psdu =
                GetPsduTo(acknowledgment->stationsReplyingWithNormalAck.begin()->first, m_psduMap);
            NS_ASSERT(psdu->GetNMpdus() == 1);
            mpdu = *psdu->begin();
        }
        else if (!acknowledgment->stationsReplyingWithBlockAck.empty())
        {
            // a station will reply immediately with a Block Ack
            timerType = WifiTxTimer::WAIT_BLOCK_ACK;
            responseTxVector =
                &acknowledgment->stationsReplyingWithBlockAck.begin()->second.blockAckTxVector;
            psdu =
                GetPsduTo(acknowledgment->stationsReplyingWithBlockAck.begin()->first, m_psduMap);
        }
        // else no station will reply immediately
    }
    /*
     * Acknowledgment via a MU-BAR Trigger Frame sent as single user frame
     */
    else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_TF_MU_BAR)
    {
        WifiDlMuTfMuBar* acknowledgment =
            static_cast<WifiDlMuTfMuBar*>(m_txParams.m_acknowledgment.get());

        if (!m_triggerFrame)
        {
            // we are transmitting the DL MU PPDU and have to schedule the
            // transmission of a MU-BAR Trigger Frame.
            // Create a TRIGVECTOR by "merging" all the BlockAck TXVECTORs
            std::map<uint16_t, CtrlBAckRequestHeader> recipients;

            NS_ASSERT(!acknowledgment->stationsReplyingWithBlockAck.empty());
            auto staIt = acknowledgment->stationsReplyingWithBlockAck.begin();
            m_trigVector = staIt->second.blockAckTxVector;
            while (staIt != acknowledgment->stationsReplyingWithBlockAck.end())
            {
                NS_ASSERT(m_apMac);
                uint16_t staId = m_apMac->GetAssociationId(staIt->first, m_linkId);

                m_trigVector.SetHeMuUserInfo(staId,
                                             staIt->second.blockAckTxVector.GetHeMuUserInfo(staId));
                recipients.emplace(staId, staIt->second.barHeader);

                staIt++;
            }
            // set the Length field of the response TXVECTOR, which is needed to correctly
            // set the UL Length field of the MU-BAR Trigger Frame
            m_trigVector.SetLength(acknowledgment->ulLength);

            NS_ASSERT(m_edca);
            m_edca->GetBaManager()->ScheduleMuBar(PrepareMuBar(m_trigVector, recipients));
        }
        else
        {
            // we are transmitting the MU-BAR following the DL MU PPDU after a SIFS.
            // m_psduMap and m_txParams are still the same as when the DL MU PPDU was sent.
            // record the set of stations expected to send a BlockAck frame
            m_staExpectTbPpduFrom.clear();
            for (auto& station : acknowledgment->stationsReplyingWithBlockAck)
            {
                m_staExpectTbPpduFrom.insert(station.first);
            }

            Ptr<WifiPsdu> triggerPsdu = GetWifiPsdu(m_triggerFrame, acknowledgment->muBarTxVector);
            Time txDuration = m_phy->CalculateTxDuration(triggerPsdu->GetSize(),
                                                         acknowledgment->muBarTxVector,
                                                         m_phy->GetPhyBand());
            // update acknowledgmentTime to correctly set the Duration/ID
            acknowledgment->acknowledgmentTime -= (m_phy->GetSifs() + txDuration);
            m_triggerFrame->GetHeader().SetDuration(GetPsduDurationId(txDuration, m_txParams));

            responseTxVector =
                &acknowledgment->stationsReplyingWithBlockAck.begin()->second.blockAckTxVector;
            Time timeout = txDuration + m_phy->GetSifs() + m_phy->GetSlot() +
                           m_phy->CalculatePhyPreambleAndHeaderDuration(*responseTxVector);

            m_txTimer.Set(WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU,
                          timeout,
                          &HeFrameExchangeManager::BlockAcksInTbPpduTimeout,
                          this,
                          &m_psduMap,
                          &m_staExpectTbPpduFrom,
                          m_staExpectTbPpduFrom.size());
            m_channelAccessManager->NotifyAckTimeoutStartNow(timeout);

            ForwardPsduDown(triggerPsdu, acknowledgment->muBarTxVector);

            // Pass TRIGVECTOR to HE PHY (equivalent to PHY-TRIGGER.request primitive)
            auto hePhy =
                StaticCast<HePhy>(m_phy->GetPhyEntity(responseTxVector->GetModulationClass()));
            hePhy->SetTrigVector(m_trigVector, timeout);

            return;
        }
    }
    /*
     * Acknowledgment requested by MU-BAR TFs aggregated to PSDUs in the DL MU PPDU
     */
    else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
    {
        WifiDlMuAggregateTf* acknowledgment =
            static_cast<WifiDlMuAggregateTf*>(m_txParams.m_acknowledgment.get());

        // record the set of stations expected to send a BlockAck frame
        m_staExpectTbPpduFrom.clear();

        m_trigVector =
            acknowledgment->stationsReplyingWithBlockAck.begin()->second.blockAckTxVector;

        for (auto& station : acknowledgment->stationsReplyingWithBlockAck)
        {
            m_staExpectTbPpduFrom.insert(station.first);
            // check that the station that is expected to send a BlockAck frame is
            // actually the receiver of a PSDU
            auto psduMapIt = std::find_if(m_psduMap.begin(),
                                          m_psduMap.end(),
                                          [&station](std::pair<uint16_t, Ptr<WifiPsdu>> psdu) {
                                              return psdu.second->GetAddr1() == station.first;
                                          });

            NS_ASSERT(psduMapIt != m_psduMap.end());
            // add a MU-BAR Trigger Frame to the PSDU
            std::vector<Ptr<WifiMpdu>> mpduList(psduMapIt->second->begin(),
                                                psduMapIt->second->end());
            NS_ASSERT(mpduList.size() == psduMapIt->second->GetNMpdus());
            // set the Length field of the response TXVECTOR, which is needed to correctly
            // set the UL Length field of the MU-BAR Trigger Frame
            station.second.blockAckTxVector.SetLength(acknowledgment->ulLength);
            mpduList.push_back(PrepareMuBar(station.second.blockAckTxVector,
                                            {{psduMapIt->first, station.second.barHeader}}));
            psduMapIt->second = Create<WifiPsdu>(std::move(mpduList));
            m_trigVector.SetHeMuUserInfo(
                psduMapIt->first,
                station.second.blockAckTxVector.GetHeMuUserInfo(psduMapIt->first));
        }

        timerType = WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU;
        responseTxVector =
            &acknowledgment->stationsReplyingWithBlockAck.begin()->second.blockAckTxVector;
        m_trigVector.SetLength(acknowledgment->ulLength);
    }
    /*
     * Basic Trigger Frame starting an UL MU transmission
     */
    else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::UL_MU_MULTI_STA_BA)
    {
        // the PSDU map being sent must contain a (Basic) Trigger Frame
        NS_ASSERT(IsTrigger(m_psduMap));
        mpdu = *m_psduMap.begin()->second->begin();

        WifiUlMuMultiStaBa* acknowledgment =
            static_cast<WifiUlMuMultiStaBa*>(m_txParams.m_acknowledgment.get());

        // record the set of stations solicited by this Trigger Frame
        m_staExpectTbPpduFrom.clear();

        for (const auto& station : acknowledgment->stationsReceivingMultiStaBa)
        {
            m_staExpectTbPpduFrom.insert(station.first.first);
        }

        // Reset stationsReceivingMultiStaBa, which will be filled as soon as
        // TB PPDUs are received
        acknowledgment->stationsReceivingMultiStaBa.clear();
        acknowledgment->baType.m_bitmapLen.clear();

        timerType = WifiTxTimer::WAIT_TB_PPDU_AFTER_BASIC_TF;
        responseTxVector = &acknowledgment->tbPpduTxVector;
        m_trigVector = GetTrigVector(m_muScheduler->GetUlMuInfo(m_linkId).trigger);
    }
    /*
     * BSRP Trigger Frame
     */
    else if (m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE &&
             !m_txParams.m_txVector.IsUlMu() && IsTrigger(m_psduMap))
    {
        CtrlTriggerHeader& trigger = m_muScheduler->GetUlMuInfo(m_linkId).trigger;
        NS_ASSERT(trigger.IsBsrp());
        NS_ASSERT(m_apMac);

        // record the set of stations solicited by this Trigger Frame
        m_staExpectTbPpduFrom.clear();

        for (const auto& userInfo : trigger)
        {
            auto staIt = m_apMac->GetStaList(m_linkId).find(userInfo.GetAid12());
            NS_ASSERT(staIt != m_apMac->GetStaList(m_linkId).end());
            m_staExpectTbPpduFrom.insert(staIt->second);
        }

        timerType = WifiTxTimer::WAIT_QOS_NULL_AFTER_BSRP_TF;
        txVector = trigger.GetHeTbTxVector(trigger.begin()->GetAid12());
        responseTxVector = &txVector;
        m_trigVector = GetTrigVector(m_muScheduler->GetUlMuInfo(m_linkId).trigger);
    }
    /*
     * TB PPDU solicited by a Basic Trigger Frame
     */
    else if (m_txParams.m_txVector.IsUlMu() &&
             m_txParams.m_acknowledgment->method == WifiAcknowledgment::ACK_AFTER_TB_PPDU)
    {
        NS_ASSERT(m_psduMap.size() == 1);
        timerType = WifiTxTimer::WAIT_BLOCK_ACK_AFTER_TB_PPDU;
        NS_ASSERT(m_staMac && m_staMac->IsAssociated());
        txVector = GetWifiRemoteStationManager()->GetBlockAckTxVector(
            m_psduMap.begin()->second->GetAddr1(),
            m_txParams.m_txVector);
        responseTxVector = &txVector;
    }
    /*
     * QoS Null frames solicited by a BSRP Trigger Frame
     */
    else if (m_txParams.m_txVector.IsUlMu() &&
             m_txParams.m_acknowledgment->method == WifiAcknowledgment::NONE)
    {
        // No response is expected, so do nothing.
    }
    else
    {
        NS_ABORT_MSG("Unable to handle the selected acknowledgment method ("
                     << m_txParams.m_acknowledgment.get() << ")");
    }

    // create a map of Ptr<const WifiPsdu>, as required by the PHY
    WifiConstPsduMap psduMap;
    for (const auto& psdu : m_psduMap)
    {
        psduMap.emplace(psdu.first, psdu.second);
    }

    Time txDuration;
    if (m_txParams.m_txVector.IsUlMu())
    {
        txDuration = HePhy::ConvertLSigLengthToHeTbPpduDuration(m_txParams.m_txVector.GetLength(),
                                                                m_txParams.m_txVector,
                                                                m_phy->GetPhyBand());
    }
    else
    {
        txDuration =
            m_phy->CalculateTxDuration(psduMap, m_txParams.m_txVector, m_phy->GetPhyBand());

        // Set Duration/ID
        Time durationId = GetPsduDurationId(txDuration, m_txParams);
        for (auto& psdu : m_psduMap)
        {
            psdu.second->SetDuration(durationId);
        }
    }

    if (timerType == WifiTxTimer::NOT_RUNNING)
    {
        if (!m_txParams.m_txVector.IsUlMu())
        {
            Simulator::Schedule(txDuration, &HeFrameExchangeManager::TransmissionSucceeded, this);
        }
    }
    else
    {
        Time timeout = txDuration + m_phy->GetSifs() + m_phy->GetSlot() +
                       m_phy->CalculatePhyPreambleAndHeaderDuration(*responseTxVector);
        m_channelAccessManager->NotifyAckTimeoutStartNow(timeout);

        // start timer
        switch (timerType)
        {
        case WifiTxTimer::WAIT_NORMAL_ACK_AFTER_DL_MU_PPDU:
            NS_ASSERT(mpdu);
            m_txTimer.Set(timerType,
                          timeout,
                          &HeFrameExchangeManager::NormalAckTimeout,
                          this,
                          mpdu,
                          m_txParams.m_txVector);
            break;
        case WifiTxTimer::WAIT_BLOCK_ACK:
            NS_ASSERT(psdu);
            m_txTimer.Set(timerType,
                          timeout,
                          &HeFrameExchangeManager::BlockAckTimeout,
                          this,
                          psdu,
                          m_txParams.m_txVector);
            break;
        case WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU:
            m_txTimer.Set(timerType,
                          timeout,
                          &HeFrameExchangeManager::BlockAcksInTbPpduTimeout,
                          this,
                          &m_psduMap,
                          &m_staExpectTbPpduFrom,
                          m_staExpectTbPpduFrom.size());
            break;
        case WifiTxTimer::WAIT_TB_PPDU_AFTER_BASIC_TF:
        case WifiTxTimer::WAIT_QOS_NULL_AFTER_BSRP_TF:
            m_txTimer.Set(timerType,
                          timeout,
                          &HeFrameExchangeManager::TbPpduTimeout,
                          this,
                          &m_psduMap,
                          &m_staExpectTbPpduFrom,
                          m_staExpectTbPpduFrom.size());
            break;
        case WifiTxTimer::WAIT_BLOCK_ACK_AFTER_TB_PPDU:
            m_txTimer.Set(timerType,
                          timeout,
                          &HeFrameExchangeManager::BlockAckAfterTbPpduTimeout,
                          this,
                          m_psduMap.begin()->second,
                          m_txParams.m_txVector);
            break;
        default:
            NS_ABORT_MSG("Unknown timer type: " << timerType);
            break;
        }
    }

    // transmit the map of PSDUs
    ForwardPsduMapDown(psduMap, m_txParams.m_txVector);

    if (timerType == WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU ||
        timerType == WifiTxTimer::WAIT_TB_PPDU_AFTER_BASIC_TF ||
        timerType == WifiTxTimer::WAIT_QOS_NULL_AFTER_BSRP_TF)
    {
        // Pass TRIGVECTOR to HE PHY (equivalent to PHY-TRIGGER.request primitive)
        auto hePhy = StaticCast<HePhy>(m_phy->GetPhyEntity(responseTxVector->GetModulationClass()));
        hePhy->SetTrigVector(m_trigVector, m_txTimer.GetDelayLeft());
    }
    else if (timerType == WifiTxTimer::NOT_RUNNING && m_txParams.m_txVector.IsUlMu())
    {
        // clear m_psduMap after sending QoS Null frames following a BSRP Trigger Frame
        Simulator::Schedule(txDuration, &WifiPsduMap::clear, &m_psduMap);
    }
}

void
HeFrameExchangeManager::ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);

    if (ns3::IsDlMu(txVector.GetPreambleType()))
    {
        auto hePhy = StaticCast<HePhy>(m_phy->GetPhyEntity(txVector.GetModulationClass()));
        auto sigBMode = hePhy->GetSigBMode(txVector);
        txVector.SetSigBMode(sigBMode);
    }

    for (const auto& psdu : psduMap)
    {
        NS_LOG_DEBUG("Transmitting: [STAID=" << psdu.first << ", " << *psdu.second << "]");
    }
    NS_LOG_DEBUG("TXVECTOR: " << txVector);
    for (const auto& [staId, psdu] : psduMap)
    {
        FinalizeMacHeader(psdu);
        NotifyTxToEdca(psdu);
    }
    if (psduMap.size() > 1 || psduMap.begin()->second->IsAggregate() ||
        psduMap.begin()->second->IsSingle())
    {
        txVector.SetAggregation(true);
    }

    m_phy->Send(psduMap, txVector);
}

Ptr<WifiMpdu>
HeFrameExchangeManager::PrepareMuBar(const WifiTxVector& responseTxVector,
                                     std::map<uint16_t, CtrlBAckRequestHeader> recipients) const
{
    NS_LOG_FUNCTION(this << responseTxVector);
    NS_ASSERT(responseTxVector.GetHeMuUserInfoMap().size() == recipients.size());
    NS_ASSERT(!recipients.empty());

    CtrlTriggerHeader muBar(TriggerFrameType::MU_BAR_TRIGGER, responseTxVector);
    SetTargetRssi(muBar);
    // Set the CS Required subfield to true, unless the UL Length subfield is less
    // than or equal to 418 (see Section 26.5.2.5 of 802.11ax-2021)
    muBar.SetCsRequired(muBar.GetUlLength() > 418);

    // Add the Trigger Dependent User Info subfield to every User Info field
    for (auto& userInfo : muBar)
    {
        auto recipientIt = recipients.find(userInfo.GetAid12());
        NS_ASSERT(recipientIt != recipients.end());

        // Store the BAR in the Trigger Dependent User Info subfield
        userInfo.SetMuBarTriggerDepUserInfo(recipientIt->second);
    }

    Ptr<Packet> bar = Create<Packet>();
    bar->AddHeader(muBar);
    Mac48Address rxAddress;
    // "If the Trigger frame has one User Info field and the AID12 subfield of the
    // User Info contains the AID of a STA, then the RA field is set to the address
    // of that STA". Otherwise, it is set to the broadcast address (Sec. 9.3.1.23 -
    // 802.11ax amendment draft 3.0)
    if (muBar.GetNUserInfoFields() > 1)
    {
        rxAddress = Mac48Address::GetBroadcast();
    }
    else
    {
        NS_ASSERT(m_apMac);
        rxAddress = m_apMac->GetStaList(m_linkId).at(recipients.begin()->first);
    }

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_TRIGGER);
    hdr.SetAddr1(rxAddress);
    hdr.SetAddr2(m_self);
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();
    hdr.SetNoRetry();
    hdr.SetNoMoreFragments();

    return Create<WifiMpdu>(bar, hdr);
}

void
HeFrameExchangeManager::CalculateProtectionTime(WifiProtection* protection) const
{
    NS_LOG_FUNCTION(this << protection);
    NS_ASSERT(protection != nullptr);

    if (protection->method == WifiProtection::MU_RTS_CTS)
    {
        WifiMuRtsCtsProtection* muRtsCtsProtection =
            static_cast<WifiMuRtsCtsProtection*>(protection);

        // Get the TXVECTOR used by one station to send the CTS response. This is used
        // to compute the TX duration, so it does not matter which station we choose
        WifiTxVector ctsTxVector =
            GetCtsTxVectorAfterMuRts(muRtsCtsProtection->muRts,
                                     muRtsCtsProtection->muRts.begin()->GetAid12());

        uint32_t muRtsSize = WifiMacHeader(WIFI_MAC_CTL_TRIGGER).GetSize() +
                             muRtsCtsProtection->muRts.GetSerializedSize() + WIFI_MAC_FCS_LENGTH;
        muRtsCtsProtection->protectionTime =
            m_phy->CalculateTxDuration(muRtsSize,
                                       muRtsCtsProtection->muRtsTxVector,
                                       m_phy->GetPhyBand()) +
            m_phy->CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand()) +
            2 * m_phy->GetSifs();
    }
    else
    {
        VhtFrameExchangeManager::CalculateProtectionTime(protection);
    }
}

void
HeFrameExchangeManager::CalculateAcknowledgmentTime(WifiAcknowledgment* acknowledgment) const
{
    NS_LOG_FUNCTION(this << acknowledgment);
    NS_ASSERT(acknowledgment);

    /*
     * Acknowledgment via a sequence of BlockAckReq and BlockAck frames
     */
    if (acknowledgment->method == WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE)
    {
        WifiDlMuBarBaSequence* dlMuBarBaAcknowledgment =
            static_cast<WifiDlMuBarBaSequence*>(acknowledgment);

        Time duration = Seconds(0);

        // normal ack or implicit BAR policy can be used for (no more than) one receiver
        NS_ABORT_IF(dlMuBarBaAcknowledgment->stationsReplyingWithNormalAck.size() +
                        dlMuBarBaAcknowledgment->stationsReplyingWithBlockAck.size() >
                    1);

        if (!dlMuBarBaAcknowledgment->stationsReplyingWithNormalAck.empty())
        {
            const auto& info =
                dlMuBarBaAcknowledgment->stationsReplyingWithNormalAck.begin()->second;
            duration +=
                m_phy->GetSifs() +
                m_phy->CalculateTxDuration(GetAckSize(), info.ackTxVector, m_phy->GetPhyBand());
        }

        if (!dlMuBarBaAcknowledgment->stationsReplyingWithBlockAck.empty())
        {
            const auto& info =
                dlMuBarBaAcknowledgment->stationsReplyingWithBlockAck.begin()->second;
            duration += m_phy->GetSifs() + m_phy->CalculateTxDuration(GetBlockAckSize(info.baType),
                                                                      info.blockAckTxVector,
                                                                      m_phy->GetPhyBand());
        }

        for (const auto& stations : dlMuBarBaAcknowledgment->stationsSendBlockAckReqTo)
        {
            const auto& info = stations.second;
            duration += m_phy->GetSifs() +
                        m_phy->CalculateTxDuration(GetBlockAckRequestSize(info.barType),
                                                   info.blockAckReqTxVector,
                                                   m_phy->GetPhyBand()) +
                        m_phy->GetSifs() +
                        m_phy->CalculateTxDuration(GetBlockAckSize(info.baType),
                                                   info.blockAckTxVector,
                                                   m_phy->GetPhyBand());
        }

        dlMuBarBaAcknowledgment->acknowledgmentTime = duration;
    }
    /*
     * Acknowledgment via a MU-BAR Trigger Frame sent as single user frame
     */
    else if (acknowledgment->method == WifiAcknowledgment::DL_MU_TF_MU_BAR)
    {
        WifiDlMuTfMuBar* dlMuTfMuBarAcknowledgment = static_cast<WifiDlMuTfMuBar*>(acknowledgment);

        Time duration = Seconds(0);

        for (const auto& stations : dlMuTfMuBarAcknowledgment->stationsReplyingWithBlockAck)
        {
            // compute the TX duration of the BlockAck response from this receiver.
            const auto& info = stations.second;
            NS_ASSERT(info.blockAckTxVector.GetHeMuUserInfoMap().size() == 1);
            uint16_t staId = info.blockAckTxVector.GetHeMuUserInfoMap().begin()->first;
            Time currBlockAckDuration = m_phy->CalculateTxDuration(GetBlockAckSize(info.baType),
                                                                   info.blockAckTxVector,
                                                                   m_phy->GetPhyBand(),
                                                                   staId);
            // update the max duration among all the Block Ack responses
            if (currBlockAckDuration > duration)
            {
                duration = currBlockAckDuration;
            }
        }

        // The computed duration may not be coded exactly in the L-SIG length, hence determine
        // the exact duration corresponding to the value that will be coded in this field.
        WifiTxVector& txVector = dlMuTfMuBarAcknowledgment->stationsReplyingWithBlockAck.begin()
                                     ->second.blockAckTxVector;
        std::tie(dlMuTfMuBarAcknowledgment->ulLength, duration) =
            HePhy::ConvertHeTbPpduDurationToLSigLength(duration, txVector, m_phy->GetPhyBand());

        uint32_t muBarSize = GetMuBarSize(dlMuTfMuBarAcknowledgment->barTypes);
        if (dlMuTfMuBarAcknowledgment->muBarTxVector.GetModulationClass() >= WIFI_MOD_CLASS_VHT)
        {
            // MU-BAR TF will be sent as an S-MPDU
            muBarSize = MpduAggregator::GetSizeIfAggregated(muBarSize, 0);
        }
        dlMuTfMuBarAcknowledgment->acknowledgmentTime =
            m_phy->GetSifs() +
            m_phy->CalculateTxDuration(muBarSize,
                                       dlMuTfMuBarAcknowledgment->muBarTxVector,
                                       m_phy->GetPhyBand()) +
            m_phy->GetSifs() + duration;
    }
    /*
     * Acknowledgment requested by MU-BAR TFs aggregated to PSDUs in the DL MU PPDU
     */
    else if (acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
    {
        WifiDlMuAggregateTf* dlMuAggrTfAcknowledgment =
            static_cast<WifiDlMuAggregateTf*>(acknowledgment);

        Time duration = Seconds(0);

        for (const auto& stations : dlMuAggrTfAcknowledgment->stationsReplyingWithBlockAck)
        {
            // compute the TX duration of the BlockAck response from this receiver.
            const auto& info = stations.second;
            NS_ASSERT(info.blockAckTxVector.GetHeMuUserInfoMap().size() == 1);
            uint16_t staId = info.blockAckTxVector.GetHeMuUserInfoMap().begin()->first;
            Time currBlockAckDuration = m_phy->CalculateTxDuration(GetBlockAckSize(info.baType),
                                                                   info.blockAckTxVector,
                                                                   m_phy->GetPhyBand(),
                                                                   staId);
            // update the max duration among all the Block Ack responses
            if (currBlockAckDuration > duration)
            {
                duration = currBlockAckDuration;
            }
        }

        // The computed duration may not be coded exactly in the L-SIG length, hence determine
        // the exact duration corresponding to the value that will be coded in this field.
        WifiTxVector& txVector =
            dlMuAggrTfAcknowledgment->stationsReplyingWithBlockAck.begin()->second.blockAckTxVector;
        std::tie(dlMuAggrTfAcknowledgment->ulLength, duration) =
            HePhy::ConvertHeTbPpduDurationToLSigLength(duration, txVector, m_phy->GetPhyBand());
        dlMuAggrTfAcknowledgment->acknowledgmentTime = m_phy->GetSifs() + duration;
    }
    /*
     * Basic Trigger Frame starting an UL MU transmission
     */
    else if (acknowledgment->method == WifiAcknowledgment::UL_MU_MULTI_STA_BA)
    {
        WifiUlMuMultiStaBa* ulMuMultiStaBa = static_cast<WifiUlMuMultiStaBa*>(acknowledgment);

        Time duration = m_phy->CalculateTxDuration(GetBlockAckSize(ulMuMultiStaBa->baType),
                                                   ulMuMultiStaBa->multiStaBaTxVector,
                                                   m_phy->GetPhyBand());
        ulMuMultiStaBa->acknowledgmentTime = m_phy->GetSifs() + duration;
    }
    /*
     * TB PPDU solicired by a Basic or BSRP Trigger Frame
     */
    else if (acknowledgment->method == WifiAcknowledgment::ACK_AFTER_TB_PPDU)
    {
        // The station solicited by the Trigger Frame does not have to account
        // for the actual acknowledgment time since it is given the PPDU duration
        // through the Trigger Frame
        acknowledgment->acknowledgmentTime = Seconds(0);
    }
    else
    {
        VhtFrameExchangeManager::CalculateAcknowledgmentTime(acknowledgment);
    }
}

WifiMode
HeFrameExchangeManager::GetCtsModeAfterMuRts() const
{
    // The CTS frame sent in response to an MU-RTS Trigger frame shall be carried in a non-HT or
    // non-HT duplicate PPDU (see Clause 17) with a 6 Mb/s rate (Sec. 26.2.6.3 of 802.11ax-2021)
    return m_phy->GetPhyBand() == WIFI_PHY_BAND_2_4GHZ ? ErpOfdmPhy::GetErpOfdmRate6Mbps()
                                                       : OfdmPhy::GetOfdmRate6Mbps();
}

WifiTxVector
HeFrameExchangeManager::GetCtsTxVectorAfterMuRts(const CtrlTriggerHeader& trigger,
                                                 uint16_t staId) const
{
    NS_LOG_FUNCTION(this << trigger << staId);

    auto userInfoIt = trigger.FindUserInfoWithAid(staId);
    NS_ASSERT_MSG(userInfoIt != trigger.end(), "User Info field for AID=" << staId << " not found");
    uint16_t bw = 0;

    if (uint8_t ru = userInfoIt->GetMuRtsRuAllocation(); ru < 65)
    {
        bw = 20;
    }
    else if (ru < 67)
    {
        bw = 40;
    }
    else if (ru == 67)
    {
        bw = 80;
    }
    else
    {
        NS_ASSERT(ru == 68);
        bw = 160;
    }

    auto txVector = GetWifiRemoteStationManager()->GetCtsTxVector(m_bssid, GetCtsModeAfterMuRts());
    // set the channel width of the CTS TXVECTOR according to the allocated RU
    txVector.SetChannelWidth(bw);

    return txVector;
}

Time
HeFrameExchangeManager::GetTxDuration(uint32_t ppduPayloadSize,
                                      Mac48Address receiver,
                                      const WifiTxParameters& txParams) const
{
    if (!txParams.m_txVector.IsMu())
    {
        return VhtFrameExchangeManager::GetTxDuration(ppduPayloadSize, receiver, txParams);
    }

    NS_ASSERT_MSG(!txParams.m_txVector.IsDlMu() || m_apMac, "DL MU can be done by an AP");
    NS_ASSERT_MSG(!txParams.m_txVector.IsUlMu() || m_staMac, "UL MU can be done by a STA");

    if (txParams.m_acknowledgment &&
        txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF)
    {
        // we need to account for the size of the aggregated MU-BAR Trigger Frame
        WifiDlMuAggregateTf* acknowledgment =
            static_cast<WifiDlMuAggregateTf*>(txParams.m_acknowledgment.get());

        const auto& info = acknowledgment->stationsReplyingWithBlockAck.find(receiver);
        NS_ASSERT(info != acknowledgment->stationsReplyingWithBlockAck.end());

        ppduPayloadSize =
            MpduAggregator::GetSizeIfAggregated(info->second.muBarSize, ppduPayloadSize);
    }

    uint16_t staId = (txParams.m_txVector.IsDlMu() ? m_apMac->GetAssociationId(receiver, m_linkId)
                                                   : m_staMac->GetAssociationId());
    Time psduDuration = m_phy->CalculateTxDuration(ppduPayloadSize,
                                                   txParams.m_txVector,
                                                   m_phy->GetPhyBand(),
                                                   staId);

    return std::max(psduDuration, txParams.m_txDuration);
}

void
HeFrameExchangeManager::TbPpduTimeout(WifiPsduMap* psduMap,
                                      const std::set<Mac48Address>* staMissedTbPpduFrom,
                                      std::size_t nSolicitedStations)
{
    NS_LOG_FUNCTION(this << psduMap << staMissedTbPpduFrom->size() << nSolicitedStations);

    NS_ASSERT(psduMap);
    NS_ASSERT(IsTrigger(*psduMap));

    // This method is called if some station(s) did not send a TB PPDU
    NS_ASSERT(!staMissedTbPpduFrom->empty());
    NS_ASSERT(m_edca);

    if (staMissedTbPpduFrom->size() == nSolicitedStations)
    {
        // no station replied, the transmission failed
        m_edca->UpdateFailedCw(m_linkId);

        TransmissionFailed();
    }
    else if (!m_multiStaBaEvent.IsRunning())
    {
        m_edca->ResetCw(m_linkId);
        TransmissionSucceeded();
    }

    m_psduMap.clear();
}

void
HeFrameExchangeManager::BlockAcksInTbPpduTimeout(
    WifiPsduMap* psduMap,
    const std::set<Mac48Address>* staMissedBlockAckFrom,
    std::size_t nSolicitedStations)
{
    NS_LOG_FUNCTION(this << psduMap << nSolicitedStations);

    NS_ASSERT(psduMap);
    NS_ASSERT(m_txParams.m_acknowledgment &&
              (m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_AGGREGATE_TF ||
               m_txParams.m_acknowledgment->method == WifiAcknowledgment::DL_MU_TF_MU_BAR));

    // This method is called if some station(s) did not send a BlockAck frame in a TB PPDU
    NS_ASSERT(!staMissedBlockAckFrom->empty());

    bool resetCw;

    if (staMissedBlockAckFrom->size() == nSolicitedStations)
    {
        // no station replied, the transmission failed
        // call ReportDataFailed to increase SRC/LRC
        GetWifiRemoteStationManager()->ReportDataFailed(*psduMap->begin()->second->begin());
        resetCw = false;
    }
    else
    {
        // the transmission succeeded
        resetCw = true;
    }

    if (m_triggerFrame)
    {
        // this is strictly needed for DL_MU_TF_MU_BAR only
        DequeueMpdu(m_triggerFrame);
        m_triggerFrame = nullptr;
    }

    for (const auto& sta : *staMissedBlockAckFrom)
    {
        Ptr<WifiPsdu> psdu = GetPsduTo(sta, *psduMap);
        NS_ASSERT(psdu);
        // If the QSRC[AC] or the QLRC[AC] has reached dot11ShortRetryLimit or dot11LongRetryLimit
        // respectively, CW[AC] shall be reset to CWmin[AC] (sec. 10.22.2.2 of 802.11-2016).
        // We should get that psduResetCw is the same for all PSDUs, but the handling of QSRC/QLRC
        // needs to be aligned to the specifications.
        bool psduResetCw;
        MissedBlockAck(psdu, m_txParams.m_txVector, psduResetCw);
        resetCw = resetCw || psduResetCw;
    }

    NS_ASSERT(m_edca);

    if (resetCw)
    {
        m_edca->ResetCw(m_linkId);
    }
    else
    {
        m_edca->UpdateFailedCw(m_linkId);
    }

    if (staMissedBlockAckFrom->size() == nSolicitedStations)
    {
        // no station replied, the transmission failed
        TransmissionFailed();
    }
    else
    {
        TransmissionSucceeded();
    }
    m_psduMap.clear();
}

void
HeFrameExchangeManager::BlockAckAfterTbPpduTimeout(Ptr<WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *psdu << txVector);

    bool resetCw;

    // call ReportDataFailed to increase SRC/LRC
    GetWifiRemoteStationManager()->ReportDataFailed(*psdu->begin());

    MissedBlockAck(psdu, m_txParams.m_txVector, resetCw);

    // This is a PSDU sent in a TB PPDU. An HE STA resumes the EDCA backoff procedure
    // without modifying CW or the backoff counter for the associated EDCAF, after
    // transmission of an MPDU in a TB PPDU regardless of whether the STA has received
    // the corresponding acknowledgment frame in response to the MPDU sent in the TB PPDU
    // (Sec. 10.22.2.2 of 11ax Draft 3.0)
    m_psduMap.clear();
}

void
HeFrameExchangeManager::NormalAckTimeout(Ptr<WifiMpdu> mpdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *mpdu << txVector);

    VhtFrameExchangeManager::NormalAckTimeout(mpdu, txVector);

    // If a Normal Ack is missed in response to a DL MU PPDU requiring acknowledgment
    // in SU format, we have to set the Retry flag for all transmitted MPDUs that have
    // not been acknowledged nor discarded and clear m_psduMap since the transmission failed.
    for (auto& psdu : m_psduMap)
    {
        for (auto& mpdu : *PeekPointer(psdu.second))
        {
            if (mpdu->IsQueued())
            {
                m_mac->GetTxopQueue(mpdu->GetQueueAc())->GetOriginal(mpdu)->GetHeader().SetRetry();
                mpdu->ResetInFlight(m_linkId);
            }
        }
    }
    m_psduMap.clear();
}

void
HeFrameExchangeManager::BlockAckTimeout(Ptr<WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *psdu << txVector);

    VhtFrameExchangeManager::BlockAckTimeout(psdu, txVector);

    // If a Block Ack is missed in response to a DL MU PPDU requiring acknowledgment
    // in SU format, we have to set the Retry flag for all transmitted MPDUs that have
    // not been acknowledged nor discarded and clear m_psduMap since the transmission failed.
    for (auto& psdu : m_psduMap)
    {
        for (auto& mpdu : *PeekPointer(psdu.second))
        {
            if (mpdu->IsQueued())
            {
                mpdu->GetHeader().SetRetry();
            }
        }
    }
    m_psduMap.clear();
}

WifiTxVector
HeFrameExchangeManager::GetTrigVector(const CtrlTriggerHeader& trigger) const
{
    WifiTxVector v;
    v.SetPreambleType(trigger.GetVariant() == TriggerFrameVariant::HE ? WIFI_PREAMBLE_HE_TB
                                                                      : WIFI_PREAMBLE_EHT_TB);
    v.SetChannelWidth(trigger.GetUlBandwidth());
    v.SetGuardInterval(trigger.GetGuardInterval());
    v.SetLength(trigger.GetUlLength());
    for (const auto& userInfoField : trigger)
    {
        v.SetHeMuUserInfo(
            userInfoField.GetAid12(),
            {userInfoField.GetRuAllocation(), userInfoField.GetUlMcs(), userInfoField.GetNss()});
    }
    return v;
}

WifiTxVector
HeFrameExchangeManager::GetHeTbTxVector(CtrlTriggerHeader trigger, Mac48Address triggerSender) const
{
    NS_ASSERT(triggerSender !=
              m_self); // TxPower information is used only by STAs, it is useless for the sending AP
                       // (which can directly use CtrlTriggerHeader::GetHeTbTxVector)
    NS_ASSERT(m_staMac);
    uint16_t staId = m_staMac->GetAssociationId();
    auto userInfoIt = trigger.FindUserInfoWithAid(staId);
    NS_ASSERT(userInfoIt != trigger.end());

    WifiTxVector v = trigger.GetHeTbTxVector(staId);

    Ptr<HeConfiguration> heConfiguration = m_mac->GetHeConfiguration();
    NS_ASSERT_MSG(heConfiguration, "This STA has to be an HE station to send an HE TB PPDU");
    v.SetBssColor(heConfiguration->GetBssColor());

    if (userInfoIt->IsUlTargetRssiMaxTxPower())
    {
        NS_LOG_LOGIC("AP requested using the max transmit power (" << m_phy->GetTxPowerEnd()
                                                                   << " dBm)");
        v.SetTxPowerLevel(m_phy->GetNTxPower());
        return v;
    }

    uint8_t powerLevel = GetWifiRemoteStationManager()->GetDefaultTxPowerLevel();
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
    auto optRssi = GetMostRecentRssi(triggerSender);
    NS_ASSERT(optRssi);
    int8_t pathLossDb =
        trigger.GetApTxPower() -
        static_cast<int8_t>(
            *optRssi); // cast RSSI to be on equal footing with AP Tx power information
    double reqTxPowerDbm = static_cast<double>(userInfoIt->GetUlTargetRssi() + pathLossDb);

    // Convert the transmit power to a power level
    uint8_t numPowerLevels = m_phy->GetNTxPower();
    if (numPowerLevels > 1)
    {
        double stepDbm = (m_phy->GetTxPowerEnd() - m_phy->GetTxPowerStart()) / (numPowerLevels - 1);
        powerLevel = static_cast<uint8_t>(
            ceil((reqTxPowerDbm - m_phy->GetTxPowerStart()) /
                 stepDbm)); // better be slightly above so as to satisfy target UL RSSI
        if (powerLevel > numPowerLevels)
        {
            powerLevel = numPowerLevels; // capping will trigger warning below
        }
    }
    if (reqTxPowerDbm > m_phy->GetPowerDbm(powerLevel))
    {
        NS_LOG_WARN("The requested power level ("
                    << reqTxPowerDbm << "dBm) cannot be satisfied (max: " << m_phy->GetTxPowerEnd()
                    << "dBm)");
    }
    v.SetTxPowerLevel(powerLevel);
    NS_LOG_LOGIC("UL power control: "
                 << "input {pathLoss=" << pathLossDb << "dB, reqTxPower=" << reqTxPowerDbm << "dBm}"
                 << " output {powerLevel=" << +powerLevel << " -> "
                 << m_phy->GetPowerDbm(powerLevel) << "dBm}"
                 << " PHY power capa {min=" << m_phy->GetTxPowerStart() << "dBm, max="
                 << m_phy->GetTxPowerEnd() << "dBm, levels:" << +numPowerLevels << "}");

    return v;
}

std::optional<double>
HeFrameExchangeManager::GetMostRecentRssi(const Mac48Address& address) const
{
    return GetWifiRemoteStationManager()->GetMostRecentRssi(address);
}

void
HeFrameExchangeManager::SetTargetRssi(CtrlTriggerHeader& trigger) const
{
    NS_LOG_FUNCTION(this);
    NS_ASSERT(m_apMac);

    trigger.SetApTxPower(static_cast<int8_t>(
        m_phy->GetPowerDbm(GetWifiRemoteStationManager()->GetDefaultTxPowerLevel())));
    for (auto& userInfo : trigger)
    {
        const auto staList = m_apMac->GetStaList(m_linkId);
        auto itAidAddr = staList.find(userInfo.GetAid12());
        NS_ASSERT(itAidAddr != staList.end());
        auto optRssi = GetMostRecentRssi(itAidAddr->second);
        NS_ASSERT(optRssi);
        int8_t rssi = static_cast<int8_t>(*optRssi);
        rssi = (rssi >= -20)
                   ? -20
                   : ((rssi <= -110) ? -110 : rssi); // cap so as to keep within [-110; -20] dBm
        userInfo.SetUlTargetRssi(rssi);
    }
}

void
HeFrameExchangeManager::PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    auto txVectorCopy = txVector;

    if (psdu->GetNMpdus() == 1 && psdu->GetHeader(0).IsTrigger())
    {
        CtrlTriggerHeader trigger;
        psdu->GetPayload(0)->PeekHeader(trigger);
        if (trigger.IsMuRts())
        {
            const WifiMacHeader& muRts = psdu->GetHeader(0);
            // A station receiving an MU-RTS behaves just like as if it received an RTS.
            // Determine whether the MU-RTS is addressed to this station or not and
            // prepare an "equivalent" RTS frame so that we can reuse the UpdateNav()
            // and SetTxopHolder() methods of the parent classes
            WifiMacHeader rts;
            rts.SetType(WIFI_MAC_CTL_RTS);
            rts.SetDsNotFrom();
            rts.SetDsNotTo();
            rts.SetDuration(muRts.GetDuration());
            rts.SetAddr2(muRts.GetAddr2());
            if (m_staMac != nullptr && m_staMac->IsAssociated() &&
                muRts.GetAddr2() == m_bssid // sent by the AP this STA is associated with
                && trigger.FindUserInfoWithAid(m_staMac->GetAssociationId()) != trigger.end())
            {
                // the MU-RTS is addressed to this station
                rts.SetAddr1(m_self);
            }
            else
            {
                rts.SetAddr1(muRts.GetAddr2()); // an address different from that of this station
            }
            psdu = Create<const WifiPsdu>(Create<Packet>(), rts);
            // The duration of the NAV reset timeout has to take into account that the CTS
            // response is sent using the 6 Mbps data rate
            txVectorCopy =
                GetWifiRemoteStationManager()->GetCtsTxVector(m_bssid, GetCtsModeAfterMuRts());
        }
    }
    VhtFrameExchangeManager::PostProcessFrame(psdu, txVectorCopy);
}

void
HeFrameExchangeManager::SendCtsAfterMuRts(const WifiMacHeader& muRtsHdr,
                                          const CtrlTriggerHeader& trigger,
                                          double muRtsSnr)
{
    NS_LOG_FUNCTION(this << muRtsHdr << trigger << muRtsSnr);

    if (!UlMuCsMediumIdle(trigger))
    {
        NS_LOG_DEBUG("UL MU CS indicated medium busy, cannot send CTS");
        return;
    }

    NS_ASSERT(m_staMac != nullptr && m_staMac->IsAssociated());
    WifiTxVector ctsTxVector = GetCtsTxVectorAfterMuRts(trigger, m_staMac->GetAssociationId());
    ctsTxVector.SetTriggerResponding(true);

    DoSendCtsAfterRts(muRtsHdr, ctsTxVector, muRtsSnr);
}

void
HeFrameExchangeManager::SendMultiStaBlockAck(const WifiTxParameters& txParams, Time durationId)
{
    NS_LOG_FUNCTION(this << &txParams << durationId.As(Time::US));

    NS_ASSERT(m_apMac);
    NS_ASSERT(txParams.m_acknowledgment &&
              txParams.m_acknowledgment->method == WifiAcknowledgment::UL_MU_MULTI_STA_BA);
    WifiUlMuMultiStaBa* acknowledgment =
        static_cast<WifiUlMuMultiStaBa*>(txParams.m_acknowledgment.get());

    NS_ASSERT(!acknowledgment->stationsReceivingMultiStaBa.empty());

    CtrlBAckResponseHeader blockAck;
    blockAck.SetType(acknowledgment->baType);

    Mac48Address receiver;

    for (const auto& staInfo : acknowledgment->stationsReceivingMultiStaBa)
    {
        receiver = staInfo.first.first;
        uint8_t tid = staInfo.first.second;
        std::size_t index = staInfo.second;

        blockAck.SetAid11(m_apMac->GetAssociationId(receiver, m_linkId), index);
        blockAck.SetTidInfo(tid, index);

        if (tid == 14)
        {
            // All-ack context
            NS_LOG_DEBUG("Multi-STA Block Ack: Sending All-ack to=" << receiver);
            blockAck.SetAckType(true, index);
            continue;
        }

        if (acknowledgment->baType.m_bitmapLen.at(index) == 0)
        {
            // Acknowledgment context
            NS_LOG_DEBUG("Multi-STA Block Ack: Sending Ack to=" << receiver);
            blockAck.SetAckType(true, index);
        }
        else
        {
            // Block acknowledgment context
            blockAck.SetAckType(false, index);

            auto agreement = m_mac->GetBaAgreementEstablishedAsRecipient(receiver, tid);
            NS_ASSERT(agreement);
            agreement->get().FillBlockAckBitmap(&blockAck, index);
            NS_LOG_DEBUG("Multi-STA Block Ack: Sending Block Ack with seq="
                         << blockAck.GetStartingSequence(index) << " to=" << receiver
                         << " tid=" << +tid);
        }
    }

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_CTL_BACKRESP);
    hdr.SetAddr1(acknowledgment->stationsReceivingMultiStaBa.size() == 1
                     ? receiver
                     : Mac48Address::GetBroadcast());
    hdr.SetAddr2(m_self);
    hdr.SetDsNotFrom();
    hdr.SetDsNotTo();

    Ptr<Packet> packet = Create<Packet>();
    packet->AddHeader(blockAck);
    Ptr<WifiPsdu> psdu =
        GetWifiPsdu(Create<WifiMpdu>(packet, hdr), acknowledgment->multiStaBaTxVector);

    Time txDuration = m_phy->CalculateTxDuration(GetBlockAckSize(acknowledgment->baType),
                                                 acknowledgment->multiStaBaTxVector,
                                                 m_phy->GetPhyBand());
    /**
     * In a BlockAck frame transmitted in response to a frame carried in HE TB PPDU under
     * single protection settings, the Duration/ID field is set to the value obtained from
     * the Duration/ID field of the frame that elicited the response minus the time, in
     * microseconds between the end of the PPDU carrying the frame that elicited the response
     * and the end of the PPDU carrying the BlockAck frame.
     * Under multiple protection settings, the Duration/ID field in a BlockAck frame transmitted
     * in response to a frame carried in HE TB PPDU is set according to the multiple protection
     * settings defined in 9.2.5.2. (Sec. 9.2.5.7 of 802.11ax-2021)
     */
    NS_ASSERT(m_edca);
    if (m_edca->GetTxopLimit(m_linkId).IsZero())
    {
        // single protection settings
        psdu->SetDuration(Max(durationId - m_phy->GetSifs() - txDuration, Seconds(0)));
    }
    else
    {
        // multiple protection settings
        psdu->SetDuration(Max(m_edca->GetRemainingTxop(m_linkId) - txDuration, Seconds(0)));
    }

    psdu->GetPayload(0)->AddPacketTag(m_muSnrTag);

    ForwardPsduDown(psdu, acknowledgment->multiStaBaTxVector);

    // continue with the TXOP if time remains
    m_psduMap.clear();
    m_edca->ResetCw(m_linkId);
    m_muSnrTag.Reset();
    Simulator::Schedule(txDuration, &HeFrameExchangeManager::TransmissionSucceeded, this);
}

void
HeFrameExchangeManager::ReceiveBasicTrigger(const CtrlTriggerHeader& trigger,
                                            const WifiMacHeader& hdr)
{
    NS_LOG_FUNCTION(this << trigger << hdr);
    NS_ASSERT(trigger.IsBasic());
    NS_ASSERT(m_staMac && m_staMac->IsAssociated());

    NS_LOG_DEBUG("Received a Trigger Frame (basic variant) soliciting a transmission");

    if (!UlMuCsMediumIdle(trigger))
    {
        return;
    }

    // Starting from the Preferred AC indicated in the Trigger Frame, check if there
    // is either a pending BlockAckReq frame or a data frame that can be transmitted
    // in the allocated time and is addressed to a station with which a Block Ack
    // agreement has been established.

    // create the sequence of TIDs to check
    std::vector<uint8_t> tids;
    uint16_t staId = m_staMac->GetAssociationId();
    AcIndex preferredAc = trigger.FindUserInfoWithAid(staId)->GetPreferredAc();
    auto acIt = wifiAcList.find(preferredAc);
    for (uint8_t i = 0; i < 4; i++)
    {
        NS_ASSERT(acIt != wifiAcList.end());
        tids.push_back(acIt->second.GetHighTid());
        tids.push_back(acIt->second.GetLowTid());

        acIt++;
        if (acIt == wifiAcList.end())
        {
            acIt = wifiAcList.begin();
        }
    }

    Ptr<WifiPsdu> psdu;
    WifiTxParameters txParams;
    WifiTxVector tbTxVector = GetHeTbTxVector(trigger, hdr.GetAddr2());
    Time ppduDuration = HePhy::ConvertLSigLengthToHeTbPpduDuration(trigger.GetUlLength(),
                                                                   tbTxVector,
                                                                   m_phy->GetPhyBand());

    for (const auto& tid : tids)
    {
        Ptr<QosTxop> edca = m_mac->GetQosTxop(tid);

        if (!m_mac->GetBaAgreementEstablishedAsOriginator(hdr.GetAddr2(), tid))
        {
            // no Block Ack agreement established for this TID
            continue;
        }

        txParams.Clear();
        txParams.m_txVector = tbTxVector;

        // first, check if there is a pending BlockAckReq frame
        if (auto mpdu = GetBar(edca->GetAccessCategory(), tid, hdr.GetAddr2());
            mpdu && TryAddMpdu(mpdu, txParams, ppduDuration))
        {
            NS_LOG_DEBUG("Sending a BAR within a TB PPDU");
            psdu = Create<WifiPsdu>(mpdu, true);
            break;
        }

        // otherwise, check if a suitable data frame is available
        auto receiver =
            GetWifiRemoteStationManager()->GetMldAddress(hdr.GetAddr2()).value_or(hdr.GetAddr2());
        if (auto mpdu = edca->PeekNextMpdu(m_linkId, tid, receiver))
        {
            mpdu = CreateAliasIfNeeded(mpdu);
            if (auto item = edca->GetNextMpdu(m_linkId, mpdu, txParams, ppduDuration, false))
            {
                // try A-MPDU aggregation
                std::vector<Ptr<WifiMpdu>> mpduList =
                    m_mpduAggregator->GetNextAmpdu(item, txParams, ppduDuration);
                psdu = (mpduList.size() > 1 ? Create<WifiPsdu>(std::move(mpduList))
                                            : Create<WifiPsdu>(item, true));
                break;
            }
        }
    }

    if (psdu)
    {
        psdu->SetDuration(hdr.GetDuration() - m_phy->GetSifs() - ppduDuration);
        SendPsduMapWithProtection(WifiPsduMap{{staId, psdu}}, txParams);
    }
    else
    {
        // send QoS Null frames
        SendQosNullFramesInTbPpdu(trigger, hdr);
    }
}

void
HeFrameExchangeManager::SendQosNullFramesInTbPpdu(const CtrlTriggerHeader& trigger,
                                                  const WifiMacHeader& hdr)
{
    NS_LOG_FUNCTION(this << trigger << hdr);
    NS_ASSERT(trigger.IsBasic() || trigger.IsBsrp());
    NS_ASSERT(m_staMac && m_staMac->IsAssociated());

    NS_LOG_DEBUG("Requested to send QoS Null frames");

    if (!UlMuCsMediumIdle(trigger))
    {
        return;
    }

    WifiMacHeader header;
    header.SetType(WIFI_MAC_QOSDATA_NULL);
    header.SetAddr1(hdr.GetAddr2());
    header.SetAddr2(m_self);
    header.SetAddr3(hdr.GetAddr2());
    header.SetDsTo();
    header.SetDsNotFrom();
    // TR3: Sequence numbers for transmitted QoS (+)Null frames may be set
    // to any value. (Table 10-3 of 802.11-2016)
    header.SetSequenceNumber(0);
    // Set the EOSP bit so that NotifyTxToEdca will add the Queue Size
    header.SetQosEosp();

    WifiTxParameters txParams;
    txParams.m_txVector = GetHeTbTxVector(trigger, hdr.GetAddr2());
    txParams.m_protection = std::unique_ptr<WifiProtection>(new WifiNoProtection);
    txParams.m_acknowledgment = std::unique_ptr<WifiAcknowledgment>(new WifiNoAck);

    Time ppduDuration = HePhy::ConvertLSigLengthToHeTbPpduDuration(trigger.GetUlLength(),
                                                                   txParams.m_txVector,
                                                                   m_phy->GetPhyBand());
    header.SetDuration(hdr.GetDuration() - m_phy->GetSifs() - ppduDuration);

    Ptr<WifiMpdu> mpdu;
    std::vector<Ptr<WifiMpdu>> mpduList;
    uint8_t tid = 0;
    header.SetQosTid(tid);

    while (tid < 8 &&
           IsWithinSizeAndTimeLimits(
               txParams.GetSizeIfAddMpdu(mpdu = Create<WifiMpdu>(Create<Packet>(), header)),
               hdr.GetAddr2(),
               txParams,
               ppduDuration))
    {
        if (!m_mac->GetBaAgreementEstablishedAsOriginator(hdr.GetAddr2(), tid))
        {
            NS_LOG_DEBUG("Skipping tid=" << +tid << " because no agreement established");
            header.SetQosTid(++tid);
            continue;
        }

        NS_LOG_DEBUG("Aggregating a QoS Null frame with tid=" << +tid);
        // We could call TryAddMpdu instead of IsWithinSizeAndTimeLimits above in order to
        // get the TX parameters updated automatically. However, aggregating the QoS Null
        // frames might fail because MPDU aggregation is disabled by default for VO
        // and BK. Therefore, we skip the check on max A-MPDU size and only update the
        // TX parameters below.
        txParams.m_acknowledgment = GetAckManager()->TryAddMpdu(mpdu, txParams);
        txParams.AddMpdu(mpdu);
        UpdateTxDuration(mpdu->GetHeader().GetAddr1(), txParams);
        mpduList.push_back(mpdu);
        header.SetQosTid(++tid);
    }

    if (mpduList.empty())
    {
        NS_LOG_DEBUG("Not enough time to send a QoS Null frame");
        return;
    }

    Ptr<WifiPsdu> psdu = (mpduList.size() > 1 ? Create<WifiPsdu>(std::move(mpduList))
                                              : Create<WifiPsdu>(mpduList.front(), true));
    uint16_t staId = m_staMac->GetAssociationId();
    SendPsduMapWithProtection(WifiPsduMap{{staId, psdu}}, txParams);
}

void
HeFrameExchangeManager::ReceiveMuBarTrigger(const CtrlTriggerHeader& trigger,
                                            uint8_t tid,
                                            Time durationId,
                                            double snr)
{
    NS_LOG_FUNCTION(this << trigger << tid << durationId.As(Time::US) << snr);

    auto agreement = m_mac->GetBaAgreementEstablishedAsRecipient(m_bssid, tid);

    if (!agreement)
    {
        NS_LOG_DEBUG("There's not a valid agreement for this BlockAckReq");
        return;
    }

    if (!UlMuCsMediumIdle(trigger))
    {
        return;
    }

    NS_LOG_DEBUG("Send Block Ack in TB PPDU");
    auto txVector = GetHeTbTxVector(trigger, m_bssid);
    SendBlockAck(*agreement, durationId, txVector, snr);
}

bool
HeFrameExchangeManager::IsIntraBssPpdu(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector) const
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    // "If, based on the MAC address information of a frame carried in a received PPDU, the
    // received PPDU satisfies both intra-BSS and inter-BSS conditions, then the received PPDU is
    // classified as an intra-BSS PPDU." (Sec. 26.2.2 of 802.11ax-2021)
    // Hence, check first if the intra-BSS conditions using MAC address information are satisfied:
    // 1. "The PPDU carries a frame that has an RA, TA, or BSSID field value that is equal to
    //    the BSSID of the BSS in which the STA is associated"
    const auto ra = psdu->GetAddr1();
    const auto ta = psdu->GetAddr2();
    const auto bssid = psdu->GetHeader(0).GetAddr3();
    const auto empty = Mac48Address();

    if (ra == m_bssid || ta == m_bssid || bssid == m_bssid)
    {
        return true;
    }

    // 2. "The PPDU carries a Control frame that does not have a TA field and that has an
    //    RA field value that matches the saved TXOP holder address of the BSS in which
    //    the STA is associated"
    if (psdu->GetHeader(0).IsCtl() && ta == empty && ra == m_txopHolder)
    {
        return true;
    }

    // If we get here, the intra-BSS conditions using MAC address information are not satisfied.
    // "If the received PPDU satisfies the intra-BSS conditions using the RXVECTOR parameter
    // BSS_COLOR and also satisfies the inter-BSS conditions using MAC address information of a
    // frame carried in the PPDU, then the classification made using the MAC address information
    // takes precedence."
    // Hence, if the inter-BSS conditions using MAC address information are satisfied, the frame
    // is classified as inter-BSS
    // 1. "The PPDU carries a frame that has a BSSID field, the value of which is not the BSSID
    //    of the BSS in which the STA is associated"
    if (bssid != empty && bssid != m_bssid)
    {
        return false;
    }

    // 2. The PPDU carries a frame that does not have a BSSID field but has both an RA field and
    //    TA field, neither value of which is equal to the BSSID of the BSS in which the STA is
    //    associated
    if (bssid == empty && ta != empty && ra != empty && ta != m_bssid && ra != m_bssid)
    {
        return false;
    }

    // If we get here, both intra-BSS and inter-bss conditions using MAC address information
    // are not satisfied. Hence, the frame is classified as intra-BSS if the intra-BSS conditions
    // using the RXVECTOR parameters are satisfied:
    // 1. The RXVECTOR parameter BSS_COLOR of the PPDU carrying the frame is the BSS color of the
    //    BSS of which the STA is a member
    // This condition is used if the BSS is not disabled ("If a STA determines that the BSS color
    // is disabled (see 26.17.3.3), then the RXVECTOR parameter BSS_COLOR of a PPDU shall not be
    // used to classify the PPDU")
    const auto bssColor = m_mac->GetHeConfiguration()->GetBssColor();

    // the other two conditions using the RXVECTOR parameter PARTIAL_AID are not implemented
    return bssColor != 0 && bssColor == txVector.GetBssColor();
}

void
HeFrameExchangeManager::UpdateNav(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    if (!psdu->HasNav())
    {
        return;
    }

    if (psdu->GetAddr1() == m_self)
    {
        // When the received frame’s RA is equal to the STA’s own MAC address, the STA
        // shall not update its NAV (IEEE 802.11-2020, sec. 10.3.2.4)
        return;
    }

    // The intra-BSS NAV is updated by an intra-BSS PPDU. The basic NAV is updated by an
    // inter-BSS PPDU or a PPDU that cannot be classified as intra-BSS or inter-BSS.
    // (Section 26.2.4 of 802.11ax-2021)
    if (!IsIntraBssPpdu(psdu, txVector))
    {
        NS_LOG_DEBUG("PPDU not classified as intra-BSS, update the basic NAV");
        VhtFrameExchangeManager::UpdateNav(psdu, txVector);
        return;
    }

    NS_LOG_DEBUG("PPDU classified as intra-BSS, update the intra-BSS NAV");
    Time duration = psdu->GetDuration();
    NS_LOG_DEBUG("Duration/ID=" << duration);

    if (psdu->GetHeader(0).IsCfEnd())
    {
        // An HE STA that maintains two NAVs (see 26.2.4) and receives a CF-End frame should reset
        // the basic NAV if the received CF-End frame is carried in an inter-BSS PPDU and reset the
        // intra-BSS NAV if the received CF-End frame is carried in an intra-BSS PPDU. (Sec. 26.2.5
        // of 802.11ax-2021)
        NS_LOG_DEBUG("Received CF-End, resetting the intra-BSS NAV");
        IntraBssNavResetTimeout();
        return;
    }

    // For all other received frames the STA shall update its NAV when the received
    // Duration is greater than the STA’s current NAV value (IEEE 802.11-2020 sec. 10.3.2.4)
    auto intraBssNavEnd = Simulator::Now() + duration;
    if (intraBssNavEnd > m_intraBssNavEnd)
    {
        m_intraBssNavEnd = intraBssNavEnd;
        NS_LOG_DEBUG("Updated intra-BSS NAV=" << m_intraBssNavEnd);

        // A STA that used information from an RTS frame as the most recent basis to update
        // its NAV setting is permitted to reset its NAV if no PHY-RXSTART.indication
        // primitive is received from the PHY during a NAVTimeout period starting when the
        // MAC receives a PHY-RXEND.indication primitive corresponding to the detection of
        // the RTS frame. NAVTimeout period is equal to:
        // (2 x aSIFSTime) + (CTS_Time) + aRxPHYStartDelay + (2 x aSlotTime)
        // The “CTS_Time” shall be calculated using the length of the CTS frame and the data
        // rate at which the RTS frame used for the most recent NAV update was received
        // (IEEE 802.11-2016 sec. 10.3.2.4)
        if (psdu->GetHeader(0).IsRts())
        {
            WifiTxVector ctsTxVector =
                GetWifiRemoteStationManager()->GetCtsTxVector(psdu->GetAddr2(), txVector.GetMode());
            auto navResetDelay =
                2 * m_phy->GetSifs() +
                WifiPhy::CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand()) +
                m_phy->CalculatePhyPreambleAndHeaderDuration(ctsTxVector) + 2 * m_phy->GetSlot();
            m_intraBssNavResetEvent =
                Simulator::Schedule(navResetDelay,
                                    &HeFrameExchangeManager::IntraBssNavResetTimeout,
                                    this);
        }
    }
    NS_LOG_DEBUG("Current intra-BSS NAV=" << m_intraBssNavEnd);

    m_channelAccessManager->NotifyNavStartNow(duration);
}

void
HeFrameExchangeManager::ClearTxopHolderIfNeeded()
{
    NS_LOG_FUNCTION(this);
    if (m_intraBssNavEnd <= Simulator::Now())
    {
        m_txopHolder.reset();
    }
}

void
HeFrameExchangeManager::NavResetTimeout()
{
    NS_LOG_FUNCTION(this);
    m_navEnd = Simulator::Now();
    // Do not reset the TXOP holder because the basic NAV is updated by inter-BSS frames
    // The NAV seen by the ChannelAccessManager is now the intra-BSS NAV only
    Time intraBssNav = Simulator::GetDelayLeft(m_intraBssNavResetEvent);
    m_channelAccessManager->NotifyNavResetNow(intraBssNav);
}

void
HeFrameExchangeManager::IntraBssNavResetTimeout()
{
    NS_LOG_FUNCTION(this);
    m_intraBssNavEnd = Simulator::Now();
    ClearTxopHolderIfNeeded();
    // The NAV seen by the ChannelAccessManager is now the basic NAV only
    Time basicNav = Simulator::GetDelayLeft(m_navResetEvent);
    m_channelAccessManager->NotifyNavResetNow(basicNav);
}

void
HeFrameExchangeManager::SetTxopHolder(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    if (psdu->GetHeader(0).IsTrigger() && psdu->GetAddr2() == m_bssid)
    {
        m_txopHolder = m_bssid;
    }
    else if (!txVector.IsUlMu()) // the sender of a TB PPDU is not the TXOP holder
    {
        VhtFrameExchangeManager::SetTxopHolder(psdu, txVector);
    }
}

bool
HeFrameExchangeManager::VirtualCsMediumIdle() const
{
    // For an HE STA maintaining two NAVs, if both the NAV timers are 0, the virtual CS indication
    // is that the medium is idle; if at least one of the two NAV timers is nonzero, the virtual CS
    // indication is that the medium is busy. (Sec. 26.2.4 of 802.11ax-2021)
    return m_navEnd <= Simulator::Now() && m_intraBssNavEnd <= Simulator::Now();
}

bool
HeFrameExchangeManager::UlMuCsMediumIdle(const CtrlTriggerHeader& trigger) const
{
    if (!trigger.GetCsRequired())
    {
        NS_LOG_DEBUG("CS not required");
        return true;
    }

    // A non-AP STA does not consider the intra-BSS NAV in determining whether to respond to a
    // Trigger frame sent by the AP with which the non-AP STA is associated.
    // A non-AP STA considers the basic NAV in determining whether to respond to a Trigger frame
    // sent by the AP with which the non-AP STA is associated. (Sec. 26.5.2.5 of 802.11ax-2021)
    const Time now = Simulator::Now();
    if (m_navEnd > now)
    {
        NS_LOG_DEBUG("Basic NAV indicates medium busy");
        return false;
    }

    NS_ASSERT_MSG(m_staMac, "UL MU CS is only performed by non-AP STAs");
    const auto userInfoIt = trigger.FindUserInfoWithAid(m_staMac->GetAssociationId());
    NS_ASSERT_MSG(userInfoIt != trigger.end(),
                  "No User Info field for STA (" << m_self
                                                 << ") AID=" << m_staMac->GetAssociationId());

    std::set<uint8_t> indices;

    if (trigger.IsMuRts())
    {
        auto ctsTxVector = GetCtsTxVectorAfterMuRts(trigger, m_staMac->GetAssociationId());
        auto bw = ctsTxVector.GetChannelWidth();
        indices = m_phy->GetOperatingChannel().GetAll20MHzChannelIndicesInPrimary(bw);
    }
    else
    {
        indices =
            m_phy->GetOperatingChannel().Get20MHzIndicesCoveringRu(userInfoIt->GetRuAllocation(),
                                                                   trigger.GetUlBandwidth());
    }
    return !m_channelAccessManager->GetPer20MHzBusy(indices);
}

void
HeFrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                    RxSignalInfo rxSignalInfo,
                                    const WifiTxVector& txVector,
                                    bool inAmpdu)
{
    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    const WifiMacHeader& hdr = mpdu->GetHeader();

    if (txVector.IsUlMu() && m_txTimer.IsRunning() &&
        m_txTimer.GetReason() == WifiTxTimer::WAIT_TB_PPDU_AFTER_BASIC_TF)
    {
        Mac48Address sender = hdr.GetAddr2();
        NS_ASSERT(m_txParams.m_acknowledgment &&
                  m_txParams.m_acknowledgment->method == WifiAcknowledgment::UL_MU_MULTI_STA_BA);
        WifiUlMuMultiStaBa* acknowledgment =
            static_cast<WifiUlMuMultiStaBa*>(m_txParams.m_acknowledgment.get());
        std::size_t index = acknowledgment->baType.m_bitmapLen.size();

        if (m_staExpectTbPpduFrom.find(sender) == m_staExpectTbPpduFrom.end())
        {
            NS_LOG_WARN("Received a TB PPDU from an unexpected station: " << sender);
            return;
        }

        if (hdr.IsBlockAckReq())
        {
            NS_LOG_DEBUG("Received a BlockAckReq in a TB PPDU from " << sender);

            CtrlBAckRequestHeader blockAckReq;
            mpdu->GetPacket()->PeekHeader(blockAckReq);
            NS_ABORT_MSG_IF(blockAckReq.IsMultiTid(), "Multi-TID BlockAckReq not supported");
            uint8_t tid = blockAckReq.GetTidInfo();
            GetBaManager(tid)->NotifyGotBlockAckRequest(
                m_mac->GetMldAddress(sender).value_or(sender),
                tid,
                blockAckReq.GetStartingSequence());

            // Block Acknowledgment context
            acknowledgment->stationsReceivingMultiStaBa.emplace(std::make_pair(sender, tid), index);
            acknowledgment->baType.m_bitmapLen.push_back(
                m_mac->GetBaTypeAsRecipient(sender, tid).m_bitmapLen.at(0));
            uint16_t staId = txVector.GetHeMuUserInfoMap().begin()->first;
            m_muSnrTag.Set(staId, rxSignalInfo.snr);
        }
        else if (hdr.IsQosData() && !inAmpdu && hdr.GetQosAckPolicy() == WifiMacHeader::NORMAL_ACK)
        {
            NS_LOG_DEBUG("Received an S-MPDU in a TB PPDU from " << sender << " (" << *mpdu << ")");

            uint8_t tid = hdr.GetQosTid();
            GetBaManager(tid)->NotifyGotMpdu(mpdu);

            // Acknowledgment context of Multi-STA Block Acks
            acknowledgment->stationsReceivingMultiStaBa.emplace(std::make_pair(sender, tid), index);
            acknowledgment->baType.m_bitmapLen.push_back(0);
            uint16_t staId = txVector.GetHeMuUserInfoMap().begin()->first;
            m_muSnrTag.Set(staId, rxSignalInfo.snr);
        }
        else if (!(hdr.IsQosData() && !hdr.HasData() && !inAmpdu))
        {
            // The other case handled by this function is when we receive a QoS Null frame
            // that is not in an A-MPDU. For all other cases, the reception is handled by
            // parent classes. In particular, in case of a QoS data frame in A-MPDU, we
            // have to wait until the A-MPDU reception is completed, but we let the
            // parent classes notify the Block Ack agreement of the reception of this MPDU
            VhtFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
            return;
        }

        // Schedule the transmission of a Multi-STA BlockAck frame if needed
        if (!acknowledgment->stationsReceivingMultiStaBa.empty() && !m_multiStaBaEvent.IsRunning())
        {
            m_multiStaBaEvent = Simulator::Schedule(m_phy->GetSifs(),
                                                    &HeFrameExchangeManager::SendMultiStaBlockAck,
                                                    this,
                                                    std::cref(m_txParams),
                                                    mpdu->GetHeader().GetDuration());
        }

        // remove the sender from the set of stations that are expected to send a TB PPDU
        m_staExpectTbPpduFrom.erase(sender);

        if (m_staExpectTbPpduFrom.empty())
        {
            // we do not expect any other BlockAck frame
            m_txTimer.Cancel();
            m_channelAccessManager->NotifyAckTimeoutResetNow();

            if (!m_multiStaBaEvent.IsRunning())
            {
                // all of the stations that replied with a TB PPDU sent QoS Null frames.
                NS_LOG_DEBUG("Continue the TXOP");
                m_psduMap.clear();
                m_edca->ResetCw(m_linkId);
                TransmissionSucceeded();
            }
        }

        // the received TB PPDU has been processed
        return;
    }

    if (txVector.IsUlMu() && m_txTimer.IsRunning() &&
        m_txTimer.GetReason() == WifiTxTimer::WAIT_QOS_NULL_AFTER_BSRP_TF &&
        !inAmpdu) // if in A-MPDU, processing is done at the end of A-MPDU reception
    {
        const auto& sender = hdr.GetAddr2();

        if (m_staExpectTbPpduFrom.find(sender) == m_staExpectTbPpduFrom.end())
        {
            NS_LOG_WARN("Received a TB PPDU from an unexpected station: " << sender);
            return;
        }
        if (!(hdr.IsQosData() && !hdr.HasData()))
        {
            NS_LOG_WARN("No QoS Null frame in the received MPDU");
            return;
        }

        NS_LOG_DEBUG("Received a QoS Null frame in a TB PPDU from " << sender);

        // remove the sender from the set of stations that are expected to send a TB PPDU
        m_staExpectTbPpduFrom.erase(sender);

        if (m_staExpectTbPpduFrom.empty())
        {
            // we do not expect any other response
            m_txTimer.Cancel();
            m_channelAccessManager->NotifyAckTimeoutResetNow();

            NS_ASSERT(m_edca);
            m_psduMap.clear();
            m_edca->ResetCw(m_linkId);
            TransmissionSucceeded();
        }

        // the received TB PPDU has been processed
        return;
    }

    if (hdr.IsCtl())
    {
        if (hdr.IsCts() && m_txTimer.IsRunning() &&
            m_txTimer.GetReason() == WifiTxTimer::WAIT_CTS && m_psduMap.size() == 1)
        {
            NS_ABORT_MSG_IF(inAmpdu, "Received CTS as part of an A-MPDU");
            NS_ASSERT(hdr.GetAddr1() == m_self);

            Mac48Address sender = m_psduMap.begin()->second->GetAddr1();
            NS_LOG_DEBUG("Received CTS from=" << sender);

            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);
            GetWifiRemoteStationManager()->ReportRxOk(sender, rxSignalInfo, txVector);
            GetWifiRemoteStationManager()->ReportRtsOk(m_psduMap.begin()->second->GetHeader(0),
                                                       rxSignalInfo.snr,
                                                       txVector.GetMode(),
                                                       tag.Get());

            m_txTimer.Cancel();
            m_channelAccessManager->NotifyCtsTimeoutResetNow();
            Simulator::Schedule(m_phy->GetSifs(),
                                &HeFrameExchangeManager::ProtectionCompleted,
                                this);
        }
        else if (hdr.IsCts() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_CTS_AFTER_MU_RTS)
        {
            NS_ABORT_MSG_IF(inAmpdu, "Received CTS as part of an A-MPDU");
            NS_ASSERT(hdr.GetAddr1() == m_self);

            NS_LOG_DEBUG("Received a CTS frame in response to an MU-RTS");

            m_txTimer.Cancel();
            m_channelAccessManager->NotifyCtsTimeoutResetNow();
            Simulator::Schedule(m_phy->GetSifs(),
                                &HeFrameExchangeManager::ProtectionCompleted,
                                this);
        }
        else if (hdr.IsAck() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_NORMAL_ACK_AFTER_DL_MU_PPDU)
        {
            NS_ASSERT(hdr.GetAddr1() == m_self);
            NS_ASSERT(m_txParams.m_acknowledgment);
            NS_ASSERT(m_txParams.m_acknowledgment->method ==
                      WifiAcknowledgment::DL_MU_BAR_BA_SEQUENCE);

            WifiDlMuBarBaSequence* acknowledgment =
                static_cast<WifiDlMuBarBaSequence*>(m_txParams.m_acknowledgment.get());
            NS_ASSERT(acknowledgment->stationsReplyingWithNormalAck.size() == 1);
            NS_ASSERT(m_apMac);
            uint16_t staId = m_apMac->GetAssociationId(
                acknowledgment->stationsReplyingWithNormalAck.begin()->first,
                m_linkId);
            auto it = m_psduMap.find(staId);
            NS_ASSERT(it != m_psduMap.end());
            NS_ASSERT(it->second->GetAddr1() ==
                      acknowledgment->stationsReplyingWithNormalAck.begin()->first);
            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);
            ReceivedNormalAck(*it->second->begin(),
                              m_txParams.m_txVector,
                              txVector,
                              rxSignalInfo,
                              tag.Get());
            m_psduMap.clear();
        }
        // TODO the PHY should not pass us a non-TB PPDU if we are waiting for a
        // TB PPDU. However, processing the PHY header is done by the PHY entity
        // corresponding to the modulation class of the PPDU being received, hence
        // it is not possible to check if a valid TRIGVECTOR is stored when receiving
        // PPDUs of older modulation classes. Therefore, we check here that we are
        // actually receiving a TB PPDU.
        else if (hdr.IsBlockAck() && txVector.IsUlMu() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_BLOCK_ACKS_IN_TB_PPDU)
        {
            Mac48Address sender = hdr.GetAddr2();
            NS_LOG_DEBUG("Received BlockAck in TB PPDU from=" << sender);

            SnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);

            // notify the Block Ack Manager
            CtrlBAckResponseHeader blockAck;
            mpdu->GetPacket()->PeekHeader(blockAck);
            uint8_t tid = blockAck.GetTidInfo();
            std::pair<uint16_t, uint16_t> ret =
                GetBaManager(tid)->NotifyGotBlockAck(m_linkId,
                                                     blockAck,
                                                     m_mac->GetMldAddress(sender).value_or(sender),
                                                     {tid});
            GetWifiRemoteStationManager()->ReportAmpduTxStatus(sender,
                                                               ret.first,
                                                               ret.second,
                                                               rxSignalInfo.snr,
                                                               tag.Get(),
                                                               m_txParams.m_txVector);

            // remove the sender from the set of stations that are expected to send a BlockAck
            if (m_staExpectTbPpduFrom.erase(sender) == 0)
            {
                NS_LOG_WARN("Received a BlockAck from an unexpected stations: " << sender);
                return;
            }

            if (m_staExpectTbPpduFrom.empty())
            {
                // we do not expect any other BlockAck frame
                m_txTimer.Cancel();
                m_channelAccessManager->NotifyAckTimeoutResetNow();
                if (m_triggerFrame)
                {
                    // this is strictly needed for DL_MU_TF_MU_BAR only
                    DequeueMpdu(m_triggerFrame);
                    m_triggerFrame = nullptr;
                }

                m_edca->ResetCw(m_linkId);
                m_psduMap.clear();
                TransmissionSucceeded();
            }
        }
        else if (hdr.IsBlockAck() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_BLOCK_ACK_AFTER_TB_PPDU)
        {
            CtrlBAckResponseHeader blockAck;
            mpdu->GetPacket()->PeekHeader(blockAck);

            NS_ABORT_MSG_IF(!blockAck.IsMultiSta(),
                            "A Multi-STA BlockAck is expected after a TB PPDU");
            NS_LOG_DEBUG("Received a Multi-STA BlockAck from=" << hdr.GetAddr2());

            NS_ASSERT(m_staMac && m_staMac->IsAssociated());
            if (hdr.GetAddr2() != m_bssid)
            {
                NS_LOG_DEBUG("The sender is not the AP we are associated with");
                return;
            }

            uint16_t staId = m_staMac->GetAssociationId();
            std::vector<uint32_t> indices = blockAck.FindPerAidTidInfoWithAid(staId);

            if (indices.empty())
            {
                NS_LOG_DEBUG("No Per AID TID Info subfield intended for me");
                return;
            }

            MuSnrTag tag;
            mpdu->GetPacket()->PeekPacketTag(tag);

            // notify the Block Ack Manager
            for (const auto& index : indices)
            {
                uint8_t tid = blockAck.GetTidInfo(index);

                if (blockAck.GetAckType(index) && tid < 8)
                {
                    // Acknowledgment context
                    NS_ABORT_IF(m_psduMap.empty() || m_psduMap.begin()->first != staId);
                    GetBaManager(tid)->NotifyGotAck(m_linkId, *m_psduMap.at(staId)->begin());
                }
                else
                {
                    // Block Acknowledgment or All-ack context
                    if (blockAck.GetAckType(index) && tid == 14)
                    {
                        // All-ack context, we need to determine the actual TID(s) of the PSDU
                        NS_ASSERT(indices.size() == 1);
                        NS_ABORT_IF(m_psduMap.empty() || m_psduMap.begin()->first != staId);
                        std::set<uint8_t> tids = m_psduMap.at(staId)->GetTids();
                        NS_ABORT_MSG_IF(tids.size() > 1, "Multi-TID A-MPDUs not supported yet");
                        tid = *tids.begin();
                    }

                    std::pair<uint16_t, uint16_t> ret = GetBaManager(tid)->NotifyGotBlockAck(
                        m_linkId,
                        blockAck,
                        m_mac->GetMldAddress(hdr.GetAddr2()).value_or(hdr.GetAddr2()),
                        {tid},
                        index);
                    GetWifiRemoteStationManager()->ReportAmpduTxStatus(hdr.GetAddr2(),
                                                                       ret.first,
                                                                       ret.second,
                                                                       rxSignalInfo.snr,
                                                                       tag.Get(staId),
                                                                       m_txParams.m_txVector);
                }

                if (m_psduMap.at(staId)->GetHeader(0).IsQosData() &&
                    (blockAck.GetAckType(index) // Ack or All-ack context
                     || std::any_of(blockAck.GetBitmap(index).begin(),
                                    blockAck.GetBitmap(index).end(),
                                    [](uint8_t b) { return b != 0; })))
                {
                    NS_ASSERT(m_psduMap.at(staId)->GetHeader(0).HasData());
                    NS_ASSERT(m_psduMap.at(staId)->GetHeader(0).GetQosTid() == tid);
                    // the station has received a response from the AP for the HE TB PPDU
                    // transmitted in response to a Basic Trigger Frame and at least one
                    // MPDU was acknowledged. Therefore, it needs to update the access
                    // parameters if it received an MU EDCA Parameter Set element.
                    m_mac->GetQosTxop(tid)->StartMuEdcaTimerNow(m_linkId);
                }
            }

            // cancel the timer
            m_txTimer.Cancel();
            m_channelAccessManager->NotifyAckTimeoutResetNow();
            // dequeue BlockAckReq frames included in acknowledged TB PPDUs (if any)
            for (const auto& [staId, psdu] : m_psduMap)
            {
                if (psdu->GetNMpdus() == 1 && psdu->GetHeader(0).IsBlockAckReq())
                {
                    DequeuePsdu(psdu);
                }
            }
            m_psduMap.clear();
        }
        else if (hdr.IsBlockAck() && m_txTimer.IsRunning() &&
                 m_txTimer.GetReason() == WifiTxTimer::WAIT_BLOCK_ACK)
        {
            // this BlockAck frame may have been sent in response to a DL MU PPDU with
            // acknowledgment in SU format or one of the consequent BlockAckReq frames.
            // We clear the PSDU map and let parent classes continue processing this frame.
            m_psduMap.clear();
            VhtFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
        }
        else if (hdr.IsTrigger())
        {
            // Trigger Frames are only processed by STAs
            if (!m_staMac)
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
            mpdu->GetPacket()->PeekHeader(trigger);

            if (hdr.GetAddr1() != m_self &&
                (!hdr.GetAddr1().IsBroadcast() || !m_staMac->IsAssociated() ||
                 hdr.GetAddr2() != m_bssid // not sent by the AP this STA is associated with
                 || trigger.FindUserInfoWithAid(m_staMac->GetAssociationId()) == trigger.end()))
            {
                // not addressed to us
                return;
            }

            uint16_t staId = m_staMac->GetAssociationId();

            if (trigger.IsMuRts())
            {
                Mac48Address sender = hdr.GetAddr2();
                NS_LOG_DEBUG("Received MU-RTS Trigger Frame from=" << sender);
                GetWifiRemoteStationManager()->ReportRxOk(sender, rxSignalInfo, txVector);

                // If a non-AP STA receives an MU-RTS Trigger frame, the non-AP STA shall commence
                // the transmission of a CTS frame response at the SIFS time boundary after
                // the end of a received PPDU when all the following conditions are met:
                // - The MU-RTS Trigger frame has one of the User Info fields addressed to
                //   the non-AP STA (this is guaranteed if we get here)
                // - The UL MU CS condition indicates that the medium is idle
                // (Sec. 26.2.6.3 of 802.11ax-2021)
                NS_LOG_DEBUG("Schedule CTS");
                Simulator::Schedule(m_phy->GetSifs(),
                                    &HeFrameExchangeManager::SendCtsAfterMuRts,
                                    this,
                                    hdr,
                                    trigger,
                                    rxSignalInfo.snr);
            }
            else if (trigger.IsMuBar())
            {
                Mac48Address sender = hdr.GetAddr2();
                NS_LOG_DEBUG("Received MU-BAR Trigger Frame from=" << sender);
                GetWifiRemoteStationManager()->ReportRxOk(sender, rxSignalInfo, txVector);

                auto userInfoIt = trigger.FindUserInfoWithAid(staId);
                NS_ASSERT(userInfoIt != trigger.end());
                CtrlBAckRequestHeader blockAckReq = userInfoIt->GetMuBarTriggerDepUserInfo();
                NS_ABORT_MSG_IF(blockAckReq.IsMultiTid(), "Multi-TID BlockAckReq not supported");
                uint8_t tid = blockAckReq.GetTidInfo();

                GetBaManager(tid)->NotifyGotBlockAckRequest(
                    m_mac->GetMldAddress(sender).value_or(sender),
                    tid,
                    blockAckReq.GetStartingSequence());

                Simulator::Schedule(m_phy->GetSifs(),
                                    &HeFrameExchangeManager::ReceiveMuBarTrigger,
                                    this,
                                    trigger,
                                    tid,
                                    hdr.GetDuration(),
                                    rxSignalInfo.snr);
            }
            else if (trigger.IsBasic())
            {
                Simulator::Schedule(m_phy->GetSifs(),
                                    &HeFrameExchangeManager::ReceiveBasicTrigger,
                                    this,
                                    trigger,
                                    hdr);
            }
            else if (trigger.IsBsrp())
            {
                Simulator::Schedule(m_phy->GetSifs(),
                                    &HeFrameExchangeManager::SendQosNullFramesInTbPpdu,
                                    this,
                                    trigger,
                                    hdr);
            }
        }
        else
        {
            // the received control frame cannot be handled here
            VhtFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
        }

        // the received control frame has been processed
        return;
    }

    // the received frame cannot be handled here
    VhtFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
    ;
}

void
HeFrameExchangeManager::EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                                        const RxSignalInfo& rxSignalInfo,
                                        const WifiTxVector& txVector,
                                        const std::vector<bool>& perMpduStatus)
{
    std::set<uint8_t> tids = psdu->GetTids();

    if (txVector.IsUlMu() && m_txTimer.IsRunning() &&
        m_txTimer.GetReason() == WifiTxTimer::WAIT_TB_PPDU_AFTER_BASIC_TF)
    {
        Mac48Address sender = psdu->GetAddr2();
        NS_ASSERT(m_txParams.m_acknowledgment &&
                  m_txParams.m_acknowledgment->method == WifiAcknowledgment::UL_MU_MULTI_STA_BA);
        WifiUlMuMultiStaBa* acknowledgment =
            static_cast<WifiUlMuMultiStaBa*>(m_txParams.m_acknowledgment.get());
        std::size_t index = acknowledgment->baType.m_bitmapLen.size();

        if (m_staExpectTbPpduFrom.find(sender) == m_staExpectTbPpduFrom.end())
        {
            NS_LOG_WARN("Received a TB PPDU from an unexpected station: " << sender);
            return;
        }

        NS_LOG_DEBUG("Received an A-MPDU in a TB PPDU from " << sender << " (" << *psdu << ")");

        if (std::any_of(tids.begin(), tids.end(), [&psdu](uint8_t tid) {
                return psdu->GetAckPolicyForTid(tid) == WifiMacHeader::NORMAL_ACK;
            }))
        {
            if (std::all_of(perMpduStatus.cbegin(), perMpduStatus.cend(), [](bool v) { return v; }))
            {
                // All-ack context
                acknowledgment->stationsReceivingMultiStaBa.emplace(std::make_pair(sender, 14),
                                                                    index);
                acknowledgment->baType.m_bitmapLen.push_back(0);
            }
            else
            {
                // Block Acknowledgment context
                std::size_t i = 0;
                for (const auto& tid : tids)
                {
                    acknowledgment->stationsReceivingMultiStaBa.emplace(std::make_pair(sender, tid),
                                                                        index + i++);
                    acknowledgment->baType.m_bitmapLen.push_back(
                        m_mac->GetBaTypeAsRecipient(sender, tid).m_bitmapLen.at(0));
                }
            }
            uint16_t staId = txVector.GetHeMuUserInfoMap().begin()->first;
            m_muSnrTag.Set(staId, rxSignalInfo.snr);
        }

        // Schedule the transmission of a Multi-STA BlockAck frame if needed
        if (!acknowledgment->stationsReceivingMultiStaBa.empty() && !m_multiStaBaEvent.IsRunning())
        {
            m_multiStaBaEvent = Simulator::Schedule(m_phy->GetSifs(),
                                                    &HeFrameExchangeManager::SendMultiStaBlockAck,
                                                    this,
                                                    std::cref(m_txParams),
                                                    psdu->GetDuration());
        }

        // remove the sender from the set of stations that are expected to send a TB PPDU
        m_staExpectTbPpduFrom.erase(sender);

        if (m_staExpectTbPpduFrom.empty())
        {
            // we do not expect any other BlockAck frame
            m_txTimer.Cancel();
            m_channelAccessManager->NotifyAckTimeoutResetNow();

            if (!m_multiStaBaEvent.IsRunning())
            {
                // all of the stations that replied with a TB PPDU sent QoS Null frames.
                NS_LOG_DEBUG("Continue the TXOP");
                m_psduMap.clear();
                m_edca->ResetCw(m_linkId);
                TransmissionSucceeded();
            }
        }

        // the received TB PPDU has been processed
        return;
    }

    if (txVector.IsUlMu() && m_txTimer.IsRunning() &&
        m_txTimer.GetReason() == WifiTxTimer::WAIT_QOS_NULL_AFTER_BSRP_TF)
    {
        Mac48Address sender = psdu->GetAddr2();

        if (m_staExpectTbPpduFrom.find(sender) == m_staExpectTbPpduFrom.end())
        {
            NS_LOG_WARN("Received a TB PPDU from an unexpected station: " << sender);
            return;
        }
        if (std::none_of(psdu->begin(), psdu->end(), [](Ptr<WifiMpdu> mpdu) {
                return mpdu->GetHeader().IsQosData() && !mpdu->GetHeader().HasData();
            }))
        {
            NS_LOG_WARN("No QoS Null frame in the received PSDU");
            return;
        }

        NS_LOG_DEBUG("Received QoS Null frames in a TB PPDU from " << sender);

        // remove the sender from the set of stations that are expected to send a TB PPDU
        m_staExpectTbPpduFrom.erase(sender);

        if (m_staExpectTbPpduFrom.empty())
        {
            // we do not expect any other response
            m_txTimer.Cancel();
            m_channelAccessManager->NotifyAckTimeoutResetNow();

            NS_ASSERT(m_edca);
            m_psduMap.clear();
            m_edca->ResetCw(m_linkId);
            TransmissionSucceeded();
        }

        // the received TB PPDU has been processed
        return;
    }

    if (m_triggerFrameInAmpdu)
    {
        // the received A-MPDU contains a Trigger Frame. It is now time to handle it.
        auto psduIt = psdu->begin();
        while (psduIt != psdu->end())
        {
            if ((*psduIt)->GetHeader().IsTrigger())
            {
                ReceiveMpdu(*psduIt, rxSignalInfo, txVector, false);
            }
            psduIt++;
        }

        m_triggerFrameInAmpdu = false;
        return;
    }

    // the received frame cannot be handled here
    VhtFrameExchangeManager::EndReceiveAmpdu(psdu, rxSignalInfo, txVector, perMpduStatus);
}

} // namespace ns3
