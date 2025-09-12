#ifndef UNITS_FREQUENCY_H
#define UNITS_FREQUENCY_H

#include "si-units-parser.h"
#include "units-aliases.h"

#include "ns3/assert.h"
#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/double.h"
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
    /// @param hz Value in Hz
    constexpr Hz_t(const Hz_t& hz)
        : val(hz.val)
    {
    }

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
    explicit Hz_t(const std::string& str);

    /// @brief Stringify with metric prefix
    /// Sub-Hertz not supported
    /// @param space Insert or omit space between a number and a unit measurement, with SI standard as default
    /// @return String with metric prefix
    std::string str(bool space=true) const; // NOLINT(readability-identifier-naming);

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
    static std::optional<Hz_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);

    /// Test if this is a multiple of the right hand side
    /// @param rhs the measuring size in Hz
    /// @returns true if a multiple, false otherwise
    bool IsMultipleOf(const Hz_t& rhs) const;
};

/// Frequency unit in kHz
struct kHz_t : Hz_t
{
    kHz_t() = default; ///< Default constructor // NOLINT(readability-identifier-naming

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
    /// @param val Value in kHz
    constexpr explicit kHz_t(double val)
        : Hz_t(val * ONE_KILO)
    {
    }

    /// Constructor
    /// @param hz Value in Hz
    constexpr kHz_t(const Hz_t& hz)
        : Hz_t(hz)
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit kHz_t(const std::string& str);
};

/// Frequency unit in MHz
struct MHz_t : Hz_t
{
    MHz_t() = default; ///< Default constructor // NOLINT(readability-identifier-naming

    /// Constructor
    /// @param val Value in MHz
    constexpr explicit MHz_t(double val)
        : Hz_t(static_cast<double>(val * ONE_MEGA))
    {
    }

    /// Constructor
    /// @param val Value in MHz
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
    /// @param hz Value in Hz
    constexpr MHz_t(const Hz_t& hz)
        : Hz_t(hz)
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit MHz_t(const std::string& str);
};

/// Frequency unit in GHz
struct GHz_t : Hz_t
{
    GHz_t() = default; ///< Default constructor // NOLINT(readability-identifier-naming

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
    /// @param hz Value in Hz
    constexpr GHz_t(const Hz_t& hz)
        : Hz_t(hz)
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit GHz_t(const std::string& str);
};

/// Frequency unit in THz
struct THz_t : Hz_t
{
    THz_t() = default; ///< Default constructor // NOLINT(readability-identifier-naming

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
    /// @param hz Value in Hz
    constexpr THz_t(const Hz_t& hz)
        : Hz_t(hz)
    {
    }

    /// Constructor
    /// @param str a string representation of the frequency
    explicit THz_t(const std::string& str);
};

// User defined literals
Hz_t operator""_Hz(unsigned long long val);
Hz_t operator""_Hz(long double val);
kHz_t operator""_kHz(unsigned long long val);
kHz_t operator""_kHz(long double val);
MHz_t operator""_MHz(unsigned long long val);
MHz_t operator""_MHz(long double val);
GHz_t operator""_GHz(unsigned long long val);
GHz_t operator""_GHz(long double val);
THz_t operator""_THz(unsigned long long val);
THz_t operator""_THz(long double val);

std::ostream& operator<<(std::ostream& os, const Hz_t& rhs);
std::istream& operator>>(std::istream& is, Hz_t& rhs);

std::ostream& operator<<(std::ostream& os, const kHz_t& rhs);
std::istream& operator>>(std::istream& is, kHz_t& rhs);

std::ostream& operator<<(std::ostream& os, const MHz_t& rhs);
std::istream& operator>>(std::istream& is, MHz_t& rhs);

std::ostream& operator<<(std::ostream& os, const GHz_t& rhs);
std::istream& operator>>(std::istream& is, GHz_t& rhs);

std::ostream& operator<<(std::ostream& os, const THz_t& rhs);
std::istream& operator>>(std::istream& is, THz_t& rhs);

Hz_t operator*(double lhs, const Hz_t& rhs);
double operator*(Time nstime, const Hz_t& rhs);

/// @cond Doxygen warning against macro internals
ATTRIBUTE_VALUE_DEFINE_WITH_NAME(Hz_t, Hz); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(Hz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(Hz_t, Hz, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(kHz_t, kHz); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(kHz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(kHz_t, kHz, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(MHz_t, MHz); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(MHz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(MHz_t, MHz, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(GHz_t, GHz); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(GHz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(GHz_t, GHz, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(THz_t, THz); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(THz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(THz_t, THz, Double);
/// @endcond
} // namespace ns3

#endif // UNITS_FREQUENCY_H
