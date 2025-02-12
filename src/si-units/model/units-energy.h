#ifndef UNITS_ENERGY_H
#define UNITS_ENERGY_H

#include "format-string.h"
#include "units-aliases.h"
#include "units-frequency.h"
#include "units-time.h"

#include "ns3/abort.h"
#include "ns3/attribute-helper.h"
#include "ns3/attribute.h"
#include "ns3/double.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <math.h> // To support Linux. <cmath> lacks log10l().
#include <optional>

// clang-format off
namespace ns3
{

struct dB_t;
struct dBm_t;
struct mWatt_t;
struct Watt_t;

/// Convert energy value in linear scale into log scale.
/// @param val Energy value in linear scale.
/// @return Energy value in log scale.
inline double
ToLogScale(double val)
{
    NS_ABORT_IF(val <= 0.);
    return 10.0 * log10l(val); // NOLINT
}

/// Convert energy value in log scale into linear scale.
/// @param val Energy value in log scale.
/// @return Energy value in linear scale.
inline double
ToLinearScale(double val)
{
    return std::pow(10.0, val / 10.0); // NOLINT
}

/// dB type
struct dB_t
{
    double val{}; ///< Value in dB

    dB_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in degree
    constexpr explicit dB_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param val Value in dB
    constexpr explicit dB_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param str a string representation of a dB value
    explicit dB_t(const std::string& str);

    /// Converts to string.
    /// @return String representation of dB_t object.
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1f dB", val);
    }

    /// Converts from linear scale.
    /// @param input Value in linear scale.
    /// @return dB_t object.
    static dB_t from_linear(double input) // NOLINT(readability-identifier-naming)
    {
        return dB_t{ToLogScale(input)};
    }

    /// Converts to linear scale.
    /// @return Value in linear scale.
    double to_linear() const // NOLINT(readability-identifier-naming)
    {
        return ToLinearScale(val);
    }

    /// Get value in dB.
    /// @return Value in dB.
    double in_dB() const // NOLINT(readability-identifier-naming)
    {
        return val;
    }

    /// Get value in linear scale.
    /// @return Value in linear scale.
    double in_linear() const // NOLINT(readability-identifier-naming)
    {
        return to_linear();
    }

