/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef MAC48_ADDRESS_H
#define MAC48_ADDRESS_H

#include "ipv4-address.h"
#include "ipv6-address.h"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"

#include <ostream>
#include <stdint.h>

namespace ns3
{

class Address;

/**
 * @ingroup address
 *
 * @brief an EUI-48 address
 *
 * This class can contain 48 bit IEEE addresses.
 *
 * @see attribute_Mac48Address
 */
class Mac48Address
{
  public:
    Mac48Address() = default;
    /**
     * @param str a string representing the new Mac48Address
     *
     * The format of the string is "xx:xx:xx:xx:xx:xx"
     */
    Mac48Address(const char* str);

    /**
     * @param buffer address in network order
     *
     * Copy the input address to our internal buffer.
     */
    void CopyFrom(const uint8_t buffer[6]);
    /**
     * @param buffer address in network order
     *
     * Copy the internal address to the input buffer.
     */
    void CopyTo(uint8_t buffer[6]) const;

    /**
     * @returns a new Address instance
     *
     * Convert an instance of this class to a polymorphic Address instance.
     */
    operator Address() const;
    /**
     * @param address a polymorphic address
     * @returns a new Mac48Address from the polymorphic address
     *
     * This function performs a type check and asserts if the
     * type of the input address is not compatible with an
     * Mac48Address.
     */
    static Mac48Address ConvertFrom(const Address& address);
    /**
     * @returns a new Address instance
     *
     * Convert an instance of this class to a polymorphic Address instance.
     */
    Address ConvertTo() const;

    /**
     * @param address address to test
     * @returns true if the address matches, false otherwise.
     */
    static bool IsMatchingType(const Address& address);
    /**
     * Allocate a new Mac48Address.
     * @returns newly allocated mac48Address
     */
    static Mac48Address Allocate();

    /**
     * Reset the Mac48Address allocation index.
     *
     * This function resets (to zero) the global integer
     * that is used for unique address allocation.
     * It is automatically called whenever
     * @code
     * SimulatorDestroy ();
     * @endcode
     * is called.  It may also be optionally called
     * by user code if there is a need to force a reset
     * of this allocation index.
     */
    static void ResetAllocationIndex();

    /**
     * @returns true if this is a broadcast address, false otherwise.
     */
    bool IsBroadcast() const;

    /**
     * @returns true if the group bit is set, false otherwise.
     */
    bool IsGroup() const;

    /**
     * @returns the broadcast address
     */
    static Mac48Address GetBroadcast();

    /**
     * @param address base IPv4 address
     * @returns a multicast address
     */
    static Mac48Address GetMulticast(Ipv4Address address);

    /**
     * @brief Get multicast address from IPv6 address.
     * @param address base IPv6 address
     * @returns a multicast address
     */
    static Mac48Address GetMulticast(Ipv6Address address);

    /**
     * @returns the multicast prefix (01:00:5e:00:00:00).
     */
    static Mac48Address GetMulticastPrefix();

    /**
     * @brief Get the multicast prefix for IPv6 (33:33:00:00:00:00).
     * @returns a multicast address.
     */
    static Mac48Address GetMulticast6Prefix();

    /**
     * TracedCallback signature for Mac48Address
     *
     * @param [in] value Current value of the Mac48Address
     */
    typedef void (*TracedCallback)(Mac48Address value);

  private:
    /**
     * @brief Return the Type of address.
     * @return type of address
     */
    static uint8_t GetType();

    /**
     * @brief Equal to operator.
     *
     * @param a the first operand
     * @param b the first operand
     * @returns true if the operands are equal
     */
    friend bool operator==(const Mac48Address& a, const Mac48Address& b);

    /**
     * @brief Not equal to operator.
     *
     * @param a the first operand
     * @param b the first operand
     * @returns true if the operands are not equal
     */
    friend bool operator!=(const Mac48Address& a, const Mac48Address& b);

    /**
     * @brief Less than operator.
     *
     * @param a the first operand
     * @param b the first operand
     * @returns true if the operand a is less than operand b
     */
    friend bool operator<(const Mac48Address& a, const Mac48Address& b);

    /**
     * @brief Stream insertion operator.
     *
     * @param os the stream
     * @param address the address
     * @returns a reference to the stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Mac48Address& address);

    /**
     * @brief Stream extraction operator.
     *
     * @param is the stream
     * @param address the address
     * @returns a reference to the stream
     */
    friend std::istream& operator>>(std::istream& is, Mac48Address& address);

    static uint64_t m_allocationIndex; //!< Address allocation index
    uint8_t m_address[6]{0};           //!< Address value
};

ATTRIBUTE_HELPER_HEADER(Mac48Address);

inline bool
operator==(const Mac48Address& a, const Mac48Address& b)
{
    return memcmp(a.m_address, b.m_address, 6) == 0;
}

inline bool
operator!=(const Mac48Address& a, const Mac48Address& b)
{
    return memcmp(a.m_address, b.m_address, 6) != 0;
}

inline bool
operator<(const Mac48Address& a, const Mac48Address& b)
{
    return memcmp(a.m_address, b.m_address, 6) < 0;
}

std::ostream& operator<<(std::ostream& os, const Mac48Address& address);
std::istream& operator>>(std::istream& is, Mac48Address& address);

} // namespace ns3

#endif /* MAC48_ADDRESS_H */
