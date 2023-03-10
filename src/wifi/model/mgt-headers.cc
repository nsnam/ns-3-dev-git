/*
 * Copyright (c) 2006 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 *          Mirko Banchi <mk.banchi@gmail.com>
 */

#include "mgt-headers.h"

#include "ns3/address-utils.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

namespace ns3
{

/***********************************************************
 *          Probe Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtProbeRequestHeader);

TypeId
MgtProbeRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtProbeRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtProbeRequestHeader>();
    return tid;
}

TypeId
MgtProbeRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

/***********************************************************
 *          Probe Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtProbeResponseHeader);

TypeId
MgtProbeResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtProbeResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtProbeResponseHeader>();
    return tid;
}

TypeId
MgtProbeResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint64_t
MgtProbeResponseHeader::GetBeaconIntervalUs() const
{
    return m_beaconInterval;
}

void
MgtProbeResponseHeader::SetBeaconIntervalUs(uint64_t us)
{
    m_beaconInterval = us;
}

const CapabilityInformation&
MgtProbeResponseHeader::Capabilities() const
{
    return m_capability;
}

CapabilityInformation&
MgtProbeResponseHeader::Capabilities()
{
    return m_capability;
}

uint64_t
MgtProbeResponseHeader::GetTimestamp() const
{
    return m_timestamp;
}

uint32_t
MgtProbeResponseHeader::GetSerializedSizeImpl() const
{
    uint32_t size = 8 /* timestamp */ + 2 /* beacon interval */;
    size += m_capability.GetSerializedSize();
    size += WifiMgtHeader<MgtProbeResponseHeader, ProbeResponseElems>::GetSerializedSizeImpl();
    return size;
}

void
MgtProbeResponseHeader::SerializeImpl(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtolsbU64(Simulator::Now().GetMicroSeconds());
    i.WriteHtolsbU16(static_cast<uint16_t>(m_beaconInterval / 1024));
    i = m_capability.Serialize(i);
    WifiMgtHeader<MgtProbeResponseHeader, ProbeResponseElems>::SerializeImpl(i);
}

uint32_t
MgtProbeResponseHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_timestamp = i.ReadLsbtohU64();
    m_beaconInterval = i.ReadLsbtohU16();
    m_beaconInterval *= 1024;
    i = m_capability.Deserialize(i);
    auto distance = i.GetDistanceFrom(start);
    return distance + WifiMgtHeader<MgtProbeResponseHeader, ProbeResponseElems>::DeserializeImpl(i);
}

/***********************************************************
 *          Beacons
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtBeaconHeader);

/* static */
TypeId
MgtBeaconHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtBeaconHeader")
                            .SetParent<MgtProbeResponseHeader>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtBeaconHeader>();
    return tid;
}

