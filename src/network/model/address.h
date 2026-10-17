/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Modified by: Dhiraj Lokesh <dhirajlokesh@gmail.com>
 */

#ifndef ADDRESS_H
#define ADDRESS_H

#include "tag-buffer.h"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"

#include <array>
#include <compare>
#include <ostream>
#include <stdint.h>
#include <unordered_map>

namespace ns3
{

/**
 * @ingroup network
 * @defgroup address Address
 *
 * Network Address abstractions, including MAC, IPv4 and IPv6.
 */

/**
 * @ingroup address
 * @brief a polymophic address class
 *
 * This class is very similar in design and spirit to the BSD sockaddr
 * structure: they are both used to hold multiple types of addresses
 * together with the type of the address.
 *
 * A new address class defined by a user needs to:
 *   - allocate a type id with Address::Register
 *   - provide a method to convert his new address to an Address
 *     instance. This method is typically a member method named ConvertTo:
 *     Address MyAddress::ConvertTo () const;
 *   - provide a method to convert an Address instance back to
 *     an instance of his new address type. This method is typically
 *     a static member method of his address class named ConvertFrom:
 *     static MyAddress MyAddress::ConvertFrom (const Address &address);
 *   - the ConvertFrom method is expected to check that the type of the
 *     input Address instance is compatible with its own type.
 *
 * Typical code to create a new class type looks like:
 * @code
 * // this class represents addresses which are 2 bytes long.
 * class MyAddress
 * {
 * public:
 *   Address ConvertTo () const;
 *   static MyAddress ConvertFrom ();
 * private:
 *   static uint8_t GetType ();
 * };
 *
 * Address MyAddress::ConvertTo () const
 * {
 *   return Address (GetType (), m_buffer, 2);
 * }
 *
 * MyAddress MyAddress::ConvertFrom (const Address &address)
 * {
 *   MyAddress ad;
 *   NS_ASSERT (address.CheckCompatible (GetType (), 2));
 *   address.CopyTo (ad.m_buffer, 2);
 *   return ad;
 * }
 *
 * uint8_t MyAddress::GetType ()
 * {
 *   static uint8_t type = Address::Register ("AddressType", 2);
 *   return type;
 * }
 * @endcode
 *
 * To convert a specific Address T (e.g., Ipv6Address) to and from an Address type,
 * a class must implement three public functions:
 * @code
 * static T ConvertFrom(const Address& address);
 * Address ConvertTo() const;
 * operator Address() const;
 * @endcode
 *
 * Furthermore, the specific address must call the static function
 * ``Address::Register(kind, length)``. This is typically done in a static function like
 * ``GetType``.
 *
 * The kind/length pair is used to set the address type when this is not known a-priori.
 * The typical use-case is when you decode a header where the address kind is known, but its
 * length is specified in the header itself. The concrete example is the ARP or NDP headers,
 * where you have a MAC address with a variable length.
 * In these cases the code will be (assuming the address is a MAC address):
 * @code
 * Address addr;
 * addr.SetType("MacAddress", addressLen);
 * ReadFrom(i, addr, addressLen);
 * @endcode
 *
 * It is forbidden to have two specific addresses sharing the same kind and length.
 *
 * @see attribute_Address
 */
class Address
{
  private:
    /**
     * Key for the address registry: kind (string) / type (uint8_t)
     */
    using KindType = std::pair<std::string, uint8_t>;

    /**
     * Structure necessary to hash KindType, used as the key for the m_typeRegistry map
     */
    struct KeyHash
    {
        /**
         * Functional operator for (string, uint8_t) hash computation.
         * @param k the key to hash
         * @return The key hash
         */
        std::size_t operator()(const KindType& k) const
        {
            return std::hash<std::string>()(k.first) ^ (std::hash<uint8_t>()(k.second));
        }
    };

    /**
     * Type of the address registry.
     */
    using KindTypeRegistry = std::unordered_map<KindType, uint8_t, Address::KeyHash>;

    /**
     * Container of allocated address types.
     *
     * The data is ordered by KindType (a {kind, length} pair), and sores the address type.
     */
    static KindTypeRegistry m_typeRegistry;

    /// Unassigned Address type is reserved. Defined for clarity.
    static constexpr uint8_t UNASSIGNED_TYPE{0};

  public:
    /**
     * The maximum size of a byte buffer which
     * can be stored in an Address instance.
     */
    static constexpr uint32_t MAX_SIZE{20};

