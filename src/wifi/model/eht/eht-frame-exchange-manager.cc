/*
 * Copyright (c) 2022 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "eht-frame-exchange-manager.h"

#include "ap-emlsr-manager.h"
#include "eht-phy.h"
#include "emlsr-manager.h"

#include "ns3/abort.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/mgt-action-headers.h"
#include "ns3/spectrum-signal-parameters.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-spectrum-phy-interface.h"

#include <algorithm>

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

namespace ns3
{

const Time EMLSR_RX_PHY_START_DELAY = MicroSeconds(20);

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
EhtFrameExchangeManager::StartTransmission(Ptr<Txop> edca, MHz_u allowedWidth)
{
    NS_LOG_FUNCTION(this << edca << allowedWidth);

    m_allowedWidth = allowedWidth;

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
            auto ehtFem =
                StaticCast<EhtFrameExchangeManager>(m_mac->GetFrameExchangeManager(linkId));

            // check if an EMLSR client is the holder of an UL TXOP on the other link
            if (ehtFem->m_ongoingTxopEnd.IsPending() && ehtFem->m_txopHolder &&
                m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(
                    ehtFem->m_txopHolder.value()))
            {
                NS_LOG_DEBUG("Involved in UL TXOP: " << ehtFem->m_txopHolder.value());
                emlsrClients.insert(ehtFem->m_txopHolder.value());
            }

            // check if EMLSR clients are involved in a DL TXOP on another link
            for (const auto& address : ehtFem->m_protectedStas)
            {
                if (m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(address))
                {
                    NS_LOG_DEBUG("Involved in DL TXOP: " << address);
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

        // let EMLSR manager decide whether to prevent or allow this UL TXOP
        if (const auto [startTxop, delay] = emlsrManager->GetDelayUntilAccessRequest(
                m_linkId,
                DynamicCast<QosTxop>(edca)->GetAccessCategory());
            !startTxop)

        {
            if (delay.IsStrictlyPositive())
            {
                NotifyChannelReleased(edca);
                Simulator::Schedule(
                    delay,
                    &Txop::StartAccessAfterEvent,
                    edca,
                    m_linkId,
                    Txop::DIDNT_HAVE_FRAMES_TO_TRANSMIT, // queued frames cannot be
                                                         // transmitted until RX ends
                    Txop::CHECK_MEDIUM_BUSY);            // generate backoff if medium busy
            }
            return false;
        }
    }

    auto started = HeFrameExchangeManager::StartTransmission(edca, allowedWidth);

    if (started && m_staMac && m_staMac->IsEmlsrLink(m_linkId))
    {
        // notify the EMLSR Manager of the UL TXOP start on an EMLSR link
        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyUlTxopStart(m_linkId);
    }

    if (started)
    {
        // we are starting a new TXOP, hence consider the previous ongoing TXOP as terminated
        m_ongoingTxopEnd.Cancel();
    }

    return started;
}

void
EhtFrameExchangeManager::ProtectionCompleted()
{
    NS_LOG_FUNCTION(this);

    if (m_staMac && m_staMac->GetEmlsrManager())
    {
        m_staMac->GetEmlsrManager()->NotifyProtectionCompleted(m_linkId);
    }

    HeFrameExchangeManager::ProtectionCompleted();
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

    if (m_apMac && psdu->GetHeader(0).IsTrigger())
    {
        for (const auto& client : m_sentRtsTo)
        {
            if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(client))
            {
                continue;
            }
            auto clientMld = GetWifiRemoteStationManager()->GetMldAddress(client);
            NS_ASSERT(clientMld);

            // block transmissions on the other EMLSR links of the EMLSR clients
            for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
            {
                if (linkId != m_linkId &&
                    m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*clientMld))
                {
                    m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                                 *clientMld,
                                                 {linkId});
                }
            }
        }
    }

    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) && psdu->GetAddr1() == m_bssid &&
        psdu->GetHeader(0).IsRts())
    {
        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyRtsSent(m_linkId, psdu, txVector);
    }

    HeFrameExchangeManager::ForwardPsduDown(psdu, txVector);
    UpdateTxopEndOnTxStart(txDuration, psdu->GetDuration());

    if (m_apMac && m_apMac->GetApEmlsrManager())
    {
        auto delay = m_apMac->GetApEmlsrManager()->GetDelayOnTxPsduNotForEmlsr(psdu,
                                                                               txVector,
                                                                               m_phy->GetPhyBand());

        // check if the EMLSR clients shall switch back to listening operation
        for (auto clientIt = m_protectedStas.begin(); clientIt != m_protectedStas.end();)
        {
            auto aid = GetWifiRemoteStationManager()->GetAssociationId(*clientIt);

            if (GetWifiRemoteStationManager()->GetEmlsrEnabled(*clientIt) &&
                GetEmlsrSwitchToListening(psdu, aid, *clientIt))
            {
                EmlsrSwitchToListening(*clientIt, delay);
                // this client is no longer involved in the current TXOP
                clientIt = m_protectedStas.erase(clientIt);
            }
            else
            {
                clientIt++;
            }
        }
    }
    else if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) &&
             m_staMac->GetEmlsrManager()->GetInDeviceInterference())
    {
        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyInDeviceInterferenceStart(m_linkId, txDuration);

        for (const auto linkId : m_staMac->GetLinkIds())
        {
            if (auto phy = m_mac->GetWifiPhy(linkId);
                phy && linkId != m_linkId && m_staMac->IsEmlsrLink(linkId))
            {
                const auto txPower = phy->GetPower(txVector.GetTxPowerLevel()) + phy->GetTxGain();
                // generate in-device interference on the other EMLSR link for the duration of this
                // transmission
                GenerateInDeviceInterference(linkId, txDuration, DbmToW(txPower));
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
        // check if this is a BSRP TF used as ICF for some EMLSR client
        if (IsTrigger(psduMap))
        {
            CtrlTriggerHeader trigger;
            psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);

            if (trigger.IsBsrp())
            {
                auto recipients = GetTfRecipients(trigger);
                for (const auto& client : recipients)
                {
                    if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(client))
                    {
                        continue;
                    }
                    auto clientMld = GetWifiRemoteStationManager()->GetMldAddress(client);
                    NS_ASSERT(clientMld);

                    // block transmissions on the other EMLSR links of the EMLSR clients
                    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
                    {
                        if (linkId != m_linkId &&
                            m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*clientMld))
                        {
                            m_mac->BlockUnicastTxOnLinks(
                                WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                *clientMld,
                                {linkId});
                        }
                    }
                }
            }
        }

        // check if the EMLSR clients shall switch back to listening operation at the end of this
        // PPDU
        for (auto clientIt = m_protectedStas.begin(); clientIt != m_protectedStas.end();)
        {
            auto aid = GetWifiRemoteStationManager()->GetAssociationId(*clientIt);
            const auto psduMapIt = psduMap.find(aid);
            const auto aidNotFoundAndNotTf = (psduMapIt == psduMap.cend()) && !IsTrigger(psduMap);
            // the PSDU to process: the one addressed to the given AID (if any) or the unique one
            const auto psdu = (psduMapIt != psduMap.cend() ? psduMapIt : psduMap.cbegin())->second;

            if (GetWifiRemoteStationManager()->GetEmlsrEnabled(*clientIt) &&
                (aidNotFoundAndNotTf || GetEmlsrSwitchToListening(psdu, aid, *clientIt)))
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
    else if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) &&
             m_staMac->GetEmlsrManager()->GetInDeviceInterference())
    {
        NS_ASSERT(m_staMac->GetEmlsrManager());
        m_staMac->GetEmlsrManager()->NotifyInDeviceInterferenceStart(m_linkId, txDuration);

        for (const auto linkId : m_staMac->GetLinkIds())
        {
            if (auto phy = m_mac->GetWifiPhy(linkId);
                phy && linkId != m_linkId && m_staMac->IsEmlsrLink(linkId))
            {
                const auto txPower = phy->GetPower(txVector.GetTxPowerLevel()) + phy->GetTxGain();
                // generate in-device interference on the other EMLSR link for the duration of this
                // transmission
                GenerateInDeviceInterference(linkId, txDuration, DbmToW(txPower));
            }
        }
    }
}

void
EhtFrameExchangeManager::GenerateInDeviceInterference(uint8_t linkId, Time duration, Watt_u txPower)
{
    NS_LOG_FUNCTION(this << linkId << duration.As(Time::US) << txPower);

    auto rxPhy = DynamicCast<SpectrumWifiPhy>(m_mac->GetWifiPhy(linkId));

    if (!rxPhy)
    {
        NS_LOG_DEBUG("No spectrum PHY operating on link " << +linkId);
        return;
    }

    auto txPhy = DynamicCast<SpectrumWifiPhy>(m_phy);
    NS_ASSERT(txPhy);

    auto psd = Create<SpectrumValue>(rxPhy->GetCurrentInterface()->GetRxSpectrumModel());
    *psd = txPower;

    auto spectrumSignalParams = Create<SpectrumSignalParameters>();
    spectrumSignalParams->duration = duration;
    spectrumSignalParams->txPhy = txPhy->GetCurrentInterface();
    spectrumSignalParams->txAntenna = txPhy->GetAntenna();
    spectrumSignalParams->psd = psd;

    rxPhy->StartRx(spectrumSignalParams, rxPhy->GetCurrentInterface());
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

bool
EhtFrameExchangeManager::UnblockEmlsrLinksIfAllowed(Mac48Address address)
{
    NS_LOG_FUNCTION(this << address);

    auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);
    NS_ASSERT_MSG(mldAddress, "MLD address not found for " << address);
    NS_ASSERT_MSG(m_apMac, "This function shall only be called by AP MLDs");
    std::set<uint8_t> linkIds{m_linkId};

    /**
     * Do nothing if the EMLSR client is involved in a DL or UL TXOP on another EMLSR link. This
     * may happen, e.g., when the AP MLD sent an MU-RTS to multiple stations on this link, some of
     * which responded, but this EMLSR client did not, e.g., because it concurrently started an UL
     * TXOP on another link. The AP MLD then started a (long) DL MU transmission on this link,
     * during which the EMLSR client completed the UL TXOP and started being involved in another
     * DL or UL TXOP (note that DL TXOP is possible because the AP MLD considered the EMLSR client
     * unprotected as soon as it detected the start of the previous UL TXOP). A Block Ack timeout
     * for the EMLSR client occurs at the end of the DL MU transmission (which brings us here) and
     * it may occur while the EMLSR client is still involved in a DL or UL TXOP.
     *
     *          ┌─────────────┐          ┌───────────────┐
     *          │  MU-RTS to  │          │    Data to    │      BA timeout for us
     *          │us and others│          │ us and others │        |
     *  ────────┴─────────────┴┬────────┬┴───────────────┴┬───────┬──────────────
     *  [this link]            │CTS from│                 │BA from│
     *                         │ others │                 │ others│
     *                         └────────┘                 └───────┘
     *                                          ┌───────┐
     *                     ┌───┐      ┌──┐      │ MU-RTS│     ┌──────┐
     *  [other link]       │CTS│      │BA│      │ to us │     │ Data │
     *  ─────────┬────────┬┴───┴┬────┬┴──┴──────┴───────┴┬───┬┴──────┴┬──┬───────
     *           │   RTS  │     │Data│                   │CTS│        │BA│
     *           │from us │     └────┘                   └───┘        └──┘
     *           └────────┘
     */

    for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); ++linkId)
    {
        if (!m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
        {
            continue; // not an EMLSR link
        }

        auto ehtFem = StaticCast<EhtFrameExchangeManager>(m_mac->GetFrameExchangeManager(linkId));

        if (ehtFem->m_ongoingTxopEnd.IsPending() && ehtFem->m_txopHolder &&
            m_mac->GetWifiRemoteStationManager(linkId)->GetMldAddress(*ehtFem->m_txopHolder) ==
                mldAddress)
        {
            NS_LOG_DEBUG("EMLSR client " << *mldAddress << " is the holder of an UL TXOP on link "
                                         << +linkId << ", do not unblock links");
            return false;
        }

        if (linkId == m_linkId)
        {
            // no need to check if the EMLSR client is involved in a DL TXOP on this link
            continue;
        }

        linkIds.insert(linkId);

        if (auto linkAddr =
                m_apMac->GetWifiRemoteStationManager(linkId)->GetAffiliatedStaAddress(*mldAddress);
            linkAddr &&
            (ehtFem->m_sentRtsTo.contains(*linkAddr) || ehtFem->m_sentFrameTo.contains(*linkAddr) ||
             ehtFem->m_protectedStas.contains(*linkAddr)))
        {
            NS_LOG_DEBUG("EMLSR client " << address
                                         << " has been sent an ICF, do not unblock links");
            return false;
        }
    }

    // unblock DL transmissions with reason USING_OTHER_EMLSR_LINK
    m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                   *mldAddress,
                                   linkIds);
    return true;
}

