/*
 * Copyright (c) 2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "mgt-action-headers.h"

#include "addba-extension.h"
#include "gcr-group-address.h"

#include "ns3/multi-link-element.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include <vector>

namespace ns3
{

WifiActionHeader::WifiActionHeader()
{
}

WifiActionHeader::~WifiActionHeader()
{
}

void
WifiActionHeader::SetAction(WifiActionHeader::CategoryValue type,
                            WifiActionHeader::ActionValue action)
{
    m_category = static_cast<uint8_t>(type);
    switch (type)
    {
    case SPECTRUM_MANAGEMENT: {
        break;
    }
    case QOS: {
        m_actionValue = static_cast<uint8_t>(action.qos);
        break;
    }
    case BLOCK_ACK: {
        m_actionValue = static_cast<uint8_t>(action.blockAck);
        break;
    }
    case PUBLIC: {
        m_actionValue = static_cast<uint8_t>(action.publicAction);
        break;
    }
    case RADIO_MEASUREMENT: {
        m_actionValue = static_cast<uint8_t>(action.radioMeasurementAction);
        break;
    }
    case MESH: {
        m_actionValue = static_cast<uint8_t>(action.meshAction);
        break;
    }
    case MULTIHOP: {
        m_actionValue = static_cast<uint8_t>(action.multihopAction);
        break;
    }
    case SELF_PROTECTED: {
        m_actionValue = static_cast<uint8_t>(action.selfProtectedAction);
        break;
    }
    case DMG: {
        m_actionValue = static_cast<uint8_t>(action.dmgAction);
        break;
    }
    case FST: {
        m_actionValue = static_cast<uint8_t>(action.fstAction);
        break;
    }
    case UNPROTECTED_DMG: {
        m_actionValue = static_cast<uint8_t>(action.unprotectedDmgAction);
        break;
    }
    case PROTECTED_EHT: {
        m_actionValue = static_cast<uint8_t>(action.protectedEhtAction);
        break;
    }
    case VENDOR_SPECIFIC_ACTION: {
        break;
    }
    }
}

WifiActionHeader::CategoryValue
WifiActionHeader::GetCategory() const
{
    switch (m_category)
    {
    case QOS:
        return QOS;
    case BLOCK_ACK:
        return BLOCK_ACK;
    case PUBLIC:
        return PUBLIC;
    case RADIO_MEASUREMENT:
        return RADIO_MEASUREMENT;
    case MESH:
        return MESH;
    case MULTIHOP:
        return MULTIHOP;
    case SELF_PROTECTED:
        return SELF_PROTECTED;
    case DMG:
        return DMG;
    case FST:
        return FST;
    case UNPROTECTED_DMG:
        return UNPROTECTED_DMG;
    case PROTECTED_EHT:
        return PROTECTED_EHT;
    case VENDOR_SPECIFIC_ACTION:
        return VENDOR_SPECIFIC_ACTION;
    default:
        NS_FATAL_ERROR("Unknown action value");
        return SELF_PROTECTED;
    }
}

WifiActionHeader::ActionValue
WifiActionHeader::GetAction() const
{
    ActionValue retval;
    retval.selfProtectedAction =
        PEER_LINK_OPEN; // Needs to be initialized to something to quiet valgrind in default cases
    switch (m_category)
    {
    case QOS:
        switch (m_actionValue)
        {
        case ADDTS_REQUEST:
            retval.qos = ADDTS_REQUEST;
            break;
        case ADDTS_RESPONSE:
            retval.qos = ADDTS_RESPONSE;
            break;
        case DELTS:
            retval.qos = DELTS;
            break;
        case SCHEDULE:
            retval.qos = SCHEDULE;
            break;
        case QOS_MAP_CONFIGURE:
            retval.qos = QOS_MAP_CONFIGURE;
            break;
        default:
            NS_FATAL_ERROR("Unknown qos action code");
            retval.qos = ADDTS_REQUEST; /* quiet compiler */
        }
        break;

    case BLOCK_ACK:
        switch (m_actionValue)
        {
        case BLOCK_ACK_ADDBA_REQUEST:
            retval.blockAck = BLOCK_ACK_ADDBA_REQUEST;
            break;
        case BLOCK_ACK_ADDBA_RESPONSE:
            retval.blockAck = BLOCK_ACK_ADDBA_RESPONSE;
            break;
        case BLOCK_ACK_DELBA:
            retval.blockAck = BLOCK_ACK_DELBA;
            break;
        default:
            NS_FATAL_ERROR("Unknown block ack action code");
            retval.blockAck = BLOCK_ACK_ADDBA_REQUEST; /* quiet compiler */
        }
        break;

    case PUBLIC:
        switch (m_actionValue)
        {
        case QAB_REQUEST:
            retval.publicAction = QAB_REQUEST;
            break;
        case QAB_RESPONSE:
            retval.publicAction = QAB_RESPONSE;
            break;
        case FILS_DISCOVERY:
            retval.publicAction = FILS_DISCOVERY;
            break;
        default:
            NS_FATAL_ERROR("Unknown public action code");
            retval.publicAction = QAB_REQUEST; /* quiet compiler */
        }
        break;

    case RADIO_MEASUREMENT:
        switch (m_actionValue)
        {
        case RADIO_MEASUREMENT_REQUEST:
            retval.radioMeasurementAction = RADIO_MEASUREMENT_REQUEST;
            break;
        case RADIO_MEASUREMENT_REPORT:
            retval.radioMeasurementAction = RADIO_MEASUREMENT_REPORT;
            break;
        case LINK_MEASUREMENT_REQUEST:
            retval.radioMeasurementAction = LINK_MEASUREMENT_REQUEST;
            break;
        case LINK_MEASUREMENT_REPORT:
            retval.radioMeasurementAction = LINK_MEASUREMENT_REPORT;
            break;
        case NEIGHBOR_REPORT_REQUEST:
            retval.radioMeasurementAction = NEIGHBOR_REPORT_REQUEST;
            break;
        case NEIGHBOR_REPORT_RESPONSE:
            retval.radioMeasurementAction = NEIGHBOR_REPORT_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown radio measurement action code");
            retval.radioMeasurementAction = RADIO_MEASUREMENT_REQUEST; /* quiet compiler */
        }
        break;

    case SELF_PROTECTED:
        switch (m_actionValue)
        {
        case PEER_LINK_OPEN:
            retval.selfProtectedAction = PEER_LINK_OPEN;
            break;
        case PEER_LINK_CONFIRM:
            retval.selfProtectedAction = PEER_LINK_CONFIRM;
            break;
        case PEER_LINK_CLOSE:
            retval.selfProtectedAction = PEER_LINK_CLOSE;
            break;
        case GROUP_KEY_INFORM:
            retval.selfProtectedAction = GROUP_KEY_INFORM;
            break;
        case GROUP_KEY_ACK:
            retval.selfProtectedAction = GROUP_KEY_ACK;
            break;
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
            retval.selfProtectedAction = PEER_LINK_OPEN; /* quiet compiler */
        }
        break;

    case MESH:
        switch (m_actionValue)
        {
        case LINK_METRIC_REPORT:
            retval.meshAction = LINK_METRIC_REPORT;
            break;
        case PATH_SELECTION:
            retval.meshAction = PATH_SELECTION;
            break;
        case PORTAL_ANNOUNCEMENT:
            retval.meshAction = PORTAL_ANNOUNCEMENT;
            break;
        case CONGESTION_CONTROL_NOTIFICATION:
            retval.meshAction = CONGESTION_CONTROL_NOTIFICATION;
            break;
        case MDA_SETUP_REQUEST:
            retval.meshAction = MDA_SETUP_REQUEST;
            break;
        case MDA_SETUP_REPLY:
            retval.meshAction = MDA_SETUP_REPLY;
            break;
        case MDAOP_ADVERTISEMENT_REQUEST:
            retval.meshAction = MDAOP_ADVERTISEMENT_REQUEST;
            break;
        case MDAOP_ADVERTISEMENTS:
            retval.meshAction = MDAOP_ADVERTISEMENTS;
            break;
        case MDAOP_SET_TEARDOWN:
            retval.meshAction = MDAOP_SET_TEARDOWN;
            break;
        case TBTT_ADJUSTMENT_REQUEST:
            retval.meshAction = TBTT_ADJUSTMENT_REQUEST;
            break;
        case TBTT_ADJUSTMENT_RESPONSE:
            retval.meshAction = TBTT_ADJUSTMENT_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
            retval.meshAction = LINK_METRIC_REPORT; /* quiet compiler */
        }
        break;

    case MULTIHOP: // not yet supported
        switch (m_actionValue)
        {
        case PROXY_UPDATE:              // not used so far
        case PROXY_UPDATE_CONFIRMATION: // not used so far
            retval.multihopAction = PROXY_UPDATE;
            break;
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
            retval.multihopAction = PROXY_UPDATE; /* quiet compiler */
        }
        break;

    case DMG:
        switch (m_actionValue)
        {
        case DMG_POWER_SAVE_CONFIGURATION_REQUEST:
            retval.dmgAction = DMG_POWER_SAVE_CONFIGURATION_REQUEST;
            break;
        case DMG_POWER_SAVE_CONFIGURATION_RESPONSE:
            retval.dmgAction = DMG_POWER_SAVE_CONFIGURATION_RESPONSE;
            break;
        case DMG_INFORMATION_REQUEST:
            retval.dmgAction = DMG_INFORMATION_REQUEST;
            break;
        case DMG_INFORMATION_RESPONSE:
            retval.dmgAction = DMG_INFORMATION_RESPONSE;
            break;
        case DMG_HANDOVER_REQUEST:
            retval.dmgAction = DMG_HANDOVER_REQUEST;
            break;
        case DMG_HANDOVER_RESPONSE:
            retval.dmgAction = DMG_HANDOVER_RESPONSE;
            break;
        case DMG_DTP_REQUEST:
            retval.dmgAction = DMG_DTP_REQUEST;
            break;
        case DMG_DTP_RESPONSE:
            retval.dmgAction = DMG_DTP_RESPONSE;
            break;
        case DMG_RELAY_SEARCH_REQUEST:
            retval.dmgAction = DMG_RELAY_SEARCH_REQUEST;
            break;
        case DMG_RELAY_SEARCH_RESPONSE:
            retval.dmgAction = DMG_RELAY_SEARCH_RESPONSE;
            break;
        case DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST:
            retval.dmgAction = DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST;
            break;
        case DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT:
            retval.dmgAction = DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT;
            break;
        case DMG_RLS_REQUEST:
            retval.dmgAction = DMG_RLS_REQUEST;
            break;
        case DMG_RLS_RESPONSE:
            retval.dmgAction = DMG_RLS_RESPONSE;
            break;
        case DMG_RLS_ANNOUNCEMENT:
            retval.dmgAction = DMG_RLS_ANNOUNCEMENT;
            break;
        case DMG_RLS_TEARDOWN:
            retval.dmgAction = DMG_RLS_TEARDOWN;
            break;
        case DMG_RELAY_ACK_REQUEST:
            retval.dmgAction = DMG_RELAY_ACK_REQUEST;
            break;
        case DMG_RELAY_ACK_RESPONSE:
            retval.dmgAction = DMG_RELAY_ACK_RESPONSE;
            break;
        case DMG_TPA_REQUEST:
            retval.dmgAction = DMG_TPA_REQUEST;
            break;
        case DMG_TPA_RESPONSE:
            retval.dmgAction = DMG_TPA_RESPONSE;
            break;
        case DMG_ROC_REQUEST:
            retval.dmgAction = DMG_ROC_REQUEST;
            break;
        case DMG_ROC_RESPONSE:
            retval.dmgAction = DMG_ROC_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown DMG management action code");
            retval.dmgAction = DMG_POWER_SAVE_CONFIGURATION_REQUEST; /* quiet compiler */
        }
        break;

    case FST:
        switch (m_actionValue)
        {
        case FST_SETUP_REQUEST:
            retval.fstAction = FST_SETUP_REQUEST;
            break;
        case FST_SETUP_RESPONSE:
            retval.fstAction = FST_SETUP_RESPONSE;
            break;
        case FST_TEAR_DOWN:
            retval.fstAction = FST_TEAR_DOWN;
            break;
        case FST_ACK_REQUEST:
            retval.fstAction = FST_ACK_REQUEST;
            break;
        case FST_ACK_RESPONSE:
            retval.fstAction = FST_ACK_RESPONSE;
            break;
        case ON_CHANNEL_TUNNEL_REQUEST:
            retval.fstAction = ON_CHANNEL_TUNNEL_REQUEST;
            break;
        default:
            NS_FATAL_ERROR("Unknown FST management action code");
            retval.fstAction = FST_SETUP_REQUEST; /* quiet compiler */
        }
        break;

    case UNPROTECTED_DMG:
        switch (m_actionValue)
        {
        case UNPROTECTED_DMG_ANNOUNCE:
            retval.unprotectedDmgAction = UNPROTECTED_DMG_ANNOUNCE;
            break;
        case UNPROTECTED_DMG_BRP:
            retval.unprotectedDmgAction = UNPROTECTED_DMG_BRP;
            break;
        case UNPROTECTED_MIMO_BF_SETUP:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_SETUP;
            break;
        case UNPROTECTED_MIMO_BF_POLL:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_POLL;
            break;
        case UNPROTECTED_MIMO_BF_FEEDBACK:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_FEEDBACK;
            break;
        case UNPROTECTED_MIMO_BF_SELECTION:
            retval.unprotectedDmgAction = UNPROTECTED_MIMO_BF_SELECTION;
            break;
        default:
            NS_FATAL_ERROR("Unknown Unprotected DMG action code");
            retval.unprotectedDmgAction = UNPROTECTED_DMG_ANNOUNCE; /* quiet compiler */
        }
        break;

    case PROTECTED_EHT:
        switch (m_actionValue)
        {
        case PROTECTED_EHT_TID_TO_LINK_MAPPING_REQUEST:
            retval.protectedEhtAction = PROTECTED_EHT_TID_TO_LINK_MAPPING_REQUEST;
            break;
        case PROTECTED_EHT_TID_TO_LINK_MAPPING_RESPONSE:
            retval.protectedEhtAction = PROTECTED_EHT_TID_TO_LINK_MAPPING_RESPONSE;
            break;
        case PROTECTED_EHT_TID_TO_LINK_MAPPING_TEARDOWN:
            retval.protectedEhtAction = PROTECTED_EHT_TID_TO_LINK_MAPPING_TEARDOWN;
            break;
        case PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_REQUEST:
            retval.protectedEhtAction = PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_REQUEST;
            break;
        case PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_RESPONSE:
            retval.protectedEhtAction = PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_RESPONSE;
            break;
        case PROTECTED_EHT_EPCS_PRIORITY_ACCESS_TEARDOWN:
            retval.protectedEhtAction = PROTECTED_EHT_EPCS_PRIORITY_ACCESS_TEARDOWN;
            break;
        case PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION:
            retval.protectedEhtAction = PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION;
            break;
        case PROTECTED_EHT_LINK_RECOMMENDATION:
            retval.protectedEhtAction = PROTECTED_EHT_LINK_RECOMMENDATION;
            break;
        case PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_REQUEST:
            retval.protectedEhtAction = PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_REQUEST;
            break;
        case PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_RESPONSE:
            retval.protectedEhtAction = PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_RESPONSE;
            break;
        default:
            NS_FATAL_ERROR("Unknown Protected EHT action code");
            retval.protectedEhtAction =
                PROTECTED_EHT_TID_TO_LINK_MAPPING_REQUEST; /* quiet compiler */
        }
        break;

    default:
        NS_FATAL_ERROR("Unsupported action");
        retval.selfProtectedAction = PEER_LINK_OPEN; /* quiet compiler */
    }
    return retval;
}

