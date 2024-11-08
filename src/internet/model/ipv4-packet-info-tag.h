/*
 * Copyright (c) 2010 Hajime Tazaki
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Hajime Tazaki <tazaki@sfc.wide.ad.jp>
 */

#ifndef IPV4_PACKET_INFO_TAG_H
#define IPV4_PACKET_INFO_TAG_H

#include "ns3/ipv4-address.h"
#include "ns3/tag.h"

namespace ns3
{

class Node;
class Packet;

/**
 * @ingroup ipv4
 *
 * @brief This class implements Linux struct pktinfo
 * in order to deliver ancillary information to the socket interface.
 * This is used with socket option such as IP_PKTINFO, IP_RECVTTL,
 * IP_RECVTOS. See linux manpage ip(7).
 *
 * See also SocketIpTosTag and SocketIpTtlTag
 *
 * The Tag does not carry the Local address (as it is not currently
 * used to force a source address).
 *
 * This tag in the send direction is presently not enabled but we
 * would accept a patch along those lines in the future.
 */
class Ipv4PacketInfoTag : public Tag
{
  public:
    Ipv4PacketInfoTag();

    /**
     * @brief Set the tag's address
     *
     * @param addr the address
     */
    void SetAddress(Ipv4Address addr);

    /**
     * @brief Get the tag's address
     *
     * @returns the address
     */
    Ipv4Address GetAddress() const;

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
     * @brief Set the tag's Time to Live
     * Implemented, but not used in the stack yet
     * @param ttl the TTL
     */
    void SetTtl(uint8_t ttl);
    /**
     * @brief Get the tag's Time to Live
     * Implemented, but not used in the stack yet
     * @returns the TTL
     */
    uint8_t GetTtl() const;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    void Print(std::ostream& os) const override;

  private:
    // Linux IP_PKTINFO ip(7) implementation
    //
    // struct in_pktinfo {
    //   unsigned int   ipi_ifindex;  /* Interface index */
    //   struct in_addr ipi_spec_dst; /* Local address */
    //   struct in_addr ipi_addr;     /* Header Destination
    //                                   address */
    // };

    Ipv4Address m_addr; //!< Header destination address
    uint32_t m_ifindex; //!< interface index

    // Used for IP_RECVTTL, though not implemented yet.
    uint8_t m_ttl; //!< Time to Live
};
} // namespace ns3

#endif /* IPV4_PACKET_INFO_TAG_H */
