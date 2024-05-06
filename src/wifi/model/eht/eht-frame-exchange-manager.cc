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
#include "ns3/mgt-action-headers.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

namespace ns3
{

/// aRxPHYStartDelay value to use when waiting for a new frame in the context of EMLSR operations
/// (Sec. 35.3.17 of 802.11be D3.1)
static constexpr uint8_t RX_PHY_START_DELAY_USEC = 20;

/**
 * Additional time (exceeding 20 us) to wait for a PHY-RXSTART.indication when the PHY is
 * decoding a PHY header.
 *
 * Values for aRxPHYStartDelay:
 * - OFDM : 20 us (for 20 MHz) [Table 17-21 of 802.11-2020]
 * - ERP-OFDM : 20 us [Table 18-5 of 802.11-2020]
 * - HT : 28 us (HT-mixed), 24 us (HT-greenfield) [Table 19-25 of 802.11-2020]
 * - VHT : 36 + 4 * max N_VHT-LTF + 4 = 72 us [Table 21-28 of 802.11-2020]
 * - HE : 32 us (for HE SU and HE TB PPDUs)
 *        32 + 4 * N_HE-SIG-B us (for HE MU PPDUs) [Table 27-54 of 802.11ax-2021]
 * - EHT : 32 us (for EHT TB PPDUs)
 *         32 + 4 * N_EHT-SIG us (for EHT MU PPDUs) [Table 36-70 of 802.11be D3.2]
 */
static constexpr uint8_t WAIT_FOR_RXSTART_DELAY_USEC = 52;

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
EhtFrameExchangeManager::UsingOtherEmlsrLink() const
{
    if (!m_staMac || !m_staMac->IsEmlsrLink(m_linkId))
    {
        return false;
    }
    auto apAddress = GetWifiRemoteStationManager()->GetMldAddress(m_bssid);
    NS_ASSERT_MSG(apAddress, "MLD address not found for BSSID " << m_bssid);
    // when EMLSR links are blocked, all TIDs are blocked (we test TID 0 here)
    WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *apAddress, 0);
    auto mask = m_staMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, m_linkId);
    NS_ASSERT_MSG(mask, "No mask for AP " << *apAddress << " on link " << m_linkId);
    return mask->test(static_cast<std::size_t>(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK));
}

