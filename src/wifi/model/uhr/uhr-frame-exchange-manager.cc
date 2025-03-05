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

        auto it = trigger.FindUserInfoWithAid(m_staMac->GetAssociationId());
        if ((hdr.GetAddr1() != m_self) &&
            (!hdr.GetAddr1().IsBroadcast() ||
             (sender != m_bssid) // not sent by the AP this STA is associated with
             || (it == trigger.end())))
        {
            return; // not addressed to us
        }

        if (auto dsoManager = m_staMac->GetDsoManager();
            trigger.IsBsrp() && !m_ongoingTxopEnd.IsPending() && dsoManager)
        {
            // this is a DSO ICF
            dsoManager->NotifyIcfReceived(m_linkId, it->GetRuAllocation());
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

} // namespace ns3
