/*
 * Copyright (c) 2010 INRIA
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

#include "int64x64-128.h"

#include "abort.h"
#include "assert.h"
#include "log.h"

/**
 * \file
 * \ingroup highprec
 * Implementation of the ns3::int64x64_t type using a native int128_t type.
 */

namespace ns3
{

// Note:  Logging in this file is largely avoided due to the
// number of calls that are made to these functions and the possibility
// of causing recursions leading to stack overflow
NS_LOG_COMPONENT_DEFINE("int64x64-128");

/**
 * \ingroup highprec
 * Compute the sign of the result of multiplying or dividing
 * Q64.64 fixed precision operands.
 *
 * \param [in]  sa The signed value of the first operand.
 * \param [in]  sb The signed value of the second operand.
 * \param [out] ua The unsigned magnitude of the first operand.
 * \param [out] ub The unsigned magnitude of the second operand.
 * \returns \c true if the result will be negative.
 */
static inline bool
output_sign(const int128_t sa, const int128_t sb, uint128_t& ua, uint128_t& ub)
{
    bool negA = sa < 0;
    bool negB = sb < 0;
    ua = negA ? -static_cast<uint128_t>(sa) : sa;
    ub = negB ? -static_cast<uint128_t>(sb) : sb;
    return negA != negB;
}

void
int64x64_t::Mul(const int64x64_t& o)
{
    uint128_t a;
    uint128_t b;
    bool negative = output_sign(_v, o._v, a, b);
    uint128_t result = Umul(a, b);
    if (negative)
    {
        NS_ASSERT_MSG(result <= HP128_MASK_HI_BIT, "overflow detected");
        _v = -result;
    }
    else
    {
        NS_ASSERT_MSG(result < HP128_MASK_HI_BIT, "overflow detected");
        _v = result;
    }
}

uint128_t
int64x64_t::Umul(const uint128_t a, const uint128_t b)
{
    uint128_t al = a & HP_MASK_LO;
    uint128_t bl = b & HP_MASK_LO;
    uint128_t ah = a >> 64;
    uint128_t bh = b >> 64;

    // Let Q(x) be the unsigned Q64.64 fixed point value represented by the uint128_t x:
    //     Q(x) * 2^64 = x = xh * 2^64 + xl.
    // (Defining x this way avoids ambiguity about the meaning of the division operators in
    //     Q(x) = x / 2^64 = xh + xl / 2^64.)
    // Then
    //     Q(a) = ah + al / 2^64
    // and
    //     Q(b) = bh + bl / 2^64.
    // We need to find uint128_t c such that
    //     Q(c) = Q(a) * Q(b).
    // Then
    //     c = Q(c) * 2^64
    //       = (ah + al / 2^64) * (bh + bl / 2^64) * 2^64
    //       = (ah * 2^64 + al) * (bh * 2^64 + bl) / 2^64
    //       = ah * bh * 2^64 + (ah * bl + al * bh) + al * bl / 2^64.
    // We compute the last part of c by (al * bl) >> 64 which truncates (instead of rounds)
    // the LSB. If c exceeds 2^127, we might assert. This is because our caller
    // (Mul function) will not be able to represent the result.

    uint128_t res = (al * bl) >> 64;
    {
        // ah, bh <= 2^63 and al, bl <= 2^64 - 1, so mid < 2^128 - 2^64 and there is no
        // integer overflow.
        uint128_t mid = ah * bl + al * bh;
        // res < 2^64, so there is no integer overflow.
        res += mid;
    }
    {
        uint128_t high = ah * bh;
        // If high > 2^63, then the result will overflow.
        NS_ASSERT_MSG(high <= (static_cast<uint128_t>(1) << 63), "overflow detected");
        high <<= 64;
        NS_ASSERT_MSG(res + high >= res, "overflow detected");
        // No overflow since res, high <= 2^127 and one of res, high is < 2^127.
        res += high;
    }

    return res;
}

void
int64x64_t::Div(const int64x64_t& o)
{
    uint128_t a;
    uint128_t b;
    bool negative = output_sign(_v, o._v, a, b);
    int128_t result = Udiv(a, b);
    _v = negative ? -result : result;
}

uint128_t
int64x64_t::Udiv(const uint128_t a, const uint128_t b)
{
    uint128_t rem = a;
    uint128_t den = b;
    uint128_t quo = rem / den;
    rem = rem % den;
    uint128_t result = quo;

    // Now, manage the remainder
    const uint64_t DIGITS = 64; // Number of fraction digits (bits) we need
    const uint128_t ZERO = 0;

    NS_ASSERT_MSG(rem < den, "Remainder not less than divisor");

    uint64_t digis = 0; // Number of digits we have already
    uint64_t shift = 0; // Number we are going to get this round

    // Skip trailing zeros in divisor
    while ((shift < DIGITS) && !(den & 0x1))
    {
        ++shift;
        den >>= 1;
    }

    while ((digis < DIGITS) && (rem != ZERO))
    {
        // Skip leading zeros in remainder
        while ((digis + shift < DIGITS) && !(rem & HP128_MASK_HI_BIT))
        {
            ++shift;
            rem <<= 1;
        }

        // Cast off denominator bits if:
        //   Need more digits and
        //     LSB is zero or
        //     rem < den
        while ((digis + shift < DIGITS) && (!(den & 0x1) || (rem < den)))
        {
            ++shift;
            den >>= 1;
        }

        // Do the division
        quo = rem / den;
        rem = rem % den;

        // Add in the quotient as shift bits of the fraction
        result <<= shift;
        result += quo;

        digis += shift;
        shift = 0;
    }
    // Did we run out of remainder?
    if (digis < DIGITS)
    {
        shift = DIGITS - digis;
        result <<= shift;
    }

    return result;
}

void
int64x64_t::MulByInvert(const int64x64_t& o)
{
    bool negResult = _v < 0;
    uint128_t a = negResult ? -static_cast<uint128_t>(_v) : _v;
    uint128_t result = UmulByInvert(a, o._v);

    _v = negResult ? -result : result;
}

uint128_t
int64x64_t::UmulByInvert(const uint128_t a, const uint128_t b)
{
    // Since b is assumed to be the output of Invert(), b <= 2^127.
    NS_ASSERT(b <= HP128_MASK_HI_BIT);

    uint128_t al = a & HP_MASK_LO;
    uint128_t bl = b & HP_MASK_LO;
    uint128_t ah = a >> 64;
    uint128_t bh = b >> 64;

    // Since ah, bh <= 2^63, high <= 2^126 and there is no overflow.
    uint128_t high = ah * bh;

    // Since ah, bh <= 2^63 and al, bl < 2^64, mid < 2^128 and there is
    // no overflow.
    uint128_t mid = ah * bl + al * bh;
    mid >>= 64;

    // Since high <= 2^126 and mid < 2^64, result < 2^127 and there is no overflow.
    uint128_t result = high + mid;

    return result;
}

int64x64_t
int64x64_t::Invert(const uint64_t v)
{
    NS_ASSERT(v > 1);
    uint128_t a;
    a = 1;
    a <<= 64;
    int64x64_t result;
    result._v = Udiv(a, v);
    int64x64_t tmp(v, 0);
    tmp.MulByInvert(result);
    if (tmp.GetHigh() != 1)
    {
        result._v += 1;
    }
    return result;
}

} // namespace ns3
