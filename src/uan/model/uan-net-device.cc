/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#include "uan-net-device.h"

#include "uan-channel.h"
#include "uan-mac.h"
#include "uan-phy.h"
#include "uan-transducer.h"

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/pointer.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/traced-callback.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("UanNetDevice");

NS_OBJECT_ENSURE_REGISTERED(UanNetDevice);

UanNetDevice::UanNetDevice()
    : NetDevice(),
      m_mtu(64000),
      m_cleared(false)
{
}

UanNetDevice::~UanNetDevice()
{
}

void
UanNetDevice::Clear()
{
    if (m_cleared)
    {
        return;
    }
    m_cleared = true;
    m_node = nullptr;
    if (m_channel)
    {
        m_channel->Clear();
        m_channel = nullptr;
    }
    if (m_mac)
    {
        m_mac->Clear();
        m_mac = nullptr;
    }
    if (m_phy)
    {
        m_phy->Clear();
        m_phy = nullptr;
    }
    if (m_trans)
    {
        m_trans->Clear();
        m_trans = nullptr;
    }
}

void
UanNetDevice::DoInitialize()
{
    m_phy->Initialize();
    m_mac->Initialize();
    m_channel->Initialize();
    m_trans->Initialize();

    NetDevice::DoInitialize();
}

void
UanNetDevice::DoDispose()
{
    Clear();
    NetDevice::DoDispose();
}

TypeId
UanNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::UanNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("Uan")
            .AddAttribute(
                "Channel",
                "The channel attached to this device.",
                PointerValue(),
                MakePointerAccessor(&UanNetDevice::DoGetChannel, &UanNetDevice::SetChannel),
                MakePointerChecker<UanChannel>())
            .AddAttribute("Phy",
                          "The PHY layer attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&UanNetDevice::GetPhy, &UanNetDevice::SetPhy),
                          MakePointerChecker<UanPhy>())
            .AddAttribute("Mac",
                          "The MAC layer attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&UanNetDevice::GetMac, &UanNetDevice::SetMac),
                          MakePointerChecker<UanMac>())
            .AddAttribute(
                "Transducer",
                "The Transducer attached to this device.",
                PointerValue(),
                MakePointerAccessor(&UanNetDevice::GetTransducer, &UanNetDevice::SetTransducer),
                MakePointerChecker<UanTransducer>())
            .AddTraceSource("Rx",
                            "Received payload from the MAC layer.",
                            MakeTraceSourceAccessor(&UanNetDevice::m_rxLogger),
                            "ns3::UanNetDevice::RxTxTracedCallback")
            .AddTraceSource("Tx",
                            "Send payload to the MAC layer.",
                            MakeTraceSourceAccessor(&UanNetDevice::m_txLogger),
                            "ns3::UanNetDevice::RxTxTracedCallback");
    return tid;
}

void
UanNetDevice::SetMac(Ptr<UanMac> mac)
{
    if (mac)
    {
        m_mac = mac;
        NS_LOG_DEBUG("Set MAC");

        if (m_phy)
        {
            m_phy->SetMac(mac);
            m_mac->AttachPhy(m_phy);
            NS_LOG_DEBUG("Attached MAC to PHY");
        }
        m_mac->SetForwardUpCb(MakeCallback(&UanNetDevice::ForwardUp, this));
    }
}

void
UanNetDevice::SetPhy(Ptr<UanPhy> phy)
{
    if (phy)
    {
        m_phy = phy;
        m_phy->SetDevice(Ptr<UanNetDevice>(this));
        NS_LOG_DEBUG("Set PHY");
        if (m_mac)
        {
            m_mac->AttachPhy(phy);
            m_phy->SetMac(m_mac);
            NS_LOG_DEBUG("Attached PHY to MAC");
        }
        if (m_trans)
        {
            m_phy->SetTransducer(m_trans);
            NS_LOG_DEBUG("Added PHY to trans");
        }
    }
}

void
UanNetDevice::SetChannel(Ptr<UanChannel> channel)
{
    if (channel)
    {
        m_channel = channel;
        NS_LOG_DEBUG("Set CHANNEL");
        if (m_trans)
        {
            m_channel->AddDevice(this, m_trans);
            NS_LOG_DEBUG("Added self to channel device list");
            m_trans->SetChannel(m_channel);
            NS_LOG_DEBUG("Set Transducer channel");
        }
        if (m_phy)
        {
            m_phy->SetChannel(channel);
        }
    }
}

