/*
 * Copyright (c) 2024 Tokushima University, Tokushima, Japan.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 *  Authors: Ryo Okuda <c611901200@tokushima-u.ac.jp>
 *           Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-nwk-payload-header.h"

#include "ns3/address-utils.h"
#include "ns3/simulator.h"

namespace ns3
{
namespace zigbee
{

/***********************************************************
 *                NWK Command Identifier
 ***********************************************************/

ZigbeePayloadType::ZigbeePayloadType()
{
    m_nwkCmdType = LEAVE_CMD;
}

ZigbeePayloadType::ZigbeePayloadType(enum NwkCommandType nwkCmdType)
{
    m_nwkCmdType = nwkCmdType;
}

TypeId
ZigbeePayloadType::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeePayloadType")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeePayloadType>();
    return tid;
}

TypeId
ZigbeePayloadType::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
ZigbeePayloadType::SetCmdType(enum NwkCommandType nwkCmdType)
{
    m_nwkCmdType = nwkCmdType;
}

NwkCommandType
ZigbeePayloadType::GetCmdType() const
{
    return m_nwkCmdType;
}

uint32_t
ZigbeePayloadType::GetSerializedSize() const
{
    return 1;
}

void
ZigbeePayloadType::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_nwkCmdType);
}

uint32_t
ZigbeePayloadType::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_nwkCmdType = static_cast<NwkCommandType>(i.ReadU8());

    return i.GetDistanceFrom(start);
}

void
ZigbeePayloadType::Print(std::ostream& os) const
{
    os << "\nPayloadType  = ";
    switch (m_nwkCmdType)
    {
    case ROUTE_REQ_CMD:
        os << "ROUTE_REQ\n";
        break;
    case ROUTE_REP_CMD:
        os << "ROUTE_REP\n";
        break;
    case NWK_STATUS_CMD:
        os << "NWK_STATUS\n";
        break;
    case LEAVE_CMD:
        os << "LEAVE\n";
        break;
    case ROUTE_RECORD_CMD:
        os << "ROUTE_RECORD\n";
        break;
    case REJOIN_REQ_CMD:
        os << "REJOIN_REQ\n";
        break;
    case REJOIN_RESP_CMD:
        os << "REJOIN_RESP\n";
        break;
    case LINK_STATUS_CMD:
        os << "LINK_STATUS\n";
        break;
    case NWK_REPORT_CMD:
        os << "NWK_REPORT \n";
        break;
    case NWK_UPDATE_CMD:
        os << "NWK_UPDATE\n";
        break;
    case TIMEOUT_REQ_CMD:
        os << "TIMEOUT_REQ\n";
        break;
    case TIMEOUT_RESP_CMD:
        os << "TIMEOUT_RESP\n";
        break;
    case LINK_POWER_DELTA_CMD:
        os << "LINK_POWER_DELTA\n";
        break;
    default:
        os << "UNKNOWN\n";
    }
}

/***********************************************************
 *                Route Request Command
 ***********************************************************/

ZigbeePayloadRouteRequestCommand::ZigbeePayloadRouteRequestCommand()
{
    m_cmdOptManyToOne = NO_MANY_TO_ONE;
    m_cmdOptDstIeeeAddr = false;
    m_cmdOptMcst = false;
    m_routeReqId = 0;
    m_pathCost = 0;
}

TypeId
ZigbeePayloadRouteRequestCommand::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeePayloadRouteRequestCommand")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeePayloadRouteRequestCommand>();
    return tid;
}

TypeId
ZigbeePayloadRouteRequestCommand::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
ZigbeePayloadRouteRequestCommand::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 5; // (Command Option + Route request identifier + Destination address + Path cost)
    if (m_cmdOptDstIeeeAddr)
    {
        size += 8;
    }

    return size;
}

void
ZigbeePayloadRouteRequestCommand::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(GetCmdOptionField());
    i.WriteU8(m_routeReqId);
    WriteTo(i, m_dstAddr);
    i.WriteU8(m_pathCost);
    if (m_cmdOptDstIeeeAddr)
    {
        WriteTo(i, m_dstIeeeAddr);
    }
}

uint32_t
ZigbeePayloadRouteRequestCommand::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint8_t cmdOptField = i.ReadU8();
    SetCmdOptionField(cmdOptField);
    m_routeReqId = (i.ReadU8());
    m_dstAddr = (i.ReadU16());
    m_pathCost = (i.ReadU8());

    if (m_cmdOptDstIeeeAddr)
    {
        ReadFrom(i, m_dstIeeeAddr);
    }

    return i.GetDistanceFrom(start);
}