    /**
     * Create an invalid address
     */
    Address() = default;
    /**
     * @brief Create an address from a type and a buffer.
     *
     * This constructor is typically invoked from the conversion
     * functions of various address types when they have to
     * convert themselves to an Address instance.
     *
     * @param type the type of the Address to create
     * @param buffer a pointer to a buffer of bytes which hold
     *        a serialized representation of the address in network
     *        byte order.
     * @param len the length of the buffer.
     */
    Address(uint8_t type, const uint8_t* buffer, uint8_t len);
    /**
     * Set the address type.
     *
     * Works only if the type is not yet set.
     * @see Register()
     *
     * @param kind address kind
     * @param length address length
     */
    void SetType(const std::string& kind, uint8_t length);
    /**
     * @return true if this address is invalid, false otherwise.
     *
     * An address is invalid if and only if it was created
     * through the default constructor and it was never
     * re-initialized.
     */
    bool IsInvalid() const;
    /**
     * @brief Get the length of the underlying address.
     * @return the length of the underlying address.
     */
    uint8_t GetLength() const;
    /**
     * @brief Copy the address bytes into a buffer.
     * @param buffer buffer to copy the address bytes to.
     * @return the number of bytes copied.
     */
    uint32_t CopyTo(uint8_t buffer[MAX_SIZE]) const;
    /**
     * @param buffer buffer to copy the whole address data structure to
     * @param len the size of the buffer
     * @return the number of bytes copied.
     *
     * Copies the type to buffer[0], the length of the address internal buffer
     * to buffer[1] and copies the internal buffer starting at buffer[2].  len
     * must be at least the size of the internal buffer plus a byte for the type
     * and a byte for the length.
     */
    uint32_t CopyAllTo(uint8_t* buffer, uint8_t len) const;
    /**
     * @param buffer pointer to a buffer of bytes which contain
     *        a serialized representation of the address in network
     *        byte order.
     * @param len length of buffer
     * @return the number of bytes copied.
     *
     * Copy the address bytes from buffer into to the internal buffer of this
     * address instance.
     */
    uint32_t CopyFrom(const uint8_t* buffer, uint8_t len);
    /**
     * @param buffer pointer to a buffer of bytes which contain
     *        a copy of all the members of this Address class.
     * @param len the length of the buffer
     * @return the number of bytes copied.
     *
     * The inverse of CopyAllTo().
     *
     * @see CopyAllTo
     */
    uint32_t CopyAllFrom(const uint8_t* buffer, uint8_t len);
    /**
     * @param type a type id as returned by Address::Register
     * @param len the length associated to this type id.
     *
     * @return true if the type of the address stored internally
     * is compatible with the requested type, false otherwise.
     */
    bool CheckCompatible(uint8_t type, uint8_t len) const;
    /**
     * @param type a type id as returned by Address::Register
     * @return true if the type of the address stored internally
     * is compatible with the requested type, false otherwise.
     *
     * This method checks that the types are _exactly_ equal.
     * This method is really used only by the PacketSocketAddress
     * and there is little point in using it otherwise so,
     * you have been warned: DO NOT USE THIS METHOD.
     */
    bool IsMatchingType(uint8_t type) const;
    /**
     * Allocate a new type id for a new type of address.
     *
     * Each address-like class that needs to be converted to/from an
     * ``Address`` needs to register itself once. This is typically done
     * in the ``GetType`` function.
     *
     * The address kind and length are typically used during the buffer
     * deserialization, where the exact address type is unknown, and only its
     * kind and length are known, e.g., if you parse an ARP reply.
     *
     * It is not allowed to have two different addresses with the same
     * kind and length.
     *
     * @param kind address kind, such as "MacAddress"
     * @param length address length
     * @return a new type id.
     */
    static uint8_t Register(const std::string& kind, uint8_t length);
    /**
     * Get the number of bytes needed to serialize the underlying Address
     * Typically, this is GetLength () + 2
     *
     * @return the number of bytes required for an Address in serialized form
     */
    uint32_t GetSerializedSize() const;
    /**
     * Serialize this address in host byte order to a byte buffer
     *
     * @param buffer output buffer that gets written with this Address
     */
    void Serialize(TagBuffer buffer) const;
    /**
     * @param buffer buffer to read address from
     *
     * The input address buffer is expected to be in host byte order format.
     */
    void Deserialize(TagBuffer buffer);

    /**
     * @brief Three-way comparison operator.
     *
     * @param other the other address to compare with
     * @return comparison result
     */
    constexpr std::strong_ordering operator<=>(const Address& other) const = default;

  private:
    /**
     * @brief Stream insertion operator.
     *
     * @param os the stream
     * @param address the address
     * @return a reference to the stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Address& address);

    /**
     * @brief Stream extraction operator.
     *
     * @param is the stream
     * @param address the address
     * @return a reference to the stream
     */
    friend std::istream& operator>>(std::istream& is, Address& address);

    uint8_t m_type{UNASSIGNED_TYPE};        //!< Type of the address
    uint8_t m_len{0};                       //!< Length of the address
    std::array<uint8_t, MAX_SIZE> m_data{}; //!< The address value
};

ATTRIBUTE_HELPER_HEADER(Address);

std::ostream& operator<<(std::ostream& os, const Address& address);
std::istream& operator>>(std::istream& is, Address& address);

} // namespace ns3

#endif /* ADDRESS_H */