TypeId
WifiActionHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiActionHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiActionHeader>();
    return tid;
}

TypeId
WifiActionHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

std::pair<WifiActionHeader::CategoryValue, WifiActionHeader::ActionValue>
WifiActionHeader::Peek(Ptr<const Packet> pkt)
{
    WifiActionHeader actionHdr;
    pkt->PeekHeader(actionHdr);
    return {actionHdr.GetCategory(), actionHdr.GetAction()};
}

std::pair<WifiActionHeader::CategoryValue, WifiActionHeader::ActionValue>
WifiActionHeader::Remove(Ptr<Packet> pkt)
{
    WifiActionHeader actionHdr;
    pkt->RemoveHeader(actionHdr);
    return {actionHdr.GetCategory(), actionHdr.GetAction()};
}

void
WifiActionHeader::Print(std::ostream& os) const
{
#define CASE_ACTION_VALUE(x)                                                                       \
    case x:                                                                                        \
        os << #x << "]";                                                                           \
        break;

    switch (m_category)
    {
    case QOS:
        os << "QOS[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(ADDTS_REQUEST);
            CASE_ACTION_VALUE(ADDTS_RESPONSE);
            CASE_ACTION_VALUE(DELTS);
            CASE_ACTION_VALUE(SCHEDULE);
            CASE_ACTION_VALUE(QOS_MAP_CONFIGURE);
        default:
            NS_FATAL_ERROR("Unknown qos action code");
        }
        break;
    case BLOCK_ACK:
        os << "BLOCK_ACK[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(BLOCK_ACK_ADDBA_REQUEST);
            CASE_ACTION_VALUE(BLOCK_ACK_ADDBA_RESPONSE);
            CASE_ACTION_VALUE(BLOCK_ACK_DELBA);
        default:
            NS_FATAL_ERROR("Unknown block ack action code");
        }
        break;
    case PUBLIC:
        os << "PUBLIC[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(QAB_REQUEST);
            CASE_ACTION_VALUE(QAB_RESPONSE);
            CASE_ACTION_VALUE(FILS_DISCOVERY);
        default:
            NS_FATAL_ERROR("Unknown public action code");
        }
        break;
    case RADIO_MEASUREMENT:
        os << "RADIO_MEASUREMENT[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(RADIO_MEASUREMENT_REQUEST);
            CASE_ACTION_VALUE(RADIO_MEASUREMENT_REPORT);
            CASE_ACTION_VALUE(LINK_MEASUREMENT_REQUEST);
            CASE_ACTION_VALUE(LINK_MEASUREMENT_REPORT);
            CASE_ACTION_VALUE(NEIGHBOR_REPORT_REQUEST);
            CASE_ACTION_VALUE(NEIGHBOR_REPORT_RESPONSE);
        default:
            NS_FATAL_ERROR("Unknown radio measurement action code");
        }
        break;
    case MESH:
        os << "MESH[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(LINK_METRIC_REPORT);
            CASE_ACTION_VALUE(PATH_SELECTION);
            CASE_ACTION_VALUE(PORTAL_ANNOUNCEMENT);
            CASE_ACTION_VALUE(CONGESTION_CONTROL_NOTIFICATION);
            CASE_ACTION_VALUE(MDA_SETUP_REQUEST);
            CASE_ACTION_VALUE(MDA_SETUP_REPLY);
            CASE_ACTION_VALUE(MDAOP_ADVERTISEMENT_REQUEST);
            CASE_ACTION_VALUE(MDAOP_ADVERTISEMENTS);
            CASE_ACTION_VALUE(MDAOP_SET_TEARDOWN);
            CASE_ACTION_VALUE(TBTT_ADJUSTMENT_REQUEST);
            CASE_ACTION_VALUE(TBTT_ADJUSTMENT_RESPONSE);
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
        }
        break;
    case MULTIHOP:
        os << "MULTIHOP[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(PROXY_UPDATE);              // not used so far
            CASE_ACTION_VALUE(PROXY_UPDATE_CONFIRMATION); // not used so far
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
        }
        break;
    case SELF_PROTECTED:
        os << "SELF_PROTECTED[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(PEER_LINK_OPEN);
            CASE_ACTION_VALUE(PEER_LINK_CONFIRM);
            CASE_ACTION_VALUE(PEER_LINK_CLOSE);
            CASE_ACTION_VALUE(GROUP_KEY_INFORM);
            CASE_ACTION_VALUE(GROUP_KEY_ACK);
        default:
            NS_FATAL_ERROR("Unknown mesh peering management action code");
        }
        break;
    case DMG:
        os << "DMG[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(DMG_POWER_SAVE_CONFIGURATION_REQUEST);
            CASE_ACTION_VALUE(DMG_POWER_SAVE_CONFIGURATION_RESPONSE);
            CASE_ACTION_VALUE(DMG_INFORMATION_REQUEST);
            CASE_ACTION_VALUE(DMG_INFORMATION_RESPONSE);
            CASE_ACTION_VALUE(DMG_HANDOVER_REQUEST);
            CASE_ACTION_VALUE(DMG_HANDOVER_RESPONSE);
            CASE_ACTION_VALUE(DMG_DTP_REQUEST);
            CASE_ACTION_VALUE(DMG_DTP_RESPONSE);
            CASE_ACTION_VALUE(DMG_RELAY_SEARCH_REQUEST);
            CASE_ACTION_VALUE(DMG_RELAY_SEARCH_RESPONSE);
            CASE_ACTION_VALUE(DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REQUEST);
            CASE_ACTION_VALUE(DMG_MULTI_RELAY_CHANNEL_MEASUREMENT_REPORT);
            CASE_ACTION_VALUE(DMG_RLS_REQUEST);
            CASE_ACTION_VALUE(DMG_RLS_RESPONSE);
            CASE_ACTION_VALUE(DMG_RLS_ANNOUNCEMENT);
            CASE_ACTION_VALUE(DMG_RLS_TEARDOWN);
            CASE_ACTION_VALUE(DMG_RELAY_ACK_REQUEST);
            CASE_ACTION_VALUE(DMG_RELAY_ACK_RESPONSE);
            CASE_ACTION_VALUE(DMG_TPA_REQUEST);
            CASE_ACTION_VALUE(DMG_TPA_RESPONSE);
            CASE_ACTION_VALUE(DMG_ROC_REQUEST);
            CASE_ACTION_VALUE(DMG_ROC_RESPONSE);
        default:
            NS_FATAL_ERROR("Unknown DMG management action code");
        }
        break;
    case FST:
        os << "FST[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(FST_SETUP_REQUEST);
            CASE_ACTION_VALUE(FST_SETUP_RESPONSE);
            CASE_ACTION_VALUE(FST_TEAR_DOWN);
            CASE_ACTION_VALUE(FST_ACK_REQUEST);
            CASE_ACTION_VALUE(FST_ACK_RESPONSE);
            CASE_ACTION_VALUE(ON_CHANNEL_TUNNEL_REQUEST);
        default:
            NS_FATAL_ERROR("Unknown FST management action code");
        }
        break;
    case UNPROTECTED_DMG:
        os << "UNPROTECTED_DMG[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(UNPROTECTED_DMG_ANNOUNCE);
            CASE_ACTION_VALUE(UNPROTECTED_DMG_BRP);
            CASE_ACTION_VALUE(UNPROTECTED_MIMO_BF_SETUP);
            CASE_ACTION_VALUE(UNPROTECTED_MIMO_BF_POLL);
            CASE_ACTION_VALUE(UNPROTECTED_MIMO_BF_FEEDBACK);
            CASE_ACTION_VALUE(UNPROTECTED_MIMO_BF_SELECTION);
        default:
            NS_FATAL_ERROR("Unknown Unprotected DMG action code");
        }
        break;
    case PROTECTED_EHT:
        os << "PROTECTED_EHT[";
        switch (m_actionValue)
        {
            CASE_ACTION_VALUE(PROTECTED_EHT_TID_TO_LINK_MAPPING_REQUEST);
            CASE_ACTION_VALUE(PROTECTED_EHT_TID_TO_LINK_MAPPING_RESPONSE);
            CASE_ACTION_VALUE(PROTECTED_EHT_TID_TO_LINK_MAPPING_TEARDOWN);
            CASE_ACTION_VALUE(PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_REQUEST);
            CASE_ACTION_VALUE(PROTECTED_EHT_EPCS_PRIORITY_ACCESS_ENABLE_RESPONSE);
            CASE_ACTION_VALUE(PROTECTED_EHT_EPCS_PRIORITY_ACCESS_TEARDOWN);
            CASE_ACTION_VALUE(PROTECTED_EHT_EML_OPERATING_MODE_NOTIFICATION);
            CASE_ACTION_VALUE(PROTECTED_EHT_LINK_RECOMMENDATION);
            CASE_ACTION_VALUE(PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_REQUEST);
            CASE_ACTION_VALUE(PROTECTED_EHT_MULTI_LINK_OPERATION_UPDATE_RESPONSE);
        default:
            NS_FATAL_ERROR("Unknown Protected EHT action code");
        }
        break;
    case VENDOR_SPECIFIC_ACTION:
        os << "VENDOR_SPECIFIC_ACTION";
        break;
    default:
        NS_FATAL_ERROR("Unknown action value");
    }
