/*
 * Copyright (c) 2007-2008 Louis Pasteur University
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sebastien Vincent <vincent@clarinet.u-strasbg.fr>
 */

#ifndef IPV6_ADDRESS_H
#define IPV6_ADDRESS_H

#include "ipv4-address.h"
#include "mac8-address.h"

#include "ns3/address.h"
#include "ns3/attribute-helper.h"
#include "ns3/deprecated.h"

#include <array>
#include <compare>
#include <cstring>
#include <optional>
#include <ostream>
#include <stdint.h>

namespace ns3
{

class Ipv6Prefix;
class Mac16Address;
class Mac48Address;
class Mac64Address;

/**
 * @ingroup address
 * @class Ipv6Address
 * @brief Describes an IPv6 address.
 * @see Ipv6Prefix
 * @see attribute_Ipv6Address
 */
class Ipv6Address
{
  public:
    /**
     * @brief Default constructor.
     */
    Ipv6Address() = default;

    /**
     * @brief Constructs an Ipv6Address by parsing the input C-string.
     * @param address the C-string containing the IPv6 address (e.g. 2001:db8:f00d:cafe::1).
     */
    Ipv6Address(const char* address);

    /**
     * @brief Checks if the string contains an Ipv6Address
     *
     * Input address is in format:
     * \c 2001:db8:f00d:cafe::1
     *
     * Note: the function uses ``inet_pton`` internally.
     *
     * @see Address::CheckCompatible hich has a similar name but which
     * instead checks the underlying type and length embedded in the Address.
     *
     * @param addressStr string containing the address as described above
     * @return true if the string can be parsed as an IPv6 address
     */
    static bool CheckCompatible(const std::string& addressStr);

    /**
     * @brief Constructs an Ipv6Address by using the input 16 bytes.
     * @param address the 128-bit address
     * @warning the parameter must point on a 16 bytes integer array!
     */
    Ipv6Address(uint8_t address[16]);

    /**
     * @brief Sets an Ipv6Address by parsing the input C-string.
     * @param address the C-string containing the IPv6 address (e.g. 2001:db8:f00d:cafe::1).
     */
    void Set(const char* address);

    /**
     * @brief Set an Ipv6Address by using the input 16 bytes.
     *
     * @param address the 128-bit address
     * @warning the parameter must point on a 16 bytes integer array!
     */
    void Set(uint8_t address[16]);

    /**
     * @brief Serialize this address to a 16-byte buffer.
     * @param buf the output buffer to which this address gets overwritten with this
     * Ipv6Address
     */
    void Serialize(uint8_t buf[16]) const;

    /**
     * @brief Deserialize this address.
     * @param buf buffer to read address from
     * @return an Ipv6Address
     */
    static Ipv6Address Deserialize(const uint8_t buf[16]);

    /**
     * @brief Make the solicited IPv6 address.
     * @param addr the IPv6 address
     * @return Solicited IPv6 address
     */
    static Ipv6Address MakeSolicitedAddress(Ipv6Address addr);

    /**
     * @brief Make the Ipv4-mapped IPv6 address.
     * @param addr the IPv4 address
     * @return Ipv4-mapped IPv6 address
     */
    static Ipv6Address MakeIpv4MappedAddress(Ipv4Address addr);

    /**
     * @brief Return the Ipv4 address.
     * @return Ipv4 address
     */
    Ipv4Address GetIpv4MappedAddress() const;

    /**
     * @brief Make the autoconfigured IPv6 address from a Mac address.
     *
     * Actually the MAC supported are: Mac8, Mac16, Mac48, and Mac64.
     *
     * @param addr the MAC address.
     * @param prefix the IPv6 prefix
     * @return autoconfigured IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredAddress(Address addr, Ipv6Address prefix);

    /**
     * @brief Make the autoconfigured IPv6 address from a Mac address.
     *
     * Actually the MAC supported are: Mac8, Mac16, Mac48, and Mac64.
     *
     * @param addr the MAC address.
     * @param prefix the IPv6 prefix
     * @return autoconfigured IPv6 address
     */