bool
EhtFrameExchangeManager::StartTransmission(Ptr<Txop> edca, uint16_t allowedWidth)
{
    NS_LOG_FUNCTION(this << edca << allowedWidth);

    if (m_apMac)
    {
        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            if (linkId == m_linkId)
            {
                continue;
            }

            // EMLSR clients involved in a DL or UL TXOP on another link
            std::set<Mac48Address> emlsrClients;

            // check if an EMLSR client is the holder of an UL TXOP on the other link
            if (auto ehtFem =
                    StaticCast<EhtFrameExchangeManager>(m_mac->GetFrameExchangeManager(linkId));
                ehtFem->m_ongoingTxopEnd.IsPending() && ehtFem->m_txopHolder &&
                m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(
                    ehtFem->m_txopHolder.value()))
            {
                emlsrClients.insert(ehtFem->m_txopHolder.value());
            }

            // check if EMLSR clients are involved in a DL TXOP on another link
            for (const auto& address : m_protectedStas)
            {
                if (m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(address))
                {
                    emlsrClients.insert(address);
                }
            }

            for (const auto& address : emlsrClients)
            {
                auto mldAddress =
                    m_mac->GetWifiRemoteStationManager(linkId)->GetMldAddress(address);
                NS_ASSERT_MSG(mldAddress, "MLD address not found for " << address);

                if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(*mldAddress))
                {
                    // EMLSR client did not enable EMLSR mode on this link, we can transmit to it
                    continue;
                }

                // check that this link is blocked as expected
                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *mldAddress, 0);
                auto mask =
                    m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, m_linkId);
                NS_ASSERT_MSG(mask,
                              "No mask for client " << *mldAddress << " on link " << +m_linkId);
                if (!mask->test(
                        static_cast<std::size_t>(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK)))
                {
                    NS_ASSERT_MSG(false,
                                  "Transmissions to " << *mldAddress << " on link " << +m_linkId
                                                      << " are not blocked");
                    // in case asserts are disabled, block transmissions on the other links because
                    // this is what we need
                    m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                                 *mldAddress,
                                                 {m_linkId});
                }
            }
        }
    }

    std::optional<Time> timeToCtsEnd;

    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        // Cannot start a transmission on a link blocked because another EMLSR link is being used
        if (UsingOtherEmlsrLink())
        {
            NS_LOG_DEBUG("StartTransmission called while another EMLSR link is being used");
            NotifyChannelReleased(edca);
            return false;
        }

        auto emlsrManager = m_staMac->GetEmlsrManager();

        if (auto elapsed = emlsrManager->GetElapsedMediumSyncDelayTimer(m_linkId);
            elapsed && emlsrManager->MediumSyncDelayNTxopsExceeded(m_linkId))
        {
            NS_LOG_DEBUG("No new TXOP attempts allowed while MediumSyncDelay is running");
            // request channel access if needed when the MediumSyncDelay timer expires; in the
            // meantime no queued packet can be transmitted
            Simulator::Schedule(
                emlsrManager->GetMediumSyncDuration() - *elapsed,
                &Txop::StartAccessAfterEvent,
                edca,
                m_linkId,
                Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT, // queued frames cannot be transmitted until
                                                     // MSD expires
                Txop::DONT_CHECK_MEDIUM_BUSY);       // generate backoff regardless of medium busy
            NotifyChannelReleased(edca);
            return false;
        }

        if (!m_phy)
        {
            NS_LOG_DEBUG("No PHY is currently operating on EMLSR link " << +m_linkId);
            NotifyChannelReleased(edca);
            return false;
        }

        if (auto mainPhy = m_staMac->GetDevice()->GetPhy(emlsrManager->GetMainPhyId());
            mainPhy != m_phy)
        {
            // an aux PHY is operating on this link
            if (!emlsrManager->GetAuxPhyTxCapable())
            {
                NS_LOG_DEBUG("Aux PHY is not capable of transmitting a PPDU");
                NotifyChannelReleased(edca);
                return false;
            }

            if (mainPhy->IsStateRx())
            {
                NS_LOG_DEBUG(
                    "Main PHY is receiving a PPDU (may be, e.g., an ICF or a Beacon); do not "
                    "transmit to avoid dropping that PPDU due to the main PHY switching to this "
                    "link to take over the TXOP");
                // Note that we do not prevent a (main or aux) PHY from starting a TXOP when
                // an(other) aux PHY is receiving a PPDU. The reason is that if the aux PHY is
                // receiving a Beacon frame, the aux PHY will not be affected by the start of
                // a TXOP; if the aux PHY is receiving an ICF, the ICF will be dropped by
                // ReceiveMpdu because another EMLSR link is being used.
                NotifyChannelReleased(edca);
                return false;
            }

            const auto rtsTxVector =
                GetWifiRemoteStationManager()->GetRtsTxVector(m_bssid, allowedWidth);
            const auto rtsTxTime =
                m_phy->CalculateTxDuration(GetRtsSize(), rtsTxVector, m_phy->GetPhyBand());
            const auto ctsTxVector =
                GetWifiRemoteStationManager()->GetCtsTxVector(m_bssid, rtsTxVector.GetMode());
            const auto ctsTxTime =
                m_phy->CalculateTxDuration(GetCtsSize(), ctsTxVector, m_phy->GetPhyBand());

            // the main PHY shall terminate the channel switch at the end of CTS reception;
            // the time remaining to the end of CTS reception includes two propagation delays
            timeToCtsEnd = rtsTxTime + m_phy->GetSifs() + ctsTxTime +
                           MicroSeconds(2 * MAX_PROPAGATION_DELAY_USEC);

            auto switchingTime = mainPhy->GetChannelSwitchDelay();

            if (mainPhy->IsStateSwitching())
            {
                // the main PHY is switching (to another link), hence the remaining time to the
                // end of the current channel switch needs to be added up
                switchingTime += mainPhy->GetDelayUntilIdle();
            }

            if (switchingTime > timeToCtsEnd)
            {
                // switching takes longer than RTS/CTS exchange, do not transmit anything to
                // avoid that the main PHY is requested to switch while already switching
                NS_LOG_DEBUG("Main PHY will still be switching channel when RTS/CTS ends, thus it "
                             "will not be able to take over this TXOP");
                NotifyChannelReleased(edca);
                return false;
            }
        }
    }

    auto started = HeFrameExchangeManager::StartTransmission(edca, allowedWidth);

    if (started && m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        // notify the EMLSR Manager of the UL TXOP start on an EMLSR link
        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyUlTxopStart(m_linkId, timeToCtsEnd);
    }

    if (started)
    {
        // we are starting a new TXOP, hence consider the previous ongoing TXOP as terminated
        m_ongoingTxopEnd.Cancel();
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
    UpdateTxopEndOnTxStart(txDuration, psdu->GetDuration());

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
    UpdateTxopEndOnTxStart(txDuration, psduMap.begin()->second->GetDuration());

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
EhtFrameExchangeManager::NavResetTimeout()
{
    NS_LOG_FUNCTION(this);
    if (UsingOtherEmlsrLink())
    {
        // the CTS may have been missed because another EMLSR link is being used; do not reset NAV
        return;
    }
    HeFrameExchangeManager::NavResetTimeout();
}

void
EhtFrameExchangeManager::IntraBssNavResetTimeout()
{
    NS_LOG_FUNCTION(this);
    if (UsingOtherEmlsrLink())
    {
        // the CTS may have been missed because another EMLSR link is being used; do not reset NAV
        return;
    }
    HeFrameExchangeManager::IntraBssNavResetTimeout();
}

void
EhtFrameExchangeManager::EmlsrSwitchToListening(const Mac48Address& address, const Time& delay)
{
    NS_LOG_FUNCTION(this << address << delay.As(Time::US));

    auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);
    NS_ASSERT_MSG(mldAddress, "MLD address not found for " << address);
    NS_ASSERT_MSG(m_apMac, "This function shall only be called by AP MLDs");

    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
    {
        if (auto ehtFem =
                StaticCast<EhtFrameExchangeManager>(m_mac->GetFrameExchangeManager(linkId));
            ehtFem->m_ongoingTxopEnd.IsPending() && ehtFem->m_txopHolder &&
            m_mac->GetWifiRemoteStationManager(linkId)->GetMldAddress(*ehtFem->m_txopHolder) ==
                mldAddress)
        {
            // this EMLSR client is the holder of an UL TXOP, do not unblock links
            return;
        }
    }

    // this EMLSR client switches back to listening operation a transition delay
    // after the given delay
    auto emlCapabilities = GetWifiRemoteStationManager()->GetStationEmlCapabilities(address);
    NS_ASSERT(emlCapabilities);

    std::set<uint8_t> linkIds;
    for (uint8_t linkId = 0; linkId < m_mac->GetNLinks(); linkId++)
    {
        if (m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
        {
            linkIds.insert(linkId);
        }
    }

    auto blockLinks = [=, this]() {
        // the reason for blocking the other EMLSR links has changed now
        m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                       *mldAddress,
                                       linkIds);

        // block DL transmissions on this link until transition delay elapses
        m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                     *mldAddress,
                                     linkIds);
    };

    delay.IsZero() ? blockLinks() : static_cast<void>(Simulator::Schedule(delay, blockLinks));

    // unblock all EMLSR links when the transition delay elapses
    auto unblockLinks = [=, this]() {
        m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                       *mldAddress,
                                       linkIds);
    };

    auto endDelay = delay + CommonInfoBasicMle::DecodeEmlsrTransitionDelay(
                                emlCapabilities->get().emlsrTransitionDelay);

    endDelay.IsZero() ? unblockLinks()
                      : static_cast<void>(m_transDelayTimer[*mldAddress] =
                                              Simulator::Schedule(endDelay, unblockLinks));
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
    // have to reset the connected PHY. Similarly, we do not have to reset the connected PHY if
    // the link does not change (this occurs when changing the channel width of aux PHYs upon
    // enabling the EMLSR mode).
    if (phy == m_phy && linkId != m_linkId)
    {
        ResetPhy();
    }
    m_staMac->NotifySwitchingEmlsrLink(phy, linkId, delay);
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

