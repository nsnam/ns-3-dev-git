#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <optional>
#include <string>
#include <vector>

// clang-format off
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

} // namespace ns3

// clang-format on

#endif // STRING_UTILS_H
