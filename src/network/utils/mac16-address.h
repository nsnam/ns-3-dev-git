/*
 * Copyright (c) 2007 INRIA
 * Copyright (c) 2011 The Boeing Company
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
 */
#ifndef MAC16_ADDRESS_H
#define MAC16_ADDRESS_H

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
 * \ingroup address
 *
 * This class can contain 16 bit addresses.
 *
 * \see attribute_Mac16Address
 */
class Mac16Address
{
  public:
    Mac16Address();
    /**
     * \param str a string representing the new Mac16Address
     *
     */
    Mac16Address(const char* str);

    /**
     * \param addr The 16 bit unsigned integer used to create a Mac16Address object.
     *
     * Create a Mac16Address from an 16 bit unsigned integer.
     */
    Mac16Address(uint16_t addr);

    /**
     * \param buffer address in network order
     *
     * Copy the input address to our internal buffer.
     */
    void CopyFrom(const uint8_t buffer[2]);

    /**
     * \param buffer address in network order
     *
     * Copy the internal address to the input buffer.
     */
    void CopyTo(uint8_t buffer[2]) const;

    /**
     * \returns a new Address instance
     *
     * Convert an instance of this class to a polymorphic Address instance.
     */
    operator Address() const;

    /**
     * \param address a polymorphic address
     * \returns a new Mac16Address from the polymorphic address
     *
     * This function performs a type check and asserts if the
     * type of the input address is not compatible with an
     * Mac16Address.
     */
    static Mac16Address ConvertFrom(const Address& address);

    /**
     * \returns a new Address instance
     *
     * Convert an instance of this class to a polymorphic Address instance.
     */
    Address ConvertTo() const;

    /**
     * \return the mac address in a 16 bit unsigned integer
     *
     * Convert an instance of this class to a 16 bit unsigned integer.
     */
    uint16_t ConvertToInt() const;

    /**
     * \param address address to test
     * \returns true if the address matches, false otherwise.
     */
    static bool IsMatchingType(const Address& address);

    /**
     * Allocate a new Mac16Address.
     * \returns newly allocated mac16Address
     */
    static Mac16Address Allocate();

    /**
     * Reset the Mac16Address allocation index.
     *
     * This function resets (to zero) the global integer
     * that is used for unique address allocation.
     * It is automatically called whenever
     * \code
     * SimulatorDestroy ();
     * \endcode
     * is called.  It may also be optionally called
     * by user code if there is a need to force a reset
     * of this allocation index.
     */
    static void ResetAllocationIndex();

    /**
     * \returns the broadcast address (0xFFFF)
     */
    static Mac16Address GetBroadcast();

    /**
     * Returns the multicast address associated with an IPv6 address
     * according to RFC 4944 Section 9.
     * An IPv6 packet with a multicast destination address (DST),
     * consisting of the sixteen octets DST[1] through DST[16], is
     * transmitted to the following 802.15.4 16-bit multicast address:

    \verbatim
      0                   1
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     |1 0 0|DST[15]* |   DST[16]     |
     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    \endverbatim

     * Here, DST[15]* refers to the last 5 bits in octet DST[15], that is,
     * bits 3-7 within DST[15].  The initial 3-bit pattern of "100" follows
     * the 16-bit address format for multicast addresses (Section 12).
     *
     * \param address base IPv6 address
     * \returns the multicast 16-bit address.
     */

    static Mac16Address GetMulticast(Ipv6Address address);

    /**
     * Checks if the address is a broadcast address according
     * to 802.15.4 scheme (i.e., 0xFFFF).
     *
     * \returns true if the address is 0xFFFF
     */
    bool IsBroadcast() const;

    /**
     * Checks if the address is a multicast address according
     * to RFC 4944 Section 9 (i.e., if its first 3 bits are 100).
     *
     * \returns true if the address is in the range 0x8000 - 0x9FFF
     */
    bool IsMulticast() const;

  private:
    /**
     * \brief Return the Type of address.
     * \return type of address
     */
    static uint8_t GetType();

    /**
     * \brief Equal to operator.
     *
     * \param a the first operand
     * \param b the first operand
     * \returns true if the operands are equal
     */
    friend bool operator==(const Mac16Address& a, const Mac16Address& b);

    /**
     * \brief Not equal to operator.
     *
     * \param a the first operand
     * \param b the first operand
     * \returns true if the operands are not equal
     */
    friend bool operator!=(const Mac16Address& a, const Mac16Address& b);

    /**
     * \brief Less than operator.
     *
     * \param a the first operand
     * \param b the first operand
     * \returns true if the operand a is less than operand b
     */
    friend bool operator<(const Mac16Address& a, const Mac16Address& b);

    /**
     * \brief Stream insertion operator.
     *
     * \param os the stream
     * \param address the address
     * \returns a reference to the stream
     */
    friend std::ostream& operator<<(std::ostream& os, const Mac16Address& address);

    /**
     * \brief Stream extraction operator.
     *
     * \param is the stream
     * \param address the address
     * \returns a reference to the stream
     */
    friend std::istream& operator>>(std::istream& is, Mac16Address& address);

    static uint64_t m_allocationIndex; //!< Address allocation index
    uint8_t m_address[2];              //!< address value
};

ATTRIBUTE_HELPER_HEADER(Mac16Address);

inline bool
operator==(const Mac16Address& a, const Mac16Address& b)
{
    return memcmp(a.m_address, b.m_address, 2) == 0;
}

inline bool
operator!=(const Mac16Address& a, const Mac16Address& b)
{
    return memcmp(a.m_address, b.m_address, 2) != 0;
}

inline bool
operator<(const Mac16Address& a, const Mac16Address& b)
{
    return memcmp(a.m_address, b.m_address, 2) < 0;
}

std::ostream& operator<<(std::ostream& os, const Mac16Address& address);
std::istream& operator>>(std::istream& is, Mac16Address& address);

} // namespace ns3

#endif /* MAC16_ADDRESS_H */
