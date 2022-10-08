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
#include "ns3/frame-exchange-manager.h"
#include "ns3/multi-user-scheduler.h"
#include "ns3/wifi-ack-manager.h"
#include "ns3/wifi-assoc-manager.h"
#include "ns3/wifi-mac-queue-scheduler.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-protection-manager.h"

namespace ns3
{

WifiMacHelper::WifiMacHelper()
{
    // By default, we create an AdHoc MAC layer (without QoS).
    SetType("ns3::AdhocWifiMac");

    m_assocManager.SetTypeId("ns3::WifiDefaultAssocManager");
    m_queueScheduler.SetTypeId("ns3::FcfsWifiQueueScheduler");
    m_protectionManager.SetTypeId("ns3::WifiDefaultProtectionManager");
    m_ackManager.SetTypeId("ns3::WifiDefaultAckManager");
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

    Ptr<WifiMac> mac = macObjectFactory.Create<WifiMac>();
    mac->SetDevice(device);
    mac->SetAddress(Mac48Address::Allocate());
    device->SetMac(mac);
    mac->ConfigureStandard(standard);

    Ptr<WifiMacQueueScheduler> queueScheduler = m_queueScheduler.Create<WifiMacQueueScheduler>();
    mac->SetMacQueueScheduler(queueScheduler);

    // WaveNetDevice stores PHY entities in a different member than WifiNetDevice, hence
    // GetNPhys() would return 0. We have to attach a protection manager and an ack manager
    // to the unique instance of frame exchange manager anyway
    for (uint8_t linkId = 0; linkId < std::max<uint8_t>(device->GetNPhys(), 1); ++linkId)
    {
        auto fem = mac->GetFrameExchangeManager(linkId);

        Ptr<WifiProtectionManager> protectionManager =
            m_protectionManager.Create<WifiProtectionManager>();
        protectionManager->SetWifiMac(mac);
        protectionManager->SetLinkId(linkId);
        fem->SetProtectionManager(protectionManager);

        Ptr<WifiAckManager> ackManager = m_ackManager.Create<WifiAckManager>();
        ackManager->SetWifiMac(mac);
        ackManager->SetLinkId(linkId);
        fem->SetAckManager(ackManager);

        // 11be MLDs require a MAC address to be assigned to each STA. Note that
        // FrameExchangeManager objects are created by WifiMac::SetupFrameExchangeManager
        // (which is invoked by WifiMac::ConfigureStandard, which is called above),
        // which sets the FrameExchangeManager's address to the address held by WifiMac.
        // Hence, in case the number of PHY objects is 1, the FrameExchangeManager's
        // address equals the WifiMac's address.
        if (device->GetNPhys() > 1)
        {
            fem->SetAddress(Mac48Address::Allocate());
        }
    }

    // create and install the Multi User Scheduler if this is an HE AP
    Ptr<ApWifiMac> apMac;
    if (standard >= WIFI_STANDARD_80211ax && m_muScheduler.IsTypeIdSet() &&
        (apMac = DynamicCast<ApWifiMac>(mac)))
    {
        Ptr<MultiUserScheduler> muScheduler = m_muScheduler.Create<MultiUserScheduler>();
        apMac->AggregateObject(muScheduler);
    }

    // create and install the Association Manager if this is a STA
    if (auto staMac = DynamicCast<StaWifiMac>(mac); staMac != nullptr)
    {
        Ptr<WifiAssocManager> assocManager = m_assocManager.Create<WifiAssocManager>();
        staMac->SetAssocManager(assocManager);
    }

    return mac;
}

} // namespace ns3
