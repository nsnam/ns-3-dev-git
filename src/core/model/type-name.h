/*
 * Copyright (c) 2007 Georgia Tech Research Corporation
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

#ifndef TYPE_NAME_H
#define TYPE_NAME_H

#include "fatal-error.h"

#include <string>

/**
 * \file
 * \ingroup attributeimpl
 * ns3::TypeNameGet() function declarations.
 */

namespace ns3
{

/**
 * \ingroup attributeimpl
 *
 * Type name strings for AttributeValue types.
 * Custom classes should add a template specialization of this function
 * using the macro \c TYPE_NAME_GET_DEFINE(T).
 *
 * \tparam T \explicit The type.
 * \returns The type name as a string.
 */
template <typename T>
std::string
TypeNameGet()
{
    NS_FATAL_ERROR("Type name not defined.");
    return "unknown";
}

/**
 * \ingroup attributeimpl
 *
 * Macro that defines a template specialization for \c TypeNameGet<T>() .
 *
 * \param T The type.
 */
#define TYPENAMEGET_DEFINE(T)                                                                      \
    template <>                                                                                    \
    inline std::string TypeNameGet<T>()                                                            \
    {                                                                                              \
        return #T;                                                                                 \
    }

/**
 * \ingroup attributeimpl
 * ns3::TypeNameGet() specialization for numeric types.
 * \returns The numeric type name as a string.
 * @{
 */
TYPENAMEGET_DEFINE(bool);
TYPENAMEGET_DEFINE(int8_t);
TYPENAMEGET_DEFINE(int16_t);
TYPENAMEGET_DEFINE(int32_t);
TYPENAMEGET_DEFINE(int64_t);
TYPENAMEGET_DEFINE(uint8_t);
TYPENAMEGET_DEFINE(uint16_t);
TYPENAMEGET_DEFINE(uint32_t);
TYPENAMEGET_DEFINE(uint64_t);
TYPENAMEGET_DEFINE(float);
TYPENAMEGET_DEFINE(double);
TYPENAMEGET_DEFINE(long double);
/** @} */

} // namespace ns3

#endif /* TYPE_NAME_H */
