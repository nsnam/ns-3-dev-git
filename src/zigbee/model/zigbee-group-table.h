/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_GROUP_TABLE_H
#define ZIGBEE_GROUP_TABLE_H

#include "ns3/simple-ref-count.h"

#include <bitset>
#include <unordered_map>
#include <vector>

namespace ns3
{
namespace zigbee
{

/**
 * @ingroup zigbee
 * The Zigbee Group Table
 * Zigbee Specification r22.1.0, Section 2.2.8.3 and 3.6.6.1
 * The group table is a special table that is accessible by both the
 * Zigbee NWK and APS layers. It is used to store group IDs and associated endpoints.
 * The group table is used in GroupCasting operations (A type of multicast in Zigbee).
 * In this implementation, the group table is represented as a map where the key is the group ID
 * and the value is a bitset representing the endpoints associated with that group ID.
 * Each bit in the bitset corresponds to an endpoint, where the index of the bit represents the
 * endpoint number.
 */
class ZigbeeGroupTable : public SimpleRefCount<ZigbeeGroupTable>
{
  public:
    /**
     * Constructor for Zigbee group table.
     */
    ZigbeeGroupTable();

    /**
     * Destructor for Zigbee group table.
     */
    ~ZigbeeGroupTable();

    /**
     * Add a group ID and its related endpoint. If the group ID already exists,
     * the endpoint is added to the existing group ID entry.
     *
     * @param groupId The group ID to add.
     * @param endPoint The endpoint to associate with the group ID.
     * @return True if the entry was added successfully or the endpoint is already a member of the
     * group, false if the table is full.
     */
    bool AddEntry(uint16_t groupId, uint8_t endPoint);

    /**
     * Remove endpoint from a group. If the endpoint is the last one
     * associated with the group ID, the group ID entry is removed.
     *
     * @param groupId The group ID of the group to remove the endpoint from.
     * @param endPoint The endpoint to remove from the group ID.
     * @return True if the entry was removed successfully, false if the group does not exist, or the
     * endpoint is not a member of the group.
     */
    bool RemoveEntry(uint16_t groupId, uint8_t endPoint);

    /**
     * Remove the endPoint from all groups.
     *
     * @param endPoint The endpoint to remove from all groups. If the removed endpoint is the last
     * one associated with the group ID, the group ID entry is also removed.
     * @return True if the endpoint was remmoved from at least one group, false if the endpoint was
     * not found in any group.
     *
     */
    bool RemoveMembership(uint8_t endPoint);

    /**
     * Indicates whether the group ID exists in the group table.
     *
     * @param groupId The group ID to query.
     * @return True if the group ID exists, false otherwise.
     */
    bool IsGroupMember(uint16_t groupId) const;

    /**
     * Look up the endpoints associated with a given group ID.
     *
     * @param groupId The group ID to look up.
     * @param endPoints A vector to store the endpoints associated with the group ID.
     * @return True if the group ID was found and endpoints were retrieved, false otherwise.
     */
    bool LookUpEndPoints(uint16_t groupId, std::vector<uint8_t>& endPoints) const;

  private:
    static constexpr int MAX_GROUP_ID_ENTRIES{256}; //!< The maximum amount of group ID
                                                    //!< entries allowed in the table.
    static constexpr int MAX_ENDPOINT_ENTRIES{256}; //!< The maximum amount of endpoints
                                                    //!< allowed per group id entry.
    std::unordered_map<uint16_t, std::bitset<MAX_ENDPOINT_ENTRIES>>
        m_groupTable; //!< The group table object
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_GROUP_TABLE_H */
