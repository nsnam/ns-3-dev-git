#ifndef FORMAT_STRING_H
#define FORMAT_STRING_H

// String formatter
// A stop gap for C++20 std::format and C++23 std::print
// TODO(porce): Deprecate by std::format when ready

#include <memory>
#include <stdexcept>
#include <string>

// clang-format off

namespace ns3
{

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

} // namespace ns3

// clang-format on

#endif // FORMAT_STRING_H