void
EhtFrameExchangeManager::SendCtsAfterMuRts(const WifiMacHeader& muRtsHdr,
                                           const CtrlTriggerHeader& trigger,
                                           double muRtsSnr)
{
    NS_LOG_FUNCTION(this << muRtsHdr << trigger << muRtsSnr);

    NS_ASSERT(m_staMac);
    if (auto emlsrManager = m_staMac->GetEmlsrManager())
    {
        auto mainPhy = m_staMac->GetDevice()->GetPhy(emlsrManager->GetMainPhyId());

        // an aux PHY that is not TX capable may get a TXOP, release the channel and request
        // the main PHY to switch channel. Shortly afterwards, the AP MLD may send an ICF, thus
        // when the main PHY is scheduled to send the CTS, the main PHY may be switching channel
        // or may be operating on another link
        if (mainPhy->IsStateSwitching() || m_mac->GetLinkForPhy(mainPhy) != m_linkId)
        {
            NS_LOG_DEBUG("Main PHY is switching or operating on another link, abort sending CTS");
            return;
        }
    }
    HeFrameExchangeManager::SendCtsAfterMuRts(muRtsHdr, trigger, muRtsSnr);
}

void
EhtFrameExchangeManager::CtsAfterMuRtsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *muRts << txVector);

    // we blocked transmissions on the other EMLSR links for the EMLSR clients we sent the ICF to.
    // Given that no client responded, we can unblock transmissions for a client if there is no
    // ongoing UL TXOP held by that client
    for (const auto& address : m_sentRtsTo)
    {
        if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
        {
            continue;
        }

        auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);
        NS_ASSERT(mldAddress);

        if (m_ongoingTxopEnd.IsPending() && m_txopHolder &&
            m_mac->GetMldAddress(*m_txopHolder) == mldAddress)
        {
            continue;
        }

        std::set<uint8_t> linkIds;
        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            if (linkId != m_linkId &&
                m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
            {
                linkIds.insert(linkId);
            }
        }
        m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                       *mldAddress,
                                       linkIds);
    }

    HeFrameExchangeManager::CtsAfterMuRtsTimeout(muRts, txVector);
}

