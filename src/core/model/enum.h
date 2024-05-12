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
#ifndef ENUM_VALUE_H
#define ENUM_VALUE_H

#include "attribute-accessor-helper.h"
#include "attribute.h"

#include <algorithm> // find_if
#include <list>
#include <numeric> // std::accumulate
#include <sstream>
#include <type_traits>
#include <typeinfo>

/**
 * \file
 * \ingroup attribute_Enum
 * ns3::EnumValue attribute value declarations.
 */

namespace ns3
{

//  Additional docs for class EnumValue:
/**
 * Hold variables of type \c enum
 *
 * This class can be used to hold variables of any kind
 * of enum.
 *
 * This is often used with ObjectFactory and Config to bind
 * the value of a particular enum to an Attribute or Config name.
 * For example,
 * \code
 *   Ptr<RateErrorModel> model = CreateObjectWithAttributes<RateErrorModel> (
 *     "ErrorRate", DoubleValue (0.05),
 *     "ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
 *
 *   Config::SetDefault ("ns3::RipNg::SplitHorizon",
 *                       EnumValue (RipNg::NO_SPLIT_HORIZON));
 * \endcode
 */
template <typename T>
class EnumValue : public AttributeValue
{
  public:
    EnumValue();
    EnumValue(const T& value);
    void Set(T value);
    T Get() const;

    bool GetAccessor(T& value) const;

    Ptr<AttributeValue> Copy() const override;
    std::string SerializeToString(Ptr<const AttributeChecker> checker) const override;
    bool DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker) override;

  private:
    T m_value{}; //!< The stored value.
};

template <typename T>
EnumValue<T>::EnumValue() = default;

template <typename T>
EnumValue<T>::EnumValue(const T& value)
    : m_value(value)
{
}

template <typename T>
void
EnumValue<T>::Set(T value)
{
    m_value = value;
}

template <typename T>
T
EnumValue<T>::Get() const
{
    return m_value;
}

template <typename T>
bool
EnumValue<T>::GetAccessor(T& value) const
{
    value = static_cast<T>(m_value);
    return true;
}

template <typename T>
Ptr<AttributeValue>
EnumValue<T>::Copy() const
{
    return Create<EnumValue>(*this);
}

template <typename T>
class EnumChecker : public AttributeChecker
{
  public:
    EnumChecker();

    /**
     * Add a default value.
     * \param [in] value The value.
     * \param [in] name Then enum symbol name.
     */
    void AddDefault(T value, std::string name);
    /**
     * Add a new value.
     * \param [in] value The value.
     * \param [in] name The enum symbol name.
     */
    void Add(T value, std::string name);

    /**
     * Get the enum symbol name by value.
     * \param [in] value The value.
     * \return The enum symbol name.
     */
    std::string GetName(T value) const;

    /**
     * Get the enum value by name.
     * \param [in] name Then enum symbol name.
     * \returns The enum value.
     */
    T GetValue(const std::string name) const;

    // Inherited
    bool Check(const AttributeValue& value) const override;
    std::string GetValueTypeName() const override;
    bool HasUnderlyingTypeInformation() const override;
    std::string GetUnderlyingTypeInformation() const override;
    Ptr<AttributeValue> Create() const override;
    bool Copy(const AttributeValue& src, AttributeValue& dst) const override;

  private:
    /** Type for the pair value, name */
    using Value = std::pair<T, std::string>;
    /** Type of container for storing Enum values and symbol names. */
    using ValueSet = std::list<Value>;
    /** The stored Enum values and symbol names. */
    ValueSet m_valueSet;
};

/**
 * Make an EnumChecker pre-configured with a set of allowed
 * values by name.
 *
 * Values are normally given as fully qualified enum symbols
 * with matching names.  For example,
 * \c MakeEnumChecker (RipNg::SPLIT_HORIZON, "ns3::RipNg::SplitHorizon");
 *
 * As many additional enum value, name pairs as desired can be passed
 * as arguments.
 *
 * \see AttributeChecker
 *
 * \tparam Ts The type list of additional parameters. Additional parameters
 *            should be T, string pairs.
 * \returns The AttributeChecker
 * \param [in] v  The default enum value.
 * \param [in] n  The corresponding name.
 * \param [in] args Any additional arguments.
 */
template <typename T, typename... Ts>
Ptr<const AttributeChecker>
MakeEnumChecker(T v, std::string n, Ts... args)
{
    Ptr<EnumChecker<T>> checker = Create<EnumChecker<T>>();
    checker->AddDefault(v, n);
    return MakeEnumChecker(checker, args...);
}

