/*
 * Copyright (c) 2010 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#ifndef INT64X64_CAIRO_H
#define INT64X64_CAIRO_H

#include "ns3/core-config.h"

#if defined(INT64X64_USE_CAIRO) && !defined(PYTHON_SCAN)
/** Using the ns3::int64x64_t based on Cairo 128-bit integers. */

#include "cairo-wideint-private.h"

#include <cmath> // pow

/**
 * @file
 * @ingroup highprec
 * Declaration of the ns3::int64x64_t type using the Cairo implementation.
 */

namespace ns3
{

/**
 * @internal
 * The implementation documented here uses cairo 128-bit integers.
 */
class int64x64_t
{
    /// High bit of fractional part
    static const uint64_t HPCAIRO_MASK_HI_BIT = (((uint64_t)1) << 63);
    /// Mask for fraction part
    static const uint64_t HP_MASK_LO = 0xffffffffffffffffULL;

  public:
    /// Floating point value of HP_MASK_LO + 1
    static constexpr long double HP_MAX_64 = (static_cast<uint64_t>(1) << 63) * 2.0L;

    /**
     * Type tag for the underlying implementation.
     *
     * A few testcases are sensitive to implementation,
     * specifically the double implementation.  To handle this,
     * we expose the underlying implementation type here.
     */
    enum impl_type
    {
        int128_impl, //!< Native int128_t implementation.
        cairo_impl,  //!< cairo wideint implementation
        ld_impl,     //!< long double implementation
    };

    /// Type tag for this implementation.
    static const enum impl_type implementation = cairo_impl;

    /// Default constructor
    inline int64x64_t()
    {
        _v.hi = 0;
        _v.lo = 0;
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
        cairo_int64_t hi = (cairo_int64_t)fhi;
        const cairo_uint64_t lo = (cairo_uint64_t)flo;
        if (flo >= HP_MAX_64)
        {
            // conversion to uint64 rolled over
            ++hi;
        }
        _v.hi = hi;
        _v.lo = lo;
        _v = negative ? _cairo_int128_negate(_v) : _v;
    }

    /**@}*/

    /**
     * @name Construct from an integral type.
     */
    /**@{*/
    /**
     * Construct from an integral type.
     *
     * @param [in] v Integer value to represent
     */
    inline int64x64_t(const int v)
    {
        _v.hi = v;
        _v.lo = 0;
    }

    inline int64x64_t(const long int v)
    {
        _v.hi = v;
        _v.lo = 0;
    }

    inline int64x64_t(const long long int v)
    {
        _v.hi = v;
        _v.lo = 0;
    }

    inline int64x64_t(const unsigned int v)
    {
        _v.hi = v;
        _v.lo = 0;
    }

    inline int64x64_t(const unsigned long int v)
    {
        _v.hi = v;
        _v.lo = 0;
    }

    inline int64x64_t(const unsigned long long int v)
    {
        _v.hi = v;
        _v.lo = 0;
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
        _v.hi = hi;
        _v.lo = lo;
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
     * @returns a copy of \pname{o}
     */
    inline int64x64_t& operator=(const int64x64_t& o)
    {
        _v = o._v;
        return *this;
    }

    /** Explicit bool conversion. */
    inline explicit operator bool() const
    {
        return (_v.hi != 0 || _v.lo != 0);
    }

    /**
     * Get this value as a double.
     *
     * @return This value in floating form.
     */
    inline double GetDouble() const
    {
        const bool negative = _cairo_int128_negative(_v);
        const cairo_int128_t value = negative ? _cairo_int128_negate(_v) : _v;
        const long double fhi = static_cast<long double>(value.hi);
        const long double flo = value.lo / HP_MAX_64;
        long double retval = fhi;
        retval += flo;
        retval = negative ? -retval : retval;
        return static_cast<double>(retval);
    }

    /**
     * Get the integer portion.
     *
     * @return The integer portion of this value.
     */
    inline int64_t GetHigh() const
    {
        return (int64_t)_v.hi;
    }

    /**
     * Get the fractional portion of this value, unscaled.
     *
     * @return The fractional portion, unscaled, as an integer.
     */
    inline uint64_t GetLow() const
    {
        return _v.lo;
    }

    /**
     * Truncate to an integer.
     * Truncation is always toward zero,
     * @return The value truncated toward zero.
     */
    int64_t GetInt() const
    {
        const bool negative = _cairo_int128_negative(_v);
        const cairo_int128_t value = negative ? _cairo_int128_negate(_v) : _v;
        int64_t retval = value.hi;
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
        const bool negative = _cairo_int128_negative(_v);
        cairo_uint128_t value = negative ? _cairo_int128_negate(_v) : _v;
        cairo_uint128_t half{1ULL << 63, 0}; // lo, hi
        value = _cairo_uint128_add(value, half);
        int64_t retval = value.hi;
        retval = negative ? -retval : retval;
        return retval;
    }

    /**
     * Multiply this value by a Q0.128 value, presumably representing an inverse,
     * completing a division operation.
     *
     * @param [in] o The inverse operand.
     *
     * @see Invert
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
        return _cairo_int128_eq(lhs._v, rhs._v);
    }

    friend inline bool operator<(const int64x64_t& lhs, const int64x64_t& rhs)
    {
        return _cairo_int128_lt(lhs._v, rhs._v);
    }

    friend inline bool operator>(const int64x64_t& lhs, const int64x64_t& rhs)
    {
        return _cairo_int128_gt(lhs._v, rhs._v);
    }

    friend inline int64x64_t& operator+=(int64x64_t& lhs, const int64x64_t& rhs)
    {
        lhs._v = _cairo_int128_add(lhs._v, rhs._v);
        return lhs;
    }

    friend inline int64x64_t& operator-=(int64x64_t& lhs, const int64x64_t& rhs)
    {
        lhs._v = _cairo_int128_sub(lhs._v, rhs._v);
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

    /** @} */

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
        int64x64_t tmp = lhs;
        tmp._v = _cairo_int128_negate(tmp._v);
        return tmp;
    }

    friend inline int64x64_t operator!(const int64x64_t& lhs)
    {
        return (lhs == int64x64_t()) ? int64x64_t(1, 0) : int64x64_t();
    }

    /** @} */

    /**
     * Implement `*=`.
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
     * We assert on integer overflow beyond the 64-bit integer portion.
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
    static cairo_uint128_t Umul(const cairo_uint128_t a, const cairo_uint128_t b);
    /**
     * Unsigned division of Q64.64 values.
     *
     * @param [in] a Numerator.
     * @param [in] b Denominator.
     * @return The Q64.64 representation of `a / b`
     */
    static cairo_uint128_t Udiv(const cairo_uint128_t a, const cairo_uint128_t b);
    /**
     * Unsigned multiplication of Q64.64 and Q0.128 values.
     *
     * @param [in] a The numerator, a Q64.64 value.
     * @param [in] b The inverse of the denominator, a Q0.128 value
     * @return The product `a * b`, representing the ration `a / b^-1`
     *
     * @see Invert
     */
    static cairo_uint128_t UmulByInvert(const cairo_uint128_t a, const cairo_uint128_t b);

    cairo_int128_t _v; //!< The Q64.64 value.
};

} // namespace ns3

#endif /* defined(INT64X64_USE_CAIRO) && !defined(PYTHON_SCAN) */
#endif /* INT64X64_CAIRO_H */
