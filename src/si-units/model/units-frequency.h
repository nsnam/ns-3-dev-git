#ifndef UNITS_FREQUENCY_H
#define UNITS_FREQUENCY_H

#include "format-string.h"
#include "units-aliases.h"

#include "ns3/attribute-helper.h"
#include <ns3/nstime.h>

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <math.h> // To support Linux. <cmath> lacks log10l().
#include <optional>
#include <ostream>

// clang-format off
namespace ns3
{

/// Frequency unit in Hz
struct Hz_t
{
    // TODO(porce): Investigate to change `double` to `int64_t` if there is no need of sub-Hertz.
    // If there is a need of sub-Hertz, such as mHz, define struct mHz and build Hz_t on top of it.
    double val{}; ///< Value in Hz

    Hz_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in Hz
    constexpr explicit Hz_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param val Value in Hz
    constexpr explicit Hz_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// @brief Stringify with metric prefix
    /// Sub-Hertz not supported
    /// @return String with metric prefix
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        const std::vector<std::string> WHOLE_UNIT_PREFIX = {"", "k", "M", "G", "T"};

        size_t idx{};
        auto valInt = static_cast<int64_t>(val); // No support of sub-Hertz
        if (valInt > 0)
        {
            while (((idx + 1) < WHOLE_UNIT_PREFIX.size()) && ((valInt % ONE_KILO) == 0))
            {
                valInt /= ONE_KILO;
                ++idx;
            }
        }

        return sformat("%lld %sHz", valInt, WHOLE_UNIT_PREFIX[idx].c_str());
    }