/**
 * Handler for enum value, name pairs other than the default.
 *
 * \tparam Ts The type list of additional parameters. Additional parameters
 *            should be T, string pairs.
 * \returns The AttributeChecker
 * \param [in] checker The AttributeChecker.
 * \param [in] v  The next enum value.
 * \param [in] n  The corresponding name.
 * \param [in] args Any additional arguments.
 */
template <typename T, typename... Ts>
Ptr<const AttributeChecker>
MakeEnumChecker(Ptr<EnumChecker<T>> checker, T v, std::string n, Ts... args)
{
    checker->Add(v, n);
    return MakeEnumChecker(checker, args...);
}

/**
 * Terminate the recursion of variadic arguments.
 *
 * \returns The \p checker
 * \param [in] checker The AttributeChecker.
 */
// inline to allow tail call optimization
template <typename T>
inline Ptr<const AttributeChecker>
MakeEnumChecker(Ptr<EnumChecker<T>> checker)
{
    return checker;
}

template <typename T, typename T1>
Ptr<const AttributeAccessor>
MakeEnumAccessor(T1 a1)
{
    return MakeAccessorHelper<EnumValue<T>>(a1);
}

template <typename T, typename T1, typename T2>
Ptr<const AttributeAccessor>
MakeEnumAccessor(T1 a1, T2 a2)
{
    return MakeAccessorHelper<EnumValue<T>>(a1, a2);
}

template <typename T>
std::string
EnumValue<T>::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    const auto p = dynamic_cast<const EnumChecker<T>*>(PeekPointer(checker));
    NS_ASSERT(p != nullptr);
    std::string name = p->GetName(m_value);
    return name;
}

template <typename T>
bool
EnumValue<T>::DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker)
{
    const auto p = dynamic_cast<const EnumChecker<T>*>(PeekPointer(checker));
    NS_ASSERT(p != nullptr);
    m_value = p->GetValue(value);
    return true;
}

template <typename T>
EnumChecker<T>::EnumChecker()
{
}

template <typename T>
void
EnumChecker<T>::AddDefault(T value, std::string name)
{
    m_valueSet.emplace_front(value, name);
}

template <typename T>
void
EnumChecker<T>::Add(T value, std::string name)
{
    m_valueSet.emplace_back(value, name);
}

template <typename T>
std::string
EnumChecker<T>::GetName(T value) const
{
    auto it = std::find_if(m_valueSet.begin(), m_valueSet.end(), [value](Value v) {
        return v.first == value;
    });

    NS_ASSERT_MSG(it != m_valueSet.end(),
                  "invalid enum value " << static_cast<int>(value)
                                        << "! Missed entry in MakeEnumChecker?");
    return it->second;
}

template <typename T>
T
EnumChecker<T>::GetValue(const std::string name) const
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

template <typename T>
bool
EnumChecker<T>::Check(const AttributeValue& value) const
{
    const auto p = dynamic_cast<const EnumValue<T>*>(&value);
    if (!p)
    {
        return false;
    }
    auto pvalue = p->Get();
    auto it = std::find_if(m_valueSet.begin(), m_valueSet.end(), [pvalue](Value v) {
        return v.first == pvalue;
    });
    return (it != m_valueSet.end());
}

template <typename T>
std::string
EnumChecker<T>::GetValueTypeName() const
{
    return "ns3::EnumValue<" + std::string(typeid(T).name()) + ">";
}

template <typename T>
bool
EnumChecker<T>::HasUnderlyingTypeInformation() const
{
    return true;
}

template <typename T>
std::string
EnumChecker<T>::GetUnderlyingTypeInformation() const
{
    std::ostringstream oss;
    bool moreValues = false;
    for (const auto& i : m_valueSet)
    {
        oss << (moreValues ? "|" : "") << i.second;
        moreValues = true;
    }
    return oss.str();
}

template <typename T>
Ptr<AttributeValue>
EnumChecker<T>::Create() const
{
    return ns3::Create<EnumValue<T>>();
}

template <typename T>
bool
EnumChecker<T>::Copy(const AttributeValue& source, AttributeValue& destination) const
{
    const auto src = dynamic_cast<const EnumValue<T>*>(&source);
    auto dst = dynamic_cast<EnumValue<T>*>(&destination);
    if (!src || !dst)
    {
        return false;
    }
    *dst = *src;
    return true;
}

} // namespace ns3

#endif /* ENUM_VALUE_H */
