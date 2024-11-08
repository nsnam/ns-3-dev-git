/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "pointer.h"

#include "log.h"
#include "object-factory.h"

#include <sstream>

/**
 * @file
 * @ingroup attribute_Pointer
 * ns3::PointerValue attribute value implementations.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Pointer");

PointerValue::PointerValue()
    : m_value()
{
    NS_LOG_FUNCTION(this);
}

PointerValue::PointerValue(const Ptr<Object>& object)
    : m_value(object)
{
    NS_LOG_FUNCTION(object);
}

void
PointerValue::SetObject(Ptr<Object> object)
{
    NS_LOG_FUNCTION(object);
    m_value = object;
}

Ptr<Object>
PointerValue::GetObject() const
{
    NS_LOG_FUNCTION(this);
    return m_value;
}

Ptr<AttributeValue>
PointerValue::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<PointerValue>(*this);
}

std::string
PointerValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    NS_LOG_FUNCTION(this << checker);
    std::ostringstream oss;
    oss << m_value;
    return oss.str();
}

bool
PointerValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    // We assume that the string you want to deserialize contains
    // a description for an ObjectFactory to create an object and then assign it to the
    // member variable.
    NS_LOG_FUNCTION(this << value << checker);
    ObjectFactory factory;
    std::istringstream iss;
    iss.str(value);
    iss >> factory;
    if (iss.fail())
    {
        return false;
    }
    m_value = factory.Create<Object>();
    return true;
}

} // namespace ns3
