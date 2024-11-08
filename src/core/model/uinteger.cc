/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "uinteger.h"

#include "fatal-error.h"
#include "log.h"

#include <sstream>

/**
 * @file
 * @ingroup attribute_Uinteger
 * ns3::UintegerValue attribute value implementations.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Uinteger");

ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(uint64_t, Uinteger);

namespace internal
{

/**
 * @ingroup attribute_Uinteger
 * Make an Uinteger attribute checker with embedded numeric type name.
 *
 * @param [in] min The minimum allowed value.
 * @param [in] max The maximum allowed value.
 * @param [in] name The original type name ("uint8_t", "uint16_t", _etc_.).
 * @returns The AttributeChecker.
 */
Ptr<const AttributeChecker>
MakeUintegerChecker(uint64_t min, uint64_t max, std::string name)
{
    NS_LOG_FUNCTION(min << max << name);

    struct Checker : public AttributeChecker
    {
        Checker(uint64_t minValue, uint64_t maxValue, std::string name)
            : m_minValue(minValue),
              m_maxValue(maxValue),
              m_name(name)
        {
        }

        bool Check(const AttributeValue& value) const override
        {
            NS_LOG_FUNCTION(&value);
            const auto v = dynamic_cast<const UintegerValue*>(&value);
            if (v == nullptr)
            {
                return false;
            }
            return v->Get() >= m_minValue && v->Get() <= m_maxValue;
        }

        std::string GetValueTypeName() const override
        {
            NS_LOG_FUNCTION_NOARGS();
            return "ns3::UintegerValue";
        }

        bool HasUnderlyingTypeInformation() const override
        {
            NS_LOG_FUNCTION_NOARGS();
            return true;
        }

        std::string GetUnderlyingTypeInformation() const override
        {
            NS_LOG_FUNCTION_NOARGS();
            std::ostringstream oss;
            oss << m_name << " " << m_minValue << ":" << m_maxValue;
            return oss.str();
        }

        Ptr<AttributeValue> Create() const override
        {
            NS_LOG_FUNCTION_NOARGS();
            return ns3::Create<UintegerValue>();
        }

        bool Copy(const AttributeValue& source, AttributeValue& destination) const override
        {
            NS_LOG_FUNCTION(&source << &destination);
            const auto src = dynamic_cast<const UintegerValue*>(&source);
            auto dst = dynamic_cast<UintegerValue*>(&destination);
            if (src == nullptr || dst == nullptr)
            {
                return false;
            }
            *dst = *src;
            return true;
        }

        uint64_t m_minValue;
        uint64_t m_maxValue;
        std::string m_name;
    }* checker = new Checker(min, max, name);

    return Ptr<const AttributeChecker>(checker, false);
}

} // namespace internal

} // namespace ns3