void
EhtFrameExchangeManager::EmlsrSwitchToListening(Mac48Address address, const Time& delay)
{
    NS_LOG_FUNCTION(this << address << delay.As(Time::US));

    auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);
    NS_ASSERT_MSG(mldAddress, "MLD address not found for " << address);
    NS_ASSERT_MSG(m_apMac, "This function shall only be called by AP MLDs");

    auto blockLinks = [=, this]() {
        if (!UnblockEmlsrLinksIfAllowed(address))
        {
            NS_LOG_DEBUG("Could not unblock transmissions to " << address);
            return;
        }

        // this EMLSR client switches back to listening operation
        std::set<uint8_t> linkIds;
        for (uint8_t linkId = 0; linkId < m_mac->GetNLinks(); linkId++)
        {
            if (m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress))
            {
                linkIds.insert(linkId);
            }
        }

        // block DL transmissions on this link until transition delay elapses
        m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                     *mldAddress,
                                     linkIds);

        auto unblockLinks = [=, this]() {
            m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::WAITING_EMLSR_TRANSITION_DELAY,
                                           *mldAddress,
                                           linkIds);
        };

        // unblock all EMLSR links when the transition delay elapses
        auto emlCapabilities = GetWifiRemoteStationManager()->GetStationEmlCapabilities(address);
        NS_ASSERT(emlCapabilities);
        auto endDelay = CommonInfoBasicMle::DecodeEmlsrTransitionDelay(
            emlCapabilities->get().emlsrTransitionDelay);

        endDelay.IsZero() ? unblockLinks()
                          : static_cast<void>(m_transDelayTimer[*mldAddress] =
                                                  Simulator::Schedule(endDelay, unblockLinks));
    };

    delay.IsZero() ? blockLinks() : static_cast<void>(Simulator::Schedule(delay, blockLinks));
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

