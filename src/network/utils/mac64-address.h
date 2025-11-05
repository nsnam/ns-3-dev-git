/*
 * Copyright (c) 2007 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef MAC64_ADDRESS_H
#define MAC64_ADDRESS_H

#include "ipv4-address.h"
#include "ipv6-address.h"

#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"

#include <array>
#include <compare>
#include <ostream>
#include <stdint.h>

namespace ns3
{

class Address;

/**
 * @ingroup address
 *
 * @brief an EUI-64 address
 *
 * This class can contain 64 bit IEEE addresses.
 *
 * @see attribute_Mac64Address
 */
class Mac64Address
{
  public:
    Mac64Address() = default;
    /**
     * @param str a string representing the new Mac64Address
     *
     * The format of the string is "xx:xx:xx:xx:xx:xx:xx:xx"
     */
    Mac64Address(const char* str);

    /**
     * @param addr The 64 bit unsigned integer used to create a Mac64Address object.
     *
     * Create a Mac64Address from an 64 bit unsigned integer.
     */
    Mac64Address(uint64_t addr);

    /**
     * @param buffer address in network order
     *
     * Copy the input address to our internal buffer.
     */
    void CopyFrom(const uint8_t buffer[8]);
    /**
     * @param buffer address in network order
     *
     * Copy the internal address to the input buffer.
     */
    void CopyTo(uint8_t buffer[8]) const;
    /**
     * @returns a new Address instance
     *
     * Convert an instance of this class to a polymorphic Address instance.
     */
    operator Address() const;
    /**
     * @param address a polymorphic address
     * @returns a new Mac64Address from the polymorphic address
     *
     * This function performs a type check and asserts if the
     * type of the input address is not compatible with an
     * Mac64Address.
     */
    static Mac64Address ConvertFrom(const Address& address);
    /**
     * @returns a new Address instance
     *
     * Convert an instance of this class to a polymorphic Address instance.
     */
    Address ConvertTo() const;

    /**
     * @return the mac address in a 64 bit unsigned integer.
     *
     * Convert an instance of this class to a 64 bit unsigned integer.
     */
    uint64_t ConvertToInt() const;

    /**
     * @param address address to test
     * @returns true if the address matches, false otherwise.
     */
    static bool IsMatchingType(const Address& address);
    /**
     * Allocate a new Mac64Address.
     * @returns newly allocated mac64Address
     */
    static Mac64Address Allocate();

    /**
     * Reset the Mac64Address allocation index.
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
     * Spaceship comparison operator. All the other comparison operators
     * are automatically generated from this one.
     *
     * @param other address to compare to this one
     * @returns The result of the comparison.
     */
    constexpr std::strong_ordering operator<=>(const Mac64Address& other) const = default;

  private:
    /**
     * @brief Return the Type of address.
     * @return type of address
     */
    static uint8_t GetType();

    /**
     * @brief Stream insertion operator.
     *
     * @param os the stream
     * @param address the address
     * @returns a reference to the stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Mac64Address& address);

    /**
     * @brief Stream extraction operator.
     *
     * @param is the stream
     * @param address the address
     * @returns a reference to the stream
     */
    friend std::istream& operator>>(std::istream& is, Mac64Address& address);

    static uint64_t m_allocationIndex;  //!< Address allocation index
    std::array<uint8_t, 8> m_address{}; //!< Address value
};

/**
 * @class ns3::Mac64AddressValue
 * @brief hold objects of type ns3::Mac64Address
 */

ATTRIBUTE_HELPER_HEADER(Mac64Address);

std::ostream& operator<<(std::ostream& os, const Mac64Address& address);
std::istream& operator>>(std::istream& is, Mac64Address& address);

} // namespace ns3

#endif /* MAC64_ADDRESS_H */
