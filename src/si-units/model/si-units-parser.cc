#include "si-units-parser.h"

#include "units-aliases.h"

#include <algorithm>
#include <cmath>
#include <map>

namespace ns3
{

/// @cond Doxygen warning about the missindg doc despite of ones in the header file

/// Definition of metric prefixes for whole numbers (not fractional ones)
const std::map<int64_t, std::string> SI_UNITS_WHOLE_METRIC_PREFIXES = {
    {1e+00, ""},
    {1e+03, "k"},
    {1e+06, "M"},
    {1e+09, "G"},
    {1e+12, "T"},

    // Expand as needed
    // {1e+15, "P"}, {1e+18, "E"}, {1e+21, "Z"}, {1e+24, "Y"},
};

std::optional<double>
MetricStrToNum(std::string str)
{
    str = Trim(str);
    if (IsNumber(str))
    {
        auto val = std::stold(str);
        return static_cast<double>(val);
    }

    auto last = str.substr(str.size() - 1, 1);
    auto front = Trim(str.substr(0, str.size() - 1));
    if (not IsNumber(front))
    {
        return std::nullopt;
    }
    auto it = std::find_if(SI_UNITS_WHOLE_METRIC_PREFIXES.begin(),
                           SI_UNITS_WHOLE_METRIC_PREFIXES.end(),
                           [&last](const auto& elem) { return elem.second == last; });
    if (it == SI_UNITS_WHOLE_METRIC_PREFIXES.end())
    {
        return std::nullopt;
    }
    auto val = std::stold(str) * it->first;
    return static_cast<double>(val);
}

std::string
NumToMetricStr(const double val, bool space)
{
    if (not IsInteger(val))
    {
        return std::to_string(val) + (space ? " " : "");
    }
    for (const auto& elem : SI_UNITS_WHOLE_METRIC_PREFIXES)
    {
        auto scaled = static_cast<int64_t>(val / elem.first);
        if ((scaled % ONE_KILO) != 0)
        {
            return std::to_string(scaled) + (space ? " " : "") + elem.second;
        }
    }
    auto last = SI_UNITS_WHOLE_METRIC_PREFIXES.rbegin();
    auto scaled = static_cast<int64_t>(val / last->first);
    return std::to_string(scaled) + (space ? " " : "") + last->second;
}

bool
IsInteger(const double val)
{
    return std::trunc(val) == val;
}

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
    if (!suffix.empty() && !str.ends_with(suffix))
    {
        return std::nullopt;
    }

    if (!suffix.empty())
    {
        str.resize(str.rfind(suffix));
        str = Trim(str);
    }

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
