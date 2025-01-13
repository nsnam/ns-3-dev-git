/*
 * Copyright (c) 2011 The Boeing Company
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Tommaso Pecorella <tommaso.pecorella@unifi.it>
 *  Margherita Filippetti <morag87@gmail.com>
 */
#include "lr-wpan-net-device.h"

#include "lr-wpan-csmaca.h"
#include "lr-wpan-error-model.h"
#include "lr-wpan-phy.h"

#include "ns3/abort.h"
#include "ns3/boolean.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/spectrum-channel.h"

namespace ns3
{
namespace lrwpan
{

NS_LOG_COMPONENT_DEFINE("LrWpanNetDevice");
NS_OBJECT_ENSURE_REGISTERED(LrWpanNetDevice);

TypeId
LrWpanNetDevice::GetTypeId()
{
    static TypeId tid =
        TypeId("ns3::lrwpan::LrWpanNetDevice")
            .AddDeprecatedName("ns3::LrWpanNetDevice")
            .SetParent<NetDevice>()
            .SetGroupName("LrWpan")
            .AddConstructor<LrWpanNetDevice>()
            .AddAttribute("Channel",
                          "The channel attached to this device",
                          PointerValue(),
                          MakePointerAccessor(&LrWpanNetDevice::DoGetChannel),
                          MakePointerChecker<SpectrumChannel>())
            .AddAttribute("Phy",
                          "The PHY layer attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&LrWpanNetDevice::GetPhy, &LrWpanNetDevice::SetPhy),
                          MakePointerChecker<LrWpanPhy>())
            .AddAttribute("Mac",
                          "The MAC layer attached to this device.",
                          PointerValue(),
                          MakePointerAccessor(&LrWpanNetDevice::GetMac, &LrWpanNetDevice::SetMac),
                          MakePointerChecker<LrWpanMac>())
            .AddAttribute("UseAcks",
                          "Request acknowledgments for data frames.",
                          BooleanValue(true),
                          MakeBooleanAccessor(&LrWpanNetDevice::m_useAcks),
                          MakeBooleanChecker())
            .AddAttribute(
                "PseudoMacAddressMode",
                "Build the pseudo-MAC Address according to RFC 4944 or RFC 6282 "
                "(default: RFC 6282).",
                EnumValue(LrWpanNetDevice::RFC6282),
                MakeEnumAccessor<PseudoMacAddressMode_e>(&LrWpanNetDevice::m_pseudoMacMode),
                MakeEnumChecker(LrWpanNetDevice::RFC6282,
                                "RFC 6282 (don't use PanId)",
                                LrWpanNetDevice::RFC4944,
                                "RFC 4944 (use PanId)"));
    return tid;
}

LrWpanNetDevice::LrWpanNetDevice()
    : m_configComplete(false)
{
    NS_LOG_FUNCTION(this);

    m_phy = CreateObject<LrWpanPhy>();
    m_csmaca = CreateObject<LrWpanCsmaCa>();
    m_mac = CreateObject<LrWpanMac>();
}

LrWpanNetDevice::~LrWpanNetDevice()
{
    NS_LOG_FUNCTION(this);
}

void
LrWpanNetDevice::DoDispose()
{
    NS_LOG_FUNCTION(this);
    m_phy->Dispose();
    m_csmaca->Dispose();
    m_phy = nullptr;
    m_mac = nullptr;
    m_csmaca = nullptr;
    m_node = nullptr;
    // chain up.
    NetDevice::DoDispose();
}

void
LrWpanNetDevice::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    m_phy->Initialize();
    AggregateObject(m_mac);
    CompleteConfig();

