/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
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

#include "eht-frame-exchange-manager.h"

#include "eht-phy.h"
#include "emlsr-manager.h"

#include "ns3/abort.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-mac-queue.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "][mac=" << m_self << "] "

namespace ns3
{

/// generic value for aRxPHYStartDelay PHY characteristic (used when we do not know the preamble
/// type of the next frame)
static constexpr uint8_t RX_PHY_START_DELAY_USEC = 48;

NS_LOG_COMPONENT_DEFINE("EhtFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(EhtFrameExchangeManager);

TypeId
EhtFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::EhtFrameExchangeManager")
                            .SetParent<HeFrameExchangeManager>()
                            .AddConstructor<EhtFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

EhtFrameExchangeManager::EhtFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
}

EhtFrameExchangeManager::~EhtFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
EhtFrameExchangeManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_ongoingTxopEnd.Cancel();
    HeFrameExchangeManager::DoDispose();
}

void
EhtFrameExchangeManager::RxStartIndication(WifiTxVector txVector, Time psduDuration)
{
    NS_LOG_FUNCTION(this << txVector << psduDuration.As(Time::MS));

    HeFrameExchangeManager::RxStartIndication(txVector, psduDuration);
    UpdateTxopEndOnRxStartIndication(psduDuration);
}

void
EhtFrameExchangeManager::SetLinkId(uint8_t linkId)
{
    if (auto protectionManager = GetProtectionManager())
    {
        protectionManager->SetLinkId(linkId);
    }
    if (auto ackManager = GetAckManager())
    {
        ackManager->SetLinkId(linkId);
    }
    m_msduAggregator->SetLinkId(linkId);
    m_mpduAggregator->SetLinkId(linkId);
    HeFrameExchangeManager::SetLinkId(linkId);
}

Ptr<WifiMpdu>
EhtFrameExchangeManager::CreateAliasIfNeeded(Ptr<WifiMpdu> mpdu) const
{
    NS_LOG_FUNCTION(this << *mpdu);

    // alias needs only be created for non-broadcast QoS data frames exchanged between two MLDs
    if (!mpdu->GetHeader().IsQosData() || m_mac->GetNLinks() == 1 ||
        mpdu->GetHeader().GetAddr1().IsGroup() ||
        !GetWifiRemoteStationManager()->GetMldAddress(mpdu->GetHeader().GetAddr1()))
    {
        return HeFrameExchangeManager::CreateAliasIfNeeded(mpdu);
    }

    mpdu = mpdu->CreateAlias(m_linkId);
    auto& hdr = mpdu->GetHeader();
    hdr.SetAddr2(GetAddress());
    auto address = GetWifiRemoteStationManager()->GetAffiliatedStaAddress(hdr.GetAddr1());
    NS_ASSERT(address);
    hdr.SetAddr1(*address);
    /*
     * Set Address3 according to Table 9-30 of 802.11-2020 and Section 35.3.3 of
     * 802.11be D2.0 ["the value of the Address 3 field and the Address 4 field (if present)
     * in the MAC header of a data frame shall be set based on Table 9-30 (Address field
     * contents) and the settings of the To DS and From DS bits, where the BSSID is the
     * MAC address of the AP affiliated with the AP MLD corresponding to that link"].
     */
    if (hdr.IsQosAmsdu())
    {
        if (hdr.IsToDs() && !hdr.IsFromDs())
        {
            // from STA to AP: BSSID is in Address1
            hdr.SetAddr3(hdr.GetAddr1());
        }
        else if (!hdr.IsToDs() && hdr.IsFromDs())
        {
            // from AP to STA: BSSID is in Address2
            hdr.SetAddr3(hdr.GetAddr2());
        }
    }

    return mpdu;
}

bool
EhtFrameExchangeManager::StartTransmission(Ptr<Txop> edca, uint16_t allowedWidth)
{
    NS_LOG_FUNCTION(this << edca << allowedWidth);

    auto started = HeFrameExchangeManager::StartTransmission(edca, allowedWidth);

    if (started && m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        // notify the EMLSR Manager of the UL TXOP start on an EMLSR link
        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyUlTxopStart(m_linkId);
    }

    return started;
}