void
EhtFrameExchangeManager::SendCtsAfterRts(const WifiMacHeader& rtsHdr,
                                         WifiMode rtsTxMode,
                                         double rtsSnr)
{
    NS_LOG_FUNCTION(this << rtsHdr << rtsTxMode << rtsSnr);

    if (m_apMac && GetWifiRemoteStationManager()->GetEmlsrEnabled(rtsHdr.GetAddr2()))
    {
        // we are going to send a CTS to an EMLSR client, transmissions to such EMLSR client
        // must be blocked on the other EMLSR links

        auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(rtsHdr.GetAddr2());
        NS_ASSERT_MSG(mldAddress, "MLD address not found for " << rtsHdr.GetAddr2());

        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
        {
            if (linkId != m_linkId &&
                m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
            {
                // check that other links are blocked as expected
                WifiContainerQueueId queueId(WIFI_QOSDATA_QUEUE, WIFI_UNICAST, *mldAddress, 0);
                auto mask =
                    m_apMac->GetMacQueueScheduler()->GetQueueLinkMask(AC_BE, queueId, linkId);
                NS_ASSERT_MSG(mask, "No mask for client " << *mldAddress << " on link " << +linkId);
                if (!mask->test(
                        static_cast<std::size_t>(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK)))
                {
                    NS_ASSERT_MSG(false,
                                  "Transmissions to " << *mldAddress << " on link " << +linkId
                                                      << " are not blocked");
                    // in case asserts are disabled, block transmissions on the other links because
                    // this is what we need
                    m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                                 *mldAddress,
                                                 {linkId});
                }
            }
        }
    }

    HeFrameExchangeManager::SendCtsAfterRts(rtsHdr, rtsTxMode, rtsSnr);
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
EhtFrameExchangeManager::TransmissionSucceeded()
{
    NS_LOG_FUNCTION(this);

    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) &&
        m_staMac->GetEmlsrManager()->GetElapsedMediumSyncDelayTimer(m_linkId))
    {
        NS_LOG_DEBUG("Reset the counter of TXOP attempts allowed while "
                     "MediumSyncDelay is running");
        m_staMac->GetEmlsrManager()->ResetMediumSyncDelayNTxops(m_linkId);
    }

    HeFrameExchangeManager::TransmissionSucceeded();
}

