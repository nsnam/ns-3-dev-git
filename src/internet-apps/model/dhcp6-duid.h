/*
 * Copyright (c) 2024 NITK Surathkal
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kavya Bhat <kavyabhat@gmail.com>
 *
 */

#ifndef DHCP6_DUID_H
#define DHCP6_DUID_H

#include "ns3/buffer.h"
#include "ns3/node.h"
#include "ns3/nstime.h"

namespace ns3
{

/**
 * @ingroup dhcp6
 *
 * @class Duid
 * @brief Implements the unique identifier for DHCPv6.
 */
class Duid
{
  public:
    /**
     * @brief Default constructor.
     */
    Duid();

    /// DUID type.
    enum class Type
    {
        LLT = 1, // Link-layer address plus time
        EN,      // Vendor-assigned unique ID based on Enterprise Number
        LL,      // Link-layer address
        UUID,    // Universally Unique Identifier (UUID) [RFC6355]
    };

    /**
     * @ingroup dhcp6
     *
     * @class DuidHash
     * @brief Class providing a hash for DUIDs
     */
    class DuidHash
    {
      public:
        /**
         * @brief Returns the hash of a DUID.
         * @param x the DUID
         * @return the hash
         *
         * This method uses std::hash rather than class Hash
         * as speed is more important than cryptographic robustness.
         */
        size_t operator()(const Duid& x) const noexcept;
    };

    /**
     * @brief Initialize the DUID for a client or server.
     * @param node The node for which the DUID is to be generated.
     */
    void Initialize(Ptr<Node> node);

    /**
     * @brief Check if the DUID is invalid.
     * @return true if the DUID is invalid.
     */
    bool IsInvalid() const;

    /**
     * @brief Get the DUID type
     * @return the DUID type.
     */
    Type GetDuidType() const;

    /**
     * @brief Set the DUID type
     * @param duidType the DUID type.
     */
    void SetDuidType(Type duidType);

    /**
     * @brief Get the hardware type.
     * @return the hardware type
     */
    uint16_t GetHardwareType() const;

    /**
     * @brief Set the hardware type.
     * @param hardwareType the hardware type.
     */
    void SetHardwareType(uint16_t hardwareType);

    /**
     * @brief Set the identifier as the DUID.
     * @param identifier the identifier of the node.
     */
    void SetDuid(std::vector<uint8_t> identifier);

    /**
     * @brief Get the time at which the DUID is generated.
     * @return the timestamp.
     */
    Time GetTime() const;

    /**
     * @brief Get the length of the DUID.
     * @return the DUID length.
     */
    uint8_t GetLength() const;

    /**
     * @brief Set the time at which DUID is generated.
     * @param time the timestamp.
     */
    void SetTime(Time time);

    /**
     * @brief Get the DUID serialized size.
     * @return The DUID serialized sized in bytes.
     */
    uint32_t GetSerializedSize() const;

    /**
     * @brief Serialize the DUID.
     * @param start The buffer iterator.
     */
    void Serialize(Buffer::Iterator start) const;

    /**
     * @brief Deserialize the DUID.
     * @param start The buffer iterator.
     * @param len The number of bytes to be read.
     * @return The number of bytes read.
     */
    uint32_t Deserialize(Buffer::Iterator start, uint32_t len);

    /**
     * @brief Comparison operator
     * @param duid header to compare
     * @return true if the headers are equal
     */
    bool operator==(const Duid& duid) const;

    /**
     * @brief Less than operator.
     *
     * @param a the first operand
     * @param b the first operand
     * @returns true if the operand a is less than operand b
     */
    friend bool operator<(const Duid& a, const Duid& b);

    // TypeId GetInstanceTypeId() const override;
    // void Print(std::ostream& os) const override;
    // uint32_t GetSerializedSize() const override;
    // void Serialize(Buffer::Iterator start) const override;
    // uint32_t Deserialize(Buffer::Iterator start) override;

  private:
    /**
     * @brief Return the identifier of the node.
     * @return the identifier.
     */
    std::vector<uint8_t> GetIdentifier() const;

    /**
     * Type of the DUID.
     * We currently use only DUID-LL, based on the link-layer address.
     */
    Type m_duidType;

    uint16_t m_hardwareType;           //!< Valid hardware type assigned by IANA.
    Time m_time;                       //!< Time at which the DUID is generated. Used in DUID-LLT.
    std::vector<uint8_t> m_identifier; //!< Identifier of the node in bytes.
};

/**
 * @brief Stream output operator
 * @param os output stream
 * @param duid The reference to the DUID object.
 * @return updated stream
 */
std::ostream& operator<<(std::ostream& os, const Duid& duid);

/**
 * Stream extraction operator
 * @param is input stream
 * @param duid The reference to the DUID object.
 * @return std::istream
 */
std::istream& operator>>(std::istream& is, Duid& duid);

} // namespace ns3

#endif
