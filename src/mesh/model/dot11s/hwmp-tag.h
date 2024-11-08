/*
 * Copyright (c) 2008,2009 IITP RAS
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Authors: Kirill Andreev <andreev@iitp.ru>
 *          Aleksey Kovalenko <kovalenko@iitp.ru>
 *          Pavel Boyko <boyko@iitp.ru>
 */

#ifndef HWMP_TAG_H
#define HWMP_TAG_H

#include "ns3/mac48-address.h"
#include "ns3/object.h"
#include "ns3/tag.h"

namespace ns3
{
namespace dot11s
{
/**
 * @ingroup dot11s
 *
 * @brief Hwmp tag implements interaction between HWMP
 * protocol and MeshWifiMac
 *
 * Hwmp tag keeps the following:
 * 1. When packet is passed from Hwmp to 11sMAC:
 *  - retransmitter address,
 *  - TTL value,
 * 2. When packet is passed to Hwmp from 11sMAC:
 *  - lasthop address,
 *  - TTL value,
 *  - metric value (metric of link is recalculated
 *  at each packet, but routing table stores metric
 *  obtained during path discovery procedure)
 */
class HwmpTag : public Tag
{
  public:
    HwmpTag();
    ~HwmpTag() override;
    /**
     * Set address
     * @param retransmitter the MAC address of the retransmitter
     */
    void SetAddress(Mac48Address retransmitter);
    /**
     * Get address from tag
     * @return the MAC address
     */
    Mac48Address GetAddress();
    /**
     * Set the TTL value
     * @param ttl
     */
    void SetTtl(uint8_t ttl);
    /**
     * Get the TTL value
     * @returns the TTL
     */
    uint8_t GetTtl() const;
    /**
     * Set the metric value
     * @param metric the metric
     */
    void SetMetric(uint32_t metric);
    /**
     * Get the metric value
     * @returns the metric
     */
    uint32_t GetMetric() const;
    /**
     * Set sequence number
     * @param seqno the sequence number
     */
    void SetSeqno(uint32_t seqno);
    /**
     * Get the sequence number
     * @returns the sequence number
     */
    uint32_t GetSeqno() const;
    /// Decrement TTL
    void DecrementTtl();

    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;
    uint32_t GetSerializedSize() const override;
    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    void Print(std::ostream& os) const override;

  private:
    Mac48Address m_address; ///< address
    uint8_t m_ttl;          ///< TTL
    uint32_t m_metric;      ///< metric
    uint32_t m_seqno;       ///< sequence no
};
} // namespace dot11s
} // namespace ns3
#endif
