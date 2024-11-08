/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef UINTEGER_H
#define UINTEGER_H

#include "attribute-helper.h"
#include "attribute.h"

#include <limits>
#include <stdint.h>

/**
 * @file
 * @ingroup attribute_Uinteger
 * ns3::UintegerValue attribute value declarations and template implementations.
 */

namespace ns3
{

//  Additional docs for class UintegerValue:
/**
 * Hold an unsigned integer type.
 *
 * This class can be used to hold variables of unsigned integer
 * type such as uint8_t, uint16_t, uint32_t, uint64_t, or,
 * unsigned int, etc.
 */
ATTRIBUTE_VALUE_DEFINE_WITH_NAME(uint64_t, Uinteger);
ATTRIBUTE_ACCESSOR_DEFINE(Uinteger);

template <typename T>
Ptr<const AttributeChecker> MakeUintegerChecker();

/**
 * Make a checker with a minimum value.
 *
 * The minimum value is included in the allowed range.
 *
 * @param [in] min The minimum value.
 * @returns The AttributeChecker.
 * @see AttributeChecker
 */
template <typename T>
Ptr<const AttributeChecker> MakeUintegerChecker(uint64_t min);

/**
 * Make a checker with a minimum and a maximum value.
 *
 * The minimum and maximum values are included in the allowed range.
 *
 * @param [in] min The minimum value.
 * @param [in] max The maximum value.
 * @returns The AttributeChecker.
 * @see AttributeChecker
 */
template <typename T>
Ptr<const AttributeChecker> MakeUintegerChecker(uint64_t min, uint64_t max);

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

#include "type-name.h"

namespace ns3
{

namespace internal
{

Ptr<const AttributeChecker> MakeUintegerChecker(uint64_t min, uint64_t max, std::string name);

} // namespace internal

template <typename T>
Ptr<const AttributeChecker>
MakeUintegerChecker()
{
    return internal::MakeUintegerChecker(std::numeric_limits<T>::min(),
                                         std::numeric_limits<T>::max(),
                                         TypeNameGet<T>());
}

template <typename T>
Ptr<const AttributeChecker>
MakeUintegerChecker(uint64_t min)
{
    return internal::MakeUintegerChecker(min, std::numeric_limits<T>::max(), TypeNameGet<T>());
}

template <typename T>
Ptr<const AttributeChecker>
MakeUintegerChecker(uint64_t min, uint64_t max)
{
    return internal::MakeUintegerChecker(min, max, TypeNameGet<T>());
}

} // namespace ns3

#endif /* UINTEGER_H */
