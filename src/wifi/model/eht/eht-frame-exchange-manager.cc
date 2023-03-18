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

#include "ns3/abort.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/log.h"
#include "ns3/wifi-mac-queue.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT std::clog << "[link=" << +m_linkId << "][mac=" << m_self << "] "

namespace ns3
{

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
EhtFrameExchangeManager::SetLinkId(uint8_t linkId)
{
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

    if (!m_apMac)
    {
        HeFrameExchangeManager::ForwardPsduDown(psdu, txVector);
        return;
    }

    auto txDuration = WifiPhy::CalculateTxDuration(psdu, txVector, m_phy->GetPhyBand());

    // check if the EMLSR clients shall switch back to listening operation at the end of this PPDU
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

    HeFrameExchangeManager::ForwardPsduDown(psdu, txVector);
}

void
EhtFrameExchangeManager::ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);

    if (!m_apMac)
    {
        HeFrameExchangeManager::ForwardPsduMapDown(psduMap, txVector);
        return;
    }

    auto txDuration = WifiPhy::CalculateTxDuration(psduMap, txVector, m_phy->GetPhyBand());

    // check if the EMLSR clients shall switch back to listening operation at the end of this PPDU
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

    HeFrameExchangeManager::ForwardPsduMapDown(psduMap, txVector);
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
                                            emlCapabilities->emlsrTransitionDelay),
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
EhtFrameExchangeManager::SendEmlOperatingModeNotification(
    const Mac48Address& dest,
    const MgtEmlOperatingModeNotification& frame)
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
        maxPaddingDelay = std::max(maxPaddingDelay, emlCapabilities->emlsrPaddingDelay);

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
        WifiMuRtsCtsProtection* protection =
            static_cast<WifiMuRtsCtsProtection*>(txParams.m_protection.get());
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
EhtFrameExchangeManager::NotifyChannelReleased(Ptr<Txop> txop)
{
    NS_LOG_FUNCTION(this << txop);

    // the channel has been released; all EMLSR clients will switch back to listening
    // operation after a timeout interval of aSIFSTime + aSlotTime + aRxPHYStartDelay
    auto delay = m_phy->GetSifs() + m_phy->GetSlot() + MicroSeconds(20);
    for (const auto& address : m_protectedStas)
    {
        if (GetWifiRemoteStationManager()->GetEmlsrEnabled(address))
        {
            EmlsrSwitchToListening(address, delay);
        }
    }

    HeFrameExchangeManager::NotifyChannelReleased(txop);
}

} // namespace ns3
