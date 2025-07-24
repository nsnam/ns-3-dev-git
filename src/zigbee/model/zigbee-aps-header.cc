/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-aps-header.h"

#include "ns3/address-utils.h"

namespace ns3
{
namespace zigbee
{

ZigbeeApsHeader::ZigbeeApsHeader()
{
    m_frameType = ApsFrameType::APS_DATA;
    m_deliveryMode = ApsDeliveryMode::APS_UCST;
    m_ackFormat = false;
    m_security = false;
    m_ackRequest = false;
    m_extHeaderPresent = false;
}

ZigbeeApsHeader::~ZigbeeApsHeader()
{
}

void
ZigbeeApsHeader::SetFrameType(enum ApsFrameType type)
{
    m_frameType = type;
}

ApsFrameType
ZigbeeApsHeader::GetFrameType() const
{
    return m_frameType;
}

void
ZigbeeApsHeader::SetDeliveryMode(enum ApsDeliveryMode mode)
{
    m_deliveryMode = mode;
}

ApsDeliveryMode
ZigbeeApsHeader::GetDeliveryMode() const
{
    return m_deliveryMode;
}

void
ZigbeeApsHeader::SetSecurity(bool enabled)
{
    m_security = enabled;
}

bool
ZigbeeApsHeader::IsSecurityEnabled() const
{
    return m_security;
}

void
ZigbeeApsHeader::SetAckRequest(bool ack)
{
    m_ackRequest = ack;
}

bool
ZigbeeApsHeader::IsAckRequested() const
{
    return m_ackRequest;
}

void
ZigbeeApsHeader::SetExtHeaderPresent(bool present)
{
    m_extHeaderPresent = present;
}

bool
ZigbeeApsHeader::IsExtHeaderPresent() const
{
    return m_extHeaderPresent;
}

void
ZigbeeApsHeader::SetDstEndpoint(uint8_t dst)
{
    m_dstEndpoint = dst;
}

uint8_t
ZigbeeApsHeader::GetDstEndpoint() const
{
    return m_dstEndpoint;
}

void
ZigbeeApsHeader::SetGroupAddress(uint16_t groupAddress)
{
    m_groupAddress = groupAddress;
}

uint16_t
ZigbeeApsHeader::GetGroupAddress() const
{
    return m_groupAddress;
}

void
ZigbeeApsHeader::SetClusterId(uint16_t clusterId)
{
    m_clusterId = clusterId;
}

uint16_t
ZigbeeApsHeader::GetClusterId() const
{
    return m_clusterId;
}

void
ZigbeeApsHeader::SetProfileId(uint16_t profileId)
{
    m_profileId = profileId;
}

uint16_t
ZigbeeApsHeader::GetProfileId() const
{
    return m_profileId;
}

void
ZigbeeApsHeader::SetSrcEndpoint(uint8_t src)
{
    m_srcEndpoint = src;
}

uint8_t
ZigbeeApsHeader::GetSrcEndpoint() const
{
    return m_srcEndpoint;
}

void
ZigbeeApsHeader::SetApsCounter(uint8_t counter)
{
    m_apsCounter = counter;
}

uint8_t
ZigbeeApsHeader::GetApsCounter() const
{
    return m_apsCounter;
}

void
ZigbeeApsHeader::Serialize(Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    // Frame control field
    uint8_t frameControl = (m_frameType & 0x03) | ((m_deliveryMode & 0x03) << 2) |
                           ((m_ackFormat ? 1 : 0) << 4) | ((m_security ? 1 : 0) << 5) |
                           ((m_ackRequest ? 1 : 0) << 6) | ((m_extHeaderPresent ? 1 : 0) << 7);

    i.WriteU8(frameControl);

    // Addressing fields

    if (m_deliveryMode == ApsDeliveryMode::APS_UCST || m_deliveryMode == ApsDeliveryMode::APS_BCST)
    {
        i.WriteU8(m_dstEndpoint);
    }

    if (m_deliveryMode == ApsDeliveryMode::APS_GROUP_ADDRESSING)
    {
        i.WriteHtolsbU16(m_groupAddress);
    }

    if (m_frameType == ApsFrameType::APS_DATA || m_frameType == ApsFrameType::APS_ACK)
    {
        i.WriteHtolsbU16(m_clusterId);
        i.WriteHtolsbU16(m_profileId);
    }

    if (m_frameType == ApsFrameType::APS_DATA)
    {
        i.WriteU8(m_srcEndpoint);
    }

    i.WriteU8(m_apsCounter);

    // Extended Header

    if (m_extHeaderPresent)
    {
        // Extended Frame control field
        uint8_t extFrameControl = (m_fragmentation & 0x03);
        i.WriteU8(extFrameControl);

        // Block control
        if (m_fragmentation != ApsFragmentation::NOT_FRAGMENTED)
        {
            i.WriteU8(m_blockNumber);
        }
        // ACK Bitfield
        if (m_frameType == ApsFrameType::APS_ACK)
        {
            i.WriteU8(m_ackBitfield);
        }
    }
}

uint32_t
ZigbeeApsHeader::Deserialize(Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    uint8_t frameControl = i.ReadU8();
    m_frameType = static_cast<ApsFrameType>(frameControl & 0x03);
    m_deliveryMode = static_cast<ApsDeliveryMode>((frameControl >> 2) & 0x03);
    m_ackFormat = (frameControl >> 4) & 0x01;
    m_security = (frameControl >> 5) & 0x01;
    m_ackRequest = (frameControl >> 6) & 0x01;
    m_extHeaderPresent = (frameControl >> 7) & 0x01;

    // Addressing fields

    if (m_deliveryMode == ApsDeliveryMode::APS_UCST || m_deliveryMode == ApsDeliveryMode::APS_BCST)
    {
        m_dstEndpoint = i.ReadU8();
    }

    if (m_deliveryMode == ApsDeliveryMode::APS_GROUP_ADDRESSING)
    {
        m_groupAddress = i.ReadLsbtohU16();
    }

    if (m_frameType == ApsFrameType::APS_DATA || m_frameType == ApsFrameType::APS_ACK)
    {
        m_clusterId = i.ReadLsbtohU16();
        m_profileId = i.ReadLsbtohU16();
    }

    if (m_frameType == ApsFrameType::APS_DATA)
    {
        m_srcEndpoint = i.ReadU8();
    }

    m_apsCounter = i.ReadU8();

    // Extended Header

    if (m_extHeaderPresent)
    {
        // Extended Frame control field
        uint8_t extFrameControl = i.ReadU8();
        m_fragmentation = static_cast<ApsFragmentation>(extFrameControl & 0x03);

        // Block control
        if (m_fragmentation != ApsFragmentation::NOT_FRAGMENTED)
        {
            m_blockNumber = i.ReadU8();
        }
        // ACK Bitfield
        if (m_frameType == ApsFrameType::APS_ACK)
        {
            m_ackBitfield = i.ReadU8();
        }
    }

    return i.GetDistanceFrom(start);
}

uint32_t
ZigbeeApsHeader::GetSerializedSize() const
{
    uint8_t totalSize;
    // See Zigbee Specification r22.1.0

    // Fixed field:
    // Frame Control (1) + APS Counter (1)
    totalSize = 2;

    // Variable Fields:
    // Destination EndPoint field (1) (Section 2.2.5.1.2)
    if (m_deliveryMode == ApsDeliveryMode::APS_UCST || m_deliveryMode == ApsDeliveryMode::APS_BCST)
    {
        totalSize += 1;
    }

    // Group Address field (2) (Section 2.2.5.1.3)
    if (m_deliveryMode == ApsDeliveryMode::APS_GROUP_ADDRESSING)
    {
        totalSize += 2;
    }

    // Cluster identifier field and Profile identifier field (4)
    // (Sections 2.2.5.1.4 and 2.2.5.15)
    if (m_frameType == ApsFrameType::APS_DATA || m_frameType == ApsFrameType::APS_ACK)
    {
        totalSize += 4;
    }

    // Source Endpoint field  (1) (Section  2.2.5.1.6)
    if (m_frameType == ApsFrameType::APS_DATA)
    {
        totalSize += 1;
    }

    // Extended header sub-frame (variable)
    if (m_extHeaderPresent)
    {
        // Extended Frame control field (1)
        totalSize += 1;

        // Block control (1)
        if (m_fragmentation != ApsFragmentation::NOT_FRAGMENTED)
        {
            totalSize += 1;
        }
        // ACK Bitfield
        if (m_frameType == ApsFrameType::APS_ACK)
        {
            totalSize += 1;
        }
    }

    return totalSize;
}

TypeId
ZigbeeApsHeader::GetTypeId()
{
    static TypeId tid = TypeId("ns3::zigbee::ZigbeeApsHeader")
                            .SetParent<Header>()
                            .SetGroupName("Zigbee")
                            .AddConstructor<ZigbeeApsHeader>();
    return tid;
}

TypeId
ZigbeeApsHeader::GetInstanceTypeId() const
{
    return GetTypeId();
}

void
ZigbeeApsHeader::Print(std::ostream& os) const
{
    // TODO:
    /* os << "\nAPS Frame Control = " << std::hex << std::showbase << static_cast<uint32_t>(
         (m_frameType & 0x03) |
         ((m_deliveryMode & 0x03) << 2) |
         ((m_ackFormat ? 1 : 0) << 4) |
         ((m_security ? 1 : 0) << 5) |
         ((m_ackRequest ? 1 : 0) << 6) |
         ((m_extHeader ? 1 : 0) << 7));

     os << " | Dst EP = " << static_cast<uint32_t>(m_dstEndpoint)
        << " | Src EP = " << static_cast<uint32_t>(m_srcEndpoint)
        << " | Cluster ID = " << m_clusterId
        << " | Profile ID = " << m_profileId
        << " | Counter = " << static_cast<uint32_t>(m_counter);
     os << "\n";*/
}

} // namespace zigbee
} // namespace ns3
