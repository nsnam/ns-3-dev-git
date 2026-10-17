//
// Copyright (c) 2008-2010 INESC Porto
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gustavo J. A. M. Carneiro  <gjc@inescporto.pt> <gjcarneiro@gmail.com>
//

#ifndef NS3_SEQ_NUM_H
#define NS3_SEQ_NUM_H

#include "ns3/assert.h"
#include "ns3/type-name.h"

#include <compare>
#include <concepts>
#include <iostream>
#include <limits>
#include <stdint.h>

namespace ns3
{

/**
 * @ingroup network
 * @defgroup seq-counters Sequence Counter
 * @brief "sequence number" classes
 */

/**
 * @ingroup seq-counters
 * @brief Generic "sequence number" class
 *
 * This class can be used to handle sequence numbers.  In networking
 * protocols, sequence numbers are fixed precision integer numbers
 * that are used to order events relative to each other.  A sequence
 * number is expected to increase over time but, since it has a
 * limited number of bits, the number will "wrap around" from the
 * maximum value that can represented with the given number of bits
 * back to zero.  For this reason, comparison of two sequence numbers,
 * and subtraction, is non-trivial.  The SequenceNumber class behaves
 * like a number, with the usual arithmetic operators implemented, but
 * knows how to correctly compare and subtract sequence numbers.
 *
 * Sequence number operations are defined in \RFC{1982}. However, the
 * RFC leaves as implementation-dependent the case of two sequence numbers
 * whose difference is equal to half of the possible range (e.g., 0 and
 * 128 for a 8-bit sequence number).
 * \RFC{3626} (OLSR) fixes this ambiguity by defining the relationship.
 * This implementation follows the \RFC{3626} definition. Hence, in this
 * example, 128 is less than 0.
 *
 * The relationship table for a 4-bit sequence number is the following:
 *   - 4-bit sequence number value 0 = 0
 *   - 4-bit sequence number value 1 > 0
 *   - 4-bit sequence number value 2 > 0
 *   - 4-bit sequence number value 3 > 0
 *   - 4-bit sequence number value 4 > 0
 *   - 4-bit sequence number value 5 > 0
 *   - 4-bit sequence number value 6 > 0
 *   - 4-bit sequence number value 7 > 0
 *   - 4-bit sequence number value 8 < 0
 *   - 4-bit sequence number value 9 < 0
 *   - 4-bit sequence number value 10 < 0
 *   - 4-bit sequence number value 11 < 0
 *   - 4-bit sequence number value 12 < 0
 *   - 4-bit sequence number value 13 < 0
 *   - 4-bit sequence number value 14 < 0
 *   - 4-bit sequence number value 15 < 0
 *
 * This is a templated class.  To use it you need to supply one
 * fundamental type as a template parameter: NUMERIC_TYPE.
 * The second parameter NUM_BITS is optional, and represents
 * the number of bits used in the SequenceNumber.
 * It must be equal or less than the number of available bits in the
 * NUMERIC_TYPE.
 *
 * For instance, SequenceNumber<uint32_t> gives
 * you a 32-bit sequence number, while SequenceNumber<uint16_t, 10>
 * is a 10-bit one.
 *
 * For your convenience, SequenceNumber32, SequenceNumber16, and
 * SequenceNumber8 are defined as typedefs.
 *
 * You can safely assign a value to a sequence number, provided that it
 * is in the allowed range. E.g, the following code will raise an assert:
 * @code{.cpp}
 * // 42 is representable in uint8_t, but needs more than 4 bits
 * SequenceNumber<uint8_t, 4> seqNum{42};
 * @endcode
 *
 * Sequence numbers can be printed, but they can not be automatically
 * converted to an integer type. Furthermore, the conversion from an
 * integer type must be explicit, e.g., it is not possible to compare
 * a sequence number and an integer:
 * @code{.cpp}
 * SequenceNumber<uint8_t, 4> seqNum1{0}; // OK
 * SequenceNumber<uint8_t, 4> seqNum2{1}; // OK
 * seqNum2 = 2;                           // OK
 * std::cout << "seqNum1 < seqNum2? " << std::boolalpha << (seqNum1 < seqNum2) << "\n"; // OK
 * // std::cout << "seqNum1 < 1? " << std::boolalpha << (seqNum1 < 1) << "\n";          // ERROR
 * @endcode
 * The above limitation is to avoid mistakes, since integers and sequence numbers have very
 * different comparison rules.
 *
 * @note Due to the internal representation and the semantics of `operator -` between two
 * sequence numbers, `SequenceNumber<uint64_t>` are disallowed.
 */
template <typename NUMERIC_TYPE, uint8_t NUM_BITS = std::numeric_limits<NUMERIC_TYPE>::digits>
    requires std::unsigned_integral<std::remove_cv_t<NUMERIC_TYPE>> && (NUM_BITS < 64) &&
             (NUM_BITS <= std::numeric_limits<NUMERIC_TYPE>::digits)

class SequenceNumber
{
  public:
    /**
     * SIGNED_TYPE is used in some operators, and is the signed counterpart
     * of NUMERIC_TYPE
     */
    using SIGNED_TYPE = std::make_signed_t<NUMERIC_TYPE>;