/***********************************************************
 *          Assoc Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAssocRequestHeader);

TypeId
MgtAssocRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAssocRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAssocRequestHeader>();
    return tid;
}

TypeId
MgtAssocRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint16_t
MgtAssocRequestHeader::GetListenInterval() const
{
    return m_listenInterval;
}

void
MgtAssocRequestHeader::SetListenInterval(uint16_t interval)
{
    m_listenInterval = interval;
}

const CapabilityInformation&
MgtAssocRequestHeader::Capabilities() const
{
    return m_capability;
}

CapabilityInformation&
MgtAssocRequestHeader::Capabilities()
{
    return m_capability;
}

uint32_t
MgtAssocRequestHeader::GetSerializedSizeImpl() const
{
    SetMleContainingFrame();

    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += 2; // listen interval
    size += WifiMgtHeader<MgtAssocRequestHeader, AssocRequestElems>::GetSerializedSizeImpl();
    return size;
}

uint32_t
MgtAssocRequestHeader::GetSerializedSizeInPerStaProfileImpl(
    const MgtAssocRequestHeader& frame) const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size +=
        MgtHeaderInPerStaProfile<MgtAssocRequestHeader,
                                 AssocRequestElems>::GetSerializedSizeInPerStaProfileImpl(frame);
    return size;
}

void
MgtAssocRequestHeader::SerializeImpl(Buffer::Iterator start) const
{
    SetMleContainingFrame();

    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i.WriteHtolsbU16(m_listenInterval);
    WifiMgtHeader<MgtAssocRequestHeader, AssocRequestElems>::SerializeImpl(i);
}

void
MgtAssocRequestHeader::SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                                    const MgtAssocRequestHeader& frame) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    MgtHeaderInPerStaProfile<MgtAssocRequestHeader,
                             AssocRequestElems>::SerializeInPerStaProfileImpl(i, frame);
}

uint32_t
MgtAssocRequestHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = i.ReadLsbtohU16();
    auto distance = i.GetDistanceFrom(start) +
                    WifiMgtHeader<MgtAssocRequestHeader, AssocRequestElems>::DeserializeImpl(i);
    if (auto& mle = Get<MultiLinkElement>())
    {
        for (std::size_t id = 0; id < mle->GetNPerStaProfileSubelements(); id++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(id);
            if (perStaProfile.HasAssocRequest())
            {
                auto& frameInPerStaProfile =
                    std::get<std::reference_wrapper<MgtAssocRequestHeader>>(
                        perStaProfile.GetAssocRequest())
                        .get();
                frameInPerStaProfile.CopyIesFromContainingFrame(*this);
            }
        }
    }
    return distance;
}

uint32_t
MgtAssocRequestHeader::DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                                        uint16_t length,
                                                        const MgtAssocRequestHeader& frame)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = frame.m_listenInterval;
    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance <= length,
                  "Bytes read (" << distance << ") exceed expected number (" << length << ")");
    return distance + MgtHeaderInPerStaProfile<MgtAssocRequestHeader, AssocRequestElems>::
                          DeserializeFromPerStaProfileImpl(i, length - distance, frame);
}

/***********************************************************
 *          Ressoc Request
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtReassocRequestHeader);

TypeId
MgtReassocRequestHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtReassocRequestHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtReassocRequestHeader>();
    return tid;
}

TypeId
MgtReassocRequestHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint16_t
MgtReassocRequestHeader::GetListenInterval() const
{
    return m_listenInterval;
}

void
MgtReassocRequestHeader::SetListenInterval(uint16_t interval)
{
    m_listenInterval = interval;
}

const CapabilityInformation&
MgtReassocRequestHeader::Capabilities() const
{
    return m_capability;
}

CapabilityInformation&
MgtReassocRequestHeader::Capabilities()
{
    return m_capability;
}

void
MgtReassocRequestHeader::SetCurrentApAddress(Mac48Address currentApAddr)
{
    m_currentApAddr = currentApAddr;
}

uint32_t
MgtReassocRequestHeader::GetSerializedSizeImpl() const
{
    SetMleContainingFrame();

    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += 2; // listen interval
    size += 6; // current AP address
    size += WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::GetSerializedSizeImpl();
    return size;
}

uint32_t
MgtReassocRequestHeader::GetSerializedSizeInPerStaProfileImpl(
    const MgtReassocRequestHeader& frame) const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size +=
        MgtHeaderInPerStaProfile<MgtReassocRequestHeader,
                                 AssocRequestElems>::GetSerializedSizeInPerStaProfileImpl(frame);
    return size;
}

void
MgtReassocRequestHeader::PrintImpl(std::ostream& os) const
{
    os << "current AP address=" << m_currentApAddr << ", ";
    WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::PrintImpl(os);
}

void
MgtReassocRequestHeader::SerializeImpl(Buffer::Iterator start) const
{
    SetMleContainingFrame();

    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i.WriteHtolsbU16(m_listenInterval);
    WriteTo(i, m_currentApAddr);
    WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::SerializeImpl(i);
}

void
MgtReassocRequestHeader::SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                                      const MgtReassocRequestHeader& frame) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    MgtHeaderInPerStaProfile<MgtReassocRequestHeader,
                             AssocRequestElems>::SerializeInPerStaProfileImpl(i, frame);
}

uint32_t
MgtReassocRequestHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = i.ReadLsbtohU16();
    ReadFrom(i, m_currentApAddr);
    auto distance = i.GetDistanceFrom(start) +
                    WifiMgtHeader<MgtReassocRequestHeader, AssocRequestElems>::DeserializeImpl(i);
    if (auto& mle = Get<MultiLinkElement>())
    {
        for (std::size_t id = 0; id < mle->GetNPerStaProfileSubelements(); id++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(id);
            if (perStaProfile.HasReassocRequest())
            {
                auto& frameInPerStaProfile =
                    std::get<std::reference_wrapper<MgtReassocRequestHeader>>(
                        perStaProfile.GetAssocRequest())
                        .get();
                frameInPerStaProfile.CopyIesFromContainingFrame(*this);
            }
        }
    }
    return distance;
}

uint32_t
MgtReassocRequestHeader::DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                                          uint16_t length,
                                                          const MgtReassocRequestHeader& frame)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    m_listenInterval = frame.m_listenInterval;
    m_currentApAddr = frame.m_currentApAddr;
    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance <= length,
                  "Bytes read (" << distance << ") exceed expected number (" << length << ")");
    return distance + MgtHeaderInPerStaProfile<MgtReassocRequestHeader, AssocRequestElems>::
                          DeserializeFromPerStaProfileImpl(i, length - distance, frame);
}

/***********************************************************
 *          Assoc/Reassoc Response
 ***********************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAssocResponseHeader);

TypeId
MgtAssocResponseHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtAssocResponseHeader")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtAssocResponseHeader>();
    return tid;
}

TypeId
MgtAssocResponseHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

StatusCode
MgtAssocResponseHeader::GetStatusCode()
{
    return m_code;
}

void
MgtAssocResponseHeader::SetStatusCode(StatusCode code)
{
    m_code = code;
}

const CapabilityInformation&
MgtAssocResponseHeader::Capabilities() const
{
    return m_capability;
}

CapabilityInformation&
MgtAssocResponseHeader::Capabilities()
{
    return m_capability;
}

void
MgtAssocResponseHeader::SetAssociationId(uint16_t aid)
{
    m_aid = aid;
}

uint16_t
MgtAssocResponseHeader::GetAssociationId() const
{
    return m_aid;
}

uint32_t
MgtAssocResponseHeader::GetSerializedSizeImpl() const
{
    SetMleContainingFrame();

    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += m_code.GetSerializedSize();
    size += 2; // aid
    size += WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::GetSerializedSizeImpl();
    return size;
}

uint32_t
MgtAssocResponseHeader::GetSerializedSizeInPerStaProfileImpl(
    const MgtAssocResponseHeader& frame) const
{
    uint32_t size = 0;
    size += m_capability.GetSerializedSize();
    size += m_code.GetSerializedSize();
    size +=
        MgtHeaderInPerStaProfile<MgtAssocResponseHeader,
                                 AssocResponseElems>::GetSerializedSizeInPerStaProfileImpl(frame);
    return size;
}

void
MgtAssocResponseHeader::PrintImpl(std::ostream& os) const
{
    os << "status code=" << m_code << ", "
       << "aid=" << m_aid << ", ";
    WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::PrintImpl(os);
}

void
MgtAssocResponseHeader::SerializeImpl(Buffer::Iterator start) const
{
    SetMleContainingFrame();

    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i = m_code.Serialize(i);
    i.WriteHtolsbU16(m_aid);
    WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::SerializeImpl(i);
}

void
MgtAssocResponseHeader::SerializeInPerStaProfileImpl(Buffer::Iterator start,
                                                     const MgtAssocResponseHeader& frame) const
{
    Buffer::Iterator i = start;
    i = m_capability.Serialize(i);
    i = m_code.Serialize(i);
    MgtHeaderInPerStaProfile<MgtAssocResponseHeader,
                             AssocResponseElems>::SerializeInPerStaProfileImpl(i, frame);
}

uint32_t
MgtAssocResponseHeader::DeserializeImpl(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    i = m_code.Deserialize(i);
    m_aid = i.ReadLsbtohU16();
    auto distance = i.GetDistanceFrom(start) +
                    WifiMgtHeader<MgtAssocResponseHeader, AssocResponseElems>::DeserializeImpl(i);
    if (auto& mle = Get<MultiLinkElement>())
    {
        for (std::size_t id = 0; id < mle->GetNPerStaProfileSubelements(); id++)
        {
            auto& perStaProfile = mle->GetPerStaProfile(id);
            if (perStaProfile.HasAssocResponse())
            {
                auto& frameInPerStaProfile = perStaProfile.GetAssocResponse();
                frameInPerStaProfile.CopyIesFromContainingFrame(*this);
            }
        }
    }
    return distance;
}

uint32_t
MgtAssocResponseHeader::DeserializeFromPerStaProfileImpl(Buffer::Iterator start,
                                                         uint16_t length,
                                                         const MgtAssocResponseHeader& frame)
{
    Buffer::Iterator i = start;
    i = m_capability.Deserialize(i);
    i = m_code.Deserialize(i);
    m_aid = frame.m_aid;
    auto distance = i.GetDistanceFrom(start);
    NS_ASSERT_MSG(distance <= length,
                  "Bytes read (" << distance << ") exceed expected number (" << length << ")");
    return distance + MgtHeaderInPerStaProfile<MgtAssocResponseHeader, AssocResponseElems>::
                          DeserializeFromPerStaProfileImpl(i, length - distance, frame);
}

/**********************************************************
 *   ActionFrame
 **********************************************************/
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
        case PROXY_UPDATE: // not used so far
            retval.multihopAction = PROXY_UPDATE;
            break;
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

