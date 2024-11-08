/*
 * Copyright (c) 2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Pavel Boyko <boyko@iitp.ru>
 */

#ifndef MESH_WIFI_INTERFACE_MAC_PLUGIN_H
#define MESH_WIFI_INTERFACE_MAC_PLUGIN_H

#include "mesh-wifi-beacon.h"

#include "ns3/mac48-address.h"
#include "ns3/packet.h"
#include "ns3/simple-ref-count.h"

namespace ns3
{

class MeshWifiInterfaceMac;

/**
 * @ingroup mesh
 *
 * @brief Common interface for mesh point interface MAC plugins
 *
 * @todo plugins description
 */
class MeshWifiInterfaceMacPlugin : public SimpleRefCount<MeshWifiInterfaceMacPlugin>
{
  public:
    /// This is for subclasses
    virtual ~MeshWifiInterfaceMacPlugin()
    {
    }

    /**
     * Each plugin must be installed on an interface to work
     *
     * @param parent the parent object
     */
    virtual void SetParent(Ptr<MeshWifiInterfaceMac> parent) = 0;
    /**
     * @brief Process received frame
     * @param packet
     * @param header
     *
     * @return false if (and only if) frame should be dropped
     * @todo define when MAC call this
     */
    virtual bool Receive(Ptr<Packet> packet, const WifiMacHeader& header) = 0;
    /**
     * @brief Update frame before it will be forwarded down
     * @param packet
     * @param header
     * @param from
     * @param to
     * @return false if (and only if) frame should be dropped
     * @todo define when MAC call this, preconditions & postconditions
     */
    virtual bool UpdateOutcomingFrame(Ptr<Packet> packet,
                                      WifiMacHeader& header,
                                      Mac48Address from,
                                      Mac48Address to) = 0;
    /**
     * @brief Update beacon before it will be formed and sent
     * @param beacon
     *
     * @todo define when MAC call this
     */
    virtual void UpdateBeacon(MeshWifiBeacon& beacon) const = 0;
    /**
     * Assign a fixed random variable stream number to the random variables
     * used by this model.  Return the number of streams (possibly zero) that
     * have been assigned.
     *
     * @param stream first stream index to use
     * @return the number of stream indices assigned by this model
     */
    virtual int64_t AssignStreams(int64_t stream) = 0;
};

} // namespace ns3

#endif /* MESH_WIFI_INTERFACE_MAC_PLUGIN_H */