std::optional<dBm_u>
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
EhtFrameExchangeManager::SetIcfPaddingAndTxVector(CtrlTriggerHeader& trigger,
                                                  WifiTxVector& txVector) const
{
    NS_LOG_FUNCTION(this << trigger << txVector);

    if (!trigger.IsMuRts() && !trigger.IsBsrp())
    {
        NS_LOG_INFO("Not an ICF");
        return;
    }

    const auto recipients = GetTfRecipients(trigger);
    uint8_t maxPaddingDelay = 0;
    bool isUnprotectedEmlsrDst = false;

    for (const auto& address : recipients)
    {
        if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(address) ||
            m_protectedStas.contains(address))
        {
            continue; // not an EMLSR client or EMLSR client already protected
        }

        isUnprotectedEmlsrDst = true;
        auto emlCapabilities = GetWifiRemoteStationManager()->GetStationEmlCapabilities(address);
        NS_ASSERT(emlCapabilities);
        maxPaddingDelay = std::max(maxPaddingDelay, emlCapabilities->get().emlsrPaddingDelay);
    }

    if (isUnprotectedEmlsrDst)
    {
        // The initial Control frame of frame exchanges shall be sent in the non-HT PPDU or
        // non-HT duplicate PPDU format using a rate of 6 Mb/s, 12 Mb/s, or 24 Mb/s.
        // (Sec. 35.3.17 of 802.11be D3.0)
        GetWifiRemoteStationManager()->AdjustTxVectorForIcf(txVector);
    }

    // add padding (if needed)
    if (maxPaddingDelay > 0)
    {
        // see formula (35-1) in Sec. 35.5.2.2.3 of 802.11be D3.0
        auto rate = txVector.GetMode().GetDataRate(txVector);
        std::size_t nDbps = rate / 1e6 * 4; // see Table 17-4 of 802.11-2020
        trigger.SetPaddingSize((1 << (maxPaddingDelay + 2)) * nDbps / 8);
    }
}