void
EhtFrameExchangeManager::TransmissionFailed()
{
    NS_LOG_FUNCTION(this);

    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) &&
        m_staMac->GetEmlsrManager()->GetElapsedMediumSyncDelayTimer(m_linkId))
    {
        NS_LOG_DEBUG("Decrement the remaining number of TXOP attempts allowed while "
                     "MediumSyncDelay is running");
        m_staMac->GetEmlsrManager()->DecrementMediumSyncDelayNTxops(m_linkId);
    }

    HeFrameExchangeManager::TransmissionFailed();
}

void
EhtFrameExchangeManager::NotifyChannelReleased(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);

    if (m_apMac)
    {
        // the channel has been released; all EMLSR clients are switching back to
        // listening operation
        for (const auto& address : m_protectedStas)
        {
            if (GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
            {
                EmlsrSwitchToListening(address, Seconds(0));
            }
        }
    }
    else if (m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        // Notify the UL TXOP end to the EMLSR Manager
        auto edca = DynamicCast<QosTxop>(txop);
        NS_ASSERT(edca);
        auto txopStart = edca->GetTxopStartTime(m_linkId);

        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyTxopEnd(m_linkId,
                                                   (!txopStart || *txopStart == Simulator::Now()),
                                                   m_ongoingTxopEnd.IsPending());
    }

    HeFrameExchangeManager::NotifyChannelReleased(txop);
}

void
EhtFrameExchangeManager::PreProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    // In addition, the timer resets to zero when any of the following events occur:
    // — The STA receives an MPDU
    // (Sec. 35.3.16.8.1 of 802.11be D3.1)
    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) &&
        m_staMac->GetEmlsrManager()->GetElapsedMediumSyncDelayTimer(m_linkId))
    {
        m_staMac->GetEmlsrManager()->CancelMediumSyncDelayTimer(m_linkId);
    }

    if (m_apMac)
    {
        // we iterate over protected STAs to consider only the case when the AP is the TXOP holder.
        // The AP received a PSDU from a non-AP STA; given that the AP is the TXOP holder, this
        // PSDU has been likely solicited by the AP. In most of the cases, we identify which EMLSR
        // clients are no longer involved in the TXOP when the AP transmits the frame soliciting
        // response(s) from client(s). This is not the case, for example, for the acknowledgment
        // in SU format of a DL MU PPDU, where all the EMLSR clients (but one) switch to listening
        // operation after the immediate response (if any) by one of the EMLSR clients.
        for (auto clientIt = m_protectedStas.begin(); clientIt != m_protectedStas.end();)
        {
            // TB PPDUs are received by the AP at distinct times, so it is difficult to take a
            // decision based on one of them. However, clients transmitting TB PPDUs are identified
            // by the soliciting Trigger Frame, thus we have already identified (when sending the
            // Trigger Frame) which EMLSR clients have switched to listening operation.
            // If the PSDU is not carried in a TB PPDU, we can determine whether this EMLSR client
            // is switching to listening operation by checking whether the AP is expecting a
            // response from it.
            if (GetWifiRemoteStationManager()->GetEmlsrEnabled(*clientIt) && !txVector.IsUlMu() &&
                !m_txTimer.GetStasExpectedToRespond().contains(*clientIt))
            {
                EmlsrSwitchToListening(*clientIt, Seconds(0));
                // this client is no longer involved in the current TXOP
                clientIt = m_protectedStas.erase(clientIt);
            }
            else
            {
                clientIt++;
            }
        }
    }

    HeFrameExchangeManager::PreProcessFrame(psdu, txVector);
}