#undef CASE_ACTION_VALUE
}

uint32_t
WifiActionHeader::GetSerializedSize() const
{
    return 2;
}

void
WifiActionHeader::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_category);
    start.WriteU8(m_actionValue);
}

uint32_t
WifiActionHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_category = i.ReadU8();
    m_actionValue = i.ReadU8();
    return i.GetDistanceFrom(start);
}

/***************************************************
 *                 ADDBARequest
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAddBaRequestHeader);

TypeId
MgtAddBaRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAddBaRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAddBaRequestHeader>();
    return tid;
}

TypeId
MgtAddBaRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtAddBaRequestHeader::Print(std::ostream& os) const
{
    os << "A-MSDU support=" << m_amsduSupport << " Policy=" << +m_policy << " TID=" << +m_tid
       << " Buffer size=" << m_bufferSize << " Timeout=" << m_timeoutValue
       << " Starting seq=" << m_startingSeq;
    if (m_gcrGroupAddress.has_value())
    {
        os << " GCR group address=" << m_gcrGroupAddress.value();
    }
}

uint32_t
MgtAddBaRequestHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 1; // Dialog token
    size += 2; // Block ack parameter set
    size += 2; // Block ack timeout value
    size += 2; // Starting sequence control
    if (m_gcrGroupAddress)
    {
        // a GCR Group Address element has to be added
        size += GcrGroupAddress().GetSerializedSize();
    }
    if (m_bufferSize >= 1024)
    {
        // an ADDBA Extension element has to be added
        size += AddbaExtension().GetSerializedSize();
    }
    return size;
}

void
MgtAddBaRequestHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    i.WriteHtolsbU16(GetParameterSet());
    i.WriteHtolsbU16(m_timeoutValue);
    i.WriteHtolsbU16(GetStartingSequenceControl());
    if (m_gcrGroupAddress)
    {
        GcrGroupAddress gcrGroupAddr;
        gcrGroupAddr.m_gcrGroupAddress = *m_gcrGroupAddress;
        i = gcrGroupAddr.Serialize(i);
    }
    if (m_bufferSize >= 1024)
    {
        AddbaExtension addbaExt;
        addbaExt.m_extParamSet.extBufferSize = m_bufferSize / 1024;
        i = addbaExt.Serialize(i);
    }
}

uint32_t
MgtAddBaRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    SetParameterSet(i.ReadLsbtohU16());
    m_timeoutValue = i.ReadLsbtohU16();
    SetStartingSequenceControl(i.ReadLsbtohU16());
    m_gcrGroupAddress.reset();
    GcrGroupAddress gcrGroupAddr;
    auto tmp = i;
    i = gcrGroupAddr.DeserializeIfPresent(i);
    if (i.GetDistanceFrom(tmp) != 0)
    {
        m_gcrGroupAddress = gcrGroupAddr.m_gcrGroupAddress;
    }
    AddbaExtension addbaExt;
    tmp = i;
    i = addbaExt.DeserializeIfPresent(i);
    if (i.GetDistanceFrom(tmp) != 0)
    {
        // the buffer size is Extended Buffer Size × 1024 + Buffer Size
        // (Sec. 9.4.2.138 of 802.11be D4.0)
        m_bufferSize += addbaExt.m_extParamSet.extBufferSize * 1024;
    }
    return i.GetDistanceFrom(start);
}

void
MgtAddBaRequestHeader::SetDelayedBlockAck()
{
    m_policy = 0;
}

void
MgtAddBaRequestHeader::SetImmediateBlockAck()
{
    m_policy = 1;
}

void
MgtAddBaRequestHeader::SetTid(uint8_t tid)
{
    NS_ASSERT(tid < 16);
    m_tid = tid;
}

void
MgtAddBaRequestHeader::SetTimeout(uint16_t timeout)
{
    m_timeoutValue = timeout;
}

void
MgtAddBaRequestHeader::SetBufferSize(uint16_t size)
{
    m_bufferSize = size;
}

void
MgtAddBaRequestHeader::SetStartingSequence(uint16_t seq)
{
    m_startingSeq = seq;
}

void
MgtAddBaRequestHeader::SetStartingSequenceControl(uint16_t seqControl)
{
    m_startingSeq = (seqControl >> 4) & 0x0fff;
}

void
MgtAddBaRequestHeader::SetAmsduSupport(bool supported)
{
    m_amsduSupport = supported;
}

void
MgtAddBaRequestHeader::SetGcrGroupAddress(const Mac48Address& address)
{
    m_gcrGroupAddress = address;
}

uint8_t
MgtAddBaRequestHeader::GetTid() const
{
    return m_tid;
}

bool
MgtAddBaRequestHeader::IsImmediateBlockAck() const
{
    return m_policy == 1;
}

uint16_t
MgtAddBaRequestHeader::GetTimeout() const
{
    return m_timeoutValue;
}

uint16_t
MgtAddBaRequestHeader::GetBufferSize() const
{
    return m_bufferSize;
}

bool
MgtAddBaRequestHeader::IsAmsduSupported() const
{
    return m_amsduSupport;
}

uint16_t
MgtAddBaRequestHeader::GetStartingSequence() const
{
    return m_startingSeq;
}

uint16_t
MgtAddBaRequestHeader::GetStartingSequenceControl() const
{
    return (m_startingSeq << 4) & 0xfff0;
}

std::optional<Mac48Address>
MgtAddBaRequestHeader::GetGcrGroupAddress() const
{
    return m_gcrGroupAddress;
}

uint16_t
MgtAddBaRequestHeader::GetParameterSet() const
{
    uint16_t res = m_amsduSupport ? 1 : 0;
    res |= m_policy << 1;
    res |= m_tid << 2;
    res |= (m_bufferSize % 1024) << 6;
    return res;
}

void
MgtAddBaRequestHeader::SetParameterSet(uint16_t params)
{
    m_amsduSupport = ((params & 0x01) == 1);
    m_policy = (params >> 1) & 0x01;
    m_tid = (params >> 2) & 0x0f;
    m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
 *                 ADDBAResponse
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAddBaResponseHeader);

TypeId
MgtAddBaResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAddBaResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAddBaResponseHeader>();
    return tid;
}

TypeId
MgtAddBaResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtAddBaResponseHeader::Print(std::ostream& os) const
{
    os << "Status code=" << m_code << "A-MSDU support=" << m_amsduSupport << " Policy=" << +m_policy
       << " TID=" << +m_tid << " Buffer size=" << m_bufferSize << " Timeout=" << m_timeoutValue;
    if (m_gcrGroupAddress.has_value())
    {
        os << " GCR group address=" << m_gcrGroupAddress.value();
    }
}

uint32_t
MgtAddBaResponseHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 1;                          // Dialog token
    size += m_code.GetSerializedSize(); // Status code
    size += 2;                          // Block ack parameter set
    size += 2;                          // Block ack timeout value
    if (m_gcrGroupAddress)
    {
        // a GCR Group Address element has to be added
        size += GcrGroupAddress().GetSerializedSize();
    }
    if (m_bufferSize >= 1024)
    {
        // an ADDBA Extension element has to be added
        size += AddbaExtension().GetSerializedSize();
    }
    return size;
}

void
MgtAddBaResponseHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_dialogToken);
    i = m_code.Serialize(i);
    i.WriteHtolsbU16(GetParameterSet());
    i.WriteHtolsbU16(m_timeoutValue);
    if (m_gcrGroupAddress)
    {
        GcrGroupAddress gcrGroupAddr;
        gcrGroupAddr.m_gcrGroupAddress = *m_gcrGroupAddress;
        i = gcrGroupAddr.Serialize(i);
    }
    if (m_bufferSize >= 1024)
    {
        AddbaExtension addbaExt;
        addbaExt.m_extParamSet.extBufferSize = m_bufferSize / 1024;
        i = addbaExt.Serialize(i);
    }
}

uint32_t
MgtAddBaResponseHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    i = m_code.Deserialize(i);
    SetParameterSet(i.ReadLsbtohU16());
    m_timeoutValue = i.ReadLsbtohU16();
    m_gcrGroupAddress.reset();
    GcrGroupAddress gcrGroupAddr;
    auto tmp = i;
    i = gcrGroupAddr.DeserializeIfPresent(i);
    if (i.GetDistanceFrom(tmp) != 0)
    {
        m_gcrGroupAddress = gcrGroupAddr.m_gcrGroupAddress;
    }
    AddbaExtension addbaExt;
    tmp = i;
    i = addbaExt.DeserializeIfPresent(i);
    if (i.GetDistanceFrom(tmp) != 0)
    {
        // the buffer size is Extended Buffer Size × 1024 + Buffer Size
        // (Sec. 9.4.2.138 of 802.11be D4.0)
        m_bufferSize += addbaExt.m_extParamSet.extBufferSize * 1024;
    }
    return i.GetDistanceFrom(start);
}

void
MgtAddBaResponseHeader::SetDelayedBlockAck()
{
    m_policy = 0;
}

void
MgtAddBaResponseHeader::SetImmediateBlockAck()
{
    m_policy = 1;
}

void
MgtAddBaResponseHeader::SetTid(uint8_t tid)
{
    NS_ASSERT(tid < 16);
    m_tid = tid;
}

void
MgtAddBaResponseHeader::SetTimeout(uint16_t timeout)
{
    m_timeoutValue = timeout;
}

void
MgtAddBaResponseHeader::SetBufferSize(uint16_t size)
{
    m_bufferSize = size;
}

void
MgtAddBaResponseHeader::SetStatusCode(StatusCode code)
{
    m_code = code;
}

void
MgtAddBaResponseHeader::SetAmsduSupport(bool supported)
{
    m_amsduSupport = supported;
}

void
MgtAddBaResponseHeader::SetGcrGroupAddress(const Mac48Address& address)
{
    m_gcrGroupAddress = address;
}

StatusCode
MgtAddBaResponseHeader::GetStatusCode() const
{
    return m_code;
}

uint8_t
MgtAddBaResponseHeader::GetTid() const
{
    return m_tid;
}

bool
MgtAddBaResponseHeader::IsImmediateBlockAck() const
{
    return m_policy == 1;
}

uint16_t
MgtAddBaResponseHeader::GetTimeout() const
{
    return m_timeoutValue;
}

uint16_t
MgtAddBaResponseHeader::GetBufferSize() const
{
    return m_bufferSize;
}

bool
MgtAddBaResponseHeader::IsAmsduSupported() const
{
    return m_amsduSupport;
}

std::optional<Mac48Address>
MgtAddBaResponseHeader::GetGcrGroupAddress() const
{
    return m_gcrGroupAddress;
}

uint16_t
MgtAddBaResponseHeader::GetParameterSet() const
{
    uint16_t res = m_amsduSupport ? 1 : 0;
    res |= m_policy << 1;
    res |= m_tid << 2;
    res |= (m_bufferSize % 1024) << 6;
    return res;
}

void
MgtAddBaResponseHeader::SetParameterSet(uint16_t params)
{
    m_amsduSupport = ((params & 0x01) == 1);
    m_policy = (params >> 1) & 0x01;
    m_tid = (params >> 2) & 0x0f;
    m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
 *                     DelBa
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtDelBaHeader);

TypeId
MgtDelBaHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtDelBaHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtDelBaHeader>();
    return tid;
}

TypeId
MgtDelBaHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtDelBaHeader::Print(std::ostream& os) const
{
    os << "Initiator=" << m_initiator << " TID=" << +m_tid;
    if (m_gcrGroupAddress.has_value())
    {
        os << " GCR group address=" << m_gcrGroupAddress.value();
    }
}

uint32_t
MgtDelBaHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 2; // DelBa parameter set
    size += 2; // Reason code
    if (m_gcrGroupAddress)
    {
        // a GCR Group Address element has to be added
        size += GcrGroupAddress().GetSerializedSize();
    }
    return size;
}

void
MgtDelBaHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtolsbU16(GetParameterSet());
    i.WriteHtolsbU16(m_reasonCode);
    if (m_gcrGroupAddress)
    {
        GcrGroupAddress gcrGroupAddr;
        gcrGroupAddr.m_gcrGroupAddress = *m_gcrGroupAddress;
        i = gcrGroupAddr.Serialize(i);
    }
}

uint32_t
MgtDelBaHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    SetParameterSet(i.ReadLsbtohU16());
    m_reasonCode = i.ReadLsbtohU16();
    m_gcrGroupAddress.reset();
    GcrGroupAddress gcrGroupAddr;
    auto tmp = i;
    i = gcrGroupAddr.DeserializeIfPresent(i);
    if (i.GetDistanceFrom(tmp) != 0)
    {
        m_gcrGroupAddress = gcrGroupAddr.m_gcrGroupAddress;
    }
    return i.GetDistanceFrom(start);
}

bool
MgtDelBaHeader::IsByOriginator() const
{
    return m_initiator == 1;
}

uint8_t
MgtDelBaHeader::GetTid() const
{
    NS_ASSERT(m_tid < 16);
    auto tid = static_cast<uint8_t>(m_tid);
    return tid;
}

void
MgtDelBaHeader::SetByOriginator()
{
    m_initiator = 1;
}

void
MgtDelBaHeader::SetByRecipient()
{
    m_initiator = 0;
}

void
MgtDelBaHeader::SetTid(uint8_t tid)
{
    NS_ASSERT(tid < 16);
    m_tid = static_cast<uint16_t>(tid);
}

void
MgtDelBaHeader::SetGcrGroupAddress(const Mac48Address& address)
{
    m_gcrGroupAddress = address;
}

std::optional<Mac48Address>
MgtDelBaHeader::GetGcrGroupAddress() const
{
    return m_gcrGroupAddress;
}

uint16_t
MgtDelBaHeader::GetParameterSet() const
{
    uint16_t res = 0;
    res |= m_initiator << 11;
    res |= m_tid << 12;
    return res;
}

void
MgtDelBaHeader::SetParameterSet(uint16_t params)
{
    m_initiator = (params >> 11) & 0x01;
    m_tid = (params >> 12) & 0x0f;
}

/***************************************************
 *     EMLSR Operating Mode Notification
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtEmlOmn);

TypeId
MgtEmlOmn::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtEmlOperatingModeNotification")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtEmlOmn>();
    return tid;
}

TypeId
MgtEmlOmn::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtEmlOmn::Print(std::ostream& os) const
{
    os << "EMLSR Mode=" << +m_emlControl.emlsrMode << " EMLMR Mode=" << +m_emlControl.emlmrMode
       << " EMLSR Parameter Update Control=" << +m_emlControl.emlsrParamUpdateCtrl;
    if (m_emlControl.linkBitmap)
    {
        os << " Link bitmap=" << std::hex << *m_emlControl.linkBitmap << std::dec;
    }
    if (m_emlsrParamUpdate)
    {
        os << " EMLSR Padding Delay="
           << CommonInfoBasicMle::DecodeEmlsrPaddingDelay(m_emlsrParamUpdate->paddingDelay)
                  .As(Time::US)
           << " EMLSR Transition Delay="
           << CommonInfoBasicMle::DecodeEmlsrTransitionDelay(m_emlsrParamUpdate->transitionDelay)
                  .As(Time::US);
    }
}

uint32_t
MgtEmlOmn::GetSerializedSize() const
{
    uint32_t size = 2; // Dialog Token (1) + first byte of EML Control
    if (m_emlControl.linkBitmap)
    {
        size += 2;
    }
    if (m_emlControl.mcsMapCountCtrl)
    {
        size += 1;
    }
    // TODO add size of EMLMR Supported MCS And NSS Set subfield when implemented
    if (m_emlsrParamUpdate)
    {
        size += 1; // EMLSR Parameter Update field
    }
    return size;
}

void
MgtEmlOmn::Serialize(Buffer::Iterator start) const
{
    start.WriteU8(m_dialogToken);

    NS_ABORT_MSG_IF(m_emlControl.emlsrMode == 1 && m_emlControl.emlmrMode == 1,
                    "EMLSR Mode and EMLMR Mode cannot be both set to 1");
    uint8_t val = m_emlControl.emlsrMode | (m_emlControl.emlmrMode << 1) |
                  (m_emlControl.emlsrParamUpdateCtrl << 2);
    start.WriteU8(val);

    NS_ABORT_MSG_IF(m_emlControl.linkBitmap.has_value() !=
                        (m_emlControl.emlsrMode == 1 || m_emlControl.emlmrMode == 1),
                    "The EMLSR/EMLMR Link Bitmap is present if and only if either of the EMLSR "
                    "Mode and EMLMR Mode subfields are set to 1");
    if (m_emlControl.linkBitmap)
    {
        start.WriteHtolsbU16(*m_emlControl.linkBitmap);
    }
    // TODO serialize MCS Map Count Control and EMLMR Supported MCS And NSS Set subfields
    // when implemented

    NS_ABORT_MSG_IF(m_emlsrParamUpdate.has_value() != (m_emlControl.emlsrParamUpdateCtrl == 1),
                    "The EMLSR Parameter Update field is present "
                        << std::boolalpha << m_emlsrParamUpdate.has_value()
                        << " if and only if the EMLSR "
                           "Parameter Update Control subfield is set to 1 "
                        << +m_emlControl.emlsrParamUpdateCtrl);
    if (m_emlsrParamUpdate)
    {
        val = m_emlsrParamUpdate->paddingDelay | (m_emlsrParamUpdate->transitionDelay << 3);
        start.WriteU8(val);
    }
}

uint32_t
MgtEmlOmn::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_dialogToken = i.ReadU8();

    uint8_t val = i.ReadU8();
    m_emlControl.emlsrMode = val & 0x01;
    m_emlControl.emlmrMode = (val >> 1) & 0x01;
    m_emlControl.emlsrParamUpdateCtrl = (val >> 2) & 0x01;

    NS_ABORT_MSG_IF(m_emlControl.emlsrMode == 1 && m_emlControl.emlmrMode == 1,
                    "EMLSR Mode and EMLMR Mode cannot be both set to 1");

    if (m_emlControl.emlsrMode == 1 || m_emlControl.emlmrMode == 1)
    {
        m_emlControl.linkBitmap = i.ReadLsbtohU16();
    }
    // TODO deserialize MCS Map Count Control and EMLMR Supported MCS And NSS Set subfields
    // when implemented

    if (m_emlControl.emlsrParamUpdateCtrl == 1)
    {
        val = i.ReadU8();
        m_emlsrParamUpdate = EmlsrParamUpdate{};
        m_emlsrParamUpdate->paddingDelay = val & 0x07;
        m_emlsrParamUpdate->transitionDelay = (val >> 3) & 0x07;
    }

    return i.GetDistanceFrom(start);
}

void
MgtEmlOmn::SetLinkIdInBitmap(uint8_t linkId)
{
    NS_ABORT_MSG_IF(linkId > 15, "Link ID must not exceed 15");
    if (!m_emlControl.linkBitmap)
    {
        m_emlControl.linkBitmap = 0;
    }
    m_emlControl.linkBitmap = *m_emlControl.linkBitmap | (1 << linkId);
}

std::list<uint8_t>
MgtEmlOmn::GetLinkBitmap() const
{
    std::list<uint8_t> list;
    NS_ASSERT_MSG(m_emlControl.linkBitmap.has_value(), "No link bitmap");
    uint16_t bitmap = *m_emlControl.linkBitmap;
    for (uint8_t linkId = 0; linkId < 16; linkId++)
    {
        if ((bitmap & 0x0001) == 1)
        {
            list.push_back(linkId);
        }
        bitmap >>= 1;
    }
    return list;
}

/***************************************************
 *                 FILS Discovery
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(FilsDiscHeader);

TypeId
FilsDiscHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::FilsDiscHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<FilsDiscHeader>();
    return tid;
}

TypeId
FilsDiscHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

FilsDiscHeader::FilsDiscHeader()
    : m_len(m_frameCtl.m_lenPresenceInd),
      m_fdCap(m_frameCtl.m_capPresenceInd),
      m_primaryCh(m_frameCtl.m_primChPresenceInd),
      m_apConfigSeqNum(m_frameCtl.m_apCsnPresenceInd),
      m_accessNetOpt(m_frameCtl.m_anoPresenceInd),
      m_chCntrFreqSeg1(m_frameCtl.m_chCntrFreqSeg1PresenceInd)
{
}

void
FilsDiscHeader::SetSsid(const std::string& ssid)
{
    m_ssid = ssid;
    m_frameCtl.m_ssidLen = ssid.length() - 1;
}

const std::string&
FilsDiscHeader::GetSsid() const
{
    return m_ssid;
}

uint32_t
FilsDiscHeader::GetInformationFieldSize() const
{
    auto size = GetSizeNonOptSubfields();
    size += m_len.has_value() ? 1 : 0;
    size += m_fdCap.has_value() ? 2 : 0;
    size += m_opClass.has_value() ? 1 : 0;
    size += m_primaryCh.has_value() ? 1 : 0;
    size += m_apConfigSeqNum.has_value() ? 1 : 0;
    size += m_accessNetOpt.has_value() ? 1 : 0;
    size += m_chCntrFreqSeg1.has_value() ? 1 : 0;
    return size;
}

uint32_t
FilsDiscHeader::GetSerializedSize() const
{
    auto size = GetInformationFieldSize();
    // Optional elements
    size += m_rnr.has_value() ? m_rnr->GetSerializedSize() : 0;
    size += m_tim.has_value() ? m_tim->GetSerializedSize() : 0;
    return size;
}

uint32_t
FilsDiscHeader::GetSizeNonOptSubfields() const
{
    return 2                  /* FILS Discovery Frame Control */
           + 8                /* Timestamp */
           + 2                /* Beacon Interval */
           + m_ssid.length(); /* SSID */
}

