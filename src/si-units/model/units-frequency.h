#ifndef UNITS_FREQUENCY_H
#define UNITS_FREQUENCY_H

#include "si-units-parser.h"
#include "units-aliases.h"

#include "ns3/assert.h"
#include "ns3/nstime.h"

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

    /// Constructor
    /// @param val Value in Hz
    constexpr explicit Hz_t(int64_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param val Value in Hz
    constexpr explicit Hz_t(uint64_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit Hz_t(const std::string& str){
        auto res = from_str(str);
        NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "Hz"));
        val = res.value().val;
    }

    /// Constructor
    /// @param hz Value in Hz
    constexpr Hz_t(const Hz_t& hz)
        : val(hz.val)
    {
    }

    /// @brief Stringify with metric prefix
    /// Sub-Hertz not supported
    /// @param space Insert or omit space between a number and a unit measurement, with SI standard as default
    /// @return String with metric prefix
    std::string str(bool space=true) const // NOLINT(readability-identifier-naming)
    {
        return NumToMetricStr(val, space) + "Hz";
    }

    /// @brief Convert a vector of double values to a vector of Hz_t values
    /// @param input vector of double values
    /// @return vector of Hz_t values
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

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const Hz_t& rhs) const = default;

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

    /// Get the value of the Hz_t in THz
    /// @return Value in THz
    inline double in_THz() const // NOLINT(readability-identifier-naming)
    {
        return val / ONE_TERA;
    }


    /// Converts from a string
    /// @param input String to convert from
    /// @return Optional Hz_t value
    static std::optional<Hz_t> from_str(const std::string& input)
    {
        std::string unit = "Hz";
        auto str = Trim(input);
        if (not str.ends_with(unit))
        {
            return std::nullopt;
        }
        str.erase(str.length() - unit.length());
        str = Trim(str);
        auto res = MetricStrToNum(str);
        return res.has_value()? std::optional(Hz_t(res.value())) : std::nullopt;
    }

    /// Test if this is a multiple of the right hand side
    /// @param rhs the measuring size in Hz
    /// @returns true if a multiple, false otherwise
    bool IsMultipleOf(const Hz_t& rhs) const
    {
        NS_ASSERT(rhs.val != 0.);
        auto div = static_cast<const int64_t>(val / rhs.val);
        return val == (div * rhs.val);
    }
};

/// User-defined literals for Hz
/// @param val The value in Hz
/// @return Hz_t object
inline Hz_t
operator""_Hz(unsigned long long val)
{
    return Hz_t{static_cast<double>(val)};
}

/// User-defined literals for Hz
/// @param val The value in Hz
/// @return Hz_t object
inline Hz_t
operator""_Hz(long double val)
{
    return Hz_t{static_cast<double>(val)};
}

/// Output stream for Hz_t
/// @param os The output stream
/// @param rhs The Hz_t object to output
/// @return The output stream
inline std::ostream&
operator<<(std::ostream& os, const Hz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for Hz_t
/// @param is The input stream
/// @param rhs The Hz_t object to input
/// @return The input stream
inline std::istream&
operator>>(std::istream& is, Hz_t& rhs)
{
    return ParseSIString(is, rhs, "Hz");
}

/// Frequency unit in kHz
struct kHz_t : Hz_t
{
    kHz_t() = default; // NOLINT(readability-identifier-naming

    /// Constructor
    /// @param val Value in kHz
    constexpr explicit kHz_t(double val)
        : Hz_t(static_cast<double>(val * ONE_KILO))
    {
    }

    /// Constructor
    /// @param val Value in kHz
    constexpr explicit kHz_t(int32_t val)
        : Hz_t(static_cast<double>(val * ONE_KILO))
    {
    }

    /// Constructor
    /// @param val Value in kHz
    constexpr explicit kHz_t(int64_t val)
        : Hz_t(static_cast<double>(val * ONE_KILO))
    {
    }

    /// Constructor
    /// @param val Value in kHz
    constexpr explicit kHz_t(uint64_t val)
        : Hz_t(val * ONE_KILO)
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit kHz_t(const std::string& str)
    {
        auto res = from_str(str);
        NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "kHz"));
        val = res.value().val;
    }

    /// Constructor
    /// @param hz Value in Hz
    constexpr kHz_t(const Hz_t hz)
        : Hz_t(hz)
    {
    }
};

/// @param val The value in kHz
/// @return Hz_t object
inline kHz_t operator""_kHz(unsigned long long val)
{
    return kHz_t{static_cast<double>(val)};
}

/// User-defined literals for kHz
/// @param val The value in kHz
/// @return Hz_t object
inline kHz_t operator""_kHz(long double val)
{
    return kHz_t{static_cast<double>(val)};
}

/// Output stream for kHz_t
/// @param os The output stream
/// @param rhs The kHz_t object to output
/// @return The output stream
inline std::ostream&
operator<<(std::ostream& os, const kHz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for kHz_t
/// @param is The input stream
/// @param rhs The kHz_t object to input
/// @return The input stream
inline std::istream&
operator>>(std::istream& is, kHz_t& rhs)
{
    return ParseSIString(is, rhs, "kHz");
}

/// Frequency unit in MHz
struct MHz_t : Hz_t
{
    MHz_t() = default; // NOLINT(readability-identifier-naming

    /// Constructor
    /// @param val Value in MHz
    constexpr explicit MHz_t(double val)
        : Hz_t(static_cast<double>(val * ONE_MEGA))
    {
    }

    /// Constructor
    /// @param val Value in MH
    constexpr explicit MHz_t(int32_t val)
        : Hz_t(static_cast<int64_t>(val * ONE_MEGA))
    {
    }

    /// Constructor
    /// @param val Value in MHz
    constexpr explicit MHz_t(uint64_t val)
        : Hz_t(static_cast<uint64_t>(val * ONE_MEGA))
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit MHz_t(const std::string& str)
    {
        auto res = from_str(str);
        NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "MHz"));
        val = res.value().val;
    }

    /// Constructor
    /// @param hz Value in Hz
    constexpr MHz_t(const Hz_t hz)
        : Hz_t(hz)
    {
    }
};


