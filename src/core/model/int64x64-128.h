/*
 * Copyright (c) 2010 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef INT64X64_128_H
#define INT64X64_128_H

#include "ns3/core-config.h"

/**
 * @ingroup highprec
 * Use uint128_t for int64x64_t implementation
 */
#if defined(INT64X64_USE_128) && !defined(PYTHON_SCAN)

#include <cmath> // pow
#include <stdint.h>

#if defined(HAVE___UINT128_T) && !defined(HAVE_UINT128_T)
/**
 * @ingroup highprec
 * Some compilers do not have this defined, so we define it.
 * @{
 */
typedef __uint128_t uint128_t;
typedef __int128_t int128_t;
/** @} */
#endif

/**
 * @file
 * @ingroup highprec
 * Declaration of the ns3::int64x64_t type using a native int128_t type.
 */

namespace ns3
{

/**
 * @internal
 * The implementation documented here is based on native 128-bit integers.
 */
class int64x64_t
{
    /// uint128_t high bit (sign bit).
    static const uint128_t HP128_MASK_HI_BIT = (((int128_t)1) << 127);
    /// Mask for fraction part.
    static const uint64_t HP_MASK_LO = 0xffffffffffffffffULL;

  public:
    /// Floating point value of HP_MASK_LO + 1.
    static constexpr long double HP_MAX_64 = (static_cast<uint64_t>(1) << 63) * 2.0L;

    /**
     * Type tag for the underlying implementation.
     *
     * A few testcases are are sensitive to implementation,
     * specifically the double implementation.  To handle this,
     * we expose the underlying implementation type here.
     */
    enum impl_type
    {
        int128_impl, //!< Native \c int128_t implementation.
        cairo_impl,  //!< Cairo wideint implementation.
        ld_impl,     //!< `long double` implementation.
    };

    /// Type tag for this implementation.
    static const enum impl_type implementation = int128_impl;

    /// Default constructor.
    inline int64x64_t()
        : _v(0)
    {
    }

    /**
     * @name Construct from a floating point value.
     */
    /**
     * @{
     * Constructor from a floating point.
     *
     * @param [in] value Floating value to represent.
     */
    inline int64x64_t(const double value)
    {
        const int64x64_t tmp((long double)value);
        _v = tmp._v;
    }

    inline int64x64_t(const long double value)
    {
        const bool negative = value < 0;
        const long double v = negative ? -value : value;

        long double fhi;
        long double flo = std::modf(v, &fhi);
        // Add 0.5 to round, which improves the last count
        // This breaks these tests:
        //   TestSuite devices-mesh-dot11s-regression
        //   TestSuite devices-mesh-flame-regression
        //   TestSuite routing-aodv-regression
        //   TestSuite routing-olsr-regression
        // Setting round = 0; breaks:
        //   TestSuite int64x64
        const long double round = 0.5;
        flo = flo * HP_MAX_64 + round;
        int128_t hi = fhi;
        const uint64_t lo = flo;
        if (flo >= HP_MAX_64)
        {
            // conversion to uint64 rolled over
            ++hi;
        }
        _v = hi << 64;
        _v |= lo;
        _v = negative ? -_v : _v;
    }

    /**@}*/

    /**
     * @name Construct from an integral type.
     */
    /**@{*/
    /**
     * Construct from an integral type.
     *
     * @param [in] v Integer value to represent.
     */
    inline int64x64_t(const int v)
        : _v(v)
    {
        _v <<= 64;
    }

    inline int64x64_t(const long int v)
        : _v(v)
    {
        _v <<= 64;
    }

    inline int64x64_t(const long long int v)
        : _v(v)
    {
        _v <<= 64;
    }

    inline int64x64_t(const unsigned int v)
        : _v(v)
    {
        _v <<= 64;
    }

    inline int64x64_t(const unsigned long int v)
        : _v(v)
    {
        _v <<= 64;
    }

    inline int64x64_t(const unsigned long long int v)
        : _v(v)
    {
        _v <<= 64;
    }

