/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 *          Aleksey Kovalenko <kovalenko@iitp.ru>
 */

#ifndef MESH_CONFIGURATION_H
#define MESH_CONFIGURATION_H

#include "ns3/mesh-information-element-vector.h"

namespace ns3
{
namespace dot11s
{

// according to IEEE 802.11 - 2012

// in 7.3.2.98.2 Active Path Selection Protocol Identifier - 802.11s-2011
enum Dot11sPathSelectionProtocol
{
    PROTOCOL_HWMP = 0x01,
};

// in 7.3.2.98.3 Active Path Selection Metric Identifier - 802.11s-2011
enum Dot11sPathSelectionMetric
{
    METRIC_AIRTIME = 0x01,
};

// in 7.3.2.98.4 Congestion Control Mode Identifier - 802.11s-2011
enum Dot11sCongestionControlMode
{
    CONGESTION_SIGNALING = 0x01,
    CONGESTION_NULL = 0x00,
};

// in 7.3.2.98.5 Synchronization Method Identifier - 802.11s-2011
enum Dot11sSynchronizationProtocolIdentifier
{
    SYNC_NEIGHBOUR_OFFSET = 0x01, // Neighbor offset synchronization method
    SYNC_NULL = 0x00,             // Reserved
};

// in 7.3.2.98.6 Authentication Protocol Identifier - 802.11s-2011
enum Dot11sAuthenticationProtocol
{
    AUTH_NULL =
        0x00, // No authentication method is required to establish mesh peerings within the MBSS
    AUTH_SAE = 0x01,  // SAE defined in 8.2a
    AUTH_IEEE = 0x02, // IEEE 802.1X authentication
};

/**
 * @ingroup dot11s
 * A set of values indicating whether a mesh STA is a possible candidate
 * for mesh peering establishment (Section 8.4.2.100.8 of IEEE 802.11-2012)
 */
class Dot11sMeshCapability
{
  public:
    Dot11sMeshCapability();
    /**
     * Size of the field in bytes when serialized
     * @returns the serialized size in bytes
     */
    uint8_t GetSerializedSize() const;
    /**
     * Serialize to a buffer
     * @param i the iterator of the buffer to serialize to
     * @returns new value of the iterator
     */
    Buffer::Iterator Serialize(Buffer::Iterator i) const;
    /**
     * Deserialize from a buffer
     * @param i the iterator from which to deserialize
     * @returns new value of the iterator
     */
    Buffer::Iterator Deserialize(Buffer::Iterator i);
    /**
     * The Mesh Capability is expressed in terms of 8 single bit fields.
     * This method returns the whole byte.
     * @returns the Mesh Capability byte
     */
    uint8_t GetUint8() const;
    bool acceptPeerLinks;    ///< accept peer links
    bool MCCASupported;      ///< MCCA supported
    bool MCCAEnabled;        ///< MCCA enabled
    bool forwarding;         ///< forwarding?
    bool beaconTimingReport; ///< beacon timing report
    bool TBTTAdjustment;     ///< TBTT adjustment
    bool powerSaveLevel;     ///< power save level
    /**
     * This is a utility function to test if the bit at position n is true
     * @param cap byte to test
     * @param n bit number to mask
     * @returns true if bit n is set in cap
     */
    bool Is(uint8_t cap, uint8_t n) const;
    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const Dot11sMeshCapability& a, const Dot11sMeshCapability& b);
};

/**
 * @ingroup dot11s
 * @brief Describes Mesh Configuration Element
 * see 7.3.2.86 of 802.11s draft 3.0
 */
class IeConfiguration : public WifiInformationElement
{
  public:
    IeConfiguration();
    /**
     * Set routing value
     * @param routingId the routing ID
     */
    void SetRouting(Dot11sPathSelectionProtocol routingId);
    /**
     * Set metric value
     * @param metricId the path selection metric
     */
    void SetMetric(Dot11sPathSelectionMetric metricId);
    /**
     * Is HWMP function
     * @returns true if Active Path Selection Protocol Identifier field set
     *          to HWMP
     */
    bool IsHWMP();
    /**
     * Is airtime function
     * @returns true if Active Path Selection Metric Identifier is set to
     *          airtime link metric
     */
    bool IsAirtime();
    /**
     * Set neighbor count
     * @param neighbors the number of neighbors
     */
    void SetNeighborCount(uint8_t neighbors);
    /**
     * Get neighbor count
     * @returns the the neighbor count
     */
    uint8_t GetNeighborCount() const;
    /**
     * Mesh capability
     * @returns the dot11s mesh capability
     */
    const Dot11sMeshCapability& MeshCapability();

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator i, uint16_t length) override;
    void Print(std::ostream& os) const override;

  private:
    /** Active Path Selection Protocol ID */
    Dot11sPathSelectionProtocol m_APSPId;
    /** Active Path Metric ID */
    Dot11sPathSelectionMetric m_APSMId;
    /** Congestion Control Mode ID */
    Dot11sCongestionControlMode m_CCMId;
    /** Sync protocol ID */
    Dot11sSynchronizationProtocolIdentifier m_SPId;
    /** Auth protocol ID */
    Dot11sAuthenticationProtocol m_APId;
    /** Mesh capability */
    Dot11sMeshCapability m_meshCap;
    uint8_t m_neighbors; ///< neighbors
    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const IeConfiguration& a, const IeConfiguration& b);
};

bool operator==(const IeConfiguration& a, const IeConfiguration& b);
bool operator==(const Dot11sMeshCapability& a, const Dot11sMeshCapability& b);
std::ostream& operator<<(std::ostream& os, const IeConfiguration& config);
} // namespace dot11s
} // namespace ns3
#endif