void
EhtFrameExchangeManager::PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    HeFrameExchangeManager::PostProcessFrame(psdu, txVector);

    if (m_apMac && m_txopHolder == psdu->GetAddr2() &&
        GetWifiRemoteStationManager()->GetEmlsrEnabled(*m_txopHolder))
    {
        if (!m_ongoingTxopEnd.IsPending())
        {
            // an EMLSR client has started an UL TXOP. Start the ongoingTxopEnd timer so that
            // the next call to UpdateTxopEndOnRxEnd does its job
            m_ongoingTxopEnd =
                Simulator::ScheduleNow(&EhtFrameExchangeManager::TxopEnd, this, m_txopHolder);
        }

        UpdateTxopEndOnRxEnd(psdu->GetDuration());
    }

    if (m_staMac && m_ongoingTxopEnd.IsPending())
    {
        if (GetEmlsrSwitchToListening(psdu, m_staMac->GetAssociationId(), m_self))
        {
            // we are no longer involved in the TXOP and switching to listening mode
            m_ongoingTxopEnd.Cancel();
            m_staMac->GetEmlsrManager()->NotifyTxopEnd(m_linkId);
        }
        else
        {
            UpdateTxopEndOnRxEnd(psdu->GetDuration());
        }
    }
}

bool
EhtFrameExchangeManager::CheckEmlsrClientStartingTxop(const WifiMacHeader& hdr,
                                                      const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this);

    auto sender = hdr.GetAddr2();

    if (m_ongoingTxopEnd.IsPending())
    {
        NS_LOG_DEBUG("A TXOP is already ongoing");
        return false;
    }

    if (auto holder = FindTxopHolder(hdr, txVector); holder != sender)
    {
        NS_LOG_DEBUG("Sender (" << sender << ") differs from the TXOP holder ("
                                << (holder ? Address(*holder) : Address()) << ")");
        return false;
    }

    if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(sender))
    {
        NS_LOG_DEBUG("Sender (" << sender << ") is not an EMLSR client");
        return false;
    }

    NS_LOG_DEBUG("EMLSR client " << sender << " is starting a TXOP");

    // Block transmissions for this EMLSR client on other links
    auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(sender);
    NS_ASSERT(mldAddress);

    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
    {
        if (linkId != m_linkId &&
            m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
        {
            m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                         *mldAddress,
                                         {linkId});
        }
    }

    // Make sure that transmissions for this EMLSR client are not blocked on this link
    // (the AP MLD may have sent an ICF on another link right before receiving this MPDU,
    // thus transmissions on this link may have been blocked)
    m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                   *mldAddress,
                                   {m_linkId});

    // Stop the transition delay timer for this EMLSR client, if any is running
    if (auto it = m_transDelayTimer.find(*mldAddress);
        it != m_transDelayTimer.end() && it->second.IsPending())
    {
        it->second.PeekEventImpl()->Invoke();
        it->second.Cancel();
    }

    return true;
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

    if (m_apMac)
    {
        // if the AP MLD received an MPDU from an EMLSR client that is starting an UL TXOP,
        // block transmissions to the EMLSR client on other links
        CheckEmlsrClientStartingTxop(hdr, txVector);
    }

    bool icfReceived = false;

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
            if (UsingOtherEmlsrLink())
            {
                // we received an ICF on a link that is blocked because another EMLSR link is
                // being used. This is likely because transmission on the other EMLSR link
                // started before the reception of the ICF ended. We drop this ICF and let the
                // UL TXOP continue.
                NS_LOG_DEBUG("Drop ICF because another EMLSR link is being used");
                return;
            }

            /**
             * It might happen that, while the aux PHY is receiving an ICF, the main PHY is
             * completing a TXOP on another link or is returning to the primary link after a TXOP
             * is completed on another link. In order to respond to the ICF, it is necessary that
             * the main PHY has enough time to switch and be ready to operate on this link by the
             * end of the ICF padding.
             *
             *                        TXOP end
             *                            │
             *                        ┌───┐                               another
             *   AP MLD               │ACK│                               link
             *  ───────────┬─────────┬┴───┴───────────────────────────────────────
             *   EMLSR     │   QoS   │    │                            main PHY
             *   client    │  Data   │    │
             *             └─────────┘    │
             *                      ┌─────┬───┐                           this
             *   AP MLD             │ ICF │pad│                           link
             *  ────────────────────┴─────┴───┴───────────────────────────────────
             *                                                          aux PHY
             */

            auto emlsrManager = m_staMac->GetEmlsrManager();
            NS_ASSERT(emlsrManager);

            if (auto mainPhy = m_staMac->GetDevice()->GetPhy(emlsrManager->GetMainPhyId());
                mainPhy != m_phy)
            {
                const auto delay = mainPhy->GetChannelSwitchDelay();

                if (mainPhy->GetState()->GetLastTime({WifiPhyState::TX,
                                                      // WifiPhyState::RX, comment out for now
                                                      WifiPhyState::SWITCHING,
                                                      WifiPhyState::SLEEP}) >
                    Simulator::Now() - delay)
                {
                    NS_LOG_DEBUG("Drop ICF due to not enough time for the main PHY to switch link");
                    return;
                }
            }

            emlsrManager->NotifyIcfReceived(m_linkId);
            icfReceived = true;

            // we just got involved in a DL TXOP. Check if we are still involved in the TXOP in a
            // SIFS (we are expected to reply by sending a CTS frame)
            m_ongoingTxopEnd.Cancel();
            NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + m_phy->GetSifs()).As(Time::S));
            m_ongoingTxopEnd = Simulator::Schedule(m_phy->GetSifs() + NanoSeconds(1),
                                                   &EhtFrameExchangeManager::TxopEnd,
                                                   this,
                                                   hdr.GetAddr2());
        }
    }

    // We impose that an aux PHY is only able to receive an ICF, a CTS or a management frame
    // (we are interested in receiving mainly Beacon frames). Note that other frames are still
    // post-processed, e.g., used to set the NAV and the TXOP holder.
    // The motivation is that, e.g., an AP MLD may send an ICF to EMLSR clients A and B;
    // A responds while B does not; the AP MLD sends a DL MU PPDU to both clients followed
    // by an MU-BAR to solicit a BlockAck from both clients. If an aux PHY of client B is
    // operating on this link, the MU-BAR will be received and a TB PPDU response sent
    // through the aux PHY.
    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) &&
        m_mac->GetLinkForPhy(m_staMac->GetEmlsrManager()->GetMainPhyId()) != m_linkId &&
        !icfReceived && !mpdu->GetHeader().IsCts() && !mpdu->GetHeader().IsMgt())
    {
        NS_LOG_DEBUG("Dropping " << *mpdu << " received by an aux PHY on link " << +m_linkId);
        return;
    }

    HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