void
EhtFrameExchangeManager::ReceivedQosNullAfterBsrpTf(Mac48Address sender)
{
    NS_LOG_FUNCTION(this << sender);

    // an EMLSR client responding to a BSRP TF must be considered protected
    if (GetWifiRemoteStationManager()->GetEmlsrEnabled(sender))
    {
        m_protectedStas.insert(sender);
    }

    HeFrameExchangeManager::ReceivedQosNullAfterBsrpTf(sender);
}

bool
EhtFrameExchangeManager::EmlsrClientCannotRespondToIcf() const
{
    NS_ASSERT(m_staMac);
    if (m_staMac->IsEmlsrLink(m_linkId))
    {
        auto mainPhy = m_staMac->GetDevice()->GetPhy(m_staMac->GetEmlsrManager()->GetMainPhyId());

        // while an ICF is being received on this link, an aux PHY that is not TX capable may get
        // a TXOP on another link, release the channel and request the main PHY to switch channel.
        // It may be decided to have the main PHY start a TXOP on the other link a PIFS after the
        // channel switch (e.g., MAC header information is not used and AllowUlTxopInRx is true).
        // Thus, when the ICF is received on this link, it is not dropped but, when the CTS must
        // be transmitted, the main PHY has already started transmitting on the other link. In
        // such a case, do not respond to the ICF.
        if (mainPhy->IsStateSwitching() || m_mac->GetLinkForPhy(mainPhy) != m_linkId)
        {
            NS_LOG_DEBUG("Main PHY is switching or operating on another link, abort ICF response");
            return true;
        }
    }
    return false;
}

