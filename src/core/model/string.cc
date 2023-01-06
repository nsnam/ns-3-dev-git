/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "string.h"

/**
 * \file
 * \ingroup attribute_String
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