    /// Converts a vector of double values represented in Hz to a vector of Hz_t
    /// @param input Vector of double values represented in Hz
    /// @return Vector of Hz_t values
    static std::vector<Hz_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<Hz_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return Hz_t{f};
        });
        return output;
    }

    /// Converts a vector of Hz_t values to a vector of double values represented in Hz
    /// @param input Vector of Hz_t values
    /// @return Vector of double values represented in Hz
    static std::vector<double> to_doubles(
        std::vector<Hz_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](Hz_t f) { return f.val; });
        return output;
    }

    /// Equality operator
    /// @param rhs Right-hand side of the equality operator
    /// @return True if the two values are equal, false otherwise
    inline bool operator==(const Hz_t& rhs) const
    {
        return val == rhs.val;
    }

    /// Inequality operator
    /// @param rhs value to compare
    /// @return True if the two values are not equal, false otherwise
    inline bool operator!=(const Hz_t& rhs) const
    {
        return !(operator==(rhs));
    }

    /// Less-than operator
    /// @param rhs value to compare
    /// @return True if the left-hand side value is less than the right-hand side value, false otherwise
    inline bool operator<(const Hz_t& rhs) const
    {
        return val < rhs.val;
    }

    /// Greater-than operator
    /// @param rhs value to compare
    /// @return True if the left-hand side value is greater than the right-hand side value, false otherwise
    inline bool operator>(const Hz_t& rhs) const
    {
        return val > rhs.val;
    }

    /// Less-than-or-equal-to operator
    /// @param rhs value to compare
    /// @return True if the left-hand side value is less than or equal to the right-hand side value, false otherwise
    inline bool operator<=(const Hz_t& rhs) const
    {
        return val <= rhs.val;
    }

    /// Greater-than-or-equal-to operator
    /// @param rhs value to compare
    /// @return True if the left-hand side value is greater than or equal to the right-hand side value, false otherwise
    inline bool operator>=(const Hz_t& rhs) const
    {
        return val >= rhs.val;
    }

    /// Negation operator
    /// @return Negated value
    inline Hz_t operator-() const
    {
        return Hz_t{-val};
    }

    /// Addition operator
    /// @param rhs value to add
    /// @return Sum of the two values
    inline constexpr Hz_t operator+(const Hz_t& rhs) const
    {
        return Hz_t{val + rhs.val};
    }

    /// Subtraction operator
    /// @param rhs value to subtract
    /// @return Difference of the two values
    inline constexpr Hz_t operator-(const Hz_t& rhs) const
    {
        return Hz_t{val - rhs.val};
    }

    /// Addition assignment operator
    /// @param rhs value to add
    /// @return Reference to this object
    inline Hz_t& operator+=(const Hz_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction assignment operator
    /// @param rhs value to subtract
    /// @return Reference to this object
    inline Hz_t& operator-=(const Hz_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Division operator
    /// @param rhs value to divide by
    /// @return Quotient of the two values
    inline Hz_t operator/(double rhs) const
    {
        return Hz_t{val / rhs};
    }

    /// Division operator
    /// @param rhs value to divide by
    /// @return Quotient of the two values
    inline double operator/(const Hz_t& rhs) const
    {
        return val / rhs.val;
    }

    /// Multiplication operator
    /// @param rhs value to multiply by
    /// @return Product of the two values
    inline Hz_t operator*(double rhs) const
    {
        return Hz_t{val * rhs};
    }

    /// Multiplication assignment operator
    /// @param rhs value to multiply by
    /// @return Reference to this object
    inline Hz_t& operator*=(double rhs)
    {
        val *= rhs;
        return *this;
    }

    /// Division assignment operator
    /// @param rhs value to divide by
    /// @return Reference to this object
    inline Hz_t& operator/=(double rhs)
    {
        val /= rhs;
        return *this;
    }

    /// Multiplication operator
    /// @param nstime time
    /// @return Unitless value
    inline double operator*(Time nstime) const
    {
        // 64-bit double storage type is large enough to support Time's storage type and its
        // metric prefixes. 64-bit double supports upto 1.7e308. Time supports up to 2^63 - 1,
        // regardless of its metric prefix. The supported range of the return value large enough
        // for ns-3 This means upto 1.8e289 Hz_t is supported for this multiplication operation
        // This range does not qualify anything about the precision and accuracy.
        return (val * nstime.GetNanoSeconds()) / ONE_GIGA;
    }

    /// Get the value of the Hz_t in Hz
    /// @return Value in Hz
    inline double in_Hz() const // NOLINT(readability-identifier-naming)
    {
        return val;
    }

    /// Get the value of the Hz_t in kHz
    /// @return Value in kHz
    inline double in_kHz() const // NOLINT(readability-identifier-naming)
    {
        return val / ONE_KILO;
    }

    /// Get the value of the Hz_t in MHz
    /// @return Value in MHz
    inline double in_MHz() const // NOLINT(readability-identifier-naming)
    {
        return val / ONE_MEGA;
    }

    /// Get the value of the Hz_t in GHz
    /// @return Value in GHz
    inline double in_GHz() const // NOLINT(readability-identifier-naming)
    {
        return val / ONE_GIGA;
    }

    /// Converts from a string
    /// @param input String to convert from
    /// @return Optional Hz_t value
    static std::optional<Hz_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

// User defined literals
Hz_t operator""_Hz(unsigned long long val);
Hz_t operator""_Hz(long double val);
Hz_t operator""_kHz(unsigned long long val);
Hz_t operator""_kHz(long double val);
Hz_t operator""_MHz(unsigned long long val);
Hz_t operator""_MHz(long double val);
Hz_t operator""_GHz(unsigned long long val);
Hz_t operator""_GHz(long double val);
Hz_t operator""_THz(unsigned long long val);
Hz_t operator""_THz(long double val);

std::ostream& operator<<(std::ostream& os, const Hz_t& rhs);
std::istream& operator>>(std::istream& is, Hz_t& rhs);

Hz_t operator*(double lfs, const Hz_t& rhs);
double operator*(Time nstime, const Hz_t& rhs);

/// Converts a value represented in kHz to Hz_t
/// @param val Value in kHz
/// @return Value in Hz_t
inline Hz_t
kHz_t(double val)
{
    return Hz_t{val * ONE_KILO};
}

/// Converts a value represented in MHz to Hz_t
/// @param val Value in MHz
/// @return Value in Hz_t
inline Hz_t
MHz_t(double val)
{
    return Hz_t{val * ONE_MEGA};
}

/// Converts a value represented in GHz to Hz_t
/// @param val Value in GHz
/// @return Value in Hz_t
inline Hz_t
GHz_t(double val)
{
    return Hz_t{val * ONE_GIGA};
}

/// Converts a value represented in THz to Hz_t
/// @param val Value in THz
/// @return Value in Hz_t
inline Hz_t
THz_t(double val)
{
    return Hz_t{val * ONE_TERA};
}

/// @cond Doxygen warning against macro internals
ATTRIBUTE_HELPER_HEADER(Hz_t);  // See si-units-test-suite.cc for usages
/// @endcond

} // namespace ns3

// clang-format on

#endif // UNITS_FREQUENCY_H
