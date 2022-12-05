/*
 * Copyright (c) 2008,2009 IITP RAS
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
 * Author: Kirill Andreev <andreev@iitp.ru>
 *         Pavel Boyko <boyko@iitp.ru>
 */

#include "mesh-helper.h"

#include "ns3/fcfs-wifi-queue-scheduler.h"
#include "ns3/frame-exchange-manager.h"
#include "ns3/mesh-point-device.h"
#include "ns3/mesh-wifi-interface-mac.h"
#include "ns3/minstrel-wifi-manager.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"
#include "ns3/wifi-default-ack-manager.h"
#include "ns3/wifi-default-protection-manager.h"
#include "ns3/wifi-helper.h"
#include "ns3/wifi-net-device.h"

namespace ns3
{
MeshHelper::MeshHelper()
    : m_nInterfaces(1),
      m_spreadChannelPolicy(ZERO_CHANNEL),
      m_stack(nullptr),
      m_standard(WIFI_STANDARD_80211a)
{
}

MeshHelper::~MeshHelper()
{
    m_stack = nullptr;
}

void
MeshHelper::SetSpreadInterfaceChannels(ChannelPolicy policy)
{
    m_spreadChannelPolicy = policy;
}

void
MeshHelper::SetNumberOfInterfaces(uint32_t nInterfaces)
{
    m_nInterfaces = nInterfaces;
}

NetDeviceContainer
MeshHelper::Install(const WifiPhyHelper& phyHelper, NodeContainer c) const
{
    NetDeviceContainer devices;
    NS_ASSERT(m_stack);
    for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        Ptr<Node> node = *i;
        // Create a mesh point device
        Ptr<MeshPointDevice> mp = CreateObject<MeshPointDevice>();
        node->AddDevice(mp);
        // Create wifi interfaces (single interface by default)
        for (uint32_t i = 0; i < m_nInterfaces; ++i)
        {
            uint32_t channel = 0;
            if (m_spreadChannelPolicy == ZERO_CHANNEL)
            {
                channel = 100;
            }
            if (m_spreadChannelPolicy == SPREAD_CHANNELS)
            {
                channel = 100 + i * 5;
            }
            Ptr<WifiNetDevice> iface = CreateInterface(phyHelper, node, channel);
            mp->AddInterface(iface);
        }
        if (!m_stack->InstallStack(mp))
        {
            NS_FATAL_ERROR("Stack is not installed!");
        }
        devices.Add(mp);
    }
    return devices;
}

MeshHelper
MeshHelper::Default()
{
    MeshHelper helper;
    helper.SetMacType();
    helper.SetRemoteStationManager("ns3::ArfWifiManager");
    helper.SetSpreadInterfaceChannels(SPREAD_CHANNELS);
    return helper;
}

void
MeshHelper::SetStandard(WifiStandard standard)
{
    m_standard = standard;
}

Ptr<WifiNetDevice>
MeshHelper::CreateInterface(const WifiPhyHelper& phyHelper,
                            Ptr<Node> node,
                            uint16_t channelId) const
{
    Ptr<WifiNetDevice> device = CreateObject<WifiNetDevice>();

    // this is a const method, but we need to force the correct QoS setting
    ObjectFactory macObjectFactory = m_mac;
    macObjectFactory.Set("QosSupported", BooleanValue(true)); // a mesh station is a QoS station
    std::vector<Ptr<WifiPhy>> phys = phyHelper.Create(node, device);
    NS_ABORT_IF(phys.size() != 1);
    node->AddDevice(device);
    phys[0]->ConfigureStandard(m_standard);
    device->SetPhy(phys[0]);
    Ptr<MeshWifiInterfaceMac> mac = macObjectFactory.Create<MeshWifiInterfaceMac>();
    NS_ASSERT(mac);
    mac->SetSsid(Ssid());
    mac->SetDevice(device);
    Ptr<WifiRemoteStationManager> manager = m_stationManager.Create<WifiRemoteStationManager>();
    NS_ASSERT(manager);
    device->SetRemoteStationManager(manager);
    mac->SetAddress(Mac48Address::Allocate());
    device->SetMac(mac);
    mac->SetMacQueueScheduler(CreateObject<FcfsWifiQueueScheduler>());
    mac->ConfigureStandard(m_standard);
    Ptr<FrameExchangeManager> fem = mac->GetFrameExchangeManager();
    if (fem)
    {
        Ptr<WifiProtectionManager> protectionManager = CreateObject<WifiDefaultProtectionManager>();
        protectionManager->SetWifiMac(mac);
        fem->SetProtectionManager(protectionManager);

        Ptr<WifiAckManager> ackManager = CreateObject<WifiDefaultAckManager>();
        ackManager->SetWifiMac(mac);
        fem->SetAckManager(ackManager);
    }
    mac->SwitchFrequencyChannel(channelId);

    return device;
}