    /// Total number of sequence numbers.
    static constexpr size_t N_SEQUENCE_NUMBERS = 1 << NUM_BITS;

    /// Number of bits used by the sequence number
    static constexpr NUMERIC_TYPE N_BITS{NUM_BITS};

    SequenceNumber() = default;

    /**
     * @brief Constructs a SequenceNumber with the given value
     * @param value the sequence number value
     */
    explicit SequenceNumber(NUMERIC_TYPE value)
    {
        NS_ASSERT_MSG(value <= MAX_VALUE,
                      "SequenceNumber: " << static_cast<uint64_t>(value)
                                         << " is outside the allowed range [0, "
                                         << static_cast<uint64_t>(MAX_VALUE) << "]");
        m_value = value;
    }

    /**
     * @brief Constructs a SequenceNumber from an assignment of given value
     * @param value sequence number to copy
     * @returns reference to the assignee
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS>& operator=(NUMERIC_TYPE value)
    {
        NS_ASSERT_MSG(value <= MAX_VALUE,
                      "SequenceNumber: " << static_cast<uint64_t>(value)
                                         << " is outside the allowed range [0, "
                                         << static_cast<uint64_t>(MAX_VALUE) << "]");
        m_value = value;
        return *this;
    }

    /**
     * @brief Extracts the numeric value of the sequence number
     * @returns the sequence number value
     */
    constexpr NUMERIC_TYPE GetValue() const
    {
        return m_value;
    }

    /**
     * @brief Prefix increment operator
     * @returns incremented sequence number
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator++()
    {
        // The following is equivalent to a modulus
        m_value = (m_value + 1) & MAX_VALUE;
        return *this;
    }

    /**
     * @brief Postfix increment operator
     * @returns incremented sequence number
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator++(int)
    {
        SequenceNumber<NUMERIC_TYPE, NUM_BITS> retval(m_value);
        m_value = (m_value + 1) & MAX_VALUE;
        return retval;
    }

    /**
     * @brief Prefix decrement operator
     * @returns decremented sequence number
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator--()
    {
        m_value = (m_value - 1) & MAX_VALUE;
        return *this;
    }

    /**
     * @brief Postfix decrement operator
     * @returns decremented sequence number
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator--(int)
    {
        SequenceNumber<NUMERIC_TYPE, NUM_BITS> retval(m_value);
        m_value = (m_value - 1) & MAX_VALUE;
        return retval;
    }

    /**
     * @brief Plus equals operator
     * @param value value to add to sequence number
     * @returns incremented sequence number
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS>& operator+=(SIGNED_TYPE value)
    {
        m_value += value;
        m_value &= MAX_VALUE;

        return *this;
    }

    /**
     * @brief Minus equals operator
     * @param value value to subtract from sequence number
     * @returns decremented sequence number
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS>& operator-=(SIGNED_TYPE value)
    {
        m_value -= value;
        m_value &= MAX_VALUE;
        return *this;
    }

    /**
     * @brief Operator defining addition of two sequence numbers.
     *
     * @note: this operator ignores the logical relationship between
     * the operands (i.e., lesser, greater) and simply adds the respective
     * values. If possible, avoid to use it.
     *
     * @param other sequence number added to this
     * @returns sequence number representing sum
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator+(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>& other) const
    {
        NUMERIC_TYPE val = m_value + other.m_value;
        val &= MAX_VALUE;
        return SequenceNumber<NUMERIC_TYPE, NUM_BITS>(val);
    }

    /**
     * @brief Addition operator for adding numeric value to sequence number.
     *
     * This function is syntactic sugar for:
     * @code{.cpp}
     * SequenceNumber<uint8_t> seqNum(0u);
     * for (auto index=0; index<value, index++)
     * {
     *   seqNum++;
     * }
     * @endcode
     *
     * @param delta value to add to sequence number
     * @returns sequence number representing sum
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator+(SIGNED_TYPE delta) const
    {
        NUMERIC_TYPE val = m_value + delta;
        val &= MAX_VALUE;
        return SequenceNumber<NUMERIC_TYPE, NUM_BITS>(val);
    }

    /**
     * @brief Subtraction operator for subtracting numeric value from sequence number.
     *
     * This function is syntactic sugar for:
     * @code{.cpp}
     * SequenceNumber<uint8_t> seqNum(0u);
     * for (auto index=0; index<value, index++)
     * {
     *   seqNum--;
     * }
     * @endcode
     *
     * @param delta value to subtract from sequence number
     * @returns sequence number representing difference
     */
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator-(SIGNED_TYPE delta) const
    {
        NUMERIC_TYPE val = m_value - delta;
        val &= MAX_VALUE;
        return SequenceNumber<NUMERIC_TYPE, NUM_BITS>(val);
    }

