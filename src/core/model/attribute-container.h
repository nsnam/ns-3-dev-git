/*
 * Copyright (c) 2018 Caliola Engineering, LLC.
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Jared Dulmage <jared.dulmage@caliola.com>
 */

#ifndef ATTRIBUTE_CONTAINER_H
#define ATTRIBUTE_CONTAINER_H

#include "attribute-helper.h"
#include "string.h"

#include <algorithm>
#include <iterator>
#include <list>
#include <sstream>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace ns3
{

/*!
 * @ingroup attributes
 * @addtogroup attribute_AttributeContainer AttributeContainer Attribute
 * AttributeValue implementation for AttributeContainer
 */

class AttributeChecker;

// A = attribute value type, C = container type to return
/**
 * @ingroup attribute_AttributeContainer
 *
 * A container for one type of attribute.
 *
 * The container uses \p A to parse items into elements.
 * Internally the container is always a list but an instance
 * can return the items in a container specified by \p C.
 *
 * @tparam A AttributeValue type to be contained.
 * @tparam Sep Character separator between elements for parsing.
 * @tparam C Possibly templated container class returned by Get.
 */
template <class A, char Sep = ',', template <class...> class C = std::list>
class AttributeContainerValue : public AttributeValue
{
  public:
    /** AttributeValue (element) type. */
    typedef A attribute_type;
    /** Type actually stored within the container. */
    typedef Ptr<A> value_type;
    /** Internal container type. */
    typedef std::list<value_type> container_type;
    /** stl-style Const iterator type. */
    typedef typename container_type::const_iterator const_iterator;
    /** stl-style Non-const iterator type. */
    typedef typename container_type::iterator iterator;
    /** Size type for container. */
    typedef typename container_type::size_type size_type;
    /** NS3 style iterator type. */
    typedef typename AttributeContainerValue::const_iterator Iterator; // NS3 type

    // use underlying AttributeValue to get return element type
    /** Item type of container returned by Get. */
    typedef typename std::invoke_result_t<decltype(&A::Get), A> item_type;
    /** Type of container returned. */
    typedef C<item_type> result_type;

    /**
     * Default constructor.
     */
    AttributeContainerValue();

    /**
     * Construct from another container.
     * @tparam CONTAINER \deduced type of container passed for initialization.
     * @param c Instance of CONTAINER with which to initialize AttributeContainerValue.
     */
    template <class CONTAINER>
    AttributeContainerValue(const CONTAINER& c);

    /**
     * Construct from iterators.
     * @tparam ITER \deduced type of iterator.
     * \param[in] begin Iterator that points to first initialization item.
     * \param[in] end Iterator that points ones past last initialization item.
     */
    template <class ITER>
    AttributeContainerValue(const ITER begin, const ITER end);

    /** Destructor. */
    ~AttributeContainerValue() override;

    // Inherited
    Ptr<AttributeValue> Copy() const override;
    bool DeserializeFromString(std::string value, Ptr<const AttributeChecker> checker) override;
    std::string SerializeToString(Ptr<const AttributeChecker> checker) const override;

    // defacto pure virtuals to integrate with built-in accessor code
    /**
     * Return a container of items.
     * @return Container of items.
     */
    result_type Get() const;
    /**
     * Copy items from container c.
     *
     * This method assumes \p c has stl-style begin and end methods.
     * The AttributeContainerValue value is cleared before copying from \p c.
     * @tparam T type of container.
     * @param c Container from which to copy items.
     */
    template <class T>
    void Set(const T& c);
    /**
     * Set the given variable to the values stored by this TupleValue object.
     *
     * @tparam T \deduced the type of the given variable (normally, the argument type
     *           of a set method or the type of a data member)
     * @param value the given variable
     * @return true if the given variable was set
     */
    template <typename T>
    bool GetAccessor(T& value) const;

    // NS3 interface
    /**
     * NS3-style Number of items.
     * @return Number of items in container.
     */
    size_type GetN() const;
    /**
     * NS3-style beginning of container.
     * @return Iterator pointing to first time in container.
     */
    Iterator Begin();
    /**
     * NS3-style ending of container.
     * @return Iterator pointing one past last item of container.
     */
    Iterator End();

    // STL-interface
    /**
     * STL-style number of items in container
     * @return number of items in container.
     */
    size_type size() const;
    /**
     * STL-style beginning of container.
     * @return Iterator pointing to first item in container.
     */
    iterator begin();
    /**
     * STL-style end of container.
     * @return Iterator pointing to one past last item in container.
     */
    iterator end();
    /**
     * STL-style const beginning of container.
     * @return Const iterator pointing to first item in container.
     */
    const_iterator begin() const;
    /**
     * STL-style const end of container.
     * @return Const iterator pointing to one past last item in container.
     */
    const_iterator end() const;

  private:
    /**
     * Copy items from \ref begin to \ref end.
     *
     * The internal container is cleared before values are copied
     * using the push_back method.
     * @tparam ITER \deduced iterator type
     * \param[in] begin Points to first item to copy
     * \param[in] end Points to one after last item to copy
     * @return This object with items copied.
     */
    template <class ITER>
    inline Ptr<AttributeContainerValue<A, Sep, C>> CopyFrom(const ITER begin, const ITER end);

    container_type m_container; //!< Internal container
};

/*!
 * @ingroup attribute_AttributeContainer
 *
 * @class  ns3::AttributeContainerChecker "attribute-container.h"
 * AttributeChecker implementation for AttributeContainerValue.
 * @see AttributeChecker
 */
class AttributeContainerChecker : public AttributeChecker
{
  public:
    /**
     * Set the item checker
     * @param itemchecker The item checker
     */
    virtual void SetItemChecker(Ptr<const AttributeChecker> itemchecker) = 0;
    /**
     * Get the item checker
     * @return The item checker
     */
    virtual Ptr<const AttributeChecker> GetItemChecker() const = 0;
};

/**
 * @ingroup attribute_AttributeContainer
 *
 * Make AttributeContainerChecker from AttributeContainerValue.
 * @tparam A \deduced AttributeValue type in container.
 * @tparam Sep \deduced Character separator between elements for parsing.
 * @tparam C \deduced Container type returned by Get.
 * \param[in] value AttributeContainerValue from which to deduce types.
 * @return AttributeContainerChecker for value.
 */
template <class A, char Sep, template <class...> class C>
Ptr<AttributeChecker> MakeAttributeContainerChecker(
    const AttributeContainerValue<A, Sep, C>& value);

/**
 * @ingroup attribute_AttributeContainer
 *
 * Make AttributeContainerChecker using explicit types, initialize item checker.
 * @tparam A AttributeValue type in container.
 * @tparam Sep Character separator between elements for parsing.
 * @tparam C Container type returned by Get.
 * \param[in] itemchecker AttributeChecker used for each item in the container.
 * @return AttributeContainerChecker.
 */
template <class A, char Sep = ',', template <class...> class C = std::list>
Ptr<const AttributeChecker> MakeAttributeContainerChecker(Ptr<const AttributeChecker> itemchecker);

/**
 * @ingroup attribute_AttributeContainer
 *
 * Make uninitialized AttributeContainerChecker using explicit types.
 * @tparam A AttributeValue type in container.
 * @tparam Sep Character separator between elements for parsing.
 * @tparam C Container type returned by Get.
 * @return AttributeContainerChecker.
 */
template <class A, char Sep = ',', template <class...> class C = std::list>
Ptr<AttributeChecker> MakeAttributeContainerChecker();

/**
 * @ingroup attribute_AttributeContainer
 *
 * Make AttributeContainerAccessor  using explicit types.
 * @tparam A AttributeValue type in container.
 * @tparam Sep Character separator between elements for parsing.
 * @tparam C Container type returned by Get.
 * @tparam T1 \deduced The type of the class data member,
 *            or the type of the class get functor or set method.
 * @param [in] a1 The address of the data member,
 *            or the get or set method.
 * @return AttributeContainerAccessor.
 */
template <typename A, char Sep = ',', template <typename...> class C = std::list, typename T1>
Ptr<const AttributeAccessor> MakeAttributeContainerAccessor(T1 a1);

/**
 * @ingroup attribute_AttributeContainer
 *
 * Make AttributeContainerAccessor  using explicit types.
 * @tparam A AttributeValue type in container.
 * @tparam Sep Character separator between elements for parsing.
 * @tparam C Container type returned by Get.
 * @tparam T1 \deduced The type of the class data member,
 *            or the type of the class get functor or set method.
 *
 * @tparam T2 \deduced The type of the getter class functor method.
 * @param [in] a2 The address of the class method to set the attribute.
 * @param [in] a1 The address of the data member,
 *            or the get or set method.
 * @return AttributeContainerAccessor.
 */
template <typename A,
          char Sep = ',',
          template <typename...> class C = std::list,
          typename T1,
          typename T2>
Ptr<const AttributeAccessor> MakeAttributeContainerAccessor(T1 a1, T2 a2);

} // namespace ns3

