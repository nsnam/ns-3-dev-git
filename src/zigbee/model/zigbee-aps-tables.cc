/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#include "zigbee-aps-tables.h"

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/simulator.h"

#include <algorithm>
#include <iomanip>

namespace ns3
{
namespace zigbee
{

NS_LOG_COMPONENT_DEFINE("ZigbeeApsTables");

/***********************************************************
 *                Source Binding Entry
 ***********************************************************/

SrcBindingEntry::SrcBindingEntry()
{
}

SrcBindingEntry::SrcBindingEntry(Mac64Address address, uint8_t endPoint, uint16_t clusterId)
{
    m_srcAddr = address;
    m_srcEndPoint = endPoint;
    m_clusterId = clusterId;
}

SrcBindingEntry::~SrcBindingEntry()
{
}

void
SrcBindingEntry::SetSrcAddress(Mac64Address address)
{
    m_srcAddr = address;
}

void
SrcBindingEntry::SetSrcEndPoint(uint8_t endPoint)
{
    m_srcEndPoint = endPoint;
}

void
SrcBindingEntry::SetClusterId(uint16_t clusterId)
{
    m_clusterId = clusterId;
}

Mac64Address
SrcBindingEntry::GetSrcAddress() const
{
    return m_srcAddr;
}

uint8_t
SrcBindingEntry::GetSrcEndPoint() const
{
    return m_srcEndPoint;
}

uint16_t
SrcBindingEntry::GetClusterId() const
{
    return m_clusterId;
}

/***********************************************************
 *                Destination Binding Entry
 ***********************************************************/

DstBindingEntry::DstBindingEntry()
{
}

DstBindingEntry::~DstBindingEntry()
{
}

void
DstBindingEntry::SetDstAddrMode(ApsDstAddressModeBind mode)
{
    m_dstAddrMode = mode;
}

void
DstBindingEntry::SetDstAddr16(Mac16Address address)
{
    m_dstAddr16 = address;
}

void
DstBindingEntry::SetDstAddr64(Mac64Address address)
{
    m_dstAddr64 = address;
}

void
DstBindingEntry::SetDstEndPoint(uint8_t endPoint)
{
    m_dstEndPoint = endPoint;
}

ApsDstAddressModeBind
DstBindingEntry::GetDstAddrMode() const
{
    return m_dstAddrMode;
}

Mac16Address
DstBindingEntry::GetDstAddr16() const
{
    return m_dstAddr16;
}

Mac64Address
DstBindingEntry::GetDstAddr64() const
{
    return m_dstAddr64;
}

uint8_t
DstBindingEntry::GetDstEndPoint() const
{
    return m_dstEndPoint;
}

/***********************************************************
 *                Binding Table
 ***********************************************************/

BindingTable::BindingTable()
{
    m_maxSrcEntries = 10;
    m_maxDstEntries = 10;
}

bool
BindingTable::CompareDestinations(const DstBindingEntry& first, const DstBindingEntry& second)
{
    if (first.GetDstAddrMode() == ApsDstAddressModeBind::GROUP_ADDR_DST_ENDPOINT_NOT_PRESENT)
    {
        // Group Addressing
        if ((first.GetDstAddr16() == second.GetDstAddr16()) &&
            (first.GetDstEndPoint() == second.GetDstEndPoint()))
        {
            return true;
        }
    }
    else if (first.GetDstAddrMode() == ApsDstAddressModeBind::DST_ADDR64_DST_ENDPOINT_PRESENT)
    {
        // IEEE Addressing
        if ((first.GetDstAddr64() == second.GetDstAddr64()) &&
            (first.GetDstEndPoint() == second.GetDstEndPoint()))
        {
            return true;
        }
    }
    return false;
}

bool
BindingTable::CompareSources(const SrcBindingEntry& first, const SrcBindingEntry& second)
{
    return ((first.GetSrcAddress() == second.GetSrcAddress()) &&
            (first.GetSrcEndPoint() == second.GetSrcEndPoint()) &&
            (first.GetClusterId() == second.GetClusterId()));
}

BindingTableStatus
BindingTable::Bind(const SrcBindingEntry& src, const DstBindingEntry& dst)
{
    for (auto& entry : m_bindingTable)
    {
        if (CompareSources(src, entry.first))
        {
            // The source exist, now check if the destination exist
            for (const auto& destination : entry.second)
            {
                if (CompareDestinations(dst, destination))
                {
                    NS_LOG_WARN("Entry already exist in binding table");
                    return BindingTableStatus::ENTRY_EXISTS;
                }
            }
            // Add the new destination bound to the source
            if (entry.second.size() >= m_maxDstEntries)
            {
                NS_LOG_WARN("Binding Table full, max destination entries (" << m_maxDstEntries
                                                                            << ") reached");
                return BindingTableStatus::TABLE_FULL;
            }
            else
            {
                entry.second.emplace_back(dst);
                return BindingTableStatus::BOUND;
            }
        }
    }

    if (m_bindingTable.size() >= m_maxSrcEntries)
    {
        NS_LOG_WARN("Binding Table full, max source entries (" << m_maxSrcEntries << ") reached");
        return BindingTableStatus::TABLE_FULL;
    }
    else
    {
        // New source with its first destination
        m_bindingTable.emplace_back(src, std::vector<DstBindingEntry>{dst});
        return BindingTableStatus::BOUND;
    }
}

BindingTableStatus
BindingTable::Unbind(const SrcBindingEntry& src, const DstBindingEntry& dst)
{
    for (auto it = m_bindingTable.begin(); it != m_bindingTable.end(); ++it)
    {
        if (CompareSources(src, it->first))
        {
            // The source exists, now check if the destination exists
            auto& destinations = it->second;
            for (auto destIt = destinations.begin(); destIt != destinations.end(); ++destIt)
            {
                if (CompareDestinations(dst, *destIt))
                {
                    // Destination found, remove it
                    destinations.erase(destIt);

                    // If no destinations left, remove the source entry
                    if (destinations.empty())
                    {
                        m_bindingTable.erase(it);
                    }
                    // Successfully unbound
                    return BindingTableStatus::UNBOUND;
                }
            }
            // Destination not found
            NS_LOG_WARN("Cannot unbind, destination entry do not exist");
            return BindingTableStatus::ENTRY_NOT_FOUND;
        }
    }
    // Source not found
    NS_LOG_WARN("Cannot unbind, source entry do not exist");
    return BindingTableStatus::ENTRY_NOT_FOUND;
}

bool
BindingTable::LookUpEntries(const SrcBindingEntry& src, std::vector<DstBindingEntry>& dstEntries)
{
    for (auto& entry : m_bindingTable)
    {
        if (CompareSources(src, entry.first))
        {
            // The source entry exist, return all the dst entries.
            dstEntries = entry.second;
            return true;
        }
    }
    return false;
}

} // namespace zigbee
} // namespace ns3
