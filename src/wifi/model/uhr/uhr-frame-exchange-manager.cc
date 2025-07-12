/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "uhr-frame-exchange-manager.h"

#include "dso-manager.h"

#include "ns3/sta-wifi-mac.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT WIFI_FEM_NS_LOG_APPEND_CONTEXT

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
    }

    EhtFrameExchangeManager::ReceiveMpdu(mpdu, rxSignalInfo, txVector, inAmpdu);
}

void
UhrFrameExchangeManager::NotifySwitchingStartNow(Time duration)
{
    NS_LOG_FUNCTION(this << duration);
    if (m_switchingForDso)
    {
        NS_LOG_DEBUG("Switching channel to/from DSO subband for DSO operations");
        m_switchingForDso = false;
        return;
    }
    EhtFrameExchangeManager::NotifySwitchingStartNow(duration);
}

void
UhrFrameExchangeManager::NotifyDsoSwitching()
{
    NS_LOG_FUNCTION(this);
    m_switchingForDso = true;
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
        dsoManager->NotifyTxopEnd(m_linkId);
    }
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
        dsoManager->NotifyTxopEnd(m_linkId, psdu, txVector);
    }

    EhtFrameExchangeManager::PostProcessFrame(psdu, txVector);
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

    EhtFrameExchangeManager::ForwardPsduMapDown(psduMap, txVector);
}

} // namespace ns3
