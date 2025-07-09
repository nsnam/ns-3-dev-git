/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-group-table.h"

#include "ns3/log.h"

namespace ns3
{
namespace zigbee
{

NS_LOG_COMPONENT_DEFINE("ZigbeeGroupTable");

ZigbeeGroupTable::ZigbeeGroupTable() = default;

ZigbeeGroupTable::~ZigbeeGroupTable() = default;

bool
ZigbeeGroupTable::AddEntry(uint16_t groupId, uint8_t endPoint)
{
    NS_LOG_FUNCTION(this << groupId << static_cast<uint16_t>(endPoint));

    if (m_groupTable.size() == MAX_GROUP_ID_ENTRIES)
    {
        NS_LOG_WARN("Group table is full, cannot add group ID " << groupId);
        return false; // too many groups
    }

    // Check the group entry existence, if it does not exist, create it.
    auto& group = m_groupTable[groupId];
    if (group[endPoint])
    {
        NS_LOG_INFO("Endpoint " << static_cast<uint16_t>(endPoint) << " already exists in group "
                                << groupId);
        return false; // endPoint already in group
    }

    NS_LOG_INFO("Adding endpoint " << static_cast<uint16_t>(endPoint) << " to group " << groupId);
    group.set(endPoint);
    return true;
}

bool
ZigbeeGroupTable::RemoveEntry(uint16_t groupId, uint8_t endPoint)
{
    NS_LOG_FUNCTION(this << groupId << static_cast<uint16_t>(endPoint));

    auto group = m_groupTable.find(groupId);
    auto endPoints = group->second;

    if (group == m_groupTable.end())
    {
        NS_LOG_WARN("Group ID " << groupId << " does not exist, cannot remove endpoint "
                                << static_cast<uint16_t>(endPoint));
        return false; // group doesn't exist
    }

    if (!endPoints[endPoint])
    {
        NS_LOG_WARN("Endpoint " << static_cast<uint16_t>(endPoint) << " not found in group "
                                << groupId);
        return false; // endpoint not in this group
    }
    else
    {
        endPoints.reset(endPoint); // Turn bit in bitset off (0)
        NS_LOG_INFO("Removed endpoint " << static_cast<uint16_t>(endPoint) << " from group "
                                        << groupId);

        if (endPoints.none())
        {
            m_groupTable.erase(group); // remove group if no endpoints left
        }
        return true; // endpoint removed successfully
    }
}

bool
ZigbeeGroupTable::RemoveMembership(uint8_t endPoint)
{
    NS_LOG_FUNCTION(this << static_cast<uint16_t>(endPoint));

    bool removed = false;
    for (auto group = m_groupTable.begin(); group != m_groupTable.end(); ++group)
    {
        if (group->second[endPoint])
        {
            group->second.reset(endPoint); // Turn bit in bitset off (0)
            NS_LOG_INFO("Removed endpoint " << static_cast<uint16_t>(endPoint) << " from group "
                                            << group->first);

            if (group->second.none())
            {
                m_groupTable.erase(group); // remove group if no endpoints left
            }
            removed = true; // endpoint removed successfully
        }
    }
    return removed;
}

bool
ZigbeeGroupTable::IsGroupMember(uint16_t groupId) const
{
    return m_groupTable.find(groupId) != m_groupTable.end();
}

bool
ZigbeeGroupTable::LookUpEndPoints(uint16_t groupId, std::vector<uint8_t>& endPoints) const
{
    bool hasEndPoints = false;
    auto group = m_groupTable.find(groupId);

    if (group == m_groupTable.end())
    {
        NS_LOG_WARN("Group ID " << groupId << " does not exist, cannot look up endpoints");
        return false; // group doesn't exist
    }

    for (auto endpoint = 0; endpoint < MAX_ENDPOINT_ENTRIES; ++endpoint)
    {
        if (group->second[endpoint])
        {
            endPoints.push_back(endpoint);
            hasEndPoints = true;
        }
    }
    return hasEndPoints;
}

} // namespace zigbee
} // namespace ns3