void
FilsDiscHeader::SetLengthSubfield()
{
    m_len.reset(); // so that Length size is not included by GetInformationFieldSize()
    auto infoFieldSize = GetInformationFieldSize();
    auto nonOptSubfieldsSize = GetSizeNonOptSubfields();
    NS_ABORT_MSG_IF(infoFieldSize < nonOptSubfieldsSize, "Length subfield is less than 0");
    m_len = infoFieldSize - nonOptSubfieldsSize;
}

void
FilsDiscHeader::Print(std::ostream& os) const
{
    os << "Control=" << m_frameCtl << ", "
       << "Time Stamp=" << m_timeStamp << ", "
       << "Beacon Interval=" << m_beaconInt << ", "
       << "SSID=" << m_ssid << ", ";
    if (m_len.has_value())
    {
        os << "Length=" << *m_len << ", ";
    }
    if (m_fdCap.has_value())
    {
        os << "FD Capability=" << *m_fdCap << ", ";
    }
    if (m_opClass.has_value())
    {
        os << "Operating Class=" << *m_opClass << ", ";
    }
    if (m_primaryCh.has_value())
    {
        os << "Primary Channel=" << *m_primaryCh << ", ";
    }
    if (m_apConfigSeqNum.has_value())
    {
        os << "AP-CSN=" << *m_apConfigSeqNum << ", ";
    }
    if (m_accessNetOpt.has_value())
    {
        os << "ANO=" << *m_accessNetOpt << ", ";
    }
    if (m_chCntrFreqSeg1.has_value())
    {
        os << "Channel Center Frequency Seg 1=" << *m_chCntrFreqSeg1 << ", ";
    }
    if (m_tim.has_value())
    {
        os << "Traffic Indicator Map=" << *m_tim;
    }
}