void
EhtFrameExchangeManager::SendCtsAfterMuRts(const WifiMacHeader& muRtsHdr,
                                           const CtrlTriggerHeader& trigger,
                                           double muRtsSnr)
{
    NS_LOG_FUNCTION(this << muRtsHdr << trigger << muRtsSnr);

    if (EmlsrClientCannotRespondToIcf())
    {
        return;
    }
    HeFrameExchangeManager::SendCtsAfterMuRts(muRtsHdr, trigger, muRtsSnr);
}

void
EhtFrameExchangeManager::SendQosNullFramesInTbPpdu(const CtrlTriggerHeader& trigger,
                                                   const WifiMacHeader& hdr)
{
    NS_LOG_FUNCTION(this << trigger << hdr);

    if (trigger.IsBsrp() && EmlsrClientCannotRespondToIcf())
    {
        return;
    }
    HeFrameExchangeManager::SendQosNullFramesInTbPpdu(trigger, hdr);
}

void
EhtFrameExchangeManager::SwitchToListeningOrUnblockLinks(const std::set<Mac48Address>& clients)
{
    NS_LOG_FUNCTION(this);

    for (const auto& address : clients)
    {
        if (GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
        {
            // EMLSR client switched to listening operations if it was protected, otherwise
            // simply unblock transmissions
            m_protectedStas.contains(address) ? EmlsrSwitchToListening(address, Seconds(0))
                                              : (void)(UnblockEmlsrLinksIfAllowed(address));
            m_protectedStas.erase(address);
        }
    }
}

void
EhtFrameExchangeManager::CtsAfterMuRtsTimeout(Ptr<WifiMpdu> muRts, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << *muRts << txVector);
    SwitchToListeningOrUnblockLinks(m_sentRtsTo);
    HeFrameExchangeManager::CtsAfterMuRtsTimeout(muRts, txVector);
}

bool
EhtFrameExchangeManager::GetUpdateCwOnCtsTimeout() const
{
    NS_LOG_FUNCTION(this);

    if (m_apMac)
    {
        if (const auto apEmlsrManager = m_apMac->GetApEmlsrManager();
            apEmlsrManager && IsCrossLinkCollision(m_sentRtsTo))
        {
            return apEmlsrManager->UpdateCwAfterFailedIcf();
        }
    }

    return HeFrameExchangeManager::GetUpdateCwOnCtsTimeout();
}

void
EhtFrameExchangeManager::TbPpduTimeout(WifiPsduMap* psduMap, std::size_t nSolicitedStations)
{
    NS_LOG_FUNCTION(this << psduMap << nSolicitedStations);

    const auto& staMissedTbPpduFrom = m_txTimer.GetStasExpectedToRespond();
    const auto crossLinkCollision = IsCrossLinkCollision(staMissedTbPpduFrom);

    if (staMissedTbPpduFrom.size() != nSolicitedStations)
    {
        // some STAs replied, hence the transmission succeeded. EMLSR clients that did not
        // respond are switching back to listening operations
        SwitchToListeningOrUnblockLinks(staMissedTbPpduFrom);
    }

    const auto apEmlsrManager = m_apMac->GetApEmlsrManager();
    const auto updateFailedCw =
        crossLinkCollision && apEmlsrManager ? apEmlsrManager->UpdateCwAfterFailedIcf() : true;
    DoTbPpduTimeout(psduMap, nSolicitedStations, updateFailedCw);
}

void
EhtFrameExchangeManager::BlockAcksInTbPpduTimeout(WifiPsduMap* psduMap,
                                                  std::size_t nSolicitedStations)
{
    NS_LOG_FUNCTION(this << psduMap << nSolicitedStations);

    const auto& staMissedTbPpduFrom = m_txTimer.GetStasExpectedToRespond();

    if (staMissedTbPpduFrom.size() != nSolicitedStations)
    {
        // some STAs replied, hence the transmission succeeded. EMLSR clients that did not
        // respond are switching back to listening operations
        SwitchToListeningOrUnblockLinks(staMissedTbPpduFrom);
    }

    HeFrameExchangeManager::BlockAcksInTbPpduTimeout(psduMap, nSolicitedStations);
}

