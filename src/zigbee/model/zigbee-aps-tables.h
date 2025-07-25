/*
 * Copyright (c) 2025 Tokushima University, Japan
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author:
 *  Alberto Gallegos Ramonet <alramonet@is.tokushima-u.ac.jp>
 */

#ifndef ZIGBEE_APS_TABLES_H
#define ZIGBEE_APS_TABLES_H

#include "ns3/mac16-address.h"
#include "ns3/mac64-address.h"
#include "ns3/output-stream-wrapper.h"
#include "ns3/timer.h"

#include <cassert>
#include <deque>
#include <stdint.h>
#include <sys/types.h>

namespace ns3
{
namespace zigbee
{

/**
 * @ingroup Zigbee
 *
 * APS Destination Address Mode for Binding
 * Zigbee Specification r22.1.0, Table 2-6
 * APSME-BIND.request Parameters
 */
enum class ApsDstAddressModeBind : std::uint8_t
{
    GROUP_ADDR_DST_ENDPOINT_NOT_PRESENT = 0x01,
    DST_ADDR64_DST_ENDPOINT_PRESENT = 0x03
};

/**
 * @ingroup Zigbee
 *
 * The status resulting of interactions with the binding table.
 */
enum class BindingTableStatus : std::uint8_t
{
    BOUND = 0,
    UNBOUND = 1,
    TABLE_FULL = 2,
    ENTRY_EXISTS = 3,
    ENTRY_NOT_FOUND = 4
};

/**
 * @ingroup Zigbee
 *
 * Binding Table entry: Source portion of the table.
 * As described in Zigbee Specification r22.1.0, Table 2-134
 */
class SrcBindingEntry : public SimpleRefCount<SrcBindingEntry>
{
  public:
    /**
     * The default constructor of the source binding entry.
     */
    SrcBindingEntry();

    /**
     * Constructor of the source binding entry.
     *
     * @param address The IEEE address to register, (typically the IEEE address of the source
     * device).
     * @param endPoint The source endpoint to register to the entry.
     * @param clusterId The cluster ID to register to the entry.
     */
    SrcBindingEntry(Mac64Address address, uint8_t endPoint, uint16_t clusterId);
    ~SrcBindingEntry();

    /**
     * Set the source IEEE address to the entry.
     *
     * @param address The IEEE address (64-bit address) of the entry
     */
    void SetSrcAddress(Mac64Address address);

    /**
     * Set the source endpoint of the source binding entry.
     * @param endPoint The source endpoint to set in the entry.
     */
    void SetSrcEndPoint(uint8_t endPoint);

    /**
     * Set the cluster ID of the source binding entry.
     * @param clusterId  The cluster ID to set in the entry.
     */
    void SetClusterId(uint16_t clusterId);

    /**
     * Get the IEEE address from the source binding entry.
     * @return The IEEE address in the source binding entry.
     */
    Mac64Address GetSrcAddress() const;

    /**
     * Get the source endpoint from the source binding entry.
     *
     * @return The source endpoint.
     */
    uint8_t GetSrcEndPoint() const;

    /**
     * Get the cluster ID from the source binding entry.
     * @return The cluster ID.
     */
    uint16_t GetClusterId() const;

  private:
    Mac64Address m_srcAddr;   //!< The source IEEE address in the source entry.
    uint8_t m_srcEndPoint{0}; //!< The source endpoint in the source entry.
    uint16_t m_clusterId{0};  //!< The cluster ID in the source entry.
};

/**
 * Binding Table entry: Destination portion of the table.
 * As described in Zigbee Specification r22.1.0, Table 2-134
 */
class DstBindingEntry : public SimpleRefCount<DstBindingEntry>
{
  public:
    /**
     * The default constructor of the destination binding entry.
     */
    DstBindingEntry();
    ~DstBindingEntry();

    /**
     * Set the destination address mode of the destination binding entry.
     * @param mode The destination address mode to set.
     */
    void SetDstAddrMode(ApsDstAddressModeBind mode);

    /**
     * Set the destination 16-bit address of the destination binding entry.
     * @param address The 16-bit address of the destination binding entry.
     */
    void SetDstAddr16(Mac16Address address);

