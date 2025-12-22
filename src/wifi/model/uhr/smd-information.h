/*
 * Copyright (c) 2025 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef SMD_INFORMATION_H
#define SMD_INFORMATION_H

#include "ns3/mac48-address.h"
#include "ns3/wifi-export.h"
#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * @brief SMD Information element
 * @ingroup wifi
 *
 * This class serializes and deserializes the SMD Information element defined in IEEE 802.11bn D2.0
 * Section 9.4.2.363.
 */
class WIFI_EXPORT SmdInformation : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    /**
     * Set the preparation timeout
     *
     * @param prepTimeout the preparation timeout to set
     */
    void SetPrepTimeout(const Time& prepTimeout);

    /**
     * Get the preparation timeout
     *
     * @return the preparation timeout
     */
    Time GetPrepTimeout() const;

    /**
     * SMD Capabilities field structure
     * IEEE 802.11bn D2.0 Figure 9-aa44
     */
    struct SmdCapabilities
    {
        uint8_t dlDataForwarding : 1 {0};         ///< DL Data Forwarding field
        uint8_t maxNumOfPrepTargetApMlds : 3 {0}; ///< Max Number of Prepared Target AP MLDs field
        uint8_t smdType : 1 {0};                  ///< SMD Type field
        uint8_t ptkMode : 1 {0};                  ///< PTK Mode field
        uint8_t neighboringApProbingSupport : 1 {0}; ///< Neighboring AP Probing Support field

        /**
         * Serialize the SMD Capabilities field
         *
         * @param start iterator pointing to where the field should be written to
         */
        void Serialize(Buffer::Iterator& start) const;

        /**
         * Deserialize the SMD Capabilities field
         *
         * @param start iterator pointing to where the field should be read from
         * @return the number of octets read
         */
        uint16_t Deserialize(Buffer::Iterator start);

        /**
         * Get the serialized size of the SMD Capabilities field
         *
         * @return the serialized size in bytes of the SMD Capabilities field
         */
        uint16_t GetSerializedSize() const;
    };

    Mac48Address m_smdIdentifier;      ///< SMD Identifier field
    SmdCapabilities m_smdCapabilities; ///< SMD Capabilities field

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;

    Time m_prepTimeout; ///< Preparation timeout. The IE stores this duration as a count of 64 TU
                        ///< units (code points 0 and 1 are reserved)
};

} // namespace ns3

#endif /* SMD_INFORMATION_H */
