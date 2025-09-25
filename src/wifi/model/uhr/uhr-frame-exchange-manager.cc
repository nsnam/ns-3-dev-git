/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-frame-exchange-manager.h"

#include "dso-manager.h"
#include "uhr-phy.h"

#include "ns3/ap-wifi-mac.h"
#include "ns3/sta-wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-ns3-constants.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

namespace
{
/**
 * @param delay the DSO Padding delay
 * @return the encoded value for the DSO Padding Delay subfield
 * TODO: not defined yet in 802.11bn D1.0, this is a temporary solution based on EMLSR padding delay
 * encoding for now
 */
uint8_t
EncodeDsoPaddingDelay(const ns3::Time& delay)
{
    auto delayUs = delay.GetMicroSeconds();

    if (delayUs == 0)
    {
        return 0;
    }

    for (uint8_t i = 1; i <= 6; i++)
    {
        if (1 << (i + 4) == delayUs)
        {
            return i;
        }
    }

    NS_ABORT_MSG("Value not allowed (" << delay.As(ns3::Time::US) << ")");
    return 0;
}
} // namespace

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UhrFrameExchangeManager");

NS_OBJECT_ENSURE_REGISTERED(UhrFrameExchangeManager);

TypeId
UhrFrameExchangeManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::UhrFrameExchangeManager")
                            .SetParent<EhtFrameExchangeManager>()
                            .AddConstructor<UhrFrameExchangeManager>()
                            .SetGroupName("Wifi");
    return tid;
}

UhrFrameExchangeManager::UhrFrameExchangeManager()
{
    NS_LOG_FUNCTION(this);
}

UhrFrameExchangeManager::~UhrFrameExchangeManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
UhrFrameExchangeManager::ReceiveMpdu(Ptr<const WifiMpdu> mpdu,
                                     RxSignalInfo rxSignalInfo,
                                     const WifiTxVector& txVector,
                                     bool inAmpdu)
{
    NS_LOG_FUNCTION(this << *mpdu << rxSignalInfo << txVector << inAmpdu);

    // The received MPDU is either broadcast or addressed to this station
    NS_ASSERT(mpdu->GetHeader().GetAddr1().IsGroup() || mpdu->GetHeader().GetAddr1() == m_self);

    const auto& hdr = mpdu->GetHeader();
    auto sender = hdr.GetAddr2();

    if (hdr.IsTrigger())
    {
        if (!m_staMac || !m_staMac->IsAssociated())
        {
            return; // Trigger Frames are only processed by associated STAs
        }

        CtrlTriggerHeader trigger;
        mpdu->GetPacket()->PeekHeader(trigger);

        if ((hdr.GetAddr1() != m_self) &&
            (!hdr.GetAddr1().IsBroadcast() ||
             (sender != m_bssid))) // not sent by the AP this STA is associated with
        {
            return; // not addressed to us
        }

        if (auto dsoManager = m_staMac->GetDsoManager();
            trigger.IsBsrp() && !m_ongoingTxopEnd.IsPending() && dsoManager)
        {
            // this is a DSO ICF

            auto it = trigger.FindUserInfoWithAid(m_staMac->GetAssociationId());
            if (it == trigger.end())
            {
                dsoManager->NotifyIcfReceived(m_linkId, std::nullopt);
                return;
            }

            dsoManager->NotifyIcfReceived(m_linkId, it->GetRuAllocation());

            // we just got involved in a DL TXOP. Check if we are still involved in the TXOP in a
            // SIFS (we are expected to reply by sending a ICR frame)
            NS_LOG_DEBUG("Expected TXOP end=" << (Simulator::Now() + m_phy->GetSifs()).As(Time::S));
            m_ongoingTxopEnd = Simulator::Schedule(
                m_phy->GetSifs() + dsoManager->GetSwitchingDelayToDsoSubband() + TimeStep(1),
                &UhrFrameExchangeManager::TxopEnd,
                this,
                sender);

            m_dsoIcfReceived = true;
        }
        m_trigger = trigger;
    }

    EhtFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

void
UhrFrameExchangeManager::ReceivedQosNullAfterBsrpTf(Mac48Address sender)
{
    NS_LOG_FUNCTION(this << sender);

    // a DSO client responding to an ICF must be considered protected
    if (GetWifiRemoteStationManager()->GetDsoEnabled(sender))
    {
        m_protectedStas.insert(sender);
    }

    EhtFrameExchangeManager::ReceivedQosNullAfterBsrpTf(sender);
}

void
UhrFrameExchangeManager::TxopEnd(const std::optional<Mac48Address>& txopHolder)
{
    NS_LOG_FUNCTION(this << txopHolder.has_value());

    EhtFrameExchangeManager::TxopEnd(txopHolder);

    if (m_ongoingTxopEnd.IsPending())
    {
        // TXOP end has been postponed
        return;
    }

    if (Ptr<DsoManager> dsoManager; m_staMac && (dsoManager = m_staMac->GetDsoManager()))
    {
        dsoManager->NotifyTxopEnd(m_linkId, m_trigger.has_value());
    }
    m_trigger.reset();
}

bool
UhrFrameExchangeManager::ShouldDsoSwitchBackToPrimary(Ptr<const WifiPsdu> psdu,
                                                      uint16_t aid,
                                                      const Mac48Address& address) const
{
    // The non-AP STA shall switch back from the DSO subband to the primary subband (...) when (...)
    // this non-AP STA does not detect (...) any of the following frames:
    // - an individually addressed frame with the RA equal to the MAC address of the non-AP STA
    // affiliated with the non-AP MLD
    if (psdu->GetAddr1() == address)
    {
        return false;
    }

    // - a Trigger frame that has one of the User Info fields addressed to the non-AP STA
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

    // - a CTS-to-self frame with the RA equal to the MAC address of the AP
    if (psdu->GetHeader(0).IsCts())
    {
        if (m_staMac && psdu->GetAddr1() == m_bssid)
        {
            return false;
        }
    }

    // - a Multi-STA BlockAck frame that has one of the Per AID TID Info fields addressed to the
    // non-AP STA
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
UhrFrameExchangeManager::PostProcessFrame(Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);

    if (Ptr<DsoManager> dsoManager;
        m_staMac && (dsoManager = m_staMac->GetDsoManager()) && m_ongoingTxopEnd.IsPending() &&
        dsoManager->IsOnDsoSubband(m_linkId) &&
        ShouldDsoSwitchBackToPrimary(psdu, m_staMac->GetAssociationId(), m_self))
    {
        NS_LOG_DEBUG("No longer involved in the TXOP and switching back to primary subchannel");
        m_ongoingTxopEnd.Cancel();
        dsoManager->NotifyTxopEnd(m_linkId, false, psdu, txVector);
    }

    EhtFrameExchangeManager::PostProcessFrame(psdu, txVector);
}

void
UhrFrameExchangeManager::ForwardPsduDown(Ptr<const WifiPsdu> psdu, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psdu << txVector);
    m_trigger.reset();
    EhtFrameExchangeManager::ForwardPsduDown(psdu, txVector);
}