    static Ipv6Address MakeAutoconfiguredAddress(Address addr, Ipv6Prefix prefix);

    /**
     * @brief Make the autoconfigured IPv6 address with Mac16Address.
     *
     * The EUI-64 scheme used is based on the \RFC{4944}.
     *
     * @param addr the MAC address (16 bits).
     * @param prefix the IPv6 prefix
     * @return autoconfigured IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredAddress(Mac16Address addr, Ipv6Address prefix);

    /**
     * @brief Make the autoconfigured IPv6 address with Mac48Address.
     *
     * The EUI-64 scheme used is based on \RFC{2464}.
     *
     * @param addr the MAC address (48 bits).
     * @param prefix the IPv6 prefix
     * @return autoconfigured IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredAddress(Mac48Address addr, Ipv6Address prefix);

    /**
     * @brief Make the autoconfigured IPv6 address with Mac64Address.
     * @param addr the MAC address (64 bits).
     * @param prefix the IPv6 prefix
     * @return autoconfigured IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredAddress(Mac64Address addr, Ipv6Address prefix);

    /**
     * @brief Make the autoconfigured IPv6 address with Mac8Address.
     *
     * The EUI-64 scheme used is loosely based on the \RFC{2464}.
     *
     * @param addr the Mac8Address address (8 bits).
     * @param prefix the IPv6 prefix
     * @return autoconfigured IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredAddress(Mac8Address addr, Ipv6Address prefix);

    /**
     * @brief Make the autoconfigured link-local IPv6 address from a Mac address.
     *
     * Actually the MAC supported are: Mac8, Mac16, Mac48, and Mac64.
     *
     * @param mac the MAC address.
     * @return autoconfigured link-local IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredLinkLocalAddress(Address mac);

    /**
     * @brief Make the autoconfigured link-local IPv6 address with Mac16Address.
     *
     * The EUI-64 scheme used is based on the \RFC{4944}.
     *
     * @param mac the MAC address (16 bits).
     * @return autoconfigured link-local IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredLinkLocalAddress(Mac16Address mac);

    /**
     * @brief Make the autoconfigured link-local IPv6 address with Mac48Address.
     *
     * The EUI-64 scheme used is based on \RFC{2464}.
     *
     * @param mac the MAC address (48 bits).
     * @return autoconfigured link-local IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredLinkLocalAddress(Mac48Address mac);

    /**
     * @brief Make the autoconfigured link-local IPv6 address with Mac64Address.
     * @param mac the MAC address (64 bits).
     * @return autoconfigured link-local IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredLinkLocalAddress(Mac64Address mac);

    /**
     * @brief Make the autoconfigured link-local IPv6 address with Mac8Address.
     *
     * The EUI-64 scheme used is loosely based on the \RFC{2464}.
     *
     * @param mac the MAC address (8 bits).
     * @return autoconfigured link-local IPv6 address
     */
    static Ipv6Address MakeAutoconfiguredLinkLocalAddress(Mac8Address mac);

    /**
     * @brief Print this address to the given output stream.
     *
     * The print format is in the typical "2001:660:4701::1".
     * @param os the output stream to which this Ipv6Address is printed
     */
    void Print(std::ostream& os) const;

    /**
     * @brief If the IPv6 address is localhost (::1).
     * @return true if localhost, false otherwise
     */
    bool IsLocalhost() const;

    /**
     * @brief If the IPv6 address is multicast (ff00::/8).
     * @return true if multicast, false otherwise
     */
    bool IsMulticast() const;

    /**
     * @brief If the IPv6 address is link-local multicast (ff02::/16).
     * @return true if link-local multicast, false otherwise
     */
    bool IsLinkLocalMulticast() const;