/*****************************************************************************
 * Implementation below
 *****************************************************************************/

namespace ns3
{

namespace internal
{

/**
 * @ingroup attribute_AttributeContainer
 *
 * @internal
 *
 * Templated AttributeContainerChecker class that is instantiated
 * in MakeAttributeContainerChecker. The non-templated base ns3::AttributeContainerChecker
 * is returned from that function. This is the same pattern as ObjectPtrContainer.
 */
template <class A, char Sep, template <class...> class C>
class AttributeContainerChecker : public ns3::AttributeContainerChecker
{
  public:
    AttributeContainerChecker();
    /**
     * Explicit constructor
     * @param itemchecker The AttributeChecker.
     */
    explicit AttributeContainerChecker(Ptr<const AttributeChecker> itemchecker);
    void SetItemChecker(Ptr<const AttributeChecker> itemchecker) override;
    Ptr<const AttributeChecker> GetItemChecker() const override;

  private:
    Ptr<const AttributeChecker> m_itemchecker; //!< The AttributeChecker
};

template <class A, char Sep, template <class...> class C>
AttributeContainerChecker<A, Sep, C>::AttributeContainerChecker()
    : m_itemchecker(nullptr)
{
}

template <class A, char Sep, template <class...> class C>
AttributeContainerChecker<A, Sep, C>::AttributeContainerChecker(
    Ptr<const AttributeChecker> itemchecker)
    : m_itemchecker(itemchecker)
{
}

template <class A, char Sep, template <class...> class C>
void
AttributeContainerChecker<A, Sep, C>::SetItemChecker(Ptr<const AttributeChecker> itemchecker)
{
    m_itemchecker = itemchecker;
}

template <class A, char Sep, template <class...> class C>
Ptr<const AttributeChecker>
AttributeContainerChecker<A, Sep, C>::GetItemChecker() const
{
    return m_itemchecker;
}

} // namespace internal

template <class A, char Sep, template <class...> class C>
Ptr<AttributeChecker>
MakeAttributeContainerChecker(const AttributeContainerValue<A, Sep, C>& value)
{
    return MakeAttributeContainerChecker<A, Sep, C>();
}

template <class A, char Sep, template <class...> class C>
Ptr<const AttributeChecker>
MakeAttributeContainerChecker(Ptr<const AttributeChecker> itemchecker)
{
    auto checker = MakeAttributeContainerChecker<A, Sep, C>();
    auto acchecker = DynamicCast<AttributeContainerChecker>(checker);
    acchecker->SetItemChecker(itemchecker);
    return checker;
}

template <class A, char Sep, template <class...> class C>
Ptr<AttributeChecker>
MakeAttributeContainerChecker()
{
    std::string containerType;
    std::string underlyingType;
    typedef AttributeContainerValue<A, Sep, C> T;
    {
        std::ostringstream oss;
        oss << "ns3::AttributeContainerValue<" << typeid(typename T::attribute_type).name() << ", "
            << typeid(typename T::container_type).name() << ">";
        containerType = oss.str();
    }

    {
        std::ostringstream oss;
        oss << "ns3::Ptr<" << typeid(typename T::attribute_type).name() << ">";
        underlyingType = oss.str();
    }

    return MakeSimpleAttributeChecker<T, internal::AttributeContainerChecker<A, Sep, C>>(
        containerType,
        underlyingType);
}

template <class A, char Sep, template <class...> class C>
AttributeContainerValue<A, Sep, C>::AttributeContainerValue()
{
}

template <class A, char Sep, template <class...> class C>
template <class CONTAINER>
AttributeContainerValue<A, Sep, C>::AttributeContainerValue(const CONTAINER& c)
    : AttributeContainerValue<A, Sep, C>(c.begin(), c.end())
{
}

template <class A, char Sep, template <class...> class C>
template <class ITER>
AttributeContainerValue<A, Sep, C>::AttributeContainerValue(const ITER begin, const ITER end)
    : AttributeContainerValue()
{
    CopyFrom(begin, end);
}

template <class A, char Sep, template <class...> class C>
AttributeContainerValue<A, Sep, C>::~AttributeContainerValue()
{
    m_container.clear();
}

template <class A, char Sep, template <class...> class C>
Ptr<AttributeValue>
AttributeContainerValue<A, Sep, C>::Copy() const
{
    auto c = Create<AttributeContainerValue<A, Sep, C>>();
    c->m_container = m_container;
    return c;
}

template <class A, char Sep, template <class...> class C>
bool
AttributeContainerValue<A, Sep, C>::DeserializeFromString(std::string value,
                                                          Ptr<const AttributeChecker> checker)
{
    auto acchecker = DynamicCast<const AttributeContainerChecker>(checker);
    if (!acchecker)
    {
        return false;
    }

    std::istringstream iss(value); // copies value
    while (std::getline(iss, value, Sep))
    {
        auto avalue = acchecker->GetItemChecker()->CreateValidValue(StringValue(value));
        if (!avalue)
        {
            return false;
        }

        auto attr = DynamicCast<A>(avalue);
        if (!attr)
        {
            return false;
        }

        // TODO(jared): make insertion more generic?
        m_container.push_back(attr);
    }
    return true;
}

template <class A, char Sep, template <class...> class C>
std::string
AttributeContainerValue<A, Sep, C>::SerializeToString(Ptr<const AttributeChecker> checker) const
{
    std::ostringstream oss;
    bool first = true;
    for (auto attr : *this)
    {
        if (!first)
        {
            oss << Sep;
        }
        oss << attr->SerializeToString(checker);
        first = false;
    }
    return oss.str();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::result_type
AttributeContainerValue<A, Sep, C>::Get() const
{
    result_type c;
    for (const value_type& a : *this)
    {
        c.insert(c.end(), a->Get());
    }
    return c;
}

template <class A, char Sep, template <class...> class C>
template <typename T>
bool
AttributeContainerValue<A, Sep, C>::GetAccessor(T& value) const
{
    result_type src = Get();
    value.clear();
    std::copy(src.begin(), src.end(), std::inserter(value, value.end()));
    return true;
}

template <class A, char Sep, template <class...> class C>
template <class T>
void
AttributeContainerValue<A, Sep, C>::Set(const T& c)
{
    m_container.clear();
    CopyFrom(c.begin(), c.end());
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::size_type
AttributeContainerValue<A, Sep, C>::GetN() const
{
    return size();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::Iterator
AttributeContainerValue<A, Sep, C>::Begin()
{
    return begin();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::Iterator
AttributeContainerValue<A, Sep, C>::End()
{
    return end();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::size_type
AttributeContainerValue<A, Sep, C>::size() const
{
    return m_container.size();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::iterator
AttributeContainerValue<A, Sep, C>::begin()
{
    return m_container.begin();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::iterator
AttributeContainerValue<A, Sep, C>::end()
{
    return m_container.end();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::const_iterator
AttributeContainerValue<A, Sep, C>::begin() const
{
    return m_container.cbegin();
}

template <class A, char Sep, template <class...> class C>
typename AttributeContainerValue<A, Sep, C>::const_iterator
AttributeContainerValue<A, Sep, C>::end() const
{
    return m_container.cend();
}

template <class A, char Sep, template <class...> class C>
template <class ITER>
Ptr<AttributeContainerValue<A, Sep, C>>
AttributeContainerValue<A, Sep, C>::CopyFrom(const ITER begin, const ITER end)
{
    for (ITER iter = begin; iter != end; ++iter)
    {
        m_container.push_back(Create<A>(*iter));
    }
    return this;
}

template <typename A, char Sep, template <typename...> class C, typename T1>
Ptr<const AttributeAccessor>
MakeAttributeContainerAccessor(T1 a1)
{
    return MakeAccessorHelper<AttributeContainerValue<A, Sep, C>>(a1);
}

template <typename A, char Sep, template <typename...> class C, typename T1, typename T2>
Ptr<const AttributeAccessor>
MakeAttributeContainerAccessor(T1 a1, T2 a2)
{
    return MakeAccessorHelper<AttributeContainerValue<A, Sep, C>>(a1, a2);
}

} // namespace ns3

#endif // ATTRIBUTE_CONTAINER_H
