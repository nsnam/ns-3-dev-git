/*
 * Copyright (c) 2005,2006 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "callback.h"

#include "log.h"

/**
 * \file
 * \ingroup callback
 * ns3::CallbackValue implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Callback");

CallbackValue::CallbackValue()
    : m_value()
{
    NS_LOG_FUNCTION(this);
}

CallbackValue::CallbackValue(const CallbackBase& value)
    : m_value(value)
{
}

CallbackValue::~CallbackValue()
{
    NS_LOG_FUNCTION(this);
}

void
CallbackValue::Set(const CallbackBase& value)
{
    NS_LOG_FUNCTION(&value);
    m_value = value;
}

CallbackBase
CallbackValue::Get()
{
    NS_LOG_FUNCTION(this);
    return m_value;
}

Ptr<AttributeValue>
CallbackValue::Copy() const
{
    NS_LOG_FUNCTION(this);
    return Create<CallbackValue>(m_value);
}

std::string
CallbackValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    NS_LOG_FUNCTION(this << checker);
    std::ostringstream oss;
    oss << PeekPointer(m_value.GetImpl());
    return oss.str();
}

bool
CallbackValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    NS_LOG_FUNCTION(this << value << checker);
    return false;
}

ATTRIBUTE_CHECKER_IMPLEMENT(Callback);

} // namespace ns3
