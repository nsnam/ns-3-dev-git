/*
 * Copyright (c) 2008 INRIA
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */
#include "double.h"

#include "log.h"
#include "object.h"

#include <sstream>

/**
 * @file
 * @ingroup attribute_Double
 * ns3::DoubleValue attribute value implementation.
 */

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("Double");

ATTRIBUTE_VALUE_IMPLEMENT_WITH_NAME(double, Double);

/** Namespace for implementation details. */
namespace internal
{

/**
 * @ingroup attribute_Double
 * Make a Double attribute checker with embedded numeric type name.
 *
 * @param [in] min The minimum allowed value.
 * @param [in] max The maximum allowed value.
 * @param [in] name The original type name ("float", "double").
 * @returns The AttributeChecker.
 */
Ptr<const AttributeChecker>
MakeDoubleChecker(double min, double max, std::string name)
{
    NS_LOG_FUNCTION(min << max << name);

    struct Checker : public AttributeChecker
    {
        Checker(double minValue, double maxValue, std::string name)
            : m_minValue(minValue),
              m_maxValue(maxValue),
              m_name(name)
        {
        }

        bool Check(const AttributeValue& value) const override
        {
            NS_LOG_FUNCTION(&value);
            const auto v = dynamic_cast<const DoubleValue*>(&value);
            if (v == nullptr)
            {
                return false;
            }
            return v->Get() >= m_minValue && v->Get() <= m_maxValue;
        }

        std::string GetValueTypeName() const override
        {
            NS_LOG_FUNCTION_NOARGS();
            return "ns3::DoubleValue";
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
            return ns3::Create<DoubleValue>();
        }

        bool Copy(const AttributeValue& source, AttributeValue& destination) const override
        {
            NS_LOG_FUNCTION(&source << &destination);
            const auto src = dynamic_cast<const DoubleValue*>(&source);
            auto dst = dynamic_cast<DoubleValue*>(&destination);
            if (src == nullptr || dst == nullptr)
            {
                return false;
            }
            *dst = *src;
            return true;
        }

        double m_minValue;
        double m_maxValue;
        std::string m_name;
    }* checker = new Checker(min, max, name);

    return Ptr<const AttributeChecker>(checker, false);
}

} // namespace internal

} // namespace ns3
