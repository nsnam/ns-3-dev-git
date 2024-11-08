/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Pavel Boyko <boyko@iitp.ru>
 */

#ifndef MESH_WIFI_BEACON_H
#define MESH_WIFI_BEACON_H

#include "mesh-information-element-vector.h"

#include "ns3/mgt-headers.h"
#include "ns3/object.h"
#include "ns3/packet.h"

namespace ns3
{

class WifiMacHeader;
class Time;

/**
 * @brief Beacon is beacon header + list of arbitrary information elements
 *
 * It is supposed that distinct mesh protocols can use beacons to transport
 * their own information elements.
 */
class MeshWifiBeacon
{
  public:
    /**
     * C-tor
     *
     * @param ssid is SSID for beacon header
     * @param rates is a set of supported rates
     * @param us beacon interval in microseconds
     */
    MeshWifiBeacon(Ssid ssid, AllSupportedRates rates, uint64_t us);

    /**
     * Read standard Wifi beacon header
     *
     * @returns the management beacon header
     */
    MgtBeaconHeader BeaconHeader() const
    {
        return m_header;
    }

    /**
     * Add information element
     *
     * @param ie the Wifi information element
     */
    void AddInformationElement(Ptr<WifiInformationElement> ie);

    /**
     * Create Wifi header for beacon frame.
     *
     * @param address is sender address
     * @param mpAddress is mesh point address
     * @returns the WifiMacHeader
     */
    WifiMacHeader CreateHeader(Mac48Address address, Mac48Address mpAddress);
    /**
     * Returns the beacon interval of Wifi beacon
     *
     * @returns the beacon interval time
     */
    Time GetBeaconInterval() const;
    /**
     * Create frame = { beacon header + all information elements sorted by ElementId () }
     *
     * @returns the frame
     */
    Ptr<Packet> CreatePacket();

  private:
    /// Beacon header
    MgtBeaconHeader m_header;
    /// List of information elements added
    MeshInformationElementVector m_elements;
};

} // namespace ns3

#endif /* MESH_WIFI_BEACON_H */