    /**
     * @brief If the IPv6 address is "all nodes multicast" (ff02::1/8).
     * @return true if "all nodes multicast", false otherwise
     */
    bool IsAllNodesMulticast() const;

    /**
     * @brief If the IPv6 address is "all routers multicast" (ff02::2/8).
     * @return true if "all routers multicast", false otherwise
     */
    bool IsAllRoutersMulticast() const;

    /**
     * @brief If the IPv6 address is a link-local address (fe80::/64).
     * @return true if the address is link-local, false otherwise
     */
    bool IsLinkLocal() const;

    /**
     * @brief If the IPv6 address is a Solicited multicast address.
     * @return true if it is, false otherwise
     */
    bool IsSolicitedMulticast() const;

    /**
     * @brief If the IPv6 address is the "Any" address.
     * @return true if it is, false otherwise
     */
    bool IsAny() const;

    /**
     * @brief If the IPv6 address is a documentation address (2001:DB8::/32).
     * @return true if the address is documentation, false otherwise
     */
    bool IsDocumentation() const;

    /**
     * @brief Compares an address and a prefix.
     * @param prefix the prefix to compare with
     * @return true if the address has the given prefix
     */
    bool HasPrefix(const Ipv6Prefix& prefix) const;

    /**
     * @brief Combine this address with a prefix.
     * @param prefix a IPv6 prefix
     * @return an IPv6 address that is this address combined
     * (bitwise AND) with a prefix, yielding an IPv6 network address.
     */
    Ipv6Address CombinePrefix(const Ipv6Prefix& prefix) const;

    /**
     * @brief If the Address matches the type.
     * @param address other address
     * @return true if the type matches, false otherwise
     */
    static bool IsMatchingType(const Address& address);

    /**
     * @brief If the address is an IPv4-mapped address
     * @return true if address is an IPv4-mapped address, otherwise false.
     */
    bool IsIpv4MappedAddress() const;

    /**
     * @brief Convert to Address object
     */
    operator Address() const;

    /**
     * @brief Convert the Address object into an Ipv6Address ones.
     * @param address address to convert
     * @return an Ipv6Address
     */
    static Ipv6Address ConvertFrom(const Address& address);

    /**
     * @brief convert the IPv6Address object to an Address object.
     * @return the Address object corresponding to this object.
     */
    Address ConvertTo() const;

    /**
     * @return true if address is initialized (i.e., set to something), false otherwise
     */
    NS_DEPRECATED_3_47("Use IsAny or std::optional")
    bool IsInitialized() const;

    /**
     * @brief Get the 0 (::) Ipv6Address.
     * @return the :: Ipv6Address representation
     */
    static Ipv6Address GetZero();

    /**
     * @brief Get the "any" (::) Ipv6Address.
     * @return the "any" (::) Ipv6Address
     */
    static Ipv6Address GetAny();

    /**
     * @brief Get the "all nodes multicast" address.
     * @return the "ff02::2/8" Ipv6Address representation
     */
    static Ipv6Address GetAllNodesMulticast();

    /**
     * @brief Get the "all routers multicast" address.
     * @return the "ff02::2/8" Ipv6Address representation
     */
    static Ipv6Address GetAllRoutersMulticast();

    /**
     * @brief Get the "all hosts multicast" address.
     * @return the "ff02::3/8" Ipv6Address representation
     */
    static Ipv6Address GetAllHostsMulticast();

    /**
     * @brief Get the loopback address.
     * @return the "::1/128" Ipv6Address representation.
     */
    static Ipv6Address GetLoopback();

    /**
     * @brief Get the "all-1" IPv6 address (ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff).
     * @return all-1 Ipv6Address representation
     */
    static Ipv6Address GetOnes();

    /**
     * @brief Get the bytes corresponding to the address.
     * @param buf buffer to store the data
     */
    void GetBytes(uint8_t buf[16]) const;