    NetDevice::DoInitialize();
}

void
LrWpanNetDevice::CompleteConfig()
{
    if (!m_mac || !m_phy || !m_csmaca || !m_node || m_configComplete)
    {
        return;
    }

    NS_LOG_FUNCTION(this);
    m_mac->SetPhy(m_phy);
    m_mac->SetCsmaCa(m_csmaca);
    m_mac->SetMcpsDataIndicationCallback(MakeCallback(&LrWpanNetDevice::McpsDataIndication, this));
    m_csmaca->SetMac(m_mac);

    Ptr<LrWpanErrorModel> model = CreateObject<LrWpanErrorModel>();
    m_phy->SetErrorModel(model);
    m_phy->SetDevice(this);

    m_phy->SetPdDataIndicationCallback(MakeCallback(&LrWpanMac::PdDataIndication, m_mac));
    m_phy->SetPdDataConfirmCallback(MakeCallback(&LrWpanMac::PdDataConfirm, m_mac));
    m_phy->SetPlmeEdConfirmCallback(MakeCallback(&LrWpanMac::PlmeEdConfirm, m_mac));
    m_phy->SetPlmeGetAttributeConfirmCallback(
        MakeCallback(&LrWpanMac::PlmeGetAttributeConfirm, m_mac));
    m_phy->SetPlmeSetTRXStateConfirmCallback(
        MakeCallback(&LrWpanMac::PlmeSetTRXStateConfirm, m_mac));
    m_phy->SetPlmeSetAttributeConfirmCallback(
        MakeCallback(&LrWpanMac::PlmeSetAttributeConfirm, m_mac));

    m_csmaca->SetLrWpanMacStateCallback(MakeCallback(&LrWpanMac::SetLrWpanMacState, m_mac));
    m_phy->SetPlmeCcaConfirmCallback(MakeCallback(&LrWpanCsmaCa::PlmeCcaConfirm, m_csmaca));
    m_configComplete = true;
}

void
LrWpanNetDevice::SetMac(Ptr<LrWpanMac> mac)
{
    NS_ABORT_MSG_IF(LrWpanNetDevice::IsInitialized(),
                    "MAC layer cannot be set after initialization");
    m_mac = mac;
}

void
LrWpanNetDevice::SetPhy(Ptr<LrWpanPhy> phy)
{
    NS_ABORT_MSG_IF(LrWpanNetDevice::IsInitialized(),
                    "PHY layer cannot be set after initialization");
    m_phy = phy;
}

void
LrWpanNetDevice::SetCsmaCa(Ptr<LrWpanCsmaCa> csmaca)
{
    NS_ABORT_MSG_IF(LrWpanNetDevice::IsInitialized(), "CSMA/CA cannot be set after initialization");
    m_csmaca = csmaca;
}

void
LrWpanNetDevice::SetChannel(Ptr<SpectrumChannel> channel)
{
    NS_ABORT_MSG_IF(LrWpanNetDevice::IsInitialized(),
                    "Spectrum channel cannot be set after initialization");
    m_phy->SetChannel(channel);
    channel->AddRx(m_phy);
}

Ptr<LrWpanMac>
LrWpanNetDevice::GetMac() const
{
    return m_mac;
}

Ptr<LrWpanPhy>
LrWpanNetDevice::GetPhy() const
{
    return m_phy;
}

Ptr<LrWpanCsmaCa>
LrWpanNetDevice::GetCsmaCa() const
{
    return m_csmaca;
}

void
LrWpanNetDevice::SetIfIndex(const uint32_t index)
{
    m_ifIndex = index;
}

uint32_t
LrWpanNetDevice::GetIfIndex() const
{
    return m_ifIndex;
}

Ptr<Channel>
LrWpanNetDevice::GetChannel() const
{
    return m_phy->GetChannel();
}

void
LrWpanNetDevice::LinkUp()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = true;
    m_linkChanges();
}

void
LrWpanNetDevice::LinkDown()
{
    NS_LOG_FUNCTION(this);
    m_linkUp = false;
    m_linkChanges();
}

Ptr<SpectrumChannel>
LrWpanNetDevice::DoGetChannel() const
{
    NS_LOG_FUNCTION(this);
    return m_phy->GetChannel();
}

void
LrWpanNetDevice::SetAddress(Address address)
{
    NS_LOG_FUNCTION(this);
    if (Mac16Address::IsMatchingType(address))
    {
        m_mac->SetShortAddress(Mac16Address::ConvertFrom(address));
    }
    else if (Mac64Address::IsMatchingType(address))
    {
        m_mac->SetExtendedAddress(Mac64Address::ConvertFrom(address));
    }
    else if (Mac48Address::IsMatchingType(address))
    {
        uint8_t buf[6];
        Mac48Address addr = Mac48Address::ConvertFrom(address);
        addr.CopyTo(buf);
        Mac16Address addr16;
        addr16.CopyFrom(buf + 4);
        m_mac->SetShortAddress(addr16);
        uint16_t panId;
        panId = buf[0];
        panId <<= 8;
        panId |= buf[1];
        m_mac->SetPanId(panId);
    }
    else
    {
        NS_ABORT_MSG("LrWpanNetDevice::SetAddress - address is not of a compatible type");
    }
}