void
UhrFrameExchangeManager::ForwardPsduMapDown(WifiConstPsduMap psduMap, WifiTxVector& txVector)
{
    NS_LOG_FUNCTION(this << psduMap << txVector);

    if (const auto addr1 = psduMap.cbegin()->second->GetAddr1();
        m_dsoIcfReceived && m_staMac && m_staMac->GetDsoManager() && (addr1 == m_bssid) &&
        txVector.IsUlMu() && psduMap.cbegin()->second->GetHeader(0).IsQosData() &&
        !psduMap.cbegin()->second->GetHeader(0).HasData())
    {
        m_staMac->GetDsoManager()->NotifyIcrTransmitted(m_linkId);
    }
    m_dsoIcfReceived = false;
    m_trigger.reset();

    if (m_apMac && IsTrigger(psduMap))
    {
        CtrlTriggerHeader trigger;
        psduMap.cbegin()->second->GetPayload(0)->PeekHeader(trigger);
        if (trigger.IsBsrp())
        {
            const auto& aidAddrMap = m_apMac->GetStaList(m_linkId);
            for (const auto& userInfo : trigger)
            {
                const auto addressIt = aidAddrMap.find(userInfo.GetAid12());
                NS_ASSERT_MSG(addressIt != aidAddrMap.end(), "AID not found");
                if (!GetWifiRemoteStationManager()->GetDsoEnabled(addressIt->second))
                {
                    continue;
                }
                m_dsoStas.emplace(addressIt->second, userInfo.GetRuAllocation());
            }
        }
    }

    EhtFrameExchangeManager::ForwardPsduMapDown(psduMap, txVector);
}