void
ZigbeePayloadRouteRequestCommand::Print(std::ostream& os) const
{
    os << "Multicast = " << static_cast<uint32_t>(m_cmdOptMcst)
       << "| Route request identifier = " << static_cast<uint32_t>(m_routeReqId)
       << "| Destination address = " << m_dstAddr
       << "| Path cost = " << static_cast<uint32_t>(m_pathCost) << "| CmdOptFieldManyToOne = ";
    switch (m_cmdOptManyToOne)
    {
    case NO_MANY_TO_ONE:
        os << "NO_MANY_TO_ONE";
        break;
    case ROUTE_RECORD:
        os << "ROUTE_RECORD";
        break;
    case NO_ROUTE_RECORD:
        os << "NO_ROUTE_RECORD";
        break;
    }

    if (m_cmdOptDstIeeeAddr)
    {
        os << "| Destination IEEE address = " << m_dstIeeeAddr;
    }

    os << "\n";
}

void
ZigbeePayloadRouteRequestCommand::SetCmdOptManyToOneField(enum ManyToOne manyToOneField)
{
    m_cmdOptManyToOne = manyToOneField;
}

uint8_t
ZigbeePayloadRouteRequestCommand::GetCmdOptManyToOneField() const
{
    return m_cmdOptManyToOne;
}

void
ZigbeePayloadRouteRequestCommand::SetRouteReqId(uint8_t id)
{
    m_routeReqId = id;
}

uint8_t
ZigbeePayloadRouteRequestCommand::GetRouteReqId() const
{
    return (m_routeReqId);
}

void
ZigbeePayloadRouteRequestCommand::SetDstAddr(Mac16Address addr)
{
    m_dstAddr = addr;
}

Mac16Address
ZigbeePayloadRouteRequestCommand::GetDstAddr() const
{
    return (m_dstAddr);
}

void
ZigbeePayloadRouteRequestCommand::SetPathCost(uint8_t cost)
{
    m_pathCost = cost;
}

uint8_t
ZigbeePayloadRouteRequestCommand::GetPathCost() const
{
    return (m_pathCost);
}

bool
ZigbeePayloadRouteRequestCommand::IsDstIeeeAddressPresent() const
{
    return m_cmdOptDstIeeeAddr;
}

void
ZigbeePayloadRouteRequestCommand::SetDstIeeeAddr(Mac64Address dst)
{
    m_cmdOptDstIeeeAddr = true;
    m_dstIeeeAddr = dst;
}

Mac64Address
ZigbeePayloadRouteRequestCommand::GetDstIeeeAddr() const
{
    return (m_dstIeeeAddr);
}

void
ZigbeePayloadRouteRequestCommand::SetCmdOptionField(uint8_t cmdOptionField)
{
    m_cmdOptManyToOne = static_cast<ManyToOne>((cmdOptionField >> 3) & (0x03)); // Bit 3-4
    m_cmdOptDstIeeeAddr = (cmdOptionField >> 5) & (0x01);                       // Bit 5
    m_cmdOptMcst = (cmdOptionField >> 6) & (0x01);                              // Bit 6
}

uint8_t
ZigbeePayloadRouteRequestCommand::GetCmdOptionField() const
{
    uint8_t val = 0;

    val |= (m_cmdOptManyToOne << 3) & (0x03 << 3);   // Bit 3-4
    val |= (m_cmdOptDstIeeeAddr << 5) & (0x01 << 5); // Bit 5
    val |= (m_cmdOptMcst << 6) & (0x01 << 6);        // Bit 6

    return val;
}

/***********************************************************
 *                Route Reply Command
 ***********************************************************/

ZigbeePayloadRouteReplyCommand::ZigbeePayloadRouteReplyCommand()
{
    m_cmdOptOrigIeeeAddr = false;
    m_cmdOptRespIeeeAddr = false;
    m_cmdOptMcst = false;
    m_routeReqId = 0;
    m_pathCost = 0;
}

TypeId
ZigbeePayloadRouteReplyCommand::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeePayloadRouteReplyCommand")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeePayloadRouteReplyCommand>();
    return tid;
}

TypeId
ZigbeePayloadRouteReplyCommand::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
ZigbeePayloadRouteReplyCommand::GetSerializedSize() const
{
    uint32_t size = 0;
    size += 7; // (Command Option + Route request identifier + Originator address + Responder
               // address + Path cost)
    if (m_cmdOptOrigIeeeAddr)
    {
        size += 8;
    }

    if (m_cmdOptRespIeeeAddr)
    {
        size += 8;
    }

    return size;
}

