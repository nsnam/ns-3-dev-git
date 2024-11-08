/*
 * Copyright (c) 2009 University of Washington
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */
#ifndef IPV4_ROUTE_H
#define IPV4_ROUTE_H

#include "ns3/ipv4-address.h"
#include "ns3/simple-ref-count.h"

#include <list>
#include <map>
#include <ostream>

namespace ns3
{

class NetDevice;

/**
 * @ingroup ipv4Routing
 *
 * @brief IPv4 route cache entry (similar to Linux struct rtable)
 *
 * This is a reference counted object.  In the future, we will add other
 * entries from struct dst_entry, struct rtable, and struct dst_ops as needed.
 */
class Ipv4Route : public SimpleRefCount<Ipv4Route>
{
  public:
    Ipv4Route();

    /**
     * @param dest Destination Ipv4Address
     */
    void SetDestination(Ipv4Address dest);
    /**
     * @return Destination Ipv4Address of the route
     */
    Ipv4Address GetDestination() const;

    /**
     * @param src Source Ipv4Address
     */
    void SetSource(Ipv4Address src);
    /**
     * @return Source Ipv4Address of the route
     */
    Ipv4Address GetSource() const;

    /**
     * @param gw Gateway (next hop) Ipv4Address
     */
    void SetGateway(Ipv4Address gw);
    /**
     * @return Ipv4Address of the gateway (next hop)
     */
    Ipv4Address GetGateway() const;

    /**
     * Equivalent in Linux to dst_entry.dev
     *
     * @param outputDevice pointer to NetDevice for outgoing packets
     */
    void SetOutputDevice(Ptr<NetDevice> outputDevice);
    /**
     * @return pointer to NetDevice for outgoing packets
     */
    Ptr<NetDevice> GetOutputDevice() const;

#ifdef NOTYET
    // rtable.idev
    void SetInputIfIndex(uint32_t iif);
    uint32_t GetInputIfIndex() const;
#endif

  private:
    Ipv4Address m_dest;            //!< Destination address.
    Ipv4Address m_source;          //!< Source address.
    Ipv4Address m_gateway;         //!< Gateway address.
    Ptr<NetDevice> m_outputDevice; //!< Output device.
#ifdef NOTYET
    uint32_t m_inputIfIndex;
#endif
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the reference to the output stream
 * @param route the Ipv4 route
 * @returns the reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, const Ipv4Route& route);

/**
 * @ingroup ipv4Routing
 *
 * @brief Ipv4 multicast route cache entry (similar to Linux struct mfc_cache)
 */
class Ipv4MulticastRoute : public SimpleRefCount<Ipv4MulticastRoute>
{
  public:
    Ipv4MulticastRoute();

    /**
     * @param group Ipv4Address of the multicast group
     */
    void SetGroup(const Ipv4Address group);
    /**
     * @return Ipv4Address of the multicast group
     */
    Ipv4Address GetGroup() const;

    /**
     * @param origin Ipv4Address of the origin address
     */
    void SetOrigin(const Ipv4Address origin);
    /**
     * @return Ipv4Address of the origin address
     */
    Ipv4Address GetOrigin() const;

    /**
     * @param iif Parent (input interface) for this route
     */
    void SetParent(uint32_t iif);
    /**
     * @return Parent (input interface) for this route
     */
    uint32_t GetParent() const;

    /**
     * @param oif Outgoing interface index
     * @param ttl time-to-live for this route
     */
    void SetOutputTtl(uint32_t oif, uint32_t ttl);

    /**
     * @return map of output interface Ids and TTLs for this route
     */
    std::map<uint32_t, uint32_t> GetOutputTtlMap() const;

    static const uint32_t MAX_INTERFACES =
        16;                              //!< Maximum number of multicast interfaces on a router
    static const uint32_t MAX_TTL = 255; //!< Maximum time-to-live (TTL)

  private:
    Ipv4Address m_group;                 //!< Group
    Ipv4Address m_origin;                //!< Source of packet
    uint32_t m_parent;                   //!< Source interface
    std::map<uint32_t, uint32_t> m_ttls; //!< Time to Live container
};

} // namespace ns3

#endif /* IPV4_ROUTE_H */