void
UhrFrameExchangeManager::NotifyChannelReleased()
{
    NS_LOG_FUNCTION(this);

    if (m_apMac && !m_dsoStas.empty())
    {
        Time delay{0};
        if (ReceivedDsoIcfResponsesExceptOnP20())
        {
            // the channel has been released; if the DSO TXOP terminated because no non-AP STA
            // assigned in the P20 subband responded to the DSO ICF and there is at least one
            // response to the DSO ICF from a non-AP STA on any other subband, DSO STAs will start
            // to switch back to the primary subband in a SIFS + slot + PHY RXSTART delay.
            delay = m_phy->GetSifs() + m_phy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
        }
        else if (const auto remTxNav = m_txNav - Simulator::Now();
                 remTxNav.IsStrictlyPositive() || m_protectSingleExchange)
        {
            // the channel has been released; if the TXNAV is still set or if
            // m_protectSingleExchange is enabled, it means no CF-End is going to be sent. In this
            // case, DSO clients wait for a slot plus the PHY RX start delay before switching back
            // to the DSO subband (in this case, this function is called a SIFS after the last frame
            // in the TXOP)
            delay = m_phy->GetSlot() + EMLSR_OR_DSO_RX_PHY_START_DELAY;
        }

        SwitchToPrimarySubchannel(delay);

        m_dsoStas.clear();
    }

    EhtFrameExchangeManager::NotifyChannelReleased();
}

void
UhrFrameExchangeManager::DsoSwitchBackToPrimary(const Mac48Address& address, const Time& delay)
{
    NS_LOG_FUNCTION(this << address << delay.As(Time::US));

    NS_ASSERT_MSG(GetWifiRemoteStationManager()->GetDsoEnabled(address),
                  "DSO switch back to primary requested for a non-DSO STA");

    const auto clientAddress =
        GetWifiRemoteStationManager()->GetMldAddress(address).value_or(address);

    NS_LOG_DEBUG("DSO switch back delay started: blocking link " << +m_linkId << " for address "
                                                                 << clientAddress);
    m_mac->BlockUnicastTxOnLinks(WifiQueueBlockedReason::WAITING_DSO_SWITCH_BACK_DELAY,
                                 clientAddress,
                                 {m_linkId});

    auto unblockLink = [=, this]() {
        NS_LOG_DEBUG("DSO switch back delay expired: unblocking link "
                     << +m_linkId << " for address " << address);
        m_mac->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::WAITING_DSO_SWITCH_BACK_DELAY,
                                       address,
                                       {m_linkId});
    };

    Simulator::Schedule(delay, unblockLink);
}

void
UhrFrameExchangeManager::TbPpduTimeout(WifiPsduMap* psduMap, std::size_t nSolicitedStations)
{
    NS_LOG_FUNCTION(this << psduMap << nSolicitedStations);
    const auto& staMissedTbPpduFrom = m_txTimer.GetStasExpectedToRespond();

    CtrlTriggerHeader trigger;
    psduMap->cbegin()->second->GetPayload(0)->PeekHeader(trigger);
    if (trigger.IsBsrp())
    {
        if (staMissedTbPpduFrom.size() == nSolicitedStations)
        {
            // No ICF response received from any DSO STA, we cannot differentiate between ICF not
            // received by all STAs from all ICF responses being corrupted. Hence, we assume the
            // worst case and we consider an ICF response is being transmitted before DSO STAs
            // switch back to the primary subband.

            const auto tbTxVector = trigger.GetHeTbTxVector(trigger.begin()->GetAid12());
            const auto tbResponseDuration =
                HePhy::ConvertLSigLengthToHeTbPpduDuration(trigger.GetUlLength(),
                                                           tbTxVector,
                                                           m_phy->GetPhyBand());
            const auto remainingIcrDuration =
                tbResponseDuration - WifiPhy::CalculatePhyPreambleAndHeaderDuration(tbTxVector);
            const auto delay =
                remainingIcrDuration + m_phy->GetSifs() + EMLSR_OR_DSO_RX_PHY_START_DELAY;

            SwitchToPrimarySubchannel(delay);

            m_dsoStas.clear();
        }
        else if (ReceivedDsoIcfResponsesExceptOnP20())
        {
            NS_LOG_DEBUG("No non-AP STA assigned in the P20 subband responded to the DSO ICF and "
                         "there is at least one response to the DSO ICF from a non-AP STA on any "
                         "other subband: terminate the TXOP");
            TransmissionFailed();
            return;
        }
    }

    EhtFrameExchangeManager::TbPpduTimeout(psduMap, nSolicitedStations);
}

bool
UhrFrameExchangeManager::ReceivedDsoIcfResponsesExceptOnP20() const
{
    auto assignedRuInP20{false};
    auto assignedRuOtherThanP20{false};
    const auto p20Index{m_phy->GetOperatingChannel().GetPrimary20Index()};
    const auto& aidAddrMap = m_apMac->GetStaList(m_linkId);
    const auto& userInfos = m_trigVector.GetHeMuUserInfoMap();
    for (const auto& [aid, userInfo] : userInfos)
    {
        const auto addressIt = aidAddrMap.find(aid);
        NS_ASSERT_MSG(addressIt != aidAddrMap.end(), "AID not found");
        if (!m_protectedStas.contains(addressIt->second))
        {
            // ICF response not received
            continue;
        }
        if (const auto indices =
                m_phy->GetOperatingChannel().Get20MHzIndicesCoveringRu(userInfo.ru,
                                                                       m_phy->GetChannelWidth());
            indices.contains(p20Index))
        {
            assignedRuInP20 = true;
        }
        else
        {
            assignedRuOtherThanP20 = true;
        }
    }
    return !assignedRuInP20 && assignedRuOtherThanP20;
}

