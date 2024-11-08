/*
 * Copyright (c) 2010 Hajime Tazaki
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef IPV6_PACKET_INFO_TAG_H
#define IPV6_PACKET_INFO_TAG_H

#include "ns3/ipv6-address.h"
#include "ns3/tag.h"

namespace ns3
{

class Node;
class Packet;

/**
 * @ingroup ipv6
 *
 * @brief This class implements a tag that carries socket ancillary
 * data to the socket interface. This is used like
 * socket option of IP_PKTINFO/IPV6_PKTINFO in \RFC{3542}
 *
 * See also SocketIpv6TclassTag and SocketIpv6HopLimitTag
 *
 * This tag in the send direction is presently not enabled but we
 * would accept a patch along those lines in the future. To include
 * the nexthop in the send direction would increase the size of the
 * tag beyond 20 bytes, so in that case, we recommend that an
 * additional tag be used to carry the IPv6 next hop address.
 */
class Ipv6PacketInfoTag : public Tag
{
  public:
    Ipv6PacketInfoTag();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * @brief Set the tag's address
     *
     * @param addr the address
     */
    void SetAddress(Ipv6Address addr);

    /**
     * @brief Get the tag's address
     *
     * @returns the address
     */
    Ipv6Address GetAddress() const;

    /**
     * @brief Set the tag's receiving interface
     *
     * @param ifindex the interface index
     */
    void SetRecvIf(uint32_t ifindex);

    /**
     * @brief Get the tag's receiving interface
     *
     * @returns the interface index
     */
    uint32_t GetRecvIf() const;

    /**
     * @brief Set the tag's Hop Limit
     *
     * @param ttl the hop limit
     */
    void SetHoplimit(uint8_t ttl);

    /**
     * @brief Get the tag's Hop Limit
     *
     * @returns the Hop Limit
     */
    uint8_t GetHoplimit() const;

    /**
     * @brief Set the tag's Traffic Class
     *
     * @param tclass the Traffic Class
     */
    void SetTrafficClass(uint8_t tclass);

    /**
     * @brief Get the tag's Traffic Class
     *
     * @returns the Traffic Class
     */
    uint8_t GetTrafficClass() const;

    // inherited functions, no doc necessary
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    void Print(std::ostream& os) const override;

  private:
    /*
     * RFC 3542 includes
     * for outgoing packet,
     *  1.  the source IPv6 address,
     *  2.  the outgoing interface index,
     *  3.  the outgoing hop limit,
     *  4.  the next hop address, and
     *  5.  the outgoing traffic class value.
     *
     * for incoming packet,
     *  1.  the destination IPv6 address,
     *  2.  the arriving interface index,
     *  3.  the arriving hop limit, and
     *  4.  the arriving traffic class value.
     */
    Ipv6Address m_addr; //!< the packet address (src or dst)
    uint8_t m_ifindex;  //!< the Interface index
    uint8_t m_hoplimit; //!< the Hop Limit
    uint8_t m_tclass;   //!< the Traffic Class
};
} // namespace ns3

#endif /* IPV6_PACKET_INFO_TAG_H */