void
FilsDiscHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    m_frameCtl.Serialize(i);
    i.WriteHtolsbU64(Simulator::Now().GetMicroSeconds()); // Time stamp
    i.WriteHtolsbU16(m_beaconInt);
    i.Write(reinterpret_cast<const uint8_t*>(m_ssid.data()), m_ssid.length());
    if (m_len.has_value())
    {
        i.WriteU8(*m_len);
    }
    if (m_fdCap.has_value())
    {
        m_fdCap->Serialize(i);
    }
    NS_ASSERT(m_opClass.has_value() == m_primaryCh.has_value());
    if (m_opClass.has_value())
    {
        i.WriteU8(*m_opClass);
    }
    if (m_primaryCh.has_value())
    {
        i.WriteU8(*m_primaryCh);
    }
    if (m_apConfigSeqNum.has_value())
    {
        i.WriteU8(*m_apConfigSeqNum);
    }
    if (m_accessNetOpt.has_value())
    {
        i.WriteU8(*m_accessNetOpt);
    }
    if (m_chCntrFreqSeg1.has_value())
    {
        i.WriteU8(*m_chCntrFreqSeg1);
    }
    i = m_rnr.has_value() ? m_rnr->Serialize(i) : i;
    i = m_tim.has_value() ? m_tim->Serialize(i) : i;
}