bool
EhtFrameExchangeManager::IsCrossLinkCollision(
    const std::set<Mac48Address>& staMissedResponseFrom) const
{
    NS_LOG_FUNCTION(this << staMissedResponseFrom.size());

    // check if all the clients that did not respond to the ICF are EMLSR clients that have sent
    // (or are sending) a frame to the AP on another link
    auto crossLinkCollision = true;

    // we blocked transmissions on the other EMLSR links for the EMLSR clients we sent the ICF to.
    // For clients that did not respond, we can unblock transmissions if there is no ongoing
    // UL TXOP held by that client
    for (const auto& address : staMissedResponseFrom)
    {
        if (!GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
        {
            crossLinkCollision = false;
            continue;
        }

        auto mldAddress = GetWifiRemoteStationManager()->GetMldAddress(address);
        NS_ASSERT(mldAddress);

        std::set<uint8_t> linkIds; // all EMLSR links of EMLSR client
        for (uint8_t linkId = 0; linkId < m_apMac->GetNLinks(); linkId++)
        {
            if (m_mac->GetWifiRemoteStationManager(linkId)->GetEmlsrEnabled(*mldAddress) &&
                linkId != m_linkId)
            {
                linkIds.insert(linkId);
            }
        }

        if (std::any_of(linkIds.cbegin(),
                        linkIds.cend(),
                        /* lambda returning true if an UL TXOP is ongoing on the given link ID */
                        [=, this](uint8_t id) {
                            auto ehtFem = StaticCast<EhtFrameExchangeManager>(
                                m_mac->GetFrameExchangeManager(id));
                            return ehtFem->m_ongoingTxopEnd.IsPending() && ehtFem->m_txopHolder &&
                                   m_mac->GetMldAddress(ehtFem->m_txopHolder.value()) == mldAddress;
                        }))
        {
            // an UL TXOP is ongoing on one EMLSR link, do not unblock links
            continue;
        }

        // no UL TXOP is ongoing on any EMLSR link; if the EMLSR client is not transmitting a
        // frame to the AP on any EMLSR link, then the lack of response to the MU-RTS was not
        // caused by a simultaneous UL transmission
        if (std::none_of(linkIds.cbegin(),
                         linkIds.cend(),
                         /* lambda returning true if an MPDU from the EMLSR client is being received
                            on the given link ID */
                         [=, this](uint8_t id) {
                             auto macHdr = m_mac->GetFrameExchangeManager(id)->GetReceivedMacHdr();
                             return macHdr.has_value() &&
                                    m_mac->GetMldAddress(macHdr->get().GetAddr2()) == mldAddress;
                         }))
        {
            crossLinkCollision = false;
        }
    }

    return crossLinkCollision;
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
EhtFrameExchangeManager::TransmissionFailed(bool forceCurrentCw)
{
    NS_LOG_FUNCTION(this << forceCurrentCw);

    if (m_staMac && m_staMac->IsEmlsrLink(m_linkId) &&
        m_staMac->GetEmlsrManager()->GetElapsedMediumSyncDelayTimer(m_linkId))
    {
        NS_LOG_DEBUG("Decrement the remaining number of TXOP attempts allowed while "
                     "MediumSyncDelay is running");
        m_staMac->GetEmlsrManager()->DecrementMediumSyncDelayNTxops(m_linkId);
    }

    HeFrameExchangeManager::TransmissionFailed(forceCurrentCw);
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

    if (m_apMac && m_apMac->GetApEmlsrManager())
    {
        m_apMac->GetApEmlsrManager()->NotifyPsduRxOk(m_linkId, psdu);
    }

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

    if (m_staMac && m_icfReceived)
    {
        // notify the EMLSR manager
        m_staMac->GetEmlsrManager()->NotifyIcfReceived(m_linkId);
        m_icfReceived = false;
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

            // the AP MLD may have sent an ICF to the EMLSR client on this link while the EMLSR
            // client was starting a TXOP on another link. To be safe, besides blocking
            // transmissions, remove the EMLSR client from the protected stations on this link
            auto linkAddr =
                m_mac->GetWifiRemoteStationManager(linkId)->GetAffiliatedStaAddress(*mldAddress);
            NS_ASSERT(linkAddr.has_value());
            auto ehtFem =
                StaticCast<EhtFrameExchangeManager>(m_mac->GetFrameExchangeManager(linkId));
            NS_LOG_DEBUG("Remove " << *linkAddr << " from protected STAs");
            ehtFem->m_protectedStas.erase(*linkAddr);
            ehtFem->m_sentRtsTo.erase(*linkAddr);
            ehtFem->m_sentFrameTo.erase(*linkAddr);
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

EventId&
EhtFrameExchangeManager::GetOngoingTxopEndEvent()
{
    return m_ongoingTxopEnd;
}

void
EhtFrameExchangeManager::PsduRxError(Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << psdu);

    if (m_apMac && m_apMac->GetApEmlsrManager())
    {
        m_apMac->GetApEmlsrManager()->NotifyPsduRxError(m_linkId, psdu);
    }
}

void
EhtFrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                     RxSignalInfo rxSignalInfo,
                                     const WifiTxVector& txVector,
                                     bool inAmpdu)
{
    NS_LOG_FUNCTION(this << *mpdu << rxSignalInfo << txVector << inAmpdu);

    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    const auto& hdr = mpdu->GetHeader();
    const auto sender = hdr.GetAddr2();

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
             sender != m_bssid // not sent by the AP this STA is associated with
             || trigger.FindUserInfoWithAid(m_staMac->GetAssociationId()) == trigger.end()))
        {
            return; // not addressed to us
        }

        if ((trigger.IsMuRts() || trigger.IsBsrp()) && !m_ongoingTxopEnd.IsPending() &&
            m_staMac->IsEmlsrLink(m_linkId))
        {
            // this is an initial Control frame
            if (DropReceivedIcf())
            {
                return;
            }

            auto emlsrManager = m_staMac->GetEmlsrManager();
            NS_ASSERT(emlsrManager);

            m_icfReceived = true;

            // we just got involved in a DL TXOP. Check if we are still involved in the TXOP in a
            // SIFS (we are expected to reply by sending a CTS frame)
            m_ongoingTxopEnd.Cancel();
            NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + m_phy->GetSifs()).As(Time::S));
            m_ongoingTxopEnd = Simulator::Schedule(m_phy->GetSifs() + NanoSeconds(1),
                                                   &EhtFrameExchangeManager::TxopEnd,
                                                   this,
                                                   sender);
        }
    }

    if (!m_icfReceived && ShallDropReceivedMpdu(mpdu))
    {
        NS_LOG_DEBUG("Drop received MPDU: " << *mpdu);
        return;
    }

    HeFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);

    if (m_apMac && GetWifiRemoteStationManager()->GetEmlsrEnabled(sender))
    {
        if (hdr.IsRts() && !m_sendCtsEvent.IsPending())
        {
            // received RTS but did not send CTS (e.g., NAV busy), start transition delay
            EmlsrSwitchToListening(sender, Time{0});
            return;
        }

        // if the AP MLD received an MPDU from an EMLSR client that is starting an UL TXOP,
        // block transmissions to the EMLSR client on other links
        CheckEmlsrClientStartingTxop(hdr, txVector);
    }
}

