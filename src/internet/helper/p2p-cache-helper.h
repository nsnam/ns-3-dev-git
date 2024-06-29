/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef P2P_CACHE_HELPER_H
#define P2P_CACHE_HELPER_H

#include "ipv4-interface-container.h"
#include "ipv6-interface-container.h"

#include "ns3/arp-cache.h"
#include "ns3/arp-header.h"
#include "ns3/arp-l3-protocol.h"
#include "ns3/channel.h"
#include "ns3/icmpv6-l4-protocol.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv4-interface.h"
#include "ns3/ipv4-l3-protocol.h"
#include "ns3/ipv6-interface.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/net-device-container.h"
#include "ns3/node-list.h"

namespace ns3
{

/**
 * \ingroup P2pCacheHelper
 *
 * @brief A helper class to populate ARP cache for P2P links.
 *
 * This class is useful when a device that is part of a network establishes a peer-to-peer
 * link with another device (referred to as the peer device). Regular ARP operations to
 * discover MAC addresses over the peer-to-peer link may fail in some situations. For instance,
 * the device may use a separate MAC address for communicating with the peer device; if so,
 * the device would use its regular MAC address as the source HW address in ARP Requests
 * sent to discover the peer device's MAC address and the peer device would store the regular
 * MAC address instead of the MAC address used by the device to communicate with the peer
 * device. This helper adds a permanent entry in the ARP cache of the device to store the
 * MAC address of the peer device, so as to avoid sending troublesome ARP Requests.
 *
 * Note: this helper is only designed to work with IPv4.
 */
class P2pCacheHelper
{
  public:
    /**
     * @brief Construct a helper class to make life easier while populating ARP caches for P2P peers
     */
    P2pCacheHelper();
    virtual ~P2pCacheHelper();

    /**
     * \brief Populate ARP cache for the P2P link between the two peers
     * \param netDevice the device to populate its cache
     * \param peerDevice the peer device to populate the cache with its addresses
     */
    void PopulateP2pCache(Ptr<NetDevice> netDevice, Ptr<NetDevice> peerDevice) const;

  private:
    /**
     * \brief Populate P2P ARP entries for given IPv4 interface.
     * \param ipv4Interface the Ipv4Interface to process
     * \param peerDeviceInterface the Ipv4Interface of the peer device
     */
    void PopulateP2pEntries(Ptr<Ipv4Interface> ipv4Interface,
                            Ptr<Ipv4Interface> peerDeviceInterface) const;

    /**
     * \brief Add an auto_generated entry to the ARP cache of an interface.
     * \param netDeviceInterface the Ipv4Interface that ARP cache belongs to
     * \param ipv4Address the IPv4 address will be added to the cache.
     * \param macAddress the MAC address will be added to the cache.
     */
    void AddEntry(Ptr<Ipv4Interface> netDeviceInterface,
                  const Ipv4Address& ipv4Address,
                  const Address& macAddress) const;
};

} // namespace ns3

#endif /* P2P_CACHE_HELPER_H */