void
EhtFrameExchangeManager::ForwardPsduDown(Ptr<const WifiPsdu> psdu, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    // EHT-SIG, the equivalent of HE-SIG-B, is present in EHT SU transmissions, too
    if (txVector.GetPreambleType() == WIFI_PREAMBLE_EHT_MU)
    {
        auto phy = StaticCast<EhtPhy>(m_phy->GetPhyEntity(WIFI_MOD_CLASS_EHT));
        auto sigBMode = phy->GetSigBMode(txVector);
        txVector.SetSigBMode(sigBMode);
    }

    auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, m_phy->GetPhyBand());

    HeFrameExchangeManager::ForwardPsduDown(psdu, txVector);
    UpdateTxopEndOnTxStart(txDuration);

    if (m_apMac)
    {
        // check if the EMLSR clients shall switch back to listening operation at the end of this
        // PPDU
        for (auto clientIt = m_protectedStas.begin(); clientIt != m_protectedStas.end();)
        {
            auto aid = GetWifiRemoteStationManager()->GetAssociationId(*clientIt);

            if (GetWifiRemoteStationManager()->GetEmlsrEnabled(*clientIt) &&
                GetEmlsrSwitchToListening(psdu, aid, *clientIt))
            {
                EmlsrSwitchToListening(*clientIt, txDuration);
                // this client is no longer involved in the current TXOP
                clientIt = m_protectedStas.erase(clientIt);
            }
            else
            {
                clientIt++;
            }
        }
    }
}

void
EhtFrameExchangeManager::ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);

    auto txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, m_phy->GetPhyBand());

    HeFrameExchangeManager::ForwardPsduMapDown(psduMap, txVector);
    UpdateTxopEndOnTxStart(txDuration);

    if (m_apMac)
    {
        // check if the EMLSR clients shall switch back to listening operation at the end of this
        // PPDU
        for (auto clientIt = m_protectedStas.begin(); clientIt != m_protectedStas.end();)
        {
            auto aid = GetWifiRemoteStationManager()->GetAssociationId(*clientIt);

            if (auto psduMapIt = psduMap.find(aid);
                GetWifiRemoteStationManager()->GetEmlsrEnabled(*clientIt) &&
                (psduMapIt == psduMap.cend() ||
                 GetEmlsrSwitchToListening(psduMapIt->second, aid, *clientIt)))
            {
                EmlsrSwitchToListening(*clientIt, txDuration);
                // this client is no longer involved in the current TXOP
                clientIt = m_protectedStas.erase(clientIt);
            }
            else
            {
                clientIt++;
            }
        }
    }
}

void
EhtFrameExchangeManager::EmlsrSwitchToListening(const Mac48Address& address, const Time& delay)
{
    NS_LOG_FUNCTION(this << address << delay.As(Time::US));

    // this EMLSR client switches back to listening operation a transition delay
    // after the given delay
    auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);
    NS_ASSERT(mldAddress);
    auto emlCapabilities = GetWifiRemoteStationManager()->GetStationEmlCapabilities(address);
    NS_ASSERT(emlCapabilities);

    for (uint8_t linkId = 0; linkId < m_mac->GetNLinks(); linkId++)
    {
        if (m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
        {
            Simulator::Schedule(delay, [=]() {
                if (linkId != m_linkId)
                {
                    // the reason for blocking the other EMLSR links has changed now
                    m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                                   *mldAddress,
                                                   {linkId});
                }

                // block DL transmissions on this link until transition delay elapses
                m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                             *mldAddress,
                                             {linkId});
            });

            // unblock all EMLSR links when the transition delay elapses
            Simulator::Schedule(delay + CommonInfoBasicMle::DecodeEmlsrTransitionDelay(
                                            emlCapabilities->get().emlsrTransitionDelay),
                                [=]() {
                                    m_mac->UnblockUnicastTxOnLinks(
                                        WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                        *mldAddress,
                                        {linkId});
                                });
        }
    }
}