    /**
     * @brief Subtraction operator for subtracting a sequence number from another sequence number.
     *
     * This function returns a signed distance between two sequence numbers. The dynamic range
     * of the difference might be larger than the underlying unit type. I.e., the following
     * code is a bug:
     * @code{.cpp}
     * SequenceNumber<uint8_t> seqNum1(0u);
     * SequenceNumber<uint8_t> seqNum2(128u);
     * int8_t difference = seqNum1 - seqNum2;
     * @endcode
     * because the result will be negative, even if the first operand is greater than the second.
     *
     * @param other sequence number to subtract from this sequence number
     * @returns numeric value representing the signed difference
     */
    int64_t operator-(const SequenceNumber<NUMERIC_TYPE, NUM_BITS>& other) const
    {
        int64_t diff = static_cast<int64_t>(m_value) - static_cast<int64_t>(other.m_value);
        static constexpr auto MAX_VALUE_D = static_cast<int64_t>(MAX_VALUE);
        static constexpr auto HALF_MAX_VALUE_D = static_cast<int64_t>(HALF_MAX_VALUE);

        if (diff > HALF_MAX_VALUE_D)
        {
            return (diff - 1 - MAX_VALUE_D);
        }
        else if (diff < -HALF_MAX_VALUE_D)
        {
            return (diff + 1 + MAX_VALUE_D);
        }
        return diff;
    }

    /**
     * @brief Equality operator for comparing sequence number
     * @param other sequence number to compare to this sequence number
     * @returns true if the sequence numbers are equal
     */
    constexpr bool operator==(const SequenceNumber<NUMERIC_TYPE, NUM_BITS>& other) const
    {
        return m_value == other.m_value;
    }

    /**
     * Spaceship comparison operator. All the other comparison operators
     * are automatically generated from this one.
     *
     * Here is the critical part, how the comparison is made taking into
     * account wrap-around. From RFC 3626:
     *
     * The sequence number S1 is said to be "greater than" the sequence
     * number S2 if:
     *      S1 > S2 AND S1 - S2 <= MAXVALUE/2 OR
     *      S2 > S1 AND S2 - S1 > MAXVALUE/2
     *
     * @param other sequence number to compare to this one
     * @returns The result of the comparison.
     */
    constexpr std::strong_ordering operator<=>(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>& other) const
    {
        if (m_value == other.m_value)
        {
            return std::strong_ordering::equivalent;
        }
        if (((m_value > other.m_value) && (m_value - other.m_value) <= HALF_MAX_VALUE) ||
            ((other.m_value > m_value) && (other.m_value - m_value) > HALF_MAX_VALUE))
        {
            return std::strong_ordering::greater;
        }
        return std::strong_ordering::less;
    }

    /**
     * @brief For printing sequence number
     * @param os output stream
     * @param val sequence number to display
     * @returns output stream os
     */
    template <typename NUMERIC_TYPE2, uint8_t NUM_BITS2>
    friend std::ostream& operator<<(std::ostream& os,
                                    const SequenceNumber<NUMERIC_TYPE2, NUM_BITS2>& val);

    /**
     * @brief For loading sequence number from input streams
     * @param is input stream
     * @param val sequence number to load
     * @returns input stream is
     */
    template <typename NUMERIC_TYPE2, uint8_t NUM_BITS2>
    friend std::istream& operator>>(std::istream& is,
                                    const SequenceNumber<NUMERIC_TYPE2, NUM_BITS2>& val);

