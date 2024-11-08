/*
 * Copyright (c) 2011 Mathieu Lacage
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Mathieu Lacage <mathieu.lacage@gmail.com>
 */
#ifndef ATTRIBUTE_CONSTRUCTION_LIST_H
#define ATTRIBUTE_CONSTRUCTION_LIST_H

#include "attribute.h"

#include <list>

/**
 * @file
 * @ingroup object
 * ns3::AttributeConstructionList declaration.
 */

namespace ns3
{

/**
 * @ingroup object
 * List of Attribute name, value and checker triples used
 * to construct Objects.
 */
class AttributeConstructionList
{
  public:
    /** A single Attribute triple */
    struct Item
    {
        /** Checker used to validate serialized values. */
        Ptr<const AttributeChecker> checker;
        /** The value of the Attribute. */
        Ptr<AttributeValue> value;
        /** The name of the Attribute. */
        std::string name;
    };

    /** Iterator type. */
    typedef std::list<Item>::const_iterator CIterator;

    /** Constructor */
    AttributeConstructionList();

    /**
     * Add an Attribute to the list.
     *
     * @param [in] name The Attribute name.
     * @param [in] checker The checker to use for this Attribute.
     * @param [in] value The AttributeValue to add.
     */
    void Add(std::string name, Ptr<const AttributeChecker> checker, Ptr<AttributeValue> value);

    /**
     * Find an Attribute in the list from its AttributeChecker.
     *
     * @param [in] checker The AttributeChecker to find.  Typically this is the
     *             AttributeChecker from TypeId::AttributeInformation.
     * @returns The AttributeValue.
     */
    Ptr<AttributeValue> Find(Ptr<const AttributeChecker> checker) const;

    /** @returns The first item in the list */
    CIterator Begin() const;
    /** @returns The end of the list (iterator to one past the last). */
    CIterator End() const;

  private:
    /** The list of Items */
    std::list<Item> m_list;
};

} // namespace ns3

#endif /* ATTRIBUTE_CONSTRUCTION_LIST_H */