/// User-defined literals for MHz
/// @param val The value in MHz
/// @return Hz_t object
inline MHz_t operator""_MHz(unsigned long long val)
{
    return MHz_t{static_cast<double>(val)};
}

/// User-defined literals for MHz
/// @param val The value in MHz
/// @return Hz_t object
inline MHz_t operator""_MHz(long double val)
{
    return MHz_t{static_cast<double>(val)};
}


/// Output stream for MHz_t
/// @param os The output stream
/// @param rhs The MHz_t object to output
/// @return The output stream
inline std::ostream&
operator<<(std::ostream& os, const MHz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for MHz_t
/// @param is The input stream
/// @param rhs The MHz_t object to input
/// @return The input stream
inline std::istream&
operator>>(std::istream& is, MHz_t& rhs)
{
    return ParseSIString(is, rhs, "MHz");
}


/// Frequency unit in GHz
struct GHz_t : Hz_t
{
    GHz_t() = default; // NOLINT(readability-identifier-naming

    /// Constructor
    /// @param val Value in GHz
    constexpr explicit GHz_t(int32_t val)
        : Hz_t(static_cast<double>(val * ONE_GIGA))
    {
    }

    /// Constructor
    /// @param val Value in GHz
    constexpr explicit GHz_t(int64_t val)
        : Hz_t(static_cast<double>(val * ONE_GIGA))
    {
    }

    /// Constructor
    /// @param val Value in GHz
    constexpr explicit GHz_t(uint64_t val)
        : Hz_t(val * ONE_GIGA)
    {
    }

    /// Constructor
    /// @param val Value in GHz
    constexpr explicit GHz_t(double val)
        : Hz_t(val * ONE_GIGA)
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit GHz_t(const std::string& str)
    {
        auto res = from_str(str);
        NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "GHz"));
        val = res.value().val;
    }

    /// Constructor
    /// @param hz Value in Hz
    constexpr GHz_t(const Hz_t hz)
        : Hz_t(hz)
    {
    }
};



/// User-defined literals for GHz
/// @param val The value in GHz
/// @return Hz_t object
inline GHz_t operator""_GHz(unsigned long long val)
{
    return GHz_t{static_cast<double>(val)};
}

/// User-defined literals for GHz
/// @param val The value in GHz
/// @return Hz_t object
inline GHz_t operator""_GHz(long double val)
{
    return GHz_t{static_cast<double>(val)};
}


/// Output stream for GHz_t
/// @param os The output stream
/// @param rhs The GHz_t object to output
/// @return The output stream
inline std::ostream&
operator<<(std::ostream& os, const GHz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for GHz_t
/// @param is The input stream
/// @param rhs The GHz_t object to input
/// @return The input stream
inline std::istream&
operator>>(std::istream& is, GHz_t& rhs)
{
    return ParseSIString(is, rhs, "GHz");
}

/// Frequency unit in THz
struct THz_t : Hz_t
{
    THz_t() = default; // NOLINT(readability-identifier-naming

    /// Constructor
    /// @param val Value in THz
    constexpr explicit THz_t(int32_t val)
        : Hz_t(static_cast<double>(val * ONE_TERA))
    {
    }

    /// Constructor
    /// @param val Value in THz
    constexpr explicit THz_t(int64_t val)
        : Hz_t(static_cast<double>(val * ONE_TERA))
    {
    }

    /// Constructor
    /// @param val Value in THz
    constexpr explicit THz_t(uint64_t val)
        : Hz_t(val * ONE_TERA)
    {
    }

    /// Constructor
    /// @param val Value in THz
    constexpr explicit THz_t(double val)
        : Hz_t(val * ONE_TERA)
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit THz_t(const std::string& str)
    {
        auto res = from_str(str);
        NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "THz"));
        val = res.value().val;
    }

    /// Constructor
    /// @param hz Value in Hz
    constexpr THz_t(const Hz_t hz)
        : Hz_t(hz)
    {
    }
};

/// User-defined literals for THz
/// @param val The value in THz
/// @return Hz_t object
inline THz_t operator""_THz(unsigned long long val)
{
    return THz_t{static_cast<double>(val)};
}

/// User-defined literals for THz
/// @param val The value in THz
/// @return Hz_t object
inline THz_t operator""_THz(long double val)
{
    return THz_t{static_cast<double>(val)};
}

/// Output stream for THz_t
/// @param os The output stream
/// @param rhs The THz_t object to output
/// @return The output stream
inline std::ostream&
operator<<(std::ostream& os, const THz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for THz_t
/// @param is The input stream
/// @param rhs The THz_t object to input
/// @return The input stream
inline std::istream&
operator>>(std::istream& is, THz_t& rhs)
{
    return ParseSIString(is, rhs, "THz");
}

/// Multiply Hz_t by double
/// @param lhs The double to multiply by
/// @param rhs The Hz_t object to multiply
/// @return The Hz_t object
inline Hz_t
operator*(double lhs, const Hz_t& rhs)
{
    return rhs * lhs;
}

/// Multiply Hz_t by Time
/// @param nstime The Time to multiply by
/// @param rhs The Hz_t object to multiply
/// @return unitless value
inline double
operator*(Time nstime, const Hz_t& rhs)
{
    return rhs * nstime;
}

} // namespace ns3

#include "units-frequency-attributes.h" // Attribute Support

#endif // UNITS_FREQUENCY_H