void
EhtFrameExchangeManager::TxopEnd(const std::optional<Mac48Address>& txopHolder)
{
    NS_LOG_FUNCTION(this << txopHolder.has_value());

    if (m_phy->IsReceivingPhyHeader())
    {
        // we may get here because the PHY has not issued the PHY-RXSTART.indication before
        // the expiration of the timer started to detect new received frames, but the PHY is
        // currently decoding the PHY header of a PPDU, so let's wait some more time to check
        // if we receive a PHY-RXSTART.indication when the PHY is done decoding the PHY header
        NS_LOG_DEBUG("PHY is decoding the PHY header of PPDU, postpone TXOP end");
        m_ongoingTxopEnd = Simulator::Schedule(MicroSeconds(WAIT_FOR_RXSTART_DELAY_USEC),
                                               &EhtFrameExchangeManager::TxopEnd,
                                               this,
                                               txopHolder);
        return;
    }

    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        m_staMac->GetEmlsrManager()->NotifyTxopEnd(m_linkId);
    }
    else if (m_apMac && txopHolder && GetWifiRemoteStationManager()->GetEmlsrEnabled(*txopHolder))
    {
        // EMLSR client terminated its TXOP and is back to listening operation
        EmlsrSwitchToListening(*txopHolder, Seconds(0));
    }
}

