/*
 * Copyright (c) 2007 INRIA, Mathieu Lacage
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@gmail.com>
 */
#include "object-ptr-container.h"

#include "log.h"

/**
 * @file
 * @ingroup attribute_ObjectPtrContainer
 * ns3::ObjectPtrContainerValue attribute value implementations.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("ObjectPtrContainer");

ObjectPtrContainerValue::ObjectPtrContainerValue()
{
    NS_LOG_FUNCTION(this);
}

ObjectPtrContainerValue::Iterator
ObjectPtrContainerValue::Begin() const
{
    NS_LOG_FUNCTION(this);
    return m_objects.begin();
}

ObjectPtrContainerValue::Iterator
ObjectPtrContainerValue::End() const
{
    NS_LOG_FUNCTION(this);
    return m_objects.end();
}

std::size_t
ObjectPtrContainerValue::GetN() const
{
    NS_LOG_FUNCTION(this);
    return m_objects.size();
}

Ptr<Object>
ObjectPtrContainerValue::Get(std::size_t i) const
{
    NS_LOG_FUNCTION(this << i);
    auto it = m_objects.find(i);
    Ptr<Object> value = nullptr;
    if (it != m_objects.end())
    {
        value = m_objects.find(i)->second;
    }
    return value;
}

Ptr<AttributeValue>
ObjectPtrContainerValue::Copy() const
{
    NS_LOG_FUNCTION(this);
    return ns3::Create<ObjectPtrContainerValue>(*this);
}

std::string
ObjectPtrContainerValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    NS_LOG_FUNCTION(this << checker);
    std::ostringstream oss;
    Iterator it;
    for (it = Begin(); it != End(); ++it)
    {
        oss << (*it).second;
        if (it != End())
        {
            oss << " ";
        }
    }
    return oss.str();
}

bool
ObjectPtrContainerValue::DeserializeFromString(std::string value,
                                               Ptr<const AttributeChecker> checker)
{
    NS_LOG_FUNCTION(this << value << checker);
    NS_FATAL_ERROR("cannot deserialize a set of object pointers.");
    return true;
}

bool
ObjectPtrContainerAccessor::Set(ObjectBase* object, const AttributeValue& value) const
{
    // not allowed.
    NS_LOG_FUNCTION(this << object << &value);
    return false;
}

bool
ObjectPtrContainerAccessor::Get(const ObjectBase* object, AttributeValue& value) const
{
    NS_LOG_FUNCTION(this << object << &value);
    auto v = dynamic_cast<ObjectPtrContainerValue*>(&value);
    if (v == nullptr)
    {
        return false;
    }
    v->m_objects.clear();
    std::size_t n;
    bool ok = DoGetN(object, &n);
    if (!ok)
    {
        return false;
    }
    for (std::size_t i = 0; i < n; i++)
    {
        std::size_t index;
        Ptr<Object> o = DoGet(object, i, &index);
        v->m_objects[index] = o;
    }
    return true;
}

bool
ObjectPtrContainerAccessor::HasGetter() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

bool
ObjectPtrContainerAccessor::HasSetter() const
{
    NS_LOG_FUNCTION(this);
    return false;
}

} // namespace ns3
