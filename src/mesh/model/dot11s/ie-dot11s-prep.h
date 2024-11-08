/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef WIFI_PREP_INFORMATION_ELEMENT_H
#define WIFI_PREP_INFORMATION_ELEMENT_H

#include "ns3/mac48-address.h"
#include "ns3/mesh-information-element-vector.h"

namespace ns3
{
namespace dot11s
{
/**
 * @ingroup dot11s
 * @brief See 7.3.2.97 of 802.11s draft 2.07
 */
class IePrep : public WifiInformationElement
{
  public:
    IePrep();
    ~IePrep() override;
    /**
     * Set flags function
     * @param flags the flags to set
     */
    void SetFlags(uint8_t flags);
    /**
     * Set hop count function
     * @param hopcount the hop count
     */
    void SetHopcount(uint8_t hopcount);
    /**
     * Set TTL function
     * @param ttl the TTL
     */
    void SetTtl(uint8_t ttl);
    /**
     * Set destination address function
     * @param dest_address
     */
    void SetDestinationAddress(Mac48Address dest_address);
    /**
     * Set destination sequence number function
     * @param dest_seq_number the destination sequence number
     */
    void SetDestinationSeqNumber(uint32_t dest_seq_number);
    /**
     * Set lifetime function
     * @param lifetime the lifetime
     */
    void SetLifetime(uint32_t lifetime);
    /**
     * Set metric function
     * @param metric the metric to set
     */
    void SetMetric(uint32_t metric);
    /**
     * Set originator address function
     * @param originator_address the originator address
     */
    void SetOriginatorAddress(Mac48Address originator_address);
    /**
     * Set originator sequence number function
     * @param originator_seq_number the originator sequence number
     */
    void SetOriginatorSeqNumber(uint32_t originator_seq_number);

    /**
     * Get flags function
     * @returns the flags
     */
    uint8_t GetFlags() const;
    /**
     * Get hop count function
     * @returns the hop count
     */
    uint8_t GetHopcount() const;
    /**
     * Get TTL function
     * @returns the TTL
     */
    uint32_t GetTtl() const;
    /**
     * Get destination address function
     * @returns the destination MAC address
     */
    Mac48Address GetDestinationAddress() const;
    /**
     * Get destination sequence number function
     * @returns the destination sequence number
     */
    uint32_t GetDestinationSeqNumber() const;
    /**
     * Get lifetime function
     * @returns the lifetime
     */
    uint32_t GetLifetime() const;
    /**
     * Get metric function
     * @returns the metric
     */
    uint32_t GetMetric() const;
    /**
     * Get originator address function
     * @returns the MAC address of the originator
     */
    Mac48Address GetOriginatorAddress() const;
    /**
     * Get originator sequence number
     * @returns the originator sequence number
     */
    uint32_t GetOriginatorSeqNumber() const;

    /// Decrement TTL function
    void DecrementTtl();
    /**
     * Increment metric function
     * @param metric the metric to increment
     */
    void IncrementMetric(uint32_t metric);

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    uint16_t GetInformationFieldSize() const override;
    void Print(std::ostream& os) const override;

  private:
    uint8_t m_flags;                   ///< flags
    uint8_t m_hopcount;                ///< hop count
    uint8_t m_ttl;                     ///< TTL
    Mac48Address m_destinationAddress; ///< destination address
    uint32_t m_destSeqNumber;          ///< destination sequence number
    uint32_t m_lifetime;               ///< lifetime
    uint32_t m_metric;                 ///< metric
    Mac48Address m_originatorAddress;  ///< originator address
    uint32_t m_originatorSeqNumber;    ///< originator sequence number
    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const IePrep& a, const IePrep& b);
};

bool operator==(const IePrep& a, const IePrep& b);
std::ostream& operator<<(std::ostream& os, const IePrep& prep);
} // namespace dot11s
} // namespace ns3
#endif
