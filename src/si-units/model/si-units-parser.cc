#include "si-units-parser.h"

#include <algorithm>

// clang-format off

namespace ns3
{

/// @cond Doxygen warning about the missindg doc despite of ones in the header file
std::string
TrimL(const std::string& str)
{
    auto s = str;
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
    return s;
}

std::string
TrimR(const std::string& str)
{
    auto s = str;
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); })
                .base(),
            s.end());
    return s;
}

std::string
Trim(const std::string& str)
{
    return TrimL(TrimR(str));
}

bool
IsNumber(const std::string& str)
{
    // Following alternatives are not useful enough
    // - std::sscanf : fails to filter out many invalid forms. Inability to raise an exception
    // - std::from_chars : Lack of support of floating point in Apple clang
    // - std::stold : fails to filter out many invalid forms.
    // - std::istream : fails to filter out many invalid forms.
    // - class NSFormatter : too much extra dependency

    auto pos = str.find_first_not_of("0123456789.-");
    if (pos != std::string::npos)
    {
        return false;
    }
    pos = str.rfind('-');
    if ((pos != std::string::npos) && (pos > 0))
    {
        return false;
    }
    if (std::count_if(str.begin(), str.end(), [](char c) { return c == '.'; }) > 1)
    {
        return false;
    }
    return true;
}

std::optional<double>
StringToDouble(std::string str, std::string suffix)
{
    str = Trim(str);
    if (not str.ends_with(suffix))
    {
        return std::nullopt;
    }
    str.resize(str.rfind(suffix));
    str = Trim(str);
    if (not IsNumber(str))
    {
        return std::nullopt;
    }

    try
    {
        auto val = std::stold(str);
        return static_cast<double>(val);
    }
    catch (const std::invalid_argument& ia)
    { // See https://cplusplus.com/reference/string/stold/
      // for defined behaviors
      // graceful propagation of conversion failure
    }
    return std::nullopt;
}

/// @endcond

} // namespace ns3

// clang-format on
