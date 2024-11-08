/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "boolean.h"

#include "fatal-error.h"
#include "log.h"

/**
 * @file
 * @ingroup attribute_Boolean
 * ns3::BooleanValue attribute value implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Boolean");

BooleanValue::BooleanValue()
    : m_value(false)
{
    NS_LOG_FUNCTION(this);
}

BooleanValue::BooleanValue(const bool& value)
    : m_value(value)
{
    NS_LOG_FUNCTION(this << value);
}

void
BooleanValue::Set(bool value)
{
    NS_LOG_FUNCTION(this << value);
    m_value = value;
}

bool
BooleanValue::Get() const
{
    NS_LOG_FUNCTION(this);
    return m_value;
}

BooleanValue::operator bool() const
{
    return m_value;
}

std::ostream&
operator<<(std::ostream& os, const BooleanValue& value)
{
    if (value.Get())
    {
        os << "true";
    }
    else
    {
        os << "false";
    }
    return os;
}

Ptr<AttributeValue>
BooleanValue::Copy() const
{
    NS_LOG_FUNCTION(this);

    return Create<BooleanValue>(*this);
}

std::string
BooleanValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    NS_LOG_FUNCTION(this << checker);

    if (m_value)
    {
        return "true";
    }
    else
    {
        return "false";
    }
}

bool
BooleanValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    NS_LOG_FUNCTION(this << value << checker);

    if (value == "true" || value == "1" || value == "t")
    {
        m_value = true;
        return true;
    }
    else if (value == "false" || value == "0" || value == "f")
    {
        m_value = false;
        return true;
    }
    else
    {
        return false;
    }
}

ATTRIBUTE_CHECKER_IMPLEMENT_WITH_NAME(Boolean, "bool");

} // namespace ns3