uint32_t
FilsDiscHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    auto nOctets = m_frameCtl.Deserialize(i);
    i.Next(nOctets);
    m_timeStamp = i.ReadLsbtohU64();
    m_beaconInt = i.ReadLsbtohU16();
    std::vector<uint8_t> ssid(m_frameCtl.m_ssidLen + 2);
    i.Read(ssid.data(), m_frameCtl.m_ssidLen + 1);
    ssid[m_frameCtl.m_ssidLen + 1] = 0;
    m_ssid = std::string(reinterpret_cast<char*>(ssid.data()));
    // Optional subfields
    if (m_frameCtl.m_lenPresenceInd)
    {
        m_len = i.ReadU8();
    }
    if (m_frameCtl.m_capPresenceInd)
    {
        nOctets = m_fdCap->Deserialize(i);
        i.Next(nOctets);
    }
    if (m_frameCtl.m_primChPresenceInd)
    {
        m_opClass = i.ReadU8();
        m_primaryCh = i.ReadU8();
    }
    if (m_frameCtl.m_apCsnPresenceInd)
    {
        m_apConfigSeqNum = i.ReadU8();
    }
    if (m_frameCtl.m_anoPresenceInd)
    {
        m_accessNetOpt = i.ReadU8();
    }
    if (m_frameCtl.m_chCntrFreqSeg1PresenceInd)
    {
        m_chCntrFreqSeg1 = i.ReadU8();
    }
    // Optional elements
    m_rnr.emplace();
    auto tmp = i;
    i = m_rnr->DeserializeIfPresent(i);
    if (i.GetDistanceFrom(tmp) == 0)
    {
        m_rnr.reset();
    }

    m_tim.emplace();
    tmp = i;
    i = m_tim->DeserializeIfPresent(i);
    if (i.GetDistanceFrom(tmp) == 0)
    {
        m_tim.reset();
    }

    return i.GetDistanceFrom(start);
}