MgtAddBaRequestHeader::MgtAddBaRequestHeader()
    : m_dialogToken(1),
      m_amsduSupport(1),
      m_bufferSize(0)
{
}

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
}

uint32_t
MgtAddBaRequestHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 1; // Dialog token
    size += 2; // Block ack parameter set
    size += 2; // Block ack timeout value
    size += 2; // Starting sequence control
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
}

uint32_t
MgtAddBaRequestHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    SetParameterSet(i.ReadLsbtohU16());
    m_timeoutValue = i.ReadLsbtohU16();
    SetStartingSequenceControl(i.ReadLsbtohU16());
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
    return m_amsduSupport == 1;
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

uint16_t
MgtAddBaRequestHeader::GetParameterSet() const
{
    uint16_t res = 0;
    res |= m_amsduSupport;
    res |= m_policy << 1;
    res |= m_tid << 2;
    res |= m_bufferSize << 6;
    return res;
}

void
MgtAddBaRequestHeader::SetParameterSet(uint16_t params)
{
    m_amsduSupport = (params)&0x01;
    m_policy = (params >> 1) & 0x01;
    m_tid = (params >> 2) & 0x0f;
    m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
 *                 ADDBAResponse
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtAddBaResponseHeader);

MgtAddBaResponseHeader::MgtAddBaResponseHeader()
    : m_dialogToken(1),
      m_amsduSupport(1),
      m_bufferSize(0)
{
}

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
    os << "status code=" << m_code;
}

