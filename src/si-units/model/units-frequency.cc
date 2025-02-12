#include "units-frequency.h"

#include "string-utils.h"

#include <iostream>

using namespace ns3;

// clang-format off
namespace ns3
{

Hz_t::Hz_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "Hz"));
    val = res.value().val;
}

MHz_t::MHz_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "MHz"));
    val = res.value().val;
}

/// User-defined literals for Hz
/// @param val The value in Hz
/// @return Hz_t object
Hz_t
operator""_Hz(unsigned long long val)
{
    return Hz_t{static_cast<double>(val)};
}

/// User-defined literals for Hz
/// @param val The value in Hz
/// @return Hz_t object
Hz_t
operator""_Hz(long double val)
{
    return Hz_t{static_cast<double>(val)};
}

/// User-defined literals for kHz
/// @param val The value in kHz
/// @return Hz_t object
Hz_t
operator""_kHz(unsigned long long val)
{
    return Hz_t{static_cast<double>(val * ONE_KILO)};
}

/// User-defined literals for kHz
/// @param val The value in kHz
/// @return Hz_t object
Hz_t
operator""_kHz(long double val)
{
    return Hz_t{static_cast<double>(val * ONE_KILO)};
}

/// User-defined literals for MHz
/// @param val The value in MHz
/// @return Hz_t object
MHz_t
operator""_MHz(unsigned long long val)
{
    return MHz_t{static_cast<double>(val)};
}

/// User-defined literals for MHz
/// @param val The value in MHz
/// @return Hz_t object
MHz_t
operator""_MHz(long double val)
{
    return MHz_t{static_cast<double>(val)};
}

/// User-defined literals for GHz
/// @param val The value in GHz
/// @return Hz_t object
Hz_t
operator""_GHz(unsigned long long val)
{
    return Hz_t{static_cast<double>(val * ONE_GIGA)};
}

/// User-defined literals for GHz
/// @param val The value in GHz
/// @return Hz_t object
Hz_t
operator""_GHz(long double val)
{
    return Hz_t{static_cast<double>(val * ONE_GIGA)};
}

/// User-defined literals for THz
/// @param val The value in THz
/// @return Hz_t object
Hz_t
operator""_THz(unsigned long long val)
{
    return Hz_t{static_cast<double>(val * ONE_TERA)};
}

/// User-defined literals for THz
/// @param val The value in THz
/// @return Hz_t object
Hz_t
operator""_THz(long double val)
{
    return Hz_t{static_cast<double>(val * ONE_TERA)};
}

/// Output stream for Hz_t
/// @param os The output stream
/// @param rhs The Hz_t object to output
/// @return The output stream
std::ostream&
operator<<(std::ostream& os, const Hz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for Hz_t
/// @param is The input stream
/// @param rhs The Hz_t object to input
/// @return The input stream
std::istream&
operator>>(std::istream& is, Hz_t& rhs)
{
    return ParseSIString(is, rhs, "Hz");
}

/// Output stream for MHz_t
/// @param os The output stream
/// @param rhs The MHz_t object to output
/// @return The output stream
std::ostream&
operator<<(std::ostream& os, const MHz_t& rhs)
{
    return os << rhs.str();
}

/// Input stream for MHz_t
/// @param is The input stream
/// @param rhs The MHz_t object to input
/// @return The input stream
std::istream&
operator>>(std::istream& is, MHz_t& rhs)
{
    return ParseSIString(is, rhs, "MHz");
}

/// Multiply Hz_t by double
/// @param lfs The double to multiply by
/// @param rhs The Hz_t object to multiply
/// @return The Hz_t object
Hz_t
operator*(double lfs, const Hz_t& rhs)
{
    return rhs * lfs;
}

/// Multiply Hz_t by Time
/// @param nstime The Time to multiply by
/// @param rhs The Hz_t object to multiply
/// @return unitless value
double
operator*(Time nstime, const Hz_t& rhs)
{
    return rhs * nstime;
}

/// Multiply MHz_t by double
/// @param lfs The double to multiply by
/// @param rhs The MHz_t object to multiply
/// @return The MHz_t object
MHz_t
operator*(double lfs, const MHz_t& rhs)
{
    return rhs * lfs;
}

/// Multiply MHz_t by Time
/// @param nstime The Time to multiply by
/// @param rhs The MHz_t object to multiply
/// @return unitless value
double
operator*(Time nstime, const MHz_t& rhs)
{
    return rhs * nstime;
}

/// Addition operator
/// @param lhs the left value
/// @param rhs the right value
/// @return Sum of the two values
Hz_t
operator+(const Hz_t& lhs, const MHz_t& rhs)
{
    return lhs + rhs.to_Hz();
}

/// Addition operator
/// @param lhs the left value
/// @param rhs the right value
/// @return Sum of the two values
MHz_t
operator+(const MHz_t& lhs, const Hz_t& rhs)
{
    return lhs + rhs.to_MHz();
}

/// Subtraction operator
/// @param lhs the left value
/// @param rhs the right value
/// @return Difference of the two values
Hz_t
operator-(const Hz_t& lhs, const MHz_t& rhs)
{
    return lhs - rhs.to_Hz();
}

/// Subtraction operator
/// @param lhs the left value
/// @param rhs the right value
/// @return Difference of the two values
MHz_t
operator-(const MHz_t& lhs, const Hz_t& rhs)
{
    return lhs - rhs.to_MHz();
}

/// Division operator
/// @param lhs the left value
/// @param rhs the right value
/// @return Quotient of the two values
double
operator/(const Hz_t& lhs, const MHz_t& rhs)
{
    return lhs / rhs.to_Hz();
}

/// Division operator
/// @param lhs the left value
/// @param rhs the right value
/// @return Quotient of the two values
double
operator/(const MHz_t& lhs, const Hz_t& rhs)
{
    return lhs.to_Hz() / rhs;
}

/// Conversion to MHz_t object
/// @return the object converted in MHz_t
MHz_t
Hz_t::to_MHz() const
{
    return MHz_t{val / ONE_MEGA};
}

/// @cond Doxygen bug: multiple @param
/// Converts a string to a Hz_t object
/// @param input The string to convert
/// @return The Hz_t object if successful, nullopt otherwise
/// @endcond
std::optional<Hz_t>
Hz_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "THz");
    if (res.has_value())
    {
        return THz_t(res.value());
    }
    res = StringToDouble(input, "GHz");
    if (res.has_value())
    {
        return GHz_t(res.value());
    }
    res = StringToDouble(input, "MHz");
    if (res.has_value())
    {
        return MHz_t{res.value()}.to_Hz();
    }
    res = StringToDouble(input, "kHz");
    if (res.has_value())
    {
        return kHz_t(res.value());
    }
    res = StringToDouble(input, "Hz");
    return res.has_value() ? std::optional(Hz_t{res.value()}) : std::nullopt;
}

/// @cond Doxygen warning against macro internasl
ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(Hz_t, Hz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(Hz_t, Hz);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(MHz_t, MHz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(MHz_t, MHz);
/// @endcond

} // namespace ns3
