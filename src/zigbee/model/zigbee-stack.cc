/*
 * Copyright (c) 2024 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-stack.h"

#include "ns3/channel.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/packet.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

using namespace ns3::lrwpan;

namespace ns3
{
namespace zigbee
{
NS_LOG_COMPONENT_DEFINE("ZigbeeStack");
NS_OBJECT_ENSURE_REGISTERED(ZigbeeStack);

TypeId
ZigbeeStack::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeeStack")
                            .SetParent<Object>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeeStack>();
    return tid;
}

ZigbeeStack::ZigbeeStack()
{
    NS_LOG_FUNCTION(this);

    m_nwk = CreateObject<zigbee::ZigbeeNwk>();
    // TODO: Create  APS layer here.
    //  m_aps = CreateObject<zigbee::ZigbeeAps> ();
}

ZigbeeStack::~ZigbeeStack()
{
    NS_LOG_FUNCTION(this);
}

void
ZigbeeStack::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_netDevice = nullptr;
    m_node = nullptr;
    m_nwk = nullptr;
    m_mac = nullptr;
    Object::DoDispose();
}

void
ZigbeeStack::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    AggregateObject(m_nwk);

    NS_ABORT_MSG_UNLESS(m_netDevice,
                        "Invalid NetDevice found when attempting to install ZigbeeStack");

    // Make sure the NetDevice is previously initialized
    // before using ZigbeeStack
    m_netDevice->Initialize();

    m_mac = m_netDevice->GetObject<lrwpan::LrWpanMacBase>();
    NS_ABORT_MSG_UNLESS(m_mac,
                        "No valid LrWpanMacBase found in this NetDevice, cannot use ZigbeeStack");

    // Set NWK callback hooks with the MAC
    m_nwk->SetMac(m_mac);
    m_mac->SetMcpsDataIndicationCallback(MakeCallback(&ZigbeeNwk::McpsDataIndication, m_nwk));
    m_mac->SetMlmeOrphanIndicationCallback(MakeCallback(&ZigbeeNwk::MlmeOrphanIndication, m_nwk));
    m_mac->SetMlmeCommStatusIndicationCallback(
        MakeCallback(&ZigbeeNwk::MlmeCommStatusIndication, m_nwk));
    m_mac->SetMlmeBeaconNotifyIndicationCallback(
        MakeCallback(&ZigbeeNwk::MlmeBeaconNotifyIndication, m_nwk));
    m_mac->SetMlmeAssociateIndicationCallback(
        MakeCallback(&ZigbeeNwk::MlmeAssociateIndication, m_nwk));
    m_mac->SetMcpsDataConfirmCallback(MakeCallback(&ZigbeeNwk::McpsDataConfirm, m_nwk));
    m_mac->SetMlmeScanConfirmCallback(MakeCallback(&ZigbeeNwk::MlmeScanConfirm, m_nwk));
    m_mac->SetMlmeStartConfirmCallback(MakeCallback(&ZigbeeNwk::MlmeStartConfirm, m_nwk));
    m_mac->SetMlmeSetConfirmCallback(MakeCallback(&ZigbeeNwk::MlmeSetConfirm, m_nwk));
    m_mac->SetMlmeGetConfirmCallback(MakeCallback(&ZigbeeNwk::MlmeGetConfirm, m_nwk));
    m_mac->SetMlmeAssociateConfirmCallback(MakeCallback(&ZigbeeNwk::MlmeAssociateConfirm, m_nwk));
    // TODO: complete other callback hooks with the MAC

    // Obtain Extended address as soon as NWK is set to begin operations
    m_mac->MlmeGetRequest(MacPibAttributeIdentifier::macExtendedAddress);

    // TODO: Set APS callback hooks with NWK when support for APS layer is added.
    //       For example:
    // m_aps->SetNwk (m_nwk);
    // m_nwk->SetNldeDataIndicationCallback (MakeCallback (&ZigbeeAps::NldeDataIndication, m_aps));

    Object::DoInitialize();
}

Ptr<Channel>
ZigbeeStack::GetChannel() const
{
    return m_netDevice->GetChannel();
}

Ptr<Node>
ZigbeeStack::GetNode() const
{
    return m_node;
}

Ptr<NetDevice>
ZigbeeStack::GetNetDevice() const
{
    return m_netDevice;
}

void
ZigbeeStack::SetNetDevice(Ptr<NetDevice> netDevice)
{
    NS_LOG_FUNCTION(this << netDevice);
    m_netDevice = netDevice;
    m_node = m_netDevice->GetNode();
}

Ptr<zigbee::ZigbeeNwk>
ZigbeeStack::GetNwk() const
{
    return m_nwk;
}

void
ZigbeeStack::SetNwk(Ptr<zigbee::ZigbeeNwk> nwk)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_IF(ZigbeeStack::IsInitialized(), "NWK layer cannot be set after initialization");
    m_nwk = nwk;
}

} // namespace zigbee
} // namespace ns3