    /**
     * Set the destination IEEE Address (64-bit address) of the destination binding entry.
     * @param address  The destination IEEE address (64-bit address) to set
     */
    void SetDstAddr64(Mac64Address address);

    /**
     * Set the destination endppoint to the destination binding entry.
     * @param endPoint The destination endpoint to set.
     */
    void SetDstEndPoint(uint8_t endPoint);

    /**
     * Get the destination address mode used by the destination entry.
     * @return The destination address mode used by the entry.
     */
    ApsDstAddressModeBind GetDstAddrMode() const;

    /**
     * Get the 16-bit address destination of the destination entry.
     * @return The 16-bit address of the destination entry.
     */
    Mac16Address GetDstAddr16() const;

    /**
     * Get the 64-bit address destination of the destination entry.
     * @return The  IEEE address (64-bit address) destination
     */
    Mac64Address GetDstAddr64() const;

    /**
     * Get the destination endpoint of the destination entry.
     * @return The destination endpoint.
     */
    uint8_t GetDstEndPoint() const;

  private:
    ApsDstAddressModeBind m_dstAddrMode{
        ApsDstAddressModeBind::DST_ADDR64_DST_ENDPOINT_PRESENT}; //!< The destination address mode
                                                                 //!< used by the entry.
    Mac16Address m_dstAddr16; //!< The destination 16-bit address in the destination entry.
    Mac64Address
        m_dstAddr64; //!< The destination IEEE address (64-bit address) in the destination entry.
    uint8_t m_dstEndPoint{0xF0}; //!< The destination endpoint in the destination entry.
};

/**
 * APS Binding Table
 * See Zigbee specification r22.1.0, Table 2-134
 * Similar to the z-boss implementation, the binding table is divided in two portions:
 * The source part and the destination part. A single source can have multiple destination
 * entries as described by the Zigbee specification. This creates a relationship one to many
 * (a source entry with multiple destination entries) which is both useful for 64 bit Address UCST
 * or 16-bit groupcast destination bindings.
 */
class BindingTable
{
  public:
    /**
     *  The constructor of the binding table.
     */
    BindingTable();

    /**
     * Add an entry to the binding table.
     * In essence it binds the source entry portion to the table to one or more
     * destination portion entries (one to many).
     *
     * @param src The source entry portion of the table.
     * @param dst The destination entry portion of the table.
     * @return The resulting status of the binding attempt.
     */
    BindingTableStatus Bind(const SrcBindingEntry& src, const DstBindingEntry& dst);

    /**
     * Unbinds a destination entry portion of a binding table from a source entry portion.
     *
     * @param src The source entry portion of the table.
     * @param dst The destination entry portion of the table.
     * @return The resulting status of the unbinding attempt.
     */
    BindingTableStatus Unbind(const SrcBindingEntry& src, const DstBindingEntry& dst);

    /**
     * Look for destination entries binded to an specific source entry portion in the binding
     * table.
     *
     * @param src The source entry portion of the table to search.
     * @param dstEntries The resulting destination entries binded to the provided source entry
     * portion
     * @return True if at least one destination entry portion was retrieved.
     */
    bool LookUpEntries(const SrcBindingEntry& src, std::vector<DstBindingEntry>& dstEntries);

  private:
    /**
     * Compare the equality of 2 source entries
     *
     * @param first The first source entry to compare.
     * @param second The second source entry to compare.
     * @return True if the destinations entries are identical
     */
    bool CompareSources(const SrcBindingEntry& first, const SrcBindingEntry& second);

    /**
     * Compare the equality of 2 destination entries
     *
     * @param first The first destination entry to compare.
     * @param second The second destination entry to compare.
     * @return True if the destinations entries are identical
     */
    bool CompareDestinations(const DstBindingEntry& first, const DstBindingEntry& second);

    std::vector<std::pair<SrcBindingEntry,
                          std::vector<DstBindingEntry>>>
        m_bindingTable;      //!< The binding table object
    uint8_t m_maxSrcEntries; //!< The maximum amount of source entries allowed in the table
    uint8_t m_maxDstEntries; //!< The maximum amount of destination entries allowed in the table
};

} // namespace zigbee
} // namespace ns3

#endif /* ZIGBEE_APS_TABLES_H */
