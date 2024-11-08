/*
 * Copyright (c) 2010 Hemanth Narra
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Hemanth Narra <hemanth@ittc.ku.com>
 *
 * James P.G. Sterbenz <jpgs@ittc.ku.edu>, director
 * ResiliNets Research Group  https://resilinets.org/
 * Information and Telecommunication Technology Center (ITTC)
 * and Department of Electrical Engineering and Computer Science
 * The University of Kansas Lawrence, KS USA.
 *
 * Work supported in part by NSF FIND (Future Internet Design) Program
 * under grant CNS-0626918 (Postmodern Internet Architecture),
 * NSF grant CNS-1050226 (Multilayer Network Resilience Analysis and Experimentation on GENI),
 * US Department of Defense (DoD), and ITTC at The University of Kansas.
 */

#ifndef DSDV_PACKET_H
#define DSDV_PACKET_H

#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"

#include <iostream>

namespace ns3
{
namespace dsdv
{
/**
 * @ingroup dsdv
 * @brief DSDV Update Packet Format
 * @verbatim
 |      0        |      1        |      2        |       3       |
  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                      Destination Address                      |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                            HopCount                           |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 |                       Sequence Number                         |
 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * @endverbatim
 */

class DsdvHeader : public Header
{
  public:
    /**
     * Constructor
     *
     * @param dst destination IP address
     * @param hopcount hop count
     * @param dstSeqNo destination sequence number
     */
    DsdvHeader(Ipv4Address dst = Ipv4Address(), uint32_t hopcount = 0, uint32_t dstSeqNo = 0);
    ~DsdvHeader() override;
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;

    /**
     * Set destination address
     * @param destination the destination IPv4 address
     */
    void SetDst(Ipv4Address destination)
    {
        m_dst = destination;
    }

    /**
     * Get destination address
     * @returns the destination IPv4 address
     */
    Ipv4Address GetDst() const
    {
        return m_dst;
    }

    /**
     * Set hop count
     * @param hopCount the hop count
     */
    void SetHopCount(uint32_t hopCount)
    {
        m_hopCount = hopCount;
    }

    /**
     * Get hop count
     * @returns the hop count
     */
    uint32_t GetHopCount() const
    {
        return m_hopCount;
    }

    /**
     * Set destination sequence number
     * @param sequenceNumber The sequence number
     */
    void SetDstSeqno(uint32_t sequenceNumber)
    {
        m_dstSeqNo = sequenceNumber;
    }

    /**
     * Get destination sequence number
     * @returns the destination sequence number
     */
    uint32_t GetDstSeqno() const
    {
        return m_dstSeqNo;
    }

  private:
    Ipv4Address m_dst;   ///< Destination IP Address
    uint32_t m_hopCount; ///< Number of Hops
    uint32_t m_dstSeqNo; ///< Destination Sequence Number
};

static inline std::ostream&
operator<<(std::ostream& os, const DsdvHeader& packet)
{
    packet.Print(os);
    return os;
}
} // namespace dsdv
} // namespace ns3

#endif /* DSDV_PACKET_H */
