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
                                          uint16_t txWidth,
                                          const Mac48Address& receiver) const
{
    NS_LOG_FUNCTION(this << muRts << txWidth << receiver);

    CtrlTriggerUserInfoField& ui = muRts.AddUserInfoField();

    NS_ABORT_MSG_IF(m_mac->GetTypeOfStation() != AP, "HE APs only can send MU-RTS");
    auto apMac = StaticCast<ApWifiMac>(m_mac);
    ui.SetAid12(apMac->GetAssociationId(receiver, m_linkId));

    const uint16_t ctsTxWidth =
        std::min(txWidth, GetWifiRemoteStationManager()->GetChannelWidthSupported(receiver));
    auto phy = m_mac->GetWifiPhy(m_linkId);
    std::size_t primaryIdx = phy->GetOperatingChannel().GetPrimaryChannelIndex(ctsTxWidth);
    if (phy->GetChannelWidth() == 160 && ctsTxWidth <= 40 && primaryIdx >= 80 / ctsTxWidth)
    {
        // the primary80 is in the higher part of the 160 MHz channel
        primaryIdx -= 80 / ctsTxWidth;
    }
    switch (ctsTxWidth)
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
