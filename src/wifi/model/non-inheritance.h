/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef NON_INHERITANCE_H
#define NON_INHERITANCE_H

#include "wifi-information-element.h"

#include <set>

namespace ns3
{

/**
 * @ingroup wifi
 *
 * The IEEE 802.11 Non-Inheritance Information Element
 */
class NonInheritance : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    /**
     * Add the Information Element specified by the given Element ID and Element ID Extension.
     *
     * @param elemId the given Element ID
     * @param elemIdExt the given Element ID Extension
     */
    void Add(uint8_t elemId, uint8_t elemIdExt = 0);

    /**
     * @param elemId the given Element ID
     * @param elemIdExt the given Element ID Extension
     * @return whether the Information Element specified by the given Element ID and
     *         Element ID Extension is present
     */
    bool IsPresent(uint8_t elemId, uint8_t elemIdExt = 0) const;

    std::set<uint8_t> m_elemIdList;    //!< list of unique Element ID values (in increasing order)
    std::set<uint8_t> m_elemIdExtList; //!< list of unique Element ID Extension values

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* NON_INHERITANCE_H */
