/*
 * Copyright (c) 2016
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "wifi-mac-helper.h"

#include "ns3/boolean.h"
#include "ns3/eht-configuration.h"
#include "ns3/emlsr-manager.h"
#include "ns3/enum.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/multi-user-scheduler.h"
#include "ns3/pointer.h"
#include "ns3/wifi-ack-manager.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection-manager.h"

#include <sstream>
#include <vector>

namespace ns3
{

WifiMacHelper::WifiMacHelper()
{
    // By default, we create an AdHoc MAC layer (without QoS).
    SetType("ns3::AdhocWifiMac");

    m_dcf.SetTypeId("ns3::Txop");
    for (const auto& [aci, ac] : wifiAcList)
    {
        auto [it, inserted] = m_edca.try_emplace(aci);
        it->second.SetTypeId("ns3::QosTxop");
    }
    m_channelAccessManager.SetTypeId("ns3::ChannelAccessManager");
    // Setting FEM attributes requires setting a TypeId first. We initialize the TypeId to the FEM
    // of the latest standard, in order to allow users to set the attributes of all the FEMs. The
    // Create method will set the requested standard before creating the FEM(s).
    m_frameExchangeManager.SetTypeId(GetFrameExchangeManagerTypeIdName(WIFI_STANDARD_COUNT, true));
    m_assocManager.SetTypeId("ns3::WifiDefaultAssocManager");
    m_queueScheduler.SetTypeId("ns3::FcfsWifiQueueScheduler");
    m_protectionManager.SetTypeId("ns3::WifiDefaultProtectionManager");
    m_ackManager.SetTypeId("ns3::WifiDefaultAckManager");
    m_emlsrManager.SetTypeId("ns3::DefaultEmlsrManager");
}

WifiMacHelper::~WifiMacHelper()
{
}

Ptr<WifiMac>
WifiMacHelper::Create(Ptr<WifiNetDevice> device, WifiStandard standard) const
{
    NS_ABORT_MSG_IF(standard == WIFI_STANDARD_UNSPECIFIED, "No standard specified!");

    // this is a const method, but we need to force the correct QoS setting
    ObjectFactory macObjectFactory = m_mac;
    if (standard >= WIFI_STANDARD_80211n)
    {
        macObjectFactory.Set("QosSupported", BooleanValue(true));
    }

    // do not create a Txop if the standard is at least 802.11n
    if (standard < WIFI_STANDARD_80211n)
    {
        auto dcf = m_dcf; // create a copy because this is a const method
        dcf.Set("AcIndex", EnumValue<AcIndex>(AC_BE_NQOS));
        macObjectFactory.Set("Txop", PointerValue(dcf.Create<Txop>()));
    }
    // create (Qos)Txop objects
    for (auto [aci, edca] : m_edca)
    {
        edca.Set("AcIndex", EnumValue<AcIndex>(aci));
        std::stringstream ss;
        ss << aci << "_Txop";
        auto s = ss.str().substr(3); // discard "AC "
        macObjectFactory.Set(s, PointerValue(edca.Create<QosTxop>()));
    }

    // WaveNetDevice (through ns-3.38) stores PHY entities in a different member than WifiNetDevice,
    // hence GetNPhys() would return 0
    auto nLinks = std::max<uint8_t>(device->GetNPhys(), 1);

    // create Channel Access Managers
    std::vector<Ptr<ChannelAccessManager>> caManagers;
    for (uint8_t linkId = 0; linkId < nLinks; ++linkId)
    {
        caManagers.emplace_back(m_channelAccessManager.Create<ChannelAccessManager>());
    }

    Ptr<WifiMac> mac = macObjectFactory.Create<WifiMac>();
    mac->SetDevice(device);
    mac->SetAddress(Mac48Address::Allocate());
    device->SetMac(mac);
    mac->SetChannelAccessManagers(caManagers);

    // create Frame Exchange Managers, each with an attached protection manager and ack manager
    std::vector<Ptr<FrameExchangeManager>> feManagers;
    auto frameExchangeManager = m_frameExchangeManager;
    frameExchangeManager.SetTypeId(
        GetFrameExchangeManagerTypeIdName(standard, mac->GetQosSupported()));
    for (uint8_t linkId = 0; linkId < nLinks; ++linkId)
    {
        auto fem = frameExchangeManager.Create<FrameExchangeManager>();
        feManagers.emplace_back(fem);

        auto protectionManager = m_protectionManager.Create<WifiProtectionManager>();
        protectionManager->SetWifiMac(mac);
        protectionManager->SetLinkId(linkId);
        fem->SetProtectionManager(protectionManager);

        auto ackManager = m_ackManager.Create<WifiAckManager>();
        ackManager->SetWifiMac(mac);
        ackManager->SetLinkId(linkId);
        fem->SetAckManager(ackManager);

        // 11be MLDs require a MAC address to be assigned to each STA
        fem->SetAddress(device->GetNPhys() > 1 ? Mac48Address::Allocate() : mac->GetAddress());
    }

    mac->SetFrameExchangeManagers(feManagers);

    Ptr<WifiMacQueueScheduler> queueScheduler = m_queueScheduler.Create<WifiMacQueueScheduler>();
    mac->SetMacQueueScheduler(queueScheduler);

    // create and install the Multi User Scheduler if this is an HE AP
    Ptr<ApWifiMac> apMac;
    if (standard >= WIFI_STANDARD_80211ax && m_muScheduler.IsTypeIdSet() &&
        (apMac = DynamicCast<ApWifiMac>(mac)))
    {
        Ptr<MultiUserScheduler> muScheduler = m_muScheduler.Create<MultiUserScheduler>();
        apMac->AggregateObject(muScheduler);
    }

    // create and install the Association Manager if this is a STA
    auto staMac = DynamicCast<StaWifiMac>(mac);
    if (staMac)
    {
        Ptr<WifiAssocManager> assocManager = m_assocManager.Create<WifiAssocManager>();
        staMac->SetAssocManager(assocManager);
    }

    // create and install the EMLSR Manager if this is an EHT non-AP MLD with EMLSR activated
    if (BooleanValue emlsrActivated;
        standard >= WIFI_STANDARD_80211be && staMac && staMac->GetNLinks() > 1 &&
        device->GetEhtConfiguration()->GetAttributeFailSafe("EmlsrActivated", emlsrActivated) &&
        emlsrActivated.Get())
    {
        auto emlsrManager = m_emlsrManager.Create<EmlsrManager>();
        staMac->SetEmlsrManager(emlsrManager);
    }

    return mac;
}

} // namespace ns3