void
EhtFrameExchangeManager::EndReceiveAmpdu(Ptr<const WifiPsdu> psdu,
                                         const RxSignalInfo& rxSignalInfo,
                                         const WifiTxVector& txVector,
                                         const std::vector<bool>& perMpduStatus)
{
    NS_LOG_FUNCTION(
        this << *psdu << rxSignalInfo << txVector << perMpduStatus.size()
             << std::all_of(perMpduStatus.begin(), perMpduStatus.end(), [](bool v) { return v; }));

    if (ShallDropReceivedMpdu(*psdu->begin()))
    {
        return;
    }

    HeFrameExchangeManager::EndReceiveAmpdu(psdu, rxSignalInfo, txVector, perMpduStatus);
}

bool
EhtFrameExchangeManager::ShallDropReceivedMpdu(Ptr<const WifiMpdu> mpdu) const
{
    NS_LOG_FUNCTION(this << *mpdu);

    // this function only checks frames that shall be dropped by an EMLSR client
    if (!m_staMac || !m_staMac->IsEmlsrLink(m_linkId))
    {
        return false;
    }

    const auto& hdr = mpdu->GetHeader();

    // We impose that an aux PHY is only able to receive an ICF, a CF-End, a CTS or a management
    // frame (we are interested in receiving mainly Beacon frames). Note that other frames are
    // still post-processed, e.g., used to set the NAV and the TXOP holder.
    // The motivation is that, e.g., an AP MLD may send an ICF to EMLSR clients A and B;
    // A responds while B does not; the AP MLD sends a DL MU PPDU to both clients followed
    // by an MU-BAR to solicit a BlockAck from both clients. If an aux PHY of client B is
    // operating on this link, the MU-BAR will be received and a TB PPDU response sent
    // through the aux PHY.
    if (hdr.IsMgt() || hdr.IsCts() || hdr.IsCfEnd() || (hdr.IsData() && hdr.GetAddr1().IsGroup()))
    {
        return false;
    }

    // other frames cannot be received by an aux PHY
    if (m_mac->GetLinkForPhy(m_staMac->GetEmlsrManager()->GetMainPhyId()) != m_linkId)
    {
        NS_LOG_DEBUG("Dropping " << *mpdu << " received by an aux PHY on link " << +m_linkId);
        return true;
    }

    // other frames cannot be received by the main PHY when not involved in any TXOP
    if (!m_ongoingTxopEnd.IsPending() &&
        std::none_of(wifiAcList.cbegin(), wifiAcList.cend(), [=, this](const auto& aciAcPair) {
            return m_mac->GetQosTxop(aciAcPair.first)->GetTxopStartTime(m_linkId).has_value();
        }))
    {
        NS_LOG_DEBUG("Dropping " << *mpdu << " received by main PHY on link " << +m_linkId
                                 << " while no TXOP is ongoing");
        return true;
    }

    // other frames can be received by the main PHY when involved in a TXOP
    return false;
}