Address
LrWpanNetDevice::GetAddress() const
{
    NS_LOG_FUNCTION(this);

    if (m_mac->GetShortAddress() == Mac16Address("00:00"))
    {
        return m_mac->GetExtendedAddress();
    }

    Mac48Address pseudoAddress = BuildPseudoMacAddress(m_mac->GetPanId(), m_mac->GetShortAddress());

    return pseudoAddress;
}

void
LrWpanNetDevice::SetPanAssociation(uint16_t panId,
                                   Mac64Address coordExtAddr,
                                   Mac16Address coordShortAddr,
                                   Mac16Address assignedShortAddr)
{
    NS_LOG_FUNCTION(this);
    m_mac->SetPanId(panId);
    m_mac->SetAssociatedCoor(coordExtAddr);
    m_mac->SetAssociatedCoor(coordShortAddr);
    m_mac->SetShortAddress(assignedShortAddr);
}

bool
LrWpanNetDevice::SetMtu(const uint16_t mtu)
{
    NS_ABORT_MSG("Unsupported");
    return false;
}

uint16_t
LrWpanNetDevice::GetMtu() const
{
    NS_LOG_FUNCTION(this);
    // Maximum payload size is: max psdu - frame control - seqno - addressing - security - fcs
    //                        = 127      - 2             - 1     - (2+2+2+2)  - 0        - 2
    //                        = 114
    // assuming no security and addressing with only 16 bit addresses without pan id compression.
    return 114;
}

bool
LrWpanNetDevice::IsLinkUp() const
{
    NS_LOG_FUNCTION(this);
    return m_phy && m_linkUp;
}

void
LrWpanNetDevice::AddLinkChangeCallback(Callback<void> callback)
{
    NS_LOG_FUNCTION(this);
    m_linkChanges.ConnectWithoutContext(callback);
}

bool
LrWpanNetDevice::IsBroadcast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
LrWpanNetDevice::GetBroadcast() const
{
    NS_LOG_FUNCTION(this);

    Mac48Address pseudoAddress =
        BuildPseudoMacAddress(m_mac->GetPanId(), Mac16Address::GetBroadcast());

    return pseudoAddress;
}

bool
LrWpanNetDevice::IsMulticast() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

Address
LrWpanNetDevice::GetMulticast(Ipv4Address multicastGroup) const
{
    NS_ABORT_MSG("Unsupported");
    return Address();
}

Address
LrWpanNetDevice::GetMulticast(Ipv6Address addr) const
{
    NS_LOG_FUNCTION(this << addr);

    Mac48Address pseudoAddress =
        BuildPseudoMacAddress(m_mac->GetPanId(), Mac16Address::GetMulticast(addr));

    return pseudoAddress;
}

bool
LrWpanNetDevice::IsBridge() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
LrWpanNetDevice::IsPointToPoint() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

