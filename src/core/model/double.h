/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#ifndef NS_DOUBLE_H
#define NS_DOUBLE_H

#include "attribute-helper.h"
#include "attribute.h"

#include <limits>
#include <stdint.h>

/**
 * @file
 * @ingroup attribute_Double
 * ns3::DoubleValue attribute value declarations and template implementations.
 */

namespace ns3
{

//  Additional docs for class DoubleValue:
/**
 * This class can be used to hold variables of floating point type
 * such as 'double' or 'float'. The internal format is 'double'.
 */
ATTRIBUTE_VALUE_DEFINE_WITH_NAME(double, Double);
ATTRIBUTE_ACCESSOR_DEFINE(Double);

template <typename T>
Ptr<const AttributeChecker> MakeDoubleChecker();

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
Ptr<const AttributeChecker> MakeDoubleChecker(double min);

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
Ptr<const AttributeChecker> MakeDoubleChecker(double min, double max);

} // namespace ns3

/***************************************************************
 *  Implementation of the templates declared above.
 ***************************************************************/

#include "type-name.h"

namespace ns3
{

namespace internal
{

Ptr<const AttributeChecker> MakeDoubleChecker(double min, double max, std::string name);

} // namespace internal

template <typename T>
Ptr<const AttributeChecker>
MakeDoubleChecker()
{
    return internal::MakeDoubleChecker(-std::numeric_limits<T>::max(),
                                       std::numeric_limits<T>::max(),
                                       TypeNameGet<T>());
}

template <typename T>
Ptr<const AttributeChecker>
MakeDoubleChecker(double min)
{
    return internal::MakeDoubleChecker(min, std::numeric_limits<T>::max(), TypeNameGet<T>());
}

template <typename T>
Ptr<const AttributeChecker>
MakeDoubleChecker(double min, double max)
{
    return internal::MakeDoubleChecker(min, max, TypeNameGet<T>());
}

} // namespace ns3

#endif /* NS_DOUBLE_H */
