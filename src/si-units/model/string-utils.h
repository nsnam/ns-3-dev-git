#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include "format-string.h"

#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace ns3
{

/// Trim white spaces off the string from left
/// @param str string
/// @return trimmed string
std::string TrimL(const std::string& str);

/// Trim white spaces off the string from right
/// @param str string
/// @return trimmed string
std::string TrimR(const std::string& str);

/// Trim white spaces off the string left and right
/// @param str string
/// @return trimmed string
std::string Trim(const std::string& str);

/// Convert a string to double, when the suffix matches
/// @param str input string
/// @param suffix suffix to match
/// @return optional double if parsed. std::nullopt otherwise
std::optional<double> StringToDouble(std::string str, std::string suffix = "");

// TODO(porce): Support binary, octal, hexadecimal notations
/// @cond Doxygen error: multiple @param
/// Test if a string is a representation of a number, sans surrounding spaces
/// No scientific notation is supported
/// @param s string to test
/// @return true if number, false otherwise
bool IsNumber(const std::string& s);

/// @endcond

/// Get parse error message
/// @param str string under parsing
/// @param suffix suffix under parsing
/// @return error message
inline std::string
GetParseErrMsg(const std::string& str, const std::string& suffix)
{
    return sformat("Parse error: %-16s Example notation: <number>%s", str.c_str(), suffix.c_str());
}

/// Parse input string stream.
/// @tparam data type of the outcome
/// @param is input string stream
/// @param outcome output value
/// @param suffix suffix to match
/// @return input string stream with error state flags
template <typename T>
std::istream&
ParseSIString(std::istream& is, T& outcome, const std::string& suffix)
{
    std::string str;
    std::getline(is, str);
    auto parsed = T::from_str(str);
    if (!parsed.has_value())
    {
        is.setstate(std::ios::failbit);
        std::cerr << GetParseErrMsg(str, suffix) << std::endl;
        return is;
    }
    outcome = parsed.value();
    return is;
}

} // namespace ns3

#endif // STRING_UTILS_H
