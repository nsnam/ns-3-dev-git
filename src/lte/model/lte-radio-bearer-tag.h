/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Author: Marco Miozzo  <marco.miozzo@cttc.es>
 */
#ifndef LTE_RADIO_BEARER_TAG_H
#define LTE_RADIO_BEARER_TAG_H

#include "ns3/tag.h"

namespace ns3
{

class Tag;

/**
 * Tag used to define the RNTI and LC id for each MAC packet transmitted
 */

class LteRadioBearerTag : public Tag
{
  public:
    /**
     * @brief Get the type ID.
     * @return the object TypeId
     */
    static TypeId GetTypeId();
    TypeId GetInstanceTypeId() const override;

    /**
     * Create an empty LteRadioBearerTag
     */
    LteRadioBearerTag();

    /**
     * Create a LteRadioBearerTag with the given RNTI and LC id
     * @param rnti the RNTI
     * @param lcId the LCID
     */
    LteRadioBearerTag(uint16_t rnti, uint8_t lcId);

    /**
     * Create a LteRadioBearerTag with the given RNTI, LC id and layer
     * @param rnti the RNTI
     * @param lcId the LCID
     * @param layer the layer
     */
    LteRadioBearerTag(uint16_t rnti, uint8_t lcId, uint8_t layer);

    /**
     * Set the RNTI to the given value.
     *
     * @param rnti the value of the RNTI to set
     */
    void SetRnti(uint16_t rnti);

    /**
     * Set the LC id to the given value.
     *
     * @param lcid the value of the RNTI to set
     */
    void SetLcid(uint8_t lcid);

    /**
     * Set the layer id to the given value.
     *
     * @param layer the value of the layer to set
     */
    void SetLayer(uint8_t layer);

    void Serialize(TagBuffer i) const override;
    void Deserialize(TagBuffer i) override;
    uint32_t GetSerializedSize() const override;
    void Print(std::ostream& os) const override;

    /**
     * Get RNTI function
     *
     * @returns RNTI
     */
    uint16_t GetRnti() const;
    /**
     * Get LCID function
     *
     * @returns LCID
     */
    uint8_t GetLcid() const;
    /**
     * Get layer function
     *
     * @returns layer
     */
    uint8_t GetLayer() const;

  private:
    uint16_t m_rnti; ///< RNTI
    uint8_t m_lcid;  ///< LCID
    uint8_t m_layer; ///< layer
};

} // namespace ns3

#endif /* LTE_RADIO_BEARER_TAG_H */