bool
EhtFrameExchangeManager::DropReceivedIcf()
{
    NS_LOG_FUNCTION(this);

    auto emlsrManager = m_staMac->GetEmlsrManager();
    NS_ASSERT(emlsrManager);

    if (UsingOtherEmlsrLink())
    {
        // we received an ICF on a link that is blocked because another EMLSR link is
        // being used. Check if there is an ongoing DL TXOP on the other EMLSR link
        auto apMldAddress = GetWifiRemoteStationManager()->GetMldAddress(m_bssid);
        NS_ASSERT_MSG(apMldAddress, "MLD address not found for " << m_bssid);

        if (auto it = std::find_if(
                m_staMac->GetLinkIds().cbegin(),
                m_staMac->GetLinkIds().cend(),
                /* lambda to find an EMLSR link on which there is an ongoing DL TXOP */
                [=, this](uint8_t linkId) {
                    auto ehtFem =
                        StaticCast<EhtFrameExchangeManager>(m_mac->GetFrameExchangeManager(linkId));
                    return linkId != m_linkId && m_staMac->IsEmlsrLink(linkId) &&
                           ehtFem->m_ongoingTxopEnd.IsPending() && ehtFem->m_txopHolder &&
                           m_mac->GetWifiRemoteStationManager(linkId)->GetMldAddress(
                               *ehtFem->m_txopHolder) == apMldAddress;
                });
            it != m_staMac->GetLinkIds().cend())
        {
            // AP is not expected to send ICFs on two links. If an ICF
            // has been received on this link, it means that the DL TXOP
            // on the other link terminated (e.g., the AP did not
            // receive our response)
            StaticCast<EhtFrameExchangeManager>(m_mac->GetFrameExchangeManager(*it))
                ->m_ongoingTxopEnd.Cancel();
            // we are going to start a TXOP on this link; unblock
            // transmissions on this link, the other links will be
            // blocked subsequently
            m_staMac->UnblockTxOnLink({m_linkId}, WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK);
        }
        else
        {
            // We get here likely because transmission on the other EMLSR link
            // started before the reception of the ICF ended. We drop this ICF and let the
            // UL TXOP continue.
            NS_LOG_DEBUG("Drop ICF because another EMLSR link is being used");
            m_icfDropCallback(WifiIcfDrop::USING_OTHER_LINK, m_linkId);
            return true;
        }
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
    else if (auto mainPhy = m_staMac->GetDevice()->GetPhy(emlsrManager->GetMainPhyId());
             mainPhy != m_phy)
    {
        const auto delay = mainPhy->GetChannelSwitchDelay();
        auto lastTime = mainPhy->GetState()->GetLastTime({WifiPhyState::TX});
        auto reason = WifiIcfDrop::NOT_ENOUGH_TIME_TX;

        if (auto lastSwitch = mainPhy->GetState()->GetLastTime({WifiPhyState::SWITCHING});
            lastSwitch > lastTime)
        {
            lastTime = lastSwitch;
            reason = WifiIcfDrop::NOT_ENOUGH_TIME_SWITCH;
        }
        if (auto lastSleep = mainPhy->GetState()->GetLastTime({WifiPhyState::SLEEP});
            lastSleep > lastTime)
        {
            lastTime = lastSleep;
            reason = WifiIcfDrop::NOT_ENOUGH_TIME_SLEEP;
        }
        // ignore RX state for now

        if (lastTime > Simulator::Now() - delay)
        {
            NS_LOG_DEBUG(
                "Drop ICF due to not enough time for the main PHY to switch link; reason = "
                << reason);
            m_icfDropCallback(reason, m_linkId);
            return true;
        }
    }
    return false;
}

void
EhtFrameExchangeManager::TxopEnd(const std::optional<Mac48Address>& txopHolder)
{
    NS_LOG_FUNCTION(this << txopHolder.has_value());

    if (m_phy && m_phy->GetInfoIfRxingPhyHeader())
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
        delay = txDuration + m_phy->GetSifs() + m_phy->GetSlot() + EMLSR_RX_PHY_START_DELAY;
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
    auto delay = m_phy->GetSifs() + m_phy->GetSlot() + EMLSR_RX_PHY_START_DELAY;
    NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + delay).As(Time::S));
    m_ongoingTxopEnd =
        Simulator::Schedule(delay, &EhtFrameExchangeManager::TxopEnd, this, m_txopHolder);
}

} // namespace ns3