    /**
     * @brief Get the address hash.
     * @return the address hash
     */
    uint32_t GetHash() const;

    /**
     * @brief Three-way comparison operator.
     *
     * @param other the other address to compare with
     * @returns comparison result
     */
    inline constexpr std::strong_ordering operator<=>(const Ipv6Address& other) const
    {
        return m_address <=> other.m_address;
    }

    /**
     * @brief Equal to operator.
     *
     * @param other the other address to compare with
     * @returns true if the operands are equal.
     */
    inline constexpr bool operator==(const Ipv6Address& other) const
    {
        return m_address == other.m_address;
    }

  private:
    /**
     * @brief Mix hash keys in-place for lookuphash
     *
     * @param a first word of the hash key
     * @param b second word of the hash key
     * @param c third word of the hash key
     */
    static void MixHashKey(uint32_t& a, uint32_t& b, uint32_t& c);

    /**
     * @brief Generate and return a hash key.
     * @return hash
     * @note Adapted from Jens Jakobsen implementation (chillispot).
     */
    uint32_t GenerateHash() const;

    /**
     * @brief Return the Type of address.
     * @return type of address
     */
    static uint8_t GetType();

    /**
     * @brief The address representation on 128 bits (16 bytes).
     */
    std::array<uint8_t, 16> m_address{};

    /**
     * @brief address hash.
     */
    mutable std::optional<uint32_t> m_hash;
};

/**
 * @ingroup address
 * @class Ipv6Prefix
 * @brief Describes an IPv6 prefix. It is just a bitmask like Ipv4Mask.
 * @see Ipv6Address
 * @see attribute_Ipv6Prefix
 */
class Ipv6Prefix
{
  public:
    /**
     * @brief Default constructor.
     *
     * The default prefix is empty, corresponding to /0
     */
    Ipv6Prefix() = default;

    /**
     * @brief Constructs an Ipv6Prefix by using the input 16 bytes.
     *
     * The prefix length is calculated as the minimum prefix length, i.e.,
     * 2001:db8:cafe:: will have a 47 bit prefix length (0xE is 0b1110).
     *
     * @param prefix the 128-bit prefix
     */
    Ipv6Prefix(uint8_t prefix[16]);

    /**
     * @brief Constructs an Ipv6Prefix by using the input string.
     *
     * The prefix length is calculated as the minimum prefix length, i.e.,
     * 2001:db8:cafe:: will have a 47 bit prefix length.
     *
     * @param prefix the 128-bit prefix
     */
    Ipv6Prefix(const char* prefix);

    /**
     * @brief Constructs an Ipv6Prefix by using the input 16 bytes.
     * @param prefix the 128-bit prefix
     * @param prefixLength the prefix length
     */
    Ipv6Prefix(uint8_t prefix[16], uint8_t prefixLength);

    /**
     * @brief Constructs an Ipv6Prefix by using the input string.
     * @param prefix the 128-bit prefix
     * @param prefixLength the prefix length
     */
    Ipv6Prefix(const char* prefix, uint8_t prefixLength);

    /**
     * @brief Constructs an Ipv6Prefix by using the input number of bits.
     * @param prefix number of bits of the prefix (0 - 128)
     * @note A valid number of bits is between 0 and 128).
     */
    Ipv6Prefix(uint8_t prefix);

    /**
     * @brief Check whether two addresses have the same bits in the prefix
     * portion of their addresses.
     *
     * If the prefix length is 0, this method will return true regardless
     * of the two address argument values.  If the prefix length is 128,
     * this method will require that the two address arguments are the same.
     *
     * @param a first address to compare
     * @param b second address to compare
     * @return true if both addresses are equal in their masked bits,
     * corresponding to the prefix length.
     */
    bool IsMatch(Ipv6Address a, Ipv6Address b) const;

    /**
     * @brief Get the bytes corresponding to the prefix.
     * @param buf buffer to store the data
     */
    void GetBytes(uint8_t buf[16]) const;