void
MeshHelper::Report(const ns3::Ptr<ns3::NetDevice>& device, std::ostream& os)
{
    NS_ASSERT(m_stack);
    Ptr<MeshPointDevice> mp = device->GetObject<MeshPointDevice>();
    NS_ASSERT(mp);
    std::vector<Ptr<NetDevice>> ifaces = mp->GetInterfaces();
    os << "<MeshPointDevice time=\"" << Simulator::Now().GetSeconds() << "\" address=\""
       << Mac48Address::ConvertFrom(mp->GetAddress()) << "\">\n";
    m_stack->Report(mp, os);
    os << "</MeshPointDevice>\n";
}

void
MeshHelper::ResetStats(const ns3::Ptr<ns3::NetDevice>& device)
{
    NS_ASSERT(m_stack);
    Ptr<MeshPointDevice> mp = device->GetObject<MeshPointDevice>();
    NS_ASSERT(mp);
    m_stack->ResetStats(mp);
}

int64_t
MeshHelper::AssignStreams(NetDeviceContainer c, int64_t stream)
{
    int64_t currentStream = stream;
    Ptr<NetDevice> netDevice;
    for (NetDeviceContainer::Iterator i = c.Begin(); i != c.End(); ++i)
    {
        netDevice = (*i);
        Ptr<MeshPointDevice> mpd = DynamicCast<MeshPointDevice>(netDevice);
        Ptr<WifiNetDevice> wifi;
        Ptr<MeshWifiInterfaceMac> mac;
        if (mpd)
        {
            currentStream += mpd->AssignStreams(currentStream);
            // To access, we need the underlying WifiNetDevices
            std::vector<Ptr<NetDevice>> ifaces = mpd->GetInterfaces();
            for (std::vector<Ptr<NetDevice>>::iterator i = ifaces.begin(); i != ifaces.end(); i++)
            {
                wifi = DynamicCast<WifiNetDevice>(*i);

                // Handle any random numbers in the PHY objects.
                currentStream += wifi->GetPhy()->AssignStreams(currentStream);

                // Handle any random numbers in the station managers.
                Ptr<WifiRemoteStationManager> manager = wifi->GetRemoteStationManager();
                Ptr<MinstrelWifiManager> minstrel = DynamicCast<MinstrelWifiManager>(manager);
                if (minstrel)
                {
                    currentStream += minstrel->AssignStreams(currentStream);
                }
                // Handle any random numbers in the mesh mac and plugins
                mac = DynamicCast<MeshWifiInterfaceMac>(wifi->GetMac());
                currentStream += mac->AssignStreams(currentStream);

                PointerValue ptr;
                mac->GetAttribute("Txop", ptr);
                Ptr<Txop> txop = ptr.Get<Txop>();
                currentStream += txop->AssignStreams(currentStream);

                mac->GetAttribute("VO_Txop", ptr);
                Ptr<QosTxop> vo_txop = ptr.Get<QosTxop>();
                currentStream += vo_txop->AssignStreams(currentStream);

                mac->GetAttribute("VI_Txop", ptr);
                Ptr<QosTxop> vi_txop = ptr.Get<QosTxop>();
                currentStream += vi_txop->AssignStreams(currentStream);

                mac->GetAttribute("BE_Txop", ptr);
                Ptr<QosTxop> be_txop = ptr.Get<QosTxop>();
                currentStream += be_txop->AssignStreams(currentStream);

                mac->GetAttribute("BK_Txop", ptr);
                Ptr<QosTxop> bk_txop = ptr.Get<QosTxop>();
                currentStream += bk_txop->AssignStreams(currentStream);
            }
        }
    }
    return (currentStream - stream);
}

void
MeshHelper::EnableLogComponents()
{
    WifiHelper::EnableLogComponents();

    LogComponentEnable("MeshL2RoutingProtocol", LOG_LEVEL_ALL);
    LogComponentEnable("MeshPointDevice", LOG_LEVEL_ALL);
    LogComponentEnable("MeshWifiInterfaceMac", LOG_LEVEL_ALL);

    LogComponentEnable("Dot11sPeerManagementProtocol", LOG_LEVEL_ALL);
    LogComponentEnable("HwmpProtocol", LOG_LEVEL_ALL);
    LogComponentEnable("HwmpProtocolMac", LOG_LEVEL_ALL);
    LogComponentEnable("HwmpRtable", LOG_LEVEL_ALL);
    LogComponentEnable("PeerManagementProtocol", LOG_LEVEL_ALL);
    LogComponentEnable("PeerManagementProtocolMac", LOG_LEVEL_ALL);

    LogComponentEnable("FlameProtocol", LOG_LEVEL_ALL);
    LogComponentEnable("FlameProtocolMac", LOG_LEVEL_ALL);
    LogComponentEnable("FlameRtable", LOG_LEVEL_ALL);
}

} // namespace ns3
