/*
 * Copyright (c) 2006, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "wifi-mac-header.h"

#include "ns3/address-utils.h"
#include "ns3/nstime.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(WifiMacHeader);

/// type enumeration
enum
{
    TYPE_MGT = 0,
    TYPE_CTL = 1,
    TYPE_DATA = 2
};

/// subtype enumeration
enum
{
    // Reserved: 0 - 1
    SUBTYPE_CTL_TRIGGER = 2,
    // Reserved: 3
    SUBTYPE_CTL_BEAMFORMINGRPOLL = 4,
    SUBTYPE_CTL_NDPANNOUNCE = 5,
    SUBTYPE_CTL_CTLFRAMEEXT = 6,
    SUBTYPE_CTL_CTLWRAPPER = 7,
    SUBTYPE_CTL_BACKREQ = 8,
    SUBTYPE_CTL_BACKRESP = 9,
    SUBTYPE_CTL_PSPOLL = 10,
    SUBTYPE_CTL_RTS = 11,
    SUBTYPE_CTL_CTS = 12,
    SUBTYPE_CTL_ACK = 13,
    SUBTYPE_CTL_END = 14,
    SUBTYPE_CTL_END_ACK = 15
};

WifiMacHeader::WifiMacHeader(WifiMacType type)
    : WifiMacHeader()
{
    SetType(type);
}

void
WifiMacHeader::SetDsFrom()
{
    m_ctrlFromDs = 1;
}

void
WifiMacHeader::SetDsNotFrom()
{
    m_ctrlFromDs = 0;
}

void
WifiMacHeader::SetDsTo()
{
    m_ctrlToDs = 1;
}

void
WifiMacHeader::SetDsNotTo()
{
    m_ctrlToDs = 0;
}

void
WifiMacHeader::SetAddr1(Mac48Address address)
{
    m_addr1 = address;
}

void
WifiMacHeader::SetAddr2(Mac48Address address)
{
    m_addr2 = address;
}

void
WifiMacHeader::SetAddr3(Mac48Address address)
{
    m_addr3 = address;
}

void
WifiMacHeader::SetAddr4(Mac48Address address)
{
    m_addr4 = address;
}

void
WifiMacHeader::SetType(WifiMacType type, bool resetToDsFromDs)
{
    switch (type)
    {
    case WIFI_MAC_CTL_TRIGGER:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_TRIGGER;
        break;
    case WIFI_MAC_CTL_CTLWRAPPER:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_CTLWRAPPER;
        break;
    case WIFI_MAC_CTL_BACKREQ:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_BACKREQ;
        break;
    case WIFI_MAC_CTL_BACKRESP:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_BACKRESP;
        break;
    case WIFI_MAC_CTL_PSPOLL:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_PSPOLL;
        break;
    case WIFI_MAC_CTL_RTS:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_RTS;
        break;
    case WIFI_MAC_CTL_CTS:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_CTS;
        break;
    case WIFI_MAC_CTL_ACK:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_ACK;
        break;
    case WIFI_MAC_CTL_END:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_END;
        break;
    case WIFI_MAC_CTL_END_ACK:
        m_ctrlType = TYPE_CTL;
        m_ctrlSubtype = SUBTYPE_CTL_END_ACK;
        break;
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 0;
        break;
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 1;
        break;
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 2;
        break;
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 3;
        break;
    case WIFI_MAC_MGT_PROBE_REQUEST:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 4;
        break;
    case WIFI_MAC_MGT_PROBE_RESPONSE:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 5;
        break;
    case WIFI_MAC_MGT_BEACON:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 8;
        break;
    case WIFI_MAC_MGT_DISASSOCIATION:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 10;
        break;
    case WIFI_MAC_MGT_AUTHENTICATION:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 11;
        break;
    case WIFI_MAC_MGT_DEAUTHENTICATION:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 12;
        break;
    case WIFI_MAC_MGT_ACTION:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 13;
        break;
    case WIFI_MAC_MGT_ACTION_NO_ACK:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 14;
        break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
        m_ctrlType = TYPE_MGT;
        m_ctrlSubtype = 15;
        break;
    case WIFI_MAC_DATA:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 0;
        break;
    case WIFI_MAC_DATA_CFACK:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 1;
        break;
    case WIFI_MAC_DATA_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 2;
        break;
    case WIFI_MAC_DATA_CFACK_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 3;
        break;
    case WIFI_MAC_DATA_NULL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 4;
        break;
    case WIFI_MAC_DATA_NULL_CFACK:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 5;
        break;
    case WIFI_MAC_DATA_NULL_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 6;
        break;
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 7;
        break;
    case WIFI_MAC_QOSDATA:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 8;
        break;
    case WIFI_MAC_QOSDATA_CFACK:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 9;
        break;
    case WIFI_MAC_QOSDATA_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 10;
        break;
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 11;
        break;
    case WIFI_MAC_QOSDATA_NULL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 12;
        break;
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 14;
        break;
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
        m_ctrlType = TYPE_DATA;
        m_ctrlSubtype = 15;
        break;
    default:
        break;
    }
    if (resetToDsFromDs)
    {
        m_ctrlToDs = 0;
        m_ctrlFromDs = 0;
    }
}

void
WifiMacHeader::SetRawDuration(uint16_t duration)
{
    NS_ASSERT(duration <= 32768);
    m_duration = duration;
}

void
WifiMacHeader::SetDuration(Time duration)
{
    auto duration_us =
        static_cast<int64_t>(ceil(static_cast<double>(duration.GetNanoSeconds()) / 1000));
    NS_ASSERT(duration_us >= 0 && duration_us <= 0x7fff);
    m_duration = static_cast<uint16_t>(duration_us);
}

void
WifiMacHeader::SetId(uint16_t id)
{
    m_duration = id;
}

void
WifiMacHeader::SetSequenceNumber(uint16_t seq)
{
    m_seqSeq = seq;
}

void
WifiMacHeader::SetFragmentNumber(uint8_t frag)
{
    m_seqFrag = frag;
}

void
WifiMacHeader::SetNoMoreFragments()
{
    m_ctrlMoreFrag = 0;
}

void
WifiMacHeader::SetMoreFragments()
{
    m_ctrlMoreFrag = 1;
}

void
WifiMacHeader::SetOrder()
{
    m_ctrlOrder = 1;
}

void
WifiMacHeader::SetNoOrder()
{
    m_ctrlOrder = 0;
}

void
WifiMacHeader::SetRetry()
{
    m_ctrlRetry = 1;
}

void
WifiMacHeader::SetNoRetry()
{
    m_ctrlRetry = 0;
}

void
WifiMacHeader::SetQosTid(uint8_t tid)
{
    m_qosTid = tid;
}

void
WifiMacHeader::SetPowerManagement()
{
    m_ctrlPowerManagement = 1;
}

void
WifiMacHeader::SetNoPowerManagement()
{
    m_ctrlPowerManagement = 0;
}

void
WifiMacHeader::SetQosEosp()
{
    m_qosEosp = 1;
}

void
WifiMacHeader::SetQosNoEosp()
{
    m_qosEosp = 0;
}

void
WifiMacHeader::SetQosAckPolicy(QosAckPolicy policy)
{
    switch (policy)
    {
    case NORMAL_ACK:
        m_qosAckPolicy = 0;
        break;
    case NO_ACK:
        m_qosAckPolicy = 1;
        break;
    case NO_EXPLICIT_ACK:
        m_qosAckPolicy = 2;
        break;
    case BLOCK_ACK:
        m_qosAckPolicy = 3;
        break;
    }
}

void
WifiMacHeader::SetQosAmsdu()
{
    m_amsduPresent = 1;
}

void
WifiMacHeader::SetQosNoAmsdu()
{
    m_amsduPresent = 0;
}

void
WifiMacHeader::SetQosTxopLimit(uint8_t txop)
{
    m_qosStuff = txop;
}

void
WifiMacHeader::SetQosQueueSize(uint8_t size)
{
    m_qosEosp = 1;
    m_qosStuff = size;
}

void
WifiMacHeader::SetQosMeshControlPresent()
{
    // Mark bit 0 of this variable instead of bit 8, since m_qosStuff is
    // shifted by one byte when serialized
    m_qosStuff = m_qosStuff | 0x01; // bit 8 of QoS Control Field
}

void
WifiMacHeader::SetQosNoMeshControlPresent()
{
    // Clear bit 0 of this variable instead of bit 8, since m_qosStuff is
    // shifted by one byte when serialized
    m_qosStuff = m_qosStuff & 0xfe; // bit 8 of QoS Control Field
}

Mac48Address
WifiMacHeader::GetAddr1() const
{
    return m_addr1;
}

Mac48Address
WifiMacHeader::GetAddr2() const
{
    return m_addr2;
}

Mac48Address
WifiMacHeader::GetAddr3() const
{
    return m_addr3;
}

Mac48Address
WifiMacHeader::GetAddr4() const
{
    return m_addr4;
}

WifiMacType
WifiMacHeader::GetType() const
{
    switch (m_ctrlType)
    {
    case TYPE_MGT:
        switch (m_ctrlSubtype)
        {
        case 0:
            return WIFI_MAC_MGT_ASSOCIATION_REQUEST;
        case 1:
            return WIFI_MAC_MGT_ASSOCIATION_RESPONSE;
        case 2:
            return WIFI_MAC_MGT_REASSOCIATION_REQUEST;
        case 3:
            return WIFI_MAC_MGT_REASSOCIATION_RESPONSE;
        case 4:
            return WIFI_MAC_MGT_PROBE_REQUEST;
        case 5:
            return WIFI_MAC_MGT_PROBE_RESPONSE;
        case 8:
            return WIFI_MAC_MGT_BEACON;
        case 10:
            return WIFI_MAC_MGT_DISASSOCIATION;
        case 11:
            return WIFI_MAC_MGT_AUTHENTICATION;
        case 12:
            return WIFI_MAC_MGT_DEAUTHENTICATION;
        case 13:
            return WIFI_MAC_MGT_ACTION;
        case 14:
            return WIFI_MAC_MGT_ACTION_NO_ACK;
        case 15:
            return WIFI_MAC_MGT_MULTIHOP_ACTION;
        }
        break;
    case TYPE_CTL:
        switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_TRIGGER:
            return WIFI_MAC_CTL_TRIGGER;
        case SUBTYPE_CTL_BACKREQ:
            return WIFI_MAC_CTL_BACKREQ;
        case SUBTYPE_CTL_BACKRESP:
            return WIFI_MAC_CTL_BACKRESP;
        case SUBTYPE_CTL_PSPOLL:
            return WIFI_MAC_CTL_PSPOLL;
        case SUBTYPE_CTL_RTS:
            return WIFI_MAC_CTL_RTS;
        case SUBTYPE_CTL_CTS:
            return WIFI_MAC_CTL_CTS;
        case SUBTYPE_CTL_ACK:
            return WIFI_MAC_CTL_ACK;
        case SUBTYPE_CTL_END:
            return WIFI_MAC_CTL_END;
        case SUBTYPE_CTL_END_ACK:
            return WIFI_MAC_CTL_END_ACK;
        }
        break;
    case TYPE_DATA:
        switch (m_ctrlSubtype)
        {
        case 0:
            return WIFI_MAC_DATA;
        case 1:
            return WIFI_MAC_DATA_CFACK;
        case 2:
            return WIFI_MAC_DATA_CFPOLL;
        case 3:
            return WIFI_MAC_DATA_CFACK_CFPOLL;
        case 4:
            return WIFI_MAC_DATA_NULL;
        case 5:
            return WIFI_MAC_DATA_NULL_CFACK;
        case 6:
            return WIFI_MAC_DATA_NULL_CFPOLL;
        case 7:
            return WIFI_MAC_DATA_NULL_CFACK_CFPOLL;
        case 8:
            return WIFI_MAC_QOSDATA;
        case 9:
            return WIFI_MAC_QOSDATA_CFACK;
        case 10:
            return WIFI_MAC_QOSDATA_CFPOLL;
        case 11:
            return WIFI_MAC_QOSDATA_CFACK_CFPOLL;
        case 12:
            return WIFI_MAC_QOSDATA_NULL;
        case 14:
            return WIFI_MAC_QOSDATA_NULL_CFPOLL;
        case 15:
            return WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL;
        }
        break;
    }
    // NOTREACHED
    NS_ASSERT(false);
    return (WifiMacType)-1;
}

bool
WifiMacHeader::IsFromDs() const
{
    return m_ctrlFromDs == 1;
}

bool
WifiMacHeader::IsToDs() const
{
    return m_ctrlToDs == 1;
}

bool
WifiMacHeader::IsData() const
{
    return (m_ctrlType == TYPE_DATA);
}

bool
WifiMacHeader::IsQosData() const
{
    return (m_ctrlType == TYPE_DATA && (m_ctrlSubtype & 0x08));
}

bool
WifiMacHeader::IsCtl() const
{
    return (m_ctrlType == TYPE_CTL);
}

bool
WifiMacHeader::IsMgt() const
{
    return (m_ctrlType == TYPE_MGT);
}

bool
WifiMacHeader::IsCfPoll() const
{
    switch (GetType())
    {
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
        return true;
    default:
        return false;
    }
}

bool
WifiMacHeader::IsCfEnd() const
{
    switch (GetType())
    {
    case WIFI_MAC_CTL_END:
    case WIFI_MAC_CTL_END_ACK:
        return true;
    default:
        return false;
    }
}

bool
WifiMacHeader::IsCfAck() const
{
    switch (GetType())
    {
    case WIFI_MAC_DATA_CFACK:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_CTL_END_ACK:
        return true;
    default:
        return false;
    }
}

bool
WifiMacHeader::HasData() const
{
    switch (GetType())
    {
    case WIFI_MAC_DATA:
    case WIFI_MAC_DATA_CFACK:
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA:
    case WIFI_MAC_QOSDATA_CFACK:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
        return true;
    default:
        return false;
    }
}

bool
WifiMacHeader::IsRts() const
{
    return (GetType() == WIFI_MAC_CTL_RTS);
}

bool
WifiMacHeader::IsCts() const
{
    return (GetType() == WIFI_MAC_CTL_CTS);
}

bool
WifiMacHeader::IsPsPoll() const
{
    return (GetType() == WIFI_MAC_CTL_PSPOLL);
}

bool
WifiMacHeader::IsAck() const
{
    return (GetType() == WIFI_MAC_CTL_ACK);
}

bool
WifiMacHeader::IsAssocReq() const
{
    return (GetType() == WIFI_MAC_MGT_ASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsAssocResp() const
{
    return (GetType() == WIFI_MAC_MGT_ASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsReassocReq() const
{
    return (GetType() == WIFI_MAC_MGT_REASSOCIATION_REQUEST);
}

bool
WifiMacHeader::IsReassocResp() const
{
    return (GetType() == WIFI_MAC_MGT_REASSOCIATION_RESPONSE);
}

bool
WifiMacHeader::IsProbeReq() const
{
    return (GetType() == WIFI_MAC_MGT_PROBE_REQUEST);
}

bool
WifiMacHeader::IsProbeResp() const
{
    return (GetType() == WIFI_MAC_MGT_PROBE_RESPONSE);
}

bool
WifiMacHeader::IsBeacon() const
{
    return (GetType() == WIFI_MAC_MGT_BEACON);
}

bool
WifiMacHeader::IsDisassociation() const
{
    return (GetType() == WIFI_MAC_MGT_DISASSOCIATION);
}

bool
WifiMacHeader::IsAuthentication() const
{
    return (GetType() == WIFI_MAC_MGT_AUTHENTICATION);
}

bool
WifiMacHeader::IsDeauthentication() const
{
    return (GetType() == WIFI_MAC_MGT_DEAUTHENTICATION);
}

bool
WifiMacHeader::IsAction() const
{
    return (GetType() == WIFI_MAC_MGT_ACTION);
}

bool
WifiMacHeader::IsActionNoAck() const
{
    return (GetType() == WIFI_MAC_MGT_ACTION_NO_ACK);
}

bool
WifiMacHeader::IsMultihopAction() const
{
    return (GetType() == WIFI_MAC_MGT_MULTIHOP_ACTION);
}

bool
WifiMacHeader::IsBlockAckReq() const
{
    return (GetType() == WIFI_MAC_CTL_BACKREQ);
}

bool
WifiMacHeader::IsBlockAck() const
{
    return (GetType() == WIFI_MAC_CTL_BACKRESP);
}

bool
WifiMacHeader::IsTrigger() const
{
    return (GetType() == WIFI_MAC_CTL_TRIGGER);
}

uint16_t
WifiMacHeader::GetRawDuration() const
{
    return m_duration;
}

Time
WifiMacHeader::GetDuration() const
{
    return MicroSeconds(m_duration);
}

uint16_t
WifiMacHeader::GetSequenceControl() const
{
    return (m_seqSeq << 4) | m_seqFrag;
}

uint16_t
WifiMacHeader::GetSequenceNumber() const
{
    return m_seqSeq;
}

uint8_t
WifiMacHeader::GetFragmentNumber() const
{
    return m_seqFrag;
}

bool
WifiMacHeader::IsRetry() const
{
    return (m_ctrlRetry == 1);
}

bool
WifiMacHeader::IsMoreData() const
{
    return (m_ctrlMoreData == 1);
}

bool
WifiMacHeader::IsMoreFragments() const
{
    return (m_ctrlMoreFrag == 1);
}

bool
WifiMacHeader::IsPowerManagement() const
{
    return (m_ctrlPowerManagement == 1);
}

bool
WifiMacHeader::IsQosBlockAck() const
{
    return (IsQosData() && m_qosAckPolicy == 3);
}

bool
WifiMacHeader::IsQosNoAck() const
{
    return (IsQosData() && m_qosAckPolicy == 1);
}

bool
WifiMacHeader::IsQosAck() const
{
    return (IsQosData() && m_qosAckPolicy == 0);
}

bool
WifiMacHeader::IsQosEosp() const
{
    return (IsQosData() && m_qosEosp == 1);
}

WifiMacHeader::QosAckPolicy
WifiMacHeader::GetQosAckPolicy() const
{
    NS_ASSERT(IsQosData());
    QosAckPolicy policy;

    switch (m_qosAckPolicy)
    {
    case 0:
        policy = NORMAL_ACK;
        break;
    case 1:
        policy = NO_ACK;
        break;
    case 2:
        policy = NO_EXPLICIT_ACK;
        break;
    case 3:
        policy = BLOCK_ACK;
        break;
    default:
        NS_ABORT_MSG("Unknown QoS Ack policy");
    }
    return policy;
}

bool
WifiMacHeader::IsQosAmsdu() const
{
    return (IsQosData() && m_amsduPresent == 1);
}

uint8_t
WifiMacHeader::GetQosTid() const
{
    NS_ASSERT(IsQosData());
    return m_qosTid;
}

uint8_t
WifiMacHeader::GetQosQueueSize() const
{
    NS_ASSERT(m_qosEosp == 1);
    return m_qosStuff;
}

uint16_t
WifiMacHeader::GetFrameControl() const
{
    uint16_t val = 0;
    val |= (m_ctrlType << 2) & (0x3 << 2);
    val |= (m_ctrlSubtype << 4) & (0xf << 4);
    val |= (m_ctrlToDs << 8) & (0x1 << 8);
    val |= (m_ctrlFromDs << 9) & (0x1 << 9);
    val |= (m_ctrlMoreFrag << 10) & (0x1 << 10);
    val |= (m_ctrlRetry << 11) & (0x1 << 11);
    val |= (m_ctrlPowerManagement << 12) & (0x1 << 12);
    val |= (m_ctrlMoreData << 13) & (0x1 << 13);
    val |= (m_ctrlWep << 14) & (0x1 << 14);
    val |= (m_ctrlOrder << 15) & (0x1 << 15);
    return val;
}

uint16_t
WifiMacHeader::GetQosControl() const
{
    uint16_t val = 0;
    val |= m_qosTid;
    val |= m_qosEosp << 4;
    val |= m_qosAckPolicy << 5;
    val |= m_amsduPresent << 7;
    val |= m_qosStuff << 8;
    return val;
}

void
WifiMacHeader::SetFrameControl(uint16_t ctrl)
{
    m_ctrlType = (ctrl >> 2) & 0x03;
    m_ctrlSubtype = (ctrl >> 4) & 0x0f;
    m_ctrlToDs = (ctrl >> 8) & 0x01;
    m_ctrlFromDs = (ctrl >> 9) & 0x01;
    m_ctrlMoreFrag = (ctrl >> 10) & 0x01;
    m_ctrlRetry = (ctrl >> 11) & 0x01;
    m_ctrlPowerManagement = (ctrl >> 12) & 0x01;
    m_ctrlMoreData = (ctrl >> 13) & 0x01;
    m_ctrlWep = (ctrl >> 14) & 0x01;
    m_ctrlOrder = (ctrl >> 15) & 0x01;
}

void
WifiMacHeader::SetSequenceControl(uint16_t seq)
{
    m_seqFrag = seq & 0x0f;
    m_seqSeq = (seq >> 4) & 0x0fff;
}

void
WifiMacHeader::SetQosControl(uint16_t qos)
{
    m_qosTid = qos & 0x000f;
    m_qosEosp = (qos >> 4) & 0x0001;
    m_qosAckPolicy = (qos >> 5) & 0x0003;
    m_amsduPresent = (qos >> 7) & 0x0001;
    m_qosStuff = (qos >> 8) & 0x00ff;
}

uint32_t
WifiMacHeader::GetSize() const
{
    uint32_t size = 0;
    switch (m_ctrlType)
    {
    case TYPE_MGT:
        size = 2 + 2 + 6 + 6 + 6 + 2;
        break;
    case TYPE_CTL:
        switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_PSPOLL:
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
        case SUBTYPE_CTL_TRIGGER:
        case SUBTYPE_CTL_END:
        case SUBTYPE_CTL_END_ACK:
            size = 2 + 2 + 6 + 6;
            break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
            size = 2 + 2 + 6;
            break;
        case SUBTYPE_CTL_CTLWRAPPER:
            size = 2 + 2 + 6 + 2 + 4;
            break;
        }
        break;
    case TYPE_DATA:
        size = 2 + 2 + 6 + 6 + 6 + 2;
        if (m_ctrlToDs && m_ctrlFromDs)
        {
            size += 6;
        }
        if (m_ctrlSubtype & 0x08)
        {
            size += 2;
        }
        break;
    }
    return size;
}

const char*
WifiMacHeader::GetTypeString() const
{
#define CASE_WIFI_MAC_TYPE(x)                                                                      \
    case WIFI_MAC_##x:                                                                             \
        return #x;

    switch (GetType())
    {
        CASE_WIFI_MAC_TYPE(CTL_RTS);
        CASE_WIFI_MAC_TYPE(CTL_CTS);
        CASE_WIFI_MAC_TYPE(CTL_ACK);
        CASE_WIFI_MAC_TYPE(CTL_BACKREQ);
        CASE_WIFI_MAC_TYPE(CTL_BACKRESP);
        CASE_WIFI_MAC_TYPE(CTL_END);
        CASE_WIFI_MAC_TYPE(CTL_END_ACK);
        CASE_WIFI_MAC_TYPE(CTL_PSPOLL);
        CASE_WIFI_MAC_TYPE(CTL_TRIGGER);

        CASE_WIFI_MAC_TYPE(MGT_BEACON);
        CASE_WIFI_MAC_TYPE(MGT_ASSOCIATION_REQUEST);
        CASE_WIFI_MAC_TYPE(MGT_ASSOCIATION_RESPONSE);
        CASE_WIFI_MAC_TYPE(MGT_DISASSOCIATION);
        CASE_WIFI_MAC_TYPE(MGT_REASSOCIATION_REQUEST);
        CASE_WIFI_MAC_TYPE(MGT_REASSOCIATION_RESPONSE);
        CASE_WIFI_MAC_TYPE(MGT_PROBE_REQUEST);
        CASE_WIFI_MAC_TYPE(MGT_PROBE_RESPONSE);
        CASE_WIFI_MAC_TYPE(MGT_AUTHENTICATION);
        CASE_WIFI_MAC_TYPE(MGT_DEAUTHENTICATION);
        CASE_WIFI_MAC_TYPE(MGT_ACTION);
        CASE_WIFI_MAC_TYPE(MGT_ACTION_NO_ACK);
        CASE_WIFI_MAC_TYPE(MGT_MULTIHOP_ACTION);

        CASE_WIFI_MAC_TYPE(DATA);
        CASE_WIFI_MAC_TYPE(DATA_CFACK);
        CASE_WIFI_MAC_TYPE(DATA_CFPOLL);
        CASE_WIFI_MAC_TYPE(DATA_CFACK_CFPOLL);
        CASE_WIFI_MAC_TYPE(DATA_NULL);
        CASE_WIFI_MAC_TYPE(DATA_NULL_CFACK);
        CASE_WIFI_MAC_TYPE(DATA_NULL_CFPOLL);
        CASE_WIFI_MAC_TYPE(DATA_NULL_CFACK_CFPOLL);
        CASE_WIFI_MAC_TYPE(QOSDATA);
        CASE_WIFI_MAC_TYPE(QOSDATA_CFACK);
        CASE_WIFI_MAC_TYPE(QOSDATA_CFPOLL);
        CASE_WIFI_MAC_TYPE(QOSDATA_CFACK_CFPOLL);
        CASE_WIFI_MAC_TYPE(QOSDATA_NULL);
        CASE_WIFI_MAC_TYPE(QOSDATA_NULL_CFPOLL);
        CASE_WIFI_MAC_TYPE(QOSDATA_NULL_CFACK_CFPOLL);
    default:
        return "ERROR";
    }
#undef CASE_WIFI_MAC_TYPE
#ifndef _WIN32
    // needed to make gcc 4.0.1 ppc darwin happy.
    return "BIG_ERROR";
#endif
}

TypeId
WifiMacHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::WifiMacHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<WifiMacHeader>();
    return tid;
}

TypeId
WifiMacHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
WifiMacHeader::PrintFrameControl(std::ostream& os) const
{
    os << "ToDS=" << std::hex << static_cast<int>(m_ctrlToDs)
       << ", FromDS=" << static_cast<int>(m_ctrlFromDs)
       << ", MoreFrag=" << static_cast<int>(m_ctrlMoreFrag)
       << ", Retry=" << static_cast<int>(m_ctrlRetry)
       << ", PowerManagement=" << static_cast<int>(m_ctrlPowerManagement)
       << ", MoreData=" << static_cast<int>(m_ctrlMoreData) << std::dec;
}

void
WifiMacHeader::Print(std::ostream& os) const
{
    os << GetTypeString() << " ";
    switch (GetType())
    {
    case WIFI_MAC_CTL_PSPOLL:
        os << "Duration/ID=" << std::hex << m_duration << std::dec << ", BSSID(RA)=" << m_addr1
           << ", TA=" << m_addr2;
        break;
    case WIFI_MAC_CTL_RTS:
    case WIFI_MAC_CTL_TRIGGER:
        os << "Duration/ID=" << m_duration << "us"
           << ", RA=" << m_addr1 << ", TA=" << m_addr2;
        break;
    case WIFI_MAC_CTL_CTS:
    case WIFI_MAC_CTL_ACK:
        os << "Duration/ID=" << m_duration << "us"
           << ", RA=" << m_addr1;
        break;
    case WIFI_MAC_MGT_BEACON:
    case WIFI_MAC_MGT_ASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_ASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_DISASSOCIATION:
    case WIFI_MAC_MGT_REASSOCIATION_REQUEST:
    case WIFI_MAC_MGT_REASSOCIATION_RESPONSE:
    case WIFI_MAC_MGT_PROBE_REQUEST:
    case WIFI_MAC_MGT_PROBE_RESPONSE:
    case WIFI_MAC_MGT_AUTHENTICATION:
    case WIFI_MAC_MGT_DEAUTHENTICATION:
    case WIFI_MAC_MGT_ACTION:
    case WIFI_MAC_MGT_ACTION_NO_ACK:
        PrintFrameControl(os);
        os << " Duration/ID=" << m_duration << "us"
           << ", DA=" << m_addr1 << ", SA=" << m_addr2 << ", BSSID=" << m_addr3
           << ", FragNumber=" << std::hex << (int)m_seqFrag << std::dec
           << ", SeqNumber=" << m_seqSeq;
        break;
    case WIFI_MAC_MGT_MULTIHOP_ACTION:
        os << " Duration/ID=" << m_duration << "us"
           << ", RA=" << m_addr1 << ", TA=" << m_addr2 << ", DA=" << m_addr3
           << ", FragNumber=" << std::hex << (int)m_seqFrag << std::dec
           << ", SeqNumber=" << m_seqSeq;
        break;
    case WIFI_MAC_DATA:
    case WIFI_MAC_QOSDATA:
        PrintFrameControl(os);
        os << " Duration/ID=" << m_duration << "us";
        if (!m_ctrlToDs && !m_ctrlFromDs)
        {
            os << ", DA(RA)=" << m_addr1 << ", SA(TA)=" << m_addr2 << ", BSSID=" << m_addr3;
        }
        else if (!m_ctrlToDs && m_ctrlFromDs)
        {
            os << ", DA(RA)=" << m_addr1 << ", SA=" << m_addr3 << ", BSSID(TA)=" << m_addr2;
        }
        else if (m_ctrlToDs && !m_ctrlFromDs)
        {
            os << ", DA=" << m_addr3 << ", SA(TA)=" << m_addr2 << ", BSSID(RA)=" << m_addr1;
        }
        else if (m_ctrlToDs && m_ctrlFromDs)
        {
            os << ", DA=" << m_addr3 << ", SA=" << m_addr4 << ", RA=" << m_addr1
               << ", TA=" << m_addr2;
        }
        else
        {
            NS_FATAL_ERROR("Impossible ToDs and FromDs flags combination");
        }
        os << ", FragNumber=" << std::hex << (int)m_seqFrag << std::dec
           << ", SeqNumber=" << m_seqSeq;
        if (IsQosData())
        {
            os << ", tid=" << +m_qosTid;
            if (IsQosAmsdu())
            {
                os << ", A-MSDU";
            }
            if (IsQosNoAck())
            {
                os << ", ack=NoAck";
            }
            else if (IsQosAck())
            {
                os << ", ack=NormalAck";
            }
            else if (IsQosBlockAck())
            {
                os << ", ack=BlockAck";
            }
        }
        break;
    case WIFI_MAC_CTL_BACKREQ:
    case WIFI_MAC_CTL_BACKRESP:
    case WIFI_MAC_CTL_CTLWRAPPER:
    case WIFI_MAC_CTL_END:
    case WIFI_MAC_CTL_END_ACK:
    case WIFI_MAC_DATA_CFACK:
    case WIFI_MAC_DATA_CFPOLL:
    case WIFI_MAC_DATA_CFACK_CFPOLL:
    case WIFI_MAC_DATA_NULL:
    case WIFI_MAC_DATA_NULL_CFACK:
    case WIFI_MAC_DATA_NULL_CFPOLL:
    case WIFI_MAC_DATA_NULL_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK:
    case WIFI_MAC_QOSDATA_CFPOLL:
    case WIFI_MAC_QOSDATA_CFACK_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL:
    case WIFI_MAC_QOSDATA_NULL_CFPOLL:
    case WIFI_MAC_QOSDATA_NULL_CFACK_CFPOLL:
    default:
        break;
    }
}

uint32_t
WifiMacHeader::GetSerializedSize() const
{
    return GetSize();
}

void
WifiMacHeader::Serialize(Buffer::Iterator i) const
{
    i.WriteHtolsbU16(GetFrameControl());
    i.WriteHtolsbU16(m_duration);
    WriteTo(i, m_addr1);
    switch (m_ctrlType)
    {
    case TYPE_MGT:
        WriteTo(i, m_addr2);
        WriteTo(i, m_addr3);
        i.WriteHtolsbU16(GetSequenceControl());
        break;
    case TYPE_CTL:
        switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_PSPOLL:
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_TRIGGER:
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
        case SUBTYPE_CTL_END:
        case SUBTYPE_CTL_END_ACK:
            WriteTo(i, m_addr2);
            break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
            break;
        default:
            // NOTREACHED
            NS_ASSERT(false);
            break;
        }
        break;
    case TYPE_DATA: {
        WriteTo(i, m_addr2);
        WriteTo(i, m_addr3);
        i.WriteHtolsbU16(GetSequenceControl());
        if (m_ctrlToDs && m_ctrlFromDs)
        {
            WriteTo(i, m_addr4);
        }
        if (m_ctrlSubtype & 0x08)
        {
            i.WriteHtolsbU16(GetQosControl());
        }
    }
    break;
    default:
        // NOTREACHED
        NS_ASSERT(false);
        break;
    }
}

uint32_t
WifiMacHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint16_t frame_control = i.ReadLsbtohU16();
    SetFrameControl(frame_control);
    m_duration = i.ReadLsbtohU16();
    ReadFrom(i, m_addr1);
    switch (m_ctrlType)
    {
    case TYPE_MGT:
        ReadFrom(i, m_addr2);
        ReadFrom(i, m_addr3);
        SetSequenceControl(i.ReadLsbtohU16());
        break;
    case TYPE_CTL:
        switch (m_ctrlSubtype)
        {
        case SUBTYPE_CTL_PSPOLL:
        case SUBTYPE_CTL_RTS:
        case SUBTYPE_CTL_TRIGGER:
        case SUBTYPE_CTL_BACKREQ:
        case SUBTYPE_CTL_BACKRESP:
        case SUBTYPE_CTL_END:
        case SUBTYPE_CTL_END_ACK:
            ReadFrom(i, m_addr2);
            break;
        case SUBTYPE_CTL_CTS:
        case SUBTYPE_CTL_ACK:
            break;
        }
        break;
    case TYPE_DATA:
        ReadFrom(i, m_addr2);
        ReadFrom(i, m_addr3);
        SetSequenceControl(i.ReadLsbtohU16());
        if (m_ctrlToDs && m_ctrlFromDs)
        {
            ReadFrom(i, m_addr4);
        }
        if (m_ctrlSubtype & 0x08)
        {
            SetQosControl(i.ReadLsbtohU16());
        }
        break;
    }
    return i.GetDistanceFrom(start);
}

} // namespace ns3