    /**
     * @brief Convert the Prefix into an IPv6 Address.
     * @return an IPv6 address representing the prefix
     */
    Ipv6Address ConvertToIpv6Address() const;

    /**
     * @brief Get prefix length.
     * @return prefix length
     */
    uint8_t GetPrefixLength() const;

    /**
     * @brief Set prefix length.
     * @param prefixLength the prefix length
     */
    void SetPrefixLength(uint8_t prefixLength);

    /**
     * @brief Get the minimum prefix length, i.e., 128 - the length of the largest sequence
     * trailing zeroes.
     * @return minimum prefix length
     */
    uint8_t GetMinimumPrefixLength() const;

    /**
     * @brief Print this address to the given output stream.
     *
     * The print format is in the typical "2001:660:4701::1".
     * @param os the output stream to which this Ipv6Address is printed
     */
    void Print(std::ostream& os) const;

    /**
     * @brief Get the loopback prefix ( /128).
     * @return a Ipv6Prefix corresponding to loopback prefix
     */
    static Ipv6Prefix GetLoopback();

    /**
     * @brief Get the "all-1" IPv6 mask (ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff).
     * @return /128 Ipv6Prefix representation
     */
    static Ipv6Prefix GetOnes();

    /**
     * @brief Get the zero prefix ( /0).
     * @return an Ipv6Prefix
     */
    static Ipv6Prefix GetZero();

    /**
     * @brief Three-way comparison operator.
     *
     * @param other the other prefix to compare with
     * @returns comparison result
     */
    constexpr std::strong_ordering operator<=>(const Ipv6Prefix& other) const = default;

  private:
    /**
     * @brief The prefix representation.
     */
    std::array<uint8_t, 16> m_prefix{};

    /**
     * @brief The prefix length.
     */
    uint8_t m_prefixLength{0};
};

ATTRIBUTE_HELPER_HEADER(Ipv6Address);
ATTRIBUTE_HELPER_HEADER(Ipv6Prefix);

/**
 * @brief Stream insertion operator.
 *
 * @param os the reference to the output stream
 * @param address the Ipv6Address
 * @returns the reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, const Ipv6Address& address);

/**
 * @brief Stream insertion operator.
 *
 * @param os the reference to the output stream
 * @param prefix the Ipv6Prefix
 * @returns the reference to the output stream
 */
std::ostream& operator<<(std::ostream& os, const Ipv6Prefix& prefix);

/**
 * @brief Stream extraction operator.
 *
 * @param is the reference to the input stream
 * @param address the Ipv6Address
 * @returns the reference to the input stream
 */
std::istream& operator>>(std::istream& is, Ipv6Address& address);

/**
 * @brief Stream extraction operator.
 *
 * @param is the reference to the input stream
 * @param prefix the Ipv6Prefix
 * @returns the reference to the input stream
 */
std::istream& operator>>(std::istream& is, Ipv6Prefix& prefix);

/**
 * @class Ipv6AddressHash
 * @brief Hash function class for IPv6 addresses.
 */
class NS_DEPRECATED_3_47("Unnecessary thanks to std::hash specialization, remove") Ipv6AddressHash
{
  public:
    /**
     * @brief Returns the hash of an IPv6 address.
     * @param x IPv6 address to hash
     * @returns the hash of the address
     */
    size_t operator()(const Ipv6Address& x) const;
};

} /* namespace ns3 */

namespace std
{

/**
 * @brief Hash function class for IPv6 addresses.
 */
template <>
struct hash<ns3::Ipv6Address>
{
    /**
     * @brief Returns the hash of an IPv6 address.
     * @param addr IPv6 address to hash
     * @returns the hash of the address
     */
    size_t operator()(const ns3::Ipv6Address& addr) const
    {
        return addr.GetHash();
    }
};
} // namespace std

#endif /* IPV6_ADDRESS_H */
