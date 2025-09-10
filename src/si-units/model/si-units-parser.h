#ifndef SI_UNITS_PARSER_H
#define SI_UNITS_PARSER_H

#include <iostream>
#include <memory>
#include <optional>
#include <string>

// clang-format off

namespace ns3
{

// TODO(porce): Use std::format() instead
// https://stackoverflow.com/a/26221725
// License: CC0 1.0
/// A string formatter returning std::string
/// @tparam Args The types of the arguments to format
/// @param format_str The format string
/// @param args The arguments to format
/// @return The formatted string in std::string
template <typename... Args>
std::string
sformat(const std::string& format_str, Args... args)
{
    int size = snprintf(nullptr, 0, format_str.c_str(), args...) + 1; // Extra space for '\0'
    if (size <= 0)
    {
        throw std::runtime_error("Error during format string.");
    }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format_str.c_str(), args...);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

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
/// @note Valid suffix is expected. This code does not handle when str is "123kHz" and suffix is "Hz".
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

// clang-format on

#endif // SI_UNITS_PARSER_H