    /**@}*/

    /**
     * Construct from explicit high and low values.
     *
     * @param [in] hi Integer portion.
     * @param [in] lo Fractional portion, already scaled to HP_MAX_64.
     */
    explicit inline int64x64_t(const int64_t hi, const uint64_t lo)
    {
        _v = (int128_t)hi << 64;
        _v |= lo;
    }

    /**
     * Copy constructor.
     *
     * @param [in] o Value to copy.
     */
    inline int64x64_t(const int64x64_t& o)
        : _v(o._v)
    {
    }

    /**
     * Assignment.
     *
     * @param [in] o Value to assign to this int64x64_t.
     * @returns This int64x64_t.
     */
    inline int64x64_t& operator=(const int64x64_t& o)
    {
        _v = o._v;
        return *this;
    }

    /** Explicit bool conversion. */
    inline explicit operator bool() const
    {
        return (_v != 0);
    }

    /**
     * Get this value as a double.
     *
     * @return This value in floating form.
     */
    inline double GetDouble() const
    {
        const bool negative = _v < 0;
        const int128_t vTemp = _v + (_v == std::numeric_limits<int128_t>::min());
        const uint128_t value = negative ? -vTemp : vTemp;
        const long double fhi = value >> 64;
        const long double flo = (value & HP_MASK_LO) / HP_MAX_64;
        long double retval = fhi;
        retval += flo;
        retval = negative ? -retval : retval;
        return retval;
    }

    /**
     * Get the integer portion.
     *
     * @return The integer portion of this value.
     */
    inline int64_t GetHigh() const
    {
        const int128_t retval = _v >> 64;
        return retval;
    }

    /**
     * Get the fractional portion of this value, unscaled.
     *
     * @return The fractional portion, unscaled, as an integer.
     */
    inline uint64_t GetLow() const
    {
        const uint128_t retval = _v & HP_MASK_LO;
        return retval;
    }

    /**
     * Truncate to an integer.
     * Truncation is always toward zero,
     * @return The value truncated toward zero.
     */
    int64_t GetInt() const
    {
        const bool negative = _v < 0;
        const uint128_t value = negative ? -_v : _v;
        int64_t retval = value >> 64;
        retval = negative ? -retval : retval;
        return retval;
    }

    /**
     * Round to the nearest int.
     * Similar to std::round this rounds halfway cases away from zero,
     * regardless of the current (floating) rounding mode.
     * @return The value rounded to the nearest int.
     */
    int64_t Round() const
    {
        const bool negative = _v < 0;
        int64x64_t value = (negative ? -(*this) : *this);
        const int64x64_t half(0, 1LL << 63);
        value += half;
        int64_t retval = value.GetHigh();
        retval = negative ? -retval : retval;
        return retval;
    }

    /**
     * Multiply this value by a Q0.128 value, presumably representing an inverse,
     * completing a division operation.
     *
     * @param [in] o The inverse operand.
     *
     * @see Invert()
     */
    void MulByInvert(const int64x64_t& o);

    /**
     * Compute the inverse of an integer value.
     *
     * Ordinary division by an integer would be limited to 64 bits of precision.
     * Instead, we multiply by the 128-bit inverse of the divisor.
     * This function computes the inverse to 128-bit precision.
     * MulByInvert() then completes the division.
     *
     * (Really this should be a separate type representing Q0.128.)
     *
     * @param [in] v The value to compute the inverse of.
     * @return A Q0.128 representation of the inverse.
     */
    static int64x64_t Invert(const uint64_t v);

  private:
    /**
     * @name Arithmetic Operators
     * Arithmetic operators for int64x64_t.
     */
    /**
     * @{
     * Arithmetic operator.
     * @param [in] lhs Left hand argument
     * @param [in] rhs Right hand argument
     * @return The result of the operator.
     */

    friend inline bool operator==(const int64x64_t& lhs, const int64x64_t& rhs)
    {
        return lhs._v == rhs._v;
    }