void
EhtFrameExchangeManager::NotifySwitchingEmlsrLink(Ptr<WifiPhy> phy, uint8_t linkId, Time delay)
{
    NS_LOG_FUNCTION(this << phy << linkId << delay.As(Time::US));

    // TODO Shall we assert that there is no ongoing frame exchange sequence? Or is it possible
    // that there is an ongoing frame exchange sequence (in such a case, we need to force a
    // timeout, just like it is done in case of a normal channel switch

    NS_ABORT_MSG_IF(!m_staMac, "This method can only be called on a STA");

    // if we receive the notification from a PHY that is not connected to us, it means that
    // we have been already connected to another PHY operating on this link, hence we do not
    // have to reset the connected PHY
    if (phy == m_phy)
    {
        ResetPhy();
    }
    m_staMac->NotifySwitchingEmlsrLink(phy, linkId);
}

void
EhtFrameExchangeManager::SendEmlOmn(const Mac48Address& dest, const MgtEmlOmn& frame)
{
    NS_LOG_FUNCTION(this << dest << frame);

    WifiMacHeader hdr;
    hdr.SetType(WIFI_MAC_MGT_ACTION);
    hdr.SetAddr1(dest);
    hdr.SetAddr2(m_self);
    hdr.SetAddr3(m_bssid);
    hdr.SetDsNotTo();
    hdr.SetDsNotFrom();

    // get the sequence number for the TWT Setup management frame
    const auto sequence = m_txMiddle->GetNextSequenceNumberFor(&hdr);
    hdr.SetSequenceNumber(sequence);

    WifiActionHeader actionHdr;
    WifiActionHeader::ActionValue action;
    action.protectedEhtAction = WifiActionHeader::PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION;
    actionHdr.SetAction(WifiActionHeader::PROTECTED_EHT, action);

    auto packet = Create<Packet>();
    packet->AddHeader(frame);
    packet->AddHeader(actionHdr);

    // Use AC_VO to send management frame addressed to a QoS STA (Sec. 10.2.3.2 of 802.11-2020)
    m_mac->GetQosTxop(AC_VO)->Queue(Create<WifiMpdu>(packet, hdr));
}

std::optional<double>
EhtFrameExchangeManager::GetMostRecentRssi(const Mac48Address& address) const
{
    auto optRssi = HeFrameExchangeManager::GetMostRecentRssi(address);

    if (optRssi)
    {
        return optRssi;
    }

    auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);

    if (!mldAddress)
    {
        // not an MLD, nothing else can be done
        return std::nullopt;
    }

    for (uint8_t linkId = 0; linkId < m_mac->GetNLinks(); linkId++)
    {
        std::optional<Mac48Address> linkAddress;
        if (linkId != m_linkId &&
            (linkAddress = m_mac->GetWifiRemoteStationManager(linkId)->GetAffiliatedStaAddress(
                 *mldAddress)) &&
            (optRssi = m_mac->GetWifiRemoteStationManager(linkId)->GetMostRecentRssi(*linkAddress)))
        {
            return optRssi;
        }
    }

    return std::nullopt;
}

void
EhtFrameExchangeManager::SendMuRts(const WifiTxParameters& txParams)
{
    NS_LOG_FUNCTION(this << &txParams);

    uint8_t maxPaddingDelay = 0;

    // block transmissions on the other EMLSR links of the EMLSR clients
    for (const auto& address : m_sentRtsTo)
    {
        if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
        {
            continue;
        }

        auto emlCapabilities = GetWifiRemoteStationManager()->GetStationEmlCapabilities(address);
        NS_ASSERT(emlCapabilities);
        maxPaddingDelay = std::max(maxPaddingDelay, emlCapabilities->get().emlsrPaddingDelay);

        auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);
        NS_ASSERT(mldAddress);

        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            if (linkId != m_linkId &&
                m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
            {
                m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                             *mldAddress,
                                             {linkId});
            }
        }
    }

    // add padding (if needed)
    if (maxPaddingDelay > 0)
    {
        NS_ASSERT(txParams.m_protection &&
                  txParams.m_protection->method == WifiProtection::MU_RTS_CTS);
        auto protection = static_cast<WifiMuRtsCtsProtection*>(txParams.m_protection.get());
        NS_ASSERT(protection->muRts.IsMuRts());

        // see formula (35-1) in Sec. 35.5.2.2.3 of 802.11be D3.0
        auto rate = protection->muRtsTxVector.GetMode().GetDataRate(protection->muRtsTxVector);
        std::size_t nDbps = rate / 1e6 * 4; // see Table 17-4 of 802.11-2020
        protection->muRts.SetPaddingSize((1 << (maxPaddingDelay + 2)) * nDbps / 8);
    }

    HeFrameExchangeManager::SendMuRts(txParams);
}

