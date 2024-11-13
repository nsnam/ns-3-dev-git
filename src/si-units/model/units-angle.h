#ifndef UNITS_ANGLE_H
#define UNITS_ANGLE_H

// Angles are dimensionless but still mentioned in the SI as an accepted unit.
// This intricacy belongs to SI not to implementation here.
//
// M_PI is 3.141.. in numeric value and is defined either in <math.h> or <cmath>.
// Note it is of radian_t not of a degree_t.

#include "format-string.h"

#include "ns3/attribute-helper.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <iostream>
#include <optional>
#include <vector>

// clang-format off

namespace ns3
{

struct radian_t;

/// Degree (angle)
struct degree_t
{
    double val{}; ///< Value in degree

    degree_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in degree
    constexpr explicit degree_t(double val)
        : val(val)
    {
    }

    /// Conversion from radian
    /// @param input radian
    /// @return degree
    static degree_t from_radian(const radian_t& input); // NOLINT(readability-identifier-naming)

    /// Conversion to radian
    /// @return radian
    radian_t to_radian() const; // NOLINT(readability-identifier-naming)

    /// Get the value in radian
    /// @return value in radian
    double in_radian() const; // NOLINT(readability-identifier-naming)

    /// Get the value in degree
    /// @return value in degree
    double in_degree() const; // NOLINT(readability-identifier-naming)

    /// String representation
    /// @return string representation of the angle
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1f degree", val);
    }

