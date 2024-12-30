/*
 * Copyright (c) 2024 DERONNE SOFTWARE ENGINEERING
 *
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#ifndef UHR_CAPABILITIES_H
#define UHR_CAPABILITIES_H

#include "ns3/wifi-information-element.h"

namespace ns3
{

/**
 * UHR MAC Capabilities Info subfield (section 9.4.2.aa2.2 of 802.11bn/D0.2)
 */
struct UhrMacCapabilities
{
    uint8_t dpsSupport : 1 {0};            //!< DPS Support
    uint8_t dpsAssistingSupport : 1 {0};   //!< DPS Assisting Support
    uint8_t mlPowerManagement : 1 {0};     //!< Multi-Link Power Management
    uint8_t dsoSupported : 1 {0};          //!< DSO supported (not defined yet in 802.11bn/D0.2)
    uint8_t npcaSupported : 1 {0};         //!< NPCA Supported
    uint8_t bsrEnhancementSupport : 1 {0}; //!< BSR Enhancement Support

    /**
     * Get the size of the serialized UHR MAC capabilities subfield
     *
     * @return the size of the serialized UHR MAC capabilities subfield
     */
    uint16_t GetSize() const;

    /**
     * Serialize the UHR MAC capabilities subfield
     *
     * @param start iterator pointing to where the UHR MAC capabilities subfield should be written
     * to
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize the UHR MAC capabilities subfield
     *
     * @param start iterator pointing to where the UHR MAC capabilities subfield should be read from
     * @return the number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator start);
};

/**
 * UHR PHY Capabilities Info subfield.
 */
struct UhrPhyCapabilities
{
    /**
     * Get the size of the serialized UHR PHY capabilities subfield
     *
     * @return the size of the serialized UHR PHY capabilities subfield
     */
    uint16_t GetSize() const;

    /**
     * Serialize the UHR PHY capabilities subfield
     *
     * @param start iterator pointing to where the UHR PHY capabilities subfield should be written
     * to
     */
    void Serialize(Buffer::Iterator& start) const;

    /**
     * Deserialize the UHR PHY capabilities subfield
     *
     * @param start iterator pointing to where the UHR PHY capabilities subfield should be read from
     * @return the number of bytes read
     */
    uint16_t Deserialize(Buffer::Iterator start);
};

/**
 * @ingroup wifi
 *
 * The IEEE 802.11bn UHR Capabilities
 */
class UhrCapabilities : public WifiInformationElement
{
  public:
    WifiInformationElementId ElementId() const override;
    WifiInformationElementId ElementIdExt() const override;
    void Print(std::ostream& os) const override;

    UhrMacCapabilities m_macCapabilities; //!< UHR MAC Capabilities Info subfield
    UhrPhyCapabilities m_phyCapabilities; //!< UHR PHY Capabilities Info subfield

  private:
    uint16_t GetInformationFieldSize() const override;
    void SerializeInformationField(Buffer::Iterator start) const override;
    uint16_t DeserializeInformationField(Buffer::Iterator start, uint16_t length) override;
};

} // namespace ns3

#endif /* UHR_CAPABILITIES_H */
