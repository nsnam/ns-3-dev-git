/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_RLC_SDU_STATUS_TAG_H
#define LTE_RLC_SDU_STATUS_TAG_H

#include "ns3/tag.h"

namespace ns3
{

/**
 * @brief This class implements a tag that carries the status of a RLC SDU
 * for the fragmentation process
 * Status of RLC SDU
 */
class LteRlcSduStatusTag : public Tag
{
  public:
    LteRlcSduStatusTag();

    /**
     * Set status function
     *
     * @param status the status
     */
    void SetStatus(uint8_t status);
    /**
     * Get status function
     *
     * @returns the status
     */
    uint8_t GetStatus() const;

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

    /// SduStatus_t enumeration
    enum SduStatus_t
    {
        FULL_SDU = 1,
        FIRST_SEGMENT = 2,
        MIDDLE_SEGMENT = 3,
        LAST_SEGMENT = 4,
        ANY_SEGMENT = 5
    };

  private:
    uint8_t m_sduStatus; ///< SDU status
};

}; // namespace ns3

#endif // LTE_RLC_SDU_STATUS_TAG_H
