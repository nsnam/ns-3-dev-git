/*
 * Copyright (c) 2025 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */
#include "wifi-coex-manager.h"

#include "sta-wifi-mac.h"
#include "wifi-net-device.h"
#include "wifi-phy.h"

#include "ns3/coex-arbitrator.h"
#include "ns3/log.h"
#include "ns3/nstime.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("WifiCoexManager");

NS_OBJECT_ENSURE_REGISTERED(WifiCoexManager);

TypeId
WifiCoexManager::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiCoexManager")
                            .SetParent<coex::DeviceManager>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiCoexManager>();
    return tid;
}

WifiCoexManager::WifiCoexManager()
{
    NS_LOG_FUNCTION(this);
}

WifiCoexManager::~WifiCoexManager()
{
    NS_LOG_FUNCTION_NOARGS();
}

void
WifiCoexManager::DoDispose()
{
    NS_LOG_FUNCTION_NOARGS();
    m_device = nullptr;
    m_resourceBusyEvent.Cancel();
    coex::DeviceManager::DoDispose();
}

void
WifiCoexManager::SetWifiNetDevice(Ptr<WifiNetDevice> device)
{
    NS_LOG_FUNCTION(this << device);
    m_device = device;
}

void
WifiCoexManager::ResourceBusyStart(const coex::Event& coexEvent)
{
    NS_LOG_FUNCTION(this << coexEvent);

    auto staMac = DynamicCast<StaWifiMac>(m_device->GetMac());

    if (!staMac)
    {
        // nothing to do if this is not a non-AP STA/MLD
        return;
    }

    // TODO For now, assume that all links are unavailable
    const auto links = staMac->GetSetupLinkIds();

    if (!m_resourceBusyEvent.IsPending())
    {
        for (const auto linkId : links)
        {
            NS_LOG_DEBUG("Switching off PHY on link " << +linkId);
            staMac->GetWifiPhy(linkId)->SetOffMode();
        }
    }

    const auto delay = Max(coexEvent.duration, Simulator::GetDelayLeft(m_resourceBusyEvent));
    m_resourceBusyEvent.Cancel();
    m_resourceBusyEvent = Simulator::Schedule(delay, [=] {
        for (const auto linkId : links)
        {
            NS_LOG_DEBUG("Switching on PHY on link " << +linkId);
            staMac->GetWifiPhy(linkId)->ResumeFromOff();
        }
    });
}

} // namespace ns3
