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
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "enum.h"

#include "fatal-error.h"
#include "log.h"

#include <algorithm> // find_if
#include <numeric>   // std::accumulate
#include <sstream>

/**
 * \file
 * \ingroup attribute_Enum
 * ns3::EnumValue attribute value implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Enum");

EnumValue::EnumValue()
    : m_value()
{
    NS_LOG_FUNCTION(this);
}

EnumValue::EnumValue(int value)
    : m_value(value)
{
    NS_LOG_FUNCTION(this << value);
}

void
EnumValue::Set(int value)
{
    NS_LOG_FUNCTION(this << value);
    m_value = value;
}

int
EnumValue::Get() const
{
    NS_LOG_FUNCTION(this);
    return m_value;
}

Ptr<AttributeValue>
EnumValue::Copy() const
{
    NS_LOG_FUNCTION(this);
    return ns3::Create<EnumValue>(*this);
}

std::string
EnumValue::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    NS_LOG_FUNCTION(this << checker);
    const EnumChecker* p = dynamic_cast<const EnumChecker*>(PeekPointer(checker));
    NS_ASSERT(p != nullptr);
    std::string name = p->GetName(m_value);
    return name;
}

bool
EnumValue::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    NS_LOG_FUNCTION(this << value << checker);
    const EnumChecker* p = dynamic_cast<const EnumChecker*>(PeekPointer(checker));
    NS_ASSERT(p != nullptr);
    m_value = p->GetValue(value);
    return true;
}

EnumChecker::EnumChecker()
{
    NS_LOG_FUNCTION(this);
}

void
EnumChecker::AddDefault(int value, std::string name)
{
    NS_LOG_FUNCTION(this << value << name);
    m_valueSet.emplace_front(value, name);
}

void
EnumChecker::Add(int value, std::string name)
{
    NS_LOG_FUNCTION(this << value << name);
    m_valueSet.emplace_back(value, name);
}

std::string
EnumChecker::GetName(int value) const
{
    auto it = std::find_if(m_valueSet.begin(), m_valueSet.end(), [value](Value v) {
        return v.first == value;
    });
    NS_ASSERT_MSG(it != m_valueSet.end(),
                  "invalid enum value " << value << "! Missed entry in MakeEnumChecker?");
    return it->second;
}

int
EnumChecker::GetValue(const std::string name) const
{
    auto it = std::find_if(m_valueSet.begin(), m_valueSet.end(), [name](Value v) {
        return v.second == name;
    });
    NS_ASSERT_MSG(
        it != m_valueSet.end(),
        "name "
            << name
            << " is not a valid enum value. Missed entry in MakeEnumChecker?\nAvailable values: "
            << std::accumulate(m_valueSet.begin(),
                               m_valueSet.end(),
                               std::string{},
                               [](std::string a, Value v) {
                                   if (a.empty())
                                   {
                                       return v.second;
                                   }
                                   else
                                   {
                                       return std::move(a) + ", " + v.second;
                                   }
                               }));
    return it->first;
}

bool
EnumChecker::Check(const AttributeValue& value) const
{
    NS_LOG_FUNCTION(this << &value);
    const EnumValue* p = dynamic_cast<const EnumValue*>(&value);
    if (p == nullptr)
    {
        return false;
    }
    auto pvalue = p->Get();
    auto it = std::find_if(m_valueSet.begin(), m_valueSet.end(), [pvalue](Value v) {
        return v.first == pvalue;
    });
    return (it != m_valueSet.end());
}

std::string
EnumChecker::GetValueTypeName() const
{
    NS_LOG_FUNCTION(this);
    return "ns3::EnumValue";
}

bool
EnumChecker::HasUnderlyingTypeInformation() const
{
    NS_LOG_FUNCTION(this);
    return true;
}

std::string
EnumChecker::GetUnderlyingTypeInformation() const
{
    NS_LOG_FUNCTION(this);
    std::ostringstream oss;
    bool moreValues = false;
    for (const auto& i : m_valueSet)
    {
        oss << (moreValues ? "|" : "") << i.second;
        moreValues = true;
    }
    return oss.str();
}

Ptr<AttributeValue>
EnumChecker::Create() const
{
    NS_LOG_FUNCTION(this);
    return ns3::Create<EnumValue>();
}

bool
EnumChecker::Copy(const AttributeValue& source, AttributeValue& destination) const
{
    NS_LOG_FUNCTION(this << &source << &destination);
    const EnumValue* src = dynamic_cast<const EnumValue*>(&source);
    EnumValue* dst = dynamic_cast<EnumValue*>(&destination);
    if (src == nullptr || dst == nullptr)
    {
        return false;
    }
    *dst = *src;
    return true;
}

} // namespace ns3