void
ZigbeePayloadRouteReplyCommand::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteU8(m_routeReqId);
    WriteTo(i, m_origAddr);
    WriteTo(i, m_respAddr);
    i.WriteU8(m_pathCost);
    if (m_cmdOptOrigIeeeAddr)
    {
        WriteTo(i, m_origIeeeAddr);
    }

    if (m_cmdOptRespIeeeAddr)
    {
        WriteTo(i, m_respIeeeAddr);
    }
}

uint32_t
ZigbeePayloadRouteReplyCommand::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_routeReqId = (i.ReadU8());
    ReadFrom(i, m_origAddr);
    ReadFrom(i, m_respAddr);
    m_pathCost = (i.ReadU8());
    if (m_cmdOptOrigIeeeAddr)
    {
        ReadFrom(i, m_origIeeeAddr);
    }

    if (m_cmdOptRespIeeeAddr)
    {
        ReadFrom(i, m_respIeeeAddr);
    }

    return i.GetDistanceFrom(start);
}

void
ZigbeePayloadRouteReplyCommand::Print(std::ostream& os) const
{
    os << "| Command name = ROUTE_REP"
       << "| Route request identifier = " << static_cast<uint32_t>(m_routeReqId)
       << "| m_cmdOptOrigIeeeAddr = " << static_cast<uint32_t>(m_cmdOptOrigIeeeAddr)
       << "| Originator address = " << m_origAddr << "| Responder address = " << m_respAddr
       << "| Path cost = " << static_cast<uint32_t>(m_pathCost);
    if (m_cmdOptOrigIeeeAddr)
    {
        os << "| Originator IEEE address = " << m_origIeeeAddr;
    }

    if (m_cmdOptRespIeeeAddr)
    {
        os << "| Responder IEEE address = " << m_respIeeeAddr;
    }

    os << "\n";
}

void
ZigbeePayloadRouteReplyCommand::SetRouteReqId(uint8_t id)
{
    m_routeReqId = id;
}

uint8_t
ZigbeePayloadRouteReplyCommand::GetRouteReqId() const
{
    return (m_routeReqId);
}

void
ZigbeePayloadRouteReplyCommand::SetOrigAddr(Mac16Address addr)
{
    m_origAddr = addr;
}

Mac16Address
ZigbeePayloadRouteReplyCommand::GetOrigAddr() const
{
    return (m_origAddr);
}

void
ZigbeePayloadRouteReplyCommand::SetRespAddr(Mac16Address addr)
{
    m_respAddr = addr;
}

Mac16Address
ZigbeePayloadRouteReplyCommand::GetRespAddr() const
{
    return (m_respAddr);
}

void
ZigbeePayloadRouteReplyCommand::SetPathCost(uint8_t cost)
{
    m_pathCost = cost;
}

uint8_t
ZigbeePayloadRouteReplyCommand::GetPathCost() const
{
    return (m_pathCost);
}

void
ZigbeePayloadRouteReplyCommand::SetOrigIeeeAddr(Mac64Address orig)
{
    m_cmdOptOrigIeeeAddr = true;
    m_origIeeeAddr = orig;
}

Mac64Address
ZigbeePayloadRouteReplyCommand::GetOrigIeeeAddr() const
{
    return (m_origIeeeAddr);
}

void
ZigbeePayloadRouteReplyCommand::SetRespIeeeAddr(Mac64Address resp)
{
    m_cmdOptRespIeeeAddr = true;
    m_respIeeeAddr = resp;
}

Mac64Address
ZigbeePayloadRouteReplyCommand::GetRespIeeeAddr() const
{
    return (m_respIeeeAddr);
}

/***********************************************************
 *                Beacon Payload
 ***********************************************************/

ZigbeeBeaconPayload::ZigbeeBeaconPayload()
{
    m_protocolId = 0x00;
    m_stackProfile = 0x00;
    m_protocolVer = 0x00;
    m_routerCapacity = false;
    m_deviceDepth = 0x00;
    m_endDevCapacity = false;
    m_extPanId = 0x00;
    m_txOffset = 0x00;
    m_nwkUpdateId = 0x00;
}

TypeId
ZigbeeBeaconPayload::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeeBeaconPayload")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeeBeaconPayload>();
    return tid;
}

TypeId
ZigbeeBeaconPayload::GetInstanceTypeId() const
{
    return GetTypeId();
}

uint32_t
ZigbeeBeaconPayload::GetSerializedSize() const
{
    return 16;
}

