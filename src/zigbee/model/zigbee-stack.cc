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
    m_aps = CreateObject<zigbee::ZigbeeAps>();
    m_zdo = CreateObject<zigbee::ZigbeeZdo>();
    m_groupTable = Create<zigbee::ZigbeeGroupTable>();

    // Default to the full stack. SetLayers can change this before initialization;
    // DoInitialize wires the selected layers and drops the unused ones.
    m_layers = StackLayers::FULL_STACK;
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
    m_zdo = nullptr;
    m_aps = nullptr;
    m_nwk = nullptr;
    m_groupTable = nullptr;
    m_mac = nullptr;
    Object::DoDispose();
}

void
ZigbeeStack::DoInitialize()
{
    NS_LOG_FUNCTION(this);

    NS_ABORT_MSG_UNLESS(m_netDevice,
                        "Invalid NetDevice found when attempting to install ZigbeeStack");

    // Make sure the NetDevice is previously initialized
    // before using ZigbeeStack (PHY and MAC are initialized)
    m_netDevice->Initialize();

    m_mac = m_netDevice->GetObject<lrwpan::LrWpanMacBase>();
    NS_ABORT_MSG_UNLESS(m_mac,
                        "Invalid LrWpanMacBase found in this NetDevice, cannot use ZigbeeStack");

    m_nwk->Initialize();
    AggregateObject(m_nwk);

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

    m_nwk->SetGroupTable(m_groupTable);

    if (m_layers >= StackLayers::NWK_AND_APS)
    {
        // Set APS callback hooks with NWK (i.e., NLDE primitives only)
        m_nwk->SetNldeDataConfirmCallback(MakeCallback(&ZigbeeAps::NldeDataConfirm, m_aps));
        m_nwk->SetNldeDataIndicationCallback(MakeCallback(&ZigbeeAps::NldeDataIndication, m_aps));

        // Connect the APS layer to the same group table used by the NWK layer
        m_aps->SetGroupTable(m_groupTable);

        m_aps->SetNwk(m_nwk);
    }
    else
    {
        // APS is not part of the selected stack: drop it so no callbacks are wired.
        m_aps = nullptr;
    }

    if (m_layers >= StackLayers::FULL_STACK)
    {
        // Connect the APS to the demultiplexer (dispatcher) to deliver the APSDE-DATA.indication
        // to either the Zigbee Device Object (ZDO) or the Application Framework (AF).
        m_aps->SetApsdeDataIndicationCallback(
            MakeCallback(&ZigbeeStack::ApsDataIndicationDispatcher, this));

        // ZDO has references to both the APS and NWK layers, so it can send requests to both layers
        m_zdo->SetAps(m_aps);
        m_zdo->SetNwk(m_nwk);

        m_zdo->Initialize();
    }
    else
    {
        // ZDO is not part of the selected stack: drop it so no callbacks are wired.
        m_zdo = nullptr;
    }

    // Obtain Extended address as soon as NWK is set to begin operations
    m_mac->MlmeGetRequest(MacPibAttributeIdentifier::macExtendedAddress);

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

void
ZigbeeStack::SetLayers(StackLayers layers)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_IF(ZigbeeStack::IsInitialized(),
                    "Zigbee layers cannot be set after initialization");
    m_layers = layers;
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

Ptr<zigbee::ZigbeeAps>
ZigbeeStack::GetAps() const
{
    return m_aps;
}

void
ZigbeeStack::SetAps(Ptr<zigbee::ZigbeeAps> aps)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_IF(ZigbeeStack::IsInitialized(), "APS layer cannot be set after initialization");
    m_aps = aps;
}

Ptr<zigbee::ZigbeeZdo>
ZigbeeStack::GetZdo() const
{
    return m_zdo;
}

void
ZigbeeStack::SetZdo(Ptr<zigbee::ZigbeeZdo> zdo)
{
    NS_LOG_FUNCTION(this);
    NS_ABORT_MSG_IF(ZigbeeStack::IsInitialized(), "ZDO layer cannot be set after initialization");
    m_zdo = zdo;
}

void
ZigbeeStack::ApsDataIndicationDispatcher(ApsdeDataIndicationParams params, Ptr<Packet> p)
{
    if (params.m_dstEndPoint == 0)
    {
        // Zigbee Device Object (ZDO), Endpoint 0.
        if (m_zdo)
        {
            NS_LOG_DEBUG("ZigbeeStack: APSDE-DATA.indication directed to ZDO (endpoint 0)");
            m_zdo->ApsDataIndication(params, p);
        }
        else
        {
            NS_LOG_DEBUG("ZigbeeStack: APSDE-DATA.indication directed to ZDO (endpoint 0) "
                         "dropped, the ZDO layer is not present in this stack");
        }
    }
    else
    {
        // Application Framework, Endpoints 1~254
        // TODO:
        // NS_LOG_DEBUG("ZigbeeStack: APSDE-DATA.indication directed to
        //               Application Framework (endpoint "
        //                << params.dstEndpoint << ")");
        // m_af->ApsDataIndication(params, p);
    }
}

} // namespace zigbee
} // namespace ns3
