/*
 * Copyright (c) 2014 Universita' di Firenze, Italy
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
 * Author: Tommaso Pecorella <tommaso.pecorella@unifi.it>
 */

#ifndef RIPNG_HEADER_H
#define RIPNG_HEADER_H

#include "ns3/header.h"
#include "ns3/ipv6-address.h"
#include "ns3/ipv6-header.h"
#include "ns3/packet.h"

#include <list>

namespace ns3
{

/**
 * \ingroup ripng
 *
 * \brief RipNg Routing Table Entry (RTE) - see \RFC{2080}
 */
class RipNgRte : public Header
{
  public:
    RipNgRte();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Return the instance type identifier.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;

    /**
     * \brief Get the serialized size of the packet.
     * \return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Serialize the packet.
     * \param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * \brief Deserialize the packet.
     * \param start Buffer iterator
     * \return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * \brief Set the prefix
     * \param prefix the prefix
     */
    void SetPrefix(Ipv6Address prefix);

    /**
     * \brief Get the prefix
     * \returns the prefix
     */
    Ipv6Address GetPrefix() const;

    /**
     * \brief Set the prefix length
     * \param prefixLen the prefix length
     */
    void SetPrefixLen(uint8_t prefixLen);

    /**
     * \brief Get the prefix length
     * \returns the prefix length
     */
    uint8_t GetPrefixLen() const;

    /**
     * \brief Set the route tag
     * \param routeTag the route tag
     */
    void SetRouteTag(uint16_t routeTag);

    /**
     * \brief Get the route tag
     * \returns the route tag
     */
    uint16_t GetRouteTag() const;

    /**
     * \brief Set the route metric
     * \param routeMetric the route metric
     */
    void SetRouteMetric(uint8_t routeMetric);

    /**
     * \brief Get the route metric
     * \returns the route metric
     */
    uint8_t GetRouteMetric() const;

  private:
    Ipv6Address m_prefix; //!< prefix
    uint16_t m_tag;       //!< route tag
    uint8_t m_prefixLen;  //!< prefix length
    uint8_t m_metric;     //!< route metric
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the reference to the output stream
 * \param h the Routing Table Entry
 * \returns the reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, const RipNgRte& h);

/**
 * \ingroup ripng
 *
 * \brief RipNgHeader - see \RFC{2080}
 */
class RipNgHeader : public Header
{
  public:
    RipNgHeader();

    /**
     * \brief Get the type ID.
     * \return the object TypeId
     */
    static TypeId GetTypeId();

    /**
     * \brief Return the instance type identifier.
     * \return instance type ID
     */
    TypeId GetInstanceTypeId() const override;

    void Print(std::ostream& os) const override;

    /**
     * \brief Get the serialized size of the packet.
     * \return size
     */
    uint32_t GetSerializedSize() const override;

    /**
     * \brief Serialize the packet.
     * \param start Buffer iterator
     */
    void Serialize(Buffer::Iterator start) const override;

    /**
     * \brief Deserialize the packet.
     * \param start Buffer iterator
     * \return size of the packet
     */
    uint32_t Deserialize(Buffer::Iterator start) override;

    /**
     * Commands to be used in RipNg headers
     */
    enum Command_e
    {
        REQUEST = 0x1,
        RESPONSE = 0x2,
    };

    /**
     * \brief Set the command
     * \param command the command
     */
    void SetCommand(Command_e command);

    /**
     * \brief Get the command
     * \returns the command
     */
    Command_e GetCommand() const;

    /**
     * \brief Add a RTE to the message
     * \param rte the RTE
     */
    void AddRte(RipNgRte rte);

    /**
     * \brief Clear all the RTEs from the header
     */
    void ClearRtes();

    /**
     * \brief Get the number of RTE included in the message
     * \returns the number of RTE in the message
     */
    uint16_t GetRteNumber() const;

    /**
     * \brief Get the list of the RTEs included in the message
     * \returns the list of the RTEs in the message
     */
    std::list<RipNgRte> GetRteList() const;

  private:
    uint8_t m_command;             //!< command type
    std::list<RipNgRte> m_rteList; //!< list of the RTEs in the message
};

/**
 * \brief Stream insertion operator.
 *
 * \param os the reference to the output stream
 * \param h the RIPng header
 * \returns the reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, const RipNgHeader& h);

} // namespace ns3

#endif /* RIPNG_HEADER_H */