bool
EhtFrameExchangeManager::GetEmlsrSwitchToListening(Ptr<const WifiPsdu> psdu,
                                                   uint16_t aid,
                                                   const Mac48Address& address) const
{
    NS_LOG_FUNCTION(this << psdu << aid << address);

    // Sec. 35.3.17 of 802.11be D3.0:
    // The non-AP MLD shall be switched back to the listening operation on the EMLSR links after
    // the EMLSR transition delay time if [...] the non-AP STA affiliated with the non-AP MLD
    // does not detect [...] any of the following frames:
    // - an individually addressed frame with the RA equal to the MAC address of the non-AP STA
    // affiliated with the non-AP MLD
    if (psdu->GetAddr1() == address)
    {
        return false;
    }

    // - a Trigger frame that has one of the User Info fields addressed to the non-AP STA
    // affiliated with the non-AP MLD
    for (const auto& mpdu : *PeekPointer(psdu))
    {
        if (mpdu->GetHeader().IsTrigger())
        {
            CtrlTriggerHeader trigger;
            mpdu->GetPacket()->PeekHeader(trigger);
            if (trigger.FindUserInfoWithAid(aid) != trigger.end())
            {
                return false;
            }
        }
    }

    // - a CTS-to-self frame with the RA equal to the MAC address of the AP affiliated with
    // the AP MLD
    if (psdu->GetHeader(0).IsCts())
    {
        if (m_apMac && psdu->GetAddr1() == m_self)
        {
            return false;
        }
        if (m_staMac && psdu->GetAddr1() == m_bssid)
        {
            return false;
        }
    }

    // - a Multi-STA BlockAck frame that has one of the Per AID TID Info fields addressed to
    // the non-AP STA affiliated with the non-AP MLD
    if (psdu->GetHeader(0).IsBlockAck())
    {
        CtrlBAckResponseHeader blockAck;
        psdu->GetPayload(0)->PeekHeader(blockAck);
        if (blockAck.IsMultiSta() && !blockAck.FindPerAidTidInfoWithAid(aid).empty())
        {
            return false;
        }
    }

    // - a NDP Announcement frame that has one of the STA Info fields addressed to the non-AP
    // STA affiliated with the non-AP MLD and a sounding NDP
    // TODO NDP Announcement frame not supported yet

    return true;
}

