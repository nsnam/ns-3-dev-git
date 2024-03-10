/*
 * Copyright (c) 2009 University of Washington
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Leonard Tracy <lentracy@gmail.com>
 */

#ifndef UAN_HEADER_COMMON_H
#define UAN_HEADER_COMMON_H

#include "ns3/header.h"
#include "ns3/mac8-address.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3
{

/**
 * \ingroup uan
 *
 *  UAN protocol descriptor
 */
struct UanProtocolBits
{
    uint8_t m_type : 4;           //!< type (4 bits)
    uint8_t m_protocolNumber : 4; //!< protocol number (4 bits)
};

/**
 * \ingroup uan
 *
 * Common packet header fields.
 *
 *  1 2 3 4 5 6 7 8 1 2 3 4 5 6 7 8 1 2 3 4 5 6 7 8
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    src addr   |    dst addr   | prtcl |  type |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 *
 * src addr: The MAC8 source address
 *
 * dst addr: The MAC8 destination address
 *
 * prtcl: The layer 3 protocol
 * prtcl=1 (IPv4)
 * prtcl=2 (ARP)
 * prtcl=3 (IPv6)
 * prtcl=4 (6LoWPAN)
 *
 * type: The type field is MAC protocol specific
 */
class UanHeaderCommon : public Header
{
  public:
    /** Default constructor */
    UanHeaderCommon();
    /**
     * Create UanHeaderCommon object with given source and destination
     * address and header type
     *
     * \param src Source address defined in header.
     * \param dest Destination address defined in header.
     * \param type Header type.
     * \param protocolNumber the layer 3 protocol number
     */
    UanHeaderCommon(const Mac8Address src,
                    const Mac8Address dest,
                    uint8_t type,
                    uint8_t protocolNumber);
    /** Destructor */
    ~UanHeaderCommon() override;

    /**
     * Register this type.
     * \return The TypeId.
     */
    static TypeId GetTypeId();

    /**
     * Set the destination address.
     *
     * \param dest Address of destination node.
     */
    void SetDest(Mac8Address dest);
    /**
     * Set the source address.
     *
     * \param src Address of packet source node.
     */
    void SetSrc(Mac8Address src);
    /**
     * Set the header type.
     *
     * Use of this value is protocol specific.
     * \param type The type value.
     */
    void SetType(uint8_t type);
    /**
     * Set the packet type.
     *
     * Used to indicate the layer 3 protocol
     * \param protocolNumber The layer 3 protocol number value.
     */
    void SetProtocolNumber(uint16_t protocolNumber);

    /**
     * Get the destination address.
     *
     * \return Mac8Address in destination field.
     */
    Mac8Address GetDest() const;
    /**
     * Get the source address
     *
     * \return Mac8Address in source field.
     */
    Mac8Address GetSrc() const;
    /**
     * Get the header type value.
     *
     * \return value of type field.
     */
    uint8_t GetType() const;
    /**
     * Get the packet type value.
     *
     * \return value of protocolNumber field.
     */
    uint16_t GetProtocolNumber() const;

    // Inherited methods
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;
    void Print(std::ostream& os) const override;
    TypeId GetInstanceTypeId() const override;

  private:
    Mac8Address m_dest;                   //!< The destination address.
    Mac8Address m_src;                    //!< The source address.
    UanProtocolBits m_uanProtocolBits{0}; //!< The type and protocol bits

}; // class UanHeaderCommon

} // namespace ns3

#endif /* UAN_HEADER_COMMON_H */