bool
LrWpanNetDevice::Send(Ptr<Packet> packet, const Address& dest, uint16_t protocolNumber)
{
    // This method basically assumes an 802.3-compliant device, but a raw
    // 802.15.4 device does not have an ethertype, and requires specific
    // McpsDataRequest parameters.
    // For further study:  how to support these methods somehow, such as
    // inventing a fake ethertype and packet tag for McpsDataRequest
    NS_LOG_FUNCTION(this << packet << dest << protocolNumber);

    if (packet->GetSize() > GetMtu())
    {
        NS_LOG_ERROR("Fragmentation is needed for this packet, drop the packet ");
        return false;
    }

    McpsDataRequestParams m_mcpsDataRequestParams;

    Mac16Address dst16;
    if (Mac48Address::IsMatchingType(dest))
    {
        uint8_t buf[6];
        dest.CopyTo(buf);
        dst16.CopyFrom(buf + 4);
    }
    else
    {
        dst16 = Mac16Address::ConvertFrom(dest);
    }
    m_mcpsDataRequestParams.m_dstAddr = dst16;
    m_mcpsDataRequestParams.m_dstAddrMode = SHORT_ADDR;
    m_mcpsDataRequestParams.m_dstPanId = m_mac->GetPanId();
    m_mcpsDataRequestParams.m_srcAddrMode = SHORT_ADDR;
    // Using ACK requests for broadcast destinations is ok here. They are disabled
    // by the MAC.
    if (m_useAcks)
    {
        m_mcpsDataRequestParams.m_txOptions = TX_OPTION_ACK;
    }
    m_mcpsDataRequestParams.m_msduHandle = 0;
    m_mac->McpsDataRequest(m_mcpsDataRequestParams, packet);
    return true;
}

bool
LrWpanNetDevice::SendFrom(Ptr<Packet> packet,
                          const Address& source,
                          const Address& dest,
                          uint16_t protocolNumber)
{
    NS_ABORT_MSG("Unsupported");
    // TODO: To support SendFrom, the MACs McpsDataRequest has to use the provided source address,
    // instead of to local one.
    return false;
}

Ptr<Node>
LrWpanNetDevice::GetNode() const
{
    return m_node;
}

void
LrWpanNetDevice::SetNode(Ptr<Node> node)
{
    m_node = node;
    CompleteConfig();
}

bool
LrWpanNetDevice::NeedsArp() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

void
LrWpanNetDevice::SetReceiveCallback(ReceiveCallback cb)
{
    NS_LOG_FUNCTION(this);
    m_receiveCallback = cb;
}

void
LrWpanNetDevice::SetPromiscReceiveCallback(PromiscReceiveCallback cb)
{
    // This method basically assumes an 802.3-compliant device, but a raw
    // 802.15.4 device does not have an ethertype, and requires specific
    // McpsDataIndication parameters.
    // For further study:  how to support these methods somehow, such as
    // inventing a fake ethertype and packet tag for McpsDataRequest
    NS_LOG_WARN("Unsupported; use LrWpan MAC APIs instead");
}

void
LrWpanNetDevice::McpsDataIndication(McpsDataIndicationParams params, Ptr<Packet> pkt)
{
    NS_LOG_FUNCTION(this);
    // TODO: Use the PromiscReceiveCallback if the MAC is in promiscuous mode.

    if (params.m_dstAddrMode == SHORT_ADDR)
    {
        m_receiveCallback(this, pkt, 0, BuildPseudoMacAddress(params.m_srcPanId, params.m_srcAddr));
    }
    else
    {
        m_receiveCallback(this, pkt, 0, params.m_srcExtAddr);
    }
}

bool
LrWpanNetDevice::SupportsSendFrom() const
{
    NS_LOG_FUNCTION_NOARGS();
    return false;
}

Mac48Address
LrWpanNetDevice::BuildPseudoMacAddress(uint16_t panId, Mac16Address shortAddr) const
{
    NS_LOG_FUNCTION(this);

    uint8_t buf[6];

    if (m_pseudoMacMode == RFC4944)
    {
        buf[0] = panId >> 8;
        // Make sure the U/L bit is set
        buf[0] |= 0x02;
        buf[1] = panId & 0xff;
    }
    else
    {
        // Make sure the U/L bit is set
        buf[0] = 0x02;
        buf[1] = 0x00;
    }
    buf[2] = 0;
    buf[3] = 0;
    shortAddr.CopyTo(buf + 4);

    Mac48Address pseudoAddress;
    pseudoAddress.CopyFrom(buf);

    return pseudoAddress;
}

int64_t
LrWpanNetDevice::AssignStreams(int64_t stream)
{
    NS_LOG_FUNCTION(stream);
    int64_t streamIndex = stream;
    streamIndex += m_csmaca->AssignStreams(stream);
    streamIndex += m_phy->AssignStreams(stream);
    NS_LOG_DEBUG("Number of assigned RV streams:  " << (streamIndex - stream));
    return (streamIndex - stream);
}

} // namespace lrwpan
} // namespace ns3