void
EhtFrameExchangeManager::TransmissionFailed()
{
    NS_LOG_FUNCTION(this);

    for (const auto& address : m_txTimer.GetStasExpectedToRespond())
    {
        if (GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
        {
            // This EMLSR client did not respond to a frame sent by the AP. Specs say:
            // The AP affiliated with the AP MLD should transmit before the TXNAV timer expires
            // another initial Control frame addressed to the non-AP STA affiliated with the
            // non-AP MLD if the AP intends to continue the frame exchanges with the STA and did
            // not receive the response frame from this STA for the most recently transmitted
            // frame that requires an immediate response after a SIFS
            // (Sec. 35.3.17 of 802.11be D3.1)
            // We let the AP continue the TXOP. TransmissionSucceeded() removes this client from
            // protected stations, hence next transmission to this client in this TXOP will be
            // protected by ICF
            NS_LOG_DEBUG("EMLSR client " << address << " did not respond, continue TXOP");
            TransmissionSucceeded();
            return;
        }
    }

    HeFrameExchangeManager::TransmissionFailed();
}

void
EhtFrameExchangeManager::NotifyChannelReleased(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);

    if (m_apMac)
    {
        // the channel has been released; all EMLSR clients will switch back to listening
        // operation after a timeout interval of aSIFSTime + aSlotTime + aRxPHYStartDelay
        auto delay = m_phy->GetSifs() + m_phy->GetSlot() + MicroSeconds(RX_PHY_START_DELAY_USEC);
        for (const auto& address : m_protectedStas)
        {
            if (GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
            {
                EmlsrSwitchToListening(address, delay);
            }
        }
    }
    else if (m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        // notify the EMLSR Manager of the UL TXOP end
        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyTxopEnd(m_linkId);
    }

    HeFrameExchangeManager::NotifyChannelReleased(txop);
}

void
EhtFrameExchangeManager::PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    HeFrameExchangeManager::PostProcessFrame(psdu, txVector);

    if (m_apMac && m_txopHolder == psdu->GetAddr2() &&
        GetWifiRemoteStationManager()->GetEmlsrEnabled(*m_txopHolder))
    {
        if (!m_ongoingTxopEnd.IsRunning())
        {
            // an EMLSR client has started an UL TXOP. We may send a response after a SIFS or
            // we may receive another frame after a SIFS. Postpone the TXOP end by considering
            // the latter (which takes longer)
            auto delay =
                m_phy->GetSifs() + m_phy->GetSlot() + MicroSeconds(RX_PHY_START_DELAY_USEC);
            NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + delay).As(Time::S));
            m_ongoingTxopEnd = Simulator::Schedule(delay, &EhtFrameExchangeManager::TxopEnd, this);

            // block transmissions on other links
            auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(psdu->GetAddr2());
            NS_ASSERT(mldAddress);

            for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
            {
                if (linkId != m_linkId &&
                    m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
                {
                    m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                                 *mldAddress,
                                                 {linkId});
                }
            }
        }
        else
        {
            UpdateTxopEndOnRxEnd();
        }
    }

    if (m_staMac && m_ongoingTxopEnd.IsRunning())
    {
        if (GetEmlsrSwitchToListening(psdu, m_staMac->GetAssociationId(), m_self))
        {
            // we are no longer involved in the TXOP and switching to listening mode
            m_ongoingTxopEnd.Cancel();
            m_staMac->GetEmlsrManager()->NotifyTxopEnd(m_linkId);
        }
        else
        {
            UpdateTxopEndOnRxEnd();
        }
    }
}

void
EhtFrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                     RxSignalInfo rxSignalInfo,
                                     const WifiTxVector& txVector,
                                     bool inAmpdu)
{
    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    const auto& hdr = mpdu->GetHeader();

    if (hdr.IsTrigger())
    {
        if (!m_staMac)
        {
            return; // Trigger Frames are only processed by STAs
        }

        CtrlTriggerHeader trigger;
        mpdu->GetPacket()->PeekHeader(trigger);

        if (hdr.GetAddr1() != m_self &&
            (!hdr.GetAddr1().IsBroadcast() || !m_staMac->IsAssociated() ||
             hdr.GetAddr2() != m_bssid // not sent by the AP this STA is associated with
             || trigger.FindUserInfoWithAid(m_staMac->GetAssociationId()) == trigger.end()))
        {
            return; // not addressed to us
        }

        if (trigger.IsMuRts() && m_staMac->IsEmlsrLink(m_linkId))
        {
            // this is an initial Control frame
            auto apAddress = GetWifiRemoteStationManager()->GetMldAddress(m_bssid);
            NS_ASSERT_MSG(apAddress, "MLD address not found for BSSID " << m_bssid);
            // when EMLSR links are blocked, all TIDs are blocked (we test TID 0 here)
            WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *apAddress, 0);
            if (auto mask =
                    m_staMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, m_linkId);
                mask && mask->test(static_cast<std::size_t>(
                            WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK)))
            {
                // we received an ICF on a link that is blocked because another EMLSR link is
                // being used. This is likely because transmission on the other EMLSR link
                // started before the reception of the ICF ended. We drop this ICF and let the
                // UL TXOP continue.
                NS_LOG_DEBUG("Drop ICF because another EMLSR link is being used");
                return;
            }

            NS_ASSERT(m_staMac->GetEmlsrManager());
            Simulator::ScheduleNow(&EmlsrManager::NotifyIcfReceived,
                                   m_staMac->GetEmlsrManager(),
                                   m_linkId);
            // we just got involved in a DL TXOP. Check if we are still involved in the TXOP in a
            // SIFS (we are expected to reply by sending a CTS frame)
            m_ongoingTxopEnd.Cancel();
            NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + m_phy->GetSifs()).As(Time::S));
            m_ongoingTxopEnd = Simulator::Schedule(m_phy->GetSifs() + NanoSeconds(1),
                                                   &EhtFrameExchangeManager::TxopEnd,
                                                   this);
        }
    }

    HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