  public:
    // Unimplemented operators
    SequenceNumber<NUMERIC_TYPE, NUM_BITS>& operator+=(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS>& operator-=(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator*(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator/(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator%(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    bool operator!() const = delete;
    bool operator&&(const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    bool operator||(const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator~() const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator&(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator|(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator^(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator<<(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    SequenceNumber<NUMERIC_TYPE, NUM_BITS> operator>>(
        const SequenceNumber<NUMERIC_TYPE, NUM_BITS>&) const = delete;
    int operator*() = delete;

  private:
    NUMERIC_TYPE m_value{0}; //!< Sequence number value

    /// Maximum representable value
    static constexpr NUMERIC_TYPE MAX_VALUE = NUM_BITS < std::numeric_limits<NUMERIC_TYPE>::digits
                                                  ? (NUMERIC_TYPE{1} << NUM_BITS) - 1
                                                  : std::numeric_limits<NUMERIC_TYPE>::max();

    /// Half the maximum representable value, used internally.
    static constexpr NUMERIC_TYPE HALF_MAX_VALUE = MAX_VALUE / 2;
};

/**
 * @brief Stream insertion operator.
 *
 * @param os the stream
 * @param val the value
 * @returns a reference to the stream
 */
template <typename NUMERIC_TYPE, uint8_t NUM_BITS>
std::ostream&
operator<<(std::ostream& os, const SequenceNumber<NUMERIC_TYPE, NUM_BITS>& val)
{
    os << static_cast<uint64_t>(val.m_value);
    return os;
}

/**
 * @brief Stream extraction operator.
 *
 * @param is the stream
 * @param val the value
 * @returns a reference to the stream
 */
template <typename NUMERIC_TYPE, uint8_t NUM_BITS>
std::istream&
operator>>(std::istream& is, const SequenceNumber<NUMERIC_TYPE, NUM_BITS>& val)
{
    NUMERIC_TYPE value;
    is >> value;

    NS_ASSERT_MSG(value <= val.MAX_VALUE,
                  "SequenceNumber: " << static_cast<uint64_t>(value)
                                     << " is outside the allowed range [0, "
                                     << static_cast<uint64_t>(val.MAX_VALUE) << "]");

    val.m_value = value;
    return is;
}

/**
 * @ingroup seq-counters
 * 32 bit Sequence number.
 */
using SequenceNumber32 = SequenceNumber<uint32_t>;
/**
 * @ingroup seq-counters
 * 16 bit Sequence number.
 */
using SequenceNumber16 = SequenceNumber<uint16_t>;
/**
 * @ingroup seq-counters
 * 8 bit Sequence number.
 */
using SequenceNumber8 = SequenceNumber<uint8_t>;

namespace TracedValueCallback
{

/**
 * @ingroup seq-counters
 * TracedValue callback signature for SequenceNumber32
 *
 * @param [in] oldValue original value of the traced variable
 * @param [in] newValue new value of the traced variable
 */
using SequenceNumber32 = void (*)(SequenceNumber32 oldValue, SequenceNumber32 newValue);

/**
 * @ingroup seq-counters
 * TracedValue callback signature for SequenceNumber16
 *
 * @param [in] oldValue original value of the traced variable
 * @param [in] newValue new value of the traced variable
 */
using SequenceNumber16 = void (*)(SequenceNumber16 oldValue, SequenceNumber16 newValue);

/**
 * @ingroup seq-counters
 * TracedValue callback signature for SequenceNumber8
 *
 * @param [in] oldValue original value of the traced variable
 * @param [in] newValue new value of the traced variable
 */
using SequenceNumber8 = void (*)(SequenceNumber8 oldValue, SequenceNumber8 newValue);

} // namespace TracedValueCallback

/**
 * @ingroup seq-counters
 *
 * ns3::TypeNameGet<SequenceNumber32>() specialization.
 * @returns The type name as a string.
 */
TYPENAMEGET_DEFINE(SequenceNumber32);

/**
 * @ingroup seq-counters
 *
 * ns3::TypeNameGet<SequenceNumber16>() specialization.
 * @returns The type name as a string.
 */
TYPENAMEGET_DEFINE(SequenceNumber16);

/**
 * @ingroup seq-counters
 *
 * ns3::TypeNameGet<SequenceNumber8>() specialization.
 * @returns The type name as a string.
 */
TYPENAMEGET_DEFINE(SequenceNumber8);

} // namespace ns3

#endif /* NS3_SEQ_NUM_H */
