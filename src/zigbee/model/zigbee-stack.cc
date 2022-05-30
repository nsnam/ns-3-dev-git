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
                            .SetParent<NetDevice>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeeStack>();
    return tid;
}

ZigbeeStack::ZigbeeStack()
{
    NS_LOG_FUNCTION(this);
    m_lrwpanNetDevice = nullptr;
    m_nwk = CreateObject<zigbee::ZigbeeNwk>();
    // TODO: Create  APS layer here.
    //  m_aps = CreateObject<ZigbeeAps> ();

    // Automatically calls m_nwk initialize and dispose functions
    AggregateObject(m_nwk);
}

ZigbeeStack::~ZigbeeStack()
{
    NS_LOG_FUNCTION(this);
}

void
ZigbeeStack::CompleteConfig()
{
    NS_LOG_FUNCTION(this);

    // TODO: Set APS callback hooks with NWK when support for APS layer is added.
    //       For example:
    // m_aps->SetNwk (m_nwk);
    // m_nwk->SetNldeDataIndicationCallback (MakeCallback (&ZigbeeAps::NldeDataIndication, m_aps));

    // Set NWK callback hooks with the MAC
    if (m_lrwpanNetDevice != nullptr)
    {
        m_mac = m_lrwpanNetDevice->GetMac();
    }

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
}

void
ZigbeeStack::DoDispose()
{
    NS_LOG_FUNCTION(this);

    m_lrwpanNetDevice = nullptr;
    m_node = nullptr;
    m_nwk = nullptr;
    m_mac = nullptr;
    Object::DoDispose();
}

void
ZigbeeStack::DoInitialize()
{
    NS_LOG_FUNCTION(this);
    Object::DoInitialize();
}

Ptr<Channel>
ZigbeeStack::GetChannel() const
{
    NS_LOG_FUNCTION(this);
    // TODO
    // NS_ABORT_MSG_UNLESS(m_lrwpanNetDevice != 0,
    //                   "Zigbee: can't find any lower-layer protocol " << m_lrwpanNetDevice);
    return m_lrwpanNetDevice->GetChannel();
}

Ptr<Node>
ZigbeeStack::GetNode() const
{
    NS_LOG_FUNCTION(this);
    return m_node;
}

void
ZigbeeStack::SetLrWpanNetDevice(Ptr<LrWpanNetDevice> lrwpanDevice)
{
    NS_LOG_FUNCTION(this << lrwpanDevice);
    m_lrwpanNetDevice = lrwpanDevice;
    m_node = lrwpanDevice->GetNode();
    CompleteConfig();
}

Ptr<zigbee::ZigbeeNwk>
ZigbeeStack::GetNwk() const
{
    NS_LOG_FUNCTION(this);
    return m_nwk;
}

void
ZigbeeStack::SetNwk(Ptr<zigbee::ZigbeeNwk> nwk)
{
    NS_LOG_FUNCTION(this);
    m_nwk = nwk;
    CompleteConfig();
}

} // namespace zigbee
} // namespace ns3