void
EhtFrameExchangeManager::TxopEnd()
{
    NS_LOG_FUNCTION(this);

    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        m_staMac->GetEmlsrManager()->NotifyTxopEnd(m_linkId);
    }
    else if (m_apMac && m_txopHolder &&
             GetWifiRemoteStationManager()->GetEmlsrEnabled(*m_txopHolder))
    {
        // EMLSR client terminated its TXOP and is back to listening operation
        EmlsrSwitchToListening(*m_txopHolder, Seconds(0));
    }
}

void
EhtFrameExchangeManager::UpdateTxopEndOnTxStart(Time txDuration)
{
    NS_LOG_FUNCTION(this << txDuration.As(Time::MS));

    if (!m_ongoingTxopEnd.IsRunning())
    {
        // nothing to do
        return;
    }

    m_ongoingTxopEnd.Cancel();
    Time delay;

    if (m_txTimer.IsRunning())
    {
        // the TX timer is running, hence we are expecting a response. Postpone the TXOP end
        // to match the TX timer (which is long enough to get the PHY-RXSTART.indication for
        // the response)
        delay = m_txTimer.GetDelayLeft();
    }
    else
    {
        // the TX Timer is not running, hence no response is expected (e.g., we are
        // transmitting a CTS after ICS). The TXOP holder may transmit a frame a SIFS
        // after the end of this PPDU, hence we need to postpone the TXOP end in order to
        // get the PHY-RXSTART.indication
        delay = txDuration + m_phy->GetSifs() + m_phy->GetSlot() +
                MicroSeconds(RX_PHY_START_DELAY_USEC);
    }

    NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + delay).As(Time::S));
    m_ongoingTxopEnd = Simulator::Schedule(delay, &EhtFrameExchangeManager::TxopEnd, this);
}

void
EhtFrameExchangeManager::UpdateTxopEndOnRxStartIndication(Time psduDuration)
{
    NS_LOG_FUNCTION(this << psduDuration.As(Time::MS));

    if (!m_ongoingTxopEnd.IsRunning() || !psduDuration.IsStrictlyPositive())
    {
        // nothing to do
        return;
    }

    // postpone the TXOP end until after the reception of the PSDU is completed
    m_ongoingTxopEnd.Cancel();

    NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + psduDuration).As(Time::S));
    m_ongoingTxopEnd =
        Simulator::Schedule(psduDuration + NanoSeconds(1), &EhtFrameExchangeManager::TxopEnd, this);
}

void
EhtFrameExchangeManager::UpdateTxopEndOnRxEnd()
{
    NS_LOG_FUNCTION(this);

    if (!m_ongoingTxopEnd.IsRunning())
    {
        // nothing to do
        return;
    }

    m_ongoingTxopEnd.Cancel();

    // we may send a response after a SIFS or we may receive another frame after a SIFS.
    // Postpone the TXOP end by considering the latter (which takes longer)
    auto delay = m_phy->GetSifs() + m_phy->GetSlot() + MicroSeconds(RX_PHY_START_DELAY_USEC);
    NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + delay).As(Time::S));
    m_ongoingTxopEnd = Simulator::Schedule(delay, &EhtFrameExchangeManager::TxopEnd, this);
}

} // namespace ns3
