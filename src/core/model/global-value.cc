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
#include "global-value.h"

#include "attribute.h"
#include "environment-variable.h"
#include "fatal-error.h"
#include "log.h"
#include "string.h"
#include "uinteger.h"

#include "ns3/core-config.h"

/**
 * \file
 * \ingroup core
 * ns3::GlobalValue implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("GlobalValue");

GlobalValue::GlobalValue(std::string name,
                         std::string help,
                         const AttributeValue& initialValue,
                         Ptr<const AttributeChecker> checker)
    : m_name(name),
      m_help(help),
      m_initialValue(nullptr),
      m_currentValue(nullptr),
      m_checker(checker)
{
    NS_LOG_FUNCTION(name << help << &initialValue << checker);
    if (!m_checker)
    {
        NS_FATAL_ERROR("Checker should not be zero on " << name);
    }
    m_initialValue = m_checker->CreateValidValue(initialValue);
    m_currentValue = m_initialValue;
    if (!m_initialValue)
    {
        NS_FATAL_ERROR("Value set by user on " << name << " is invalid.");
    }
    GetVector()->push_back(this);
    InitializeFromEnv();
}

void
GlobalValue::InitializeFromEnv()
{
    NS_LOG_FUNCTION(this);

    auto [found, value] = EnvironmentVariable::Get("NS_GLOBAL_VALUE", m_name);
    if (found)
    {
        Ptr<AttributeValue> v = m_checker->CreateValidValue(StringValue(value));
        if (v)
        {
            m_initialValue = v;
            m_currentValue = v;
        }
    }
}

std::string
GlobalValue::GetName() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_name;
}

std::string
GlobalValue::GetHelp() const
{
    NS_LOG_FUNCTION_NOARGS();
    return m_help;
}

void
GlobalValue::GetValue(AttributeValue& value) const
{
    NS_LOG_FUNCTION(&value);
    bool ok = m_checker->Copy(*m_currentValue, value);
    if (ok)
    {
        return;
    }
    StringValue* str = dynamic_cast<StringValue*>(&value);
    if (str == nullptr)
    {
        NS_FATAL_ERROR("GlobalValue name=" << m_name << ": input value is not a string");
    }
    str->Set(m_currentValue->SerializeToString(m_checker));
}

Ptr<const AttributeChecker>
GlobalValue::GetChecker() const
{
    NS_LOG_FUNCTION(this);

    return m_checker;
}

bool
GlobalValue::SetValue(const AttributeValue& value)
{
    NS_LOG_FUNCTION(&value);

    Ptr<AttributeValue> v = m_checker->CreateValidValue(value);
    if (!v)
    {
        return 0;
    }
    m_currentValue = v;
    return true;
}

void
GlobalValue::Bind(std::string name, const AttributeValue& value)
{
    NS_LOG_FUNCTION(name << &value);

    for (Iterator i = Begin(); i != End(); i++)
    {
        if ((*i)->GetName() == name)
        {
            if (!(*i)->SetValue(value))
            {
                NS_FATAL_ERROR("Invalid new value for global value: " << name);
            }
            return;
        }
    }
    NS_FATAL_ERROR("Non-existent global value: " << name);
}

bool
GlobalValue::BindFailSafe(std::string name, const AttributeValue& value)
{
    NS_LOG_FUNCTION(name << &value);

    for (Iterator i = Begin(); i != End(); i++)
    {
        if ((*i)->GetName() == name)
        {
            return (*i)->SetValue(value);
        }
    }
    return false;
}

GlobalValue::Iterator
GlobalValue::Begin()
{
    NS_LOG_FUNCTION_NOARGS();

    return GetVector()->begin();
}

GlobalValue::Iterator
GlobalValue::End()
{
    NS_LOG_FUNCTION_NOARGS();
    return GetVector()->end();
}

void
GlobalValue::ResetInitialValue()
{
    NS_LOG_FUNCTION(this);
    m_currentValue = m_initialValue;
}

bool
GlobalValue::GetValueByNameFailSafe(std::string name, AttributeValue& value)
{
    NS_LOG_FUNCTION(name << &value);
    for (GlobalValue::Iterator gvit = GlobalValue::Begin(); gvit != GlobalValue::End(); ++gvit)
    {
        if ((*gvit)->GetName() == name)
        {
            (*gvit)->GetValue(value);
            return true;
        }
    }
    return false; // not found
}

void
GlobalValue::GetValueByName(std::string name, AttributeValue& value)
{
    NS_LOG_FUNCTION(name << &value);
    if (!GetValueByNameFailSafe(name, value))
    {
        NS_FATAL_ERROR("Could not find GlobalValue named \"" << name << "\"");
    }
}

GlobalValue::Vector*
GlobalValue::GetVector()
{
    NS_LOG_FUNCTION_NOARGS();
    static Vector vector;
    return &vector;
}

} // namespace ns3