void
ZigbeeBeaconPayload::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    uint16_t dataBundle1 = 0;
    uint32_t dataBundle2 = 0;

    dataBundle1 = m_stackProfile & (0x0F);                  // Bit 0-3
    dataBundle1 |= (m_protocolVer << 4) & (0x0F << 4);      // Bit 4-7
    dataBundle1 |= (m_routerCapacity << 10) & (0x01 << 10); // Bit 10
    dataBundle1 |= (m_deviceDepth << 11) & (0x0F << 11);    // Bit 11-14
    dataBundle1 |= (m_endDevCapacity << 15) & (0x01 << 15); // Bit 15

    dataBundle2 = m_txOffset & (0xFFFFFF);               // Bit 0-23
    dataBundle2 |= (m_nwkUpdateId << 24) & (0xFF << 24); // Bit 24-31

    i.WriteU8(m_protocolId);
    i.WriteHtolsbU16(dataBundle1);
    i.WriteU64(m_extPanId);
    i.WriteHtolsbU32(dataBundle2);
}

uint32_t
ZigbeeBeaconPayload::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    uint16_t dataBundle1 = 0;
    uint32_t dataBundle2 = 0;

    m_protocolId = i.ReadU8();
    dataBundle1 = i.ReadLsbtohU16();
    m_extPanId = i.ReadU64();
    dataBundle2 = i.ReadLsbtohU32();

    m_stackProfile = (dataBundle1) & (0x0F);         // Bit 0-3
    m_protocolVer = (dataBundle1 >> 4) & (0x0F);     // Bit 4-7
    m_routerCapacity = (dataBundle1 >> 10) & (0x01); // Bit 10
    m_deviceDepth = (dataBundle1 >> 11) & (0x0F);    // Bit 11-14
    m_endDevCapacity = (dataBundle1 >> 15) & (0x01); // Bit 15

    m_txOffset = (dataBundle2) & (0xFFFFFF);      // Bit 0-23
    m_nwkUpdateId = (dataBundle2 >> 24) & (0xFF); // Bit 24-31

    return i.GetDistanceFrom(start);
}

void
ZigbeeBeaconPayload::Print(std::ostream& os) const
{
    os << "\n";
    os << " | Protocol Id = " << static_cast<uint32_t>(m_protocolId)
       << " | Stack Profile = " << static_cast<uint32_t>(m_stackProfile)
       << " | Nwk Protocol Version = " << static_cast<uint32_t>(m_protocolVer)
       << " | Router Capacity = " << static_cast<uint32_t>(m_routerCapacity)
       << " | Device Depth = " << static_cast<uint32_t>(m_deviceDepth)
       << " | End Device Capacity = " << static_cast<uint32_t>(m_endDevCapacity)
       << " | nwk Pan Id = " << static_cast<uint32_t>(m_extPanId)
       << " | TxOffset = " << static_cast<uint32_t>(m_txOffset)
       << " | Nwk Update Id = " << static_cast<uint32_t>(m_nwkUpdateId);

    os << "\n";
}

void
ZigbeeBeaconPayload::SetStackProfile(uint8_t stackProfile)
{
    m_stackProfile = stackProfile;
}

uint8_t
ZigbeeBeaconPayload::GetProtocolId() const
{
    return m_protocolId;
}

uint8_t
ZigbeeBeaconPayload::GetStackProfile() const
{
    return m_stackProfile;
}

void
ZigbeeBeaconPayload::SetRouterCapacity(bool routerCapacity)
{
    m_routerCapacity = routerCapacity;
}

bool
ZigbeeBeaconPayload::GetRouterCapacity() const
{
    return m_routerCapacity;
}

void
ZigbeeBeaconPayload::SetDeviceDepth(uint8_t deviceDepth)
{
    m_deviceDepth = deviceDepth;
}

uint8_t
ZigbeeBeaconPayload::GetDeviceDepth() const
{
    return m_deviceDepth;
}

void
ZigbeeBeaconPayload::SetEndDevCapacity(bool endDevCapacity)
{
    m_endDevCapacity = endDevCapacity;
}

bool
ZigbeeBeaconPayload::GetEndDevCapacity() const
{
    return m_endDevCapacity;
}

void
ZigbeeBeaconPayload::SetExtPanId(uint64_t extPanId)
{
    m_extPanId = extPanId;
}

uint64_t
ZigbeeBeaconPayload::GetExtPanId() const
{
    return m_extPanId;
}

void
ZigbeeBeaconPayload::SetTxOffset(uint32_t txOffset)
{
    m_txOffset = txOffset;
}

uint32_t
ZigbeeBeaconPayload::GetTxOffset() const
{
    return m_txOffset;
}

void
ZigbeeBeaconPayload::SetNwkUpdateId(uint8_t nwkUpdateId)
{
    m_nwkUpdateId = nwkUpdateId;
}

uint8_t
ZigbeeBeaconPayload::GetNwkUpdateId() const
{
    return m_nwkUpdateId;
}

} // namespace zigbee
} // namespace ns3
