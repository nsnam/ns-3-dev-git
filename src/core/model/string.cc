/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 */

#include "string.h"

/**
 * @file
 * @ingroup attribute_String
 * ns3::StringValue attribute value implementation.
 */

namespace ns3
{

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_NAME(String, "std::string");
ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(std::string, String);

StringVector
SplitString(const std::string& str, const std::string& delim)
{
    std::vector<std::string> result;
    std::string token;
    std::size_t pos{0};
    do
    {
        std::size_t next = str.find(delim, pos);
        std::string token{str.substr(pos, next - pos)};
        result.push_back(token);
        if (next < str.size())
        {
            pos = next + delim.size();
        }
        else
        {
            pos = str.size() + 1;
        }
    } while (pos <= str.size());
    return result;
}

} // namespace ns3
