/*
 * Copyright (c) 2024 Universita' di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "advanced-ap-emlsr-manager.h"

#include "eht-frame-exchange-manager.h"

#include "ns3/ap-wifi-mac.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/wifi-phy.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("AdvancedApEmlsrManager");

NS_OBJECT_ENSURE_REGISTERED(AdvancedApEmlsrManager);

TypeId
AdvancedApEmlsrManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::AdvancedApEmlsrManager")
            .SetParent<DefaultApEmlsrManager>()
            .SetGroupName("Wifi")
            .AddConstructor<AdvancedApEmlsrManager>()
            .AddAttribute("UseNotifiedMacHdr",
                          "Whether to use the information about the MAC header of the MPDU "
                          "being received, if notified by the PHY.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&AdvancedApEmlsrManager::m_useNotifiedMacHdr),
                          MakeBooleanChecker())
            .AddAttribute("EarlySwitchToListening",
                          "Whether the AP MLD assumes that an EMLSR client is able to detect at "
                          "the end of the MAC header that a PSDU is not addressed to it and "
                          "immediately starts switching to listening mode.",
                          BooleanValue(false),
                          MakeBooleanAccessor(&AdvancedApEmlsrManager::m_earlySwitchToListening),
                          MakeBooleanChecker())
            .AddAttribute(
                "WaitTransDelayOnPsduRxError",
                "If true, the AP MLD waits for a response timeout after a PSDU reception "
                "error before starting the transition delay for the EMLSR client that "
                "sent the failed PSDU. Otherwise, the AP MLD does not start the "
                "transition delay timer for the EMLSR client that sent the failed PSDU.",
                BooleanValue(true),
                MakeBooleanAccessor(&AdvancedApEmlsrManager::m_waitTransDelayOnPsduRxError),
                MakeBooleanChecker())
            .AddAttribute("UpdateCwAfterFailedIcf",
                          "Whether the AP MLD shall double the CW upon CTS timeout after an "
                          "MU-RTS in case all the clients solicited by the MU-RTS are EMLSR "
                          "clients that have sent (or are sending) a frame to the AP on "
                          "another link.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&AdvancedApEmlsrManager::m_updateCwAfterFailedIcf),
                          MakeBooleanChecker());
    return tid;
}

AdvancedApEmlsrManager::AdvancedApEmlsrManager()
{
    NS_LOG_FUNCTION(this);
}

AdvancedApEmlsrManager::~AdvancedApEmlsrManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
AdvancedApEmlsrManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    for (uint8_t linkId = 0; linkId < GetApMac()->GetNLinks(); linkId++)
    {
        auto phy = GetApMac()->GetWifiPhy(linkId);
        phy->TraceDisconnectWithoutContext(
            "PhyRxMacHeaderEnd",
            MakeCallback(&AdvancedApEmlsrManager::ReceivedMacHdr, this).Bind(linkId));
    }
    DefaultApEmlsrManager::DoDispose();
}

void
AdvancedApEmlsrManager::DoSetWifiMac(Ptr<ApWifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);

    for (uint8_t linkId = 0; linkId < GetApMac()->GetNLinks(); linkId++)
    {
        auto phy = GetApMac()->GetWifiPhy(linkId);
        phy->TraceConnectWithoutContext(
            "PhyRxMacHeaderEnd",
            MakeCallback(&AdvancedApEmlsrManager::ReceivedMacHdr, this).Bind(linkId));
    }
}

void
AdvancedApEmlsrManager::ReceivedMacHdr(uint8_t linkId,
                                       const WifiMacHeader& macHdr,
                                       const WifiTxVector& txVector,
                                       Time psduDuration)
{
    NS_LOG_FUNCTION(this << linkId << macHdr << txVector << psduDuration.As(Time::MS));

    if (m_useNotifiedMacHdr && GetEhtFem(linkId)->CheckEmlsrClientStartingTxop(macHdr, txVector))
    {
        // the AP MLD is receiving an MPDU from an EMLSR client that is starting an UL TXOP.
        // CheckEmlsrClientStartingTxop has blocked transmissions to the EMLSR client on other
        // links. If the reception of the PSDU fails, however, the AP MLD does not respond and
        // the EMLSR client will switch to listening mode after the ack timeout.
        m_blockedLinksOnMacHdrRx.insert(linkId);
    }
}

void
AdvancedApEmlsrManager::NotifyPsduRxOk(uint8_t linkId, Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << linkId << *psdu);
    m_blockedLinksOnMacHdrRx.erase(linkId);
}

void
AdvancedApEmlsrManager::NotifyPsduRxError(uint8_t linkId, Ptr<const WifiPsdu> psdu)
{
    NS_LOG_FUNCTION(this << linkId << *psdu);

    if (auto it = m_blockedLinksOnMacHdrRx.find(linkId); it == m_blockedLinksOnMacHdrRx.cend())
    {
        return;
    }
    else
    {
        m_blockedLinksOnMacHdrRx.erase(it);
    }

    if (m_waitTransDelayOnPsduRxError)
    {
        auto phy = GetApMac()->GetWifiPhy(linkId);
        auto delay = phy->GetSifs() + phy->GetSlot() + EMLSR_RX_PHY_START_DELAY;
        GetEhtFem(linkId)->EmlsrSwitchToListening(psdu->GetAddr2(), delay);
        return;
    }

    // all other EMLSR links were blocked when receiving MAC header; unblock them now
    auto mldAddress =
        GetApMac()->GetWifiRemoteStationManager(linkId)->GetMldAddress(psdu->GetAddr2());
    if (!mldAddress.has_value())
    {
        NS_LOG_DEBUG(psdu->GetAddr2() << " is not an EMLSR client");
        return;
    }

    std::set<uint8_t> linkIds;
    for (uint8_t id = 0; id < GetApMac()->GetNLinks(); id++)
    {
        if (GetApMac()->GetWifiRemoteStationManager(id)->GetEmlsrEnabled(*mldAddress))
        {
            linkIds.insert(id);
        }
    }
    GetApMac()->UnblockUnicastTxOnLinks(WifiQueueBlockedReason::USING_OTHER_EMLSR_LINK,
                                        *mldAddress,
                                        linkIds);
}

Time
AdvancedApEmlsrManager::GetDelayOnTxPsduNotForEmlsr(Ptr<const WifiPsdu> psdu,
                                                    const WifiTxVector& txVector,
                                                    WifiPhyBand band)
{
    NS_LOG_FUNCTION(this << psdu << txVector << band);

    if (!m_earlySwitchToListening)
    {
        return DefaultApEmlsrManager::GetDelayOnTxPsduNotForEmlsr(psdu, txVector, band);
    }

    // EMLSR clients switch back to listening operation at the end of MAC header RX
    auto macHdrSize = (*psdu->begin())->GetHeader().GetSerializedSize() +
                      ((psdu->GetNMpdus() > 1 || psdu->IsSingle()) ? 4 : 0);
    // return the duration of the MAC header TX
    return DataRate(txVector.GetMode().GetDataRate(txVector)).CalculateBytesTxTime(macHdrSize);
}

bool
AdvancedApEmlsrManager::UpdateCwAfterFailedIcf()
{
    return m_updateCwAfterFailedIcf;
}

} // namespace ns3