Ptr<UanChannel>
UanNetDevice::DoGetChannel() const
{
    return m_channel;
}

Ptr<UanMac>
UanNetDevice::GetMac() const
{
    return m_mac;
}

Ptr<UanPhy>
UanNetDevice::GetPhy() const
{
    return m_phy;
}

void
UanNetDevice::SetIfIndex(uint32_t index)
{
    m_ifIndex = index;
}

uint32_t
UanNetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
UanNetDevice::GetChannel() const
{
    return m_channel;
}

Address
UanNetDevice::GetAddress() const
{
    return m_mac->GetAddress();
}

bool
UanNetDevice::SetMtu(uint16_t mtu)
{
    /// @todo  Check this in MAC
    NS_LOG_WARN("UanNetDevice:  MTU is not implemented");
    m_mtu = mtu;
    return true;
}

uint16_t
UanNetDevice::GetMtu() const
{
    return m_mtu;
}

bool
UanNetDevice::IsLinkUp() const
{
    return (m_linkup && m_phy);
}

bool
UanNetDevice::IsBroadcast() const
{
    return true;
}

Address
UanNetDevice::GetBroadcast() const
{
    return m_mac->GetBroadcast();
}

bool
UanNetDevice::IsMulticast() const
{
    return true;
}

Address
UanNetDevice::GetMulticast(Ipv4Address /* multicastGroup */) const
{
    return m_mac->GetBroadcast();
}

Address
UanNetDevice::GetMulticast(Ipv6Address addr) const
{
    return m_mac->GetBroadcast();
}

bool
UanNetDevice::IsBridge() const
{
    return false;
}

bool
UanNetDevice::IsPointToPoint() const
{
    return false;
}

bool
UanNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    uint8_t tmp[6];
    dest.CopyTo(tmp);
    Mac8Address udest(tmp[0]);

    return m_mac->Enqueue(packet, protocolNumber, udest);
}

bool
UanNetDevice::SendFrom(Ptr<Packet> /* packet */,
                       const Address& /* source */,
                       const Address& /* dest */,
                       uint16_t /* protocolNumber */)
{
    // Not yet implemented
    NS_ASSERT_MSG(false, "Not yet implemented");
    return false;
}

Ptr<Node>
UanNetDevice::GetNode() const
{
    return m_node;
}

void
UanNetDevice::SetNode(Ptr<Node> node)
{
    m_node = node;
}

bool
UanNetDevice::NeedsArp() const
{
    return true;
}

void
UanNetDevice::SetReceiveCallback(NetDevice::ReceiveCallback cb)
{
    m_forwardUp = cb;
}

void
UanNetDevice::ForwardUp(Ptr<Packet> pkt, uint16_t protocolNumber, const Mac8Address& src)
{
    NS_LOG_DEBUG("Forwarding packet up to application");
    m_rxLogger(pkt, src);
    m_forwardUp(this, pkt, protocolNumber, src);
}

Ptr<UanTransducer>
UanNetDevice::GetTransducer() const
{
    return m_trans;
}

void
UanNetDevice::SetTransducer(Ptr<UanTransducer> trans)
{
    if (trans)
    {
        m_trans = trans;
        NS_LOG_DEBUG("Set Transducer");
        if (m_phy)
        {
            m_phy->SetTransducer(m_trans);
            NS_LOG_DEBUG("Attached Phy to transducer");
        }

        if (m_channel)
        {
            m_channel->AddDevice(this, m_trans);
            m_trans->SetChannel(m_channel);
            NS_LOG_DEBUG("Added self to channel device list");
        }
    }
}

void
UanNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    m_linkChanges.ConnectWithoutContext(callback);
}

void
UanNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback cb)
{
    // Not implemented yet
    NS_ASSERT_MSG(0, "Not yet implemented");
}

bool
UanNetDevice::SupportsSendFrom() const
{
    return false;
}

void
UanNetDevice::SetAddress(Address address)
{
    NS_ASSERT_MSG(m_mac, "Tried to set MAC address with no MAC");
    m_mac->SetAddress(Mac8Address::ConvertFrom(address));
}

void
UanNetDevice::SetSleepMode(bool sleep)
{
    m_phy->SetSleepMode(sleep);
}

void
UanNetDevice::SetTxModeIndex(uint32_t txModeIndex)
{
    m_mac->SetTxModeIndex(txModeIndex);
}

uint32_t
UanNetDevice::GetTxModeIndex()
{
    return m_mac->GetTxModeIndex();
}

} // namespace ns3
