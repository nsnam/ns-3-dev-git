/*
 * Copyright (c) 2022
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
 * Author: Sharan Naribole <sharan.naribole@gmail.com>
 */

#include <ns3/assert.h>
#include <ns3/tid-to-link-mapping-element.h>

namespace ns3
{

uint16_t
TidToLinkMapping::Control::GetSubfieldSize() const
{
    // IEEE 802.11be D2.0 Figure 9-1002an
    NS_ASSERT_MSG(defaultMapping != presenceBitmap.has_value(),
                  "Presence bitmap not expected if default mapping is set");
    auto size = WIFI_TID_TO_LINK_MAPPING_CONTROL_BASIC_SIZE_B;
    if (defaultMapping)
    {
        return size;
    }
    return size + WIFI_LINK_MAPPING_PRESENCE_IND_SIZE_B;
}

void
TidToLinkMapping::Control::Serialize(Buffer::Iterator& start) const
{
    uint8_t val = static_cast<uint8_t>(direction);
    val |= ((defaultMapping ? 1 : 0) << 2);
    start.WriteU8(val);
    NS_ASSERT_MSG(defaultMapping != presenceBitmap.has_value(),
                  "Presence bitmap not expected if default mapping is set");
    if (presenceBitmap.has_value())
    {
        start.WriteU8(presenceBitmap.value());
    }
}

uint16_t
TidToLinkMapping::Control::Deserialize(Buffer::Iterator start)
{
    auto i = start;
    uint8_t count = 0;
    auto val = i.ReadU8();
    count++;

    direction = static_cast<TidLinkMapDir>(val & 0x03);
    defaultMapping = (((val >> 2) & 0x01) == 1);
    if (defaultMapping)
    {
        return count;
    }
    presenceBitmap = i.ReadU8();
    return ++count;
}

WifiInformationElementId
TidToLinkMapping::ElementId() const
{
    return IE_EXTENSION;
}

WifiInformationElementId
TidToLinkMapping::ElementIdExt() const
{
    return IE_EXT_TID_TO_LINK_MAPPING_ELEMENT;
}

void
TidToLinkMapping::SetLinkMappingOfTid(uint8_t tid, std::list<uint8_t> linkIds)
{
    NS_ABORT_MSG_IF(tid > 7, "Invalid tid: " << +tid);
    NS_ABORT_MSG_IF(m_control.defaultMapping,
                    "Per-TID link mapping not expected if default mapping is set");

    // derive link mapping for the given TID
    uint16_t linkMapping = 0;

    for (const auto& linkId : linkIds)
    {
        linkMapping |= (1 << linkId);
    }

    m_linkMapping[tid] = linkMapping;

    if (!m_control.presenceBitmap.has_value())
    {
        m_control.presenceBitmap = 0;
    }
    *m_control.presenceBitmap |= (1 << tid);
}

std::list<uint8_t>
TidToLinkMapping::GetLinkMappingOfTid(uint8_t tid) const
{
    auto it = m_linkMapping.find(tid);

    if (it == m_linkMapping.end())
    {
        return {};
    }

    std::list<uint8_t> linkIds;
    for (uint8_t linkId = 0; linkId < 15; linkId++)
    {
        if (((it->second >> linkId) & 0x0001) == 1)
        {
            linkIds.push_back(linkId);
        }
    }

    return linkIds;
}

uint16_t
TidToLinkMapping::GetInformationFieldSize() const
{
    // IEEE 802.11be D2.0 9.4.2.314 TID-To-Link Mapping element
    uint16_t ret = WIFI_IE_ELEMENT_ID_EXT_SIZE; // Element ID Extension
    ret += m_control.GetSubfieldSize();
    NS_ASSERT_MSG(m_linkMapping.empty() == m_control.defaultMapping,
                  "Per-TID link mapping not expected if default mapping is set");
    ret += WIFI_LINK_MAPPING_PER_TID_SIZE_B * (m_linkMapping.size());
    return ret;
}

void
TidToLinkMapping::SerializeInformationField(Buffer::Iterator start) const
{
    // IEEE 802.11be D2.0 9.4.2.314 TID-To-Link Mapping element
    m_control.Serialize(start);
    NS_ASSERT_MSG(m_linkMapping.empty() == m_control.defaultMapping,
                  "Per-TID link mapping not expected if default mapping is set");

    for (const auto& [tid, linkMapping] : m_linkMapping)
    {
        start.WriteHtolsbU16(linkMapping);
    }
}

uint16_t
TidToLinkMapping::DeserializeInformationField(Buffer::Iterator start, uint16_t length)
{
    auto i = start;
    uint16_t count = 0;
    auto nCtrlOctets = m_control.Deserialize(i);
    NS_ASSERT_MSG(nCtrlOctets <= length, "Tid-to-Link Mapping deserialize error");
    i.Next(nCtrlOctets);
    count += nCtrlOctets;
    m_linkMapping.clear();
    if (m_control.presenceBitmap.has_value())
    {
        NS_ABORT_MSG_IF(m_control.defaultMapping,
                        "Default mapping should not be set when presence bitmap is present");
        const auto presenceBitmap = m_control.presenceBitmap.value();
        for (uint8_t tid = 0; tid < 8; tid++)
        {
            if (((presenceBitmap >> tid) & 0x01) == 1)
            {
                m_linkMapping[tid] = i.ReadLsbtohU16();
                count += 2;
            }
        }
    }

    NS_ABORT_MSG_IF(count != length,
                    "TID-to-Link Mapping Length (" << +length
                                                   << ") differs "
                                                      "from actual number of bytes read ("
                                                   << +count << ")");
    return count;
}

} // namespace ns3