    /// Converts from vector of linear scale.
    /// @param input Vector of linear scale.
    /// @return Vector of dB_t objects.
    static std::vector<dB_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<dB_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return dB_t{f};
        });
        return output;
    }

    /// Converts to vector of linear scale.
    /// @param input Vector of dB_t objects.
    /// @return Vector of linear scale.
    static std::vector<double> to_doubles(
        std::vector<dB_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](dB_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const dB_t& rhs) const = default;

    /// Negation operator.
    /// @return Negated value.
    inline dB_t operator-() const // Negation
    {
        return dB_t{-val};
    }

    /// Addition operator.
    /// @param rhs value to add
    /// @return Sum of values.
    inline dB_t operator+(const dB_t& rhs) const
    {
        return dB_t{val + rhs.val};
    }

    /// Subtraction operator.
    /// @param rhs value to subtract
    /// @return Difference of values.
    inline dB_t operator-(const dB_t& rhs) const
    {
        return dB_t{val - rhs.val};
    }

    /// Addition assignment operator.
    /// @param rhs value to add
    /// @return Reference to self.
    inline dB_t& operator+=(const dB_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction assignment operator.
    /// @param rhs value to subtract
    /// @return Reference to self.
    inline dB_t& operator-=(const dB_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Addition of dBms
    /// @param rhs value to add
    /// @return Sum of values.
    dBm_t operator+(const dBm_t& rhs) const;

    /// Subtraction of dBms
    /// @param rhs value to subtract
    /// @return Difference of values.
    dBm_t operator-(const dBm_t& rhs) const;

    /// Multiplier Operator
    /// @param rhs right-hand side operand in double type
    /// @returns a dB_t power unit struct with value equals to val * rhs
    inline dB_t operator*(double rhs) const
    {
        return dB_t{val * rhs};
    }

    /// Divider Operator
    /// @param rhs right-hand side operand in double type
    /// @returns a dB_t power unit struct with value equals to val / rhs
    inline dB_t operator/(double rhs) const
    {
        return dB_t{val / rhs};
    }

    /// Convert from string.
    /// @param input string to convert
    /// @return Optional dB value.
    static std::optional<dB_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

/// dBr type
using dBr_t = dB_t;

/// dBm type
struct dBm_t
{
    double val{}; ///< Value in dBm

    dBm_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in dBm
    constexpr explicit dBm_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param val Value in dBm
    constexpr explicit dBm_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param str a string representation of a dBm value
    explicit dBm_t(const std::string& str);

    /// Converts from mWatt
    /// @param input input power struct in mWatt_t
    /// @return dBm_t object
    static dBm_t from_mWatt(const mWatt_t& input); // NOLINT(readability-identifier-naming)

    /// Converts to mWatt
    /// @return power unit struct in mWatt_t
    mWatt_t to_mWatt() const; // NOLINT(readability-identifier-naming)

    /// Gets power unit in mWatt
    /// @return power unit in mWatt
    double in_mWatt() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Sets power unit in dBm_t from Watt_t
    /// @param input input power struct in Watt_t
    /// @returns power unit struct in dBm_t
    static dBm_t from_Watt(const Watt_t& input); // NOLINT(readability-identifier-naming)

    /// Converts a dBm_t power unit struct to Watt_t
    /// @returns power unit struct in Watt_t
    Watt_t to_Watt() const; // NOLINT(readability-identifier-naming)

    /// Returns a dBm_t power unit struct value in Watt_t
    /// @returns a power unit value in Watt_t
    double in_Watt() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Returns a dBm_t power unit struct value in dBm_t
    /// @returns a power unit value in dBm_t
    double in_dBm() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Represents the power unit as a string
    /// @returns a string representation of the power unit
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1f dBm", val);
    }

    /// Converts a vector of doubles representing in dBm to a vector of dBm_t
    /// @param input input power values in dBm
    /// @returns a vector of dBm_t
    static std::vector<dBm_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<dBm_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return dBm_t{f};
        });
        return output;
    }

    /// Converts a vector of dBm_t to a vector of doubles
    /// @param input input power values in dBm_t
    /// @returns a vector of doubles
    static std::vector<double> to_doubles(
        std::vector<dBm_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](dBm_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const dBm_t& rhs) const = default;

    /// Negation operator.
    /// @returns a new dBm_t value with the value negated
    inline dBm_t operator-() const
    {
        return dBm_t{-val};
    }

    /// Addition operator.
    /// @param rhs value to add
    /// @returns a new dBm_t value with the value added
    inline dBm_t operator+(const dB_t& rhs) const
    {
        return dBm_t{val + rhs.val};
    }

    /// Addition assignment operator.
    /// @param rhs value to add
    /// @returns a reference to this object
    inline dBm_t& operator+=(const dB_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction operator.
    /// @param rhs value to subtract
    /// @returns a new dBm_t value with the value subtracted
    inline dBm_t operator-(const dB_t& rhs) const
    {
        return dBm_t{val - rhs.val};
    }

    /// Subtraction assignment operator.
    /// @param rhs value to subtract
    /// @returns a reference to this object
    inline dBm_t& operator-=(const dB_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    // Disallow addition/subtraction between dBm_t and mWatt_t.
    // Convert operands to the same unit first before addition or subtraction operation

// Energy's addition and subtraction in dBm_t
// are prohibited due to the fact that those operators have conflicting meanings.
// While energy may be added or separated/consumed, + and - have the meanings of
// multiplication and division respectively in log scale. In order to avoid this confusion
// and potential harm, addition and subtraction of energy is allowed only in the linear scale.
#if false // IMPLEMENTED BUT NOT ALLOWED TO BE USED
    inline dBm_t operator+(const dBm_t& rhs) const
  {
        return dBm_t{ToLogScale(ToLinearScale(val) + ToLinearScale(rhs.val))};
  }

  // Subtraction in energy may mean consumption
    inline dBm_t operator-(const dBm_t& rhs) const
  {
        return dBm_t{ToLogScale(ToLinearScale(val) - ToLinearScale(rhs.val))};
  }

    inline dBm_t& operator+=(const dBm_t& rhs)
  {
        val = ToLogScale(ToLinearScale(val) + ToLinearScale(rhs.val));
    return *this;
  }

    inline dBm_t& operator-=(const dBm_t& rhs)
  {
        val = ToLogScale(ToLinearScale(val) - ToLinearScale(rhs.val));
    return *this;
  }
#endif    // IMPLEMENTED BUT NOT ALLOWED TO BE USED

    static std::optional<dBm_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

/// mWatt
struct mWatt_t
{
    double val{}; ///< Value in mWatt

    mWatt_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in degree
    constexpr explicit mWatt_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param val Value in mWatt
    constexpr explicit mWatt_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param str a string representation of a mWatt value
    explicit mWatt_t(const std::string& str);

    // Note these are exceptions to method naming rules in order output be consistent. dBm_t,
    // mWatt_t, and Watt_t conversion method names.

    /// Conversion from dBm_t
    /// @param input input power struct in dBm_t
    /// @returns power unit struct in mWatt_t
    static mWatt_t from_dBm(const dBm_t& input); // NOLINT(readability-identifier-naming)

    /// Conversion to dBm_t
    /// @returns power unit struct in dBm_t
    dBm_t to_dBm() const; // NOLINT(readability-identifier-naming)

    /// Get the value in dBm
    /// @returns power unit in dBm
    double in_dBm() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Sets power unit in mWatt_t from Watt_t
    /// @param input input power struct in Watt_t
    /// @returns power unit struct in mWatt_t
    static mWatt_t from_Watt(const Watt_t& input); // NOLINT(readability-identifier-naming)

    /// Converts a mWatt_t power unit struct to Watt_t
    /// @returns power unit struct in Watt_t
    Watt_t to_Watt() const; // NOLINT(readability-identifier-naming)

    /// Returns a mWatt_t power unit struct value in Watt_t
    /// @returns a power unit value in Watt_t
    double in_Watt() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Returns a mWatt_t power unit struct value in mWatt_t
    /// @returns a power unit value in mWatt_t
    double in_mWatt() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Returns a string representation of the power unit
    /// @returns a string representation of the power unit
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1f mWatt", val);
    }

    /// Converts a vector of double values represented in mWatt to a vector of mWatt_t
    /// @param input vector of double values in mWatt
    /// @returns vector of mWatt_t values
    static std::vector<mWatt_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<mWatt_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return mWatt_t{f};
        });
        return output;
    }

    /// Converts a vector of mWatt_t values to a vector of double values
    /// @param input vector of mWatt_t values
    /// @returns vector of double values in mWatt
    static std::vector<double> to_doubles(
        std::vector<mWatt_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](mWatt_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const mWatt_t& rhs) const = default;

    /// Equality comparison
    /// @param rhs right hand side
    /// @return true if equal, false otherwise
    bool operator==(const mWatt_t& rhs) const = default;

    /// Negation Operator
    /// @returns a mWatt_t power unit struct with value equals to -val
    inline mWatt_t operator-() const
    {
        return mWatt_t{-val};
    }

    /// Addition Operator
    /// @param rhs value to add
    /// @returns a mWatt_t power unit struct with value equals to val + rhs.val
    inline mWatt_t operator+(const mWatt_t& rhs) const
    {
        return mWatt_t{val + rhs.val};
    }

    /// Subtraction Operator
    /// @param rhs value to subtract
    /// @returns a mWatt_t power unit struct with value equals to val - rhs.val
    inline mWatt_t operator-(const mWatt_t& rhs) const
    {
        return mWatt_t{val - rhs.val};
    }

    /// Addition Assignment Operator
    /// @param rhs value to add
    /// @returns a reference to the current mWatt_t power unit struct
    inline mWatt_t& operator+=(const mWatt_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction Assignment Operator
    /// @param rhs value to subtract
    /// @returns a reference to the current mWatt_t power unit struct
    inline mWatt_t& operator-=(const mWatt_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Multiplier Operator
    /// @param rhs right-hand side operand in double type
    /// @returns a mWatt_t power unit struct with value equals to val * rhs
    inline mWatt_t operator*(double rhs) const
    {
        return mWatt_t{val * rhs};
    }

    /// Divider Operator
    /// @param rhs right-hand side operand in double type
    /// @returns a mWatt_t power unit struct with value equals to val / rhs
    inline mWatt_t operator/(double rhs) const
    {
        return mWatt_t{val / rhs};
    }

    // mWatt_t - Watt_t Cross Operations
    // Keeping classes simple, even if duplication is present. Preferring faster runtime.

    /// Equality Operator
    /// @param rhs right-hand side operand
    /// @returns true if the two power values are equal
    bool operator==(const Watt_t& rhs) const;

    /// Inequality Operator
    /// @param rhs right-hand side operand
    /// @returns true if the two power values are not equal
    bool operator!=(const Watt_t& rhs) const;

    /// Less Than Comparison Operator
    /// @param rhs right-hand side operand
    /// @returns true if the power value is less than the rhs value
    bool operator<(const Watt_t& rhs) const;

    /// Greater Than Comparison Operator
    /// @param rhs input Watt_t power unit struct to compare to
    /// @returns true if the power value is greater than the rhs value
    bool operator>(const Watt_t& rhs) const;

    /// Less Than or Equal Comparison Operator
    /// @param rhs right-hand side operand
    /// @returns true if the power value is less than or equal rhs value
    bool operator<=(const Watt_t& rhs) const;

    /// Greater Than or Equal Comparison Operator
    /// @param rhs right-hand side operand
    /// @returns true if the power value is greater than or equal the rhs value
    bool operator>=(const Watt_t& rhs) const;

    /// Addition Operator
    /// @param rhs right-hand side operand
    /// @returns a mWatt_t power unit struct with value equals the sum of the two
    mWatt_t operator+(const Watt_t& rhs) const;

    /// Subtraction Operator
    /// @param rhs right-hand side operand
    /// @returns a mWatt_t power unit struct with value equals val - rhs.val
    mWatt_t operator-(const Watt_t& rhs) const;

    /// Addition Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a mWatt_t power unit struct with value equals to val + rhs.val
    mWatt_t& operator+=(const Watt_t& rhs);

    /// Subtraction Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a mWatt_t power unit struct with value equals to val - rhs.val
    mWatt_t& operator-=(const Watt_t& rhs);

    static std::optional<mWatt_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

/// Multiplier Operator for mWatt_t
/// @param lfs left-hand side operand in double type
/// @param rhs right-hand side operand in mWatt_t type
/// @returns a mWatt_t power unit struct with value equals to lfs * rhs.val
mWatt_t operator*(const double& lfs, const mWatt_t& rhs);

/// Watt_t power unit structure
struct Watt_t
{
    double val{}; ///< Value in Watt

    Watt_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in degree
    constexpr explicit Watt_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param val Value in Watt
    constexpr explicit Watt_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param str a string representation of a Watt value
    explicit Watt_t(const std::string& str);

    // mWatt_t-Watt_t-dBm_t Conversion Operators
    /// Sets power unit in Watt_t from dBm_t
    /// @param input input power struct in dBm_t
    /// @returns power unit struct in Watt_t
    static Watt_t from_dBm(const dBm_t& input); // NOLINT(readability-identifier-naming)

    /// Converts a Watt_t power unit struct to dBm_t
    /// @returns power unit struct in dBm_t
    dBm_t to_dBm() const; // NOLINT(readability-identifier-naming)

    /// Returns a Watt_t power unit struct value in dBm_t
    /// @returns a power unit value in dBm_t
    double in_dBm() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Sets power unit in Watt_t from mWatt_t
    /// @param input input power struct in mWatt_t
    /// @returns power unit struct in Watt_t
    static Watt_t from_mWatt(const mWatt_t& input); // NOLINT(readability-identifier-naming)

    /// Converts a Watt_t power unit struct to mWatt_t
    /// @returns power unit struct in mWatt_t
    mWatt_t to_mWatt() const; // NOLINT(readability-identifier-naming)

    /// Returns a Watt_t power unit struct value in mWatt_t
    /// @returns a power unit value in mWatt_t
    double in_mWatt() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Returns Watt_t power unit struct value
    /// @returns power unit value in Watt_t
    double in_Watt() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Returns Watt_t power unit value in floating point string representations
    /// @returns a string of format "%.1f Watt" representing the power value in Watt_t
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1f Watt", val);
    }

    /// Rerturns a vector of Watt_t power unit structs from an input vector values
    /// @param input a vector of input power values in Watt_t
    /// @returns  a vector of Watt_t power unit structs
    static std::vector<Watt_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<Watt_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return Watt_t{f};
        });
        return output;
    }

    /// Converts a vector of Watt_t power unit structs to a output vector values
    /// @param input a vector of Watt_t power unit structs
    /// @returns a vector of output power values in Watt_t
    static std::vector<double> to_doubles(
        std::vector<Watt_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](Watt_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    // Watt_t-Watt_t Operators
    // Keeping classes simple, even if duplication is present. Preferring faster runtime.

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const Watt_t& rhs) const = default;

    /// Equality comparison
    /// @param rhs right hand side
    /// @return true if equal, false otherwise
    bool operator==(const Watt_t& rhs) const = default;

    /// Arithmetic Negation Operator
    /// @returns negated power value
    inline Watt_t operator-() const
    {
        return Watt_t{-val};
    }

    /// Addition Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals the sum of the two
    inline Watt_t operator+(const Watt_t& rhs) const
    {
        return Watt_t{val + rhs.val};
    }

    /// Subtraction Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals val - rhs.val
    inline Watt_t operator-(const Watt_t& rhs) const
    {
        return Watt_t{val - rhs.val};
    }

    /// Addition Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals to val + rhs.val
    inline Watt_t& operator+=(const Watt_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals to val - rhs.val
    inline Watt_t& operator-=(const Watt_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Multiplication Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals to val * rhs.val
    inline Watt_t operator*(const Watt_t& rhs) const
    {
        return Watt_t{val * rhs.val};
    }

    /// Multiplier Operator
    /// @param rhs right-hand side operand in double type
    /// @returns a Watt_t power unit struct with value equals to val * rhs
    inline Watt_t operator*(double rhs) const
    {
        return Watt_t{val * rhs};
    }

    /// Divider Operator
    /// @param rhs right-hand side operand in double type
    /// @returns a Watt_t power unit struct with value equals to val / rhs
    inline Watt_t operator/(double rhs) const
    {
        return Watt_t{val / rhs};
    }

    /// Divider Operator
    /// @param rhs right-hand side operand in Watt_t type
    /// @returns a ratio with value equals to val / rhs.val
    inline double operator/(const Watt_t& rhs) const
    {
        return val / rhs.val;
    }

    /// Multiplication Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals to val * rhs
    inline Watt_t& operator*=(double rhs)
    {
        val *= rhs;
        return *this;
    }

    // Watt_t-mWatt_t Cross-operators
    // Keeping classes simple, even if duplication is present. Preferring faster runtime.

    /// Equality Operator
    /// @param rhs right-hand side operand
    /// @returns true if the two power values are equal
    bool operator==(const mWatt_t& rhs) const;

    /// Inequality Operator
    /// @param rhs right-hand side operand
    /// @returns true if the two power values are not equal
    bool operator!=(const mWatt_t& rhs) const;

    /// Less Than Comparison Operator
    /// @param rhs right-hand side operand
    /// @returns true if the power value is less than the rhs value
    bool operator<(const mWatt_t& rhs) const;

    /// Greater Than Comparison Operator
    /// @param rhs right-hand side operand
    /// @returns true if the power value is greater than the rhs value
    bool operator>(const mWatt_t& rhs) const;

    /// Less Than or Equal Comparison Operator
    /// @param rhs right-hand side operand
    /// @returns true if the power value is less than or equal the rhs value
    bool operator<=(const mWatt_t& rhs) const;

    /// Greater Than or Equal Comparison Operator
    /// @param rhs right-hand side operand
    /// @returns true if the power value is greater than or equal the rhs value
    bool operator>=(const mWatt_t& rhs) const;

    /// Addition Operator
    /// @param rhs right-hand side operand
    /// @returns a mWatt_t power unit struct with value equals the sum of the two values
    Watt_t operator+(const mWatt_t& rhs) const;

    /// Subtraction Operator
    /// @param rhs right-hand side operand
    /// @returns a mWatt_t power unit struct with value equals val - rhs.val
    Watt_t operator-(const mWatt_t& rhs) const;

    /// Addition Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals to val + rhs.val
    Watt_t& operator+=(const mWatt_t& rhs);

    /// Subtraction Assignment Operator
    /// @param rhs right-hand side operand
    /// @returns a Watt_t power unit struct with value equals to val - rhs.val
    Watt_t& operator-=(const mWatt_t& rhs);

    static std::optional<Watt_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

/// Power spectral density
struct dBm_per_Hz_t // NOLINT(readability-identifier-naming)
{
    double val{}; ///< Value in dBm/Hz

    dBm_per_Hz_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in degree
    constexpr explicit dBm_per_Hz_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param val Value in dBm/Hz
    constexpr explicit dBm_per_Hz_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param str a string representation of a dBm/Hz_t value
    explicit dBm_per_Hz_t(const std::string& str);

    /// Get value in dBm
    /// @returns a double value in dBm
    double in_dBm() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Convert to dBm
    /// @returns a dBm_t power unit struct with value equals to val
    dBm_t to_dBm() const // NOLINT(readability-identifier-naming)
    {
        return dBm_t{val};
    }

    /// Converts from a vector of double values represented in dBm/Hz to a vector of dBm_per_Hz_t
    /// @param input vector of double values represented in dBm/Hz
    /// @returns a vector of dBm_per_Hz_t values
    static std::vector<dBm_per_Hz_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<dBm_per_Hz_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return dBm_per_Hz_t{f};
        });
        return output;
    }

    /// Converts from a vector of dBm_per_Hz_t values to a vector of double values represented in
    /// dBm/Hz
    /// @param input vector of dBm_per_Hz_t values
    /// @returns a vector of double values represented in dBm/Hz
    static std::vector<double> to_doubles(
        std::vector<dBm_per_Hz_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](dBm_per_Hz_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    /// Represents a value in dBm/Hz as a string
    /// @returns a string representation of the value
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1Lf dBm/Hz", val);
    }

    /// Calculate average PSD
    /// @param power power in dBm
    /// @param bandwidth bandwidth in Hz
    /// @returns average PSD in dBm/Hz
    static dBm_per_Hz_t AveragePsd(dBm_t power, Hz_t bandwidth)
    {
        return dBm_per_Hz_t{power.val - ToLogScale(static_cast<double>(bandwidth.val))};
    }

    /// Calculate average PSD
    /// @param power power in dBm
    /// @param bandwidth bandwidth in MHz
    /// @returns average PSD in dBm/Hz
    static dBm_per_Hz_t AveragePsd(dBm_t power, MHz_t bandwidth)
    {
        return AveragePsd(power, bandwidth.to_Hz());
    }

    /// Calculate power over bandwidth
    /// @param rhs bandwidth in Hz
    /// @returns power over bandwidth in dBm
    inline dBm_t OverBandwidth(const Hz_t& rhs) const
    {
        return dBm_t{val + ToLogScale(static_cast<double>(rhs.val))};
    }

    /// Calculate power over bandwidth
    /// @param rhs bandwidth in MHz
    /// @returns power over bandwidth in dBm
    inline dBm_t OverBandwidth(const MHz_t& rhs) const
    {
        return OverBandwidth(rhs.to_Hz());
    }

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const dBm_per_Hz_t& rhs) const = default;

    /// Negation operator
    /// @returns negated value
    inline dBm_per_Hz_t operator-() const
    {
        return dBm_per_Hz_t{-val};
    }

    /// Addition operator
    /// @param rhs value to add
    /// @returns sum
    inline dBm_per_Hz_t operator+(const dB_t& rhs) const
    {
        return dBm_per_Hz_t{val + rhs.val};
    }

    /// Addition assignment operator
    /// @param rhs value to add
    /// @returns sum
    inline dBm_per_Hz_t& operator+=(const dB_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction operator
    /// @param rhs value to subtract
    /// @returns difference
    inline dBm_per_Hz_t operator-(const dB_t& rhs) const
    {
        return dBm_per_Hz_t{val - rhs.val};
    }

    /// Subtraction assignment operator
    /// @param rhs value to subtract
    /// @returns difference
    inline dBm_per_Hz_t& operator-=(const dB_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Converts from string
    /// @param input string to convert
    /// @returns converted value or empty optional if conversion failed
    static std::optional<dBm_per_Hz_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

/// Power spectral density
struct dBm_per_MHz_t // NOLINT(readability-identifier-naming)
{
    double val{}; ///< Value in dBm/MHz

    dBm_per_MHz_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in degree
    constexpr explicit dBm_per_MHz_t(int32_t val)
        : val(static_cast<double>(val))
    {
    }

    /// Constructor
    /// @param val Value in dBm/MHz
    constexpr explicit dBm_per_MHz_t(double val)
        : val(val)
    {
    }

    /// Constructor
    /// @param str a string representation of a dBm/MHz_t value
    explicit dBm_per_MHz_t(const std::string& str);

    /// Get value in dBm
    /// @returns a double value in dBm
    double in_dBm() const; // NOLINT(readability-identifier-naming). Return quantity only

    /// Convert to dBm
    /// @returns a dBm_t power unit struct with value equals to val
    dBm_t to_dBm() const // NOLINT(readability-identifier-naming)
    {
        return dBm_t{val};
    }

    /// Converts from a vector of double values represented in dBm/MHz to a vector of dBm_per_MHz_t
    /// @param input vector of double values represented in dBm/MHz
    /// @returns a vector of dBm_per_MHz_t values
    static std::vector<dBm_per_MHz_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<dBm_per_MHz_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return dBm_per_MHz_t{f};
        });
        return output;
    }

    /// Converts from a vector of dBm_per_MHz_t values to a vector of double values represented in
    /// dBm/MHz
    /// @param input vector of dBm_per_MHz_t values
    /// @returns a vector of double values represented in dBm/MHz
    static std::vector<double> to_doubles(
        std::vector<dBm_per_MHz_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](dBm_per_MHz_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    /// Represents a value in dBm/MHz as a string
    /// @returns a string representation of the value
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1Lf dBm/Hz", val);
    }

    /// Calculate average PSD
    /// @param power power in dBm
    /// @param bandwidth bandwidth in MHz
    /// @returns average PSD in dBm/MHz
    static dBm_per_MHz_t AveragePsd(dBm_t power, MHz_t bandwidth)
    {
        return dBm_per_MHz_t{power.val - ToLogScale(static_cast<double>(bandwidth.val))};
    }

    /// Calculate average PSD
    /// @param power power in dBm
    /// @param bandwidth bandwidth in Hz
    /// @returns average PSD in dBm/MHz
    static dBm_per_MHz_t AveragePsd(dBm_t power, Hz_t bandwidth)
    {
        return AveragePsd(power, bandwidth.to_MHz());
    }

    /// Calculate power over bandwidth
    /// @param rhs bandwidth in MHz
    /// @returns power over bandwidth in dBm
    inline dBm_t OverBandwidth(const MHz_t& rhs) const
    {
        return dBm_t{val + ToLogScale(static_cast<double>(rhs.val))};
    }

    /// Calculate power over bandwidth
    /// @param rhs bandwidth in Hz
    /// @returns power over bandwidth in dBm
    inline dBm_t OverBandwidth(const Hz_t& rhs) const
    {
        return OverBandwidth(rhs.to_MHz());
    }

    /// Three-way comparison
    /// @param rhs right hand side
    /// @return deduced comparison type
    auto operator<=>(const dBm_per_MHz_t& rhs) const = default;

    /// Negation operator
    /// @returns negated value
    inline dBm_per_MHz_t operator-() const
    {
        return dBm_per_MHz_t{-val};
    }

    /// Addition operator
    /// @param rhs value to add
    /// @returns sum
    inline dBm_per_MHz_t operator+(const dB_t& rhs) const
    {
        return dBm_per_MHz_t{val + rhs.val};
    }

    /// Addition assignment operator
    /// @param rhs value to add
    /// @returns sum
    inline dBm_per_MHz_t& operator+=(const dB_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction operator
    /// @param rhs value to subtract
    /// @returns difference
    inline dBm_per_MHz_t operator-(const dB_t& rhs) const
    {
        return dBm_per_MHz_t{val - rhs.val};
    }

    /// Subtraction assignment operator
    /// @param rhs value to subtract
    /// @returns difference
    inline dBm_per_MHz_t& operator-=(const dB_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Converts from string
    /// @param input string to convert
    /// @returns converted value or empty optional if conversion failed
    static std::optional<dBm_per_MHz_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

dB_t operator""_dB(long double val);
dB_t operator""_dB(unsigned long long val);
dBr_t operator""_dBr(long double val);
dBr_t operator""_dBr(unsigned long long val);
dBm_t operator""_dBm(long double val);
dBm_t operator""_dBm(unsigned long long val);
mWatt_t operator""_mWatt(long double val);
mWatt_t operator""_mWatt(unsigned long long val);
mWatt_t operator""_pWatt(long double val);
mWatt_t operator""_pWatt(unsigned long long val);
Watt_t operator""_Watt(long double val);
Watt_t operator""_Watt(unsigned long long val);
dBm_per_Hz_t operator""_dBm_per_Hz(long double val);
dBm_per_MHz_t operator""_dBm_per_MHz(long double val);

std::ostream& operator<<(std::ostream& os, const dB_t& rhs);
std::ostream& operator<<(std::ostream& os, const dBm_t& rhs);
std::ostream& operator<<(std::ostream& os, const mWatt_t& rhs);
std::ostream& operator<<(std::ostream& os, const Watt_t& rhs);
std::ostream& operator<<(std::ostream& os, const dBm_per_Hz_t& rhs);
std::ostream& operator<<(std::ostream& os, const dBm_per_MHz_t& rhs);

std::istream& operator>>(std::istream& is, dB_t& q);
std::istream& operator>>(std::istream& is, dBm_t& q);
std::istream& operator>>(std::istream& is, mWatt_t& q);
std::istream& operator>>(std::istream& is, Watt_t& q);
std::istream& operator>>(std::istream& is, dBm_per_Hz_t& q);
std::istream& operator>>(std::istream& is, dBm_per_MHz_t& q);

// Related physical constants
const auto THERMAL_NOISE_AT_290K = -173.9763_dBm_per_Hz; ///< Thermal noise reference

/// @cond Doxygen warning against the internal of the macro
ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dB_t, dB); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(dB);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dB_t, dB, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dBr_t, dBr); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(dBr);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dBr_t, dBr, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dBm_t, dBm); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(dBm);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dBm_t, dBm, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(mWatt_t, mWatt); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(mWatt);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(mWatt_t, mWatt, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(Watt_t, Watt); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(Watt);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(Watt_t, Watt, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dBm_per_Hz_t, dBm_per_Hz); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(dBm_per_Hz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dBm_per_Hz_t, dBm_per_Hz, Double);

ATTRIBUTE_VALUE_DEFINE_WITH_NAME(dBm_per_MHz_t,
                                 dBm_per_MHz); // See si-units-test-suite.cc for usages
ATTRIBUTE_ACCESSOR_DEFINE(dBm_per_MHz);
ATTRIBUTE_CHECKER_DEFINE_WITH_CONVERTER(dBm_per_MHz_t, dBm_per_MHz, Double);
/// @endcond
} // namespace ns3

#endif // UNITS_ENERGY_H