    friend inline bool operator<(const int64x64_t& lhs, const int64x64_t& rhs)
    {
        return lhs._v < rhs._v;
    }

    friend inline bool operator>(const int64x64_t& lhs, const int64x64_t& rhs)
    {
        return lhs._v > rhs._v;
    }

    friend inline int64x64_t& operator+=(int64x64_t& lhs, const int64x64_t& rhs)
    {
        lhs._v += rhs._v;
        return lhs;
    }

    friend inline int64x64_t& operator-=(int64x64_t& lhs, const int64x64_t& rhs)
    {
        lhs._v -= rhs._v;
        return lhs;
    }

    friend inline int64x64_t& operator*=(int64x64_t& lhs, const int64x64_t& rhs)
    {
        lhs.Mul(rhs);
        return lhs;
    }

    friend inline int64x64_t& operator/=(int64x64_t& lhs, const int64x64_t& rhs)
    {
        lhs.Div(rhs);
        return lhs;
    }

    /**@}*/

    /**
     * @name Unary Operators
     * Unary operators for int64x64_t.
     */
    /**
     * @{
     * Unary operator.
     * @param [in] lhs Left hand argument
     * @return The result of the operator.
     */
    friend inline int64x64_t operator+(const int64x64_t& lhs)
    {
        return lhs;
    }

    friend inline int64x64_t operator-(const int64x64_t& lhs)
    {
        int64x64_t res;
        res._v = -lhs._v;
        return res;
    }

    friend inline int64x64_t operator!(const int64x64_t& lhs)
    {
        return int64x64_t(!lhs._v);
    }

    /**@}*/

    /**
     * Implement `*=`.
     * We assert if the product cannot be encoded in int64x64_t.
     *
     * @param [in] o The other factor.
     */
    void Mul(const int64x64_t& o);
    /**
     * Implement `/=`.
     *
     * @param [in] o The divisor.
     */
    void Div(const int64x64_t& o);
    /**
     * Unsigned multiplication of Q64.64 values.
     *
     * Mathematically this should produce a Q128.128 value;
     * we keep the central 128 bits, representing the Q64.64 result.
     * We might assert if the result, in uint128_t format, exceeds 2^127.
     *
     * @param [in] a First factor.
     * @param [in] b Second factor.
     * @return The Q64.64 product.
     *
     * @internal
     *
     * It might be tempting to just use \pname{a} `*` \pname{b}
     * and be done with it, but it's not that simple.  With \pname{a}
     * and \pname{b} as 128-bit integers, \pname{a} `*` \pname{b}
     * mathematically produces a 256-bit result, which the computer
     * truncates to the lowest 128 bits.  In our case, where \pname{a}
     * and \pname{b} are interpreted as Q64.64 fixed point numbers,
     * the multiplication mathematically produces a Q128.128 fixed point number.
     * We want the middle 128 bits from the result, truncating both the
     * high and low 64 bits.  To achieve this, we carry out the multiplication
     * explicitly with 64-bit operands and 128-bit intermediate results.
     */
    static uint128_t Umul(const uint128_t a, const uint128_t b);
    /**
     * Unsigned division of Q64.64 values.
     *
     * @param [in] a Numerator.
     * @param [in] b Denominator.
     * @return The Q64.64 representation of `a / b`.
     */
    static uint128_t Udiv(const uint128_t a, const uint128_t b);
    /**
     * Unsigned multiplication of Q64.64 and Q0.128 values.
     *
     * @param [in] a The numerator, a Q64.64 value.
     * @param [in] b The inverse of the denominator, a Q0.128 value
     * @return The product `a * b`, representing the ration `a / b^-1`.
     *
     * @see Invert()
     */
    static uint128_t UmulByInvert(const uint128_t a, const uint128_t b);

    int128_t _v; //!< The Q64.64 value.
};

} // namespace ns3

#endif /* defined(INT64X64_USE_128) && !defined(PYTHON_SCAN) */
#endif /* INT64X64_128_H */