std::ostream&
operator<<(std::ostream& os, const FilsDiscHeader::FilsDiscFrameControl& control)
{
    os << "ssidLen:" << control.m_ssidLen << " capPresenceInd:" << control.m_capPresenceInd
       << " shortSsidInd:" << control.m_shortSsidInd
       << " apCsnPresenceInd:" << control.m_apCsnPresenceInd
       << " anoPresenceInd:" << control.m_anoPresenceInd
       << " chCntrFreqSeg1PresenceInd:" << control.m_chCntrFreqSeg1PresenceInd
       << " primChPresenceInd:" << control.m_primChPresenceInd
       << " rsnInfoPresenceInd:" << control.m_rsnInfoPresenceInd
       << " lenPresenceInd:" << control.m_lenPresenceInd
       << " mdPresenceInd:" << control.m_mdPresenceInd;
    return os;
}

void
FilsDiscHeader::FilsDiscFrameControl::Serialize(Buffer::Iterator& start) const
{
    uint16_t val = m_ssidLen | ((m_capPresenceInd ? 1 : 0) << 5) | (m_shortSsidInd << 6) |
                   ((m_apCsnPresenceInd ? 1 : 0) << 7) | ((m_anoPresenceInd ? 1 : 0) << 8) |
                   ((m_chCntrFreqSeg1PresenceInd ? 1 : 0) << 9) |
                   ((m_primChPresenceInd ? 1 : 0) << 10) | (m_rsnInfoPresenceInd << 11) |
                   ((m_lenPresenceInd ? 1 : 0) << 12) | (m_mdPresenceInd << 13);
    start.WriteHtolsbU16(val);
}

