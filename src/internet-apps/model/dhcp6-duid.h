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
         * @returns the hash
         *
         * This method uses std::hash rather than class Hash
         * as speed is more important than cryptographic robustness.
         */
        size_t operator()(const Duid& x) const noexcept;
    };

    /**
     * Initialize a DUID.
     *
     * The DUID initialization logic is the following:
     *   - DUID-LLT and DUID-LL: the identifier is created by selecting
     *     the NetDevices with the longest MAC address length, and among
     *     them, the one with the smallest index.
     *   - DUID-LLT: the creation time is taken from the simulation time.
     *   - DUID-EN: the Enterprise number is 0xf00dcafe (arbitrary number,
     *     currently unused), and the identifier is based on the Node ID.
     *   - DUID-UUID: the UUID is based on the Node ID.
     *
     * @param duidType the DUID type.
     * @param node the node this DUID refers to
     * @param enIdentifierLength the DUID-EN identifier length (unused in other DUID types)
     */
    void Initialize(Type duidType, Ptr<Node> node, uint32_t enIdentifierLength);

    /**
     * @brief Initialize the DUID-LL for a client or server.
     * @param node The node for which the DUID is to be generated.
     */
    void InitializeLL(Ptr<Node> node);

    /**
     * @brief Initialize the DUID-LLT for a client or server.
     * @param node The node for which the DUID is to be generated.
     */
    void InitializeLLT(Ptr<Node> node);

    /**
     * @brief Initialize the DUID-EN for a client or server.
     * @param enNumber The Enterprise Number.
     * @param identifier The device Identifier.
     */
    void InitializeEN(uint32_t enNumber, const std::vector<uint8_t>& identifier);

    /**
     * @brief Initialize the DUID-EN for a client or server.
     * @param uuid The UUID.
     */
    void InitializeUUID(const std::vector<uint8_t>& uuid);

    /**
     * @brief Check if the DUID is invalid.
     * @returns true if the DUID is invalid.
     */
    bool IsInvalid() const;

    /**
     * @brief Get the DUID type
     * @returns the DUID type.
     */
    Type GetDuidType() const;

    /**
     * @brief Get the length of the DUID.
     * @returns the DUID length.
     */
    uint8_t GetLength() const;

    /**
     * @brief Return the identifier of the node.
     * @returns the identifier.
     */
    std::vector<uint8_t> GetIdentifier() const;

    /**
     * @brief Get the DUID serialized size.
     * @returns The DUID serialized sized in bytes.
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
     * @returns The number of bytes read.
     */
    uint32_t Deserialize(Buffer::Iterator start, uint32_t len);

    /**
     * @brief Comparison operator.
     *
     * @note Defined because gcc-13.3 raises an error in optimized builds.
     * Other than that, the comparison is semantically identical to the default one.
     *
     * @param other DUID to compare to this one.
     * @return true if the duids are equal.
     */
    bool operator==(const Duid& other) const;

    /**
     * Spaceship comparison operator. All the other comparison operators
     * are automatically generated from this one.
     *
     * @note Defined because gcc-13.3 raises an error in optimized builds.
     * Other than that, the comparison is semantically identical to the default one.
     *
     * @param a left operand DUID to compare.
     * @param b right operand DUID to compare.
     * @returns The result of the comparison.
     */
    friend std::strong_ordering operator<=>(const Duid& a, const Duid& b);

    /**
     * @brief Stream output operator.
     *
     * The DUID is printed according to its type, e.g.:
     *   - DUID-LL HWtype: 1 Identifier: 0x000000000002
     *   - DUID-LLT HWtype: 1 Time: 0 Identifier: 0x000000000002
     *   - DUID-EN EnNumber: 0xf00dcafe Identifier: 0x000102030405060708090a
     *   - DUID-UUID UUID: 0x000102030405060708090a0b0c0d0e0f
     *
     * @param os output stream
     * @param duid The reference to the DUID object.
     * @returns updated stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Duid& duid);

  private:
    /**
     * DUID data offset.
     *
     * Each DUID has some fixed-length fields (e.g., Hardware Type), and a
     * variable length part. This is the offset to the variable length part.
     */
    enum Offset
    {
        LLT = 6,
        EN = 4,
        LL = 2,
        UUID = 0
    };

    /**
     * Find a suitable NetDevice address to create a DUID-LL or DUID-LLT.
     *
     * We select the NetDevices with the longest MAC address length,
     * and among them, the one with the smallest index.
     *
     * @param node The node for which the DUID is to be generated.
     * @returns the L2 address for the DUID Hardware Identifier.
     */
    Address FindSuitableNetDeviceAddress(Ptr<Node> node);

    /**
     * Utility function to convert a 32-bit number to
     * an array with its bytes in network order.
     *
     * @param[in] source the number to convert.
     * @param[out] dest the converted number.
     */
    void HtoN(uint32_t source, std::vector<uint8_t>::iterator dest);

    Type m_duidType{Duid::Type::LL}; //!< DUID type.

    std::vector<uint8_t> m_blob{}; //!< DUID data, content depending on the DUID type.
};

} // namespace ns3

#endif
