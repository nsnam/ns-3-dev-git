#include "units-frequency.h"

#include <iostream>

using namespace ns3;

// clang-format off
namespace ns3
{
std::string
Hz_t::str(bool space) const // NOLINT(readability-identifier-naming)
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

    return sformat(space ? "%lld %sHz" : "%lld%sHz", valInt, WHOLE_UNIT_PREFIX[idx].c_str());
}

Hz_t::Hz_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "Hz"));
    val = res.value().val;
}

kHz_t::kHz_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "kHz"));
    val = res.value().val;
}

MHz_t::MHz_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "MHz"));
    val = res.value().val;
}

GHz_t::GHz_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "GHz"));
    val = res.value().val;
}

THz_t::THz_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "THz"));
    val = res.value().val;
}

bool
Hz_t::IsMultipleOf(const Hz_t& rhs) const
{
    NS_ASSERT(rhs.val != 0.);
    auto div = static_cast<const int64_t>(val / rhs.val);
    return val == (div * rhs.val);
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
kHz_t
operator""_kHz(unsigned long long val)
{
    return kHz_t{static_cast<double>(val)};
}

/// User-defined literals for kHz
/// @param val The value in kHz
/// @return Hz_t object
kHz_t
operator""_kHz(long double val)
{
    return kHz_t{static_cast<double>(val)};
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
GHz_t
operator""_GHz(unsigned long long val)
{
    return GHz_t{static_cast<double>(val)};
}

/// User-defined literals for GHz
/// @param val The value in GHz
/// @return Hz_t object
GHz_t
operator""_GHz(long double val)
{
    return GHz_t{static_cast<double>(val)};
}

/// User-defined literals for THz
/// @param val The value in THz
/// @return Hz_t object
THz_t
operator""_THz(unsigned long long val)
{
    return THz_t{static_cast<double>(val)};
}

/// User-defined literals for THz
/// @param val The value in THz
/// @return Hz_t object
THz_t
operator""_THz(long double val)
{
    return THz_t{static_cast<double>(val)};
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
/// @param lhs The double to multiply by
/// @param rhs The Hz_t object to multiply
/// @return The Hz_t object
Hz_t
operator*(double lhs, const Hz_t& rhs)
{
    return rhs * lhs;
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

Hz_t
Hz_t::to_Hz() const
{
    return *this;
}

kHz_t
Hz_t::to_kHz() const
{
    return kHz_t{val / ONE_KILO};
}

MHz_t
Hz_t::to_MHz() const
{
    return MHz_t{val / ONE_MEGA};
}

GHz_t
Hz_t::to_GHz() const
{
    return GHz_t{val / ONE_GIGA};
}

THz_t
Hz_t::to_THz() const
{
    return THz_t{val / ONE_TERA};
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
        return MHz_t{res.value()};
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

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(kHz_t, kHz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(kHz_t, kHz);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(MHz_t, MHz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(MHz_t, MHz);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(GHz_t, GHz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(GHz_t, GHz);

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(THz_t, THz);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(THz_t, THz);
/// @endcond

} // namespace ns3
