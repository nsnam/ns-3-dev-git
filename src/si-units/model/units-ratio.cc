#include "units-ratio.h"

#include "string-utils.h"
#include "units-aliases.h"

using namespace ns3;

// clang-format off
namespace ns3
{

percent_t::percent_t(const std::string& str)
{
    auto res = from_str(str);
    NS_ABORT_MSG_IF(!res.has_value(), GetParseErrMsg(str, "percent"));
    val = res.value().val;
}

/// Multiply operation
/// @param lfs a unitless value
/// @param rhs percent
/// @return multiplied percent
percent_t
operator*(const double& lfs, const percent_t& rhs)
{
    return percent_t{lfs * rhs.val};
}

/// User defined literals for percent
/// @param val percent value
/// @return percent_t object
percent_t
operator""_percent(long double val)
{
    return percent_t{static_cast<double>(val)};
}

/// User defined literals for percent
/// @param val percent value
/// @return percent_t object
percent_t
operator""_percent(unsigned long long val)
{
    return percent_t{static_cast<double>(val)};
}

/// Output stream operator for percent
/// @param os output stream
/// @param rhs percent
/// @return output stream
std::ostream&
operator<<(std::ostream& os, const percent_t& rhs)
{
    return os << rhs.str();
}

/// Input stream operator for percent
/// @param is input stream
/// @param rhs percent
/// @return input stream
std::istream&
operator>>(std::istream& is, percent_t& rhs)
{
    return ParseSIString(is, rhs, "percent");
}

/// @cond Doxygen bug: multiple @param
/// Convert string to percent
/// @param input string
/// @return percent_t object or nullopt if conversion failed
/// @endcond
std::optional<percent_t>
percent_t::from_str(const std::string& input)
{
    auto res = StringToDouble(input, "percent");
    if (res.has_value())
    {
        return std::optional(percent_t{res.value()});
    }
    res = StringToDouble(input, "%");
    if (res.has_value())
    {
        return std::optional(percent_t{res.value()});
    }
    return std::nullopt;
}

/// @cond Doxygen warning against macro internals
ATTRIBUTE_CHECKER_IMPLEMENT_WITH_CONVERTER(percent_t, percent);
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(percent_t, percent);
/// @endcond

} // namespace ns3
