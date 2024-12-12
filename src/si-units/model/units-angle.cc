#include "units-angle.h"

#include "string-utils.h"

using namespace ns3;

// clang-format off

namespace ns3
{

degree_t
degree_t::from_radian(const radian_t& input)
{
    return degree_t{input.val * 180.0 / M_PI};
}

radian_t
degree_t::to_radian() const
{
    return radian_t{val / 180.0 * M_PI};
}

double
degree_t::in_radian() const
{
    return to_radian().val;
}

double
degree_t::in_degree() const
{
    return val;
}

/// Multiplication by a unitless number
/// @param lhs Left hand side
/// @param rhs Right hand side
/// @return Product
degree_t
operator*(const degree_t& lhs, double rhs)
{
    return degree_t{lhs.val * rhs};
}

/// Multiplication by a unitless number
/// @param lhs Left hand side
/// @param rhs Right hand side
/// @return Product
degree_t
operator*(double lhs, const degree_t& rhs)
{
    return rhs * lhs;
}

/// Division by a unitless number
/// @param lhs Left hand side
/// @param rhs Right hand side
/// @return Division
degree_t
operator/(const degree_t& lhs, double rhs)
{
    return degree_t{lhs.val / rhs};
}

radian_t
radian_t::from_degree(const degree_t& input)
{
    return radian_t{input.val / 180.0 * M_PI};
}

degree_t
radian_t::to_degree() const
{
    return degree_t{val * 180.0 / M_PI};
}

double
radian_t::in_degree() const
{
    return to_degree().val;
}

double
radian_t::in_radian() const
{
    return val;
}

/// Multiplication by a unitless number
/// @param lhs Left hand side
/// @param rhs Right hand side
/// @return Product
radian_t
operator*(const radian_t& lhs, double rhs)
{
    return radian_t{lhs.val * rhs};
}

/// Multiplication by a unitless number
/// @param lhs Left hand side
/// @param rhs Right hand side
/// @return Product
radian_t
operator*(double lhs, const radian_t& rhs)
{
    return rhs * lhs;
}

/// Division by a unitless number
/// @param lhs Left hand side
/// @param rhs Right hand side
/// @return Product
radian_t
operator/(const radian_t& lhs, double rhs)
{
    return radian_t{lhs.val / rhs};
}

// User defined literals

/// Degree literal
/// @param val Value
/// @return Degree value
degree_t
operator""_degree(long double val)
{
    return degree_t{static_cast<double>(val)};
}

/// Degree literal
/// @param val Value
/// @return Degree value
degree_t
operator""_degree(unsigned long long val)
{
    return degree_t{static_cast<double>(val)};
}

/// Radian literal
/// @param val Value
/// @return Radian value
radian_t
operator""_radian(long double val)
{
    return radian_t{static_cast<double>(val)};
}

/// Radian literal
/// @param val Value
/// @return Radian value
radian_t
operator""_radian(unsigned long long val)
{
    return radian_t{static_cast<double>(val)};
}

// Output-input operator overloading
/// Output operator
/// @param os Output stream
/// @param rhs degree value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const degree_t& rhs)
{
    return os << rhs.str();
}

/// Output operator
/// @param os Output stream
/// @param rhs radian value
/// @return Output stream
std::ostream&
operator<<(std::ostream& os, const radian_t& rhs)
{
    return os << rhs.str();
}

/// Input operator
/// @param is Input stream
/// @param rhs degree value
/// @return Input stream
std::istream&
operator>>(std::istream& is, degree_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// Input operator
/// @param is Input stream
/// @param rhs radian value
/// @return Input stream
std::istream&
operator>>(std::istream& is, radian_t& rhs)
{
    is >> rhs.val;
    return is;
}

/// @cond Doxygen error: multiple @param
/// Convert string to degree value
/// @param input String to convert
/// @return optional Degree value if convertible, nullopt otherwise
std::optional<degree_t>
degree_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "degree");
    return res.has_value() ? std::optional(degree_t{res.value()}) : std::nullopt;
}

/// Convert string to radian value
/// @param input String to convert
/// @return optional Radian value if convertible, nullopt otherwise
std::optional<radian_t>
radian_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "radian");
    return res.has_value() ? std::optional(radian_t{res.value()}) : std::nullopt;
}

ATTRIBUTE_HELPER_CPP(degree_t); ///< Attribute for degree_t
ATTRIBUTE_HELPER_CPP(radian_t); ///< Attribute for radian_t
/// @endcond

} // namespace ns3

// clang-format on