uint32_t
FilsDiscHeader::FilsDiscFrameControl::Deserialize(Buffer::Iterator start)
{
    auto val = start.ReadLsbtohU16();

    m_ssidLen = val & 0x001f;
    m_capPresenceInd = ((val >> 5) & 0x0001) == 1;
    m_shortSsidInd = (val >> 6) & 0x0001;
    m_apCsnPresenceInd = ((val >> 7) & 0x0001) == 1;
    m_anoPresenceInd = ((val >> 8) & 0x0001) == 1;
    m_chCntrFreqSeg1PresenceInd = ((val >> 9) & 0x0001) == 1;
    m_primChPresenceInd = ((val >> 10) & 0x0001) == 1;
    m_rsnInfoPresenceInd = (val >> 11) & 0x0001;
    m_lenPresenceInd = ((val >> 12) & 0x0001) == 1;
    m_mdPresenceInd = (val >> 13) & 0x0001;

    return 2;
}

std::ostream&
operator<<(std::ostream& os, const FilsDiscHeader::FdCapability& capability)
{
    os << "ess:" << capability.m_ess << " privacy:" << capability.m_privacy
       << " channelWidth:" << capability.m_chWidth << " maxNss:" << capability.m_maxNss
       << " multiBssidInd:" << capability.m_multiBssidPresenceInd
       << " phyIdx:" << capability.m_phyIdx << " minRate:" << capability.m_minRate;
    return os;
}

void
FilsDiscHeader::FdCapability::Serialize(Buffer::Iterator& start) const
{
    uint16_t val = m_ess | (m_privacy << 1) | (m_chWidth << 2) | (m_maxNss << 5) |
                   (m_multiBssidPresenceInd << 9) | (m_phyIdx << 10) | (m_minRate << 13);
    start.WriteHtolsbU16(val);
}

uint32_t
FilsDiscHeader::FdCapability::Deserialize(Buffer::Iterator start)
{
    auto val = start.ReadLsbtohU16();

    m_ess = val & 0x0001;
    m_privacy = (val >> 1) & 0x0001;
    m_chWidth = (val >> 2) & 0x0007;
    m_maxNss = (val >> 5) & 0x0007;
    m_multiBssidPresenceInd = (val >> 9) & 0x0001;
    m_phyIdx = (val >> 10) & 0x0007;
    m_minRate = (val >> 13) & 0x0007;

    return 2;
}

void
FilsDiscHeader::FdCapability::SetOpChannelWidth(MHz_u width)
{
    m_chWidth = (width == MHz_u{20} || width == MHz_u{22}) ? 0
                : (width == MHz_u{40})                     ? 1
                : (width == MHz_u{80})                     ? 2
                : (width == MHz_u{160})                    ? 3
                                                           : 4;
}

MHz_u
FilsDiscHeader::FdCapability::GetOpChannelWidth() const
{
    switch (m_chWidth)
    {
    case 0:
        return m_phyIdx == 0 ? MHz_u{22} : MHz_u{20}; // PHY Index 0 indicates 802.11b
    case 1:
        return MHz_u{40};
    case 2:
        return MHz_u{80};
    case 3:
        return MHz_u{160};
    default:
        NS_ABORT_MSG("Reserved value: " << +m_chWidth);
    }
    return MHz_u{0};
}

void
FilsDiscHeader::FdCapability::SetMaxNss(uint8_t maxNss)
{
    NS_ABORT_MSG_IF(maxNss < 1, "NSS is equal to 0");
    maxNss--;
    // 4 is the maximum value for the Maximum Number of Spatial Streams subfield
    m_maxNss = std::min<uint8_t>(maxNss, 4);
}

uint8_t
FilsDiscHeader::FdCapability::GetMaxNss() const
{
    return m_maxNss + 1;
}

void
FilsDiscHeader::FdCapability::SetStandard(WifiStandard standard)
{
    switch (standard)
    {
    case WIFI_STANDARD_80211b:
        m_phyIdx = 0;
        break;
    case WIFI_STANDARD_80211a:
    case WIFI_STANDARD_80211g:
        m_phyIdx = 1;
        break;
    case WIFI_STANDARD_80211n:
        m_phyIdx = 2;
        break;
    case WIFI_STANDARD_80211ac:
        m_phyIdx = 3;
        break;
    case WIFI_STANDARD_80211ax:
        m_phyIdx = 4;
        break;
    case WIFI_STANDARD_80211be:
        m_phyIdx = 5;
        break;
    default:
        NS_ABORT_MSG("Unsupported standard: " << standard);
    }
}

WifiStandard
FilsDiscHeader::FdCapability::GetStandard(WifiPhyBand band) const
{
    switch (m_phyIdx)
    {
    case 0:
        return WIFI_STANDARD_80211b;
    case 1:
        NS_ABORT_MSG_IF(band != WIFI_PHY_BAND_2_4GHZ && band != WIFI_PHY_BAND_5GHZ,
                        "Invalid PHY band (" << band << ") with PHY index of 1");
        return band == WIFI_PHY_BAND_5GHZ ? WIFI_STANDARD_80211a : WIFI_STANDARD_80211g;
    case 2:
        return WIFI_STANDARD_80211n;
    case 3:
        return WIFI_STANDARD_80211ac;
    case 4:
        return WIFI_STANDARD_80211ax;
    case 5:
        return WIFI_STANDARD_80211be;
    default:
        NS_ABORT_MSG("Invalid PHY index: " << m_phyIdx);
    }

    return WIFI_STANDARD_UNSPECIFIED;
}

} // namespace ns3
