/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef WIFI_TIMING_ELEMENT_H
#define WIFI_TIMING_ELEMENT_H

#include "ns3/mesh-information-element-vector.h"
#include "ns3/nstime.h"

#include <vector>

namespace ns3
{
namespace dot11s
{
/**
 * @ingroup dot11s
 * @brief Information element describing one unit of beacon timing element
 */
class IeBeaconTimingUnit : public SimpleRefCount<IeBeaconTimingUnit>
{
  public:
    IeBeaconTimingUnit();
    /**
     * Set AID value
     * @param aid the AID
     */
    void SetAid(uint8_t aid);
    /**
     * Set last beacon value
     * @param lastBeacon the last beacon
     */
    void SetLastBeacon(uint16_t lastBeacon);
    /**
     * Set beacon interval value
     * @param beaconInterval the beacon interval
     */
    void SetBeaconInterval(uint16_t beaconInterval);

    /**
     * Get AID value
     * @returns the AID
     */
    uint8_t GetAid() const;
    /**
     * Get last beacon value
     * @returns the last beacon
     */
    uint16_t GetLastBeacon() const;
    /**
     * Get beacon interval
     * @returns the beacon interval
     */
    uint16_t GetBeaconInterval() const;

  private:
    /// Least significant octet of AID:
    uint8_t m_aid;
    /// Last time we received a beacon in accordance with a local TSF measured in 256 microseconds
    /// unit
    uint16_t m_lastBeacon;
    /// Beacon interval of remote mesh point
    uint16_t m_beaconInterval;
    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const IeBeaconTimingUnit& a, const IeBeaconTimingUnit& b);
};

/**
 * @ingroup dot11s
 * @brief See 7.3.2.89 of 802.11s draft 2.07
 */
class IeBeaconTiming : public WifiInformationElement
{
  public:
    /**
     * @ingroup dot11s
     * This type is a list of timing elements obtained from neighbours with their beacons:
     */
    typedef std::vector<Ptr<IeBeaconTimingUnit>> NeighboursTimingUnitsList;

    IeBeaconTiming();
    /**
     * This methods are needed for beacon collision
     * avoidance module:
     * @returns the neighbor timing elements list
     */
    NeighboursTimingUnitsList GetNeighboursTimingElementsList();
    /**
     * Add neighbors timing element unit
     * @param aid the AID
     * @param last_beacon the last beacon time
     * @param beacon_interval the beacon interval
     */
    void AddNeighboursTimingElementUnit(uint16_t aid, Time last_beacon, Time beacon_interval);
    /**
     * Delete neighbors timing element unit
     * @param aid the AID
     * @param last_beacon the last beacon time
     * @param beacon_interval the beacon interval
     */
    void DelNeighboursTimingElementUnit(uint16_t aid, Time last_beacon, Time beacon_interval);
    /// Clear timing element
    void ClearTimingElement();

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator i, uint16_t length) override;
    void Print(std::ostream& os) const override;

    /**
     * equality operator
     *
     * @param a lhs
     * @returns true if equal
     */
    bool operator==(const WifiInformationElement& a) const override;

  private:
    // Converters:
    /**
     * Timestamp to U16 function
     * @param x the timestamp
     * @returns the U16
     */
    static uint16_t TimestampToU16(Time x);
    /**
     * Beacon interval to U16 function
     * @param x the beacon interval
     * @returns the U16
     */
    static uint16_t BeaconIntervalToU16(Time x);
    /**
     * Aid to U8 function
     * @param x the U16
     * @returns the AID
     */
    static uint8_t AidToU8(uint16_t x);

    NeighboursTimingUnitsList m_neighbours; ///< the neighbors
    /**
     * Timing element parameters:
     */
    uint16_t m_numOfUnits;
};

bool operator==(const IeBeaconTimingUnit& a, const IeBeaconTimingUnit& b);
std::ostream& operator<<(std::ostream& os, const IeBeaconTiming& beaconTiming);
} // namespace dot11s
} // namespace ns3
#endif
