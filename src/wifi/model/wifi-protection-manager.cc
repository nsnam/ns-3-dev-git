/*
 * Copyright (c) 2020 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#include "wifi-protection-manager.h"

#include "ap-wifi-mac.h"
#include "wifi-phy.h"

#include "ns3/log.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiProtectionManager");

NS_OBJECT_ENSURE_REGISTERED(WifiProtectionManager);

TypeId
WifiProtectionManager::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::WifiProtectionManager").SetParent<Object>().SetGroupName("Wifi");
    return tid;
}

WifiProtectionManager::WifiProtectionManager()
    : m_linkId(0)
{
    NS_LOG_FUNCTION(this);
}

WifiProtectionManager::~WifiProtectionManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
WifiProtectionManager::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_mac = nullptr;
    Object::DoDispose();
}

void
WifiProtectionManager::SetWifiMac(Ptr<WifiMac> mac)
{
    NS_LOG_FUNCTION(this << mac);
    m_mac = mac;
}

Ptr<WifiRemoteStationManager>
WifiProtectionManager::GetWifiRemoteStationManager() const
{
    return m_mac->GetWifiRemoteStationManager(m_linkId);
}

void
WifiProtectionManager::SetLinkId(uint8_t linkId)
{
    NS_LOG_FUNCTION(this << +linkId);
    m_linkId = linkId;
}

void
WifiProtectionManager::AddUserInfoToMuRts(CtrlTriggerHeader& muRts,
                                          MHz_u txWidth,
                                          const Mac48Address& receiver) const
{
    NS_LOG_FUNCTION(this << muRts << txWidth << receiver);

    CtrlTriggerUserInfoField& ui = muRts.AddUserInfoField();

    NS_ABORT_MSG_IF(m_mac->GetTypeOfStation() != AP, "HE APs only can send MU-RTS");
    auto apMac = StaticCast<ApWifiMac>(m_mac);
    ui.SetAid12(apMac->GetAssociationId(receiver, m_linkId));

    const auto ctsTxWidth =
        std::min(txWidth, GetWifiRemoteStationManager()->GetChannelWidthSupported(receiver));
    auto phy = m_mac->GetWifiPhy(m_linkId);
    std::size_t primaryIdx = phy->GetOperatingChannel().GetPrimaryChannelIndex(ctsTxWidth);
    std::size_t idx80MHz = MHz_u{80} / ctsTxWidth;
    if ((phy->GetChannelWidth() == MHz_u{160}) && (ctsTxWidth <= MHz_u{40}) &&
        (primaryIdx >= idx80MHz))
    {
        // the primary80 is in the higher part of the 160 MHz channel
        primaryIdx -= idx80MHz;
    }
    switch (static_cast<uint16_t>(ctsTxWidth))
    {
    case 20:
        ui.SetMuRtsRuAllocation(61 + primaryIdx);
        break;
    case 40:
        ui.SetMuRtsRuAllocation(65 + primaryIdx);
        break;
    case 80:
        ui.SetMuRtsRuAllocation(67);
        break;
    case 160:
        ui.SetMuRtsRuAllocation(68);
        break;
    default:
        NS_ABORT_MSG("Unhandled TX width: " << ctsTxWidth << " MHz");
    }
}

} // namespace ns3
