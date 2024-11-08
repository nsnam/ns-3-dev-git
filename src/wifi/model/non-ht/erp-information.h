/*
 * Copyright (c) 2015 Sébastien Deronne
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef ERP_INFORMATION_H
#define ERP_INFORMATION_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * @brief The ErpInformation Information Element
 * @ingroup wifi
 *
 * This class knows how to serialise and deserialise the ErpInformation Information Element.
 */
class ErpInformation : public WifiInformationElement
{
  public:
    ErpInformation();

    // Implementations of pure virtual methods of WifiInformationElement
    WifiInformationElementId ElementId() const override;
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    /**
     * Set the Barker_Preamble_Mode field in the ErpInformation information element.
     *
     * @param barkerPreambleMode the Barker_Preamble_Mode field in the ErpInformation information
     * element
     */
    void SetBarkerPreambleMode(uint8_t barkerPreambleMode);
    /**
     * Set the Use_Protection field in the ErpInformation information element.
     *
     * @param useProtection the Use_Protection field in the ErpInformation information element
     */
    void SetUseProtection(uint8_t useProtection);
    /**
     * Set the Non_Erp_Present field in the ErpInformation information element.
     *
     * @param nonErpPresent the Non_Erp_Present field in the ErpInformation information element
     */
    void SetNonErpPresent(uint8_t nonErpPresent);

    /**
     * Return the Barker_Preamble_Mode field in the ErpInformation information element.
     *
     * @return the Barker_Preamble_Mode field in the ErpInformation information element
     */
    uint8_t GetBarkerPreambleMode() const;
    /**
     * Return the Use_Protection field in the ErpInformation information element.
     *
     * @return the Use_Protection field in the ErpInformation information element
     */
    uint8_t GetUseProtection() const;
    /**
     * Return the Non_Erp_Present field in the ErpInformation information element.
     *
     * @return the Non_Erp_Present field in the ErpInformation information element
     */
    uint8_t GetNonErpPresent() const;

  private:
    uint8_t m_erpInformation; ///< ERP information
};

/**
 * output stream output operator
 *
 * @param os output stream
 * @param erpInformation the ERP Information
 *
 * @returns output stream
 */
std::ostream& operator<<(std::ostream& os, const ErpInformation& erpInformation);

} // namespace ns3

#endif /* ERP_INFORMATION_H */