std::set<uint8_t>
UhrFrameExchangeManager::GetIndicesOccupyingRu(const CtrlTriggerHeader& trigger, uint16_t aid) const
{
    NS_ASSERT(m_staMac);
    if (m_staMac->GetDevice()->IsDsoActivated() && !trigger.IsMuRts() &&
        (trigger.GetVariant() == TriggerFrameVariant::UHR) &&
        (trigger.GetUlBandwidth() > m_phy->GetChannelWidth()))
    {
        auto userInfoIt = trigger.FindUserInfoWithAid(aid);
        NS_ASSERT_MSG(userInfoIt != trigger.end(),
                      "No User Info field for STA (" << m_self << ") AID=" << aid);
        return m_phy->GetOperatingChannel().Get20MHzIndicesCoveringRu(userInfoIt->GetRuAllocation(),
                                                                      m_phy->GetChannelWidth());
    }
    return EhtFrameExchangeManager::GetIndicesOccupyingRu(trigger, aid);
}

void
UhrFrameExchangeManager::SwitchToPrimarySubchannel(const Time& delay)
{
    NS_LOG_FUNCTION(this << delay);

    Time maxSwitchChannelBackDelay;
    for (const auto& [address, ru] : m_dsoStas)
    {
        auto dsoParams = GetWifiRemoteStationManager()->GetStationDsoParameters(address);
        maxSwitchChannelBackDelay =
            dsoParams
                ? std::max(maxSwitchChannelBackDelay, dsoParams->dsoSwitchBackDelay)
                : std::max(maxSwitchChannelBackDelay,
                           DEFAULT_CHANNEL_SWITCH_DELAY); // use default value since exchange of DSO
                                                          // parameters is not implemented yet
    }

    for (const auto& [address, ru] : m_dsoStas)
    {
        DsoSwitchBackToPrimary(address, maxSwitchChannelBackDelay + delay);
    }
}

void
UhrFrameExchangeManager::SetIcfPaddingAndTxVector(CtrlTriggerHeader& trigger,
                                                  WifiTxVector& txVector) const
{
    NS_LOG_FUNCTION(this << trigger << txVector);

    EhtFrameExchangeManager::SetIcfPaddingAndTxVector(trigger, txVector);

    if (!trigger.IsBsrp())
    {
        NS_LOG_DEBUG("Not a DSO ICF");
        return;
    }

    uint8_t maxPaddingDelay{0};
    bool isUnprotectedDsoDst{false};

    const auto recipients = GetTfRecipients(trigger);
    for (const auto& address : recipients)
    {
        if (!GetWifiRemoteStationManager()->GetDsoEnabled(address) ||
            m_protectedStas.contains(address))
        {
            continue; // not a DSO client or DSO client already protected
        }

        isUnprotectedDsoDst = true;
        auto dsoParams = GetWifiRemoteStationManager()->GetStationDsoParameters(address);
        if (dsoParams)
        {
            const auto staPaddingDelay = EncodeDsoPaddingDelay(dsoParams->dsoPaddingDelay);
            maxPaddingDelay = std::max(maxPaddingDelay, staPaddingDelay);
        }
    }

    if (isUnprotectedDsoDst)
    {
        // The initial Control frame of frame exchanges shall be sent in the non-HT PPDU or
        // non-HT duplicate PPDU format using a rate of 6 Mb/s, 12 Mb/s, or 24 Mb/s.
        // (Sec. 35.3.17 of 802.11be D3.0)
        GetWifiRemoteStationManager()->AdjustTxVectorForIcf(txVector);

        // The number of spatial streams in response to the BSRP Trigger frame as the DSO ICF shall
        // be limited to one for all the scheduled DSO non-AP STAs and shall be indicated in the
        // BSRP Trigger frame.
        for (auto& userInfo : trigger)
        {
            userInfo.SetSsAllocation(1, 1);
        }
    }

    // add padding (if needed)
    if (maxPaddingDelay > 0)
    {
        // TODO: DSO padding not defined in 802.11bn D1.0, hence use same calculation as EMLSR
        // padding for now
        auto rate = txVector.GetMode().GetDataRate(txVector);
        std::size_t nDbps = rate / 1e6 * 4;
        trigger.SetPaddingSize((1 << (maxPaddingDelay + 2)) * nDbps / 8);
    }
}

} // namespace ns3