void
EhtFrameExchangeManager::UpdateTxopEndOnTxStart(Time txDuration, Time durationId)
{
    NS_LOG_FUNCTION(this << txDuration.As(Time::MS) << durationId.As(Time::US));

    if (!m_ongoingTxopEnd.IsPending())
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
    else if (durationId <= m_phy->GetSifs())
    {
        // the TX timer is not running, hence no response is expected, and the Duration/ID value
        // is less than or equal to a SIFS; the TXOP will end after this transmission
        NS_LOG_DEBUG("Assume TXOP will end based on Duration/ID value");
        delay = txDuration;
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
    m_ongoingTxopEnd =
        Simulator::Schedule(delay, &EhtFrameExchangeManager::TxopEnd, this, m_txopHolder);
}

void
EhtFrameExchangeManager::UpdateTxopEndOnRxStartIndication(Time psduDuration)
{
    NS_LOG_FUNCTION(this << psduDuration.As(Time::MS));

    if (!m_ongoingTxopEnd.IsPending() || !psduDuration.IsStrictlyPositive())
    {
        // nothing to do
        return;
    }

    // postpone the TXOP end until after the reception of the PSDU is completed
    m_ongoingTxopEnd.Cancel();

    NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + psduDuration).As(Time::S));
    m_ongoingTxopEnd = Simulator::Schedule(psduDuration + NanoSeconds(1),
                                           &EhtFrameExchangeManager::TxopEnd,
                                           this,
                                           m_txopHolder);
}

void
EhtFrameExchangeManager::UpdateTxopEndOnRxEnd(Time durationId)
{
    NS_LOG_FUNCTION(this << durationId.As(Time::US));

    if (!m_ongoingTxopEnd.IsPending())
    {
        // nothing to do
        return;
    }

    m_ongoingTxopEnd.Cancel();

    // if the Duration/ID of the received frame is less than a SIFS, the TXOP
    // is terminated
    if (durationId <= m_phy->GetSifs())
    {
        NS_LOG_DEBUG("Assume TXOP ended based on Duration/ID value");
        TxopEnd(m_txopHolder);
        return;
    }

    // we may send a response after a SIFS or we may receive another frame after a SIFS.
    // Postpone the TXOP end by considering the latter (which takes longer)
    auto delay = m_phy->GetSifs() + m_phy->GetSlot() + MicroSeconds(RX_PHY_START_DELAY_USEC);
    NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + delay).As(Time::S));
    m_ongoingTxopEnd =
        Simulator::Schedule(delay, &EhtFrameExchangeManager::TxopEnd, this, m_txopHolder);
}

} // namespace ns3
