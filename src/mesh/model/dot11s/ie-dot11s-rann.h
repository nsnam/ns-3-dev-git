/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Kirill Andreev <andreev@iitp.ru>
 */

#ifndef RANN_INFORMATION_ELEMENT_H
#define RANN_INFORMATION_ELEMENT_H

#include "ns3/mac48-address.h"
#include "ns3/mesh-information-element-vector.h"

namespace ns3
{
namespace dot11s
{
/**
 * @ingroup dot11s
 * @brief Root announcement (RANN) element
 */
class IeRann : public WifiInformationElement
{
  public:
    IeRann();
    ~IeRann() override;
    /**
     * Set flags field
     * @param flags the flags value to set
     */
    void SetFlags(uint8_t flags);
    /**
     * Set hop count value to number of hops from the originating root mesh
     * STA to the mesh STA transmitting this element
     * @param hopcount the hop count
     */
    void SetHopcount(uint8_t hopcount);
    /**
     * Set TTL value to the remaining number of hops allowed
     * @param ttl the TTL
     */
    void SetTTL(uint8_t ttl);
    /**
     * Set originator address value
     * @param originator_address the originator MAC address
     */
    void SetOriginatorAddress(Mac48Address originator_address);
    /**
     * Set destination sequence number value
     * @param dest_seq_number the destination sequence number
     */
    void SetDestSeqNumber(uint32_t dest_seq_number);
    /**
     * Set metric value to cumulative metric from originating root mesh STA
     * to the mesh STA transmitting the announcement
     * @param metric the metric to set
     */
    void SetMetric(uint32_t metric);
    /**
     * Get flags value
     * @returns the flags
     */
    uint8_t GetFlags() const;
    /**
     * Get hop count value
     * @returns the hop count
     */
    uint8_t GetHopcount() const;
    /**
     * Get TTL value
     * @returns the TTL
     */
    uint8_t GetTtl() const;
    /**
     * Get originator address value
     * @returns the MAC address of the originator
     */
    Mac48Address GetOriginatorAddress();
    /**
     * Get destination sequence number value
     * @returns the destination sequence number
     */
    uint32_t GetDestSeqNumber() const;
    /**
     * Get metric value
     * @returns the metric
     */
    uint32_t GetMetric() const;
    /// Decrement TTL function
    void DecrementTtl();
    /**
     * Increment metric
     * @param metric the value to increment
     */
    void IncrementMetric(uint32_t metric);

    // Inherited from WifiInformationElement
    WifiInformationElementId ElementId() const override;
    void SerializeInformationField(Buffer::Iterator i) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
    uint16_t GetInformationFieldSize() const override;
    void Print(std::ostream& os) const override;

  private:
    uint8_t m_flags;                  ///< flags
    uint8_t m_hopcount;               ///< hop count
    uint8_t m_ttl;                    ///< TTL
    Mac48Address m_originatorAddress; ///< originator address
    uint32_t m_destSeqNumber;         ///< destination sequence number
    uint32_t m_metric;                ///< metric

    /**
     * equality operator
     *
     * @param a lhs
     * @param b rhs
     * @returns true if equal
     */
    friend bool operator==(const IeRann& a, const IeRann& b);
};

bool operator==(const IeRann& a, const IeRann& b);
std::ostream& operator<<(std::ostream& os, const IeRann& rann);
} // namespace dot11s
} // namespace ns3

#endif