    /// Convert from a vector of doubles to a vector of degrees
    /// @param input vector of doubles
    /// @return vector of degrees
    static std::vector<degree_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<degree_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return degree_t{f};
        });
        return output;
    }

    /// Convert from a vector of degrees to a vector of doubles
    /// @param input vector of degrees
    /// @return vector of doubles
    static std::vector<double> to_doubles(
        std::vector<degree_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](degree_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    /// Equality operator
    /// @param rhs right hand side of the operator
    /// @return true if equal, false otherwise
    inline bool operator==(const degree_t& rhs) const
    {
        return val == rhs.val;
    }

    /// Inequality operator
    /// @param rhs right hand side of the operator
    /// @return true if not equal, false otherwise
    inline bool operator!=(const degree_t& rhs) const
    {
        return !(operator==(rhs));
    }

    /// Less than operator
    /// @param rhs right hand side of the operator
    /// @return true if less than, false otherwise
    inline bool operator<(const degree_t& rhs) const
    {
        return val < rhs.val;
    }

    /// Greater than operator
    /// @param rhs right hand side of the operator
    /// @return true if greater than, false otherwise
    inline bool operator>(const degree_t& rhs) const
    {
        return val > rhs.val;
    }

    /// Less than or equal to operator
    /// @param rhs right hand side of the operator
    /// @return true if less than or equal to, false otherwise
    inline bool operator<=(const degree_t& rhs) const
    {
        return val <= rhs.val;
    }

    /// Greater than or equal to operator
    /// @param rhs right hand side of the operator
    /// @return true if greater than or equal to, false otherwise
    inline bool operator>=(const degree_t& rhs) const
    {
        return val >= rhs.val;
    }

    /// Unary minus operator
    /// @return negative of the degree
    inline degree_t operator-() const
    {
        return degree_t{-val};
    }

    /// Addition operator
    /// @param rhs right hand side of the operator
    /// @return sum of the degrees
    inline degree_t operator+(const degree_t& rhs) const
    {
        return degree_t{val + rhs.val};
    }

    /// Subtraction operator
    /// @param rhs right hand side of the operator
    /// @return difference of the degrees
    inline degree_t operator-(const degree_t& rhs) const
    {
        return degree_t{val - rhs.val};
    }

    /// Addition assignment operator
    /// @param rhs right hand side of the operator
    /// @return sum of the degrees
    inline degree_t& operator+=(const degree_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction assignment operator
    /// @param rhs right hand side of the operator
    /// @return difference of the degrees
    inline degree_t& operator-=(const degree_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Normalizes the angle to [0.0, 360.0)
    /// @return product of the degrees in [0.0, 360.0)
    inline degree_t& normalize() // NOLINT(readability-identifier-naming)
    {
        val = std::remainder(val + 180.0, 360.0);      // NOLINT
        val = (val < 0.0) ? val + 180.0 : val - 180.0; // NOLINT
        return *this;
    }

    /// Converts the string to degree
    /// @param input string to be converted
    /// @return degree value if the string is valid, empty optional otherwise
    static std::optional<degree_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

/// Radians (angle)
struct radian_t
{
    double val{}; ///< Value in radian

    radian_t() = default; ///< Default constructor

    /// Constructor
    /// @param val Value in radian
    constexpr explicit radian_t(double val)
        : val(val)
    {
    }

    /// Conversion from degree
    /// @param input degree
    /// @return radian
    static radian_t from_degree(const degree_t& input); // NOLINT(readability-identifier-naming)

    /// Conversion to degree
    /// @return degree
    degree_t to_degree() const; // NOLINT(readability-identifier-naming)

    /// Get the value in degree
    /// @return value in degree
    double in_degree() const; // NOLINT(readability-identifier-naming)

    /// Get the value in radian
    /// @return value in radian
    double in_radian() const; // NOLINT(readability-identifier-naming)

    /// String representation
    /// @return string representation of the angle
    std::string str() const // NOLINT(readability-identifier-naming)
    {
        return sformat("%.1f radian", val);
    }

    /// Convert from a vector of doubles to a vector of radians
    /// @param input vector of doubles
    /// @return vector of radians
    static std::vector<radian_t> from_doubles(
        std::vector<double>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<radian_t> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](double f) {
            return radian_t{f};
        });
        return output;
    }

    /// Convert from a vector of radians to a vector of doubles
    /// @param input vector of radians
    /// @return vector of doubles
    static std::vector<double> to_doubles(
        std::vector<radian_t>& input) // NOLINT(readability-identifier-naming)
    {
        std::vector<double> output(input.size());
        std::transform(input.cbegin(), input.cend(), output.begin(), [](radian_t f) {
            return static_cast<double>(f.val);
        });
        return output;
    }

    /// Equality operator
    /// @param rhs right hand side of the equality operator
    /// @return true if the two angles are equal, false otherwise
    inline bool operator==(const radian_t& rhs) const
    {
        return val == rhs.val;
    }

    /// Inequality operator
    /// @param rhs right hand side of the inequality operator
    /// @return true if the two angles are not equal, false otherwise
    inline bool operator!=(const radian_t& rhs) const
    {
        return !(operator==(rhs));
    }

    /// Less than operator
    /// @param rhs right hand side of the less than operator
    /// @return true if the left hand side is less than the right hand side, false otherwise
    inline bool operator<(const radian_t& rhs) const
    {
        return val < rhs.val;
    }

    /// Greater than operator
    /// @param rhs right hand side of the greater than operator
    /// @return true if the left hand side is greater than the right hand side, false otherwise
    inline bool operator>(const radian_t& rhs) const
    {
        return val > rhs.val;
    }

    /// Less than or equal to operator
    /// @param rhs right hand side of the less than or equal to operator
    /// @return true if the left hand side is less than or equal to the right hand side, false
    /// otherwise
    inline bool operator<=(const radian_t& rhs) const
    {
        return val <= rhs.val;
    }

    /// Greater than or equal to operator
    /// @param rhs right hand side of the greater than or equal to operator
    /// @return true if the left hand side is greater than or equal to the right hand side, false
    /// otherwise
    inline bool operator>=(const radian_t& rhs) const
    {
        return val >= rhs.val;
    }

    /// Unary minus operator
    /// @return the negated angle
    inline radian_t operator-() const
    {
        return radian_t{-val};
    }

    /// Addition operator
    /// @param rhs right hand side of the addition operator
    /// @return the sum of the two angles
    inline radian_t operator+(const radian_t& rhs) const
    {
        return radian_t{val + rhs.val};
    }

    /// Subtraction operator
    /// @param rhs right hand side of the subtraction operator
    /// @return the difference of the two angles
    inline radian_t operator-(const radian_t& rhs) const
    {
        return radian_t{val - rhs.val};
    }

    /// Addition assignment operator
    /// @param rhs right hand side of the addition assignment operator
    /// @return the result of the addition assignment
    inline radian_t& operator+=(const radian_t& rhs)
    {
        val += rhs.val;
        return *this;
    }

    /// Subtraction assignment operator
    /// @param rhs right hand side of the subtraction assignment operator
    /// @return the result of the subtraction assignment
    inline radian_t& operator-=(const radian_t& rhs)
    {
        val -= rhs.val;
        return *this;
    }

    /// Normalizes to between [-M_PI, +M_PI)
    /// @return the normalized angle
    inline radian_t& normalize() // NOLINT(readability-identifier-naming)
    {
        val = std::remainder(val + M_PI, 2 * M_PI);
        val = (val < 0.0) ? val + M_PI : val - M_PI;
        return *this;
    }

    /// Converts a string to a radian
    /// @param input string to convert
    /// @return the converted angle or std::nullopt if the string is not a valid angle
    static std::optional<radian_t> from_str( // NOLINT(readability-identifier-naming)
        const std::string& input);
};

degree_t operator*(const degree_t& lhs, double rhs);
degree_t operator*(double lhs, const degree_t& rhs);
degree_t operator/(const degree_t& lhs, double rhs);
radian_t operator*(const radian_t& lhs, double rhs);
radian_t operator*(double lhs, const radian_t& rhs);
radian_t operator/(const radian_t& lhs, double rhs);

degree_t operator""_degree(long double val);
degree_t operator""_degree(unsigned long long val);
radian_t operator""_radian(long double val);
radian_t operator""_radian(unsigned long long val);

std::ostream& operator<<(std::ostream& os, const degree_t& rhs);
std::ostream& operator<<(std::ostream& os, const radian_t& rhs);
std::istream& operator>>(std::istream& is, degree_t& rhs);
std::istream& operator>>(std::istream& is, radian_t& rhs);

const auto ZERO_RADIAN = 0_radian;      ///< Zero angle in radian
const auto PI_RADIANS = radian_t{M_PI}; ///< Pi angle in radian

/// @cond Doxygen warning about the macro internal
ATTRIBUTE_HELPER_HEADER(degree_t); ///< Attribute for degree_t
ATTRIBUTE_HELPER_HEADER(radian_t); ///< Attribute for radian_t
/// @endcond
} // namespace ns3

// clang-format on

#endif // UNITS_ANGLE_H
