/*
 * Copyright (c) 2023 Universita' degli Studi di Napoli Federico II
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
 * Author: Stefano Avallone <stavallo@unina.it>
 */

#ifndef NON_INHERITANCE_H
#define NON_INHERITANCE_H

#include "wifi-information-element.h"

#include <set>

namespace ns3
{

/**
 * \ingroup wifi
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
     * \param elemId the given Element ID
     * \param elemIdExt the given Element ID Extension
     */
    void Add(uint8_t elemId, uint8_t elemIdExt = 0);

    /**
     * \param elemId the given Element ID
     * \param elemIdExt the given Element ID Extension
     * \return whether the Information Element specified by the given Element ID and
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
