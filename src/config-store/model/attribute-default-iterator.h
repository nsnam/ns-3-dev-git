/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Faker Moatamri <faker.moatamri@sophia.inria.fr>
 *          Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#ifndef ATTRIBUTE_DEFAULT_ITERATOR_H
#define ATTRIBUTE_DEFAULT_ITERATOR_H

#include "ns3/type-id.h"

#include <string>

namespace ns3
{

/**
 * @ingroup configstore
 *
 * @brief Iterator to iterate on the default values of attributes of an ns3::Object
 */
class AttributeDefaultIterator
{
  public:
    virtual ~AttributeDefaultIterator() = 0;
    /**
     * @brief This function will go through all the TypeIds and get only the attributes which are
     * explicit values (not vectors or pointer or arrays) and apply StartVisitTypeId
     * and VisitAttribute on the attributes in one TypeId. At the end of each TypeId
     * EndVisitTypeId is called.
     */
    void Iterate();

  private:
    /**
     * @brief Begin the analysis of a TypeId
     * @param name TypeId name
     */
    virtual void StartVisitTypeId(std::string name);
    /**
     * @brief End the analysis of a TypeId
     */
    virtual void EndVisitTypeId();
    /**
     * @brief Visit an Attribute
     * @param tid the TypeId the attribute belongs to
     * @param name the Attribute name
     * @param defaultValue the attribute default value
     * @param index the index of the Attribute
     */
    virtual void VisitAttribute(TypeId tid,
                                std::string name,
                                std::string defaultValue,
                                uint32_t index);
    /**
     * @brief Visit an Attribute
     * @param name the Attribute name
     * @param defaultValue the attribute default value
     */
    virtual void DoVisitAttribute(std::string name, std::string defaultValue);
};

} // namespace ns3

#endif /* ATTRIBUTE_DEFAULT_ITERATOR_H */
