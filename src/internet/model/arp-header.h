/*
 * Copyright (c) 2005 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef ARP_HEADER_H
#define ARP_HEADER_H

#include "ns3/address.h"
#include "ns3/header.h"
#include "ns3/ipv4-address.h"

namespace ns3
{
/**
 * @ingroup arp
 * @brief The packet header for an ARP packet
 */
class ArpHeader : public Header
{
  public:
    /**
     * @brief Enumeration listing the possible ARP types
     *
     * These ARP types are part of the standard ARP packet format as defined in Section
     * "Definitions" of \RFC{826}.
     */
    enum ArpType_e : uint16_t
    {
        ARP_TYPE_REQUEST = 1,
        ARP_TYPE_REPLY = 2
    };

    /**
     * @brief Enumeration listing the supported hardware types
     *
     * \RFC{826} specifies that the Hardware Type field in the ARP packet indicates the type
     of hardware used.
     * For the full list of Hardware Types, refer to:
     * https://www.iana.org/assignments/arp-parameters/arp-parameters.xhtml
     */
    enum class HardwareType : uint16_t
    {
        UNKNOWN = 0,
        ETHERNET = 1,
        EUI_64 = 27,
    };

    /**
     * @brief Set the ARP request parameters
     * @param sourceHardwareAddress the source hardware address
     * @param sourceProtocolAddress the source IP address
     * @param destinationHardwareAddress the destination hardware address (usually the
     * broadcast address)
     * @param destinationProtocolAddress the destination IP address
     */
    void SetRequest(Address sourceHardwareAddress,
                    Ipv4Address sourceProtocolAddress,
                    Address destinationHardwareAddress,
                    Ipv4Address destinationProtocolAddress);
    /**
     * @brief Set the ARP reply parameters
     * @param sourceHardwareAddress the source hardware address
     * @param sourceProtocolAddress the source IP address
     * @param destinationHardwareAddress the destination hardware address (usually the
     * broadcast address)
     * @param destinationProtocolAddress the destination IP address
     */
    void SetReply(Address sourceHardwareAddress,
                  Ipv4Address sourceProtocolAddress,
                  Address destinationHardwareAddress,
                  Ipv4Address destinationProtocolAddress);

    /**
     * @brief Determines the hardware type based on the length of the address.
     *
     * This method determines the hardware type based on the length of the address.
     * It supports two common hardware address lengths:
     * - 6 bytes: Assumed to be Ethernet.
     * - 8 bytes: Assumed to be EUI-64.
     *
     * If the length of the address does not match these common lengths, the method defaults
     * to Unknown hardware type.
     *
     * @param address The address whose length is used to determine the hardware type.
     * @return The corresponding hardware type.
     */
    HardwareType DetermineHardwareType(const Address& address) const;

    /**
     * @brief Check if the ARP is a request
     * @returns true if it is a request
     */
    bool IsRequest() const;

    /**
     * @brief Check if the ARP is a reply
     * @returns true if it is a reply
     */
    bool IsReply() const;

    /**
     * @brief Get the hardware type
     * @return the hardware type
     */
    HardwareType GetHardwareType() const;

    /**
     * @brief Returns the source hardware address
     * @returns the source hardware address
     */
    Address GetSourceHardwareAddress() const;

    /**
     * @brief Returns the destination hardware address
     * @returns the destination hardware address
     */
    Address GetDestinationHardwareAddress() const;

    /**
     * @brief Returns the source IP address
     * @returns the source IP address
     */
    Ipv4Address GetSourceIpv4Address() const;

    /**
     * @brief Returns the destination IP address
     * @returns the destination IP address
     */
    Ipv4Address GetDestinationIpv4Address() const;

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    void Print(std::ostream& os) const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(Buffer::Iterator start) const override;
    uint32_t Deserialize(Buffer::Iterator start) override;

    HardwareType m_hardwareType; //!< hardware type
    ArpType_e m_type;            //!< type of the ICMP packet
    Address m_macSource;         //!< hardware source address
    Address m_macDest;           //!< hardware destination address
    Ipv4Address m_ipv4Source;    //!< IP source address
    Ipv4Address m_ipv4Dest;      //!< IP destination address
};

/**
 * @brief Stream insertion operator.
 *
 * @param os The reference to the output stream
 * @param hardwareType the hardware type
 * @return The reference to the output stream.
 */
std::ostream& operator<<(std::ostream& os, ArpHeader::HardwareType hardwareType);

} // namespace ns3

#endif /* ARP_HEADER_H */
