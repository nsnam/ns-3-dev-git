/*
 * Copyright (c) 2011,2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Marco Miozzo  <marco.miozzo@cttc.es>
 *         Nicola Baldo <nbaldo@cttc.es>
 */
#ifndef EPS_BEARER_TAG_H
#define EPS_BEARER_TAG_H

#include "ns3/tag.h"

namespace ns3
{

class Tag;

/**
 * Tag used to define the RNTI and EPS bearer ID for packets
 * interchanged between the EpcEnbApplication and the LteEnbNetDevice
 */

class EpsBearerTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * Create an empty EpsBearerTag
     */
    EpsBearerTag();

    /**
     * Create a EpsBearerTag with the given RNTI and bearer id
     *
     * @param rnti the value of the RNTI to set
     * @param bid the value of the Bearer Id to set
     */
    EpsBearerTag(uint16_t rnti, uint8_t bid);

    /**
     * Set the RNTI to the given value.
     *
     * @param rnti the value of the RNTI to set
     */
    void SetRnti(uint16_t rnti);

    /**
     * Set the bearer id to the given value.
     *
     * @param bid the value of the Bearer Id to set
     */
    void SetBid(uint8_t bid);

    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

    /**
     * Get RNTI function
     * @returns the RNTI
     */
    uint16_t GetRnti() const;
    /**
     * Get Bearer Id function
     * @returns the Bearer Id
     */
    uint8_t GetBid() const;

  private:
    uint16_t m_rnti; ///< RNTI value
    uint8_t m_bid;   ///< Bearer Id value
};

} // namespace ns3

#endif /* EPS_BEARER_TAG_H */
