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

#include "non-inheritance.h"

#include <algorithm>
#include <iterator>

namespace ns3
{

WifiInformationElementId
NonInheritance::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
NonInheritance::ElementIdExt() const
{
    return IE_EXT_NON_INHERITANCE;
}

void
NonInheritance::Print(std::ostream& os) const
{
    os << "NonInheritance=[";
    std::copy(m_elemIdList.cbegin(), m_elemIdList.cend(), std::ostream_iterator<uint16_t>(os, " "));
    os << "][";
    std::copy(m_elemIdExtList.cbegin(),
              m_elemIdExtList.cend(),
              std::ostream_iterator<uint16_t>(os, " "));
    os << "]";
}

uint16_t
NonInheritance::GetInformationFieldSize() const
{
    uint16_t size = 1; // Element ID Extension
    size += 1 /* Length */ + m_elemIdList.size();
    size += 1 /* Length */ + m_elemIdExtList.size();
    return size;
}

void
NonInheritance::SerializeInformationField(Buffer::Iterator start) const
{
    start.WriteU8(m_elemIdList.size());
    for (const auto id : m_elemIdList)
    {
        start.WriteU8(id);
    }
    start.WriteU8(m_elemIdExtList.size());
    for (const auto id : m_elemIdExtList)
    {
        start.WriteU8(id);
    }
}

uint16_t
NonInheritance::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    auto i = start;
    auto count = i.ReadU8();
    for (uint8_t j = 0; j < count; j++)
    {
        m_elemIdList.emplace(i.ReadU8());
    }
    count = i.ReadU8();
    for (uint8_t j = 0; j < count; j++)
    {
        m_elemIdExtList.emplace(i.ReadU8());
    }
    return i.GetDistanceFrom(start);
}

void
NonInheritance::Add(uint8_t elemId, uint8_t elemIdExt)
{
    elemId != IE_EXTENSION ? m_elemIdList.insert(elemId) : m_elemIdExtList.insert(elemIdExt);
}

bool
NonInheritance::IsPresent(uint8_t elemId, uint8_t elemIdExt) const
{
    return elemId != IE_EXTENSION ? m_elemIdList.count(elemId) == 1
                                  : m_elemIdExtList.count(elemIdExt) == 1;
}

} // namespace ns3
