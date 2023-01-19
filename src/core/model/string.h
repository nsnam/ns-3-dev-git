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

#ifndef NS3_STRING_H
#define NS3_STRING_H

#include "attribute-helper.h"

#include <string>
#include <vector>

/**
 * \file
 * \ingroup attribute_String
 * ns3::StringValue attribute value declarations.
 */

namespace ns3
{

/** Return type of SplitString. */
using StringVector = std::vector<std::string>;

/**
 * Split a string on a delimiter.
 * The input string is ummodified.
 * \param [in] str The string.
 * \param [in] delim The delimiter.
 * \returns A vector of the components of \p str which were separated
 * by \p delim.
 */
StringVector SplitString(const std::string& str, const std::string& delim);

//  Additional docs for class StringValue:
/**
 * Hold variables of type string
 *
 * This class can be used to hold variables of type string,
 * that is, either char * or std::string.
 */
ATTRIBUTE_VALUE_DEFINE_WITH_NAME(std::string, String);
ATTRIBUTE_ACCESSOR_DEFINE(String);
ATTRIBUTE_CHECKER_DEFINE(String);

} // namespace ns3

#endif /* NS3_STRING_H */