uint32_t
MgtAddBaResponseHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 1;                          // Dialog token
    size += m_code.GetSerializedSize(); // Status code
    size += 2;                          // Block ack parameter set
    size += 2;                          // Block ack timeout value
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
}

uint32_t
MgtAddBaResponseHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_dialogToken = i.ReadU8();
    i = m_code.Deserialize(i);
    SetParameterSet(i.ReadLsbtohU16());
    m_timeoutValue = i.ReadLsbtohU16();
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
    return m_amsduSupport == 1;
}

uint16_t
MgtAddBaResponseHeader::GetParameterSet() const
{
    uint16_t res = 0;
    res |= m_amsduSupport;
    res |= m_policy << 1;
    res |= m_tid << 2;
    res |= m_bufferSize << 6;
    return res;
}

void
MgtAddBaResponseHeader::SetParameterSet(uint16_t params)
{
    m_amsduSupport = (params)&0x01;
    m_policy = (params >> 1) & 0x01;
    m_tid = (params >> 2) & 0x0f;
    m_bufferSize = (params >> 6) & 0x03ff;
}

/***************************************************
 *                     DelBa
 ****************************************************/

NS_OBJECT_ENSURE_REGISTERED(MgtDelBaHeader);

MgtDelBaHeader::MgtDelBaHeader()
    : m_reasonCode(1)
{
}

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
}

uint32_t
MgtDelBaHeader::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 2; // DelBa parameter set
    size += 2; // Reason code
    return size;
}

void
MgtDelBaHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtolsbU16(GetParameterSet());
    i.WriteHtolsbU16(m_reasonCode);
}

uint32_t
MgtDelBaHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    SetParameterSet(i.ReadLsbtohU16());
    m_reasonCode = i.ReadLsbtohU16();
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
    uint8_t tid = static_cast<uint8_t>(m_tid);
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

NS_OBJECT_ENSURE_REGISTERED(MgtEmlOperatingModeNotification);

TypeId
MgtEmlOperatingModeNotification::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MgtEmlOperatingModeNotification")
                            .SetParent<Header>()
                            .SetGroupName("Wifi")
                            .AddConstructor<MgtEmlOperatingModeNotification>();
    return tid;
}

TypeId
MgtEmlOperatingModeNotification::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
MgtEmlOperatingModeNotification::Print(std::ostream& os) const
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
MgtEmlOperatingModeNotification::GetSerializedSize() const
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
MgtEmlOperatingModeNotification::Serialize(Buffer::Iterator start) const
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
MgtEmlOperatingModeNotification::Deserialize(Buffer::Iterator start)
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
MgtEmlOperatingModeNotification::SetLinkIdInBitmap(uint8_t linkId)
{
    NS_ABORT_MSG_IF(linkId > 15, "Link ID must not exceed 15");
    if (!m_emlControl.linkBitmap)
    {
        m_emlControl.linkBitmap = 0;
    }
    m_emlControl.linkBitmap = *m_emlControl.linkBitmap | (1 << linkId);
}

std::list<uint8_t>
MgtEmlOperatingModeNotification::GetLinkBitmap() const
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

} // namespace ns3
